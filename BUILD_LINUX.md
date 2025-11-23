# RCBot2 AI-Enhanced - Linux Build Guide (Ubuntu Server 24.x)

Complete guide for building RCBot2 on Ubuntu Server 24.x with ML/AI support.

## 🚀 Quick Start

```bash
# 1. Clone the repository
git clone https://github.com/your-repo/rcbot2.git
cd rcbot2

# 2. Make build script executable
chmod +x build-linux.sh

# 3. Run automated build
./build-linux.sh --auto

# That's it! The script handles everything.
```

---

## 📋 What the Script Does

The `build-linux.sh` script automates the entire build process:

### 1. **System Verification**
- ✅ Checks Ubuntu version (24.x)
- ✅ Verifies architecture (x86_64 or aarch64)
- ✅ Ensures sudo privileges available

### 2. **Dependency Installation**
- ✅ **Build tools:** gcc, g++, make, git
- ✅ **32-bit support:** lib32z1-dev, libc6-dev-i386
- ✅ **Python:** Python 3.8+ with pip
- ✅ **AMBuild:** AlliedModders build system
- ✅ **ML packages:** numpy, onnx, pytorch (optional)

### 3. **SDK Downloads**
- ✅ **HL2SDK:** Half-Life 2 SDK for multiple games (TF2, HL2DM, CSS, DoD:S)
- ✅ **Metamod:Source:** Plugin interface
- ✅ **ONNX Runtime:** ML inference library (optional)

### 4. **Builds Both Configurations**
- ✅ **Debug:** With symbols for development
- ✅ **Release:** Optimized for production

### 5. **Generates Comprehensive Logs**
- ✅ Build output logs
- ✅ Error-only logs (Claude-ready)
- ✅ Summary reports with troubleshooting

---

## 🎯 Usage Options

### Interactive Mode (Default)
Prompts for confirmation before installing each component:
```bash
./build-linux.sh
```

### Fully Automated
No prompts, installs everything automatically:
```bash
./build-linux.sh --auto
```

### Build Specific Configuration
```bash
# Debug only
./build-linux.sh --dev-only

# Release only
./build-linux.sh --release-only

# Both (default)
./build-linux.sh
```

### Skip Dependencies
If you've already installed dependencies:
```bash
./build-linux.sh --skip-deps
```

### Verbose Output
Show all build details:
```bash
./build-linux.sh --verbose
```

### Combine Options
```bash
./build-linux.sh --auto --release-only --verbose
```

---

## 📦 Dependencies Installed

### System Packages
```
build-essential        # GCC, G++, make, etc.
gcc-multilib          # 32-bit compilation support
g++-multilib          # 32-bit C++ support
git                   # Version control
python3               # Python 3.8+
python3-pip           # Python package manager
python3-venv          # Virtual environments
lib32z1-dev          # 32-bit zlib
libc6-dev-i386       # 32-bit C library
linux-libc-dev:i386  # 32-bit kernel headers
```

### Python Packages
```
ambuild2              # AMBuild build system (required)
numpy                 # ML/numerical computing
onnx                  # ONNX model format
onnxruntime          # ONNX Runtime inference
torch                # PyTorch (for training)
scikit-learn         # ML utilities
```

### Downloaded SDKs
```
HL2SDK (hl2dm)       # Half-Life 2 Deathmatch SDK
HL2SDK (tf2)         # Team Fortress 2 SDK
HL2SDK (css)         # Counter-Strike: Source SDK
HL2SDK (dods)        # Day of Defeat: Source SDK
Metamod:Source       # Plugin framework
ONNX Runtime         # ML inference library
```

All SDKs are downloaded to `./dependencies/`:
```
dependencies/
├── hl2sdk/
│   ├── hl2sdk-hl2dm/
│   ├── hl2sdk-tf2/
│   ├── hl2sdk-css/
│   └── hl2sdk-dods/
├── metamod-source/
└── onnxruntime/
    ├── include/
    └── lib/
        └── libonnxruntime.so
```

---

## 🏗️ Build Output

After successful build, you'll find:

```
rcbot2/
├── build-debug/               # Debug build artifacts
│   └── package/
│       └── addons/
│           └── rcbot2/
│               └── bin/
│                   ├── rcbot.2.hl2dm.so
│                   ├── rcbot.2.tf2.so
│                   └── ...
├── build-release/             # Release build artifacts
│   └── package/
│       └── addons/
│           └── rcbot2/
│               └── bin/
│                   ├── rcbot.2.hl2dm.so
│                   ├── rcbot.2.tf2.so
│                   └── ...
├── build-logs/               # Build logs (for debugging)
│   ├── configure-debug-*.log
│   ├── configure-release-*.log
│   ├── build-debug-*.log
│   ├── build-release-*.log
│   ├── errors-debug-*.log
│   ├── errors-release-*.log
│   ├── summary-debug-*.txt
│   ├── summary-release-*.txt
│   └── dependency-report-*.txt
└── dependencies/             # Downloaded SDKs
```

---

## 📥 Installation

### For Game Servers

After building, install the plugin:

```bash
# 1. Copy the built plugin to your game server
# For HL2DM:
cp build-release/package/addons/rcbot2/bin/rcbot.2.hl2dm.so \
   /path/to/gameserver/hl2mp/addons/rcbot2/bin/

# For TF2:
cp build-release/package/addons/rcbot2/bin/rcbot.2.tf2.so \
   /path/to/gameserver/tf/addons/rcbot2/bin/

# 2. Copy ONNX Runtime (if using ML features)
cp dependencies/onnxruntime/lib/libonnxruntime.so* \
   /path/to/gameserver/hl2mp/addons/rcbot2/bin/

# 3. Set library path (add to server startup script)
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/path/to/gameserver/hl2mp/addons/rcbot2/bin
```

### For Local Testing

```bash
# Use the debug build for testing
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)/build-debug/package/addons/rcbot2/bin
```

---

## 🤖 ML Features Setup

### Enabling ML Features

The build script automatically detects and enables ONNX Runtime if available.

**Verification:**
```bash
# Check if ONNX Runtime was found during build
grep "ONNX Runtime detected" build-logs/configure-release-*.log

# Check if RCBOT_WITH_ONNX was defined
grep "RCBOT_WITH_ONNX" build-release/build.ninja
```

### Runtime Requirements

For ML features to work at runtime:

1. **ONNX Runtime library must be accessible:**
   ```bash
   export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/path/to/onnxruntime/lib
   ```

2. **Train or download ML models:**
   - See `docs/ML_TRAINING_GUIDE.md` for training
   - Place `.onnx` models in game server's `addons/rcbot2/models/`

3. **Configure at runtime:**
   - See `docs/ONNX_SETUP.md` for server configuration

### Disabling ML Features

If you don't want ML support:

```bash
# Remove or rename ONNX Runtime before building
mv dependencies/onnxruntime dependencies/onnxruntime.disabled
./build-linux.sh --auto
```

The plugin will build without ML support (traditional rule-based AI only).

---

## 🐛 Troubleshooting

### Build Fails with "ambuild2 not found"

**Cause:** Python can't find the AMBuild module.

**Fix:**
```bash
# Ensure pip user packages are in PATH
echo 'export PATH=$HOME/.local/bin:$PATH' >> ~/.bashrc
source ~/.bashrc

# Verify AMBuild
python3 -c "import ambuild2; print(ambuild2.__version__)"

# If still fails, reinstall
python3 -m pip install --user --force-reinstall git+https://github.com/alliedmodders/ambuild
```

### Build Fails with "cannot find -lz" or "cannot find -lstdc++"

**Cause:** Missing 32-bit libraries.

**Fix:**
```bash
sudo dpkg --add-architecture i386
sudo apt-get update
sudo apt-get install -y lib32z1-dev libc6-dev-i386 libstdc++6:i386
```

### Build Fails with "hl2sdk not found"

**Cause:** HL2SDK not downloaded or path incorrect.

**Fix:**
```bash
# Re-run with dependency installation
./build-linux.sh --auto

# Or manually download
./build-linux.sh  # Choose "y" when prompted for HL2SDK
```

### ONNX Runtime Not Detected

**Cause:** ONNX Runtime download failed or architecture unsupported.

**Fix:**
```bash
# Check download
ls -la dependencies/onnxruntime/lib/libonnxruntime.so

# If missing, manually download for your architecture
# x86_64:
wget https://github.com/microsoft/onnxruntime/releases/download/v1.16.3/onnxruntime-linux-x64-1.16.3.tgz
tar -xzf onnxruntime-linux-x64-1.16.3.tgz
mv onnxruntime-linux-x64-1.16.3 dependencies/onnxruntime

# aarch64 (ARM):
wget https://github.com/microsoft/onnxruntime/releases/download/v1.16.3/onnxruntime-linux-aarch64-1.16.3.tgz
tar -xzf onnxruntime-linux-aarch64-1.16.3.tgz
mv onnxruntime-linux-aarch64-1.16.3 dependencies/onnxruntime
```

### Python Version Too Old

**Cause:** Ubuntu Server 24.x should have Python 3.12, but check anyway.

**Fix:**
```bash
# Check version
python3 --version

# Should be >= 3.8. If not:
sudo apt-get update
sudo apt-get install -y python3.12 python3.12-venv python3.12-pip

# Make it default (if needed)
sudo update-alternatives --install /usr/bin/python3 python3 /usr/bin/python3.12 1
```

### Build Succeeds but .so Files Missing

**Cause:** Build may have partially succeeded.

**Fix:**
```bash
# Check build log for actual errors
grep -i "error" build-logs/build-release-*.log

# Look for built files
find build-release -name "*.so"

# Try clean rebuild
rm -rf build-release
./build-linux.sh --release-only
```

---

## 📊 Build Logs for Claude Analysis

When build fails, the script generates Claude-ready error logs:

### Files to Upload to Claude:

1. **`build-logs/errors-release-TIMESTAMP.log`**
   - Contains only error messages
   - Easy for Claude to parse

2. **`build-logs/summary-release-TIMESTAMP.txt`**
   - Build summary with context
   - Common issues and suggestions

3. **`build-logs/build-release-TIMESTAMP.log`** (if needed)
   - Full build output
   - Upload if errors are unclear

### Sample Claude Prompt:

```
I'm getting build errors for RCBot2 on Ubuntu Server 24.x.
Please analyze the attached error log and summary, and suggest fixes.

System: Ubuntu Server 24.04
Architecture: x86_64
Configuration: Release
```

---

## ⚡ Performance

### Build Times (Typical)

| Configuration | First Build | Incremental |
|--------------|-------------|-------------|
| **Debug** | 3-5 minutes | 30-60 seconds |
| **Release** | 4-6 minutes | 30-60 seconds |

First build is slower because it compiles all SDK code. Subsequent builds are much faster.

### Optimization Tips

1. **Use --release-only for production:**
   ```bash
   ./build-linux.sh --release-only
   ```

2. **Use --skip-deps after first build:**
   ```bash
   ./build-linux.sh --skip-deps --release-only
   ```

3. **Use ccache for faster rebuilds:**
   ```bash
   sudo apt-get install ccache
   export CC="ccache gcc"
   export CXX="ccache g++"
   ./build-linux.sh --release-only
   ```

---

## 🔄 Updating

### Update RCBot2 Source
```bash
git pull origin main
./build-linux.sh --skip-deps --release-only
```

### Update SDKs
```bash
cd dependencies/hl2sdk/hl2sdk-tf2
git pull
cd ../../..
./build-linux.sh --skip-deps --release-only
```

### Update ONNX Runtime
```bash
rm -rf dependencies/onnxruntime
./build-linux.sh  # Will re-download latest
```

---

## 💡 Tips

1. **First build takes time:** Go grab coffee ☕ (5-10 minutes for both configs)
2. **Use screen/tmux:** For remote builds that might disconnect
3. **Check logs first:** Before asking for help, check build-logs/
4. **Debug vs Release:** Debug is 3-5x slower at runtime, use Release for testing
5. **Keep dependencies:** The dependencies/ folder can be reused across updates

---

## 📖 Additional Documentation

- **ML Training:** `docs/ML_TRAINING_GUIDE.md`
- **ONNX Setup:** `docs/ONNX_SETUP.md`
- **Windows Build:** `ps/BUILD_GUIDE.md`
- **AMBuild Docs:** https://wiki.alliedmods.net/AMBuild

---

## 🤝 Getting Help

**Build Issues:**
1. Check `build-logs/summary-*.txt` for common problems
2. Search existing GitHub issues
3. Upload error logs to Claude for analysis
4. Open new issue with logs attached

**Runtime Issues:**
1. Check game server console for errors
2. Verify library paths: `ldd build-release/package/addons/rcbot2/bin/rcbot.2.*.so`
3. See runtime documentation in `docs/`

---

## 🏗️ Architecture Support

| Architecture | Status | Notes |
|-------------|--------|-------|
| **x86_64** | ✅ Full support | Recommended |
| **aarch64** | ✅ Supported | ARM64 servers |
| **i386** | ⚠️ Not tested | 32-bit only systems |

---

## 📝 License

RCBot2 is licensed under the GNU General Public License v2 (GPLv2+).
See LICENSE file for details.

---

## 🎉 Success Checklist

After running the build script, verify:

- [ ] Script completed without errors
- [ ] `build-release/package/addons/rcbot2/bin/` contains `.so` files
- [ ] `dependencies/onnxruntime/lib/libonnxruntime.so` exists (if using ML)
- [ ] Build logs show "BUILD SUCCESSFUL"
- [ ] Can load plugin in game server

If all checked, you're ready to run RCBot2! 🚀
