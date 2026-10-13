# Linux 0.5.1 (experimental)

This community build is based on [lolPing by Danrui Meng](https://github.com/darrenprx/lolPing) and the [Linux X11 port in PR #60](https://github.com/darrenprx/lolPing/pull/60). It includes a fix to link the X11 helper with `libXext` on Linux.

## Downloads

- `lolPing-0.5.1-amd64.deb`: Ubuntu/Debian x86-64. Install with `sudo apt install ./lolPing-0.5.1-amd64.deb`.
- `lolPing-0.5.1-x86_64.AppImage`: portable x86-64 build. Run `chmod +x lolPing-0.5.1-x86_64.AppImage` and then `./lolPing-0.5.1-x86_64.AppImage`.

A genuine X11/Xorg desktop session is required. Wayland and XWayland sessions are not supported. See [linux.md](linux.md) for desktop dependencies, known limits, and build instructions.

## Validation

- TypeScript typecheck passed.
- Native helper tests: 411 passed.
- Project tests: 716 passed, 1 skipped.
- X11 input integration test passed in a private Xvfb display.
- The packaged app started and remained running for 12 seconds in a private Xvfb display. It has not been visually checked on a physical desktop.

The source code retains the original MIT copyright notice. Riot Games assets are not covered by the MIT license.
