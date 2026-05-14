# Contributing to silicon-net

## Development Workflow

### Branching
- `main` — stable, all tests pass
- `milestone/N-description` — feature branches per milestone
- PRs require passing CI

### Build & Test (C++)
```bash
cmake -B build -DBUILD_TESTS=ON
cmake --build build -j$(nproc)
cd build && ctest --output-on-failure
```

### Python (NOS layer)
```bash
cd nos/
pip install -r requirements.txt
python -m pytest tests/ -v
```

### Code Style
- C++: clang-format (Google style, 100 col)
- Python: ruff (default config)

### Milestone Workflow
1. Read SPEC.md for current milestone requirements
2. Implement bottom-up (tests first where possible)
3. Each component must be testable in isolation before integration
4. Update README status table when milestone completes

### Commit Messages
```
[layer] brief description

layer = asic | sdk | sai | nos | tools | ci | docs
Example: [asic] implement FDB table add/remove/lookup
```
