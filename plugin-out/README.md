# RCBot2 AI-Enhanced - Release Build Output

This directory contains the successfully compiled release binaries for RCBot2 AI-Enhanced.

## Build Information

- **Build Date**: January 8, 2026
- **Configuration**: Release (optimized)
- **Architecture**: x86_64 (64-bit Linux)
- **Compiler**: Clang 18.1
- **SDK Targets**: HL2DM, TF2
- **Naming Convention**: kebab-case (Metamod-compatible)

## Files

### Core Plugin Files

1. **RCBot2Meta.x64.so**
   - Main Metamod:Source plugin loader
   - Required for loading RCBot2 into the game server
   - Place in: `addons/rcbot2/bin/`

2. **rcbot-2-hl2dm.so**
   - RCBot2 plugin for Half-Life 2: Deathmatch
   - Includes SourceMod extension integration
   - Place in: `addons/rcbot2/bin/x64/`

3. **rcbot-2-tf2.so**
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
   │               ├── rcbot-2-hl2dm.so
   │               └── rcbot-2-tf2.so
   ```

2. Ensure you have Metamod:Source installed on your server

3. Load the plugin via Metamod:Source

## Architecture Notes

**IMPORTANT**: These are 64-bit binaries. Most Source engine game servers (HL2DM, TF2, CSS, DODS) are **32-bit**.

If you see the error:
```
wrong ELF class: ELFCLASS64
```

You need to rebuild with 32-bit target:
```bash
./build-linux.sh --x86-only
```

For 32-bit builds:
- Loader: `RCBot2Meta_i486.so` goes in `addons/rcbot2/bin/`
- Plugins: `rcbot-2-*.so` go in `addons/rcbot2/bin/` (not x64/)

## Features

- SourceMod extension integration enabled
- Optimized release build for production use
- Support for up to 101 players (HL2DM, TF2)
- Kebab-case naming (Metamod-compatible)

## Build Environment

Successfully built with all submodules initialized:
- hl2sdk-manifests
- alliedmodders/metamod-source
- alliedmodders/sourcemod (with recursive submodules)
- alliedmodders/hl2sdk-hl2dm
- alliedmodders/hl2sdk-tf2

## Notes

- ML features (ONNX Runtime) not included in this build
- For ML features, see docs/ONNX_SETUP.md
- These are production-ready binaries with optimizations enabled
