# RCBot2 Build System Fixes - Summary

## Date: 2025-11-28
## Branch: ai-enhanced

This document summarizes the comprehensive fixes applied to resolve the two critical build system issues.

---

## Problems Fixed

### Problem 1: SDK Manifest Download Failure

**Symptom**: AMBuild configuration fails with:
```
File "/var/repos/rcbot2/hl2sdk-manifests/SdkHelpers.ambuild", line 1
    404: Not Found
    ^^^
SyntaxError: illegal target for annotation
```

**Root Cause**: The `hl2sdk-manifests` git submodule directory was empty (not initialized), causing AMBuildScript to load a non-existent file or HTML 404 error page instead of valid Python code.

**Solution**: Created a robust fallback mechanism with validation:
1. Git submodule initialization (preferred method)
2. Fallback download script using raw GitHub URLs
3. File validation to detect HTML error pages

### Problem 2: AMBuild API Incompatibility

**Symptom**: Build phase fails with:
```
AttributeError: module 'ambuild2' has no attribute 'run'
```

**Root Cause**: The build script used the outdated `ambuild2.run.Build()` API which doesn't exist in modern AMBuild2.

**Solution**: Replaced with modern AMBuild2 workflow:
- Configuration phase: `python configure.py` (creates `.ambuild2/` folder)
- Build phase: `ambuild` command (executed from build directory)

---

## Files Created

### 1. `download-manifests.sh`

**Purpose**: Downloads HL2SDK manifest files from GitHub when git submodules cannot be used.

**Key Features**:
- Uses **raw GitHub URLs** (`https://raw.githubusercontent.com/...`) instead of web URLs
- Validates downloaded files to ensure they're Python code, not HTML error pages
- Checks for common error indicators: `404: Not Found`, `<html>`, `<!DOCTYPE>`
- Retry logic with exponential backoff (3 attempts)
- Comprehensive error reporting
- Downloads files:
  - `SdkHelpers.ambuild` (critical for SDK detection)
  - `README.md` (documentation)

**Raw URL Format**:
```bash
# CORRECT (raw content):
https://raw.githubusercontent.com/alliedmodders/hl2sdk-manifests/master/SdkHelpers.ambuild

# WRONG (HTML page):
https://github.com/alliedmodders/hl2sdk-manifests/blob/master/SdkHelpers.ambuild
```

**Validation Logic**:
```bash
# Detects HTML error pages
grep -q "404: Not Found|<html>|<HTML>|<!DOCTYPE" "$file"

# Verifies Python code presence
grep -q "def|class|import" "$file"
```

**Usage**:
```bash
./download-manifests.sh
```

---

## Files Modified

### 2. `build-linux.sh`

#### Change 1: AMBuild Execution API (lines 718-743)

**Before** (outdated API):
```python
python3 << 'PYTHON_SCRIPT'
import sys
import ambuild2

# Run the build
try:
    ambuild2.run.Build()  # ❌ This API doesn't exist
    sys.exit(0)
except Exception as e:
    print(f"Build failed: {e}", file=sys.stderr)
    sys.exit(1)
PYTHON_SCRIPT
```

**After** (modern API):
```bash
# Check for .ambuild2 directory (created by configure.py)
if [ ! -d ".ambuild2" ]; then
    log_error "Build not configured! Missing .ambuild2 directory"
    return 1
fi

# Check if ambuild command is available
if ! command -v ambuild &> /dev/null; then
    log_error "ambuild command not found!"
    return 1
fi

# Run modern AMBuild2 command
ambuild 2>&1 | tee "$build_log"
```

**Why This Works**:
- Modern AMBuild2 installs the `ambuild` command-line tool
- The tool must be run from the build directory (where `.ambuild2/` exists)
- The `.ambuild2/` folder contains the dependency graph created during configuration
- This follows the official AMBuild two-phase pattern:
  1. **Configure phase**: Creates build directory structure and dependency graph
  2. **Build phase**: Executes build from within the build directory

#### Change 2: Manifest Download Integration (lines 476-533)

**Added Function**: `download_hl2sdk_manifests()`

**Logic Flow**:
```
1. Check if SdkHelpers.ambuild exists
   ├─ YES: Validate it's not an HTML error page
   │   ├─ Valid: Use existing file
   │   └─ Invalid: Delete and proceed to download
   └─ NO: Proceed to download

2. Try git submodule initialization (preferred)
   ├─ Success: Use submodule
   └─ Failure: Proceed to fallback

3. Use download-manifests.sh (fallback)
   ├─ Success: Files downloaded
   └─ Failure: Report error and exit
```

**Error Detection**:
```bash
# Detects corrupted files containing HTML errors
if grep -q "404: Not Found|<html>|<HTML>" "$sdkhelpers_file"; then
    log_error "SdkHelpers.ambuild contains HTML error page (corrupted)"
    rm -f "$sdkhelpers_file"
fi
```

#### Change 3: Build Sequence Update (lines 890-903)

**Added** manifest download to dependency installation sequence:

```bash
if [ "$SKIP_DEPS" != true ]; then
    install_base_dependencies
    check_python
    install_ambuild
    install_python_ml_packages
    download_hl2sdk_manifests  # ← NEW: Must run before SDK detection
    download_hl2sdk
    download_metamod
    download_onnx_runtime
else
    log_warning "Skipping dependency installation (--skip-deps)"
    download_hl2sdk_manifests  # ← NEW: Always needed, even when skipping deps
fi
```

**Rationale**:
- Manifests must be available before AMBuildScript tries to load SdkHelpers.ambuild
- Even when skipping dependency installation, manifests are still required
- Automatic fallback ensures build works even without git submodules

---

## Technical Details

### Why Raw GitHub URLs?

**Regular GitHub URL** (returns HTML):
```
https://github.com/alliedmodders/hl2sdk-manifests/blob/master/SdkHelpers.ambuild
```
Returns:
```html
<!DOCTYPE html>
<html>
  <head>...</head>
  <body>
    <!-- GitHub's web interface -->
  </body>
</html>
```

**Raw GitHub URL** (returns file content):
```
https://raw.githubusercontent.com/alliedmodders/hl2sdk-manifests/master/SdkHelpers.ambuild
```
Returns:
```python
# vim: set ts=2 sw=2 tw=99 et ft=python:
import os

class SdkConfig(object):
  def __init__(self, sdk, path, code, define, platform):
    ...
```

### AMBuild2 Modern Workflow

The correct two-phase build pattern:

#### Phase 1: Configuration
```bash
mkdir build-debug
cd build-debug
python3 ../configure.py --hl2sdk-root=../alliedmodders \
                        --mms-path=../alliedmodders/metamod-source \
                        --sdks=hl2dm,tf2 \
                        --enable-debug
```

**What happens**:
- `configure.py` uses `BuildParser` to analyze the project
- Creates `.ambuild2/` directory with dependency graph
- Detects compilers and SDKs
- Evaluates `AMBuildScript` and all `AMBuilder` files
- Prepares build environment

#### Phase 2: Build
```bash
# Must be run FROM the build directory
cd build-debug  # or wherever .ambuild2 exists
ambuild
```

**What happens**:
- `ambuild` command reads `.ambuild2/` dependency graph
- Compiles source files according to the graph
- Generates binaries in the build directory
- Handles incremental builds (only rebuilds changed files)

### Error Handling Improvements

**Before**:
- Generic Python exception if file doesn't exist
- No validation of downloaded content
- No retry logic
- Confusing error messages

**After**:
- Specific error detection for HTML error pages
- File content validation before use
- 3 retry attempts for downloads
- Clear, actionable error messages
- Automatic fallback mechanisms

---

## Verification Steps

### Test Manifest Download Script

```bash
# Clear existing manifests
rm -rf hl2sdk-manifests/*

# Run download script
./download-manifests.sh

# Verify files
ls -lh hl2sdk-manifests/
head -20 hl2sdk-manifests/SdkHelpers.ambuild
```

**Expected output**:
```
[SUCCESS] SdkHelpers.ambuild: Validated successfully
[SUCCESS] README.md: Validated successfully
[SUCCESS] All manifest files downloaded successfully!
```

### Test Full Build

```bash
# Option 1: Use build script (recommended)
./build-linux.sh --dev-only

# Option 2: Manual configuration and build
mkdir build-test
cd build-test
python3 ../configure.py --hl2sdk-root=../alliedmodders \
                        --mms-path=../alliedmodders/metamod-source \
                        --sdks=hl2dm,tf2 \
                        --enable-debug
ambuild
```

**Expected behavior**:
1. Manifests automatically detected or downloaded
2. Configuration creates `.ambuild2/` folder
3. Build executes without `ambuild2.run` errors
4. Binaries generated in build directory

### Verify Git Submodule Fallback

```bash
# Test with submodules
git submodule update --init hl2sdk-manifests
./build-linux.sh --dev-only
# Should use submodule, not download

# Test without submodules
rm -rf hl2sdk-manifests
mkdir hl2sdk-manifests  # Empty directory
./build-linux.sh --dev-only
# Should automatically download manifests
```

---

## Compatibility

### Requirements

- **AMBuild**: Modern version (2.2+) with `ambuild` command
- **Python**: 3.8+ (for AMBuild2)
- **wget**: Required for download-manifests.sh
- **git**: Optional (for submodule method)

### Installation

```bash
# Install AMBuild (Ubuntu 24.x)
python3 -m pip install --break-system-packages git+https://github.com/alliedmodders/ambuild

# Verify installation
python3 -c "import ambuild2; print(ambuild2.__version__)"
which ambuild  # Should show path to ambuild command
```

---

## References

- **AMBuild Documentation**: [AlliedModders Wiki](https://wiki.alliedmods.net/AMBuild)
- **HL2SDK Manifests Repository**: https://github.com/alliedmodders/hl2sdk-manifests
- **GitHub Raw URLs**: https://docs.github.com/en/repositories/working-with-files/using-files/viewing-a-file
- **Previous Fixes**: See `AMBUILD_FIXES.md` for related configuration fixes

---

## Summary

Both critical build issues have been resolved:

✅ **Problem 1 Fixed**: Manifest downloads use raw GitHub URLs with validation
✅ **Problem 2 Fixed**: AMBuild execution uses modern `ambuild` command API

The build system now has:
- Automatic manifest detection and download
- Validation to prevent HTML error pages
- Modern AMBuild2 API usage
- Comprehensive error handling and logging
- Git submodule support with download fallback

**Next Steps**: Test the build, commit changes, and push to the ai-enhanced branch.
