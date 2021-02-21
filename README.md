# RCBot2 for Windows and Linux (TF2, HL2:DM, DOD:S)

## Information

This is a fork of [the official RCBot2 plugin][rcbot2] written by Cheeseh.
Special thanks to pongo1231 for adding more TF2 support and NoSoop for adding AMBuild support and many more!

The [bots-united.com discord][] and [forums][bots-united forums] are the places to ask for
general RCBot2 support. I'm not present in either of those<strike>; file an issue on this repository if
you need support for this particular project</strike>. 

[rcbot2]: http://rcbot.bots-united.com/
[bots-united.com discord]: https://discord.gg/BbxR5wY
[bots-united forums]: http://rcbot.bots-united.com/forums/index.php?showforum=18

## Changes from upstream

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

## Installation

1. [Install MetaMod:Source][].
2. Download or build the RCBot2 package.
3. Extract the package into your game directory, similar to the process of installing MM:S.
4. Start the server.
5. To verify that the installation was successful, type `rcbotd` in your server console or RCON.
You should see multiple lines starting with "[RCBot]".

Things like the waypointing guide, hookinfo updater, and waypoints themselves are currently not
available here.  You can download those from the [official release thread][].  Waypoints are
also available at [this page][waypoints].

[Install MetaMod:Source]: https://wiki.alliedmods.net/Installing_Metamod:Source
[official release thread]: http://rcbot.bots-united.com/forums/index.php?showtopic=1994
[waypoints]: http://rcbot.bots-united.com/waypoints.php

## Building

### Cloning from source

RCBot2's repo history had all sorts of build artifacts / binaries at various points in time, so
pulling the repository down normally takes an unusually long while.  I'd highly recommend
passing in `--depth 1` or a few to avoid retrieving the files that were removed since then.

### Compiling on Windows / Linux

1. [Install the prerequisites for building SourceMod for your OS.][Building SourceMod]
2. Create a `build/` subdirectory, then run `configure.py`.
	- The project currently assumes GCC 5.4.0 (Ubuntu 16.04 LTS) on Linux, and MSVC version
	1900 (VC++2014.3 v14.00 last I checked).  Other compiler toolchains are not guaranteed to
	work at this time.
	- I use the following options (where `${MOD}` is only TF2):
	`python ../configure.py -s ${MOD} --mms_path ${MMS_PATH} --hl2sdk-root ${HL2SDK_ROOT}`
	- Specifying an `--sm-path` argument enables linking to SourceMod.
	- Note that the automatic versioning system requires an installation of `git` and a
	relatively modern version of Python 3.
3. Run `ambuild`.  MetaMod:Source plugin is built and the base install files will be available
in `build/package`.

[Building SourceMod]: https://wiki.alliedmods.net/Building_SourceMod

## Credits:

Cheeseh - Founder

Bot base code - Botman's HPB Template

Linux Conversion and waypointing - [APG]RoboCop[CL]

TF2 support and enhancements - Ducky1231/Pongo

SourceMod and AMBuild support - nosoop

Synergy support - Anonymous Player/caxanga334


## Waypointers:-

NightC0re

wakaflaka

dgesd

naMelesS

ati251

Sandman[SA]

Speed12	

MarioGuy3

Sjru	

Fillmore

htt123

swede

YouLoseAndIWin

madmax2
