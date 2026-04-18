#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-cmake-build-debug}"

if [[ -n "${QT6_PREFIX:-}" ]]; then
  cmake -S . -B "${BUILD_DIR}" -DCMAKE_PREFIX_PATH="${QT6_PREFIX}"
  exit 0
fi

if [[ -n "${CMAKE_PREFIX_PATH:-}" ]]; then
  cmake -S . -B "${BUILD_DIR}"
  exit 0
fi

cat <<'EOF'
Qt6 was not located automatically.

Set one of these before running:
  export QT6_PREFIX=/path/to/Qt/6.x/gcc_64
  export CMAKE_PREFIX_PATH=/path/to/Qt/6.x/gcc_64

Then run:
  ./scripts/configure.sh
EOF

exit 1
