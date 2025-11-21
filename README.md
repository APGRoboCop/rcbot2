# RCBot2 for Windows and Linux

[![Build Status](https://github.com/ethanbissbort/rcbot2/workflows/build/badge.svg)](https://github.com/ethanbissbort/rcbot2/actions)
[![License: AGPL v3](https://img.shields.io/badge/License-AGPL%20v3-blue.svg)](https://www.gnu.org/licenses/agpl-3.0)

**AI-powered bots for Source Engine games**

RCBot2 is a MetaMod:Source plugin that adds intelligent AI bots to Source Engine games. Bots can navigate maps, understand objectives, use class-specific abilities, and provide challenging gameplay.

## Supported Games

| Game | Status | Features |
|------|--------|----------|
| **Team Fortress 2** | ✅ Production | All 9 classes, most game modes, MvM support |
| **Day of Defeat: Source** | ✅ Production | All classes, capture points, team tactics |
| **Half-Life 2: Deathmatch** | ✅ Production | Combat, navigation, physics weapons |
| **Counter-Strike: Source** | 🔶 Beta | Combat, navigation (buy menu WIP) |
| **Black Mesa** | 🔶 Beta | Cooperative gameplay, basic support |
| **Synergy** | 🔶 Beta | Cooperative gameplay, basic support |

## Quick Links

- 📖 **[Development Guide](claude.md)** - Comprehensive developer documentation
- 🗺️ **[Project Roadmap](roadmap.md)** - Feature roadmap and development priorities
- 🐛 **[Issue Tracker](https://github.com/ethanbissbort/rcbot2/issues)** - Report bugs or request features
- 💬 **[Bots United Discord](https://discord.gg/5v5YvKG4Hr)** - Community support and discussion
- 📚 **[Official Forums](http://rcbot.bots-united.com/forums/index.php?showforum=18)** - Original RCBot2 community

## About This Fork

This is a fork of [the official RCBot2 plugin][rcbot2] written by Cheeseh.
Special thanks to pongo1231 for adding TF2 support and nosoop for adding AMBuild support!

This fork includes significant improvements over the upstream version. For general RCBot2 support, visit the [Bots United Discord][bots-united.com discord] or [forums][bots-united forums]. For issues specific to this fork, please file an issue on this repository.

[rcbot2]: http://rcbot.bots-united.com/
[bots-united.com discord]: https://discord.gg/5v5YvKG4Hr
[bots-united forums]: http://rcbot.bots-united.com/forums/index.php?showforum=18

## Changes from upstream:-

- Build process uses [AMBuild][] instead of `make` or Visual Studio.  This removes the need for
Valve's cross platform make conversion tool and keeping copies of modified Source SDK files.
- The plugin has been split into SDK-specific builds to ensure proper compatibility, using the
same loader shim SourceMod uses to load mod-specific builds.
	- The shim is named `RCBot2Meta` to maintain compatibility with existing files; mod-specific
	plugins are named `rcbot.2.${MOD}`.
	- The `sdk-split` branch only contains modifications to get the project running on the
	new build tooling and SDK support without issues.  It should be fairly painless to merge
	(though it does remove `using namespace std;` for sanity).
- The usage of the install directory has been dropped.  In particular, waypoints must be located
under `rcbot2/waypoints/${MOD}` instead of nested under a folder matching the name of the
steamdir.
- Removed custom loadout and attribute support from the TF2 portion of the plugin. Other server
plugins (namely [tf2attributes][] and [TF2Items][], where the implementation was ported from)
are better-suited and maintained to handle that stuff; this plugin should only deal with bots
themselves.
- The Metamod:Source plugin can now optionally expose natives to SourceMod, adding some
functionality to control the RCBot2 plugin from SourcePawn.

[AMBuild]: https://wiki.alliedmods.net/AMBuild
[tf2attributes]: https://github.com/FlaminSarge/tf2attributes
[TF2Items]: https://github.com/asherkin/TF2Items

## Quick Start

### Installation

1. **[Install MetaMod:Source]** on your game server
2. **Download** the latest [RCBot2 release](https://github.com/ethanbissbort/rcbot2/releases) or build from source
3. **Extract** the package into your game directory (same process as installing MetaMod:Source)
4. **Start** your server
5. **Verify** installation by typing `rcbotd` in your server console - you should see lines starting with `[RCBot]`

### Getting Waypoints

Waypoints are required for bots to navigate maps:
- **Download waypoints** from the [official waypoint repository][waypoints]
- **Extract** to `rcbot2/waypoints/${GAME}/` (e.g., `rcbot2/waypoints/tf2/`)
- **Waypoint tools** available from the [official release thread][]

[Install MetaMod:Source]: https://wiki.alliedmods.net/Installing_Metamod:Source
[official release thread]: http://rcbot.bots-united.com/forums/index.php?showtopic=1994
[waypoints]: http://rcbot.bots-united.com/waypoints.php

### Basic Commands

```
rcbotd                    # Display bot debug information
rcbot_create             # Create a bot
rcbot_removeall          # Remove all bots
rcbot_quota <number>     # Set bot count
rcbot_debug <level>      # Set debug level (0-3)
```

For a full command list, see the [documentation](docs/commands.md).

## Building from Source

### Prerequisites

- [Install build prerequisites for SourceMod][Building SourceMod]
- Python 3.6+ (Python 2 is deprecated)
- Git
- MetaMod:Source SDK
- HL2SDK for target game(s)
- SourceMod SDK (optional, for natives)

### Quick Build

```bash
# Clone repository (use --depth 1 for faster clone)
git clone --depth 1 https://github.com/ethanbissbort/rcbot2.git
cd rcbot2

# Create and enter build directory
mkdir build && cd build

# Configure for your target game
python ../configure.py \
  -s TF2 \
  --mms-path /path/to/metamod-source \
  --hl2sdk-root /path/to/hl2sdk-root \
  --sm-path /path/to/sourcemod  # Optional

# Build
ambuild

# Output will be in build/package/
```

**Supported SDKs**: TF2, HL2DM, DODS, CSS, and others (use `-s` flag)

For detailed build instructions, see the [build guide](docs/building.md).

[Building SourceMod]: https://wiki.alliedmods.net/Building_SourceMod

## Documentation

Comprehensive documentation is available in the [`docs/`](docs/) directory:

- **[Development Guide](claude.md)** - Architecture, coding guidelines, and development workflow
- **[Project Roadmap](roadmap.md)** - Feature roadmap and development priorities
- **[Command Reference](docs/commands.md)** - Complete list of console commands
- **[Building Guide](docs/building.md)** - Detailed build instructions
- **[Configuration Guide](docs/configuration.md)** - Server and bot configuration
- **[Waypoint Guide](docs/waypoints.md)** - Creating and managing waypoints
- **[Troubleshooting](docs/troubleshooting.md)** - Common issues and solutions
- **[API Reference](docs/api.md)** - SourceMod natives and integration

## Contributing

We welcome contributions! Here's how you can help:

- **Report bugs** via [GitHub Issues](https://github.com/ethanbissbort/rcbot2/issues)
- **Submit code** via pull requests (see [Development Guide](claude.md))
- **Create waypoints** for popular maps (see [Waypoint Guide](docs/waypoints.md))
- **Improve documentation** (always appreciated!)
- **Test features** and provide feedback

See the [Roadmap](roadmap.md) for current priorities and planned features.

## License

RCBot2 is released under the [GNU Affero General Public License v3.0][AGPL].

**Important**: Any modifications must have sources available under the same license to players on your server.

**Note**: `rcbot/logging.{h,cpp}` is released separately under the [BSD Zero Clause License][BSD-0].

[AGPL]: https://spdx.org/licenses/AGPL-3.0-only.html
[BSD-0]: https://spdx.org/licenses/0BSD.html

## Current Development Status

### High Priority Items

See the full [Roadmap](roadmap.md) for details. Current priorities:

**Critical Bugs:**
- 🔴 Fix Engineer bot sentry turret orientation
- 🔴 Restore Demo bot sticky jumping (regression)

**High Priority Features:**
- 🟠 MvM upgrade menu support
- 🟠 Scream Fortress XV Zombie Infection maps
- 🟠 Robot Destruction game mode
- 🟠 Improved Medic/Spy AI vs sentries

**Medium Priority:**
- 🟡 Kart minigame support (sd_doomsday_event)
- 🟡 Implement `CBotTF2::changeClass()`
- 🟡 Better game detection for unlisted mods

**Future:**
- 🔵 TF2 Classic support
- 🔵 Enhanced CS:S features (buy menu, bomb defusal)
- 🔵 Additional Source game support

## Acknowledgments

- Founder - Cheeseh
- Bot base code - Botman's HPB Template
- Linux Conversion and Waypointing - [APG]RoboCop[CL]
- TF2 support and enhancements - Ducky1231/Pongo
- SourceMod and AMBuild support - nosoop
- Synergy, TF2, MvM, CSS and AMBuild support - Anonymous Player/caxanga334
- TF2 Classic support - Technochips
- TF2 additional gamemodes support - RussiaTails
- Linux Black Mesa and SDK2013 mathlib fix - Sappho
- Dystopia support - Soft As Hell
- Major waypointer for TF2 - LK777, RussiaTails, Witch, Francis, RevilleAJ, Waffle033
- Major waypointer for DoDS - INsane, Gamelarg05, genmac

## Waypointers:-

- NightC0re
- wakaflaka
- dgesd
- naMelesS
- ati251
- Sandman[SA]
- Speed12	
- MarioGuy3
- Sjru	
- Fillmore
- htt123
- swede
- YouLoseAndIWin
- ChiefEnragedDemo
- madmax2
- Pyri
- Softny		
- Wolf
- TesterYYY		
- darkranger		
- Emotional
- J@ck@l		
- YuriFR
- Otakumanu		
- 芝士人
- Eye of Justice
- TheSpyhasaGun (ScoutDogger)		
- NifesNforks
- parkourtrane
- assface
- Doc Lithius
- Kamaji
- Geralt
- Smoker		
- dzoo11
- Combine Soldier		
- cyglade
- TFBot_Maverick
- apdonato
- Sntr
- mehdichallenger
- Mikou
