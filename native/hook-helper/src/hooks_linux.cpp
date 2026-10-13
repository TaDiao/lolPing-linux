// X11 backend. Passive grabs swallow wheel gestures; ReplayPointer preserves normal clicks.
// XI2 raw keyboard events observe modifiers without taking keyboard focus or reading /dev/input.
#include "app.h"
#include "commands.h"
#include "linuxinput.h"
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XTest.h>
#include <poll.h>
#include <unistd.h>
#include <array>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <set>
// Xlib macros collide with scoped enum members in the shared logic.
#undef None

namespace lp {
namespace {
uint64_t nowMs() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::steady_clock::now().time_since_epoch()).count();
}
int grabErrors = 0;
int xError(Display* d, XErrorEvent* e) {
  if (e->error_code == BadAccess) { ++grabErrors; return 0; }
  char message[256]; XGetErrorText(d, e->error_code, message, sizeof(message));
  std::cerr << "X11: " << message << '\n';
  return 0;
}
Btn button(unsigned int n) {
  switch(n) {
    case 1: return Btn::Left; case 2: return Btn::Middle; case 3: return Btn::Right;
    case 8: return Btn::X1; case 9: return Btn::X2; default: return Btn::None;
  }
}
unsigned int buttonNumber(Btn b) {
  switch(b) {
    case Btn::Left: return 1; case Btn::Middle: return 2; case Btn::Right: return 3;
    case Btn::X1: return 8; case Btn::X2: return 9; default: return 0;
  }
}
class Hooks {
 public:
  Display* display;
  Window root;
  Output& out;
  Decision decision;
  std::array<uint32_t, 256> vk{};
  std::array<bool, 256> swallowed{};
  bool suspended = false;
  bool quit = false;
  bool pointerActive = false;
  std::set<unsigned int> heldButtons;
  explicit Hooks(Display* d, Output& o): display(d), root(DefaultRootWindow(d)), out(o) {
    decision.setMaskMenuKeys(false);
    refreshKeys();
  }
  void refreshKeys() {
    XkbDescPtr map = XkbGetKeyboard(display, XkbAllComponentsMask, XkbUseCoreKbd);
    for (int code = 8; code < 256; ++code) {
      uint32_t physical = 0;
      if (map && map->names && map->names->keys && code >= map->min_key_code && code <= map->max_key_code)
        physical = x11::keyNameToVk(map->names->keys[code].name);
      vk[code] = physical ? physical : x11::keysymToVk(XkbKeycodeToKeysym(display, code, 0, 0));
    }
    if (map) XkbFreeKeyboard(map, XkbAllComponentsMask, True);
  }
  void grabButtons() {
    // Grab individually: AnyModifier would fail the whole grab if a window manager owns one chord.
    for (unsigned int b : {1u, 8u, 9u})
      for (unsigned int mods = 0; mods < 256; ++mods)
        XGrabButton(display, b, mods, root, False, ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                    GrabModeSync, GrabModeAsync, 0, 0);
  }
  void configureGrabs() {
    XUngrabButton(display, AnyButton, AnyModifier, root);
    XUngrabKey(display, AnyKey, AnyModifier, root);
    grabButtons();
    const Config& c = decision.config();
    const auto own = [&](uint32_t key, Trigger t, uint32_t custom) {
      return decision.enabled() && !suspended && t == Trigger::CustomVk && key == custom;
    };
    for (int code = 8; code < 256; ++code) {
      if (!vk[code]) continue;
      for (unsigned int mods = 0; mods < 256; ++mods) {
        const unsigned int chord = ((c.toggleMods & ModCtrl) ? ControlMask : 0) |
          ((c.toggleMods & ModAlt) ? Mod1Mask : 0) | ((c.toggleMods & ModShift) ? ShiftMask : 0) |
          ((c.toggleMods & ModWin) ? Mod4Mask : 0);
        if ((pointerActive && vk[code] == 0x1B) || own(vk[code], c.trigger, c.triggerVk) || own(vk[code], c.emoteTrigger, c.emoteTriggerVk) ||
            (!suspended && vk[code] == c.toggleVk && (mods & ~(LockMask | Mod2Mask)) == chord))
          XGrabKey(display, code, mods, root, False, GrabModeAsync, GrabModeSync);
      }
    }
    XSync(display, False);
    if (grabErrors) {
      std::cerr << "Some input chords are reserved by another application or window manager; choose a different trigger.\n";
      grabErrors = 0;
    }
  }
  void apply(const Result& r) {
    for (const auto& e : r.emits) out.emit(e);
    if (r.injects.empty()) return;
    // Temporarily remove our passive grabs under a server grab so replay never triggers another wheel.
    XGrabServer(display);
    // Release any physical-down state while OUR grab still receives the synthetic up.
    for (const auto& i : r.injects) {
      if (i.kind == InjectKind::ButtonDown && pointerActive)
        XTestFakeButtonEvent(display, buttonNumber(i.btn), False, CurrentTime);
    }
    XSync(display, False);
    XUngrabPointer(display, CurrentTime);
    pointerActive = false;
    heldButtons.clear();
    XUngrabButton(display, AnyButton, AnyModifier, root);
    for (const auto& i : r.injects) {
      const unsigned int n = buttonNumber(i.btn);
      if (!n) continue;
      if (i.kind == InjectKind::ButtonDown) {
        XTestFakeButtonEvent(display, n, True, CurrentTime);
      } else if (i.kind == InjectKind::ButtonUp) XTestFakeButtonEvent(display, n, False, CurrentTime);
    }
    XSync(display, False);
    grabButtons();
    XUngrabServer(display);
    XFlush(display);
  }
  void rawKey(int code, bool down) {
    if (code < 8 || code > 255 || !vk[code]) return;
    InputEvent e; e.kind = down ? EvKind::KeyDown : EvKind::KeyUp; e.vk = vk[code]; e.timeMs = nowMs();
    const bool before = decision.enabled();
    Result r = decision.onEvent(e);
    swallowed[code] = r.swallow;
    apply(r);
    if (before != decision.enabled()) configureGrabs();
  }
  void mouse(const XEvent& event) {
    InputEvent e; e.timeMs = nowMs();
    if (event.type == MotionNotify) {
      e.kind = EvKind::MouseMove; e.x = event.xmotion.x_root; e.y = event.xmotion.y_root;
    } else {
      e.kind = event.type == ButtonPress ? EvKind::MouseDown : EvKind::MouseUp;
      e.btn = button(event.xbutton.button); e.x = event.xbutton.x_root; e.y = event.xbutton.y_root;
    }
    const bool wasActive = pointerActive;
    if (event.type == ButtonPress) heldButtons.insert(event.xbutton.button);
    if (event.type == ButtonRelease) heldButtons.erase(event.xbutton.button);
    Result r = decision.onEvent(e);
    if (event.type == ButtonPress && !wasActive) {
      pointerActive = r.swallow;
      XAllowEvents(display, r.swallow ? AsyncPointer : ReplayPointer, event.xbutton.time);
      if (!r.swallow) heldButtons.clear();
      configureGrabs();
    }
    apply(r);
    if (event.type == ButtonRelease && heldButtons.empty()) {
      XUngrabPointer(display, event.xbutton.time);
      pointerActive = false;
      configureGrabs();
    }
  }
  void command(const std::string& line) {
    const Command c = parseCommand(line, decision.config());
    switch(c.kind) {
      case Command::Kind::Config: apply(decision.setConfig(c.config)); configureGrabs(); break;
      case Command::Kind::SetEnabled: apply(decision.setEnabled(c.flag)); configureGrabs(); break;
      case Command::Kind::Suspend: suspended = c.flag; apply(decision.setSuspended(c.flag)); configureGrabs(); break;
      case Command::Kind::Shutdown: quit = true; break;
      default: out.error("invalid command: " + line); break;
    }
  }
};
}
int runHooks(Output& out) {
  const char* session = std::getenv("XDG_SESSION_TYPE");
  if ((session && std::strcmp(session, "wayland") == 0) || std::getenv("WAYLAND_DISPLAY")) {
    out.error("noAccess", "Linux requires an X11 login session; XWayland cannot capture input from native Wayland apps.");
    return 3;
  }
  Display* display = XOpenDisplay(nullptr);
  if (!display) { out.error("noAccess", "Cannot connect to X11. Start lolPing inside an X11 desktop session."); return 3; }
  XSetErrorHandler(xError);
  int opcode, eventBase, errorBase, major = 2, minor = 1;
  if (!XQueryExtension(display, "XInputExtension", &opcode, &eventBase, &errorBase) ||
      XIQueryVersion(display, &major, &minor) != Success || (major == 2 && minor < 1) ||
      !XTestQueryExtension(display, &eventBase, &errorBase, &major, &minor)) {
    out.error("noAccess", "XInput 2.1 and XTest are required."); XCloseDisplay(display); return 3;
  }
  Hooks h(display, out);
  unsigned char bits[XIMaskLen(XI_LASTEVENT)]{};
  XISetMask(bits, XI_RawKeyPress); XISetMask(bits, XI_RawKeyRelease);
  XIEventMask mask{XIAllMasterDevices, sizeof(bits), bits};
  XISelectEvents(display, h.root, &mask, 1);
  Bool repeat; XkbSetDetectableAutoRepeat(display, True, &repeat);
  h.configureGrabs();
  char keys[32]; XQueryKeymap(display, keys);
  for (int code = 8; code < 256; ++code) if (keys[code/8] & (1 << (code%8))) h.rawKey(code, true);
  out.ready();
  std::string pending;
  while (!h.quit) {
    while (XPending(display)) {
      XEvent e; XNextEvent(display, &e);
      if (e.type == GenericEvent && e.xcookie.extension == opcode && XGetEventData(display, &e.xcookie)) {
        if (e.xcookie.evtype == XI_RawKeyPress || e.xcookie.evtype == XI_RawKeyRelease) {
          const auto* raw = static_cast<XIRawEvent*>(e.xcookie.data);
          h.rawKey(raw->detail, e.xcookie.evtype == XI_RawKeyPress);
        }
        XFreeEventData(display, &e.xcookie);
      } else if (e.type == KeyPress || e.type == KeyRelease) {
        XAllowEvents(display, h.swallowed[e.xkey.keycode] ? AsyncKeyboard : ReplayKeyboard, e.xkey.time);
      } else if (e.type == ButtonPress || e.type == ButtonRelease || e.type == MotionNotify) h.mouse(e);
      else if (e.type == MappingNotify) { XRefreshKeyboardMapping(&e.xmapping); h.refreshKeys(); h.configureGrabs(); }
    }
    h.apply(h.decision.tick(nowMs()));
    XFlush(display);
    pollfd fds[] = {{ConnectionNumber(display), POLLIN, 0}, {STDIN_FILENO, POLLIN, 0}};
    if (poll(fds, 2, 25) < 0) break;
    if (fds[1].revents & (POLLIN | POLLHUP | POLLERR)) {
      char buf[4096]; const ssize_t count = read(STDIN_FILENO, buf, sizeof(buf));
      if (count <= 0) break;
      pending.append(buf, count);
      size_t end;
      while ((end = pending.find('\n')) != std::string::npos) {
        h.command(pending.substr(0, end)); pending.erase(0, end + 1);
        if (h.quit) break;
      }
      if (pending.size() > 65536) { out.error("command too long"); break; }
    }
  }
  XUngrabPointer(display, CurrentTime); XUngrabKeyboard(display, CurrentTime);
  XCloseDisplay(display);
  return 0;
}
}
