#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="build"
BUILD_TYPE="${BUILD_TYPE:-Release}"

usage() {
  echo "Usage: ./script.sh <command>"
  echo ""
  echo "Commands:"
  echo "  test        Configure, build, and run tests"
  echo "  benchmark   Configure, build, and run benchmarks"
  echo "  clean       Remove the build directory"
  echo ""
  echo "Environment variables:"
  echo "  BUILD_TYPE  CMake build type (default: Release)"
}

cmd_test() {
  echo "==> Configuring (${BUILD_TYPE})..."
  cmake -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DSIMDITOA_DEVELOPER_MODE=ON

  echo "==> Building..."
  cmake --build "${BUILD_DIR}" --config "${BUILD_TYPE}"

  echo "==> Running tests..."
  ctest --test-dir "${BUILD_DIR}" --output-on-failure -C "${BUILD_TYPE}"
}

cmd_benchmark() {
  echo "==> Configuring (${BUILD_TYPE})..."
  cmake -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DSIMDITOA_DEVELOPER_MODE=ON \
    -DSIMDITOA_BUILD_BENCHMARKS=ON

  echo "==> Building..."
  cmake --build "${BUILD_DIR}" --config "${BUILD_TYPE}" --target simditoa_benchmark

  echo "==> Running benchmark..."
  "${BUILD_DIR}/benchmarks/simditoa_benchmark"
}

cmd_clean() {
  echo "==> Removing ${BUILD_DIR}/"
  rm -rf "${BUILD_DIR}"
}

if [[ $# -lt 1 ]]; then
  usage
  exit 1
fi

case "$1" in
  test)      cmd_test ;;
  benchmark) cmd_benchmark ;;
  clean)     cmd_clean ;;
  *)         usage; exit 1 ;;
esac
