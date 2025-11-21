# Claude.md - RCBot2 Development Guide

## Project Overview

RCBot2 is a bot plugin for Source Engine games, providing AI-controlled players for:
- Team Fortress 2 (TF2)
- Half-Life 2: Deathmatch (HL2:DM)
- Day of Defeat: Source (DOD:S)
- Counter-Strike: Source (CSS)
- Black Mesa
- Other Source Engine games

This is a fork of the official RCBot2 plugin by Cheeseh, with significant enhancements for build systems, mod compatibility, and game support.

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
│   └── [game]_*.cpp/h     # Game-specific implementations
├── rcbot/                 # Shared utilities and helpers
│   ├── entprops.cpp/h     # Entity property management
│   ├── helper.cpp/h       # Helper functions
│   ├── logging.cpp/h      # Logging system (BSD-0 license)
│   └── tf2/               # TF2-specific utilities
├── loader/                # Plugin loader shim
├── sm_ext/                # SourceMod extension natives
├── scripting/             # SourcePawn scripts
├── package/               # Installation files and configs
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

### Team Fortress 2

- **Most actively maintained** game support
- Complex class-specific behaviors (9 classes)
- MvM (Mann vs Machine) support being improved
- Multiple game modes: Payload, CTF, CP, KOTH, etc.
- Bots can build sentries (Engineer), heal (Medic), backstab (Spy)
- Known issues with:
  - New zombie infection maps (Scream Fortress XV)
  - Robot Destruction mode
  - Kart minigames from sd_doomsday_event
  - Engineer bot sentry placement orientation

### Day of Defeat: Source

- Historical warfare gameplay
- Capture point mechanics
- Limited bot classes compared to TF2

### Half-Life 2: Deathmatch

- Standard deathmatch gameplay
- Physics weapon support (gravity gun)

### Counter-Strike: Source

- Buy menu support needed
- Bomb planting/defusing logic
- Economy system awareness

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

## Important To-Do Items

Current development priorities (from README.md):

1. **MVM Upgrades**: Allow bots to use buy menu for MVM upgrades
2. **Game Detection**: Improve detection for non-listed Source gamemods
3. **TF2 Zombie Infection**: Support new maps from Scream Fortress XV
4. **Robot Destruction**: Prevent bot destruction when not ubered
5. **Engineer Bots**: Fix sentry turret orientation issues
6. **Demo Sticky Jumps**: Re-enable sticky jumping (check v1.7-beta)
7. **Medic/Spy AI**: Improve interaction with sentries
8. **Kart Minigames**: Fix bot behavior in kart game modes
9. **Class Changes**: Implement `CBotTF2::changeClass()` properly
10. **New Game Support**: TF2C, Black Mesa, CSS, Synergy, Dystopia

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

**Last Updated**: 2025-11-21
**Project**: RCBot2 for Source Engine Games
**Repository**: ethanbissbort/rcbot2
