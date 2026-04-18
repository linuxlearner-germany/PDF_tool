#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-cmake-build-debug}"

if [[ ! -f "${BUILD_DIR}/CMakeCache.txt" ]]; then
  ./scripts/configure.sh "${BUILD_DIR}"
fi

cmake --build "${BUILD_DIR}"
ctest --test-dir "${BUILD_DIR}" --output-on-failure
