#!/usr/bin/env bash
set -euo pipefail

# DigitalOcean/AWS/Ubuntu bootstrap script for simditoa benchmarks.
#
# Detects AVX-512 IFMA + VBMI support on the host CPU and enables the
# AVX-512 code path automatically when available. Falls back to a
# portable scalar build otherwise.
#
# Usage:
#   curl -fsSL https://gist.githubusercontent.com/hirebarend/357f646d2f642a0e3d7ed233abb550a0/raw/script.sh | bash
#
# Optional environment overrides:
#   REPO_URL=https://github.com/simditoa/simditoa.git
#   BRANCH=main
#   WORKDIR=${HOME}/simditoa-benchmark
#   BUILD_TYPE=Release
#   EXTRA_CXX_FLAGS=""    # overrides auto-detected -march flags
#   FORCE_AVX512=0        # set to 1 to force AVX-512 flags even if cpuinfo lookup fails
#
# Example:
#   curl -fsSL https://gist.githubusercontent.com/<user>/<gist>/raw/script.sh \
#     | BRANCH=main bash

REPO_URL="${REPO_URL:-https://github.com/simditoa/simditoa.git}"
BRANCH="${BRANCH:-main}"
WORKDIR="${WORKDIR:-${HOME}/simditoa-benchmark}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
EXTRA_CXX_FLAGS="${EXTRA_CXX_FLAGS:-}"
FORCE_AVX512="${FORCE_AVX512:-0}"

if [[ "$(uname -s)" != "Linux" ]]; then
  echo "This bootstrap script is intended for Linux hosts." >&2
  exit 1
fi

if ! command -v apt-get >/dev/null 2>&1; then
  echo "This bootstrap script expects an apt-based distro such as Ubuntu." >&2
  exit 1
fi

if [[ "${EUID}" -eq 0 ]]; then
  SUDO=()
else
  SUDO=(sudo)
fi

echo "==> Installing build dependencies..."
"${SUDO[@]}" apt-get update
"${SUDO[@]}" apt-get install -y --no-install-recommends \
  ca-certificates \
  build-essential \
  cmake \
  git

# ---- Detect AVX-512 IFMA + VBMI support --------------------------------------
#
# simditoa's AVX-512 path is gated on __AVX512IFMA__ being defined at compile
# time, which only happens when the compiler is told to target IFMA + VBMI.
# We probe /proc/cpuinfo and add the right -m flags when the host supports it.

HAS_AVX512_IFMA=0
HAS_AVX512_VBMI=0
if grep -q -E '^flags\s*:.*\bavx512ifma\b' /proc/cpuinfo 2>/dev/null; then
  HAS_AVX512_IFMA=1
fi
if grep -q -E '^flags\s*:.*\bavx512vbmi\b' /proc/cpuinfo 2>/dev/null; then
  HAS_AVX512_VBMI=1
fi

AUTO_FLAGS=""
if [[ -z "${EXTRA_CXX_FLAGS}" ]]; then
  if [[ "${HAS_AVX512_IFMA}" -eq 1 && "${HAS_AVX512_VBMI}" -eq 1 ]] || [[ "${FORCE_AVX512}" -eq 1 ]]; then
    AUTO_FLAGS="-mavx512f -mavx512dq -mavx512bw -mavx512vl -mavx512ifma -mavx512vbmi"
    echo "==> Detected AVX-512 IFMA + VBMI: enabling AVX-512 build (${AUTO_FLAGS})"
  else
    echo "==> AVX-512 IFMA + VBMI not detected: building scalar fallback only"
    echo "    (set FORCE_AVX512=1 to override, or pass EXTRA_CXX_FLAGS explicitly)"
  fi
else
  echo "==> Using user-provided EXTRA_CXX_FLAGS=${EXTRA_CXX_FLAGS}"
fi

CXX_FLAGS="${EXTRA_CXX_FLAGS:-${AUTO_FLAGS}}"

echo "==> Preparing workspace: ${WORKDIR}"
mkdir -p "${WORKDIR}"
cd "${WORKDIR}"

if [[ -d simditoa/.git ]]; then
  echo "==> Updating existing repository..."
  git -C simditoa fetch --all --prune
  git -C simditoa switch "${BRANCH}"
  git -C simditoa pull --ff-only
else
  echo "==> Cloning ${REPO_URL} (${BRANCH})..."
  git clone --branch "${BRANCH}" --single-branch "${REPO_URL}" simditoa
fi

cd simditoa

# Wipe stale build directory so previously-cached CMAKE_CXX_FLAGS don't linger
# across re-runs of this script.
if [[ -d build ]]; then
  echo "==> Removing previous build directory to apply fresh flags..."
  rm -rf build
fi

echo "==> Configuring benchmark build (BUILD_TYPE=${BUILD_TYPE}, CXX_FLAGS='${CXX_FLAGS}')..."
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
  -DSIMDITOA_DEVELOPER_MODE=ON \
  -DSIMDITOA_BUILD_BENCHMARKS=ON \
  -DCMAKE_CXX_FLAGS="${CXX_FLAGS}"

echo "==> Building benchmark..."
cmake --build build --config "${BUILD_TYPE}" --target simditoa_benchmark

echo "==> Running benchmark..."
./build/benchmarks/simditoa_benchmark
