# Pull Request Template

## Description
Brief summary of changes.

## Type of Change
- [ ] Bug fix
- [ ] New feature
- [ ] Refactoring
- [ ] Documentation
- [ ] Test improvement
- [ ] Build/CI

## Related Issue
Fixes #<issue_number>

## Testing
- [ ] `cmake --preset=release && ninja -C build` passes
- [ ] `cmake --preset=debug && ninja -C build-debug` passes
- [ ] `.\build-debug\bin\perft_test.exe` passes (ASan/UBSan clean)
- [ ] `.\build\bin\perft_test.exe` passes (Release)
- [ ] `.\build\bin\bench.exe` runs (if search/eval changed)
- [ ] Manual UCI smoke test: `echo "uci`nisready`nposition startpos`ngo depth 10`nquit" | .\build\bin\axiom.exe`

## Benchmarks (if applicable)
| Metric | Before | After | Delta |
|--------|--------|-------|-------|
| Perft nodes/sec | | | |
| Search NPS | | | |
| Eval speed | | | |

## Checklist
- [ ] Code follows C++20 standards and project conventions
- [ ] No heap allocation / virtual functions / exceptions in hot paths
- [ ] No UB (shifts ≥64, signed overflow, OOB access)
- [ ] Sanitizers clean in Debug build
- [ ] Documentation updated (README, STUDY_NOTES.md, comments)
- [ ] Commit messages follow convention

## Additional Notes
Anything else reviewers should know.