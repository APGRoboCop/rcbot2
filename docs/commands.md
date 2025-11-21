# RCBot2 Command Reference

Complete reference for all RCBot2 console commands and configuration variables.

## Command Categories

- [General Commands](#general-commands)
- [Bot Management](#bot-management)
- [Waypoint Commands](#waypoint-commands)
- [Debug Commands](#debug-commands)
- [Configuration Variables (CVars)](#configuration-variables)
- [Game-Specific Commands](#game-specific-commands)

---

## General Commands

### `rcbotd`

Display RCBot2 information and available commands.

**Usage**: `rcbotd`

**Example**:
```
rcbotd
```

**Output**: Lists all available bot commands and their descriptions.

---

## Bot Management

### `rcbot addbot` / `rcbot_create`

Create a new bot.

**Usage**: `rcbot addbot [name] [class] [team]`

**Parameters**:
- `name` (optional): Bot name
- `class` (optional): Class name (TF2: scout, soldier, pyro, demoman, heavy, engineer, medic, sniper, spy)
- `team` (optional): Team number (typically 2 or 3)

**Examples**:
```
rcbot addbot                    # Create bot with random name/class
rcbot addbot BotName            # Create bot with specific name
rcbot addbot BotName soldier    # Create soldier bot
rcbot addbot BotName heavy 2    # Create heavy on team 2
```

**Aliases**: `rcbot_create`, `rcbot_create_class`

---

### `rcbot kickbot`

Remove a bot from the server.

**Usage**: `rcbot kickbot [team]`

**Parameters**:
- `team` (optional): Team number to kick bot from

**Examples**:
```
rcbot kickbot       # Kick random bot
rcbot kickbot 2     # Kick random bot from team 2
```

**Note**: Use `rcbot_nonrandom_kicking 1` to kick bots in order instead of randomly.

---

### `rcbot removeall` / `rcbot_removeall`

Remove all bots from the server.

**Usage**: `rcbot_removeall`

**Example**:
```
rcbot_removeall
```

---

### `rcbot control`

Take control of a bot.

**Usage**: `rcbot control <bot_name>`

**Parameters**:
- `bot_name`: Name of the bot to control

**Example**:
```
rcbot control BotName
```

---

## Waypoint Commands

### `rcbot wpt`

Main waypoint command. Lists all waypoint subcommands.

**Usage**: `rcbot wpt`

**Subcommands**:
- `add` - Add waypoint at current location
- `delete` - Delete nearest waypoint
- `save` - Save waypoints to file
- `load` - Load waypoints from file
- `on` - Enable waypoint display
- `off` - Disable waypoint display
- `info` - Show info about nearest waypoint
- `drawtype <type>` - Set waypoint display type
- `givetype <type>` - Give type to nearest waypoint
- `clear` - Clear all waypoints

---

### `rcbot wpt add`

Add a waypoint at your current location.

**Usage**: `rcbot wpt add [flags]`

**Common Flags**:
- `jump` - Jump waypoint
- `crouch` - Crouch waypoint
- `ladder` - Ladder waypoint
- `wait` - Wait waypoint
- `sniper` - Sniper spot
- `sentry` - Sentry gun spot (TF2 Engineer)
- `teleport` - Teleporter spot
- `health` - Health pack location
- `ammo` - Ammo pack location
- `resupply` - Resupply locker

**Examples**:
```
rcbot wpt add              # Add normal waypoint
rcbot wpt add jump         # Add jump waypoint
rcbot wpt add sniper       # Add sniper waypoint
rcbot wpt add sentry       # Add sentry spot
```

---

### `rcbot wpt delete`

Delete the waypoint nearest to your current position.

**Usage**: `rcbot wpt delete`

**Example**:
```
rcbot wpt delete
```

---

### `rcbot wpt save`

Save current waypoints to file.

**Usage**: `rcbot wpt save`

**Example**:
```
rcbot wpt save
```

**Note**: Waypoints are saved to `rcbot2/waypoints/{game}/{mapname}.rcw`

---

### `rcbot wpt load`

Load waypoints from file for current map.

**Usage**: `rcbot wpt load`

**Example**:
```
rcbot wpt load
```

---

### `rcbot wpt on` / `rcbot wpt off`

Enable or disable waypoint visualization.

**Usage**: `rcbot wpt on` or `rcbot wpt off`

**Examples**:
```
rcbot wpt on     # Show waypoints
rcbot wpt off    # Hide waypoints
```

**Note**: Waypoints only visible when `sv_cheats 1` is enabled.

---

### `rcbot wpt info`

Display information about the nearest waypoint.

**Usage**: `rcbot wpt info`

**Example**:
```
rcbot wpt info
```

**Output**: Shows waypoint index, type, flags, and connections.

---

### `rcbot wpt clear`

Delete all waypoints on the current map.

**Usage**: `rcbot wpt clear`

**Example**:
```
rcbot wpt clear
```

**Warning**: This cannot be undone! Save your waypoints first.

---

### `rcbot autowaypoint`

Enable automatic waypoint generation while playing.

**Usage**: `rcbot autowaypoint <on|off>`

**Examples**:
```
rcbot autowaypoint on      # Enable auto-waypointing
rcbot autowaypoint off     # Disable auto-waypointing
```

**Note**: Generates waypoints as you move around. Requires manual cleanup and optimization.

---

## Debug Commands

### `rcbot debug`

Set debug level and display debug information.

**Usage**: `rcbot debug [level] [bot_name]`

**Parameters**:
- `level`: Debug level (0-3)
  - `0` - No debug output
  - `1` - Basic debug info
  - `2` - Detailed debug info
  - `3` - Very detailed debug info
- `bot_name` (optional): Debug specific bot

**Examples**:
```
rcbot debug 0              # Disable debug
rcbot debug 1              # Basic debug for all bots
rcbot debug 2 BotName      # Detailed debug for specific bot
```

---

### `rcbot util`

Utility commands for testing and debugging.

**Usage**: `rcbot util <subcommand>`

**Subcommands**:
- `teleport` - Teleport to waypoint
- `printent` - Print entity information
- `getprop` - Get entity property value

---

## Configuration Variables (CVars)

### Bot Behavior

| CVar | Default | Description |
|------|---------|-------------|
| `rcbot_quota` | `0` | Number of bots to maintain |
| `rcbot_quota_interval` | `0.0` | Interval to add bots (seconds) |
| `rcbot_change_classes` | `0` | Allow bots to change classes |
| `rcbot_skill_min` | `0.0` | Minimum bot skill (0.0-1.0) |
| `rcbot_skill_max` | `1.0` | Maximum bot skill (0.0-1.0) |
| `rcbot_nonrandom_kicking` | `0` | Kick bots in order instead of randomly |
| `rcbot_melee_only` | `0` | Bots only use melee weapons |
| `rcbot_shoot_breakables` | `1` | Allow bots to shoot breakable objects |
| `rcbot_taunt` | `1` | Allow bots to taunt |

### Navigation

| CVar | Default | Description |
|------|---------|-------------|
| `rcbot_autowaypoint_dist` | `200` | Auto-waypoint generation distance |
| `rcbot_supermode` | `0` | Super navigation mode (testing) |
| `rcbot_pathrevision` | `0` | Enable path revision |
| `rcbot_wpt_autotype` | `0` | Automatically assign waypoint types |

### Team Fortress 2 Specific

| CVar | Default | Description |
|------|---------|-------------|
| `rcbot_tf2_protect_cap_time` | `8.0` | Time to protect capture point after capping |
| `rcbot_tf2_protect_cap_percent` | `0.5` | Percentage of team to protect cap point |
| `rcbot_tf2_spy_mode` | `0` | Spy behavior mode |
| `rcbot_tf2_engineer_mode` | `0` | Engineer behavior mode |
| `rcbot_tf2_medic_mode` | `0` | Medic behavior mode |
| `rcbot_tf2_autoupdate_point_time` | `0.5` | Control point update interval |
| `rcbot_tf2_payload_dist_retreat` | `800` | Distance to retreat from payload |
| `rcbot_tf2_sentry_type` | `0` | Preferred sentry type (0=normal, 1=mini) |

### Counter-Strike: Source Specific

| CVar | Default | Description |
|------|---------|-------------|
| `rcbot_css_difficulty` | `1` | CSS bot difficulty |
| `rcbot_css_enable_buy` | `1` | Enable bot weapon buying |

### Debug and Display

| CVar | Default | Description |
|------|---------|-------------|
| `rcbot_debug_iglev` | `0` | Debug ignore level |
| `rcbot_debug_notasks` | `0` | Disable all bot tasks |
| `rcbot_debug_dont_shoot` | `0` | Prevent bots from shooting |
| `rcbot_debug_show_route` | `0` | Show bot routes |
| `rcbot_debug_hear` | `0` | Debug sound hearing |
| `rcbot_debug_see` | `0` | Debug vision |

### Advanced

| CVar | Default | Description |
|------|---------|-------------|
| `rcbot_runplayer_cmd_hook` | `1` | Use player command hooks |
| `rcbot_enemydrops` | `1` | Track enemy dropped weapons |
| `rcbot_notarget` | `0` | Bots ignore all threats (testing) |
| `rcbot_dont_move` | `0` | Prevent bot movement (testing) |
| `rcbot_jump_obst_dist` | `80` | Distance to obstacle before jumping |
| `rcbot_move_dist` | `8` | Movement distance threshold |

---

## Game-Specific Commands

### Team Fortress 2

```
rcbot_tf2_auto_engi_debug <0|1>       # Debug engineer AI
rcbot_tf2_debug_spies <0|1>           # Debug spy detection
rcbot_tf2_medigun_autoheal <0|1>      # Auto-heal with medigun
rcbot_tf2_dispenserbuild <0|1>        # Allow dispenser building
rcbot_tf2_teleporterbuild <0|1>       # Allow teleporter building
```

### Day of Defeat: Source

```
rcbot_dods_flag_capture_radius <value>  # Flag capture radius
```

### Counter-Strike: Source

```
rcbot_css_buy_item <item>             # Force bots to buy specific item
rcbot_css_force_weapon <weapon>       # Force specific weapon usage
```

---

## Usage Examples

### Server Startup Config

Add to `cfg/autoexec.cfg`:

```
// Load RCBot2
rcbot_quota 8                    // Maintain 8 bots
rcbot_quota_interval 0.5         // Add bot every 0.5 seconds
rcbot_skill_min 0.3              // Minimum skill 30%
rcbot_skill_max 0.8              // Maximum skill 80%
rcbot_change_classes 1           // Allow class changes (TF2)
```

### Waypoint Creation Session

```
sv_cheats 1                      // Enable cheats for waypoint display
rcbot wpt on                     // Show waypoints
rcbot autowaypoint on            // Enable auto-waypointing

// Play the map and move around...
// Add special waypoints manually:

rcbot wpt add sniper             // Mark sniper spot
rcbot wpt add sentry             // Mark sentry spot
rcbot wpt add jump               // Mark jump spot

rcbot wpt save                   // Save waypoints
rcbot wpt off                    // Hide waypoints
```

### Debug Bot Behavior

```
rcbot debug 2 BotName            // Enable debug for specific bot
rcbot_debug_show_route 1         // Show bot navigation path
rcbot_debug_dont_shoot 1         // Prevent shooting (observe only)
```

---

## Access Levels

Commands require different access levels:

- **All Users**: Basic informational commands
- **Bot Access**: Commands that modify bot behavior
- **Admin Access**: Configuration and advanced commands
- **Dedicated Server**: Some commands only work on listen servers

Access is controlled via the `rcbot users` system (see User Management section).

---

## Notes

- Most commands can be abbreviated (e.g., `rcbot` → `rcb`)
- Use `sv_cheats 1` for waypoint visualization
- Waypoints are automatically loaded when maps load
- Some CVars require map restart to take effect
- Use `find rcbot` to list all RCBot CVars

---

**See Also**:
- [Configuration Guide](configuration.md) - Detailed CVar explanations
- [Waypoint Guide](waypoints.md) - Complete waypoint creation guide
- [Troubleshooting](troubleshooting.md) - Command-related issues

---

**Last Updated**: 2025-11-21
