# Contributing to Axiom

## Development Workflow

1. Fork & clone
2. Create feature branch: `git checkout -b feature/description`
3. Make changes with tests
4. Run `cmake --preset=debug && ninja -C build-debug && .\build-debug\bin\perft_test.exe`
5. Submit PR

## Code Standards

- **C++20** - use `std::array`, `enum class`, `constexpr`, structured bindings
- **No** heap allocation, virtual functions, exceptions, or `std::vector` in hot paths
- **Fixed-size** structures (MoveList with `std::array`)
- **Bit operations:** `std::popcount`, `std::countr_zero`, PEXT for sliding attacks
- **UB awareness:** no shifts ≥64, no signed overflow, bounds-checked arrays

## Testing

- Every change must pass `perft_test.exe` (once implemented)
- Debug build with ASan/UBSan must run clean
- Bench numbers in PR description for search/eval changes

## Commit Messages

```
<component>: <imperative summary>

<optional body explaining why>

Fixes #<issue>
```

## Architecture Notes

- Single-header modules where possible
- Mailbox + bitboards kept in sync
- Zobrist keys computed from fixed random tables
- History stack for repetition detection (1024 entries)

## License

By contributing, you agree your contributions are licensed under MIT.