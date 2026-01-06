# RCBot2 Development Guide

**Last Updated**: 2025-12-25
**Version**: 1.7+ (AI-Enhanced Fork)
**License**: GNU AGPL-3.0

---

## Table of Contents

1. [AI Agent Instructions](#1-ai-agent-instructions)
2. [Project Overview](#2-project-overview)
3. [Architecture](#3-architecture)
4. [Build System](#4-build-system)
5. [Code Organization](#5-code-organization)
6. [Development Guidelines](#6-development-guidelines)
7. [Security Considerations](#7-security-considerations)
8. [Testing](#8-testing)
9. [Contributing](#9-contributing)

---

## 1. AI Agent Instructions

### 1.1 Todo List Requirements

**MANDATORY**: When working on any task in this repository, the AI agent MUST generate a comprehensive, detailed todo list that:

1. **Breaks down the task** into discrete, actionable steps
2. **Includes all sub-tasks** - no step is too small to document
3. **Specifies file paths** where changes will be made
4. **Notes dependencies** between tasks (what must be done before what)
5. **Includes verification steps** (testing, building, validation)
6. **Estimates complexity** for each item (simple/moderate/complex)

### 1.2 Todo List Persistence

Every generated todo list MUST be saved to the `claude/` directory in the repository root using the following format:

**Filename Format**: `todo_YYYY-MM-DD_HH-MM-SS_nnnnnn.md`

- Uses **America/Toronto timezone** (Eastern Time)
- Atomic timestamp with microseconds for uniqueness
- Example: `todo_2026-01-05_14-32-45_123456.md`

**File Structure**:
```markdown
# Todo List: [Brief Task Description]

**Generated**: YYYY-MM-DD HH:MM:SS America/Toronto
**Task Summary**: [One-line description of the overall goal]

## Tasks

- [ ] Task 1 description
  - File(s): `path/to/file.cpp`
  - Complexity: simple|moderate|complex
  - Dependencies: none|Task N

- [ ] Task 2 description
  - File(s): `path/to/file.cpp`, `path/to/other.h`
  - Complexity: simple|moderate|complex
  - Dependencies: Task 1

## Verification

- [ ] Build succeeds
- [ ] Tests pass
- [ ] Documentation updated (if applicable)

## Notes

[Any additional context, considerations, or potential issues]
```

### 1.3 Todo List Usage

- **Create the todo list BEFORE starting work** on any non-trivial task
- **Update task status** as work progresses (mark items complete)
- **Add new tasks** discovered during implementation
- **Reference the todo file** in commit messages when relevant

### 1.4 Task Completion Protocol

**CRITICAL**: After completing EACH task item, the AI agent MUST:

1. **Update the todo file** - Mark the completed item with `[x]` and add a completion timestamp
2. **Re-review the entire todo list** - Before proceeding to the next task:
   - Verify the original goal is still being addressed
   - Confirm remaining tasks are still relevant and correctly ordered
   - Check that no scope creep or deviation has occurred
   - Ensure the next task aligns with the overall objective
3. **Add course-correction notes** if any deviation is detected in the `## Notes` section

This prevents the agent from "going off track" during long or complex implementations.

### 1.5 Todo List Retention Policy

**NEVER delete todo list files.** All todo lists are permanent records that:

- Provide audit trails for completed work
- Enable review of past decisions and approaches
- Support debugging if issues arise from previous changes
- Document the evolution of task understanding

### 1.6 Repository Synchronization

**MANDATORY**: Todo list files MUST be pushed to the repository:

1. **Initial commit** - Push the todo file immediately after creation
2. **Progress commits** - Push updates after completing each task item (or batch of related items)
3. **Final commit** - Ensure the completed todo file is pushed before ending the session

**Commit message format for todo updates:**
```
chore(todo): Update todo list - [brief description]

- Completed: [task description]
- Remaining: [N] tasks
```

---

## 2. Project Overview

### 2.1 What is RCBot2?

RCBot2 is an AI-powered bot plugin for Source Engine games, providing intelligent AI-controlled bots with advanced navigation, tactical decision-making, and game-specific behaviors. Originally created by Cheeseh, this enhanced fork adds ML/AI infrastructure, comprehensive SourceMod integration, and improved waypoint systems.

### 2.2 Supported Games

| Game | Status | Implementation | Notes |
|------|--------|----------------|-------|
| Half-Life 2: Deathmatch | Production | 925 lines | Primary ML target |
| Team Fortress 2 | Production | 8,500 lines | All 9 classes, MvM |
| Day of Defeat: Source | Production | 3,800 lines | All classes |
| Counter-Strike: Source | Beta | 1,100 lines | Buy menu WIP |
| Black Mesa | Beta | - | Cooperative |
| Synergy | Beta | - | Cooperative |

### 2.3 Key Features (This Fork)

- **ML/AI Infrastructure**: ONNX Runtime integration for behavior cloning and inference
- **SourceMod Integration**: 70+ natives across 7 implementation phases
- **Enhanced Waypoints**: Auto-generation, undo/redo, weapon tracking, compressed format
- **HL2DM Enhancements**: NPC combat system, cooperative mode, Gravity Gun API
- **Configuration-Based Gamemode Detection**: No recompilation for new maps

### 2.4 Codebase Statistics

| Metric | Value |
|--------|-------|
| Total Source Files | ~176 C++ files |
| Total Lines of Code | ~83,000 lines |
| Largest File | `bot_fortress.cpp` (8,500 lines) |
| Class Definitions | 1,864 |
| Git Submodules | 12 |

---

## 3. Architecture

### 3.1 Core Components

```
RCBot2 Plugin
├── Bot AI Core (bot.h/cpp, bot_task.cpp)
│   ├── Task Scheduling System
│   ├── Perception System
│   └── Navigation System
├── Game Implementations
│   ├── TF2 (bot_fortress.cpp) - All 9 classes
│   ├── HL2DM (bot_hldm_bot.cpp) - ML target
│   ├── DOD:S (bot_dod_bot.cpp)
│   └── CSS (bot_css_bot.cpp)
├── SourceMod Extension (sm_ext/)
│   └── 70+ natives for bot control
├── ML Infrastructure
│   ├── ONNX Runtime wrapper (bot_onnx.cpp)
│   ├── Feature extraction (bot_features.cpp)
│   ├── Gameplay recording (bot_recorder.cpp)
│   └── ML controller (bot_ml_controller.cpp)
└── Waypoint System
    ├── Navigation mesh (bot_waypoint.cpp)
    ├── Auto-generation (bot_waypoint_auto.cpp)
    ├── HL2DM extensions (bot_waypoint_hl2dm.cpp)
    └── Compressed format (bot_waypoint_compress.cpp)
```

### 3.2 Design Patterns

**Polymorphic Game Support**
```cpp
class CBot { /* Base class */ };
class CBotTF2 : public CBot { /* TF2-specific */ };
class CBotHLDM : public CBot { /* HL2DM-specific */ };
class CBotDOD : public CBot { /* DOD:S-specific */ };
```

**Task-Based AI**
- Hierarchical task scheduling with priority-based execution
- Tasks pushed to stack, executed in priority order
- Supports interruptions and complex multi-step behaviors
- Key schedules: `SCHED_ATTACK_ENEMY`, `SCHED_MOVE_TO_WAYPOINT`, `SCHED_SPY_SAP_BUILDING`

**Entity Handle System**
- `MyEHandle` class for safe entity references
- Validates serial numbers to prevent use-after-free
- More robust than raw pointers for entity tracking

**Singleton Pattern**
- Used for ML components, waypoint managers, NPC databases
- `CCoopModeDetector`, `CNPCDatabase`, `CNPCCombatManager`
- Proper deleted copy constructors and assignment operators

### 3.3 Directory Structure

```
rcbot2/
├── utils/RCBot2_meta/       # Main bot implementation (~71,000 lines)
│   ├── bot*.cpp/h           # Core AI systems
│   ├── bot_fortress.cpp     # TF2 implementation (largest file)
│   ├── bot_task.cpp         # Task scheduling
│   ├── bot_hldm_bot.cpp     # HL2DM implementation
│   └── ml/                  # ML header documentation
├── rcbot/                   # Shared utilities
│   ├── entprops.cpp/h       # Entity properties
│   ├── logging.cpp/h        # Logging (BSD-0)
│   └── tf2/                 # TF2 utilities
├── loader/                  # Metamod plugin loader
├── sm_ext/                  # SourceMod extension (70+ natives)
├── scripting/               # SourcePawn examples
├── config/                  # ML configuration
├── docs/                    # User documentation
├── guides/                  # Consolidated guides and roadmaps
├── package/                 # Installation templates
├── plugin-out/              # Built binaries
├── alliedmodders/           # SDK submodules
└── tools/                   # Build and ML training tools
```

---

## 4. Build System

### 4.1 Overview

RCBot2 uses **AMBuild2** (AlliedModders Build System) for cross-platform compilation.

- **C++ Standard**: C++17
- **Platforms**: Linux (x86, x64), Windows (x86, x64)
- **Compilers**: GCC/Clang (Linux), MSVC 2019+ (Windows)

### 4.2 Quick Build

```bash
# Clone with submodules
git clone --recursive https://github.com/ethanbissbort/rcbot2.git
cd rcbot2

# Linux automated build (recommended)
chmod +x build-linux.sh
./build-linux.sh --auto

# Or manual build
mkdir build-release && cd build-release
python3 ../configure.py \
    --hl2sdk-root=../alliedmodders \
    --mms-path=../alliedmodders/metamod-source \
    --sdks=hl2dm,tf2 \
    --enable-optimize
ambuild
```

### 4.3 Build Outputs

| Target | Platform | Size |
|--------|----------|------|
| `RCBot2Meta.x64.so` | Linux x64 | 32 KB |
| `rcbot.2.tf2.so` | Linux | 11.5 MB |
| `rcbot.2.hl2dm.so` | Linux | 11.5 MB |
| `rcbot.2.dods.so` | Linux | ~11 MB |
| `rcbot.2.css.so` | Linux | ~11 MB |

### 4.4 Dependencies

**Required:**
- GCC 9+ (Linux) or Visual Studio 2019+ (Windows)
- Python 3.8+
- AMBuild 2.2+
- MetaMod:Source SDK
- HL2SDK (game-specific)

**Optional:**
- SourceMod SDK (for extension)
- ONNX Runtime 1.16+ (for ML features)

### 4.5 Common Build Issues

| Issue | Solution |
|-------|----------|
| Missing `SdkHelpers.ambuild` | Run `git submodule update --init --recursive` |
| `IExtensionSys.h` not found | Initialize SourceMod submodule recursively |
| 32-bit library errors | `sudo apt install lib32z1-dev libc6-dev-i386` |
| `ambuild2.run` API error | Use modern `ambuild` command from build directory |

See `guides/BUILD_GUIDE.md` for complete build documentation.

---

## 5. Code Organization

### 5.1 Key Source Files

| File | Lines | Purpose |
|------|-------|---------|
| `bot.h/cpp` | ~2,500 | Core CBot class |
| `bot_fortress.cpp` | 8,500 | TF2 implementation (all 9 classes) |
| `bot_task.cpp` | 6,050 | Task scheduling system |
| `bot_waypoint.cpp` | 4,000 | Navigation system |
| `bot_weapons.cpp` | 1,500 | Weapon selection |
| `bot_hldm_bot.cpp` | 925 | HL2DM implementation |
| `entprops.cpp` | 2,650 | Entity properties |
| `bot_npc_combat.cpp` | 671 | NPC combat (HL2DM coop) |

### 5.2 Header Conventions

```cpp
#ifndef __BOT_EXAMPLE_H__
#define __BOT_EXAMPLE_H__

#include "bot.h"
#include "bot_globals.h"

class CExampleClass
{
public:
    CExampleClass();
    virtual ~CExampleClass();

private:
    int m_iMemberVar;        // m_ prefix for members
    float m_fFloatValue;     // Type hint: i=int, f=float, b=bool
    CBot* m_pBotPointer;     // p prefix for pointers
};

#endif
```

### 5.3 Naming Conventions

| Type | Convention | Example |
|------|------------|---------|
| Classes | CamelCase with C prefix | `CBot`, `CWaypoint` |
| Member variables | m_ prefix + Hungarian | `m_iHealth`, `m_pPlayer` |
| Global variables | g_ prefix | `g_GamemodeConfig` |
| Enums | e or E prefix | `eTFMapType`, `ENPCThreatLevel` |
| Constants | ALL_CAPS | `MAX_PLAYERS`, `BOT_THINK_RATE` |
| Functions | camelCase | `getHealth()`, `setTarget()` |

---

## 6. Development Guidelines

### 6.1 Code Style

- Use C++17 features where appropriate
- Prefer `std::vector`, `std::unique_ptr` over raw containers/pointers
- Use `nullptr` instead of `NULL`
- Keep functions under 100 lines when possible
- Add comments for complex algorithms

### 6.2 Memory Management

**Prefer smart pointers for new code:**
```cpp
// Good
std::unique_ptr<CBot> pBot = std::make_unique<CBot>();

// Avoid in new code
CBot* pBot = new CBot();
delete pBot;  // Easy to forget
```

**Use safe entity handles:**
```cpp
// Good - validates entity still exists
MyEHandle hEnemy;
hEnemy.set(pEnemy);
if (hEnemy.isValid()) {
    // Safe to use
}
```

### 6.3 Error Handling

**Check pointers before use:**
```cpp
// Good
IPlayerInfo* pInfo = playerinfomanager->GetPlayerInfo(pEdict);
if (pInfo == nullptr) {
    return;
}
const char* name = pInfo->GetName();

// Bad - potential crash
const char* name = playerinfomanager->GetPlayerInfo(pEdict)->GetName();
```

**Use safe string functions:**
```cpp
// Good
char buffer[128];
std::snprintf(buffer, sizeof(buffer), "Player: %s", name);

// Bad - buffer overflow risk
std::sprintf(buffer, "Player: %s", name);  // No size limit
```

### 6.4 Adding New Features

1. Create feature branch: `git checkout -b feature/my-feature`
2. Add header and implementation files
3. Update `AMBuilder` to include new source files
4. Add console commands in `bot_commands.cpp` if needed
5. Update documentation
6. Test on all relevant games
7. Submit pull request

### 6.5 Game-Specific Development

**TF2 Development** (`bot_fortress.cpp`):
- All 9 classes have unique behaviors
- Consider MvM, CTF, CP, PL, KOTH modes
- Test spy, engineer, medic interactions

**HL2DM Development** (`bot_hldm_bot.cpp`):
- Simplest game - good for testing core features
- Primary target for ML/AI development
- Test cooperative maps (coop_*, js_coop_*)

---

## 7. Security Considerations

### 7.1 Known Vulnerabilities

The codebase contains buffer handling issues that require attention:

| Issue | Count | Severity | Files |
|-------|-------|----------|-------|
| Unsafe `strcpy` | 25+ | CRITICAL | bot_globals.cpp, bot_waypoint.cpp |
| Unsafe `strcat` | 15+ | CRITICAL | bot_globals.cpp, bot.cpp |
| Unsafe `sprintf` | 26+ | CRITICAL | bot_task.cpp (26 instances) |
| Null dereference risks | 50+ | HIGH | Various API call sites |

### 7.2 Secure Coding Practices

**String Handling:**
```cpp
// Use safe alternatives
std::strncpy(dest, src, sizeof(dest) - 1);
dest[sizeof(dest) - 1] = '\0';

// Or use std::string
std::string name = pInfo->GetName();
```

**Buffer Operations:**
```cpp
// Always use size-limited functions
std::snprintf(buffer, sizeof(buffer), "Format: %s", value);
```

**Pointer Validation:**
```cpp
// Validate before chaining calls
IServerEntity* pEntity = pEdict->GetIServerEntity();
if (pEntity != nullptr) {
    ICollideable* pCollide = pEntity->GetCollideable();
    if (pCollide != nullptr) {
        Vector origin = pCollide->GetCollisionOrigin();
    }
}
```

### 7.3 Priority Files for Security Review

1. `utils/RCBot2_meta/bot_globals.cpp` - 25+ unsafe operations
2. `utils/RCBot2_meta/bot_task.cpp` - 26+ sprintf calls (ignores bufferSize param)
3. `utils/RCBot2_meta/bot_waypoint.cpp` - strcpy/strcat chains
4. `utils/RCBot2_meta/bot.cpp` - Null dereference risks

---

## 8. Testing

### 8.1 Manual Testing

**Basic Bot Testing:**
```
rcbot addbot              # Add a bot
rcbot debug 2             # Enable debug output
```

**ML Feature Testing:**
```
rcbot ml_record_start     # Start recording
rcbot ml_record_stop      # Stop recording
rcbot ml_features_dump    # Show extracted features
rcbot ml_model_test test  # Test ONNX model
```

**Waypoint Testing:**
```
rcbot wpt on              # Enable waypoint visibility
rcbot wpt add             # Add waypoint
rcbot wpt save            # Save waypoints
```

### 8.2 Game-Specific Testing

| Game | Test Focus |
|------|------------|
| TF2 | All 9 classes, MvM, multiple game modes |
| HL2DM | Deathmatch, cooperative maps, NPC combat |
| DOD:S | Capture points, bomb objectives |
| CSS | Combat, navigation (buy menu WIP) |

### 8.3 Performance Targets

- Feature extraction: <0.1ms per bot
- ONNX inference: <1.0ms per bot
- Bot think cycle: <1ms total per bot

---

## 9. Contributing

### 9.1 Getting Started

1. Fork the repository
2. Clone with submodules: `git clone --recursive`
3. Create feature branch
4. Make changes following coding guidelines
5. Test on relevant games
6. Submit pull request

### 9.2 Pull Request Guidelines

- Clear, descriptive title
- Explain what the change does and why
- Reference related issues
- Include testing notes
- Update documentation if needed

### 9.3 Code Review Checklist

- [ ] Follows naming conventions
- [ ] No new buffer overflow vulnerabilities
- [ ] Null checks on all pointer operations
- [ ] Updated AMBuilder if new files added
- [ ] Tested on at least one supported game
- [ ] Documentation updated

### 9.4 Resources

- **GitHub Issues**: [Report bugs](https://github.com/ethanbissbort/rcbot2/issues)
- **Discord**: [Bots United](https://discord.gg/5v5YvKG4Hr)
- **AMBuild Docs**: [AlliedModders Wiki](https://wiki.alliedmods.net/AMBuild)
- **Original RCBot2**: http://rcbot.bots-united.com/

---

## Appendix A: Console Commands Quick Reference

### Bot Management
| Command | Description |
|---------|-------------|
| `rcbot addbot` | Add a bot |
| `rcbot kickbot` | Kick a bot |
| `rcbot quota <n>` | Set bot count |
| `rcbot debug <level>` | Set debug level (0-5) |

### ML Commands
| Command | Description |
|---------|-------------|
| `rcbot ml_record_start` | Start recording gameplay |
| `rcbot ml_record_stop` | Stop recording |
| `rcbot ml_model_load <name> <path>` | Load ONNX model |
| `rcbot ml_enable <bot> <model>` | Enable ML for bot |
| `rcbot ml_features_dump` | Show feature vector |

### Waypoint Commands
| Command | Description |
|---------|-------------|
| `rcbot wpt on/off` | Toggle waypoint display |
| `rcbot wpt add` | Add waypoint at position |
| `rcbot wpt remove` | Remove nearest waypoint |
| `rcbot wpt save` | Save waypoints to file |

See `docs/commands.md` for complete reference.

---

## Appendix B: File Quick Reference

| Task | File(s) |
|------|---------|
| Add bot behavior | `bot_task.cpp`, `bot_schedule.cpp` |
| Add TF2 class feature | `bot_fortress.cpp` |
| Add HL2DM feature | `bot_hldm_bot.cpp` |
| Add console command | `bot_commands.cpp` |
| Add SourceMod native | `sm_ext/bot_sm_natives.cpp` |
| Add ML feature | `bot_onnx.cpp`, `bot_features.cpp`, `bot_ml_controller.cpp` |
| Modify waypoints | `bot_waypoint.cpp` |
| Add entity property | `rcbot/entprops.cpp` |

---

## Appendix C: ML/AI Quick Reference

### Architecture
```
Game State → Feature Extraction → ONNX Model → Action Decoding → Bot Commands
                                      ↓
                            Rule-Based Fallback (Hybrid Mode)
```

### HL2DM Feature Vector (56 features)
- [0-11] Self State: health, armor, position, velocity, weapon, ammo
- [12-35] Enemies: 4 enemies × 6 features (distance, angle, health)
- [36-47] Navigation: waypoints, cover, path, stuck
- [48-55] Pickups: health packs, ammo, weapons

### Why HL2DM First?
- Simplest game (no classes, no complex objectives)
- Smallest codebase (925 lines vs TF2's 8,500)
- Fewer features (56 vs TF2's 96+)
- Faster iteration for ML development
- 70-80% code reuse for TF2/DOD/CS:S expansion

---

**Original Author**: Cheeseh
**Fork Maintainer**: ethanbissbort
**Key Contributors**: nosoop, caxanga334, APGRoboCop, pongo1231, Technochips
