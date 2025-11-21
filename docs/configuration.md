# RCBot2 Configuration Guide

Complete guide to configuring RCBot2 for optimal bot behavior.

## Table of Contents

- [Configuration Files](#configuration-files)
- [Basic Configuration](#basic-configuration)
- [Bot Behavior](#bot-behavior)
- [Skill Levels](#skill-levels)
- [Game-Specific Settings](#game-specific-settings)
- [Performance Tuning](#performance-tuning)
- [Advanced Configuration](#advanced-configuration)

---

## Configuration Files

### File Locations

RCBot2 configuration files are located in:
```
{game}/rcbot2/config/
```

**Key files:**
- `config.ini` - Main bot configuration
- `bot_profiles.ini` - Bot personality profiles
- `bot_skills.ini` - Skill level definitions
- `weapons.ini` - Weapon preferences
- `{game}_bot.ini` - Game-specific settings

### Server CFG Integration

Add RCBot2 commands to your server config files:

**`cfg/server.cfg`** - Main server config:
```
// RCBot2 Settings
exec rcbot.cfg
```

**`cfg/rcbot.cfg`** - RCBot2-specific config:
```
// Basic Bot Settings
rcbot_quota 8
rcbot_quota_interval 0.5
rcbot_skill_min 0.3
rcbot_skill_max 0.8

// Bot Behavior
rcbot_change_classes 1
rcbot_taunt 1
rcbot_shoot_breakables 1

// TF2 Specific (if running TF2)
rcbot_tf2_protect_cap_percent 0.5
rcbot_tf2_autoupdate_point_time 0.5
```

---

## Basic Configuration

### Bot Quota

Control the number of bots on your server:

```
rcbot_quota 8                    // Maintain 8 bots
rcbot_quota_interval 0.5         // Add bot every 0.5 seconds
```

**Dynamic quota** (adjusts based on human players):
```
rcbot_quota 12                   // Max total players
mp_maxplayers 16                 // Server max players

// Server will maintain 12 players total (humans + bots)
```

### Bot Names

Bots use random names from the Source Engine's bot name list.

**Custom names**: Edit `scripts/bot_names.txt` (varies by game)

### Team Assignment

```
// Auto-balance teams (game default)
mp_autoteambalance 1
mp_teams_unbalance_limit 2

// Force bots to specific team
// Use rcbot addbot <name> <class> <team>
rcbot addbot BotName soldier 2   // Add to team 2 (RED in TF2)
```

---

## Bot Behavior

### Movement and Navigation

```
rcbot_move_dist 8                // Movement distance threshold
rcbot_jump_obst_dist 80          // Distance to obstacle before jumping
rcbot_pathrevision 1             // Enable path revision for smoother movement
rcbot_supermode 0                // Disable experimental super navigation
```

### Combat Behavior

```
rcbot_shoot_breakables 1         // Allow shooting breakable objects
rcbot_enemydrops 1               // Track enemy dropped weapons
rcbot_melee_only 0               // Force melee weapons only (fun mode)
```

### Social Behavior

```
rcbot_taunt 1                    // Allow taunting after kills
rcbot_notarget 0                 // Bots ignore all threats (testing only)
```

### Class Selection

**Team Fortress 2:**
```
rcbot_change_classes 1           // Allow class changes
rcbot_tf2_engineer_mode 0        // Engineer behavior (0=normal, 1=defensive, 2=aggressive)
rcbot_tf2_medic_mode 0           // Medic behavior (0=normal, 1=pocket, 2=battle medic)
rcbot_tf2_spy_mode 0             // Spy behavior (0=normal, 1=aggressive, 2=passive)
```

---

## Skill Levels

### Skill Range

Bot skill affects aim, reaction time, and decision-making:

```
rcbot_skill_min 0.3              // Minimum skill: 30% (easier)
rcbot_skill_max 0.8              // Maximum skill: 80% (harder)
```

**Skill values:**
- `0.0` - Very easy (poor aim, slow reactions)
- `0.3` - Easy (beginner bots)
- `0.5` - Medium (average player)
- `0.7` - Hard (skilled player)
- `0.9` - Very hard (near-perfect aim)
- `1.0` - Impossible (aimbot-level, not recommended)

### Skill Profiles

Create skill presets in `rcbot.cfg`:

**Public server** (casual gameplay):
```
alias rcbot_skill_casual "rcbot_skill_min 0.2; rcbot_skill_max 0.6"
```

**Competitive server** (challenging bots):
```
alias rcbot_skill_comp "rcbot_skill_min 0.6; rcbot_skill_max 0.9"
```

**Practice server** (training):
```
alias rcbot_skill_easy "rcbot_skill_min 0.1; rcbot_skill_max 0.4"
```

Usage:
```
rcbot_skill_casual   // Execute alias
```

---

## Game-Specific Settings

### Team Fortress 2

#### Objective Settings

```
// Capture Points
rcbot_tf2_protect_cap_time 8.0           // Time to protect after capping (seconds)
rcbot_tf2_protect_cap_percent 0.5        // % of team to protect
rcbot_tf2_autoupdate_point_time 0.5      // Update interval for CP detection

// Payload
rcbot_tf2_payload_dist_retreat 800       // Distance to retreat from cart
```

#### Class-Specific Settings

```
// Engineer
rcbot_tf2_sentry_type 0                  // 0=normal, 1=mini sentry
rcbot_tf2_dispenserbuild 1               // Allow dispenser building
rcbot_tf2_teleporterbuild 1              // Allow teleporter building
rcbot_tf2_auto_engi_debug 0              // Debug engineer AI

// Medic
rcbot_tf2_medigun_autoheal 1             // Auto-heal with medigun
rcbot_tf2_medic_letgotime 2.0            // Time to hold heal before switching

// Spy
rcbot_tf2_debug_spies 0                  // Debug spy detection
```

#### MvM Settings

```
// Mann vs Machine (when implemented)
// rcbot_tf2_mvm_upgrade_enabled 1       // Enable MvM upgrade system (planned)
```

### Day of Defeat: Source

```
rcbot_dods_flag_capture_radius 200       // Flag capture radius
```

### Counter-Strike: Source

```
rcbot_css_difficulty 1                   // CSS bot difficulty (0-3)
rcbot_css_enable_buy 1                   // Enable weapon buying
// rcbot_css_buy_item "ak47"             // Force specific weapon (planned)
```

### Half-Life 2: Deathmatch

```
// No specific settings (uses default behavior)
```

---

## Performance Tuning

### Server Performance

```
// Reduce bot AI load
rcbot_quota 6                    // Fewer bots = better performance

// Increase tick interval
rcbot_tf2_autoupdate_point_time 1.0    // Update less frequently
```

### Waypoint Performance

```
// Optimize waypoint calculations
rcbot_autowaypoint_dist 250      // Larger distance = fewer waypoints
```

### Debug Overhead

```
// Disable debug output for production
rcbot_debug 0
rcbot_debug_show_route 0
rcbot_debug_dont_shoot 0
rcbot_debug_notasks 0
```

---

## Advanced Configuration

### Per-Map Configuration

Create map-specific configs in `cfg/`:

**`cfg/maps/pl_badwater.cfg`:**
```
// Badwater-specific settings
rcbot_quota 10                   // More bots on large map
rcbot_tf2_payload_dist_retreat 1000
```

Server automatically executes `maps/{mapname}.cfg` on map load.

### Access Control

Configure user access levels for bot commands:

```
// In-game
rcbot users add <player_name> <access_level>
rcbot users remove <player_name>
rcbot users list
```

**Access levels:**
- `1` - Basic access (view commands)
- `2` - Bot control (create/kick bots)
- `3` - Configuration (change CVars)
- `4` - Admin (all commands)

### Waypoint Auto-Type

```
rcbot_wpt_autotype 1             // Automatically assign waypoint types
```

**Auto-detected types:**
- Health packs
- Ammo packs
- Resupply lockers
- Control points
- Objective areas

### Testing and Debug

```
// Testing configuration
sv_cheats 1                      // Required for some features
rcbot_notarget 1                 // Bots ignore threats
rcbot_dont_move 1                // Freeze bots in place
rcbot_debug_notasks 1            // Disable all tasks
```

---

## Example Configurations

### Public TF2 Server

```
// cfg/rcbot.cfg
// Public TF2 Server Configuration

// Bot Quota
rcbot_quota 12
rcbot_quota_interval 0.5

// Skill Levels (Easy to Medium)
rcbot_skill_min 0.25
rcbot_skill_max 0.65

// Behavior
rcbot_change_classes 1
rcbot_taunt 1
rcbot_shoot_breakables 1
rcbot_melee_only 0

// TF2 Settings
rcbot_tf2_protect_cap_percent 0.4
rcbot_tf2_autoupdate_point_time 0.5
rcbot_tf2_medigun_autoheal 1
rcbot_tf2_dispenserbuild 1
rcbot_tf2_teleporterbuild 1
rcbot_tf2_sentry_type 0

// Performance
rcbot_debug 0
```

### Training Server

```
// cfg/rcbot.cfg
// Training Server Configuration

// Few easy bots
rcbot_quota 4
rcbot_quota_interval 1.0

// Low skill
rcbot_skill_min 0.1
rcbot_skill_max 0.4

// Debug enabled
rcbot_debug 1
rcbot_debug_show_route 1
sv_cheats 1
```

### Competitive Practice

```
// cfg/rcbot.cfg
// Competitive Practice Configuration

// Challenging bots
rcbot_quota 6
rcbot_quota_interval 0.3

// High skill
rcbot_skill_min 0.65
rcbot_skill_max 0.90

// Realistic behavior
rcbot_change_classes 1
rcbot_taunt 0
rcbot_shoot_breakables 1

// TF2 Competitive settings
rcbot_tf2_protect_cap_percent 0.6
rcbot_tf2_medic_mode 1           // Pocket medic
```

---

## Troubleshooting

### Bots are too easy/hard

Adjust skill levels:
```
rcbot_skill_min 0.4
rcbot_skill_max 0.7
```

### Bots not building (TF2 Engineer)

Enable building:
```
rcbot_tf2_dispenserbuild 1
rcbot_tf2_teleporterbuild 1
```

### Bots not changing classes

```
rcbot_change_classes 1
```

### Performance issues

Reduce bot count and update frequency:
```
rcbot_quota 6
rcbot_tf2_autoupdate_point_time 1.0
```

---

**See Also**:
- [Command Reference](commands.md) - Complete CVar list
- [Troubleshooting](troubleshooting.md) - Common issues
- [Performance Guide](performance.md) - Optimization tips

---

**Last Updated**: 2025-11-21
