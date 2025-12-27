# RCBot2 Usage Guide

Complete guide to installing, configuring, and using RCBot2 on your Source engine server.

---

## Table of Contents

- [Overview](#overview)
- [Installation](#installation)
- [Quick Start](#quick-start)
- [Command Reference](#command-reference)
- [Configuration](#configuration)
- [SourceMod API](#sourcemod-api)
- [Troubleshooting](#troubleshooting)
- [Getting Help](#getting-help)

---

## Overview

RCBot2 is an advanced AI bot plugin for Source engine games including:

- Team Fortress 2
- Day of Defeat: Source
- Half-Life 2: Deathmatch
- Counter-Strike: Source

### Features

- Intelligent bot navigation using waypoints
- Game-specific AI behavior
- Class-based strategies (TF2)
- Configurable skill levels
- SourceMod integration (optional)
- Automatic waypoint testing and refinement

---

## Installation

### Requirements

- Source Engine dedicated server (TF2, DOD:S, HL2:DM, CS:S)
- MetaMod:Source 1.10+ installed
- SourceMod (optional, for extended functionality)

### Quick Install

1. **Install MetaMod:Source** on your server
   - [MetaMod:Source Installation Guide](https://wiki.alliedmods.net/Installing_Metamod:Source)

2. **Download RCBot2**
   - [Latest Release](https://github.com/ethanbissbort/rcbot2/releases)

3. **Extract to game directory**
   - Extract into your game folder (e.g., `tf/` or `cstrike/`)

4. **Download waypoints**
   - [Waypoint Repository](http://rcbot.bots-united.com/waypoints.php)
   - Extract to `rcbot2/waypoints/{game}/`

5. **Restart server**

6. **Verify installation**
   ```
   rcbotd
   ```

### File Structure

```
{game}/
├── addons/
│   ├── metamod.vdf
│   └── rcbot2meta/
│       ├── bin/
│       │   └── rcbot.2.{game}.so
│       └── rcbot2meta.vdf
└── rcbot2/
    ├── config/
    │   ├── config.ini
    │   └── bot_profiles.ini
    └── waypoints/
        └── {game}/
            └── {mapname}.rcw
```

---

## Quick Start

### Adding Bots

```
rcbot addbot               // Create one bot
rcbot_quota 8              // Set bot count to 8
rcbot addbot soldier       // Create specific class (TF2)
```

### Managing Bots

```
rcbot kickbot              // Remove one bot
rcbot_removeall            // Remove all bots
rcbot debug 2              // Enable debug output
```

### Waypoint Commands

```
rcbot wpt on               // Show waypoints
rcbot wpt load             // Load waypoints
rcbot wpt save             // Save waypoints
```

---

## Command Reference

### General Commands

| Command | Description |
|---------|-------------|
| `rcbotd` | Display RCBot2 info and commands |
| `rcbot help` | Show command help |

### Bot Management

| Command | Description |
|---------|-------------|
| `rcbot addbot [name] [class] [team]` | Create a bot |
| `rcbot kickbot [team]` | Remove a bot |
| `rcbot_removeall` | Remove all bots |
| `rcbot control <name>` | Take control of a bot |

**Examples:**
```
rcbot addbot                    // Random bot
rcbot addbot BotName            // Named bot
rcbot addbot BotName soldier    // Soldier bot (TF2)
rcbot addbot BotName heavy 2    // Heavy on team 2
rcbot kickbot                   // Kick random bot
rcbot kickbot 2                 // Kick from team 2
```

### Waypoint Commands

| Command | Description |
|---------|-------------|
| `rcbot wpt on` | Show waypoints |
| `rcbot wpt off` | Hide waypoints |
| `rcbot wpt add [type]` | Add waypoint |
| `rcbot wpt delete` | Delete nearest waypoint |
| `rcbot wpt save` | Save waypoints |
| `rcbot wpt load` | Load waypoints |
| `rcbot wpt info` | Show waypoint info |
| `rcbot wpt clear` | Delete all waypoints |
| `rcbot wpt givetype <type>` | Change waypoint type |
| `rcbot autowaypoint <on\|off>` | Toggle auto-waypointing |
| `rcbot pathwaypoint create` | Create path connection |
| `rcbot pathwaypoint remove` | Remove path connection |

**Waypoint Types:**
- `jump` - Jump waypoint
- `crouch` - Crouch waypoint
- `ladder` - Ladder waypoint
- `wait` - Wait/camp waypoint
- `sniper` - Sniper spot
- `sentry` - Sentry position (TF2)
- `health` - Health pack
- `ammo` - Ammo pack
- `resupply` - Resupply locker

### Debug Commands

| Command | Description |
|---------|-------------|
| `rcbot debug [level] [bot]` | Set debug level (0-3) |
| `rcbot util teleport` | Teleport to waypoint |
| `rcbot util printent` | Print entity info |

**Debug Levels:**
- `0` - No debug
- `1` - Basic info
- `2` - Detailed info
- `3` - Verbose output

### Nav-Test Commands

| Command | Description |
|---------|-------------|
| `rcbot navtest start` | Start automated testing |
| `rcbot navtest stop` | Stop testing |
| `rcbot navtest status` | Show test status |
| `rcbot navtest report` | Generate issue report |

### Auto-Refine Commands

| Command | Description |
|---------|-------------|
| `rcbot autorefine analyze` | Analyze nav-test results |
| `rcbot autorefine suggest` | Show suggestions |
| `rcbot autorefine apply` | Apply suggestions |
| `rcbot autorefine undo` | Undo changes |

### Tactical Commands

| Command | Description |
|---------|-------------|
| `rcbot tactical status` | Show tactical state |
| `rcbot tactical playstyle <type>` | Set playstyle |
| `rcbot tactical debug` | Toggle debug |

---

## Configuration

### Configuration Files

Located in `{game}/rcbot2/config/`:

| File | Purpose |
|------|---------|
| `config.ini` | Main bot configuration |
| `bot_profiles.ini` | Bot personality profiles |
| `bot_skills.ini` | Skill level definitions |
| `weapons.ini` | Weapon preferences |

### Server CFG Integration

**`cfg/rcbot.cfg`:**
```
// Bot Quota
rcbot_quota 8
rcbot_quota_interval 0.5

// Skill Levels
rcbot_skill_min 0.3
rcbot_skill_max 0.8

// Behavior
rcbot_change_classes 1
rcbot_taunt 1
rcbot_shoot_breakables 1

// TF2 Specific
rcbot_tf2_protect_cap_percent 0.5
rcbot_tf2_autoupdate_point_time 0.5
```

**`cfg/server.cfg`:**
```
exec rcbot.cfg
```

### Configuration Variables (CVars)

#### Bot Behavior

| CVar | Default | Description |
|------|---------|-------------|
| `rcbot_quota` | `0` | Number of bots to maintain |
| `rcbot_quota_interval` | `0.0` | Interval to add bots (seconds) |
| `rcbot_change_classes` | `0` | Allow bots to change classes |
| `rcbot_skill_min` | `0.0` | Minimum bot skill (0.0-1.0) |
| `rcbot_skill_max` | `1.0` | Maximum bot skill (0.0-1.0) |
| `rcbot_nonrandom_kicking` | `0` | Kick bots in order |
| `rcbot_melee_only` | `0` | Bots only use melee weapons |
| `rcbot_shoot_breakables` | `1` | Shoot breakable objects |
| `rcbot_taunt` | `1` | Allow taunting |

#### Navigation

| CVar | Default | Description |
|------|---------|-------------|
| `rcbot_autowaypoint_dist` | `200` | Auto-waypoint distance |
| `rcbot_supermode` | `0` | Super navigation mode |
| `rcbot_pathrevision` | `0` | Enable path revision |
| `rcbot_wpt_autotype` | `0` | Auto-assign waypoint types |

#### Debug

| CVar | Default | Description |
|------|---------|-------------|
| `rcbot_debug_show_route` | `0` | Show bot routes |
| `rcbot_debug_dont_shoot` | `0` | Prevent shooting |
| `rcbot_debug_notasks` | `0` | Disable all tasks |
| `rcbot_notarget` | `0` | Bots ignore threats |
| `rcbot_dont_move` | `0` | Prevent movement |

#### TF2 Specific

| CVar | Default | Description |
|------|---------|-------------|
| `rcbot_tf2_protect_cap_time` | `8.0` | Time to protect cap |
| `rcbot_tf2_protect_cap_percent` | `0.5` | Team % to protect |
| `rcbot_tf2_spy_mode` | `0` | Spy behavior mode |
| `rcbot_tf2_engineer_mode` | `0` | Engineer behavior mode |
| `rcbot_tf2_medic_mode` | `0` | Medic behavior mode |
| `rcbot_tf2_sentry_type` | `0` | Sentry type (0=normal, 1=mini) |
| `rcbot_tf2_dispenserbuild` | `1` | Allow dispenser building |
| `rcbot_tf2_teleporterbuild` | `1` | Allow teleporter building |
| `rcbot_tf2_medigun_autoheal` | `1` | Auto-heal with medigun |

#### CSS Specific

| CVar | Default | Description |
|------|---------|-------------|
| `rcbot_css_difficulty` | `1` | CSS bot difficulty |
| `rcbot_css_enable_buy` | `1` | Enable weapon buying |

#### DOD:S Specific

| CVar | Default | Description |
|------|---------|-------------|
| `rcbot_dods_flag_capture_radius` | `200` | Flag capture radius |

### Skill Configuration

**Skill values:**
- `0.0` - Very easy (poor aim, slow reactions)
- `0.3` - Easy (beginner bots)
- `0.5` - Medium (average player)
- `0.7` - Hard (skilled player)
- `0.9` - Very hard (near-perfect aim)
- `1.0` - Impossible (not recommended)

**Skill presets:**
```
// Casual gameplay
alias rcbot_skill_casual "rcbot_skill_min 0.2; rcbot_skill_max 0.6"

// Competitive
alias rcbot_skill_comp "rcbot_skill_min 0.6; rcbot_skill_max 0.9"

// Practice/Easy
alias rcbot_skill_easy "rcbot_skill_min 0.1; rcbot_skill_max 0.4"
```

### Example Configurations

**Public TF2 Server:**
```
rcbot_quota 12
rcbot_quota_interval 0.5
rcbot_skill_min 0.25
rcbot_skill_max 0.65
rcbot_change_classes 1
rcbot_taunt 1
rcbot_tf2_protect_cap_percent 0.4
rcbot_tf2_dispenserbuild 1
rcbot_tf2_teleporterbuild 1
```

**Training Server:**
```
rcbot_quota 4
rcbot_quota_interval 1.0
rcbot_skill_min 0.1
rcbot_skill_max 0.4
rcbot_debug 1
rcbot_debug_show_route 1
```

**Competitive Practice:**
```
rcbot_quota 6
rcbot_skill_min 0.65
rcbot_skill_max 0.90
rcbot_change_classes 1
rcbot_taunt 0
```

---

## SourceMod API

RCBot2 provides optional SourceMod natives for plugin integration.

### Setup

1. Build RCBot2 with SourceMod support:
   ```bash
   python configure.py -s TF2 --sm-path ~/sourcemod ...
   ```

2. Include in your plugin:
   ```sourcepawn
   #include <rcbot2>
   ```

### Bot Management Natives

```sourcepawn
// Create a bot
native int RCBot_CreateBot(const char[] name = "", const char[] class = "", int team = 0);

// Kick a bot
native bool RCBot_KickBot(int client = 0, int team = 0);

// Remove all bots
native int RCBot_RemoveAll();

// Check if client is RCBot
native bool RCBot_IsBot(int client);

// Get bot count
native int RCBot_GetBotCount();
```

### Bot Properties Natives

```sourcepawn
// Get/set bot skill
native float RCBot_GetBotSkill(int client);
native bool RCBot_SetBotSkill(int client, float skill);

// Get/set bot quota
native int RCBot_GetBotQuota();
native bool RCBot_SetBotQuota(int quota);
```

### Waypoint Natives

```sourcepawn
// Get nearest waypoint
native int RCBot_GetNearestWaypoint(const float pos[3]);

// Get waypoint count
native int RCBot_GetWaypointCount();
```

### Forwards

```sourcepawn
// Called when bot is created
forward void RCBot_OnBotCreated(int client, const char[] name, int team);

// Called when bot is kicked
forward void RCBot_OnBotKicked(int client);

// Called during bot think
forward Action RCBot_OnBotThink(int client);
```

### Example Plugin

```sourcepawn
#include <sourcemod>
#include <rcbot2>

public void OnPluginStart() {
    RegAdminCmd("sm_botinfo", CMD_BotInfo, ADMFLAG_ROOT);
}

public Action CMD_BotInfo(int client, int args) {
    int botCount = RCBot_GetBotCount();
    int wptCount = RCBot_GetWaypointCount();

    ReplyToCommand(client, "RCBots: %d, Waypoints: %d", botCount, wptCount);

    for (int i = 1; i <= MaxClients; i++) {
        if (IsClientInGame(i) && RCBot_IsBot(i)) {
            float skill = RCBot_GetBotSkill(i);
            ReplyToCommand(client, "  Bot %d: skill=%.2f", i, skill);
        }
    }

    return Plugin_Handled;
}

public void RCBot_OnBotCreated(int client, const char[] name, int team) {
    PrintToServer("RCBot %s joined team %d", name, team);
}
```

---

## Troubleshooting

### Installation Issues

#### RCBot2 not loading

**Symptoms:** No RCBot messages in console, `rcbotd` not found

**Solutions:**

1. Verify MetaMod:Source is loaded:
   ```
   meta list
   ```

2. Check plugin file location:
   ```
   {game}/addons/rcbot2meta/bin/rcbot.2.{game}.so
   ```

3. Verify VDF file exists:
   ```
   {game}/addons/rcbot2meta.vdf
   ```

4. Check file permissions (Linux):
   ```bash
   chmod +x addons/rcbot2meta/bin/*.so
   ```

#### Bots can't join (server full)

**Solution:**
```
maxplayers 24        // Command line
mp_maxplayers 24     // In server.cfg
rcbot_quota 8        // Less than maxplayers
```

### Bot Behavior Issues

#### Bots not moving

**Solutions:**

1. Check for waypoints:
   ```
   rcbot wpt load
   rcbot wpt info
   ```

2. Disable debug flags:
   ```
   rcbot_dont_move 0
   rcbot_debug_notasks 0
   ```

3. Enable route debug:
   ```
   rcbot_debug_show_route 1
   ```

#### Bots getting stuck

- Run nav-test to identify issues:
  ```
  rcbot navtest start
  rcbot navtest report
  ```
- Add waypoints in problem areas
- Add jump/crouch waypoints if needed

#### Bots not attacking

**Solutions:**
```
rcbot_debug_dont_shoot 0
rcbot_notarget 0
```

#### Bots too easy/hard

**Solutions:**
```
// Too easy (bots dominating)
rcbot_skill_min 0.2
rcbot_skill_max 0.5

// Too hard (bots useless)
rcbot_skill_min 0.5
rcbot_skill_max 0.8
```

#### Bots ignoring objectives

- Ensure dense waypoint coverage around objectives
- Check game mode detection in console
- Verify objective waypoints exist

### Waypoint Issues

#### Waypoints not visible

```
sv_cheats 1
rcbot wpt on
```

#### Waypoints won't save

- Check directory permissions
- Ensure directory exists:
  ```bash
  mkdir -p rcbot2/waypoints/tf2/
  ```

#### Waypoints won't load

- Verify file exists: `rcbot2/waypoints/{game}/{mapname}.rcw`
- Check filename matches map (case-sensitive on Linux)

### Performance Issues

#### Server lag with bots

**Solutions:**
```
rcbot_quota 6                    // Fewer bots
rcbot_tf2_autoupdate_point_time 1.0
rcbot_debug 0
rcbot_debug_show_route 0
```

### Game-Specific Issues

#### TF2: Engineers not building

```
rcbot_tf2_dispenserbuild 1
rcbot_tf2_teleporterbuild 1
```
- Ensure sentry waypoints exist

#### TF2: Medics not ubering

- Check skill level (low skill = poor uber timing)
- Verify medigun settings

### Error Messages

#### "Failed to load plugin"

- Check dependencies (Linux):
  ```bash
  ldd addons/rcbot2meta/bin/rcbot.2.tf2.so
  sudo apt-get install lib32stdc++6
  ```

#### "Access denied"

- Use server console (dedicated server)
- Grant access: `rcbot users add <name> 4`

#### "Waypoints not found"

- Download waypoints from repository
- Create waypoints for the map
- Check filename matches map exactly

### Diagnostic Commands

```
rcbotd                   // Display bot info
meta list                // Verify MM:S loaded
status                   // Show players/bots
find rcbot               // List all RCBot CVars
rcbot debug 2            // Enable debug
rcbot_debug_show_route 1 // Show navigation
```

---

## Getting Help

### Before Asking

1. Check this troubleshooting guide
2. Search existing GitHub issues
3. Check console for errors
4. Test with default settings
5. Test on different map

### Information to Provide

When reporting issues:
- RCBot2 version
- Game and version
- Server OS (Linux/Windows)
- Map name
- Console output
- Steps to reproduce

### Resources

- **GitHub Issues**: [Report bugs](https://github.com/ethanbissbort/rcbot2/issues)
- **Discord**: [Bots United](https://discord.gg/5v5YvKG4Hr)
- **Forums**: [Official Forums](http://rcbot.bots-united.com/forums/)
- **Website**: http://rcbot.bots-united.com/
- **Waypoints**: http://rcbot.bots-united.com/waypoints.php

---

## License

RCBot2 is released under the **GNU Affero General Public License v3.0 (AGPL-3.0)**.

Modifications must have sources available to players on your server.

---

**See Also**:
- [Waypoint Guide](waypoints.md) - Complete waypoint documentation
- [Building Guide](BUILDING.md) - Compile from source
- [ML Documentation](ML.md) - Machine learning features

---

**Last Updated**: 2025-12-27
**Version**: 1.7+
