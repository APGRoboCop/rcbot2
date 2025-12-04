# Build Environment Fixes

## Problem Summary

When syncing commits from the successful build environment to your local machine (ai-enhanced branch), the build fails during configuration even though:
- ✓ You're synced to the correct branch
- ✓ You have the correct commit hash
- ✓ The build works in the container/CI environment

## Root Causes Identified

### 1. **Missing hl2sdk-manifests Submodule** (CRITICAL)
```
FileNotFoundError: [Errno 2] No such file or directory:
  '/var/repos/rcbot2/hl2sdk-manifests/manifests'
```

**Issue**: The `hl2sdk-manifests` git submodule is defined in `.gitmodules` but:
- Not initialized in `.git/config`
- The directory exists but is completely empty
- The AMBuildScript requires `SdkHelpers.ambuild` from this submodule

**Why it happens**: Git submodules don't sync automatically when you pull commits. Your git proxy at `127.0.0.1:61614` may also interfere with submodule initialization.

### 2. **Missing 32-bit Development Libraries**
```
Failed Checks:
  - linux-libc-dev:i386
```

**Issue**: The build requires 32-bit development headers for cross-compilation.

**Why it matters**: Even though you're on a 64-bit system, RCBot2 builds 32-bit binaries for compatibility with Source engine servers.

### 3. **Git Submodule Configuration Mismatch**
```bash
git submodule status
# Shows: fatal: no submodule mapping found in .gitmodules for path 'dependencies/hl2sdk/hl2sdk-css'
```

**Issue**: There's a corrupted submodule reference in git's internal state that doesn't match `.gitmodules`.

### 4. **Uninitialized AlliedModders Submodules**
The following submodules show as uninitialized (leading `-` in status):
- `alliedmodders/metamod-source`
- `alliedmodders/sourcemod` (CRITICAL for SourceMod extension build)
- `alliedmodders/hl2sdk-tf2`
- `alliedmodders/hl2sdk-css`
- `alliedmodders/hl2sdk-dods`
- etc.

### 5. **Missing SourceMod Headers** (COMPILATION PHASE)
```
fatal error: IExtensionSys.h: No such file or directory
```

**Issue**: When building with SourceMod integration (default), the build requires headers from the `alliedmodders/sourcemod` submodule. If this submodule is not initialized, compilation fails when trying to include SourceMod extension headers.

**Files affected**:
- `sm_ext/bot_sm_ext.h` - Includes `<IExtensionSys.h>` and `<smsdk_config.h>`
- `sm_ext/bot_sm_forwards.h` - Includes SourceMod forward system headers
- All files in `sm_ext/` directory

**Why it matters**: The RCBot2 build includes SourceMod extension integration by default, which provides advanced scripting capabilities. This requires the SourceMod public headers to be available.

### 6. **Missing SourcePawn VM Headers** (COMPILATION PHASE - NESTED SUBMODULES)
```
fatal error: sp_vm_types.h: No such file or directory
```

**Issue**: The `alliedmodders/sourcemod` submodule has its own nested submodules, including `sourcepawn`. The header `sp_vm_types.h` is located in `alliedmodders/sourcemod/sourcepawn/include/`. If the sourcemod submodule is initialized but its nested submodules are not, these headers will be missing.

**SourceMod nested submodules**:
- `sourcepawn` - SourcePawn VM and compiler (CRITICAL - contains sp_vm_types.h, sp_vm_api.h)
- `public/amtl` - AlliedModders Template Library
- `public/safetyhook` - SafetyHook library
- `hl2sdk-manifests` - SDK manifest files

**Why it happens**: Using `git submodule update --init` without `--recursive` only initializes the top-level submodule, not its nested submodules.

**Solution**: Initialize sourcemod with `--recursive` flag:
```bash
git submodule update --init --recursive alliedmodders/sourcemod
```

## The Fix Script

The `fix-build-environment.sh` script addresses all these issues:

### What It Does

1. **Syncs git submodules**: Re-registers submodules from `.gitmodules` to `.git/config`

2. **Initializes hl2sdk-manifests**:
   - Tries `git submodule update --init hl2sdk-manifests`
   - Falls back to `download-manifests.sh` if git fails (proxy issues)

3. **Installs 32-bit dependencies**:
   - Adds i386 architecture
   - Installs `linux-libc-dev:i386`

4. **Cleans up corrupted references**: Removes broken submodule paths from git cache

5. **Initializes AlliedModders submodules**: Includes metamod, sourcemod (required), and hl2sdk submodules
   - Uses `--recursive` for sourcemod to initialize nested submodules (sourcepawn, amtl, etc.)

6. **Cleans build artifacts**: Forces a fresh configure

7. **Verifies environment**: Comprehensive checks before you attempt to build

### Usage

```bash
# Interactive mode (recommended first time)
./fix-build-environment.sh

# Auto-fix everything without prompts
./fix-build-environment.sh --auto

# Skip sudo commands (if you don't have sudo)
./fix-build-environment.sh --no-sudo
```

## Why Your Build Fails vs Container Build Succeeds

| Aspect | Your Machine | Container Environment |
|--------|-------------|----------------------|
| Submodules | Not initialized (git doesn't auto-sync) | Pre-initialized or scripted setup |
| i386 libs | Missing (not in base install) | Included in base image |
| Git proxy | Local proxy may block submodules | Direct git access |
| Build artifacts | May have stale cache | Clean build every time |

## Manual Steps (If Script Fails)

If the fix script doesn't work, try these manual steps:

### Step 1: Fix hl2sdk-manifests
```bash
# Option A: Via git submodule
git submodule sync
git submodule update --init --recursive hl2sdk-manifests

# Option B: Via download script (if git fails)
bash download-manifests.sh

# Verify
ls -la hl2sdk-manifests/
# Should see: SdkHelpers.ambuild, README.md
```

### Step 2: Install 32-bit dependencies
```bash
sudo dpkg --add-architecture i386
sudo apt-get update
sudo apt-get install -y linux-libc-dev:i386
```

### Step 3: Clean and rebuild
```bash
rm -rf build/debug build/release
python3 configure.py --enable-optimize
ambuild build/debug
```

## After Running Fixes

Once the environment is fixed:

```bash
# Option 1: Use the build script
./build-linux.sh --skip-deps

# Option 2: Manual configure and build
python3 configure.py --enable-optimize
ambuild build/debug

# Check logs if it fails
cat build-logs/errors-debug-*.log
```

## Preventing Future Issues

### When pulling new commits:

```bash
# Always update submodules after pulling
git pull origin ai-enhanced
git submodule update --init --recursive
```

### Or use a combined command:

```bash
git pull --recurse-submodules origin ai-enhanced
```

### Add to your git config:

```bash
# Auto-update submodules on pull
git config submodule.recurse true
```

## Common Errors and Solutions

### Error: "FileNotFoundError: hl2sdk-manifests/manifests"
**Fix**: Run `./fix-build-environment.sh` or `bash download-manifests.sh`

### Error: "Compiler clang for CC failed"
**Not a problem**: This is just testing compilers. It finds gcc afterwards.

### Error: "linux-libc-dev:i386 not installed"
**Fix**:
```bash
sudo dpkg --add-architecture i386
sudo apt-get update
sudo apt-get install linux-libc-dev:i386
```

### Error: "fatal: no submodule mapping found"
**Fix**:
```bash
git rm --cached dependencies/hl2sdk/hl2sdk-css
git submodule sync
```

### Error: "fatal error: IExtensionSys.h: No such file or directory"
**Fix**: Initialize the sourcemod submodule
```bash
git submodule update --init --depth=1 alliedmodders/sourcemod
# Or run the full fix script
./fix-build-environment.sh
```

### Error: "fatal error: sp_vm_types.h: No such file or directory"
**Fix**: Initialize sourcemod with nested submodules (recursive)
```bash
# The sourcemod submodule has its own nested submodules
git submodule update --init --recursive alliedmodders/sourcemod
# Or run the full fix script (now handles recursive init)
./fix-build-environment.sh
```

**Why this happens**: The `sp_vm_types.h` header is in `alliedmodders/sourcemod/sourcepawn/include/`. Even if sourcemod is initialized, its nested `sourcepawn` submodule must also be initialized using `--recursive`.

## Technical Details

### Why Submodules Don't Sync

Git submodules are **pointers** to specific commits in other repositories. When you:
1. Clone a repo → submodules are **empty directories**
2. Pull new commits → submodules **don't auto-update**
3. Switch branches → submodules **stay at old commits**

You must explicitly:
- `git submodule init` - Register submodules in .git/config
- `git submodule update` - Checkout the correct commits

### The Git Proxy Issue

Your git is configured with:
```
origin http://local_proxy@127.0.0.1:61614/git/ethanbissbort/rcbot2
```

This proxy may:
- Block external URLs (alliedmodders/hl2sdk-manifests)
- Not support git submodule protocol properly
- Time out on submodule operations

The fix script handles this by falling back to direct HTTP downloads via `download-manifests.sh`.

### The Path Mismatch

Build logs show:
```
/var/repos/rcbot2/hl2sdk-manifests/manifests
```

But your actual path is:
```
/home/user/rcbot2/hl2sdk-manifests/manifests
```

This is normal - the logs are generated at runtime and use the actual working directory. The `/var/repos/rcbot2` in older logs was from a different environment.

## Verification

After running fixes, verify with:

```bash
# Check submodule
ls -la hl2sdk-manifests/
# Should show: SdkHelpers.ambuild

# Check 32-bit libs
dpkg -l | grep linux-libc-dev:i386
# Should show: ii  linux-libc-dev:i386

# Try configure
python3 configure.py --enable-optimize
# Should complete without FileNotFoundError

# Check dependency report
cat build-logs/dependency-report-*.txt
# Should show: hl2sdk_manifests: existing:...
```

## Questions?

If the fix script doesn't resolve your issues:

1. Check `fix-environment.log` for detailed error messages
2. Share the log with the error messages
3. Try manual steps above
4. Verify you're in the correct directory and branch

## Success Indicators

You'll know the environment is fixed when:
- ✓ `hl2sdk-manifests/SdkHelpers.ambuild` exists
- ✓ `python3 configure.py` completes without errors
- ✓ No "FileNotFoundError" in build logs
- ✓ `ambuild build/debug` starts compiling
