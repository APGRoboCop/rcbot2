# Building RCBot2 from Source

Complete guide to compiling RCBot2 on Windows and Linux.

---

## Table of Contents

- [Prerequisites](#prerequisites)
- [Setting Up Dependencies](#setting-up-dependencies)
- [Building on Linux](#building-on-linux)
- [Building on Windows](#building-on-windows)
- [Build Options](#build-options)
- [Output Structure](#output-structure)
- [Build Scripts](#build-scripts)
- [Continuous Integration](#continuous-integration)
- [Troubleshooting](#troubleshooting)

---

## Prerequisites

### Required Tools

- **Python 3.6+** (Python 2 is deprecated)
- **Git** (for cloning and versioning)
- **C++ Compiler**:
  - Linux: GCC 7+ or Clang 5+
  - Windows: Visual Studio 2019+ or VS 2022
- **AMBuild** (AlliedModders Build system)

### Required SDKs

- **MetaMod:Source 1.10+** - Required for all builds
- **HL2SDK** - Source Engine SDK for your target game(s)
- **SourceMod 1.10+** (Optional) - For SourceMod native integration

---

## Setting Up Dependencies

### Install AMBuild

```bash
git clone https://github.com/alliedmodders/ambuild
cd ambuild
python setup.py install
# OR for user install:
python setup.py install --user
```

Verify installation:
```bash
ambuild --version
```

### Download MetaMod:Source

```bash
git clone https://github.com/alliedmodders/metamod-source --recursive
cd metamod-source
git checkout 1.11-dev
```

### Download HL2SDK

RCBot2 supports multiple Source Engine games, each requiring its own SDK.

```bash
# Create SDK directory
mkdir hl2sdk-root
cd hl2sdk-root

# Clone SDKs for games you want to build
# Team Fortress 2
git clone https://github.com/alliedmodders/hl2sdk -b tf2 hl2sdk-tf2

# Day of Defeat: Source
git clone https://github.com/alliedmodders/hl2sdk -b dods hl2sdk-dods

# Half-Life 2: Deathmatch
git clone https://github.com/alliedmodders/hl2sdk -b hl2dm hl2sdk-hl2dm

# Counter-Strike: Source
git clone https://github.com/alliedmodders/hl2sdk -b css hl2sdk-css
```

### Download SourceMod (Optional)

```bash
git clone https://github.com/alliedmodders/sourcemod --recursive
cd sourcemod
git checkout 1.11-dev
```

---

## Building on Linux

### 1. Install Build Tools

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install build-essential git python3 python3-pip
sudo apt-get install g++-multilib  # For 32-bit builds
```

**Fedora/RHEL:**
```bash
sudo dnf install gcc-c++ git python3 python3-pip
sudo dnf install glibc-devel.i686 libstdc++-devel.i686  # For 32-bit
```

### 2. Clone RCBot2

```bash
git clone --depth 1 https://github.com/ethanbissbort/rcbot2.git
cd rcbot2
```

### 3. Configure Build

```bash
mkdir build
cd build

# Configure for TF2 (example)
python ../configure.py \
  -s TF2 \
  --mms-path ~/metamod-source \
  --hl2sdk-root ~/hl2sdk-root \
  --sm-path ~/sourcemod
```

**Configuration Options:**
- `-s TF2` - Build for Team Fortress 2
- `--mms-path` - Path to MetaMod:Source
- `--hl2sdk-root` - Root directory containing HL2SDK folders
- `--sm-path` - Path to SourceMod (optional)
- `--enable-debug` - Build with debug symbols
- `--enable-optimize` - Build with optimizations
- `--targets x86_64` - Build for 64-bit (HL2DM)

### 4. Build

```bash
ambuild
```

Build output will be in `build/package/`.

### 5. Install

```bash
cp -r package/* /path/to/game/server/tf/
```

---

## Building on Windows

### 1. Install Visual Studio

Download [Visual Studio 2022 Community](https://visualstudio.microsoft.com/downloads/).

**Required components:**
- Desktop development with C++
- Windows 10 SDK
- MSVC v143 build tools
- C++ CMake tools (optional)

### 2. Install Python 3

Download from [python.org](https://www.python.org/downloads/).

Ensure Python is in your PATH:
```cmd
python --version
```

### 3. Install Git

Download from [git-scm.com](https://git-scm.com/download/win).

### 4. Clone RCBot2

```cmd
git clone --depth 1 https://github.com/ethanbissbort/rcbot2.git
cd rcbot2
```

### 5. Configure Build

Open **x86 Native Tools Command Prompt for VS 2022**.

```cmd
mkdir build
cd build

python ..\configure.py ^
  -s TF2 ^
  --mms-path C:\path\to\metamod-source ^
  --hl2sdk-root C:\path\to\hl2sdk-root ^
  --sm-path C:\path\to\sourcemod
```

### 6. Build

```cmd
ambuild
```

### 7. Install

Copy contents of `build\package\` to your game server directory.

---

## Build Options

### Supported Games (SDK)

| Game | SDK Flag | Notes |
|------|----------|-------|
| Team Fortress 2 | `TF2` | Most actively developed |
| Day of Defeat: Source | `DODS` | Stable |
| HL2: Deathmatch | `HL2DM` | Stable |
| Counter-Strike: Source | `CSS` | Beta support |
| Black Mesa | `BMS` | Experimental |
| SDK2013 | `SDK2013` | Generic Source SDK |

**Build multiple games:**
```bash
python configure.py -s TF2,DODS,HL2DM --mms-path ... --hl2sdk-root ...
```

**Build all available:**
```bash
python configure.py -s all --mms-path ... --hl2sdk-root ...
```

### Build Configurations

**Debug Build** (for development):
```bash
python configure.py -s TF2 --enable-debug --mms-path ... --hl2sdk-root ...
```

**Optimized Build** (for production):
```bash
python configure.py -s TF2 --enable-optimize --mms-path ... --hl2sdk-root ...
```

**With SourceMod natives:**
```bash
python configure.py -s TF2 --sm-path ~/sourcemod --mms-path ... --hl2sdk-root ...
```

### Architecture

RCBot2 builds 32-bit binaries by default (Source Engine is 32-bit).

For 64-bit builds (HL2DM on Linux):
```bash
python configure.py -s hl2dm --targets x86_64 --mms-path ... --hl2sdk-root ...
```

---

## Output Structure

After building, `build/package/` contains:

```
package/
├── addons/
│   └── rcbot2meta/
│       ├── bin/          # Platform-specific binaries
│       │   ├── rcbot.2.tf2.so (Linux)
│       │   └── rcbot.2.tf2.dll (Windows)
│       └── RCBot2Meta.vdf  # MetaMod plugin definition
└── rcbot2/
    ├── config/           # Configuration files
    └── waypoints/        # Waypoint directory
```

---

## Build Scripts

### Quick Build Script (Linux)

Save as `build.sh`:

```bash
#!/bin/bash

# Configuration
GAME="TF2"
MMS_PATH=~/metamod-source
HL2SDK_ROOT=~/hl2sdk-root
SM_PATH=~/sourcemod

# Build
mkdir -p build
cd build
python ../configure.py \
  -s $GAME \
  --mms-path $MMS_PATH \
  --hl2sdk-root $HL2SDK_ROOT \
  --sm-path $SM_PATH \
  --enable-optimize

ambuild
```

Usage:
```bash
chmod +x build.sh
./build.sh
```

### Quick Build Script (Windows)

Save as `build.bat`:

```batch
@echo off

REM Configuration
set GAME=TF2
set MMS_PATH=C:\path\to\metamod-source
set HL2SDK_ROOT=C:\path\to\hl2sdk-root
set SM_PATH=C:\path\to\sourcemod

REM Build
if not exist build mkdir build
cd build

python ..\configure.py ^
  -s %GAME% ^
  --mms-path %MMS_PATH% ^
  --hl2sdk-root %HL2SDK_ROOT% ^
  --sm-path %SM_PATH% ^
  --enable-optimize

ambuild

cd ..
```

### Clean Build

```bash
rm -rf build/
mkdir build
cd build
python ../configure.py ...
ambuild
```

---

## Continuous Integration

### GitHub Actions Example

`.github/workflows/build.yml`:

```yaml
name: Build RCBot2

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3

      - name: Setup Python
        uses: actions/setup-python@v4
        with:
          python-version: '3.10'

      - name: Install dependencies
        run: |
          sudo apt-get install g++-multilib
          python -m pip install ambuild

      - name: Clone dependencies
        run: |
          git clone --recursive https://github.com/alliedmodders/metamod-source
          git clone https://github.com/alliedmodders/hl2sdk -b tf2 hl2sdk-tf2

      - name: Build
        run: |
          mkdir build && cd build
          python ../configure.py -s TF2 --mms-path ../metamod-source --hl2sdk-root ..
          ambuild

      - name: Upload artifacts
        uses: actions/upload-artifact@v3
        with:
          name: rcbot2-tf2
          path: build/package/
```

---

## Troubleshooting

### "ambuild: command not found"

AMBuild not installed or not in PATH.

**Solution:**
```bash
python -m pip install --user ambuild
export PATH=$PATH:~/.local/bin  # Linux
```

### "Could not find HL2SDK"

SDK path is incorrect or SDK not downloaded.

**Solution:**
- Verify `--hl2sdk-root` points to directory containing `hl2sdk-tf2`, etc.
- Ensure SDKs are cloned with correct branch names

### "Python version too old"

**Solution:**
```bash
sudo apt-get install python3 python3-pip
python3 configure.py ...
```

### Build fails with "undefined reference"

Missing SDK or MetaMod:Source.

**Solution:**
- Ensure all dependencies are downloaded
- Check paths are correct
- Verify MetaMod:Source was cloned with `--recursive`

### "Permission denied" on Linux

**Solution:**
```bash
chmod -R u+w build/
```

### Visual Studio version mismatch (Windows)

**Solution:**
- Use "x86 Native Tools Command Prompt for VS 2022"
- Ensure MSVC v143 build tools are installed

### "Git not found" during configure

**Solution:**
```bash
sudo apt-get install git  # Linux
```

### "Failed to load plugin" at runtime

Missing dependencies or wrong architecture.

**Solution (Linux):**
```bash
ldd addons/rcbot2meta/bin/rcbot.2.tf2.so
sudo apt-get install lib32stdc++6
```

**Verify architecture:**
```bash
file addons/rcbot2meta/bin/rcbot.2.tf2.so
# Should show: ELF 32-bit
```

---

## Advanced Options

### Parallel Builds

AMBuild automatically uses multiple cores. To limit:
```bash
ambuild -j 4  # Use 4 cores
```

### Breakpad Symbols

For crash reporting:
```bash
python configure.py -s TF2 --breakpad-upload --mms-path ... --hl2sdk-root ...
```

### ONNX Runtime Support

See [ML Documentation](ML.md#onnx-runtime-integration) for building with ML support.

---

## Next Steps

After building:

1. **Install** the plugin on your server
2. **Configure** bot settings (see [Usage Guide](USAGE.md#configuration))
3. **Download waypoints** for your maps (see [Waypoint Guide](waypoints.md))
4. **Test** the bots on your server

---

**See Also**:
- [Usage Guide](USAGE.md) - Installation and configuration
- [Waypoint Guide](waypoints.md) - Waypoint documentation
- [ML Documentation](ML.md) - ML build options

---

**Last Updated**: 2025-12-27
