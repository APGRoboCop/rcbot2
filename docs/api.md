# RCBot2 API Reference

SourceMod natives and API documentation for RCBot2 integration.

## Table of Contents

- [Overview](#overview)
- [Setup](#setup)
- [Natives](#natives)
- [Forwards](#forwards)
- [Usage Examples](#usage-examples)
- [Integration Guide](#integration-guide)

---

## Overview

RCBot2 provides optional SourceMod natives for controlling and interacting with bots from SourcePawn plugins. This allows server operators and plugin developers to:

- Create and manage bots programmatically
- Control bot behavior and settings
- Detect and query bot status
- Hook into bot events
- Customize bot loadouts and properties

---

## Setup

### Prerequisites

- **SourceMod 1.10+** installed on server
- **RCBot2 built with SourceMod support** (`--sm-path` during build)
- **Extension loaded**: RCBot2 SM extension must be loaded

### Enabling SourceMod Integration

1. **Build RCBot2 with SM support**:
   ```bash
   python configure.py -s TF2 --sm-path ~/sourcemod --mms-path ... --hl2sdk-root ...
   ```

2. **Verify extension loaded**:
   ```
   sm exts list
   ```
   Look for "RCBot2" in the list

3. **Include in your plugin**:
   ```sourcepawn
   #include <rcbot2>
   ```

---

## Natives

### Bot Management

#### `RCBot_CreateBot`

Create a new bot.

```sourcepawn
/**
 * Creates a new RCBot
 *
 * @param name      Bot name (empty string for random)
 * @param class     Bot class (empty for random)
 * @param team      Team number (0 for auto-assign)
 * @return          Client index of created bot, or 0 on failure
 */
native int RCBot_CreateBot(const char[] name = "", const char[] class = "", int team = 0);
```

**Example**:
```sourcepawn
int botClient = RCBot_CreateBot("MyBot", "soldier", 2);
if (botClient > 0) {
    PrintToServer("Created bot: %d", botClient);
}
```

---

#### `RCBot_KickBot`

Remove a specific bot or random bot.

```sourcepawn
/**
 * Kicks an RCBot
 *
 * @param client    Client index of bot to kick (0 for random)
 * @param team      Team to kick from (0 for any team)
 * @return          True on success, false on failure
 */
native bool RCBot_KickBot(int client = 0, int team = 0);
```

**Example**:
```sourcepawn
// Kick random bot
RCBot_KickBot();

// Kick specific bot
RCBot_KickBot(botClient);

// Kick random bot from team 2
RCBot_KickBot(0, 2);
```

---

#### `RCBot_RemoveAll`

Remove all bots from the server.

```sourcepawn
/**
 * Removes all RCBots from the server
 *
 * @return          Number of bots removed
 */
native int RCBot_RemoveAll();
```

**Example**:
```sourcepawn
int removed = RCBot_RemoveAll();
PrintToServer("Removed %d bots", removed);
```

---

### Bot Queries

#### `RCBot_IsBot`

Check if a client is an RCBot.

```sourcepawn
/**
 * Checks if a client is an RCBot
 *
 * @param client    Client index to check
 * @return          True if client is an RCBot, false otherwise
 */
native bool RCBot_IsBot(int client);
```

**Example**:
```sourcepawn
if (RCBot_IsBot(client)) {
    PrintToServer("Client %d is an RCBot", client);
}
```

---

#### `RCBot_GetBotCount`

Get the number of RCBots on the server.

```sourcepawn
/**
 * Gets the number of RCBots currently on the server
 *
 * @return          Number of RCBots
 */
native int RCBot_GetBotCount();
```

**Example**:
```sourcepawn
int botCount = RCBot_GetBotCount();
PrintToServer("There are %d bots on the server", botCount);
```

---

#### `RCBot_GetBotSkill`

Get a bot's skill level.

```sourcepawn
/**
 * Gets a bot's current skill level
 *
 * @param client    Client index of bot
 * @return          Bot skill level (0.0 - 1.0), or -1.0 if not a bot
 */
native float RCBot_GetBotSkill(int client);
```

**Example**:
```sourcepawn
float skill = RCBot_GetBotSkill(client);
if (skill >= 0.0) {
    PrintToServer("Bot skill: %.2f", skill);
}
```

---

#### `RCBot_SetBotSkill`

Set a bot's skill level.

```sourcepawn
/**
 * Sets a bot's skill level
 *
 * @param client    Client index of bot
 * @param skill     Skill level (0.0 - 1.0)
 * @return          True on success, false on failure
 */
native bool RCBot_SetBotSkill(int client, float skill);
```

**Example**:
```sourcepawn
// Set bot to hard difficulty
RCBot_SetBotSkill(client, 0.8);
```

---

### Bot Behavior Control

#### `RCBot_SetBotQuota`

Set the bot quota (number of bots to maintain).

```sourcepawn
/**
 * Sets the bot quota
 *
 * @param quota     Number of bots to maintain
 * @return          True on success
 */
native bool RCBot_SetBotQuota(int quota);
```

**Example**:
```sourcepawn
// Maintain 10 bots
RCBot_SetBotQuota(10);
```

---

#### `RCBot_GetBotQuota`

Get the current bot quota.

```sourcepawn
/**
 * Gets the current bot quota
 *
 * @return          Bot quota value
 */
native int RCBot_GetBotQuota();
```

---

### Waypoint Interaction

#### `RCBot_GetNearestWaypoint`

Get the nearest waypoint to a position.

```sourcepawn
/**
 * Gets the nearest waypoint to a position
 *
 * @param pos       Position vector
 * @return          Waypoint index, or -1 if none found
 */
native int RCBot_GetNearestWaypoint(const float pos[3]);
```

**Example**:
```sourcepawn
float pos[3];
GetClientAbsOrigin(client, pos);
int waypoint = RCBot_GetNearestWaypoint(pos);
```

---

#### `RCBot_GetWaypointCount`

Get the total number of waypoints on the current map.

```sourcepawn
/**
 * Gets the number of waypoints on the current map
 *
 * @return          Waypoint count
 */
native int RCBot_GetWaypointCount();
```

---

## Forwards

### `RCBot_OnBotCreated`

Called when an RCBot is created.

```sourcepawn
/**
 * Called when an RCBot is created
 *
 * @param client    Client index of the bot
 * @param name      Bot's name
 * @param team      Bot's team
 */
forward void RCBot_OnBotCreated(int client, const char[] name, int team);
```

**Example**:
```sourcepawn
public void RCBot_OnBotCreated(int client, const char[] name, int team) {
    PrintToServer("RCBot %s joined team %d", name, team);
}
```

---

### `RCBot_OnBotKicked`

Called when an RCBot is removed.

```sourcepawn
/**
 * Called when an RCBot is kicked
 *
 * @param client    Client index of the bot
 */
forward void RCBot_OnBotKicked(int client);
```

**Example**:
```sourcepawn
public void RCBot_OnBotKicked(int client) {
    PrintToServer("RCBot %d was kicked", client);
}
```

---

### `RCBot_OnBotThink`

Called every think frame for each bot (advanced).

```sourcepawn
/**
 * Called during bot think cycle
 *
 * @param client    Client index of the bot
 */
forward Action RCBot_OnBotThink(int client);
```

**Return values**:
- `Plugin_Continue` - Allow normal bot think
- `Plugin_Handled` - Block bot think for this frame
- `Plugin_Stop` - Stop calling other forwards

---

## Usage Examples

### Example 1: Auto-Balance Plugin

Maintain specific bot count based on human players:

```sourcepawn
#include <sourcemod>
#include <rcbot2>

ConVar g_cvTotalPlayers;

public void OnPluginStart() {
    g_cvTotalPlayers = CreateConVar("sm_rcbot_total_players", "12",
        "Total players (humans + bots) to maintain");

    CreateTimer(5.0, Timer_CheckBotQuota, _, TIMER_REPEAT);
}

public Action Timer_CheckBotQuota(Handle timer) {
    int humanCount = 0;
    int botCount = 0;

    for (int i = 1; i <= MaxClients; i++) {
        if (!IsClientInGame(i)) continue;

        if (RCBot_IsBot(i)) {
            botCount++;
        } else if (!IsFakeClient(i)) {
            humanCount++;
        }
    }

    int targetTotal = g_cvTotalPlayers.IntValue;
    int currentTotal = humanCount + botCount;
    int needed = targetTotal - currentTotal;

    if (needed > 0) {
        // Add bots
        RCBot_SetBotQuota(botCount + needed);
    } else if (needed < 0) {
        // Remove bots
        for (int i = 0; i < -needed; i++) {
            RCBot_KickBot();
        }
    }

    return Plugin_Continue;
}
```

---

### Example 2: Skill-Based Bot Management

Adjust bot skill based on human player performance:

```sourcepawn
#include <sourcemod>
#include <rcbot2>

float g_fAverageHumanKD = 1.0;

public void OnPluginStart() {
    HookEvent("player_death", Event_PlayerDeath);
    CreateTimer(30.0, Timer_AdjustBotSkill, _, TIMER_REPEAT);
}

public Action Timer_AdjustBotSkill(Handle timer) {
    // Calculate average human K/D
    g_fAverageHumanKD = CalculateAverageKD();

    // Adjust bot skill inversely to human performance
    float targetSkill = 1.0 / (g_fAverageHumanKD + 0.5);
    targetSkill = Math_Clamp(targetSkill, 0.3, 0.9);

    // Apply to all bots
    for (int i = 1; i <= MaxClients; i++) {
        if (IsClientInGame(i) && RCBot_IsBot(i)) {
            RCBot_SetBotSkill(i, targetSkill);
        }
    }

    return Plugin_Continue;
}

float CalculateAverageKD() {
    // Implementation here
    return 1.0;
}

float Math_Clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}
```

---

### Example 3: Bot Team Balancer

Ensure bots are distributed evenly across teams:

```sourcepawn
#include <sourcemod>
#include <rcbot2>
#include <tf2>

public void OnPluginStart() {
    CreateTimer(10.0, Timer_BalanceBots, _, TIMER_REPEAT);
}

public Action Timer_BalanceBots(Handle timer) {
    int redBots = 0, bluBots = 0;

    for (int i = 1; i <= MaxClients; i++) {
        if (!IsClientInGame(i) || !RCBot_IsBot(i)) continue;

        TFTeam team = TF2_GetClientTeam(i);
        if (team == TFTeam_Red) redBots++;
        else if (team == TFTeam_Blue) bluBots++;
    }

    // Balance if difference > 1
    if (Abs(redBots - bluBots) > 1) {
        TFTeam targetTeam = (redBots > bluBots) ? TFTeam_Blue : TFTeam_Red;

        // Find bot on overpopulated team and switch
        for (int i = 1; i <= MaxClients; i++) {
            if (!IsClientInGame(i) || !RCBot_IsBot(i)) continue;

            if (TF2_GetClientTeam(i) != targetTeam) {
                TF2_ChangeClientTeam(i, targetTeam);
                break;
            }
        }
    }

    return Plugin_Continue;
}

int Abs(int value) {
    return value < 0 ? -value : value;
}
```

---

### Example 4: Custom Bot Names

Assign custom names to bots when they're created:

```sourcepawn
#include <sourcemod>
#include <rcbot2>

char g_sBotNames[][] = {
    "Alpha", "Bravo", "Charlie", "Delta", "Echo",
    "Foxtrot", "Golf", "Hotel", "India", "Juliet"
};

int g_iNameIndex = 0;

public void RCBot_OnBotCreated(int client, const char[] name, int team) {
    // Set custom name
    char newName[MAX_NAME_LENGTH];
    Format(newName, sizeof(newName), "[BOT] %s", g_sBotNames[g_iNameIndex]);
    SetClientName(client, newName);

    // Cycle to next name
    g_iNameIndex = (g_iNameIndex + 1) % sizeof(g_sBotNames);
}
```

---

## Integration Guide

### Creating a Plugin with RCBot2 Support

1. **Create include file** (if not provided):

Save as `addons/sourcemod/scripting/include/rcbot2.inc`:

```sourcepawn
#if defined _rcbot2_included
  #endinput
#endif
#define _rcbot2_included

// Natives
native int RCBot_CreateBot(const char[] name = "", const char[] class = "", int team = 0);
native bool RCBot_KickBot(int client = 0, int team = 0);
native int RCBot_RemoveAll();
native bool RCBot_IsBot(int client);
native int RCBot_GetBotCount();
native float RCBot_GetBotSkill(int client);
native bool RCBot_SetBotSkill(int client, float skill);
native bool RCBot_SetBotQuota(int quota);
native int RCBot_GetBotQuota();
native int RCBot_GetNearestWaypoint(const float pos[3]);
native int RCBot_GetWaypointCount();

// Forwards
forward void RCBot_OnBotCreated(int client, const char[] name, int team);
forward void RCBot_OnBotKicked(int client);
forward Action RCBot_OnBotThink(int client);

public SharedPlugin __pl_rcbot2 = {
    name = "rcbot2",
    file = "rcbot2.smx",
    #if defined REQUIRE_PLUGIN
    required = 1,
    #else
    required = 0,
    #endif
};

#if !defined REQUIRE_PLUGIN
public void __pl_rcbot2_SetNTVOptional() {
    MarkNativeAsOptional("RCBot_CreateBot");
    MarkNativeAsOptional("RCBot_KickBot");
    MarkNativeAsOptional("RCBot_RemoveAll");
    MarkNativeAsOptional("RCBot_IsBot");
    MarkNativeAsOptional("RCBot_GetBotCount");
    MarkNativeAsOptional("RCBot_GetBotSkill");
    MarkNativeAsOptional("RCBot_SetBotSkill");
    MarkNativeAsOptional("RCBot_SetBotQuota");
    MarkNativeAsOptional("RCBot_GetBotQuota");
    MarkNativeAsOptional("RCBot_GetNearestWaypoint");
    MarkNativeAsOptional("RCBot_GetWaypointCount");
}
#endif
```

2. **Include in your plugin**:

```sourcepawn
#include <sourcemod>
#include <rcbot2>

public void OnPluginStart() {
    // Your plugin code
}
```

3. **Compile and test**:

```bash
./spcomp myplugin.sp
```

---

## Notes

- **Optional natives**: Use `#if !defined REQUIRE_PLUGIN` for optional RCBot2 support
- **Check availability**: Use `LibraryExists("rcbot2")` to verify extension is loaded
- **Performance**: Avoid calling natives every frame; use timers for periodic checks
- **Compatibility**: Test with RCBot2 disabled to ensure graceful degradation

---

**See Also**:
- [SourceMod API](https://sm.alliedmods.net/new-api/) - SourceMod scripting reference
- [Development Guide](../claude.md) - RCBot2 development information
- [Configuration Guide](configuration.md) - RCBot2 settings

---

**Last Updated**: 2025-11-21
**Status**: API may expand in future versions
