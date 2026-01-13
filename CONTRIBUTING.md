<!-- SPDX-License-Identifier: MIT
  Copyright (c) 2025 DagIR contributors -->
# Contributing to DagIR

Thank you for your interest in contributing! DagIR is an open-source, header-only C++20 library for external DAG traversal, IR generation, and multi-backend rendering. We welcome contributions from the community to make this project better.

---

## ✅ How to Contribute

### 1. **Fork and Clone**
- Fork the repository on GitHub.
- Clone your fork locally:
  ```bash
  git clone https://github.com/<your-username>/dagir.git
  cd dagir
  ```

### 2. **Branching**
- Create a feature branch:
  ```bash
  git checkout -b feature/my-awesome-change
  ```

### 3. **Coding Guidelines**
- Use **C++20** features (concepts, ranges, `std::span`).
# Contributing to DagIR

Thank you for your interest in contributing! DagIR is an open-source, header-only C++20 library for external DAG traversal, IR generation, and multi-backend rendering. We welcome contributions from the community to make this project better.

## How to Contribute

1. Fork the repository on GitHub and clone your fork locally:

```bash
git clone https://github.com/<your-username>/dagir.git
cd dagir
```

2. Create a feature branch:

```bash
git checkout -b feature/my-awesome-change
```

3. Coding Guidelines

- Use **C++20** features (concepts, ranges, `std::span`).
- Keep the library header-only.
- Follow modern C++ best practices: prefer `constexpr`, avoid raw pointers unless necessary, and use `std::ranges` for traversal.
- Keep code minimal and generic; use policy objects for customization and avoid backend-specific logic in algorithms.

4. Testing

- Add unit tests for new features.
- Use Catch2 (preferred) or GoogleTest.
- Ensure all tests pass locally using `ctest`.

5. Documentation

- Document public APIs with Doxygen-style comments.
- Update `README.md` for user-facing changes.

6. Commit Messages

- Use clear, conventional commit messages, for example:

```
feat: add Mermaid renderer
fix: correct topo_order cycle detection
docs: update README with new example
```

7. Pull Request

- Push your branch: `git push origin feature/my-awesome-change`
- Open a PR on GitHub with a clear description, linked issues, and tests/examples where appropriate.

## Code of Conduct

Please be respectful and constructive. We follow the Contributor Covenant: https://www.contributor-covenant.org/.

## Code Style & Formatting

We follow a project-wide C++ style to keep contributions consistent.

- A `.clang-format` file at the repo root defines the canonical formatting rules.
- Please run `clang-format -i` on changed C/C++ files before committing.
- The repository contains a `scripts/pre-commit` template that will be copied into `.git/hooks/pre-commit` during CMake configure (if this is a git repo).

Automated checks:

- A GitHub Actions workflow `/.github/workflows/format-and-cppcheck.yml` runs a formatting check and `cppcheck` on pushes and PRs.

If you prefer manual formatting, run:

```pwsh
clang-format -i <file1> <file2>
git add <file1> <file2>
```

## Code Coverage Policy

We maintain high code quality through comprehensive test coverage.

### Coverage Requirements

- **Coverage Badge**: The repository displays a coverage badge from [Coveralls](https://coveralls.io/github/Alan-Jowett/dagir?branch=main) on the README.
- **Coverage Gate**: Pull requests are automatically checked to ensure they do not reduce overall coverage by more than **2%**.
- **Coverage Reports**: Every push to `main` and every pull request generates a coverage report available as a workflow artifact.

### How Coverage Works

1. The `.github/workflows/coverage.yml` workflow runs on all PRs and pushes to main branches.
2. Coverage data is collected using `lcov` during test execution with `ENABLE_COVERAGE=ON`.
3. Results are uploaded to Coveralls for tracking and badge generation.
4. For pull requests, the workflow compares the PR's coverage against the base branch coverage.
5. If coverage decreases by more than 2%, the coverage gate fails and the PR cannot be merged.

### Best Practices

- **Add tests for new code**: Ensure all new features and bug fixes include appropriate unit tests.
- **Run tests locally**: Before submitting a PR, build and run tests with coverage enabled:
  ```bash
  cmake -S . -B build -DENABLE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug
  cmake --build build --config Debug --parallel
  ctest --test-dir build --output-on-failure
  ```
- **Review coverage reports**: Check the coverage HTML report in `build/coverage_html/` to identify untested code paths.
- **Address coverage decreases**: If the coverage gate fails, add tests to cover the new or modified code.

### Viewing Coverage Locally

After building with coverage enabled and running tests:

```bash
# Generate coverage report
cd build
lcov --capture --directory . --output-file coverage.info
lcov --remove coverage.info '/usr/*' '*/_deps/*' --output-file coverage_filtered.info

# Generate HTML report
genhtml coverage_filtered.info --output-directory coverage_html

# Open in browser (Linux)
xdg-open coverage_html/index.html

# Open in browser (macOS)
open coverage_html/index.html

# Open in browser (Windows)
start coverage_html/index.html
```

## Areas to Contribute

- Core algorithms: topo sort, postorder fold, cycle detection.
- IR layer: node/edge attributes, global graph metadata.
- Renderers: DOT, Mermaid, JSON.
- Adapters: TeDDy, CUDD, mock graphs.
- Examples & Docs: show real-world usage.

## Questions?

Open an issue at https://github.com/<your-org>/dagir/issues or start a discussion in GitHub Discussions.

Happy coding! 🚀
