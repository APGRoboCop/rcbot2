# Phase 1 Implementation Guide: Essential Bot Control

This guide provides step-by-step instructions for implementing Phase 1 of the SourceMod Integration Roadmap.

---

## Overview

Phase 1 focuses on exposing essential bot control, weapon management, and task system queries to SourcePawn. This phase builds upon the existing 7 natives and adds approximately 20-25 new natives.

**Estimated Timeline**: 2-4 weeks
**Priority**: High
**Dependencies**: None (builds on existing infrastructure)

---

## Pre-Implementation Checklist

- [ ] SourceMod 1.11+ development environment set up
- [ ] RCBot2 compiles successfully with existing natives
- [ ] Familiar with existing native implementations in `sm_ext/bot_sm_natives.cpp`
- [ ] Have access to test server for validation

---

## Step 1: Define New Enumerations

### File: `scripting/include/rcbot2.inc`

Add new enumerations after the existing `RCBotProfileVar` enum:

```sourcepawn
/**
 * Bot objectives that can be assigned
 */
enum RCBotObjective {
    RCBotObjective_None = 0,        // No specific objective
    RCBotObjective_Attack,          // Attack a position/entity
    RCBotObjective_Defend,          // Defend a position
    RCBotObjective_Goto,            // Navigate to position
    RCBotObjective_Retreat,         // Retreat from danger
    RCBotObjective_Follow           // Follow an entity
};

/**
 * Bot actions that can be forced
 */
enum RCBotAction {
    RCBotAction_Jump = 0,           // Make bot jump
    RCBotAction_Crouch,             // Toggle crouch
    RCBotAction_Scope,              // Toggle scope (sniper)
    RCBotAction_Reload,             // Force reload
    RCBotAction_UseItem             // Use current item
};

/**
 * Bot task types (read-only, for queries)
 */
enum RCBotTask {
    RCBotTask_None = 0,
    RCBotTask_FindPath,
    RCBotTask_GetAmmo,
    RCBotTask_GetHealth,
    RCBotTask_Attack,
    RCBotTask_Defend,
    RCBotTask_Build,
    RCBotTask_UseDispenser,
    RCBotTask_UseTeleporter,
    RCBotTask_FindEnemy,
    RCBotTask_AvoidDanger,
    RCBotTask_FollowLeader
};
```

---

## Step 2: Declare New Natives in SourcePawn Header

### File: `scripting/include/rcbot2.inc`

Add native declarations after existing natives:

```sourcepawn
//=============================================================================
// Phase 1: Bot Command & Control
//=============================================================================

/**
 * Sets a bot's primary objective with an optional target position.
 *
 * @param client        Bot client index
 * @param objective     Objective type to assign
 * @param origin        Target position (used for Goto, Attack, Defend)
 * @return              true on success, false on failure
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_SetBotObjective(int client, RCBotObjective objective, const float origin[3] = {0.0, 0.0, 0.0});

/**
 * Forces a bot to target a specific enemy.
 *
 * @param client        Bot client index
 * @param target        Target client index
 * @return              true on success, false on failure
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_SetBotEnemy(int client, int target);

/**
 * Clears a bot's current enemy target.
 *
 * @param client        Bot client index
 * @return              true on success, false on failure
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_ClearBotEnemy(int client);

/**
 * Forces a bot to perform a specific action.
 *
 * @param client        Bot client index
 * @param action        Action to perform
 * @return              true on success, false on failure
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_ForceBotAction(int client, RCBotAction action);

/**
 * Gets the bot's current team.
 *
 * @param client        Bot client index
 * @return              Team index, or -1 on failure
 * @error               Invalid client index or client is not an RCBot
 */
native int RCBot2_GetBotTeam(int client);

/**
 * Gets the bot's current health.
 *
 * @param client        Bot client index
 * @return              Health value, or -1 on failure
 * @error               Invalid client index or client is not an RCBot
 */
native int RCBot2_GetBotHealth(int client);

/**
 * Gets the bot's current enemy target.
 *
 * @param client        Bot client index
 * @return              Enemy client index, or -1 if no enemy
 * @error               Invalid client index or client is not an RCBot
 */
native int RCBot2_GetBotEnemy(int client);

/**
 * Gets the bot's current position.
 *
 * @param client        Bot client index
 * @param origin        Array to store position
 * @return              true on success, false on failure
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_GetBotOrigin(int client, float origin[3]);

/**
 * Gets the bot's current eye angles.
 *
 * @param client        Bot client index
 * @param angles        Array to store angles
 * @return              true on success, false on failure
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_GetBotEyeAngles(int client, float angles[3]);

//=============================================================================
// Phase 1: Weapon & Equipment Management
//=============================================================================

/**
 * Forces a bot to select a specific weapon.
 *
 * @param client        Bot client index
 * @param weapon        Weapon classname (e.g., "weapon_crowbar")
 * @return              true on success, false on failure
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_SelectWeapon(int client, const char[] weapon);

/**
 * Gets the bot's currently selected weapon.
 *
 * @param client        Bot client index
 * @param weapon        Buffer to store weapon classname
 * @param maxlength     Maximum buffer size
 * @return              true on success, false on failure
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_GetCurrentWeapon(int client, char[] weapon, int maxlength);

/**
 * Forces a bot to attack with current weapon.
 *
 * @param client        Bot client index
 * @param primary       true for primary fire, false for secondary
 * @return              true on success, false on failure
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_ForceAttack(int client, bool primary = true);

/**
 * Forces a bot to reload current weapon.
 *
 * @param client        Bot client index
 * @return              true on success, false on failure
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_ForceReload(int client);

//=============================================================================
// Phase 1: Task System Queries
//=============================================================================

/**
 * Gets the bot's current primary task.
 *
 * @param client        Bot client index
 * @return              Current RCBotTask, or RCBotTask_None on failure
 * @error               Invalid client index or client is not an RCBot
 */
native RCBotTask RCBot2_GetCurrentTask(int client);

/**
 * Clears all of a bot's scheduled tasks.
 *
 * @param client        Bot client index
 * @return              true on success, false on failure
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_ClearSchedule(int client);

/**
 * Gets the progress of the bot's current task (0.0 to 1.0).
 *
 * @param client        Bot client index
 * @return              Task progress as float, or -1.0 on failure
 * @error               Invalid client index or client is not an RCBot
 */
native float RCBot2_GetTaskProgress(int client);
```

---

## Step 3: Declare Native Functions in C++ Header

### File: `sm_ext/bot_sm_natives.h`

Add declarations for new native implementations:

```cpp
#ifndef _INCLUDE_SOURCEMOD_EXTENSION_NATIVES_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_NATIVES_H_

#include <IExtension.h>

using namespace SourceMod;

// Existing natives
cell_t sm_RCBotIsWaypointAvailable(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotCreate(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotSetProfileInt(IPluginContext* pContext, const cell_t* params);
cell_t sm_RCBotSetProfileFloat(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotGetProfileInt(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotGetProfileFloat(IPluginContext* pContext, const cell_t* params);
cell_t sm_RCBotIsClientBot(IPluginContext *pContext, const cell_t *params);

// Phase 1: Bot Command & Control
cell_t sm_RCBotSetBotObjective(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotSetBotEnemy(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotClearBotEnemy(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotForceBotAction(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotGetBotTeam(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotGetBotHealth(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotGetBotEnemy(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotGetBotOrigin(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotGetBotEyeAngles(IPluginContext *pContext, const cell_t *params);

// Phase 1: Weapon & Equipment Management
cell_t sm_RCBotSelectWeapon(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotGetCurrentWeapon(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotForceAttack(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotForceReload(IPluginContext *pContext, const cell_t *params);

// Phase 1: Task System Queries
cell_t sm_RCBotGetCurrentTask(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotClearSchedule(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotGetTaskProgress(IPluginContext *pContext, const cell_t *params);

// Native registration array
extern const sp_nativeinfo_t g_RCBotNatives[];

#endif // _INCLUDE_SOURCEMOD_EXTENSION_NATIVES_H_
```

---

## Step 4: Implement Native Functions

### File: `sm_ext/bot_sm_natives.cpp`

Add implementations at the end of the file:

```cpp
//=============================================================================
// Phase 1: Bot Command & Control
//=============================================================================

/* native bool RCBot2_SetBotObjective(int client, RCBotObjective objective, const float origin[3]); */
cell_t sm_RCBotSetBotObjective(IPluginContext *pContext, const cell_t *params) {
    const int client = params[1];
    const int objective = params[2];

    // Validate client
    if (client < 1 || client > gpGlobals->maxClients) {
        return pContext->ThrowNativeError("Invalid client index %d", client);
    }

    CBot* bot = CBots::getBot(client - 1);
    if (!bot) {
        return pContext->ThrowNativeError("Client index %d is not a RCBot", client);
    }

    // Get origin parameter
    cell_t *addr;
    pContext->LocalToPhysAddr(params[3], &addr);
    Vector vOrigin(sp_ctof(addr[0]), sp_ctof(addr[1]), sp_ctof(addr[2]));

    // Implementation depends on objective type
    // This is a simplified example - actual implementation needs task system integration
    switch (objective) {
        case 0: // None
            bot->getCurrentTask()->setComplete();
            return 1;
        case 1: // Attack
            // Set attack task at position
            // TODO: Implement via task system
            return 1;
        case 2: // Defend
            // Set defend task
            // TODO: Implement via task system
            return 1;
        case 3: // Goto
            bot->setMoveTo(vOrigin);
            return 1;
        default:
            return 0;
    }

    return 0;
}

/* native bool RCBot2_SetBotEnemy(int client, int target); */
cell_t sm_RCBotSetBotEnemy(IPluginContext *pContext, const cell_t *params) {
    const int client = params[1];
    const int target = params[2];

    if (client < 1 || client > gpGlobals->maxClients) {
        return pContext->ThrowNativeError("Invalid client index %d", client);
    }

    CBot* bot = CBots::getBot(client - 1);
    if (!bot) {
        return pContext->ThrowNativeError("Client index %d is not a RCBot", client);
    }

    if (target < 1 || target > gpGlobals->maxClients) {
        return pContext->ThrowNativeError("Invalid target index %d", target);
    }

    edict_t *pTarget = engine->PEntityOfEntIndex(target);
    if (!pTarget || pTarget->IsFree()) {
        return 0;
    }

    bot->setEnemy(pTarget);
    return 1;
}

/* native bool RCBot2_ClearBotEnemy(int client); */
cell_t sm_RCBotClearBotEnemy(IPluginContext *pContext, const cell_t *params) {
    const int client = params[1];

    if (client < 1 || client > gpGlobals->maxClients) {
        return pContext->ThrowNativeError("Invalid client index %d", client);
    }

    CBot* bot = CBots::getBot(client - 1);
    if (!bot) {
        return pContext->ThrowNativeError("Client index %d is not a RCBot", client);
    }

    bot->setEnemy(nullptr);
    return 1;
}

/* native int RCBot2_GetBotTeam(int client); */
cell_t sm_RCBotGetBotTeam(IPluginContext *pContext, const cell_t *params) {
    const int client = params[1];

    if (client < 1 || client > gpGlobals->maxClients) {
        return pContext->ThrowNativeError("Invalid client index %d", client);
    }

    const CBot* bot = CBots::getBot(client - 1);
    if (!bot) {
        return pContext->ThrowNativeError("Client index %d is not a RCBot", client);
    }

    return bot->getTeam();
}

/* native int RCBot2_GetBotHealth(int client); */
cell_t sm_RCBotGetBotHealth(IPluginContext *pContext, const cell_t *params) {
    const int client = params[1];

    if (client < 1 || client > gpGlobals->maxClients) {
        return pContext->ThrowNativeError("Invalid client index %d", client);
    }

    const CBot* bot = CBots::getBot(client - 1);
    if (!bot) {
        return pContext->ThrowNativeError("Client index %d is not a RCBot", client);
    }

    return bot->getHealthPercent() * 100; // Return as percentage
}

/* native int RCBot2_GetBotEnemy(int client); */
cell_t sm_RCBotGetBotEnemy(IPluginContext *pContext, const cell_t *params) {
    const int client = params[1];

    if (client < 1 || client > gpGlobals->maxClients) {
        return pContext->ThrowNativeError("Invalid client index %d", client);
    }

    const CBot* bot = CBots::getBot(client - 1);
    if (!bot) {
        return pContext->ThrowNativeError("Client index %d is not a RCBot", client);
    }

    edict_t *pEnemy = bot->getEnemy();
    if (!pEnemy || pEnemy->IsFree()) {
        return -1;
    }

    return engine->IndexOfEdict(pEnemy);
}

/* native bool RCBot2_GetBotOrigin(int client, float origin[3]); */
cell_t sm_RCBotGetBotOrigin(IPluginContext *pContext, const cell_t *params) {
    const int client = params[1];

    if (client < 1 || client > gpGlobals->maxClients) {
        return pContext->ThrowNativeError("Invalid client index %d", client);
    }

    const CBot* bot = CBots::getBot(client - 1);
    if (!bot) {
        return pContext->ThrowNativeError("Client index %d is not a RCBot", client);
    }

    Vector vOrigin = bot->getOrigin();

    cell_t *addr;
    pContext->LocalToPhysAddr(params[2], &addr);
    addr[0] = sp_ftoc(vOrigin.x);
    addr[1] = sp_ftoc(vOrigin.y);
    addr[2] = sp_ftoc(vOrigin.z);

    return 1;
}

// Continue with other implementations...
```

---

## Step 5: Register New Natives

### File: `sm_ext/bot_sm_natives.cpp`

Update the native registration array:

```cpp
const sp_nativeinfo_t g_RCBotNatives[] = {
    // Existing natives
    {"RCBot2_IsWaypointAvailable", sm_RCBotIsWaypointAvailable},
    {"RCBot2_CreateBot", sm_RCBotCreate},
    {"IsRCBot2Client", sm_RCBotIsClientBot},
    {"RCBot2_SetProfileInt", sm_RCBotSetProfileInt},
    {"RCBot2_GetProfileInt", sm_RCBotGetProfileInt},
    {"RCBot2_SetProfileFloat", sm_RCBotSetProfileFloat},
    {"RCBot2_GetProfileFloat", sm_RCBotGetProfileFloat},

    // Phase 1: Bot Command & Control
    {"RCBot2_SetBotObjective", sm_RCBotSetBotObjective},
    {"RCBot2_SetBotEnemy", sm_RCBotSetBotEnemy},
    {"RCBot2_ClearBotEnemy", sm_RCBotClearBotEnemy},
    {"RCBot2_ForceBotAction", sm_RCBotForceBotAction},
    {"RCBot2_GetBotTeam", sm_RCBotGetBotTeam},
    {"RCBot2_GetBotHealth", sm_RCBotGetBotHealth},
    {"RCBot2_GetBotEnemy", sm_RCBotGetBotEnemy},
    {"RCBot2_GetBotOrigin", sm_RCBotGetBotOrigin},
    {"RCBot2_GetBotEyeAngles", sm_RCBotGetBotEyeAngles},

    // Phase 1: Weapon & Equipment
    {"RCBot2_SelectWeapon", sm_RCBotSelectWeapon},
    {"RCBot2_GetCurrentWeapon", sm_RCBotGetCurrentWeapon},
    {"RCBot2_ForceAttack", sm_RCBotForceAttack},
    {"RCBot2_ForceReload", sm_RCBotForceReload},

    // Phase 1: Task System
    {"RCBot2_GetCurrentTask", sm_RCBotGetCurrentTask},
    {"RCBot2_ClearSchedule", sm_RCBotClearSchedule},
    {"RCBot2_GetTaskProgress", sm_RCBotGetTaskProgress},

    {nullptr, nullptr}
};
```

---

## Step 6: Add Helper Methods to CBot Class

### File: `utils/RCBot2_meta/bot.h`

Add public accessor methods to the `CBot` class:

```cpp
class CBot {
public:
    // Existing methods...

    // Phase 1: Accessors for SourceMod integration
    inline edict_t* getEnemy() const { return m_pEnemy; }
    inline void setEnemy(edict_t* pEnemy) { m_pEnemy = pEnemy; }
    inline int getTeam() const { return m_iTeam; }
    inline Vector getOrigin() const { return m_vOrigin; }
    inline QAngle getEyeAngles() const { return m_vEyeAngles; }
    inline float getHealthPercent() const;
    inline CBotTask* getCurrentTask() const;
    inline void setMoveTo(const Vector& vOrigin);

    // ... rest of class
};
```

### File: `utils/RCBot2_meta/bot.cpp`

Implement helper methods:

```cpp
float CBot::getHealthPercent() const {
    if (!m_pEdict || m_pEdict->IsFree()) {
        return 0.0f;
    }

    int health = getHealth();
    int maxHealth = getMaxHealth();

    if (maxHealth <= 0) {
        return 0.0f;
    }

    return static_cast<float>(health) / static_cast<float>(maxHealth);
}

CBotTask* CBot::getCurrentTask() const {
    if (m_pSchedule && !m_pSchedule->isEmpty()) {
        return m_pSchedule->getCurrentTask();
    }
    return nullptr;
}

void CBot::setMoveTo(const Vector& vOrigin) {
    // Create a simple move task
    m_vMoveTo = vOrigin;
    m_bHasMoveTo = true;
}
```

---

## Step 7: Testing

### Create Test Plugin

**File**: `scripting/rcbot_phase1_test.sp`

```sourcepawn
#include <sourcemod>
#include <rcbot2>

#pragma semicolon 1
#pragma newdecls required

public void OnPluginStart() {
    RegAdminCmd("sm_rcbot_test_phase1", CMD_TestPhase1, ADMFLAG_ROOT, "Test Phase 1 natives");
}

public Action CMD_TestPhase1(int client, int args) {
    // Find first RCBot
    int bot = -1;
    for (int i = 1; i <= MaxClients; i++) {
        if (IsClientInGame(i) && IsRCBot2Client(i)) {
            bot = i;
            break;
        }
    }

    if (bot == -1) {
        // Create a bot
        bot = RCBot2_CreateBot("TestBot");
        if (bot == -1) {
            ReplyToCommand(client, "[RCBot] Failed to create bot");
            return Plugin_Handled;
        }
    }

    // Test Bot Command & Control
    ReplyToCommand(client, "[RCBot] Testing Bot #%d", bot);

    int team = RCBot2_GetBotTeam(bot);
    ReplyToCommand(client, "Team: %d", team);

    int health = RCBot2_GetBotHealth(bot);
    ReplyToCommand(client, "Health: %d%%", health);

    float origin[3];
    if (RCBot2_GetBotOrigin(bot, origin)) {
        ReplyToCommand(client, "Origin: %.1f, %.1f, %.1f", origin[0], origin[1], origin[2]);
    }

    // Test enemy targeting
    if (args >= 1) {
        char arg[8];
        GetCmdArg(1, arg, sizeof(arg));
        int target = StringToInt(arg);

        if (RCBot2_SetBotEnemy(bot, target)) {
            ReplyToCommand(client, "Bot now targeting client #%d", target);
        }
    }

    int enemy = RCBot2_GetBotEnemy(bot);
    ReplyToCommand(client, "Current enemy: %d", enemy);

    // Test task queries
    RCBotTask task = RCBot2_GetCurrentTask(bot);
    ReplyToCommand(client, "Current task: %d", task);

    float progress = RCBot2_GetTaskProgress(bot);
    ReplyToCommand(client, "Task progress: %.1f%%", progress * 100.0);

    return Plugin_Handled;
}
```

### Compile and Test

```bash
cd build
./ambuild

# Copy to server
cp package/addons/metamod/bin/* $SERVER/addons/metamod/bin/
cp package/addons/rcbot2/bin/* $SERVER/addons/rcbot2/bin/

# Test in-game
sm_rcbot_test_phase1
sm_rcbot_test_phase1 2  # Set bot to target client #2
```

---

## Step 8: Validation Checklist

- [ ] All Phase 1 natives compile without errors
- [ ] Natives validate client indexes correctly
- [ ] Natives throw proper errors for invalid inputs
- [ ] Test plugin demonstrates all new functionality
- [ ] No crashes with invalid parameters
- [ ] Memory leaks checked with Valgrind (Linux)
- [ ] Performance impact measured (<2% overhead)
- [ ] Documentation updated in .inc file
- [ ] Example usage added to wiki

---

## Common Pitfalls & Solutions

### Issue: Client Index Off-by-One

**Problem**: RCBot uses 0-based indexing, SourcePawn uses 1-based.

**Solution**: Always subtract 1 when calling `CBots::getBot()`:
```cpp
const CBot* bot = CBots::getBot(client - 1);  // Convert 1-based to 0-based
```

### Issue: Invalid Edict Access

**Problem**: Edict might be freed between calls.

**Solution**: Always validate before use:
```cpp
edict_t *pEnemy = bot->getEnemy();
if (!pEnemy || pEnemy->IsFree()) {
    return -1;
}
```

### Issue: Vector Parameter Passing

**Problem**: Float arrays need special handling.

**Solution**: Use `LocalToPhysAddr` for array parameters:
```cpp
cell_t *addr;
pContext->LocalToPhysAddr(params[2], &addr);
Vector vOrigin(sp_ctof(addr[0]), sp_ctof(addr[1]), sp_ctof(addr[2]));
```

---

## Next Steps

After completing Phase 1:

1. Gather community feedback on API design
2. Begin Phase 2: Game-Specific Extensions
3. Create comprehensive test suite
4. Update documentation with real-world examples
5. Consider performance optimizations

---

## Resources

- [SourceMod Native API Reference](https://sm.alliedmods.net/new-api/core/IPluginContext)
- [RCBot2 Source Code](https://github.com/APGRoboCop/rcbot2)
- [Bots United Discord](https://discord.gg/5v5YvKG4Hr)
- [SourceMod Forums](https://forums.alliedmods.net/)

---

**Document Version**: 1.0
**Author**: RCBot2 Development Team
**Last Updated**: 2025-11-22
