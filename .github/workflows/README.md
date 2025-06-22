# GitHub Actions Workflows

This directory contains the CI/CD workflows for the Game Engine project.

## Workflows

### 1. CI Workflow (`ci.yml`)
Continuous Integration pipeline that runs on every push and pull request.

**Features:**
- Multi-platform builds (Ubuntu, macOS, Windows)
- Dependency caching for faster builds
- Comprehensive test suite execution
- Build artifact uploads

**Triggers:**
- Push to: `master`, `main`, `develop`, `feature/*`
- Pull requests to: `master`, `main`, `develop`

### 2. Code Quality Workflow (`ci.yml` - code-quality job)
Static analysis and code formatting checks.

**Tools:**
- **clang-format**: Code style enforcement
- **clang-tidy**: Static analysis and modernization suggestions
- **cppcheck**: Additional static analysis

**Configuration files:**
- `.clang-format`: Code style rules
- `.clang-tidy`: Static analysis checks

### 3. Release Workflow (`release.yml`)
Automated release creation when pushing version tags.

**Features:**
- Triggered by tags matching `v*.*.*` pattern
- Multi-platform binary builds
- Automatic GitHub Release creation
- Platform-specific archives (tar.gz for Unix, zip for Windows)

**Creating a release:**
```bash
git tag v1.0.0
git push origin v1.0.0
```

## Usage

### Running Workflows Locally
You can test workflows locally using [act](https://github.com/nektos/act):

```bash
# Install act
brew install act  # macOS
# or
curl https://raw.githubusercontent.com/nektos/act/master/install.sh | sudo bash  # Linux

# Run CI workflow
act -W .github/workflows/ci.yml

# Run specific job
act -W .github/workflows/ci.yml -j build

# Run with specific event
act push -W .github/workflows/ci.yml
```

### Caching Strategy
The workflows use multi-layer caching to optimize build times:

1. **pip cache**: Python dependencies
2. **CMake cache**: Build configuration and dependencies
3. **ccache**: Compiled object files

Cache keys are based on file hashes to ensure freshness:
- `${{ runner.os }}-pip-${{ hashFiles('**/requirements.txt') }}`
- `${{ runner.os }}-cmake-${{ hashFiles('**/CMakeLists.txt') }}`
- `${{ runner.os }}-ccache-${{ github.sha }}`

## Troubleshooting

### Common Issues

**1. Build failures on Windows**
- Ensure MinGW is properly installed via Chocolatey
- Check that the MinGW bin directory is in PATH

**2. Cache misses**
- Caches expire after 7 days of inactivity
- Modifying CMakeLists.txt invalidates CMake cache
- Force cache refresh by incrementing cache key version

**3. Test failures**
- Check test logs in artifacts
- Run tests locally: `make all-tests`
- Individual test categories: `make test-unit`, `make test-integration`

**4. Code quality failures**
- Format code locally: `clang-format -i src/**/*.cpp`
- Run clang-tidy: `clang-tidy src/*.cpp -- -std=c++17`
- Suppress false positives with inline comments

### Debugging Workflows

**View workflow logs:**
1. Go to Actions tab in GitHub
2. Click on the workflow run
3. Expand job steps to see detailed logs

**Download artifacts:**
1. In the workflow run summary
2. Scroll to "Artifacts" section
3. Download test results, logs, or binaries

**Re-run failed jobs:**
- Click "Re-run failed jobs" in the workflow run
- Or re-run all jobs for a clean state

## Best Practices

1. **Keep workflows DRY**: Use matrix strategies for multi-platform builds
2. **Cache aggressively**: But ensure cache keys change when dependencies do
3. **Fail fast**: Use `continue-on-error: false` for critical steps
4. **Upload artifacts**: Always upload logs and test results for debugging
5. **Version your actions**: Use specific versions (e.g., `@v3`) not `@latest`

## Contributing

When modifying workflows:
1. Test changes locally with `act` when possible
2. Create a pull request to test workflow changes
3. Monitor the Actions tab for any failures
4. Update this documentation if adding new workflows