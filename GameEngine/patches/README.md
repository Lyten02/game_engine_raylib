# Patches Directory

This directory contains patches that are applied to third-party dependencies during the build process.

## Current Patches

### raylib-cmake-fix.patch
- **Purpose**: Updates raylib's minimum CMake version from 3.5 to 3.11
- **Reason**: Modern CMake (3.28+) has removed compatibility with versions < 3.5
- **Affected platforms**: Primarily macOS with latest CMake
- **Applied to**: raylib 5.0

## How Patches Are Applied

Patches are automatically applied during the CMake configuration phase using the `apply-patches.cmake` script.

The script:
1. Copies the patch file to the source directory
2. Attempts to apply the patch using:
   - `patch -p1` on Unix-like systems (Linux, macOS)
   - `git apply` on Windows (if Git is available)
3. Handles cases where patches are already applied

## Adding New Patches

1. Create a patch file using `git diff` or `diff -u`
2. Place it in this directory with a descriptive name
3. Update `apply-patches.cmake` to include the new patch
4. Test on all supported platforms