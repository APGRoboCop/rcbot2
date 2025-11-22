# Claude.md - RCBot2 Development Guide

## Project Overview

RCBot2 is a bot plugin for Source Engine games, providing AI-controlled players for:
- **Half-Life 2: Deathmatch (HL2:DM)** - Primary development and ML target
- Team Fortress 2 (TF2)
- Day of Defeat: Source (DOD:S)
- Counter-Strike: Source (CSS)
- Black Mesa
- Other Source Engine games

This is a fork of the official RCBot2 plugin by Cheeseh, with significant enhancements for:
- **ML/AI Infrastructure** (ONNX Runtime integration, HL2DM-first approach)
- **Comprehensive SourceMod Integration** (70+ natives across 7 implementation phases)
- **Enhanced Waypoint System** (weapon pickups, interactive objects, NPC combat)
- **Build Systems** (AMBuild, multi-SDK support)
- **Game Compatibility** (extensive mod support and detection)

**License**: GNU Affero General Public License (AGPL-3.0)

## Architecture

### Core Components

1. **MetaMod:Source Plugin**: Main plugin interface with Source Engine
2. **Bot AI System**: Complex behavior system with waypointing, decision making, and game-specific logic
3. **SourceMod Extension** (optional): Native functions for SourcePawn scripting
4. **Loader Shim**: Loads mod-specific plugin builds (`rcbot.2.${MOD}`)

### Directory Structure

```
rcbot2/
├── utils/RCBot2_meta/     # Main bot implementation (C++)
│   ├── bot*.cpp/h         # Core bot AI, behavior, and game logic
│   ├── [game]_*.cpp/h     # Game-specific implementations
│   └── ml/                # ML/AI infrastructure (Phase 0)
│       ├── bot_ml_onnx.h      # ONNX Runtime integration
│       ├── bot_ml_features.h  # Feature extraction (HL2DM-first)
│       ├── bot_ml_controller.h # ML controller and hybrid AI
│       ├── bot_ml_recorder.h  # Gameplay recording
│       └── bot_ml_cvars.h     # ML console commands
├── rcbot/                 # Shared utilities and helpers
│   ├── entprops.cpp/h     # Entity property management
│   ├── helper.cpp/h       # Helper functions
│   ├── logging.cpp/h      # Logging system (BSD-0 license)
│   └── tf2/               # TF2-specific utilities
├── loader/                # Plugin loader shim
├── sm_ext/                # SourceMod extension natives (70+ natives)
├── scripting/             # SourcePawn scripts and examples
├── package/               # Installation files and configs
├── roadmaps/              # Development roadmaps
│   ├── roadmap.md             # Main feature roadmap
│   ├── roadmap-intelligence.md # AI/ML roadmap
│   ├── roadmap-sourcemod.md   # SourceMod integration roadmap
│   └── IMPLEMENTATION_NOTES.md # Implementation details
├── docs/                  # Comprehensive documentation
├── alliedmodders/         # AlliedModders build infrastructure
├── hl2sdk-manifests/      # HL2SDK configuration
├── versioning/            # Build versioning
└── tests/                 # Test files

```

## Key Files and Modules

### Core Bot Files (utils/RCBot2_meta/)

- `bot.cpp/h` - Main bot class and behavior management
- `bot_client.cpp/h` - Client/player interface
- `bot_commands.cpp/h` - Console commands
- `bot_configfile.cpp/h` - Configuration management
- `bot_navigator.cpp/h` - Navigation and pathfinding
- `bot_waypoint*.cpp/h` - Waypoint system for navigation
- `bot_schedule.cpp/h` - Task scheduling system
- `bot_weapons.cpp/h` - Weapon handling
- `bot_perceptron.cpp/h` - Neural network for decision making
- `bot_ga*.cpp/h` - Genetic algorithm components
- `bot_getprop.cpp/h` - Entity property retrieval
- `bot_sigscan.cpp/h` - Signature scanning for SDK functions

### Game-Specific Implementations

- `bot_tf2*.cpp/h` - Team Fortress 2 logic
  - `bot_tf2_points.h` - TF2 objective points
  - `bot_tf2_mod.cpp` - TF2 modifications
- `bot_css_bot.cpp` - Counter-Strike: Source
- `bot_black_mesa.cpp/h` - Black Mesa
- `bot_synergy.cpp/h` - Synergy mod
- `bot_coop.cpp/h` - Cooperative gameplay
- `bot_fortress.cpp/h` - Team Fortress Classic

### Utility Modules

- `bot_fortress_capture_point.cpp/h` - Capture point handling
- `bot_globals.cpp/h` - Global variables and definitions
- `bot_fortress_misc_classes.cpp/h` - Miscellaneous game classes
- `bot_mtrand.cpp/h` - Mersenne Twister random number generator
- `bot_mods.cpp/h` - Game mod detection
- `bot_menu.cpp/h` - Menu system
- `bot_profile.cpp/h` - Bot profiles and personalities

### ML/AI Infrastructure (utils/RCBot2_meta/ml/)

**Status**: Architecture defined, implementation pending (Phase 0)
**Primary Target**: **HL2DM** (Half-Life 2: Deathmatch) - simplest game, fastest iteration

#### Why HL2DM First?

The ML/AI roadmap takes an **HL2DM-first approach** for pragmatic reasons:
- ✅ **Simplest Game**: Pure deathmatch, no classes, no complex objectives
- ✅ **Smallest Codebase**: `bot_hldm_bot.cpp` only 884 lines vs TF2's 8,485 lines
- ✅ **Fewer Features**: 48-64 features vs TF2's 96+ features
- ✅ **Faster Training**: Simpler state/action space = faster ML convergence
- ✅ **Quick Iteration**: Simple game = quick testing cycles, easier debugging
- ✅ **Foundation for Expansion**: Once working, ~70-80% code reuse for TF2/DOD/CS:S

**Expansion Path**: HL2DM (Phase 0) → TF2 (Phase 0.5) → DOD:S/CS:S (Phase 0.5) → Advanced ML (Phase 1+)

#### ML Module Components

- **`bot_ml_onnx.h/cpp`** - ONNX Runtime wrapper for model inference
  - `CONNXModel` - Load and run ONNX models (<1ms inference target)
  - `CModelManager` - Manage multiple models for different games

- **`bot_ml_features.h/cpp`** - Feature extraction system
  - `IFeatureExtractor` - Base interface for all games
  - **`CHL2DMFeatureExtractor`** - HL2DM features (48-64 floats) - **PRIMARY TARGET**
  - `CTF2FeatureExtractor` - TF2 features (96 floats) - Phase 0.5
  - `CDODFeatureExtractor` - DOD:S features (~64 floats) - Phase 0.5
  - `CFeatureStatistics` - Track feature min/max/mean for normalization

- **`bot_ml_recorder.h/cpp`** - Gameplay recording for training data
  - `CBotRecorder` - Record bot/human gameplay states and actions
  - `CDemoParser` - Parse SourceTV demos for human data
  - `CDataAugmenter` - Augment training data (flip maps, vary speeds)

- **`bot_ml_controller.h/cpp`** - ML controller and hybrid AI
  - `CMLController` - Per-bot ML coordination
  - `CMLManager` - Global ML management
  - `CHybridAI` - Blend ML predictions with rule-based AI

- **`bot_ml_cvars.h/cpp`** - Console commands and variables
  - `rcbot_ml_load_model` - Load ONNX model
  - `rcbot_record_start/stop` - Record gameplay
  - `rcbot_ml_features_dump` - Debug feature extraction

#### ML Approaches Supported

1. **Behavior Cloning** (Phase 0 - HL2DM)
   - Train neural network to imitate human HL2DM players
   - Simplest approach, works on HL2DM first
   - Input: 48-64 feature vector → Output: 10 action values

2. **Reinforcement Learning** (Phase 1+)
   - DQN, PPO for autonomous learning
   - Requires Phase 0 infrastructure first

3. **Hybrid AI** (All phases)
   - ML for movement/aiming, rules for tactics
   - Graceful fallback to rule-based AI

For complete details, see:
- **[AI/ML Roadmap](roadmaps/roadmap-intelligence.md)** - Comprehensive ML strategy
- **[ML Implementation Plan](IMPLEMENTATION_PLAN.md)** - Week-by-week implementation guide
- **[ML Module README](utils/RCBot2_meta/ml/README.md)** - Architecture and usage

### SourceMod Integration (sm_ext/)

This fork includes **70+ SourceMod natives** across 7 implementation phases, providing complete programmatic control over bots:

**Phase 1: Enhanced Bot Command & Control**
- Direct bot movement, targeting, actions
- Weapon selection, forced attacks, reload control

**Phase 2: TF2-Specific Extensions**
- Class-specific behavior control
- Building placement and management
- MvM upgrade control

**Phase 3: Navigation & Pathfinding**
- Waypoint queries and path management
- Stuck detection and recovery
- Custom navigation goals

**Phase 4: Event System & Callbacks**
- Forward hooks for bot actions
- Event-driven bot control
- Real-time bot state monitoring

**Phase 5: Squad & Team Coordination**
- Squad creation and leadership
- Tactical commands and formations
- Team-wide behavior control

**Phase 6: Advanced Bot Management**
- Bot enumeration and filtering
- Lifecycle control and statistics
- Performance monitoring

**Phase 7: Perception & AI Configuration**
- FOV control and visibility queries
- Condition management
- AI tuning and difficulty scaling

**Example Scripts**: See `scripting/` directory for comprehensive examples demonstrating all phases

For complete API details, see:
- **[SourceMod Integration Roadmap](roadmaps/roadmap-sourcemod.md)** - Complete native reference
- **[API Documentation](docs/api.md)** - Usage examples and integration guide

### Waypoint System Enhancements

Recent enhancements to the waypoint system add support for:

**Weapon Pickups** (HL2DM focus):
- Waypoint flags for weapon locations (pistol, shotgun, SMG, AR2, etc.)
- Ammo crate locations
- Health and armor pickups
- Bots intelligently navigate to needed weapons/items

**Interactive Objects**:
- Buttons, doors, elevators
- Map-specific interactive elements
- Waypoint flags for contextual actions

**NPC Combat System** (HL2DM cooperative maps):
- NPC threat detection and prioritization
- Combine soldier, zombie, headcrab recognition
- Cooperative tactics for NPC encounters
- Health/ammo management during NPC fights

**Implementation Files**:
- `bot_waypoint.cpp/h` - Core waypoint system
- `bot_waypoint_locations.cpp/h` - Location management
- `bot_hldm_bot.cpp` - HL2DM-specific waypoint usage with NPC combat

For waypoint creation and usage, see:
- **[Waypoint Guide](docs/waypoints.md)** - Creating and managing waypoints
- **[Waypoint System Enhancements PR #17](https://github.com/ethanbissbort/rcbot2/pull/17)** - Implementation details

## Build System

### AMBuild Configuration

The project uses **AMBuild** (AlliedModders Build system) instead of Make or Visual Studio.

**Configuration Script**: `configure.py`

### Build Options

```bash
python configure.py \
  -s ${MOD} \                    # SDK: TF2, HL2DM, DODS, CSS, etc.
  --mms_path ${MMS_PATH} \       # MetaMod:Source path
  --hl2sdk-root ${HL2SDK_ROOT} \ # HL2SDK root directory
  --sm-path ${SM_PATH}           # Optional: SourceMod path
```

### Build Process

1. **Create build directory**: `mkdir build && cd build`
2. **Configure**: Run configure.py with appropriate paths
3. **Build**: Run `ambuild`
4. **Output**: Compiled plugins in `build/package/`

### Build Scripts

- `AMBuildScript` - Main AMBuild configuration
- `AMBuilder` - AMBuild helper
- `configure.py` - Build configuration script
- `PackageScript` - Package creation
- `BreakpadSymbols` - Breakpad symbol generation
- `premake5.lua` - Premake5 alternative build config

## Development Guidelines

### Code Style

1. **No `using namespace std;`** - Removed for clarity (see sdk-split branch)
2. **Indentation**: Follow existing style (appears to be tabs)
3. **Naming Conventions**:
   - Classes: `CBot`, `CBotTF2`, `CBotWeapon`
   - Member variables: `m_variableName`
   - Functions: `functionName()` (camelCase)
   - Constants: `CONSTANT_NAME`
4. **File Organization**: Each major subsystem has its own .cpp/.h pair

### Important Coding Patterns

1. **Entity Properties**: Use `entprops.cpp/h` for accessing entity properties
2. **Logging**: Use the logging system in `rcbot/logging.h`
3. **Memory Management**: Use proper cleanup, especially for dynamic allocations
4. **Game Detection**: Check game mod before using game-specific code
5. **Waypoint System**: Navigation relies heavily on pre-built waypoint files

### Common Pitfalls

1. **Don't assume game mod** - Always check which game is running
2. **SDK version differences** - Different games use different SDK versions
3. **Waypoint path changes** - Waypoints must be in `rcbot2/waypoints/${MOD}/`
4. **Memory leaks** - Recent commits show focus on mem fixes (see commit 3a3ed03)
5. **Signature scanning** - SDK signatures can change between game updates

## Game-Specific Notes

### Half-Life 2: Deathmatch ⭐ PRIMARY ML/AI TARGET

- **Primary development focus** for ML/AI features (Phase 0)
- Pure deathmatch gameplay - no classes, no complex objectives
- Physics weapon support (gravity gun, crossbow)
- **Enhanced Features**:
  - ✅ Comprehensive NPC combat system for cooperative maps
  - ✅ Weapon pickup waypoint system (pistol, shotgun, SMG, AR2, RPG, etc.)
  - ✅ Interactive object support (buttons, doors)
  - ✅ Health/ammo management during NPC encounters
  - ✅ SourceMod integration with Gravity Gun API
  - ✅ Event system for pickups, weapon changes, NPC kills
- **NPC Types Supported**: Combine soldiers, zombies, headcrabs, antlions
- **Codebase**: `bot_hldm_bot.cpp` (884 lines) - smallest, simplest game
- **ML Features**: 48-64 feature vector (simpler than TF2's 96)
- **Why HL2DM First?**: Fastest iteration, easiest debugging, foundation for TF2/DOD/CS:S expansion

### Team Fortress 2

- **Second most maintained** game support (after HL2DM focus shift)
- Complex class-specific behaviors (9 classes)
- MvM (Mann vs Machine) support being improved
- Multiple game modes: Payload, CTF, CP, KOTH, etc.
- Bots can build sentries (Engineer), heal (Medic), backstab (Spy)
- **Recent Fixes**:
  - ✅ Engineer bot sentry placement orientation (bot_fortress.cpp:1149)
  - ✅ Class change implementation (CBotTF2::changeClass)
  - ✅ Demoman sticky jumping (fully functional with waypoint flags)
- **Partial Implementations**:
  - 🔶 Zombie Infection map detection (AI needs work)
  - 🔶 Robot Destruction mode detection (core collection needs work)
  - 🔶 Basic Medic uber deployment (advanced sentry coordination pending)
- **Known Issues**:
  - MvM upgrade menu support (stub exists)
  - Kart minigames from sd_doomsday_event

### Day of Defeat: Source

- Historical warfare gameplay
- Capture point mechanics
- Limited bot classes compared to TF2
- **ML Expansion**: Phase 0.5 (after HL2DM working)

### Counter-Strike: Source

- Buy menu support needed
- Bomb planting/defusing logic
- Economy system awareness
- **ML Expansion**: Phase 0.5 (after HL2DM working)

## Testing and Debugging

### Testing Setup

1. Install MetaMod:Source on a test server
2. Build and install RCBot2
3. Load the appropriate mod (TF2, DOD:S, etc.)
4. Use console command `rcbotd` to verify installation
5. Use `rcbot_*` commands for testing

### Common Debug Commands

- `rcbotd` - Display RCBot debug info
- `rcbot_debug` - Enable debug mode
- `rcbot_create_class [class]` - Create a bot with specific class
- Console shows `[RCBot]` prefixed messages

### Debugging Tools

- Waypoint visualization tools
- Bot navigation debugging
- Hook info updater (from official release)

## Development Priorities and Roadmaps

### Current Status Summary

**Completed (Enhanced Branch)**:
- ✅ Engineer sentry orientation fix (bot_fortress.cpp:1149)
- ✅ Class change implementation (CBotTF2::changeClass)
- ✅ Demoman sticky jumping (fully functional)
- ✅ SourceMod integration (70+ natives, 7 phases)
- ✅ Waypoint system enhancements (weapons, interactables)
- ✅ NPC combat system for HL2DM cooperative maps
- ✅ HL2DM performance optimizations
- ✅ Enhanced game detection system

**In Progress**:
- 🔶 ML/AI infrastructure (Phase 0 - HL2DM first)
- 🔶 Advanced Medic uber coordination vs sentries
- 🔶 Zombie Infection full AI (detection complete)
- 🔶 Robot Destruction core collection AI

**High Priority (Next Steps)**:
1. **ML/AI Phase 0** - HL2DM behavior cloning (see roadmaps/roadmap-intelligence.md)
   - Data collection and ONNX integration
   - Feature extraction for HL2DM (48-64 features)
   - Behavior cloning model training and deployment
2. **MvM Upgrade Menu** - Bot upgrade purchasing in MvM
3. **Complete Zombie Infection AI** - Full AI for Scream Fortress maps
4. **Complete Robot Destruction** - Core collection behavior

**Future Priorities**:
- ML expansion to TF2, DOD:S, CS:S (Phase 0.5)
- Reinforcement learning (Phase 1+)
- Advanced game mode support (Kart minigames)
- Additional game support (TF2C, enhanced CS:S/Black Mesa)

### Development Roadmaps

The project has comprehensive roadmaps in `roadmaps/`:

1. **[roadmap.md](roadmaps/roadmap.md)** - Main feature roadmap (8 phases)
   - Core bot improvements and game mode support
   - TF2-specific features and fixes

2. **[roadmap-intelligence.md](roadmaps/roadmap-intelligence.md)** - AI/ML roadmap
   - **Phase 0**: HL2DM behavior cloning (foundation)
   - **Phase 0.5**: Expand to TF2, DOD:S, CS:S
   - **Phase 1+**: Reinforcement learning, advanced ML
   - HL2DM-first approach with expansion strategy

3. **[roadmap-sourcemod.md](roadmaps/roadmap-sourcemod.md)** - SourceMod integration
   - 7 phases, 70+ natives (100% complete)
   - Comprehensive bot control API

4. **[IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md)** - ML implementation guide
   - Week-by-week plan for Phase 0
   - Technical architecture and setup

5. **[IMPLEMENTATION_NOTES.md](roadmaps/IMPLEMENTATION_NOTES.md)** - Detailed notes
   - Implementation details and decisions
   - Code references and examples

## Integration Points

### MetaMod:Source Integration

- Plugin loads via MetaMod:Source VDF file
- Uses MetaMod API for server events
- Hooks game events and entity spawning

### SourceMod Integration (Optional)

- Exposes natives to SourcePawn via `sm_ext/`
- Natives defined in `bot_sm_natives.cpp`
- Allows SourceMod plugins to control bots
- Example: `ClassRestrictionsForBots.smx`

### Game Engine Integration

- Uses Source SDK for entity manipulation
- Signature scanning for dynamic SDK function resolution
- Game-specific hook management

## Resources and Community

### Official Resources

- **Original RCBot2**: http://rcbot.bots-united.com/
- **Official Forums**: http://rcbot.bots-united.com/forums/
- **Waypoints**: http://rcbot.bots-united.com/waypoints.php
- **Bots United Discord**: https://discord.gg/5v5YvKG4Hr

### SourceMod Resources

- **SourceMod Scripting Wiki**: https://wiki.alliedmods.net/Category:SourceMod_Scripting
- **SourceMod API Reference**: https://sm.alliedmods.net/new-api/

### This Fork

- **GitHub**: Current repository (file issues here)
- **Build System**: Uses AMBuild instead of Make/VS
- **Changes**: See "Changes from upstream" in README.md

## Development Workflow

### Making Changes

1. **Understand the system**: Read relevant bot_*.cpp files
2. **Check game-specific code**: Look in appropriate game module
3. **Test thoroughly**: Test on actual game servers
4. **Check waypoints**: Ensure waypoint compatibility
5. **Memory safety**: Use proper cleanup and sanity checks
6. **Multiple SDKs**: Consider impact on different game SDKs

### Committing Code

- Clear, descriptive commit messages
- Reference issues if applicable
- Test build process after changes
- Verify no regressions in other game modes

### Pull Requests

- Test on Linux and Windows if possible
- Document any new features or commands
- Update comments and documentation
- Consider backward compatibility

## Advanced Topics

### Waypoint System

- Pre-built navigation graphs for maps
- Visibility calculations between waypoints
- Path optimization and distance calculations
- Color coding for waypoint types
- Files in `bot_waypoint*.cpp/h`, `bot_wpt_*.cpp/h`

### AI Decision Making

- Neural network (perceptron) for learning
- Genetic algorithms for evolution
- Schedule-based task system
- Behavior trees for complex actions
- Profile-based personalities

### Performance Considerations

- Waypoint visibility caching
- Pathfinding optimization
- Entity property caching
- Efficient signature scanning
- Profiling support in `bot_profiling.h`

### Signature Scanning

- Locates game functions at runtime
- Handles SDK version differences
- Implementation in `bot_sigscan.cpp/h`
- Critical for cross-version compatibility

## Troubleshooting

### Build Issues

- Ensure HL2SDK paths are correct
- Check MetaMod:Source path
- Verify Python 3 and Git installed
- Check SDK manifests in `hl2sdk-manifests/`

### Runtime Issues

- Verify MetaMod:Source loads first
- Check game mod detection
- Ensure waypoints exist for map
- Review console for `[RCBot]` errors
- Check plugin VDF file

### Bot Behavior Issues

- Check waypoint quality for map
- Verify game-specific code for mod
- Review bot profiles and configs
- Check for recent game updates breaking signatures

## Credits and Contributors

See README.md for full contributor list. Key contributors:

- **Cheeseh** - Original creator
- **nosoop** - AMBuild support, SourceMod integration
- **Pongo/Ducky1231** - TF2 enhancements
- **caxanga334** - MvM, AMBuild support
- Plus many waypointers and testers

## License Notes

- Main code: **GNU AGPL-3.0** (requires source availability)
- `rcbot/logging.*`: **BSD-0 License** (public domain equivalent)
- Any modifications must be released under same license
- Source must be available to server players

## File Naming Conventions

- `bot_*.cpp/h` - Core bot functionality
- `bot_[game]*.cpp/h` - Game-specific code
- `bot_wpt*.cpp/h` - Waypoint system
- `bot_ga*.cpp/h` - Genetic algorithm
- `*_mod.cpp` - Game modification/detection

## Quick Reference

### Important Directories for Development

- `/utils/RCBot2_meta/` - Main bot code (start here)
- `/rcbot/` - Utility functions
- `/package/` - Installation files
- `/sm_ext/` - SourceMod natives

### Configuration Files

- `package/rcbot2.vdf` - MetaMod plugin definition
- `package/config/` - Bot configuration files

### Building Quick Reference

```bash
# Linux/Windows
cd rcbot2
mkdir build && cd build
python ../configure.py -s TF2 --mms_path ~/mms --hl2sdk-root ~/hl2sdk
ambuild
# Output in build/package/
```

### Common Bot Console Commands

- `rcbotd` - Bot debug info
- `rcbot_create` - Create bot
- `rcbot_change_classes` - Allow class changes
- `rcbot_debug` - Debug mode

---

**Last Updated**: 2025-11-22
**Branch**: enhanced
**Project**: RCBot2 for Source Engine Games
**Repository**: ethanbissbort/rcbot2

## Recent Enhancements (Enhanced Branch)

**November 2025 - Major Updates**:

1. **SourceMod Integration** (100% Complete)
   - 70+ natives across 7 implementation phases
   - Comprehensive bot control, navigation, squad coordination
   - TF2-specific and HL2DM-specific features
   - Example scripts in `scripting/`

2. **ML/AI Infrastructure** (Architecture Complete)
   - HL2DM-first approach for fastest iteration
   - ONNX Runtime integration designed
   - Feature extraction, recording, hybrid AI planned
   - Comprehensive roadmaps and implementation guides

3. **Waypoint System Enhancements**
   - Weapon pickup support (HL2DM focus)
   - Interactive object waypoints
   - NPC combat system for cooperative maps
   - Health/ammo management

4. **HL2DM Enhancements**
   - NPC threat detection and prioritization
   - Gravity Gun API integration
   - Event system for pickups, weapon changes, NPC kills
   - Performance optimizations

5. **TF2 Core Fixes**
   - Engineer sentry orientation (bot_fortress.cpp:1149)
   - Class change system (CBotTF2::changeClass)
   - Demoman sticky jumping
   - Enhanced game mode detection
