# RCBot2 Development Roadmap

**Last Updated**: 2025-12-25
**Status**: Active Development

This document consolidates all development roadmaps and implementation plans.

---

## Table of Contents

1. [Feature Roadmap](#1-feature-roadmap)
2. [ML/AI Development](#2-mlai-development)
3. [SourceMod Integration](#3-sourcemod-integration)
4. [Implementation Progress](#4-implementation-progress)

---

## 1. Feature Roadmap

### Phase Overview

| Phase | Feature | Status | Priority |
|-------|---------|--------|----------|
| 0 | HL2DM Core Features | Completed | High |
| 1 | HL2DM Waypoint Enhancements | Completed | High |
| 2 | HL2DM Advanced Features | Completed | High |
| 3 | Engineer Sentry Orientation | Completed | Medium |
| 4 | Class Change Implementation | Completed | Medium |
| 5 | Demoman Sticky Jumping | Completed | Medium |
| 6 | Medic/Spy Improvements | Partial | Medium |
| 7 | MVM Upgrade System | Pending | Low |
| 8 | Zombie/Robot Modes | Partial | Low |
| 9 | Kart Game Support | Partial | Low |
| 10 | Advanced Features | Future | Low |

### Phase 0: HL2DM Core Features (Completed)

**Focus**: Establish solid HL2DM bot foundation for all future development.

**Completed Features**:
- **NPC Combat System**: Full combat AI for 28+ NPCs including Combine soldiers, zombies, antlions, etc.
  - Threat assessment and prioritization
  - NPC-specific combat tactics
  - Cover usage against NPCs
  - Implemented in `bot_npc_combat.h/cpp`

- **Performance Optimization**: Optimized think loop for HL2DM
  - Reduced CPU overhead per bot
  - Improved frame timing
  - Better spawn point utilization

- **Enhanced Game Detection**: Config-based gamemode detection
  - Flexible gamemode identification
  - Support for custom game modes
  - Configuration in `package/config/gamemodes.ini`

### Phase 1: HL2DM Waypoint Enhancements (Completed)

**Focus**: Advanced navigation system for HL2DM maps.

**Completed Features**:
- **Auto-generation**: Automatic waypoint placement system
  - Implemented in `bot_waypoint_auto.h/cpp`
  - Intelligent placement based on map geometry
  - Reduced manual waypoint creation time

- **Undo/Redo System**: Quality-of-life improvements for waypoint editing
  - Implemented in `bot_waypoint_undo.h/cpp`
  - Multi-level undo/redo support
  - Prevents accidental waypoint destruction

- **Compression**: Waypoint file size optimization
  - Implemented in `bot_waypoint_compress.h/cpp`
  - Smaller waypoint files
  - Faster loading times

- **HL2DM Extensions**: HL2DM-specific waypoint features
  - Implemented in `bot_waypoint_hl2dm.h/cpp`
  - Weapon pickup awareness
  - Health/armor charger locations
  - Gravity gun-specific waypoints

### Phase 2: HL2DM Advanced Features (Completed)

**Focus**: HL2DM-specific advanced AI capabilities.

**Completed Features**:
- **SourceMod HL2DM Natives**: Dedicated HL2DM control interface
  - Implemented in `bot_sm_natives_hldm.h/cpp`
  - HL2DM-specific bot commands
  - Example scripts in `scripting/rcbot_hldm_demo.sp`

- **Gravity Gun Tactics**: Advanced physics weapon handling
  - Object pickup and throw mechanics
  - Prop selection for combat
  - Environmental hazard usage

- **Item Priority System**: Smart item collection
  - Health/armor priority based on need
  - Weapon preference system
  - Ammo management

### Phase 3: Engineer Sentry Orientation (Completed)

**Problem**: Engineer bots placed sentries facing random directions.

**Solution**: Implemented sentry placement orientation in `bot_fortress.cpp:1149`:
- Calculate optimal facing direction toward high-traffic areas
- Consider enemy spawn points and objectives
- Account for nearby waypoint connections

### Phase 4: Class Change Implementation (Completed)

**Implementation**: `CBotTF2::changeClass()` method
- Bots can now change classes based on team composition
- Respects class limits and team balance
- Considers game mode requirements (e.g., more engineers in MvM)

### Phase 5: Demoman Sticky Jumping (Completed)

**Features**:
- Sticky jump waypoint flag detection
- Proper sticky placement timing
- Air control during jumps
- Landing zone prediction

### Phase 6: Medic/Spy Improvements (Partial)

**Completed**:
- Basic uber deployment logic
- Spy disguise and backstab detection

**Pending**:
- Advanced uber coordination vs. sentries
- Multi-uber push coordination
- Spy sapper priority logic

### Phase 7: MVM Upgrade System (Pending)

**Planned Features**:
- Buy menu navigation
- Upgrade priority logic per class
- Economy tracking
- Team coordination for upgrades

### Phase 8: Zombie/Robot Modes (Partial)

**Completed**:
- Zombie Infection map detection
- Robot Destruction mode detection

**Pending**:
- Full AI for Scream Fortress zombie maps
- Core collection behavior in Robot Destruction

### Phase 9: Kart Game Support (Partial)

**Status**: Basic detection for sd_doomsday_event kart minigames

### Phase 10: Advanced Features (Future)

- TF2 Classic support
- Enhanced CS:S buy menu
- Additional game mode support

---

## 2. ML/AI Development

### Philosophy: HL2DM-First Approach

The ML/AI development takes an **HL2DM-first approach** for pragmatic reasons:

| Factor | HL2DM | TF2 |
|--------|-------|-----|
| Codebase Size | 925 lines | 8,500 lines |
| Feature Count | 56 features | 96+ features |
| Complexity | Pure deathmatch | 9 classes, 10+ modes |
| Iteration Speed | Fast | Slow |

**Expansion Path**: HL2DM (Phase 0) → TF2 (Phase 0.5) → DOD:S/CS:S → Advanced ML (Phase 1+)

### Phase 0: HL2DM Foundation (In Progress)

**Status**: Architecture complete, implementation ready

#### Priority 1: Data Collection (Completed)
- `bot_replay_format.h` - Frame capture structure
- `bot_recorder.h/cpp` - Recording system with circular buffer
- Binary, JSON, CSV export formats
- Console commands: `ml_record_start`, `ml_record_stop`, etc.

#### Priority 2: ONNX Runtime Integration (Completed)
- `bot_onnx.h/cpp` - ONNX model loading and inference
- `CONNXModel` class with performance tracking
- `CONNXModelManager` for multiple models
- Target: <0.5ms inference per bot

#### Priority 3: Feature Extraction (Completed)
- `bot_features.h/cpp` - Feature extraction system
- `CHL2DMFeatureExtractor` - 56 features for HL2DM
- Normalization helpers for health, distance, angles
- Console commands: `ml_features_dump`, `ml_features_test`

#### Priority 4: Behavior Cloning (Completed)
- `bot_ml_controller.h/cpp` - ML controller system
- `tools/train_hl2dm_behavior_clone.py` - Training pipeline
- Action smoothing and hybrid AI fallback
- Console commands: `ml_enable`, `ml_disable`, `ml_status`

### Phase 0.5: Game Expansion (Planned)

**Target Games**: TF2, DOD:S, CS:S

**Approach**:
- Transfer learning from HL2DM models
- Extended feature vectors per game
- Game-specific action spaces

**TF2 Features** (96 planned):
- Self state (12): health, position, velocity, class, etc.
- Enemies (32): 4 enemies × 8 features
- Allies (24): 4 allies × 6 features
- Objectives (16): flags, points, carts
- Environment (12): ammo, health packs, hazards

### Phase 1+: Advanced ML (Future)

- Reinforcement learning (DQN, PPO)
- Online learning during gameplay
- Multi-agent coordination
- Transfer learning between games

### ML Console Commands

| Command | Description |
|---------|-------------|
| `rcbot ml_record_start` | Start recording gameplay |
| `rcbot ml_record_stop` | Stop recording |
| `rcbot ml_record_save <file>` | Save recording to binary |
| `rcbot ml_record_export_json <file>` | Export to JSON |
| `rcbot ml_model_load <name> <path>` | Load ONNX model |
| `rcbot ml_model_unload <name>` | Unload model |
| `rcbot ml_model_list` | List loaded models |
| `rcbot ml_model_test <name>` | Test inference |
| `rcbot ml_model_benchmark <name>` | Benchmark performance |
| `rcbot ml_features_dump` | Show feature vector |
| `rcbot ml_enable <bot> <model>` | Enable ML for bot |
| `rcbot ml_disable <bot>` | Disable ML for bot |

---

## 3. SourceMod Integration

### Status: 100% Complete (70+ Natives)

All 7 phases have been implemented and tested.

### Phase 1: Bot Command & Control

| Native | Description |
|--------|-------------|
| `RCBot2_SetBotEnemy()` | Set bot's target enemy |
| `RCBot2_GetBotEnemy()` | Get current enemy |
| `RCBot2_ForceBotAction()` | Force specific action |
| `RCBot2_GetBotTeam()` | Get bot's team |
| `RCBot2_GetBotHealth()` | Get health value |
| `RCBot2_GetBotOrigin()` | Get position |
| `RCBot2_GotoOrigin()` | Navigate to position |
| `RCBot2_StopMovement()` | Stop bot movement |

### Phase 2: TF2-Specific Extensions

| Native | Description |
|--------|-------------|
| `RCBot2_TF2_GetClass()` | Get bot's TF2 class |
| `RCBot2_TF2_SetClass()` | Set bot's TF2 class |
| `RCBot2_TF2_BuildSentry()` | Build sentry gun |
| `RCBot2_TF2_BuildDispenser()` | Build dispenser |
| `RCBot2_TF2_UpgradeBuilding()` | Upgrade building |

### Phase 3: Navigation & Pathfinding

| Native | Description |
|--------|-------------|
| `RCBot2_GetWaypointCount()` | Total waypoints |
| `RCBot2_GetNearestWaypoint()` | Find nearest waypoint |
| `RCBot2_GetWaypointOrigin()` | Get waypoint position |
| `RCBot2_HasPath()` | Check if has path |
| `RCBot2_ClearPath()` | Clear current path |
| `RCBot2_IsStuck()` | Check if stuck |

### Phase 4: Event System & Callbacks

| Forward | Description |
|---------|-------------|
| `RCBot2_OnBotSpawn()` | Bot spawned |
| `RCBot2_OnBotDeath()` | Bot died |
| `RCBot2_OnBotThink()` | Bot think cycle |
| `RCBot2_OnBotPathComplete()` | Path completed |

### Phase 5: Squad & Team Coordination

| Native | Description |
|--------|-------------|
| `RCBot2_CreateSquad()` | Create new squad |
| `RCBot2_DestroySquad()` | Destroy squad |
| `RCBot2_AddBotToSquad()` | Add bot to squad |
| `RCBot2_GetSquadLeader()` | Get squad leader |
| `RCBot2_GetSquadMemberCount()` | Count members |

### Phase 6: Advanced Bot Management

| Native | Description |
|--------|-------------|
| `RCBot2_KickBot()` | Kick specific bot |
| `RCBot2_CountBots()` | Count all bots |
| `RCBot2_GetBotByIndex()` | Get bot by index |
| `RCBot2_SaveProfile()` | Save bot profile |
| `RCBot2_LoadProfile()` | Load bot profile |
| `RCBot2_GetBotKills()` | Get kill count |

### Phase 7: Perception & AI Configuration

| Native | Description |
|--------|-------------|
| `RCBot2_SetBotFOV()` | Set field of view |
| `RCBot2_GetVisibleEnemies()` | Get visible enemies |
| `RCBot2_SetCondition()` | Set AI condition |
| `RCBot2_HasCondition()` | Check condition |
| `RCBot2_SetWeaponPreference()` | Set weapon priority |

### Example Scripts

See `scripting/` directory for examples:
- `rcbot_hldm_demo.sp` - HL2DM features
- `rcbot_tf2_test.sp` - TF2 features
- `rcbot_phase1_test.sp` through `rcbot_phase7_test.sp` - Phase tests
- `rcbot_extended_demo.sp` - Advanced patterns

---

## 4. Implementation Progress

### Completed Features

| Feature | Status | Commit/Notes |
|---------|--------|--------------|
| Enhanced Game Detection | Completed | Config-based gamemode detection |
| SourceMod Integration | Completed | 70+ natives, 7 phases |
| Waypoint Enhancements | Completed | Auto-gen, undo/redo, compression |
| HL2DM NPC Combat | Completed | 28+ NPCs, threat assessment |
| HL2DM Performance | Completed | Optimized think loop |
| TF2 Sentry Orientation | Completed | Facing direction logic |
| TF2 Class Changes | Completed | Dynamic class switching |
| Demoman Sticky Jumping | Completed | Full implementation |
| ML Data Collection | Completed | Recording system |
| ONNX Integration | Completed | Model loading/inference |
| Feature Extraction | Completed | HL2DM 56-feature vector |
| Behavior Cloning | Completed | Training pipeline |

### In Progress

| Feature | Status | Notes |
|---------|--------|-------|
| ML Model Training | Awaiting Data | Needs human gameplay recordings |
| Medic Uber Coordination | Partial | Basic deployment works |
| Spy Sapper Logic | Partial | Basic sapping works |

### Planned

| Feature | Priority | Timeline |
|---------|----------|----------|
| TF2 Feature Extraction | Medium | Phase 0.5 |
| MVM Upgrade System | Medium | - |
| CS:S Buy Menu | Medium | - |
| Reinforcement Learning | Low | Phase 1+ |

---

## Appendix: File References

### ML/AI Files
- `utils/RCBot2_meta/bot_onnx.h/cpp` - ONNX wrapper
- `utils/RCBot2_meta/bot_features.h/cpp` - Feature extraction
- `utils/RCBot2_meta/bot_recorder.h/cpp` - Gameplay recording
- `utils/RCBot2_meta/bot_ml_controller.h/cpp` - ML controller
- `utils/RCBot2_meta/bot_replay_format.h` - Recording format
- `utils/RCBot2_meta/bot_ml_commands.cpp` - ML console commands
- `tools/train_hl2dm_behavior_clone.py` - Training pipeline
- `tools/create_test_model.py` - Test model generator

### SourceMod Files
- `sm_ext/bot_sm_natives.h/cpp` - Core natives
- `sm_ext/bot_sm_natives_tf2.h/cpp` - TF2 natives
- `sm_ext/bot_sm_natives_hldm.h/cpp` - HL2DM natives
- `sm_ext/bot_sm_forwards.h/cpp` - Forwards
- `sm_ext/bot_sm_events.h/cpp` - Events
- `scripting/include/rcbot2.inc` - Include file

### Waypoint Files
- `utils/RCBot2_meta/bot_waypoint.h/cpp` - Core system
- `utils/RCBot2_meta/bot_waypoint_auto.h/cpp` - Auto-generation
- `utils/RCBot2_meta/bot_waypoint_undo.h/cpp` - Undo/redo
- `utils/RCBot2_meta/bot_waypoint_compress.h/cpp` - Compression
- `utils/RCBot2_meta/bot_waypoint_hl2dm.h/cpp` - HL2DM extensions
- `utils/RCBot2_meta/bot_npc_combat.h/cpp` - NPC combat

### Configuration Files
- `package/config/gamemodes.ini` - Gamemode detection
- `config/ml_models.json` - ML model config
- `config/ml_features.json` - Feature definitions
