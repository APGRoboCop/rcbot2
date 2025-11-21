# RCBot2 Waypoint Guide

Complete guide to creating, editing, and managing waypoints for RCBot2.

## Table of Contents

- [What are Waypoints?](#what-are-waypoints)
- [Getting Waypoints](#getting-waypoints)
- [Waypoint Basics](#waypoint-basics)
- [Creating Waypoints](#creating-waypoints)
- [Waypoint Types and Flags](#waypoint-types-and-flags)
- [Waypoint Editing](#waypoint-editing)
- [Best Practices](#best-practices)
- [Troubleshooting](#troubleshooting)

---

## What are Waypoints?

Waypoints are navigation nodes that guide bots through maps. They form a network of interconnected points that bots use to:

- Navigate around the map
- Find objectives
- Locate resources (health, ammo)
- Position strategically (sniper spots, sentry locations)
- Understand map-specific mechanics

**Without waypoints**, bots cannot navigate properly and will wander aimlessly.

---

## Getting Waypoints

### Download Existing Waypoints

1. Visit the [official waypoint repository](http://rcbot.bots-united.com/waypoints.php)
2. Download waypoints for your game and maps
3. Extract to `{game}/rcbot2/waypoints/{game}/`

**Example structure:**
```
tf/rcbot2/waypoints/tf2/
├── pl_badwater.rcw
├── cp_dustbowl.rcw
├── ctf_2fort.rcw
└── ...
```

### Verify Waypoints

Start your server and load a map:

```
map pl_badwater
rcbot wpt load
rcbot wpt info
```

If waypoints loaded successfully, you'll see waypoint count in console.

---

## Waypoint Basics

### Viewing Waypoints

Enable waypoint visualization:

```
sv_cheats 1          // Required for waypoint display
rcbot wpt on         // Show waypoints
```

**Waypoint colors:**
- **Blue** - Normal waypoint
- **Red** - Important waypoint (objectives, etc.)
- **Green** - Wait/Camp waypoint
- **Yellow** - Jump waypoint
- **Purple** - Special waypoint (class-specific)

### Waypoint Connections

Waypoints are connected to form a navigation network. Connections are:
- **Automatic** - Created based on line-of-sight and distance
- **Manual** - Created with pathwaypoint commands
- **Bidirectional** or **one-way**

---

## Creating Waypoints

### Prerequisites

```
sv_cheats 1              // Enable cheats
rcbot wpt on             // Show waypoints
noclip                   // Recommended for easier movement
```

### Method 1: Automatic Waypointing

Easiest method for beginners:

```
rcbot autowaypoint on    // Enable auto-waypointing
```

**Steps:**
1. Play through the map normally
2. Visit all areas, including alternate routes
3. Waypoints are created automatically as you move
4. Save when finished: `rcbot wpt save`

**Pros:**
- Fast and easy
- Good coverage

**Cons:**
- May create too many waypoints
- Requires manual cleanup
- Missing special waypoint types

### Method 2: Manual Waypointing

Recommended for quality waypoints:

```
rcbot autowaypoint off   // Disable auto-waypointing
```

**Steps:**
1. Position yourself where you want a waypoint
2. Add waypoint: `rcbot wpt add`
3. Move to next location
4. Repeat until map is covered
5. Add special waypoints (see [Waypoint Types](#waypoint-types-and-flags))
6. Save: `rcbot wpt save`

**Pros:**
- Full control over placement
- Can add special types immediately
- Cleaner waypoint network

**Cons:**
- Time-consuming
- Requires map knowledge

### Method 3: Hybrid Approach

Best of both worlds:

1. Use auto-waypointing for initial coverage
2. Delete unnecessary waypoints
3. Manually add special waypoints (sniper spots, sentry positions, etc.)
4. Fine-tune connections

---

## Waypoint Types and Flags

### General Waypoint Types

#### Normal Waypoint
```
rcbot wpt add
```
Basic navigation waypoint. No special properties.

#### Jump Waypoint
```
rcbot wpt add jump
```
Bot will jump when reaching this waypoint. Use for:
- Gaps
- Small obstacles
- Crouch-jump locations

#### Crouch Waypoint
```
rcbot wpt add crouch
```
Bot will crouch when moving to this waypoint. Use for:
- Low passages
- Crawl spaces
- Crouch-only areas

#### Ladder Waypoint
```
rcbot wpt add ladder
```
Indicates ladder navigation. Place at:
- Bottom of ladder
- Top of ladder
- Mid-ladder (for long ladders)

#### Wait Waypoint
```
rcbot wpt add wait
```
Bot will wait/camp at this location. Use for:
- Ambush positions
- Defensive spots
- Choke points

### Resource Waypoints

#### Health Pack
```
rcbot wpt add health
```
Marks health pack location. Bots will seek when injured.

#### Ammo Pack
```
rcbot wpt add ammo
```
Marks ammo pack location. Bots will seek when low on ammo.

#### Resupply Locker
```
rcbot wpt add resupply
```
Marks resupply cabinet (TF2). Bots will use to heal/restock.

### Team Fortress 2 Specific

#### Sentry Position
```
rcbot wpt add sentry
```
Engineer bot sentry gun placement. Place at:
- Defensive positions
- Covering objectives
- Covering chokepoints

**Tips:**
- Face the direction the sentry should aim
- Ensure good field of view
- Near dispenser metal source

#### Dispenser Position
```
rcbot wpt add dispenser
```
Engineer bot dispenser placement.

#### Teleporter Position
```
rcbot wpt add teleporter_exit
rcbot wpt add teleporter_entrance
```
Teleporter placement positions.

#### Sniper Spot
```
rcbot wpt add sniper
```
Sniper vantage point. Place at:
- Long sightlines
- Elevated positions
- Covering key areas

#### Defend Area
```
rcbot wpt add defend
```
General defensive position.

### Objective Waypoints

These are often auto-detected but can be manually added:

```
rcbot wpt add capture_point    // Capture point
rcbot wpt add payload           // Payload cart
rcbot wpt add flag              // CTF flag
rcbot wpt add objective         // Generic objective
```

---

## Waypoint Editing

### Viewing Waypoint Info

```
rcbot wpt info
```

Shows information about the nearest waypoint:
- Waypoint index
- Type and flags
- Connections
- Distance from player

### Deleting Waypoints

**Delete nearest waypoint:**
```
rcbot wpt delete
```

**Delete all waypoints (WARNING!):**
```
rcbot wpt clear
```

**Delete specific waypoint:**
Position yourself near it and use `rcbot wpt delete`.

### Modifying Waypoint Types

**Give type to nearest waypoint:**
```
rcbot wpt givetype sniper       // Make it a sniper waypoint
rcbot wpt givetype jump          // Make it a jump waypoint
```

**Remove type:**
```
rcbot wpt givetype none          // Remove special type
```

### Path Connections

Manual path creation:

```
rcbot pathwaypoint create        // Create path to another waypoint
rcbot pathwaypoint remove        // Remove path
```

**Creating one-way paths:**
Used for:
- Drop-downs (can't climb back up)
- One-way routes
- Jump-down locations

---

## Best Practices

### Waypoint Density

**General guidelines:**
- **Open areas**: Waypoints every 200-300 units
- **Corridors**: Waypoints every 150-200 units
- **Tight spaces**: Waypoints every 100 units
- **Objective areas**: Dense coverage

**Check density:**
```
rcbot autowaypoint_dist 200      // Adjust auto-waypoint distance
```

### Coverage Checklist

Ensure waypoints cover:

- [ ] **All routes** to objectives
- [ ] **Alternate paths** and flanks
- [ ] **High ground** and elevated areas
- [ ] **Health pack** locations
- [ ] **Ammo pack** locations
- [ ] **Resupply areas**
- [ ] **Sniper positions**
- [ ] **Sentry positions** (TF2)
- [ ] **Objective areas** (capture points, cart, etc.)
- [ ] **Spawn exits**
- [ ] **Teleporter positions** (TF2)

### Quality Tips

1. **Test with bots**: Watch bots navigate and fix problem areas
2. **Check connections**: Ensure waypoints connect properly
3. **Face important directions**: Waypoint orientation matters for some types
4. **Avoid over-waypointing**: Too many waypoints = slower navigation
5. **Mark special areas**: Use appropriate types (sniper, sentry, etc.)

### Game-Specific Tips

**Team Fortress 2:**
- Mark all sentry spots for Engineer bots
- Mark sniper sightlines
- Dense waypoints around objectives
- Mark flank routes clearly
- Add jump/crouch waypoints for mobility

**Counter-Strike: Source:**
- Mark bombsites densely
- Mark common camping spots
- Add cover positions
- Mark buy zones

**Day of Defeat: Source:**
- Mark all capture points
- Mark MG positions
- Mark sniper windows
- Dense coverage in combat zones

---

## Waypoint Workflow

### Creating Waypoints from Scratch

**1. Initial Pass - Main Routes**
```
sv_cheats 1
rcbot wpt on
rcbot autowaypoint on
```

- Walk main routes to each objective
- Cover all major pathways
- Visit spawn areas

**2. Save Baseline**
```
rcbot wpt save
```

**3. Add Special Waypoints**
```
rcbot autowaypoint off
```

- Add sniper spots: `rcbot wpt add sniper`
- Add sentry spots: `rcbot wpt add sentry`
- Add jump spots: `rcbot wpt add jump`
- Mark resources (health, ammo)

**4. Cleanup**

- Delete redundant waypoints
- Check connections
- Fix problem areas

**5. Test with Bots**
```
rcbot_create
rcbot_debug 1
rcbot_debug_show_route 1
```

- Watch bot navigation
- Note stuck areas
- Check objective understanding

**6. Refine**

- Fix problem areas identified in testing
- Add missing connections
- Adjust waypoint types

**7. Final Save**
```
rcbot wpt save
```

### Editing Existing Waypoints

```
rcbot wpt load                   // Load existing waypoints
rcbot wpt on                     // Show waypoints
```

- Identify problem areas
- Delete/add waypoints as needed
- Test changes

```
rcbot wpt save                   // Save modifications
```

---

## Waypoint Commands Reference

| Command | Description |
|---------|-------------|
| `rcbot wpt on` | Show waypoints |
| `rcbot wpt off` | Hide waypoints |
| `rcbot wpt add [type]` | Add waypoint |
| `rcbot wpt delete` | Delete nearest waypoint |
| `rcbot wpt save` | Save waypoints to file |
| `rcbot wpt load` | Load waypoints from file |
| `rcbot wpt info` | Show waypoint info |
| `rcbot wpt clear` | Delete all waypoints |
| `rcbot wpt givetype <type>` | Change waypoint type |
| `rcbot wpt drawtype <type>` | Change visualization |
| `rcbot autowaypoint <on|off>` | Toggle auto-waypointing |
| `rcbot pathwaypoint create` | Create path connection |
| `rcbot pathwaypoint remove` | Remove path connection |

---

## Troubleshooting

### Waypoints not visible

```
sv_cheats 1              // Enable cheats (required)
rcbot wpt on             // Show waypoints
```

### Waypoints won't save

- Check write permissions on `rcbot2/waypoints/{game}/` directory
- Ensure directory exists
- Check server console for errors

### Bots getting stuck

- Add more waypoints in stuck area
- Add jump/crouch waypoints if needed
- Check waypoint connections
- Use `rcbot_debug_show_route 1` to see bot pathing

### Waypoints won't load

- Verify file exists: `rcbot2/waypoints/{game}/{mapname}.rcw`
- Check file permissions
- Ensure file is not corrupted (re-download or re-create)

### Too many waypoints

```
rcbot wpt on                     // Show waypoints
```

- Delete redundant waypoints in open areas
- Keep essential navigation points
- Remove waypoints that are too close together

### Bot ignores objective

- Add objective waypoints near goal
- Ensure dense coverage around objectives
- Add defend/attack waypoints
- Check game-specific objective detection (may be automatic)

---

## Contributing Waypoints

### Waypoint Quality Standards

Before sharing waypoints:

- [ ] All major routes covered
- [ ] Objectives waypointed
- [ ] Resources marked (health, ammo)
- [ ] Class-specific spots marked (sniper, sentry, etc.)
- [ ] Tested with bots (no stuck spots)
- [ ] Optimized (no unnecessary waypoints)
- [ ] Jump/crouch waypoints where needed

### Submitting Waypoints

1. Test thoroughly on your server
2. Package waypoint file (.rcw)
3. Include map name and game
4. Submit to community waypoint repository
5. Document any special notes or requirements

---

## Advanced Topics

### Waypoint Visibility

Waypoints automatically calculate visibility to other waypoints. This is used for:
- Pathfinding optimization
- Cover detection
- Tactical positioning

### Waypoint Areas

Group waypoints into logical areas:
- Spawn areas
- Objective zones
- Flank routes
- Defensive positions

### Waypoint Radius

Each waypoint has a radius that determines when a bot has "reached" it. Default is usually sufficient.

---

**See Also**:
- [Command Reference](commands.md) - Waypoint commands
- [Troubleshooting](troubleshooting.md) - Common waypoint issues
- [Configuration Guide](configuration.md) - Waypoint-related CVars

---

**Last Updated**: 2025-11-21
**Community Waypoints**: http://rcbot.bots-united.com/waypoints.php
