# RCBot2 Build Guide

**Last Updated**: 2025-12-25
**Platforms**: Linux (Ubuntu 22.04+), Windows (VS 2019+)

This document consolidates all build documentation, troubleshooting, and fixes.

---

## Table of Contents

1. [Quick Start](#1-quick-start)
2. [Prerequisites](#2-prerequisites)
3. [Build Process](#3-build-process)
4. [Configuration Options](#4-configuration-options)
5. [Troubleshooting](#5-troubleshooting)
6. [ML Features Setup](#6-ml-features-setup)
7. [CI/CD](#7-cicd)

---

## 1. Quick Start

### Linux (Recommended)

```bash
# Clone with submodules
git clone --recursive https://github.com/ethanbissbort/rcbot2.git
cd rcbot2

# Automated build (handles everything)
chmod +x build-linux.sh
./build-linux.sh --auto

# Output in build-release/package/
```

### Manual Build

```bash
# Ensure submodules are initialized
git submodule update --init --recursive

# Create build directory
mkdir build-release && cd build-release

# Configure
python3 ../configure.py \
    --hl2sdk-root=../alliedmodders \
    --mms-path=../alliedmodders/metamod-source \
    --sdks=hl2dm,tf2 \
    --enable-optimize

# Build
ambuild

# Output in build-release/package/
```

### Windows

```powershell
# From Developer Command Prompt for VS 2022
cd rcbot2
mkdir build-release
cd build-release

python ..\configure.py `
    --hl2sdk-root=..\alliedmodders `
    --mms-path=..\alliedmodders\metamod-source `
    --sdks=hl2dm,tf2 `
    --enable-optimize

ambuild
```

---

## 2. Prerequisites

### Linux Dependencies

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    gcc-multilib \
    g++-multilib \
    git \
    python3 \
    python3-pip \
    lib32z1-dev \
    libc6-dev-i386 \
    linux-libc-dev:i386

# Install AMBuild
pip install git+https://github.com/alliedmodders/ambuild
```

### Windows Dependencies

- Visual Studio 2019 or 2022 (C++ workload)
- Python 3.8+
- Git for Windows
- AMBuild: `pip install git+https://github.com/alliedmodders/ambuild`

### SDK Requirements

The following SDKs are required (provided as submodules):

| SDK | Path | Purpose |
|-----|------|---------|
| MetaMod:Source | `alliedmodders/metamod-source` | Plugin framework |
| HL2SDK (TF2) | `alliedmodders/hl2sdk-tf2` | TF2 support |
| HL2SDK (HL2DM) | `alliedmodders/hl2sdk-hl2dm` | HL2DM support |
| HL2SDK (DOD:S) | `alliedmodders/hl2sdk-dods` | DOD:S support |
| HL2SDK (CSS) | `alliedmodders/hl2sdk-css` | CS:S support |
| SourceMod | `alliedmodders/sourcemod` | Extension (optional) |
| SDK Manifests | `hl2sdk-manifests` | SDK detection |

---

## 3. Build Process

### Two-Phase Build (Modern AMBuild2)

```
Phase 1: Configuration
┌────────────────────────────────────────────┐
│ mkdir build-{config}                       │
│ cd build-{config}                          │
│ python3 ../configure.py [options]          │
└────────────────────────────────────────────┘
          ↓ (creates .ambuild2/)
Phase 2: Compilation
┌────────────────────────────────────────────┐
│ ambuild                                    │
└────────────────────────────────────────────┘
          ↓
Output: build-{config}/package/
```

### Build Script Options

```bash
./build-linux.sh [options]

Options:
  --auto          # No prompts, install everything
  --dev-only      # Debug build only
  --release-only  # Release build only
  --skip-deps     # Skip dependency installation
  --verbose       # Show all build output
```

### Build Outputs

| File | Platform | Size | Description |
|------|----------|------|-------------|
| `RCBot2Meta.x64.so` | Linux x64 | 32 KB | Metamod loader |
| `RCBot2Meta_i486.so` | Linux x86 | 32 KB | Metamod loader (32-bit) |
| `rcbot.2.tf2.so` | Linux | 11.5 MB | TF2 plugin |
| `rcbot.2.hl2dm.so` | Linux | 11.5 MB | HL2DM plugin |
| `rcbot.2.dods.so` | Linux | ~11 MB | DOD:S plugin |
| `rcbot.2.css.so` | Linux | ~11 MB | CS:S plugin |

### Package Structure

```
build-release/package/
└── addons/
    ├── metamod/
    │   ├── rcbot2.vdf
    │   ├── RCBot2Meta_i486.so
    │   └── RCBot2Meta.x64.so
    └── rcbot2/
        ├── bin/
        │   ├── rcbot.2.tf2.so
        │   ├── rcbot.2.hl2dm.so
        │   └── ...
        ├── config/
        │   ├── config.ini
        │   ├── weapons.ini
        │   └── ...
        ├── profiles/
        └── waypoints/
```

---

## 4. Configuration Options

### configure.py Options

| Option | Description |
|--------|-------------|
| `--hl2sdk-root=PATH` | Root directory containing hl2sdk-* folders |
| `--mms-path=PATH` | Path to MetaMod:Source |
| `--sm-path=PATH` | Path to SourceMod (optional) |
| `--sdks=SDKS` | Comma-separated list: tf2,hl2dm,dods,css,all,present |
| `--enable-debug` | Debug build with symbols |
| `--enable-optimize` | Release build with optimizations |
| `--targets=TARGETS` | Architecture: x86,x86_64 (default: both) |

### Examples

```bash
# Release build for TF2 and HL2DM only
python3 ../configure.py \
    --hl2sdk-root=../alliedmodders \
    --mms-path=../alliedmodders/metamod-source \
    --sdks=tf2,hl2dm \
    --enable-optimize

# Debug build with SourceMod extension
python3 ../configure.py \
    --hl2sdk-root=../alliedmodders \
    --mms-path=../alliedmodders/metamod-source \
    --sm-path=../alliedmodders/sourcemod \
    --sdks=all \
    --enable-debug

# Build only available SDKs
python3 ../configure.py \
    --hl2sdk-root=../alliedmodders \
    --mms-path=../alliedmodders/metamod-source \
    --sdks=present \
    --enable-optimize
```

### Compiler Flags

**Linux (GCC/Clang)**:
- Release: `-O3`, `-fvisibility=hidden`, LTO
- Debug: `-g3`, `-O0`
- Common: `-std=c++17`, `-fno-rtti`, `-fexceptions`

**Windows (MSVC)**:
- Release: `/Ox`, `/Zo`, static runtime (`/MT`)
- Debug: `/Od`, `/RTC1`, static debug runtime (`/MTd`)
- Common: `/std:c++17`, `/EHsc`

---

## 5. Troubleshooting

### Submodule Issues

#### Error: `SdkHelpers.ambuild: 404: Not Found`

**Cause**: hl2sdk-manifests submodule not initialized.

**Fix**:
```bash
git submodule update --init --recursive hl2sdk-manifests
# Or use the fix script
./fix-build-environment.sh
```

#### Error: `IExtensionSys.h: No such file or directory`

**Cause**: SourceMod submodule or its nested submodules not initialized.

**Fix**:
```bash
git submodule update --init --recursive alliedmodders/sourcemod
```

#### Error: `sp_vm_types.h: No such file or directory`

**Cause**: SourceMod's nested sourcepawn submodule not initialized.

**Fix**:
```bash
git submodule update --init --recursive alliedmodders/sourcemod
```

#### Error: `no submodule mapping found`

**Cause**: Corrupted git submodule state.

**Fix**:
```bash
git submodule sync
git submodule update --init --recursive
# Or run the fix script
./fix-build-environment.sh
```

### Library Issues

#### Error: `cannot find -lz` or 32-bit library errors

**Cause**: Missing 32-bit development libraries.

**Fix**:
```bash
sudo dpkg --add-architecture i386
sudo apt-get update
sudo apt-get install -y lib32z1-dev libc6-dev-i386 linux-libc-dev:i386
```

#### Error: `libstdc++.so.6: version 'GLIBCXX_X.X' not found`

**Cause**: Incompatible C++ library version.

**Fix**: Build uses static linking by default. Ensure you're using GCC 9+.

### AMBuild Issues

#### Error: `module 'ambuild2' has no attribute 'run'`

**Cause**: Using outdated AMBuild API.

**Fix**: Use the modern two-phase build process:
```bash
# Wrong (old API)
python3 configure.py && ambuild2.run.Build()

# Correct (modern)
cd build-release
python3 ../configure.py [options]
ambuild
```

#### Error: `ambuild: command not found`

**Cause**: AMBuild not installed or not in PATH.

**Fix**:
```bash
pip install git+https://github.com/alliedmodders/ambuild
export PATH=$HOME/.local/bin:$PATH
```

### Configuration Issues

#### Error: `hl2sdk not found`

**Cause**: SDK path incorrect or submodule not initialized.

**Fix**:
```bash
# Verify submodules exist
ls alliedmodders/hl2sdk-tf2
ls alliedmodders/metamod-source

# Initialize if missing
git submodule update --init --recursive

# Use correct paths
python3 ../configure.py --hl2sdk-root=../alliedmodders
```

### Build Verification

After building, verify:

```bash
# Check binary exists
ls -la build-release/package/addons/rcbot2/bin/

# Check dependencies (Linux)
ldd build-release/package/addons/rcbot2/bin/rcbot.2.tf2.so

# Check symbols (debug build)
file build-release/package/addons/rcbot2/bin/rcbot.2.tf2.so
```

---

## 6. ML Features Setup

### ONNX Runtime (Optional)

For ML features, install ONNX Runtime:

```bash
# Download ONNX Runtime
wget https://github.com/microsoft/onnxruntime/releases/download/v1.16.3/onnxruntime-linux-x64-1.16.3.tgz
tar -xzf onnxruntime-linux-x64-1.16.3.tgz
mv onnxruntime-linux-x64-1.16.3 dependencies/onnxruntime
```

### Verify ONNX Support

After building:
```bash
grep "RCBOT_WITH_ONNX" build-release/build.ninja
# If found, ONNX support is enabled
```

### Test Models

```bash
# Generate test model
pip install torch onnx onnxruntime numpy
python3 tools/create_test_model.py

# Test in-game
rcbot ml_model_load test models/test_model.onnx
rcbot ml_model_test test
rcbot ml_model_benchmark test 1000
```

### Performance Targets

| Model | Features | Target |
|-------|----------|--------|
| HL2DM | 56 | <0.5ms |
| TF2 | 96 | <1.0ms |
| General | - | <2.0ms |

---

## 7. CI/CD

### GitHub Actions

The project uses GitHub Actions for CI/CD (`.github/workflows/build.yml`):

- **Platforms**: Ubuntu 22.04, Ubuntu latest, Windows x86, Windows x64
- **Artifacts**: Built plugins with debug symbols
- **Triggers**: Push to main branches, pull requests

### Build Logs

Build scripts generate logs in `build-logs/`:

| File | Description |
|------|-------------|
| `configure-{config}-{timestamp}.log` | Configuration output |
| `build-{config}-{timestamp}.log` | Full build output |
| `errors-{config}-{timestamp}.log` | Errors only |
| `summary-{config}-{timestamp}.txt` | Build summary |
| `dependency-report-{timestamp}.txt` | Dependency status |

### Incremental Builds

AMBuild supports incremental builds:

```bash
# Rebuild only changed files
cd build-release
ambuild

# Force full rebuild
rm -rf .ambuild2
python3 ../configure.py [options]
ambuild
```

---

## Appendix: Quick Reference

### Common Build Commands

```bash
# Full automated build
./build-linux.sh --auto

# Debug build only
./build-linux.sh --dev-only

# Release build, skip deps
./build-linux.sh --release-only --skip-deps

# Manual release build
mkdir build-release && cd build-release
python3 ../configure.py --enable-optimize ...
ambuild

# Clean and rebuild
rm -rf build-release
./build-linux.sh --release-only
```

### Fix Scripts

```bash
# Fix submodule issues
./fix-build-environment.sh

# Download manifests manually
./download-manifests.sh
```

### Verification

```bash
# Check submodule status
git submodule status

# Verify SDK paths
ls -la alliedmodders/

# Check build output
ls -la build-release/package/addons/rcbot2/bin/

# Check library dependencies
ldd build-release/package/addons/rcbot2/bin/*.so
```
