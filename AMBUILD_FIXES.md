# AMBuild Configuration Fixes - Summary

## Date: 2025-11-23
## Branch: ai-enhanced

This document summarizes all critical fixes applied to the AMBuild configuration based on official AMBuild documentation (docs/ambuild.pdf and docs/ambuild-tutorial.pdf).

---

## Critical Issues Identified and Fixed

### 1. **configure.py - Incorrect Build Directory Handling**

**Issue**: configure.py was incorrectly trying to create and manage the build directory within the source tree.

**Location**: `configure.py` lines 5-6

**Problem**:
```python
build_dir = os.path.join(sys.path[0], 'post')
os.makedirs(build_dir, exist_ok=True)
```

**Fix**: Removed these lines. According to the AMBuild Tutorial (page 2), the build directory is determined by the current working directory when configure.py is executed, NOT by a hardcoded path in the script.

**Correct Workflow** (from AMBuild docs):
1. Create build directory: `mkdir build`
2. Change to build directory: `cd build`
3. Run configure from there: `python ../configure.py`

The BuildParser automatically uses `pwd` as the build directory.

**Files Modified**: `configure.py`

---

### 2. **AMBuildScript - Duplicate Import Statement**

**Issue**: Redundant `import os` statement at line 285.

**Location**: `AMBuildScript` line 285

**Problem**: The file already imports os at line 1 (`import os, sys`), and line 285 had a duplicate `import os` which is unnecessary and poor practice.

**Fix**: Removed the duplicate import statement.

**Files Modified**: `AMBuildScript`

---

### 3. **build-linux.sh - Incorrect Dependency Paths**

**Issue**: The build script was trying to use a non-existent `dependencies/` directory instead of the git submodules in `alliedmodders/`.

**Location**: `build-linux.sh` lines 621-622

**Problem**:
```bash
local hl2sdk_root="${DEPS_DIR}/hl2sdk"
local mms_path="${DEPS_DIR}/metamod-source"
```

This project uses git submodules, not a separate dependencies directory. The paths were pointing to locations that don't exist.

**Fix**: Updated paths to use git submodules:
```bash
local hl2sdk_root="${SCRIPT_DIR}/alliedmodders"
local mms_path="${SCRIPT_DIR}/alliedmodders/metamod-source"
```

**Rationale**: Looking at `.gitmodules`, all dependencies are defined as submodules:
- `alliedmodders/hl2sdk-hl2dm`
- `alliedmodders/hl2sdk-tf2`
- `alliedmodders/metamod-source`
- `hl2sdk-manifests`

The AMBuildScript's `findSdkPath()` function (line 213) expects:
```python
sdk_path = os.path.join(builder.options.hl2sdk_root, dir_name)
```

Where `dir_name = 'hl2sdk-{sdk_name}'`, so passing `alliedmodders` as the root will correctly find `alliedmodders/hl2sdk-hl2dm`, etc.

**Files Modified**: `build-linux.sh`

---

### 4. **Git Submodules - Missing Initialization**

**Issue**: Critical git submodules were not initialized, causing "404: Not Found" errors when AMBuild tried to load `SdkHelpers.ambuild`.

**Submodules Initialized**:
1. `hl2sdk-manifests` - Contains `SdkHelpers.ambuild` (critical for SDK detection)
2. `alliedmodders/metamod-source` - Metamod:Source framework
3. `alliedmodders/hl2sdk-hl2dm` - Half-Life 2 Deathmatch SDK
4. `alliedmodders/hl2sdk-tf2` - Team Fortress 2 SDK

**Error from logs**:
```
File "/var/repos/rcbot2/hl2sdk-manifests/SdkHelpers.ambuild", line 1
    404: Not Found
    ^^^
SyntaxError: illegal target for annotation
```

This was because the `hl2sdk-manifests` directory was empty (submodule not initialized), so the file didn't exist.

**Fix**: Ran `git submodule update --init` for all required submodules.

---

## Summary of Changes

### Files Modified:
1. **configure.py**
   - Removed incorrect build_dir creation
   - Added explanatory comment about AMBuild's build directory detection

2. **AMBuildScript**
   - Removed duplicate `import os` statement

3. **build-linux.sh**
   - Changed `hl2sdk_root` from `${DEPS_DIR}/hl2sdk` to `${SCRIPT_DIR}/alliedmodders`
   - Changed `mms_path` from `${DEPS_DIR}/metamod-source` to `${SCRIPT_DIR}/alliedmodders/metamod-source`
   - Updated error messages to guide users to initialize submodules

### Git Operations:
- Initialized `hl2sdk-manifests` submodule
- Initialized `alliedmodders/metamod-source` submodule
- Initialized `alliedmodders/hl2sdk-hl2dm` submodule
- Initialized `alliedmodders/hl2sdk-tf2` submodule

---

## AMBuild Best Practices Applied

Based on the official AMBuild documentation:

1. **Build Directory Management** (Tutorial p.2):
   - Build directory is determined by CWD when running configure.py
   - Do not hardcode build paths in configure.py
   - Workflow: `mkdir build && cd build && python ../configure.py`

2. **Source Path** (Tutorial p.1):
   - Use `run.BuildParser(sourcePath=sys.path[0], api='2.2')`
   - `sys.path[0]` points to the directory containing configure.py

3. **Dependency Graph** (Main docs p.3):
   - AMBuild stores dependency graph in `.ambuild2/` folder inside the **build path**
   - Build artifacts should be in build directory, not source directory

4. **SDK Detection**:
   - Use `--hl2sdk-root` to specify parent directory containing `hl2sdk-{name}` subdirectories
   - AMBuildScript's `findSdkPath()` automatically appends `hl2sdk-{sdk_name}` to the root

---

## Verification Steps

To verify the fixes:

1. Ensure all submodules are initialized:
   ```bash
   git submodule update --init alliedmodders/hl2sdk-hl2dm \
                               alliedmodders/hl2sdk-tf2 \
                               alliedmodders/metamod-source \
                               hl2sdk-manifests
   ```

2. Create build directory and configure:
   ```bash
   mkdir -p build-debug
   cd build-debug
   python3 ../configure.py --hl2sdk-root=../alliedmodders \
                           --mms-path=../alliedmodders/metamod-source \
                           --sdks=hl2dm,tf2 \
                           --enable-debug
   ```

3. Build:
   ```bash
   ambuild
   ```

Or use the build-linux.sh script (now fixed):
```bash
./build-linux.sh --dev-only
```

---

## References

- AMBuild Documentation: `docs/ambuild.pdf`
- AMBuild Tutorial: `docs/ambuild-tutorial.pdf`
- AlliedModders Wiki: https://wiki.alliedmods.net/AMBuild
- Git Submodules: `.gitmodules`

---

## Notes

- The build-linux.sh script's dependency download functions (lines 393-473) are now redundant since we use git submodules
- Future improvement: Could remove or conditionally disable the download functions in build-linux.sh
- The `post/` directory that was being created by the old configure.py should be removed if it exists
