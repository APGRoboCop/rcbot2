# RCBot2 SourceMod Integration Roadmap

## Overview

This roadmap outlines the implementation plan for two major SourceMod enhancements to RCBot2:

1. **Extended SourceMod Natives** - Comprehensive control and customization APIs
2. **SourceMod Plugin Suite** - Management UI, balancing, and statistics plugins

These enhancements will provide server administrators and plugin developers with powerful tools to manage, customize, and monitor RCBot2 bots through SourceMod.

---

## Current State Analysis

### Existing SourceMod Extension (`sm_ext/`)

The current RCBot2 SourceMod extension provides a basic foundation:

**Current Natives** (see `sm_ext/bot_sm_natives.h`):
- `RCBot2_CreateBot()` - Create a bot
- `IsRCBot2Client()` - Check if client is an RCBot
- `RCBot2_IsWaypointAvailable()` - Check waypoint availability
- `RCBot2_SetProfileInt/Float()` - Set profile properties
- `RCBot2_GetProfileInt/Float()` - Get profile properties
- `RCBot2_KickBot()` - Kick a specific bot
- `RCBot2_KickAll()` - Kick all bots
- `RCBot2_GetBotCount()` - Get bot count
- `RCBot2_SetSkill()` - Set bot skill level
- `RCBot2_GetSkill()` - Get bot skill level
- `RCBot2_GetTeam()` - Get bot team
- `RCBot2_GetClass()` - Get bot class
- `RCBot2_SetClass()` - Set bot class

**Profile Variables Exposed** (see `rcbot2.inc`):
- `RCBotProfile_iVisionTicks` - Entity recognition speed
- `RCBotProfile_iPathTicks` - Pathfinding speed
- `RCBotProfile_iVisionTicksClients` - Player recognition speed
- `RCBotProfile_iSensitivity` - Mouse sensitivity (1-20)
- `RCBotProfile_fBraveness` - Danger sensitivity (0.0-1.0)
- `RCBotProfile_fAimSkill` - Aim prediction (0.0-1.0)
- `RCBotProfile_iClass` - Preferred class

### Current Limitations

1. **No behavior control** - Can't modify schedules, tasks, or goals
2. **No state introspection** - Can't query bot's current action, target, or state
3. **No weapon control** - Can't force weapon usage or manage loadout
4. **No navigation control** - Can't set goals or waypoint destinations
5. **No statistics tracking** - No performance metrics or combat stats
6. **No event forwarding** - SourceMod plugins can't react to bot events
7. **Limited customization** - Only basic profile properties exposed

---

## Phase 1: Extended SourceMod Natives

### Goal
Provide comprehensive native API for controlling and monitoring RCBot2 bots from SourceMod plugins.

### Implementation Phases

#### Phase 1.1: Bot State and Information Natives

**File**: `sm_ext/bot_sm_natives.cpp`

**New Natives to Implement**:

```cpp
// Bot State Information
native bool RCBot2_GetCurrentTask(int client, char[] buffer, int maxlen);
native int RCBot2_GetCurrentEnemy(int client);
native int RCBot2_GetCurrentGoal(int client);
native bool RCBot2_IsBusy(int client);
native bool RCBot2_IsInCombat(int client);
native float RCBot2_GetHealth(int client);
native float RCBot2_GetHealthPercent(int client);

// Bot Position and Navigation
native void RCBot2_GetPosition(int client, float pos[3]);
native void RCBot2_GetEyePosition(int client, float pos[3]);
native void RCBot2_GetVelocity(int client, float vel[3]);
native int RCBot2_GetCurrentWaypoint(int client);
native int RCBot2_GetNextWaypoint(int client);
native float RCBot2_GetDistanceToGoal(int client);
```

**Implementation Steps**:
1. Add native declarations to `bot_sm_natives.h`
2. Implement native handlers in `bot_sm_natives.cpp`
3. Access bot state through `CBot` class methods:
   - `getSchedule()` for current task
   - `getVisibles()` for current enemy
   - `getNavigator()` for waypoint data
4. Update `rcbot2.inc` with new native declarations
5. Add safety checks for null pointers and invalid states

**Testing**:
- Create test plugin that queries bot state every second
- Verify data accuracy in TF2, DOD:S, and HL2:DM
- Test edge cases (dead bots, disconnected bots)

---

#### Phase 1.2: Bot Control and Command Natives

**New Natives to Implement**:

```cpp
// Direct Bot Control
native bool RCBot2_SetGoalWaypoint(int client, int waypointId);
native bool RCBot2_SetGoalEntity(int client, int entity);
native bool RCBot2_SetGoalPosition(int client, const float pos[3]);
native void RCBot2_ClearGoal(int client);

// Action Commands
native bool RCBot2_AttackEntity(int client, int entity);
native bool RCBot2_DefendEntity(int client, int entity);
native bool RCBot2_FollowClient(int client, int target);
native bool RCBot2_StopFollowing(int client);
native bool RCBot2_HoldPosition(int client);

// Weapon Control
native int RCBot2_GetCurrentWeapon(int client);
native bool RCBot2_SwitchWeapon(int client, int weaponSlot);
native bool RCBot2_ForceWeaponReload(int client);
native bool RCBot2_SetPrimaryAttack(int client, bool attack);
native bool RCBot2_SetSecondaryAttack(int client, bool attack);
```

**Implementation Steps**:
1. Add schedule manipulation methods to `CBot` class
2. Create helper functions to inject tasks into bot's schedule
3. Implement waypoint targeting:
   - Use `CWaypoints::getWaypoint(int id)` to validate waypoint
   - Call `bot->getNavigator()->goTo(waypoint)`
4. Implement entity targeting:
   - Validate entity with `CBotGlobals::entityIsValid()`
   - Create appropriate schedule (attack, defend, follow)
5. Weapon control through `CBotWeapons` class
6. Add command queuing system to prevent conflicts

**Testing**:
- Test goal-setting commands on various maps
- Verify pathfinding works correctly
- Test weapon switching and attack commands
- Ensure commands don't break existing AI

---

#### Phase 1.3: Advanced Profile and Personality Natives

**New Natives to Implement**:

```cpp
// Advanced Profile Control
native bool RCBot2_SetPersonality(int client, const char[] personality);
native bool RCBot2_GetPersonality(int client, char[] buffer, int maxlen);
native void RCBot2_SetReactionTime(int client, float seconds);
native float RCBot2_GetReactionTime(int client);
native void RCBot2_SetAccuracy(int client, float accuracy); // 0.0-1.0
native float RCBot2_GetAccuracy(int client);

// Behavior Flags
native void RCBot2_SetAutoKick(int client, bool enabled);
native bool RCBot2_GetAutoKick(int client);
native void RCBot2_SetCanChangeClass(int client, bool enabled);
native bool RCBot2_GetCanChangeClass(int client);
native void RCBot2_SetUseTeleporters(int client, bool enabled);
native void RCBot2_SetUseDispensers(int client, bool enabled);
```

**Implementation Steps**:
1. Extend `CBotProfile` class with new fields if needed
2. Implement personality preset system:
   - Create personality configs (aggressive, defensive, support, balanced)
   - Load from config files in `package/config/personalities/`
3. Add behavior flag bitset to `CBot` class
4. Wire up new natives to profile and bot state

**Testing**:
- Create bots with different personalities
- Verify behavior differences are noticeable
- Test flag combinations

---

#### Phase 1.4: Statistics and Performance Natives

**New Natives to Implement**:

```cpp
// Combat Statistics
native int RCBot2_GetKills(int client);
native int RCBot2_GetDeaths(int client);
native int RCBot2_GetDamageDealt(int client);
native int RCBot2_GetDamageTaken(int client);
native float RCBot2_GetKDRatio(int client);
native float RCBot2_GetAccuracyPercent(int client);

// Performance Metrics
native float RCBot2_GetObjectiveScore(int client);
native int RCBot2_GetCapturePoints(int client);
native int RCBot2_GetFlagCaptures(int client);
native float RCBot2_GetHealingDone(int client); // Medic
native int RCBot2_GetSentryKills(int client); // Engineer

// Lifetime Stats
native int RCBot2_GetTotalPlaytime(int client); // seconds
native void RCBot2_ResetStats(int client);
native void RCBot2_GetStatsSummary(int client, any[] data);
```

**Implementation Steps**:
1. Create `CBotStatistics` class to track metrics:
   ```cpp
   class CBotStatistics {
       int m_iKills;
       int m_iDeaths;
       int m_iDamageDealt;
       int m_iDamageTaken;
       int m_iShotsFired;
       int m_iShotsHit;
       // ... game-specific stats
   };
   ```
2. Hook game events to track statistics:
   - `player_hurt` for damage
   - `player_death` for kills/deaths
   - `teamplay_flag_event` for flag captures
   - `teamplay_point_captured` for CP captures
3. Add persistent storage option (SQLite/flat file)
4. Implement stat aggregation and calculation methods

**Testing**:
- Run bots through full matches
- Verify stat accuracy vs. scoreboard
- Test stat persistence across map changes
- Benchmark performance impact

---

#### Phase 1.5: Event Forwards to SourceMod

**New Forwards to Implement**:

```cpp
// Bot Lifecycle Events
forward void RCBot2_OnBotCreated(int client);
forward void RCBot2_OnBotKicked(int client);
forward void RCBot2_OnBotSpawned(int client);
forward void RCBot2_OnBotDied(int client, int attacker);

// Bot Behavior Events
forward Action RCBot2_OnBotThink(int client);
forward Action RCBot2_OnBotSelectClass(int client, int &classId);
forward Action RCBot2_OnBotSelectWeapon(int client, int &weaponSlot);
forward Action RCBot2_OnBotAttack(int client, int target);

// Bot State Changes
forward void RCBot2_OnBotGoalChanged(int client, int goalType);
forward void RCBot2_OnBotEnemyFound(int client, int enemy);
forward void RCBot2_OnBotEnemyLost(int client, int enemy);
forward void RCBot2_OnBotStuck(int client);

// Game-Specific Events (TF2)
forward void RCBot2_OnBotBuildSentry(int client, int sentryEntity);
forward void RCBot2_OnBotUseUber(int client, int patient);
forward void RCBot2_OnBotBackstab(int client, int victim);
```

**Implementation Steps**:
1. Create forward handles in `bot_sm_ext.cpp`:
   ```cpp
   IForward *g_pOnBotCreated = nullptr;
   IForward *g_pOnBotThink = nullptr;
   // ...
   ```
2. Initialize forwards in extension load:
   ```cpp
   g_pOnBotCreated = g_pForwards->CreateForward("RCBot2_OnBotCreated",
       ET_Ignore, 1, nullptr, Param_Cell);
   ```
3. Add hook points in bot code:
   - `CBots::createBot()` → fire OnBotCreated
   - `CBot::Think()` → fire OnBotThink
   - `CBot::died()` → fire OnBotDied
4. Handle `Plugin_Changed` and `Plugin_Handled` return values appropriately

**Testing**:
- Create listener plugin for all forwards
- Verify timing and parameters
- Test action forwards that modify behavior

---

#### Phase 1.6: Waypoint and Navigation Natives

**New Natives to Implement**:

```cpp
// Waypoint Information
native int RCBot2_GetWaypointCount();
native bool RCBot2_GetWaypointInfo(int waypointId, any[] info);
native int RCBot2_FindNearestWaypoint(const float pos[3], float maxDist = 400.0);
native void RCBot2_GetWaypointPosition(int waypointId, float pos[3]);
native int RCBot2_GetWaypointFlags(int waypointId);
native bool RCBot2_IsWaypointVisible(int waypointId, const float pos[3]);

// Path Operations
native int RCBot2_GetPathCost(int fromWpt, int toWpt);
native bool RCBot2_GetPath(int fromWpt, int toWpt, int[] pathArray, int maxlen);
native void RCBot2_RecalculatePath(int client);
```

**Implementation Steps**:
1. Expose `CWaypoints` class methods:
   - `numWaypoints()` for count
   - `getWaypoint(int)` for waypoint access
   - `nearestWaypointTo()` for finding waypoints
2. Create waypoint info structure:
   ```cpp
   enum WaypointInfo {
       Waypoint_Id,
       Waypoint_Flags,
       Float:Waypoint_X,
       Float:Waypoint_Y,
       Float:Waypoint_Z,
       Waypoint_Area
   };
   ```
3. Implement path-finding wrapper around `CWaypointNavigator`

**Testing**:
- Verify waypoint queries on multiple maps
- Test path calculations
- Ensure performance is acceptable (waypoint queries can be expensive)

---

### Phase 1 Integration Tasks

1. **Update `rcbot2.inc` header file**:
   - Add all new native declarations
   - Add forward declarations
   - Add new enums and constants
   - Add comprehensive documentation

2. **Create example plugins**:
   - `rcbot2_control.sp` - Bot command and control example
   - `rcbot2_stats.sp` - Statistics tracking and display
   - `rcbot2_events.sp` - Event listener example
   - `rcbot2_navigation.sp` - Custom navigation example

3. **Documentation**:
   - Update `sm_ext/README.md` with native reference
   - Create wiki pages for each native
   - Document forwards with examples
   - Create troubleshooting guide

4. **Performance Optimization**:
   - Profile native call overhead
   - Implement caching where appropriate
   - Add rate limiting for expensive operations
   - Consider async operations for heavy queries

5. **Backwards Compatibility**:
   - Ensure existing plugins continue working
   - Version the extension properly
   - Provide migration guide if needed

---

## Phase 2: SourceMod Plugin Suite

### Goal
Create a comprehensive set of SourceMod plugins that provide management interfaces, automated balancing, and statistics tracking.

---

### Plugin 2.1: RCBot2 Manager (`rcbot2_manager.sp`)

**Description**: Complete bot management interface with admin commands and menus.

**Features**:
- Admin menu integration for bot management
- Console commands for all bot operations
- Bot quota system (maintain X bots on server)
- Team balancing (keep teams even)
- Skill level management
- Bot name customization

**Commands to Implement**:
```sourcepawn
// Admin Commands
sm_rcbot_add [name] [team] [class] - Add a bot
sm_rcbot_kick <name|#userid> - Kick specific bot
sm_rcbot_kickall [team] - Kick all bots (or team-specific)
sm_rcbot_quota <number> - Set bot quota
sm_rcbot_skill <value> - Set default bot skill (0.0-1.0)
sm_rcbot_skill_min <value> - Set minimum skill
sm_rcbot_skill_max <value> - Set maximum skill

// Management Commands
sm_rcbot_menu - Open bot management menu
sm_rcbot_list - List all bots
sm_rcbot_balance - Force team balance
sm_rcbot_info <bot> - Show bot information
sm_rcbot_control <bot> - Control specific bot
```

**ConVars**:
```sourcepawn
rcbot2_quota "0" - Number of bots to maintain (0 = disabled)
rcbot2_quota_mode "fill" - Quota mode: fill, match, fixed
rcbot2_auto_balance "1" - Auto-balance teams with bots
rcbot2_skill_default "0.5" - Default bot skill
rcbot2_skill_random "0" - Randomize bot skill (0=off, 1=on)
rcbot2_skill_min "0.3" - Minimum skill if randomized
rcbot2_skill_max "0.8" - Maximum skill if randomized
rcbot2_auto_kick "1" - Kick bots when players join
rcbot2_min_bots "0" - Never kick below this many bots
```

**Menu System**:
```
[RCBot2 Management]
├── Add Bot
│   ├── Select Team
│   └── Select Class
├── Kick Bot
│   └── [List of bots]
├── Bot Quota
│   ├── Set Quota
│   └── Quota Mode
├── Bot Settings
│   ├── Skill Level
│   ├── Auto-Balance
│   └── Auto-Kick Settings
└── Statistics
    └── [Show bot performance]
```

**Implementation Details**:

1. **Quota System**:
```sourcepawn
void MaintainQuota() {
    int desired = g_cvQuota.IntValue;
    int current = RCBot2_GetBotCount();
    int humans = GetHumanCount();

    int target = 0;
    switch (g_iQuotaMode) {
        case QUOTA_FILL: // Fill to server max
            target = MaxClients - humans;
        case QUOTA_MATCH: // Match human count
            target = humans;
        case QUOTA_FIXED: // Fixed number
            target = desired;
    }

    target = min(target, desired);

    if (current < target) {
        // Add bots
        for (int i = current; i < target; i++)
            CreateRandomBot();
    } else if (current > target && g_cvAutoKick.BoolValue) {
        // Remove bots
        KickRandomBot();
    }
}
```

2. **Team Balancing**:
```sourcepawn
void BalanceTeams() {
    int redCount = GetTeamClientCount(TEAM_RED);
    int blueCount = GetTeamClientCount(TEAM_BLUE);
    int diff = abs(redCount - blueCount);

    if (diff <= 1) return;

    int toMove = diff / 2;
    int fromTeam = (redCount > blueCount) ? TEAM_RED : TEAM_BLUE;

    // Find bots to move
    for (int i = 1; i <= MaxClients && toMove > 0; i++) {
        if (IsClientInGame(i) && IsRCBot2Client(i)) {
            if (RCBot2_GetTeam(i) == fromTeam) {
                int newTeam = (fromTeam == TEAM_RED) ? TEAM_BLUE : TEAM_RED;
                ChangeClientTeam(i, newTeam);
                toMove--;
            }
        }
    }
}
```

3. **Bot Creation with Skill Randomization**:
```sourcepawn
int CreateRandomBot() {
    char name[64];
    GenerateBotName(name, sizeof(name));

    int bot = RCBot2_CreateBot(name);
    if (bot == -1) return -1;

    // Set random skill if enabled
    if (g_cvSkillRandom.BoolValue) {
        float skill = GetRandomFloat(
            g_cvSkillMin.FloatValue,
            g_cvSkillMax.FloatValue
        );
        RCBot2_SetSkill(bot, skill);
    } else {
        RCBot2_SetSkill(bot, g_cvSkillDefault.FloatValue);
    }

    return bot;
}
```

**Testing**:
- Test quota system with varying player counts
- Verify team balancing works correctly
- Test all menu options
- Ensure auto-kick doesn't remove bots during active gameplay

---

### Plugin 2.2: RCBot2 Statistics Tracker (`rcbot2_stats.sp`)

**Description**: Comprehensive statistics tracking and display system.

**Features**:
- Track per-bot statistics (kills, deaths, damage, accuracy)
- Track global statistics (total bot kills, average KD, etc.)
- MySQL/SQLite persistent storage
- Web stats integration (optional)
- Chat commands for stat display
- Top performer tracking

**Commands**:
```sourcepawn
sm_botstats [bot] - Show bot statistics
sm_botstats_top [category] - Show top performers
sm_botstats_reset <bot> - Reset bot stats
sm_botstats_resetall - Reset all bot stats
sm_botstats_global - Show global statistics
```

**ConVars**:
```sourcepawn
rcbot2_stats_enabled "1" - Enable statistics tracking
rcbot2_stats_database "rcbot2" - Database config name
rcbot2_stats_persistent "1" - Save stats to database
rcbot2_stats_announce "1" - Announce bot achievements
rcbot2_stats_announce_interval "300" - Announcement interval
```

**Database Schema**:
```sql
CREATE TABLE rcbot2_stats (
    bot_id INTEGER PRIMARY KEY AUTO_INCREMENT,
    bot_name VARCHAR(64),
    steam_id VARCHAR(32), -- For persistent tracking
    kills INTEGER DEFAULT 0,
    deaths INTEGER DEFAULT 0,
    assists INTEGER DEFAULT 0,
    damage_dealt INTEGER DEFAULT 0,
    damage_taken INTEGER DEFAULT 0,
    shots_fired INTEGER DEFAULT 0,
    shots_hit INTEGER DEFAULT 0,
    healing_done FLOAT DEFAULT 0,
    captures INTEGER DEFAULT 0,
    defenses INTEGER DEFAULT 0,
    playtime INTEGER DEFAULT 0, -- seconds
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);

CREATE TABLE rcbot2_matches (
    match_id INTEGER PRIMARY KEY AUTO_INCREMENT,
    map_name VARCHAR(64),
    started_at TIMESTAMP,
    ended_at TIMESTAMP,
    winning_team INTEGER
);

CREATE TABLE rcbot2_match_stats (
    stat_id INTEGER PRIMARY KEY AUTO_INCREMENT,
    match_id INTEGER,
    bot_id INTEGER,
    kills INTEGER,
    deaths INTEGER,
    -- ... other per-match stats
    FOREIGN KEY (match_id) REFERENCES rcbot2_matches(match_id),
    FOREIGN KEY (bot_id) REFERENCES rcbot2_stats(bot_id)
);
```

**Implementation Details**:

1. **Event Tracking**:
```sourcepawn
public void OnClientPutInServer(int client) {
    if (IsRCBot2Client(client)) {
        LoadBotStats(client);
    }
}

public void RCBot2_OnBotDied(int client, int attacker) {
    g_iBotDeaths[client]++;

    if (attacker > 0 && attacker <= MaxClients) {
        if (IsRCBot2Client(attacker))
            g_iBotKills[attacker]++;
        else
            g_iKillsByHumans[attacker]++;
    }

    SaveBotStats(client);
}

public void OnPlayerHurt(Event event, const char[] name, bool dontBroadcast) {
    int victim = GetClientOfUserId(event.GetInt("userid"));
    int attacker = GetClientOfUserId(event.GetInt("attacker"));
    int damage = event.GetInt("dmghp_taken");

    if (attacker > 0 && IsRCBot2Client(attacker)) {
        g_iBotDamageDealt[attacker] += damage;
    }

    if (victim > 0 && IsRCBot2Client(victim)) {
        g_iBotDamageTaken[victim] += damage;
    }
}
```

2. **Statistics Display**:
```sourcepawn
void DisplayBotStats(int client, int bot) {
    char name[64];
    GetClientName(bot, name, sizeof(name));

    float kd = (g_iBotDeaths[bot] > 0) ?
        float(g_iBotKills[bot]) / float(g_iBotDeaths[bot]) :
        float(g_iBotKills[bot]);

    float accuracy = (g_iBotShotsFired[bot] > 0) ?
        (float(g_iBotShotsHit[bot]) / float(g_iBotShotsFired[bot])) * 100.0 :
        0.0;

    PrintToChat(client, "\x04[RCBot Stats]\x01 %s", name);
    PrintToChat(client, "  K/D: %d/%d (%.2f)",
        g_iBotKills[bot], g_iBotDeaths[bot], kd);
    PrintToChat(client, "  Damage: %d dealt, %d taken",
        g_iBotDamageDealt[bot], g_iBotDamageTaken[bot]);
    PrintToChat(client, "  Accuracy: %.1f%%", accuracy);
    PrintToChat(client, "  Playtime: %d minutes",
        g_iBotPlaytime[bot] / 60);
}
```

3. **Leaderboards**:
```sourcepawn
void DisplayTopBots(int client, const char[] category) {
    // Sort bots by category
    ArrayList sortedBots = new ArrayList(2);

    for (int i = 1; i <= MaxClients; i++) {
        if (IsClientInGame(i) && IsRCBot2Client(i)) {
            int value = 0;
            if (StrEqual(category, "kills"))
                value = g_iBotKills[i];
            else if (StrEqual(category, "kd"))
                value = RoundToNearest(GetKDRatio(i) * 100);
            else if (StrEqual(category, "damage"))
                value = g_iBotDamageDealt[i];

            sortedBots.Set(sortedBots.Push(i), value, 1);
        }
    }

    sortedBots.Sort(Sort_Descending, Sort_Integer);

    PrintToChat(client, "\x04[Top Bots - %s]", category);
    for (int i = 0; i < min(5, sortedBots.Length); i++) {
        int bot = sortedBots.Get(i, 0);
        int value = sortedBots.Get(i, 1);
        char name[64];
        GetClientName(bot, name, sizeof(name));
        PrintToChat(client, "  %d. %s - %d", i+1, name, value);
    }

    delete sortedBots;
}
```

**Testing**:
- Run extended matches with stat tracking
- Verify database persistence
- Test stat reset functionality
- Benchmark database query performance

---

### Plugin 2.3: RCBot2 Dynamic Balancer (`rcbot2_balancer.sp`)

**Description**: Intelligent bot skill balancing based on team performance.

**Features**:
- Monitor team performance and adjust bot difficulty
- Individual bot skill adjustment based on performance
- Handicap system for dominant teams
- Comeback mechanics (boost losing team's bots)
- Match quality scoring

**ConVars**:
```sourcepawn
rcbot2_balance_enabled "1" - Enable dynamic balancing
rcbot2_balance_interval "30.0" - Balance check interval (seconds)
rcbot2_balance_threshold "0.3" - Performance difference to trigger balance
rcbot2_balance_speed "0.05" - How quickly to adjust (per interval)
rcbot2_balance_max_skill "0.9" - Maximum bot skill
rcbot2_balance_min_skill "0.2" - Minimum bot skill
rcbot2_comeback_enabled "1" - Enable comeback mechanics
rcbot2_comeback_multiplier "1.5" - Boost multiplier for losing team
```

**Implementation Details**:

1. **Performance Monitoring**:
```sourcepawn
void CalculateTeamPerformance() {
    // Calculate performance metrics for each team
    for (int team = 2; team <= 3; team++) {
        int kills = 0, deaths = 0, captures = 0;
        int botCount = 0;

        for (int i = 1; i <= MaxClients; i++) {
            if (!IsClientInGame(i)) continue;
            if (GetClientTeam(i) != team) continue;

            if (IsRCBot2Client(i)) {
                botCount++;
                kills += g_iBotKills[i];
                deaths += g_iBotDeaths[i];
                captures += g_iBotCaptures[i];
            }
        }

        if (botCount > 0) {
            g_fTeamPerformance[team] =
                (float(kills) * 1.0) +
                (float(captures) * 5.0) -
                (float(deaths) * 0.5);
            g_fTeamPerformance[team] /= float(botCount);
        }
    }
}
```

2. **Dynamic Skill Adjustment**:
```sourcepawn
void AdjustBotSkills() {
    float perfRed = g_fTeamPerformance[TEAM_RED];
    float perfBlue = g_fTeamPerformance[TEAM_BLUE];

    float diff = perfRed - perfBlue;
    float absDiff = FloatAbs(diff);

    if (absDiff < g_cvBalanceThreshold.FloatValue)
        return; // Teams are balanced

    int dominantTeam = (diff > 0) ? TEAM_RED : TEAM_BLUE;
    int losingTeam = (diff > 0) ? TEAM_BLUE : TEAM_RED;

    float adjustment = g_cvBalanceSpeed.FloatValue;

    // Reduce dominant team bot skill
    for (int i = 1; i <= MaxClients; i++) {
        if (!IsClientInGame(i) || !IsRCBot2Client(i)) continue;

        int team = GetClientTeam(i);
        if (team == dominantTeam) {
            float skill = RCBot2_GetSkill(i);
            skill = max(skill - adjustment, g_cvMinSkill.FloatValue);
            RCBot2_SetSkill(i, skill);
        } else if (team == losingTeam) {
            float skill = RCBot2_GetSkill(i);
            float boost = adjustment;
            if (g_cvComebackEnabled.BoolValue)
                boost *= g_cvComebackMultiplier.FloatValue;
            skill = min(skill + boost, g_cvMaxSkill.FloatValue);
            RCBot2_SetSkill(i, skill);
        }
    }
}
```

3. **Individual Performance Adjustment**:
```sourcepawn
void AdjustIndividualSkill(int client) {
    if (!IsRCBot2Client(client)) return;

    float kd = GetKDRatio(client);
    float currentSkill = RCBot2_GetSkill(client);
    float targetSkill = currentSkill;

    // High performers get harder
    if (kd > 2.0) {
        targetSkill += 0.05;
    } else if (kd > 1.5) {
        targetSkill += 0.02;
    }
    // Low performers get easier
    else if (kd < 0.5) {
        targetSkill -= 0.05;
    } else if (kd < 0.8) {
        targetSkill -= 0.02;
    }

    targetSkill = clamp(targetSkill,
        g_cvMinSkill.FloatValue,
        g_cvMaxSkill.FloatValue);

    RCBot2_SetSkill(client, targetSkill);
}
```

**Testing**:
- Test with intentionally imbalanced teams
- Monitor skill adjustments over multiple rounds
- Verify balancing doesn't oscillate wildly
- Test comeback mechanics

---

### Plugin 2.4: RCBot2 Class Manager (`rcbot2_classes.sp`)

**Description**: Advanced class restriction and management for TF2 and other class-based games.

**Features**:
- Class restrictions per team
- Class quotas (max X of each class)
- Required classes (ensure at least X medics)
- Smart class selection (fill needed roles)
- Dynamic class changing based on gamemode/map

**Commands**:
```sourcepawn
sm_rcbot_class_limit <class> <max> - Set class limit
sm_rcbot_class_require <class> <min> - Require minimum bots
sm_rcbot_class_ban <class> - Ban bots from class
sm_rcbot_class_reset - Reset all restrictions
```

**ConVars**:
```sourcepawn
rcbot2_class_limits_enabled "1" - Enable class limits
rcbot2_class_smart_selection "1" - Smart class distribution
rcbot2_class_enforce_needed "1" - Ensure needed classes (medic, engi)
rcbot2_class_max_scouts "3" - Max scouts
rcbot2_class_max_soldiers "3" - Max soldiers
// ... etc for each class
rcbot2_class_min_medics "1" - Minimum medics
rcbot2_class_min_engineers "1" - Minimum engineers
```

**Implementation**:
```sourcepawn
public Action RCBot2_OnBotSelectClass(int client, int &classId) {
    int team = GetClientTeam(client);

    // Count current class distribution
    int classCounts[10];
    for (int i = 1; i <= MaxClients; i++) {
        if (IsClientInGame(i) && GetClientTeam(i) == team) {
            int cls = IsRCBot2Client(i) ?
                RCBot2_GetClass(i) :
                GetEntProp(i, Prop_Send, "m_iClass");
            classCounts[cls]++;
        }
    }

    // Check if selected class is at limit
    if (IsClassAtLimit(classId, classCounts)) {
        // Select different class
        classId = SelectBestClass(team, classCounts);
        return Plugin_Changed;
    }

    return Plugin_Continue;
}

int SelectBestClass(int team, int[] counts) {
    // Priority: Fill minimum requirements first
    if (counts[TFClass_Medic] < g_cvMinMedics.IntValue)
        return TFClass_Medic;
    if (counts[TFClass_Engineer] < g_cvMinEngineers.IntValue)
        return TFClass_Engineer;

    // Then select based on smart distribution
    ArrayList available = new ArrayList();
    for (int cls = 1; cls <= 9; cls++) {
        if (!IsClassAtLimit(cls, counts) && !IsClassBanned(cls)) {
            available.Push(cls);
        }
    }

    int selected = available.Get(GetRandomInt(0, available.Length - 1));
    delete available;
    return selected;
}
```

**Testing**:
- Test class limits enforcement
- Verify smart selection provides good team comp
- Test with various map types (payload, CP, CTF, etc.)

---

### Plugin 2.5: RCBot2 Loadout Manager (`rcbot2_loadout.sp`)

**Description**: Manage bot weapon loadouts and cosmetics (TF2).

**Features**:
- Custom weapon loadouts per class
- Randomized weapon selection
- Restrict certain weapons
- Paint/cosmetic support
- Loadout profiles (aggressive, defensive, support)

**Implementation**:
```sourcepawn
void GiveBotLoadout(int client, int class, const char[] profile) {
    // Read loadout config
    char path[PLATFORM_MAX_PATH];
    BuildPath(Path_SM, path, sizeof(path),
        "configs/rcbot2/loadouts/%s_%s.cfg",
        g_sClassNames[class], profile);

    KeyValues kv = new KeyValues("Loadout");
    if (!kv.ImportFromFile(path)) {
        delete kv;
        return;
    }

    // Give weapons for each slot
    for (int slot = 0; slot < 3; slot++) {
        char key[32], weapon[64];
        Format(key, sizeof(key), "slot%d", slot);
        kv.GetString(key, weapon, sizeof(weapon));

        if (!StrEqual(weapon, "")) {
            GivePlayerItem(client, weapon);
        }
    }

    delete kv;
}
```

---

### Plugin 2.6: RCBot2 Web Stats (`rcbot2_webstats.sp`)

**Description**: Web interface integration for viewing bot statistics.

**Features**:
- JSON API endpoint for stats
- Generate HTML stat pages
- Real-time server status
- Historical match data
- Bot performance graphs

**API Endpoints**:
```
/api/bots/current - Current bot list
/api/bots/{id}/stats - Bot statistics
/api/matches/recent - Recent matches
/api/server/status - Server status
```

**Implementation**:
Uses SteamWorks extension to create HTTP server or exports JSON files to web directory.

---

## Phase 3: Testing and Deployment

### Testing Plan

1. **Unit Testing**:
   - Test each native individually
   - Verify parameter validation
   - Test error handling

2. **Integration Testing**:
   - Test natives working together
   - Test plugin interactions
   - Test with multiple game modes

3. **Performance Testing**:
   - Profile native call overhead
   - Benchmark statistics tracking
   - Load test with max bots

4. **User Acceptance Testing**:
   - Beta test with server admins
   - Gather feedback on functionality
   - Iterate on UI/UX

### Deployment Strategy

1. **Phased Rollout**:
   - Phase 1: Release extended natives
   - Phase 2: Release management plugins
   - Phase 3: Release statistics and balancing plugins

2. **Documentation**:
   - API reference documentation
   - Plugin configuration guides
   - Example plugin tutorials
   - Migration guides

3. **Support**:
   - GitHub issues for bug reports
   - Discord support channel
   - FAQ and troubleshooting guide

---

## Timeline Estimates

### Phase 1: Extended SourceMod Natives
- **Phase 1.1** (State/Info): 1-2 weeks
- **Phase 1.2** (Control): 2-3 weeks
- **Phase 1.3** (Profiles): 1 week
- **Phase 1.4** (Statistics): 2 weeks
- **Phase 1.5** (Forwards): 1-2 weeks
- **Phase 1.6** (Waypoints): 1 week
- **Integration & Testing**: 1-2 weeks

**Total Phase 1**: 9-13 weeks

### Phase 2: SourceMod Plugin Suite
- **Plugin 2.1** (Manager): 2 weeks
- **Plugin 2.2** (Stats): 2-3 weeks
- **Plugin 2.3** (Balancer): 1-2 weeks
- **Plugin 2.4** (Classes): 1 week
- **Plugin 2.5** (Loadout): 1 week
- **Plugin 2.6** (Web Stats): 2-3 weeks
- **Integration & Testing**: 2 weeks

**Total Phase 2**: 11-15 weeks

### Phase 3: Testing & Deployment
- **Testing**: 2-3 weeks
- **Documentation**: 1-2 weeks
- **Beta period**: 2-4 weeks

**Total Phase 3**: 5-9 weeks

### **Grand Total**: 25-37 weeks (6-9 months)

---

## Success Metrics

1. **Adoption Metrics**:
   - Number of servers using SourceMod integration
   - Number of custom plugins developed
   - GitHub stars/downloads

2. **Performance Metrics**:
   - Native call overhead < 0.1ms average
   - Statistics tracking overhead < 5% CPU
   - Plugin suite stable with no memory leaks

3. **User Satisfaction**:
   - Positive feedback from server admins
   - Active community contributions
   - Low bug report rate after stabilization

---

## Future Enhancements

After completing Phase 1 and 2, consider:

1. **Machine Learning Integration**:
   - Train bot behaviors from player demos
   - Adaptive difficulty based on player skill
   - Personality learning from matches

2. **Advanced Game Mode Support**:
   - Custom game mode plugins (dodgeball, prophunt, etc.)
   - Special event support (Halloween, Christmas)
   - Tournament/competitive mode

3. **Cross-Plugin Integration**:
   - Integration with SourceBans
   - Integration with HLStatsX
   - Integration with SourceComms

4. **Mobile Management**:
   - Mobile app for server administration
   - Push notifications for bot events
   - Remote bot control

---

## Dependencies

### Required Software:
- **MetaMod:Source** 1.11+
- **SourceMod** 1.11+
- **HL2SDK** (appropriate version for target game)
- **AMBuild** 2.2+

### Optional Software:
- **MySQL/MariaDB** (for persistent stats)
- **SteamWorks Extension** (for web stats)
- **cURL Extension** (for external APIs)

### Development Tools:
- **Python** 3.8+ (for build system)
- **Git** (version control)
- **GCC/Clang** or **MSVC** 2019+ (compiler)
- **spcomp** (SourcePawn compiler)

---

## Resources

### SourceMod Documentation:
- **SourceMod Scripting**: https://wiki.alliedmods.net/Category:SourceMod_Scripting
- **SourceMod API**: https://sm.alliedmods.net/new-api/
- **Extension Development**: https://wiki.alliedmods.net/Extensions_(SourceMod)
- **Natives Guide**: https://wiki.alliedmods.net/Adding_Natives_(SourceMod_Scripting)

### RCBot2 Resources:
- **Bot Architecture**: See `claude.md` in repository
- **Current Extension**: `sm_ext/` directory
- **Example Scripts**: `scripting/` directory

### Community:
- **AlliedModders Forums**: https://forums.alliedmods.net/
- **RCBot Discord**: https://discord.gg/5v5YvKG4Hr
- **GitHub Repository**: ethanbissbort/rcbot2

---

## Conclusion

This roadmap provides a comprehensive plan for enhancing RCBot2's SourceMod integration. By implementing extended natives and a robust plugin suite, we will provide server administrators with powerful tools for managing, customizing, and monitoring bots.

The phased approach ensures steady progress while maintaining code quality and backwards compatibility. Each phase builds upon the previous, creating a solid foundation for future enhancements.

Key benefits of this implementation:
- **Server admins** get easy-to-use management tools
- **Plugin developers** get comprehensive API access
- **Players** get better bot experiences through dynamic balancing
- **Community** gets active development and support

The estimated timeline of 6-9 months is ambitious but achievable with dedicated development effort. Regular testing and community feedback throughout the process will ensure the final product meets user needs and maintains high quality standards.
