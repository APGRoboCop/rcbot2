# RCBot2 Troubleshooting Guide

Solutions to common problems with RCBot2.

## Table of Contents

- [Installation Issues](#installation-issues)
- [Bot Behavior Issues](#bot-behavior-issues)
- [Waypoint Issues](#waypoint-issues)
- [Performance Issues](#performance-issues)
- [Game-Specific Issues](#game-specific-issues)
- [Build Issues](#build-issues)
- [Getting Help](#getting-help)

---

## Installation Issues

### RCBot2 not loading

**Symptoms**: No RCBot messages in console, `rcbotd` command not found

**Solutions**:

1. **Verify MetaMod:Source is loaded**:
   ```
   meta list
   ```
   Should show loaded plugins. If MM:S not loaded, install it first.

2. **Check plugin file location**:
   ```
   {game}/addons/rcbot2meta/bin/rcbot.2.{game}.so   (Linux)
   {game}/addons/rcbot2meta/bin/rcbot.2.{game}.dll  (Windows)
   ```

3. **Verify VDF file**:
   ```
   {game}/addons/rcbot2meta.vdf
   ```
   Should contain:
   ```
   "Plugin"
   {
       "file"    "../{game}/addons/rcbot2meta/bin/rcbot.2.{game}"
   }
   ```

4. **Check file permissions (Linux)**:
   ```bash
   chmod +x addons/rcbot2meta/bin/*.so
   ```

5. **Verify correct game build**:
   Ensure you downloaded/built RCBot2 for your specific game (TF2, DOD:S, etc.)

6. **Check server console for errors**:
   Look for "Failed to load plugin" or library errors

---

### `rcbotd` command does nothing

**Symptoms**: Command executes but shows nothing

**Solutions**:

1. **Plugin may not have loaded properly**:
   ```
   meta list
   ```
   Look for RCBot2Meta in the list

2. **Check access level**:
   Some commands require admin access. Try from server console (dedicated server).

3. **Reinstall clean**:
   - Remove `addons/rcbot2meta/`
   - Remove `rcbot2/` directory
   - Reinstall fresh

---

### Bots can't join (server full)

**Symptoms**: "couldn't create bot! (Check maxplayers)" error

**Solutions**:

1. **Increase max players**:
   ```
   maxplayers 24        // Command line
   // OR
   mp_maxplayers 24     // In server.cfg
   ```

2. **Check current player count**:
   ```
   status
   ```

3. **Adjust bot quota**:
   ```
   rcbot_quota 8        // Should be less than maxplayers
   ```

---

## Bot Behavior Issues

### Bots standing still/not moving

**Symptoms**: Bots spawn but don't move

**Solutions**:

1. **Check for waypoints**:
   ```
   rcbot wpt load
   rcbot wpt info
   ```
   If no waypoints, bots can't navigate. [Download or create waypoints](waypoints.md).

2. **Verify waypoints loaded**:
   Check console for "Loaded X waypoints" message

3. **Debug bot state**:
   ```
   rcbot debug 2
   rcbot_debug_show_route 1
   ```
   Look for navigation errors

4. **Check debug flags** (testing mode might be on):
   ```
   rcbot_dont_move 0            // Ensure bots can move
   rcbot_debug_notasks 0        // Ensure tasks are enabled
   ```

---

### Bots walking into walls/getting stuck

**Symptoms**: Bots repeatedly stuck in same spot

**Solutions**:

1. **Waypoint issue**:
   - Check waypoint quality in stuck area
   - Add jump or crouch waypoints if needed
   - Add more waypoints in problem area

2. **Visualize bot path**:
   ```
   sv_cheats 1
   rcbot_debug_show_route 1
   ```

3. **Check for obstacles**:
   - Ensure waypoints avoid obstacles
   - Add appropriate waypoint flags (jump, crouch, etc.)

4. **Report to community**:
   If persistent, waypoints may need fixing

---

### Bots not attacking/shooting

**Symptoms**: Bots see enemies but don't fire

**Solutions**:

1. **Check debug flags**:
   ```
   rcbot_debug_dont_shoot 0     // Ensure shooting is enabled
   ```

2. **Check notarget mode**:
   ```
   rcbot_notarget 0             // Bots should target enemies
   ```

3. **Verify weapon availability**:
   Bots may not have weapons (rare, usually on spawn)

4. **Check bot skill**:
   ```
   rcbot_skill_min 0.3          // Too low = poor aim
   rcbot_skill_max 0.7
   ```

---

### Bots too easy/too hard

**Symptoms**: Bots are dominating or useless

**Solutions**:

1. **Adjust skill levels**:

   **Too easy** (bots dominating):
   ```
   rcbot_skill_min 0.2
   rcbot_skill_max 0.5
   ```

   **Too hard** (bots useless):
   ```
   rcbot_skill_min 0.5
   rcbot_skill_max 0.8
   ```

2. **Verify skill range is reasonable**:
   - Min should be less than max
   - Avoid 0.0 (completely broken)
   - Avoid 1.0 (aimbot-level, unfun)

---

### Bots not playing objective

**Symptoms**: Bots ignore capture points, payload, etc.

**Solutions**:

1. **Verify game mode detection**:
   Check console for game mode detection messages

2. **Check waypoint coverage**:
   Ensure objective areas are densely waypointed

3. **Game-specific settings**:

   **TF2**:
   ```
   rcbot_tf2_autoupdate_point_time 0.5
   rcbot_tf2_protect_cap_percent 0.5
   ```

4. **Debug objective detection**:
   ```
   rcbot debug 2
   ```
   Look for objective-related messages

---

## Waypoint Issues

### Waypoints not visible

**Symptoms**: `rcbot wpt on` shows nothing

**Solutions**:

1. **Enable sv_cheats**:
   ```
   sv_cheats 1              // Required for waypoint display
   rcbot wpt on
   ```

2. **Check waypoints exist**:
   ```
   rcbot wpt info
   ```
   Should show waypoint count

3. **Reload waypoints**:
   ```
   rcbot wpt load
   ```

---

### Waypoints won't save

**Symptoms**: `rcbot wpt save` fails or doesn't persist

**Solutions**:

1. **Check directory permissions (Linux)**:
   ```bash
   chmod -R 755 rcbot2/waypoints/
   chown -R <server_user> rcbot2/waypoints/
   ```

2. **Verify directory exists**:
   ```bash
   mkdir -p rcbot2/waypoints/tf2/     # For TF2
   ```

3. **Check disk space**:
   ```bash
   df -h
   ```

4. **Console errors**:
   Check server console for write errors

---

### Waypoints won't load

**Symptoms**: Waypoints don't load on map start

**Solutions**:

1. **Verify file exists**:
   ```bash
   ls rcbot2/waypoints/tf2/         # Check for {mapname}.rcw
   ```

2. **Check filename matches map**:
   - Map: `pl_badwater`
   - File: `pl_badwater.rcw`
   - Case-sensitive on Linux!

3. **Manual load**:
   ```
   rcbot wpt load
   ```

4. **Check file permissions**:
   ```bash
   chmod 644 rcbot2/waypoints/tf2/*.rcw
   ```

---

## Performance Issues

### Server lag with bots

**Symptoms**: Low tick rate, high CPU usage

**Solutions**:

1. **Reduce bot count**:
   ```
   rcbot_quota 6            // Fewer bots
   ```

2. **Reduce update frequency**:
   ```
   rcbot_tf2_autoupdate_point_time 1.0
   ```

3. **Disable debug output**:
   ```
   rcbot_debug 0
   rcbot_debug_show_route 0
   ```

4. **Optimize waypoints**:
   - Remove redundant waypoints
   - Simplify waypoint network

5. **Server specs**:
   Ensure server has adequate CPU/RAM for bot count

---

### Slow bot spawning

**Symptoms**: Bots take forever to join

**Solutions**:

1. **Increase spawn interval**:
   ```
   rcbot_quota_interval 0.2     // Faster spawning (default 0.5)
   ```

2. **Check server tick rate**:
   ```
   sv_maxrate 60000
   sv_minrate 20000
   ```

---

## Game-Specific Issues

### Team Fortress 2

#### Engineer bots not building sentries

**Symptoms**: Engineers don't place buildings

**Solutions**:

1. **Enable building**:
   ```
   rcbot_tf2_dispenserbuild 1
   rcbot_tf2_teleporterbuild 1
   ```

2. **Check for sentry waypoints**:
   Map needs sentry waypoints. Add them:
   ```
   rcbot wpt add sentry
   ```

3. **Known issue**: Sentry orientation bug (see [Roadmap](../roadmap.md))

#### Medics not ubering

**Symptoms**: Medics build uber but don't use it

**Solutions**:

1. **Check medigun settings**:
   ```
   rcbot_tf2_medigun_autoheal 1
   ```

2. **Skill level issue**:
   Low skill bots may not uber effectively:
   ```
   rcbot_skill_min 0.5
   ```

#### Demoman can't sticky jump

**Known issue**: Regression from v1.7-beta. See [Roadmap](../roadmap.md) for fix status.

#### Bots fail on Halloween maps

**Known issue**: Some Halloween events not fully supported (see [Roadmap](../roadmap.md)).

### Counter-Strike: Source

#### Bots not buying weapons

**Known issue**: Buy menu not implemented yet (see [Roadmap](../roadmap.md)).

Temporary workaround: Bots will pick up dropped weapons.

#### Bots not planting/defusing bomb

**Partial support**: Basic bomb handling implemented, needs improvement.

### Day of Defeat: Source

#### Bots not capturing flags

**Solutions**:

1. **Check waypoint coverage** around flags

2. **Verify flag radius**:
   ```
   rcbot_dods_flag_capture_radius 200
   ```

---

## Build Issues

### Configure fails

**Symptoms**: `configure.py` errors

**Solutions**:

1. **Python version**:
   ```bash
   python3 --version     // Should be 3.6+
   ```

2. **Missing dependencies**:
   ```bash
   # Install AMBuild
   pip3 install ambuild
   ```

3. **SDK paths**:
   Verify paths are correct:
   ```bash
   python configure.py \
     -s TF2 \
     --mms-path ~/metamod-source \
     --hl2sdk-root ~/hl2sdk-root
   ```

4. **Check SDK directory structure**:
   ```bash
   ls ~/hl2sdk-root/
   # Should show: hl2sdk-tf2, hl2sdk-dods, etc.
   ```

See [Building Guide](building.md) for detailed build troubleshooting.

---

## Error Messages

### "Failed to load plugin"

**Cause**: Missing dependencies or wrong architecture

**Solutions**:

1. **Check dependencies (Linux)**:
   ```bash
   ldd addons/rcbot2meta/bin/rcbot.2.tf2.so
   ```
   All libraries should be found

2. **Install 32-bit libraries** (Linux):
   ```bash
   sudo apt-get install lib32stdc++6
   ```

3. **Verify architecture**:
   ```bash
   file addons/rcbot2meta/bin/rcbot.2.tf2.so
   # Should show: ELF 32-bit
   ```

### "Access denied" for commands

**Cause**: Insufficient user access level

**Solutions**:

1. **Use server console** (dedicated server)

2. **Grant access**:
   ```
   rcbot users add <your_name> 4
   ```

3. **Use RCON** with admin privileges

### "Waypoints not found"

**Cause**: Waypoint file missing for current map

**Solutions**:

1. **Download waypoints** from repository

2. **Create waypoints** for the map (see [Waypoint Guide](waypoints.md))

3. **Check filename**:
   Must match map name exactly (case-sensitive on Linux)

---

## Diagnostic Commands

### Check RCBot Status

```
rcbotd                   // Display bot info
meta list                // Verify MM:S loaded
meta plugins             // List all plugins
status                   // Show players and bots
```

### Debug Bot Behavior

```
rcbot debug 2            // Enable detailed debug
rcbot_debug_show_route 1 // Show bot navigation
rcbot_debug_hear 1       // Debug sound detection
rcbot_debug_see 1        // Debug vision
```

### Check Configuration

```
find rcbot               // List all RCBot CVars
rcbot_quota              // Check bot quota
rcbot_skill_min          // Check skill range
rcbot_skill_max
```

---

## Getting Help

### Before Asking for Help

1. **Check this troubleshooting guide**
2. **Search existing issues** on GitHub
3. **Check console for errors**
4. **Try with default settings** (rule out configuration issues)
5. **Test on different map** (rule out map-specific issues)

### Gathering Information

When reporting issues, include:

- **RCBot2 version**: Check build date/version
- **Game and version**: TF2, DOD:S, etc.
- **Server OS**: Linux/Windows
- **Server type**: Listen/Dedicated
- **Map name**: Which map has the issue
- **Console output**: Errors and warnings
- **Steps to reproduce**: How to trigger the issue
- **Screenshots/video**: If relevant

### Where to Get Help

1. **GitHub Issues**: [Report bugs](https://github.com/ethanbissbort/rcbot2/issues)
2. **Discord**: [Bots United Discord](https://discord.gg/5v5YvKG4Hr)
3. **Forums**: [Official Forums](http://rcbot.bots-united.com/forums/)
4. **Documentation**: Check [docs directory](README.md)

---

## Common Misconceptions

### "Bots need sv_cheats 1"

**False**: Bots work with `sv_cheats 0`. Only waypoint visualization needs sv_cheats.

### "More waypoints = better navigation"

**False**: Too many waypoints can slow navigation. Quality > quantity.

### "Bots cheat/have aimbot"

**Partly false**: Bots use simulated vision and skill-based aim. High skill bots are accurate but not perfect.

### "Bots require SourceMod"

**False**: SourceMod is optional for extended functionality.

---

**See Also**:
- [Configuration Guide](configuration.md) - Settings and CVars
- [Command Reference](commands.md) - All commands
- [Waypoint Guide](waypoints.md) - Waypoint help
- [Building Guide](building.md) - Build troubleshooting

---

**Last Updated**: 2025-11-21
