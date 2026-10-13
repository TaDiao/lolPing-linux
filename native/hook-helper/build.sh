#!/bin/sh
# Native helper and shared decision tests; CXX/CPPFLAGS/LDFLAGS support local toolchains.
set -eu
cd "$(dirname "$0")"
mkdir -p build
case "$(uname -s)" in
  Darwin)
    CXX=${CXX:-clang++}
    FLAGS="-std=c++20 -arch arm64 -mmacosx-version-min=12.0 -Wall -Wextra"
    "$CXX" $FLAGS -O2 src/main_posix.cpp src/hooks_mac.cpp src/macinput.cpp src/simulate.cpp src/commands.cpp \
      src/output.cpp src/json.cpp src/decision.cpp -framework ApplicationServices -framework CoreFoundation -o build/hook-helper
    ;;
  Linux)
    CXX=${CXX:-c++}
    FLAGS="-std=c++20 -Wall -Wextra -pthread"
    "$CXX" $FLAGS ${CPPFLAGS:-} -O2 src/main_posix.cpp src/hooks_linux.cpp src/linuxinput.cpp src/simulate.cpp \
      src/commands.cpp src/output.cpp src/json.cpp src/decision.cpp ${LDFLAGS:-} -lX11 -lXi -lXtst -lXext -o build/hook-helper
    ;;
  *) echo "Unsupported operating system" >&2; exit 1 ;;
esac
"$CXX" $FLAGS ${CPPFLAGS:-} -O0 tests/helper_tests.cpp src/decision.cpp src/commands.cpp src/macinput.cpp src/json.cpp src/output.cpp \
  -o build/helper_tests
echo "Built build/hook-helper and build/helper_tests"
