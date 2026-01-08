# RCBot2 AI-Enhanced - Release Build Output

This directory contains the successfully compiled release binaries for RCBot2 AI-Enhanced.

## Build Information

- **Build Date**: January 8, 2026
- **Configuration**: Release (optimized)
- **Architectures**: x86 (32-bit) and x86_64 (64-bit) Linux
- **Compiler**: GCC/Clang
- **SDK Targets**: HL2DM, TF2
- **Naming Convention**: kebab-case (Metamod-compatible)

## Directory Structure

```
plugin-out/
├── RCBot2Meta.x64.so       # 64-bit Metamod loader
├── RCBot2Meta_i486.so      # 32-bit Metamod loader
├── x64/                    # 64-bit plugins
│   ├── rcbot-2-hl2dm.so
│   └── rcbot-2-tf2.so
└── x86/                    # 32-bit plugins
    ├── rcbot-2-hl2dm.so
    └── rcbot-2-tf2.so
```

## Installation

### For 32-bit Servers (Most Common)

Most Source engine servers (HL2DM, TF2, CSS, DODS) are **32-bit**.

1. Copy files to your server:
   ```
   your-server/
   ├── addons/
   │   └── rcbot2/
   │       └── bin/
   │           ├── RCBot2Meta_i486.so
   │           ├── rcbot-2-hl2dm.so   (from x86/)
   │           └── rcbot-2-tf2.so     (from x86/)
   ```

2. For 32-bit servers, plugins go directly in `bin/`, NOT in a subdirectory.

### For 64-bit Servers

Some newer Source engine builds may use 64-bit:

1. Copy files to your server:
   ```
   your-server/
   ├── addons/
   │   └── rcbot2/
   │       └── bin/
   │           ├── RCBot2Meta.x64.so
   │           └── x64/
   │               ├── rcbot-2-hl2dm.so   (from x64/)
   │               └── rcbot-2-tf2.so     (from x64/)
   ```

2. For 64-bit servers, plugins go in `bin/x64/`.

## Troubleshooting

### Error: "wrong ELF class: ELFCLASS64"

This means you're using 64-bit plugins on a 32-bit server. Use the files from the `x86/` folder instead.

### Error: "wrong ELF class: ELFCLASS32"

This means you're using 32-bit plugins on a 64-bit server. Use the files from the `x64/` folder instead.

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
