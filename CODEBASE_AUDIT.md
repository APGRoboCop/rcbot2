# RCBot2 Comprehensive Codebase Audit

**Audit Date:** December 25, 2025
**Repository:** ethanbissbort/rcbot2
**Branch:** claude/project-codebase-audit-pqgOs

---

## Executive Summary

RCBot2 is an AI-powered bot plugin for Source Engine games (TF2, HL2DM, DoD:S, CSS, and others) built as a MetaMod:Source plugin. This fork includes significant enhancements: ML/AI infrastructure with ONNX Runtime integration, comprehensive SourceMod integration (70+ natives), and improved waypoint navigation systems.

**Overall Assessment:** The project is well-organized with good documentation and modern build infrastructure. However, there are **critical security vulnerabilities** related to buffer handling that require immediate attention.

| Category | Rating | Notes |
|----------|--------|-------|
| Architecture | Good | Well-structured, polymorphic game support |
| Build System | Good | Modern AMBuild2, cross-platform support |
| Code Quality | Moderate | Mix of legacy and modern patterns |
| Security | Critical | Buffer overflow vulnerabilities |
| Documentation | Excellent | Comprehensive docs, roadmaps, API reference |
| Testing | Poor | Limited test coverage |

---

## Table of Contents

1. [Project Overview](#1-project-overview)
2. [Architecture Analysis](#2-architecture-analysis)
3. [Build System Review](#3-build-system-review)
4. [Code Quality Assessment](#4-code-quality-assessment)
5. [Security Audit](#5-security-audit)
6. [Documentation Review](#6-documentation-review)
7. [Recommendations](#7-recommendations)

---

## 1. Project Overview

### 1.1 Project Purpose
RCBot2 adds intelligent AI-controlled bots to Source Engine games. Bots can navigate maps, understand objectives, use class-specific abilities, and provide challenging gameplay.

### 1.2 Supported Games

| Game | Status | Notes |
|------|--------|-------|
| Half-Life 2: Deathmatch | Production | Primary ML/AI development target |
| Team Fortress 2 | Production | All 9 classes, most game modes, MvM |
| Day of Defeat: Source | Production | All classes, capture points |
| Counter-Strike: Source | Beta | Combat, navigation (buy menu WIP) |
| Black Mesa | Beta | Cooperative gameplay |
| Synergy | Beta | Cooperative gameplay |

### 1.3 Key Features (This Fork)
- **ML/AI Infrastructure**: ONNX Runtime integration, feature extraction, gameplay recording
- **SourceMod Integration**: 70+ natives across 7 implementation phases
- **Enhanced Waypoints**: Weapon pickups, NPC combat support, interactive objects
- **HL2DM Enhancements**: NPC combat system, Gravity Gun API, event system

### 1.4 License
- **Primary**: GNU Affero General Public License v3.0 (AGPL-3.0)
- **Exception**: `rcbot/logging.{h,cpp}` under BSD Zero Clause License

---

## 2. Architecture Analysis

### 2.1 Directory Structure

```
rcbot2/
├── utils/RCBot2_meta/       # Main bot implementation (~71,000 lines)
│   ├── bot*.cpp/h           # Core AI systems
│   ├── bot_fortress.cpp/h   # TF2-specific (263KB, largest file)
│   ├── bot_task.cpp/h       # Task scheduling (137KB)
│   ├── ml/                  # ML/AI infrastructure (Phase 0)
│   └── [game]_*.cpp/h       # Game-specific implementations
├── rcbot/                   # Shared utilities
│   ├── entprops.cpp/h       # Entity property management
│   └── tf2/                 # TF2 utilities
├── loader/                  # Metamod plugin loader shim
├── sm_ext/                  # SourceMod extension (70+ natives)
├── scripting/               # SourcePawn example scripts
├── config/                  # ML configuration files
├── docs/                    # User and API documentation
├── roadmaps/                # Development planning
├── package/                 # Installation templates
├── plugin-out/              # Built binaries
├── alliedmodders/           # SDK submodules
└── dependencies/            # ONNX Runtime, external libs
```

### 2.2 Core Components

| Component | Files | Purpose |
|-----------|-------|---------|
| Bot AI Core | `bot.h/cpp`, `bot_task.cpp` | Central bot behavior, task scheduling |
| TF2 Implementation | `bot_fortress.cpp` | All 9 classes, game modes, MvM |
| Navigation | `bot_waypoint.cpp` | Pathfinding, waypoint system |
| Weapons | `bot_weapons.cpp` | Weapon selection logic |
| SourceMod Extension | `sm_ext/*.cpp` | 70+ natives for bot control |
| ML Infrastructure | `ml/*.h` | ONNX integration, feature extraction |

### 2.3 Design Patterns

**Polymorphic Game Support:**
- Base `CBot` class with game-specific subclasses (`CBotTF2`, `CBotCSS`, `CBotDOD`)
- Allows same core logic with game-specific behaviors

**Task-Based AI:**
- Hierarchical task scheduling system (`bot_task.h/cpp`)
- Tasks pushed to stack, executed in priority order
- Supports interruptions and complex multi-step behaviors

**Entity Handle System:**
- Custom `MyEHandle` class for safe entity references
- Validates serial numbers to prevent use-after-free

**Singleton Pattern:**
- Used for ML components (`CCoopModeDetector`, `CNPCDatabase`)
- Proper deleted copy constructors

### 2.4 Codebase Statistics

| Metric | Value |
|--------|-------|
| Total Source Files | ~176 C++ files |
| Total Lines of Code | ~83,000 lines |
| Largest File | `bot_fortress.cpp` (8,500 lines) |
| Class Definitions | 1,864 |
| Virtual Methods | 26 virtual destructors |
| Git Submodules | 12 |

---

## 3. Build System Review

### 3.1 Build Infrastructure

**Primary System:** AMBuild2 (AlliedModders Build)

| File | Purpose |
|------|---------|
| `configure.py` | Build configuration entry point |
| `AMBuildScript` | SDK detection, versioning |
| `AMBuilder` | Main plugin build specification |
| `build-linux.sh` | Automated Linux build (1,012 lines) |
| `ps/build-ambuild.ps1` | Windows PowerShell build |

### 3.2 Compiler Configuration

**C++ Standard:** C++17

**Linux (GCC/Clang):**
- Optimization: `-O3` (release), `-g3` (debug)
- Flags: `-fno-rtti`, `-fexceptions`, `-fvisibility=hidden`
- Static linking: `libgcc`, `libstdc++`

**Windows (MSVC):**
- Optimization: `/Ox /Zo` (release)
- Runtime: `/MT` (static)
- Exception handling: `/EHsc`

### 3.3 Build Targets

| Target | Platform | Size |
|--------|----------|------|
| `RCBot2Meta.x64.so` | Linux x64 | 32 KB |
| `rcbot.2.tf2.so` | Linux | 11.5 MB |
| `rcbot.2.hl2dm.so` | Linux | 11.5 MB |
| `rcbot.2.dods.so` | Linux | Similar |
| `rcbot.2.css.so` | Linux | Similar |

### 3.4 Known Build Issues

| Issue | Severity | Status |
|-------|----------|--------|
| SourceMod submodule headers | Fixed | Requires `--recursive` init |
| HL2SDK manifest download | Fixed | Fallback mechanism added |
| AMBuild API compatibility | Fixed | Two-phase build process |
| 32-bit dev libraries | Documentation | Documented fix |

### 3.5 CI/CD

**GitHub Actions:** `.github/workflows/build.yml`
- Tests: Ubuntu 22.04, Ubuntu latest, Windows x86/x64
- Generates debug symbols (PDB/DBG)
- Uploads artifacts

---

## 4. Code Quality Assessment

### 4.1 Coding Style

**Strengths:**
- Consistent Hungarian notation (`m_` for members, `p` for pointers)
- Clear separation of headers and implementation
- Modern C++ features (std::vector, std::unique_ptr, constexpr)

**Concerns:**
- Large header files (bot.h: 1,189 lines, bot_task.h: 1,630 lines)
- Mix of legacy macros and modern patterns
- Some files with 30+ includes (high coupling)

### 4.2 Error Handling

**Patterns Used:**
- Exception handling in ONNX integration (714 try/catch instances)
- Return codes for traditional game code
- Defensive null checks (329 nullptr/NULL checks)

**Issues:**
- Inconsistent between old and new code
- Some getters return raw pointers without null guarantees
- Chained pointer dereferences without intermediate validation

### 4.3 Memory Management

**Smart Pointers:**
- 128 occurrences of `std::unique_ptr`, `std::shared_ptr`
- ONNX integration uses modern patterns
- Limited adoption in legacy game interface code

**Raw Pointers:**
- 1,287 occurrences of `new`/`delete`/`malloc`/`free`
- Required for Half-Life engine interop
- Creates memory management complexity

**Custom Containers:**
- `bot_genclass.h`: 1,070 lines of custom collections
- Predates STL, should be replaced with standard containers

### 4.4 Code Smells

| Issue | Severity | Count/Location |
|-------|----------|----------------|
| Giant classes | High | bot.h, bot_task.h, bot_fortress.h |
| Deep inheritance | Medium | 12+ interrupt class hierarchy |
| Custom collections | Medium | bot_genclass.h (1,070 lines) |
| Global variables | Medium | 420+ TODO/FIXME comments |
| Magic numbers | Low | Scattered throughout |
| Duplicate code | Low | Similar bot implementations |

### 4.5 Static Analysis

The codebase includes PVS-Studio headers indicating use of static analysis tools:
```cpp
// This is an open source non-commercial project. Dear PVS-Studio...
```

---

## 5. Security Audit

### 5.1 Critical Vulnerabilities

#### 5.1.1 Buffer Overflow - strcpy (CRITICAL)

**Affected Files:** 25+ locations
- `bot_globals.cpp` (lines 336, 343, 747, etc.)
- `bot_menu.cpp` (lines 531, 556)
- `bot_waypoint.cpp` (lines 1765, 1769, etc.)
- `bot_strings.cpp` (line 94)
- `bot_squads.h` (lines 157, 160, 163, 167)

**Example:**
```cpp
// bot_globals.cpp:336 - No bounds check
char arg_lwr[128];
std::strcpy(arg_lwr, name);  // name length not validated
```

**Risk:** Stack buffer overflow if input exceeds buffer size.

#### 5.1.2 Buffer Overflow - strcat (CRITICAL)

**Affected Files:** 15+ locations
- `bot_globals.cpp` (lines 753, 964, 1034, etc.)
- `bot_waypoint.cpp` (lines 1876-1877, 3207, etc.)
- `bot.cpp` (lines 1806-1807)

**Example:**
```cpp
// bot_globals.cpp:1034-1036
void CBotGlobals::addDirectoryDelimiter(char *szString)
{
    std::strcat(szString, "\\");  // No size check
}
```

#### 5.1.3 Buffer Overflow - sprintf (CRITICAL)

**Affected Files:** 26+ instances in `bot_task.cpp` alone

**Example:**
```cpp
// bot_task.cpp:302
void CBotTF2ShootLastEnemyPosition::debugString(char* string, unsigned bufferSize)
{
    std::sprintf(string, "...", m_vPosition.x, m_vPosition.y, m_vPosition.z);
    // bufferSize parameter exists but is NOT USED!
}
```

### 5.2 High Severity Issues

#### 5.2.1 Null Pointer Dereference

**Pattern:**
```cpp
// bot.cpp:314 - No null check after GetPlayerInfo
m_pPlayerInfo = playerinfomanager->GetPlayerInfo(m_pEdict);
std::strncpy(m_szBotName, m_pPlayerInfo->GetName(), 63);  // Could be nullptr
```

**Affected:** 50+ locations using `GetPlayerInfo`, `GetCollideable`, `GetIServerEntity`

#### 5.2.2 Unsafe Pointer Chains

**Pattern:**
```cpp
// bot_task.cpp:570 - get() dereference without null check
if (m_pBombTarget.get()->GetUnknown() == nullptr)
```

**Risk:** 15+ similar patterns found with `get()->method()` without validation

### 5.3 Security Summary Table

| Category | Count | Severity |
|----------|-------|----------|
| Buffer Overflow (strcpy) | 25+ | CRITICAL |
| Buffer Overflow (strcat) | 15+ | CRITICAL |
| Buffer Overflow (sprintf) | 26+ | CRITICAL |
| Null Pointer Dereference | 50+ | HIGH |
| Input Validation | 5 | HIGH |
| Format String | 1 | LOW |
| Command Injection | 0 | None |
| Hardcoded Credentials | 0 | None |

### 5.4 Files Requiring Immediate Attention

1. `utils/RCBot2_meta/bot_globals.cpp` - 25+ unsafe operations
2. `utils/RCBot2_meta/bot_task.cpp` - 26+ sprintf calls
3. `utils/RCBot2_meta/bot_waypoint.cpp` - strcpy/strcat chains
4. `utils/RCBot2_meta/bot_css_buying.cpp` - sprintf buffer overflow
5. `utils/RCBot2_meta/bot.cpp` - Multiple null dereference risks

---

## 6. Documentation Review

### 6.1 Documentation Quality

**Rating:** Excellent

The project has comprehensive documentation across multiple categories:

| Category | Files | Quality |
|----------|-------|---------|
| README | `README.md` | Excellent - comprehensive |
| API Reference | `docs/api.md` | Good - 70+ natives documented |
| Build Guide | `docs/building.md` | Good |
| Commands | `docs/commands.md` | Good |
| Configuration | `docs/configuration.md` | Good |
| Troubleshooting | `docs/troubleshooting.md` | Good |
| Waypoints | `docs/waypoints.md` | Good |
| ML Training | `docs/ML_TRAINING_GUIDE.md` | Good |
| ONNX Setup | `docs/ONNX_SETUP.md` | Good |

### 6.2 Roadmaps

| Document | Purpose | Status |
|----------|---------|--------|
| `roadmaps/roadmap.md` | 8-phase feature roadmap | Active |
| `roadmaps/roadmap-intelligence.md` | ML/AI development | Active |
| `roadmaps/roadmap-sourcemod.md` | SourceMod integration | Complete |
| `IMPLEMENTATION_PLAN.md` | Week-by-week ML guide | Active |

### 6.3 Example Code

The `scripting/` directory contains 10+ demonstration scripts:
- `rcbot_hldm_demo.sp` - HL2DM features
- `rcbot_tf2_test.sp` - TF2 features
- `rcbot_phase1_test.sp` through `rcbot_phase7_test.sp` - All SM phases
- `rcbot_extended_demo.sp` - Advanced patterns

### 6.4 Documentation Gaps

| Gap | Impact |
|-----|--------|
| Inline code comments | Low - some complex algorithms lack explanation |
| Test documentation | Medium - no test guide |
| Security guidelines | High - no security best practices |

---

## 7. Recommendations

### 7.1 Critical Priority (Immediate)

#### Security Fixes

1. **Replace unsafe string functions**
   - Replace all `strcpy` with `strncpy` or `std::string`
   - Replace all `strcat` with `strncat` or `std::string`
   - Ensure all `sprintf` calls use `snprintf` with actual size checking
   - Estimated effort: 2-3 days

2. **Add null pointer validation**
   - Add checks after all `GetPlayerInfo`, `GetCollideable`, `GetIServerEntity` calls
   - Wrap pointer chains with intermediate validation
   - Estimated effort: 1-2 days

3. **Validate input sizes**
   - Validate lengths before copying player names, map names, model names
   - Add size checks in configuration file parsing
   - Estimated effort: 1 day

### 7.2 High Priority (Short-term)

#### Code Quality

4. **Refactor giant classes**
   - Split `bot.h` (1,189 lines) into focused modules
   - Break down `bot_task.h` (1,630 lines)
   - Estimated effort: 1 week

5. **Replace custom containers**
   - Replace `bot_genclass.h` (1,070 lines) with STL containers
   - Use `std::vector`, `std::queue`, `std::deque`
   - Estimated effort: 2-3 days

6. **Standardize error handling**
   - Define consistent error handling strategy
   - Migrate to exceptions or standardized error codes
   - Estimated effort: 1 week

### 7.3 Medium Priority (Mid-term)

7. **Increase smart pointer usage**
   - Migrate raw pointers to `std::unique_ptr`/`std::shared_ptr` where safe
   - Reduce manual memory management
   - Estimated effort: 1-2 weeks

8. **Add test coverage**
   - Implement unit tests for core bot logic
   - Add integration tests for SourceMod extension
   - Estimated effort: 2-3 weeks

9. **Reduce global variable usage**
   - Apply dependency injection patterns
   - Reduce `CBotGlobals` static method count
   - Estimated effort: 1 week

### 7.4 Low Priority (Long-term)

10. **Template common patterns**
    - Extract template method pattern from repeated bot implementations
    - Reduce code duplication across game-specific classes
    - Estimated effort: 1-2 weeks

11. **Improve inline documentation**
    - Add comments to complex algorithms
    - Document design decisions
    - Estimated effort: Ongoing

12. **Security documentation**
    - Create secure coding guidelines
    - Document input validation requirements
    - Estimated effort: 2-3 days

---

## Appendix A: Git Submodules

| Submodule | Path | Purpose |
|-----------|------|---------|
| hl2sdk-tf2 | `alliedmodders/hl2sdk-tf2` | TF2 SDK |
| hl2sdk-hl2dm | `alliedmodders/hl2sdk-hl2dm` | HL2DM SDK |
| hl2sdk-dods | `alliedmodders/hl2sdk-dods` | DoD:S SDK |
| hl2sdk-css | `alliedmodders/hl2sdk-css` | CS:S SDK |
| hl2sdk-bms | `alliedmodders/hl2sdk-bms` | Black Mesa SDK |
| hl2sdk-sdk2013 | `alliedmodders/hl2sdk-sdk2013` | SDK 2013 |
| hl2sdk-orangebox | `alliedmodders/hl2sdk-orangebox` | Orange Box SDK |
| hl2sdk-episode1 | `alliedmodders/hl2sdk-episode1` | Episode 1 SDK |
| metamod-source | `alliedmodders/metamod-source` | Metamod SDK |
| sourcemod | `alliedmodders/sourcemod` | SourceMod SDK |
| hl2sdk-manifests | `hl2sdk-manifests` | SDK helper scripts |
| hl2sdk-proxy-repo | `alliedmodders/hl2sdk-proxy-repo` | SDK proxy |

---

## Appendix B: File Size Analysis

### Top 10 Largest Source Files

| File | Size | Lines |
|------|------|-------|
| `bot_fortress.cpp` | 263 KB | ~8,500 |
| `bot_task.cpp` | 137 KB | ~6,050 |
| `bot_dod_bot.cpp` | 106 KB | ~3,800 |
| `bot_waypoint.cpp` | 92 KB | ~4,000 |
| `entprops.cpp` | 75 KB | ~2,650 |
| `bot_weapons.cpp` | 39 KB | ~1,500 |
| `bot_events.cpp` | 34 KB | ~1,200 |
| `bot_globals.cpp` | 33 KB | ~1,100 |
| `bot_client.cpp` | 32 KB | ~1,100 |
| `bot_synergy.cpp` | 31 KB | ~1,100 |

---

## Appendix C: Acknowledgments

**Original Author:** Cheeseh
**Bot Base Code:** Botman's HPB Template
**Key Contributors:**
- APGRoboCop[CL] - Linux conversion
- pongo1231/Ducky1231 - TF2 support
- nosoop - SourceMod and AMBuild support
- caxanga334 - Synergy, TF2, MvM, CSS, AMBuild
- Technochips - TF2 Classic support
- RussiaTails - TF2 additional gamemodes
- Sappho - Linux Black Mesa, SDK2013 mathlib fix

---

**Audit Completed:** December 25, 2025
**Auditor:** Claude Code (Opus 4.5)
