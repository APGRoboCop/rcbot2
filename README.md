# RCBot2 for Windows and Linux (TF2, HL2:DM, DOD:S)

## Information:-

This is a fork of [the official RCBot2 plugin][rcbot2] written by Cheeseh.
Special thanks to pongo1231 for adding more TF2 support and NoSoop for adding AMBuild support and many more!

The [bots-united.com discord][] and [forums][bots-united forums] are the places to ask for
general RCBot2 support. I'm not present in either of those; file an issue on this repository if
you need support for this particular project. 

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

## Installation:-

1. [Install MetaMod:Source].
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

## Building:-

### Cloning from source

RCBot2's repo history had all sorts of build artifacts / binaries at various points in time, so
pulling the repository down normally takes an unusually long while.  I'd highly recommend
passing in `--depth 1` or a few to avoid retrieving the files that were removed since then.

### Compiling on Windows / Linux

1. [Install the prerequisites for building SourceMod for your OS.][Building SourceMod]
2. Create a `build/` subdirectory, then run `configure.py`.
	- Use the following options (where `${MOD}` is only TF2):
	`python ../configure.py -s ${MOD} --mms-path ${MMS_PATH} --hl2sdk-root ${HL2SDK_ROOT}`
	- Specifying an `--sm-path ${SM_PATH}` argument enables linking to SourceMod.
	- Note that the automatic versioning system requires an installation of `git` and a
	relatively modern version of Python 3. Python version 2 is now deprecated.
	i.e.
	Using bash:
	```
	python ../configure.py -s hl2dm --mms-path "${PWD%/*}/alliedmodders/metamod-source" \
	--hl2sdk-root "${PWD%/*}/alliedmodders" --sm-path "${PWD%/*}/alliedmodders/sourcemod"
	```
	Using Powershell:
	```
	python ../configure.py -s hl2dm --mms-path "$((Get-Item $pwd).parent.FullName)\alliedmodders\metamod-source" `
	--hl2sdk-root "$((Get-Item $pwd).parent.FullName)\alliedmodders" `
	--sm-path "$((Get-Item $pwd).parent.FullName)\alliedmodders\sourcemod"
	```
3. Run `ambuild`.  MetaMod:Source plugin is built and the base install files will be available
in `build/package`.

[Building SourceMod]: https://wiki.alliedmods.net/Building_SourceMod

## License:-

RCBot2 is released under the [GNU Affero General Public License][].  Among other things, this
means that any modifications you make to RCBot2 must have the sources available under the same
license to players on your server.

Additionally, `rcbot/logging.{h,cpp}` is released separately under the
[BSD Zero Clause License][].

[GNU Affero General Public License]: https://spdx.org/licenses/AGPL-3.0-only.html
[BSD Zero Clause License]: https://spdx.org/licenses/0BSD.html

### To-do:-

- To allow bots to menuselect in order to buy upgrades for MVM
- To improve game detection for non-listed Source gamemods
- To add proper support for the new Zombie Infection TF2 maps since Scream Fortress XV update
- To add proper support for Robot Destruction gameplay by destroying bots when not ubered
- To prevent EngiBots from facing their SG Turrets the wrong way
- To prevent bots from shooting at ghost players - like in plr_hightower_event Hell Zone
- To allow bots to attack Skeleton Mobsters in pl_spineyard
- To improve on how Medic and Spy bots to behave smarter and properly when interacting with SG Turrets and Healing/Ubering
- Bots needs to understand how to play Kart games from sd_doomsday_event as they only wonder around those minigames
- CBotTF2::changeClass needs to be implemented to avoid bots punting when using ClassRestrictionsForBots.smx and `rcbot_change_classes 1`
- And to add TF2C support along with Black Mesa Source, Counter-Strike: Source, Synergy and Dystopia

## Credits:-

- Founder - Cheeseh
- Bot base code - Botman's HPB Template
- Linux Conversion and Waypointing - [APG]RoboCop[CL]
- TF2 support and enhancements - Ducky1231/Pongo
- SourceMod and AMBuild support - nosoop
- Synergy, MvM and CSS support - Anonymous Player/caxanga334
- TF2 Classic support - Technochips
- Linux Black Mesa and SDK2013 mathlib fix - Sappho
- Dystopia support - Soft As Hell
- Major waypointer for TF2 - LK777, RussiaTails, Witch, Francis, RevilleAJ
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
