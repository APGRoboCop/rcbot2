# RCBot2 AI-Enhanced - Release Build Output

This directory contains the successfully compiled release binaries for RCBot2 AI-Enhanced.

## Build Information

- **Build Date**: December 4, 2025
- **Configuration**: Release (optimized)
- **Architecture**: x86_64 (64-bit Linux)
- **Compiler**: Clang 18.1
- **SDK Targets**: HL2DM, TF2

## Files

### Core Plugin Files

1. **RCBot2Meta.x64.so**
   - Main Metamod:Source plugin loader
   - Required for loading RCBot2 into the game server
   - Place in: `addons/rcbot2/bin/`

2. **rcbot.2.hl2dm.so**
   - RCBot2 plugin for Half-Life 2: Deathmatch
   - Includes SourceMod extension integration
   - Place in: `addons/rcbot2/bin/x64/`

3. **rcbot.2.tf2.so**
   - RCBot2 plugin for Team Fortress 2
   - Includes SourceMod extension integration
   - Place in: `addons/rcbot2/bin/x64/`

## Installation

1. Copy these files to your game server's RCBot2 directory structure:
   ```
   your-server/
   ├── addons/
   │   └── rcbot2/
   │       └── bin/
   │           ├── RCBot2Meta.x64.so
   │           └── x64/
   │               ├── rcbot.2.hl2dm.so
   │               └── rcbot.2.tf2.so
   ```

2. Ensure you have Metamod:Source installed on your server

3. Load the plugin via Metamod:Source

## Features

- SourceMod extension integration enabled
- Optimized release build for production use
- Support for up to 101 players (HL2DM, TF2)
- 64-bit architecture support

## Build Environment

Successfully built with all submodules initialized:
- ✓ hl2sdk-manifests
- ✓ alliedmodders/metamod-source
- ✓ alliedmodders/sourcemod (with recursive submodules)
- ✓ alliedmodders/hl2sdk-hl2dm
- ✓ alliedmodders/hl2sdk-tf2

## Notes

- ML features (ONNX Runtime) not included in this build
- For ML features, see docs/ONNX_SETUP.md
- These are production-ready binaries with optimizations enabled

## Build Logs

Full build logs available in:
- `build-logs/build-release-20251204-141449.log`
- `build-logs/summary-release-20251204-141449.txt`
