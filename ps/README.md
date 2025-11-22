# RCBot2 AI-Enhanced - PowerShell Build Scripts

This directory contains PowerShell scripts for managing dependencies and building the RCBot2 project on Windows.

## 📋 Scripts Overview

### 1. `verify-dependencies.ps1`
Interactive script to verify and install all dependencies required for development and production.

**Usage:**
```powershell
# Basic verification (interactive)
.\ps\verify-dependencies.ps1

# Auto-install missing packages (requires admin)
.\ps\verify-dependencies.ps1 -AutoInstall

# Skip optional dependencies
.\ps\verify-dependencies.ps1 -SkipOptional
```

**What it checks:**
- ✅ Visual Studio / MSVC Build Tools
- ✅ Python 3.8+ and pip
- ✅ Python ML packages (numpy, onnx, pytorch, etc.)
- ✅ ONNX Runtime C++ libraries
- ✅ Source SDK / Metamod:Source
- ✅ Git
- ✅ CMake (optional)
- ✅ AMBuild

### 2. `build-solution.ps1`
Comprehensive build script with verbose error logging and Claude-ready output.

**Usage:**
```powershell
# Standard release build
.\ps\build-solution.ps1

# Debug build
.\ps\build-solution.ps1 -Configuration Debug

# Clean build
.\ps\build-solution.ps1 -Clean

# Parallel compilation (faster)
.\ps\build-solution.ps1 -Parallel

# Custom log directory
.\ps\build-solution.ps1 -LogDir "C:\BuildLogs"
```

**Features:**
- 🔍 Auto-detects build system (MSBuild, CMake, AMBuild)
- 📝 Comprehensive logging (main log, errors, warnings)
- 📊 Error analysis and categorization
- 🤖 Claude-ready error logs for analysis
- ⚡ Parallel compilation support
- 🧹 Clean build option

## 🚀 Quick Start

### First Time Setup

1. **Verify Dependencies:**
   ```powershell
   # Run as Administrator for best results
   .\ps\verify-dependencies.ps1
   ```

2. **Install Missing Dependencies:**
   Follow the instructions provided by the verification script for any missing dependencies.

3. **Build the Project:**
   ```powershell
   .\ps\build-solution.ps1 -Configuration Release
   ```

### Troubleshooting Build Errors

If the build fails, the script generates detailed logs:

```
build-logs/
  ├── build-YYYYMMDD-HHMMSS.log      # Full build output
  ├── errors-YYYYMMDD-HHMMSS.log     # Compilation errors only
  ├── warnings-YYYYMMDD-HHMMSS.log   # Warnings only
  └── summary-YYYYMMDD-HHMMSS.txt    # Error summary
```

**To get help from Claude:**
1. Upload `errors-YYYYMMDD-HHMMSS.log` to Claude
2. Upload `summary-YYYYMMDD-HHMMSS.txt` for context
3. Ask: "Please analyze these compilation errors and suggest fixes"

## 📖 Detailed Documentation

### verify-dependencies.ps1 Options

| Parameter | Description |
|-----------|-------------|
| `-AutoInstall` | Automatically install Python packages without prompting |
| `-SkipOptional` | Skip checking optional dependencies (Git, CMake) |
| `-Verbose` | Display detailed information during checks |

**Output:**
- Console summary with color-coded status
- `dependency-report.txt` - JSON report of all dependency status
- Manual installation instructions for missing dependencies

### build-solution.ps1 Options

| Parameter | Description | Default |
|-----------|-------------|---------|
| `-Configuration` | Build configuration (Debug/Release) | Release |
| `-Clean` | Perform clean build (rebuild all) | False |
| `-LogDir` | Directory for build logs | `.\build-logs` |
| `-SkipTests` | Skip running tests after build | False |
| `-Parallel` | Enable parallel compilation | False |
| `-Verbose` | Show detailed build output | False |

## 🔧 Environment Variables

The scripts use/set these environment variables:

- `ONNXRUNTIME_DIR` - Path to ONNX Runtime installation
- `SOURCE_SDK_DIR` - Path to Source SDK (optional)

## 📁 Directory Structure

After building, you'll have:

```
rcbot2/
  ├── ps/                          # PowerShell scripts
  ├── build-logs/                  # Build logs (auto-created)
  ├── build-Release/               # Build output (AMBuild)
  ├── build-cmake-Release/         # Build output (CMake)
  └── dependency-report.txt        # Dependency status
```

## 🐛 Common Issues

### "Visual Studio not found"
**Solution:** Install Visual Studio 2019/2022 with "Desktop development with C++" workload
- Download: https://visualstudio.microsoft.com/downloads/

### "ONNX Runtime not found"
**Solution:** Download and extract ONNX Runtime
1. Download from: https://github.com/microsoft/onnxruntime/releases
2. Extract to `C:\onnxruntime`
3. Set environment variable: `setx ONNXRUNTIME_DIR "C:\onnxruntime"`

### "Python packages missing"
**Solution:** Install with pip
```powershell
pip install numpy onnx onnxruntime torch scikit-learn
```

### "AMBuild not found"
**Solution:** Install AMBuild from GitHub
```powershell
pip install git+https://github.com/alliedmodders/ambuild
```

### Build fails with "Cannot find vcvarsall.bat"
**Solution:** Ensure Visual Studio C++ tools are installed
1. Open Visual Studio Installer
2. Modify your installation
3. Select "Desktop development with C++"
4. Install

## 📋 Requirements

### Minimum Requirements
- Windows 10/11 (64-bit)
- PowerShell 5.1 or later
- Visual Studio 2019/2022 or Build Tools
- 4 GB RAM
- 10 GB free disk space

### Recommended
- PowerShell 7+
- Visual Studio 2022
- 16 GB RAM
- SSD storage

## 🤝 Getting Help

1. **Check logs:** All build errors are logged to `build-logs/`
2. **Verify dependencies:** Run `.\ps\verify-dependencies.ps1`
3. **Upload to Claude:** Share error logs for AI-assisted debugging
4. **Check docs:** See `docs/ONNX_SETUP.md` and `docs/ML_TRAINING_GUIDE.md`

## 📝 License

These scripts are part of the RCBot2 project and inherit the same license (GPLv2+).

## 🔄 Updates

To update the scripts:
```powershell
git pull origin main
```

The scripts are version-controlled with the project and will be updated as needed.
