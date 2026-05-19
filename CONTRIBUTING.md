# Contributing to simditoa

Thank you for your interest in contributing to **simditoa**, a SIMD-accelerated integer-to-string conversion library. We welcome bug reports, feature suggestions, and pull requests.

## Development Setup

```bash
cmake -B build -DSIMDITOA_DEVELOPER_MODE=ON
cmake --build build
ctest --test-dir build
```

Benchmarks live in a separate repository: [simditoa/benchmarks](https://github.com/simditoa/benchmarks).

## Coding Guidelines

- C++17 minimum
- Follow the `.clang-format` configuration (LLVM style base)
- Keep implementations minimal and focused
- Comment non-obvious algorithmic choices with paper references where applicable

## Pull Request Process

1. Fork the repository and create a feature branch.
2. Ensure all tests pass (`ctest --test-dir build`).
3. Keep changes focused — one logical change per PR.
4. Add tests for new functionality or bug fixes.

## Architecture

The library uses compile-time implementation selection:

- `src/avx512.cpp` — AVX-512 IFMA+VBMI (Intel Ice Lake+, AMD Zen 4+)
- `src/fallback.cpp` — Portable scalar implementation

Only the best available implementation is compiled for the target platform.

## License

By contributing, you agree that your contributions will be licensed under the same dual MIT/Apache-2.0 license as the project.
