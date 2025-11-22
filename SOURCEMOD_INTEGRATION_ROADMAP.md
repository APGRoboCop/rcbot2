# RCBot2 SourceMod Integration Roadmap

## Overview

This roadmap outlines the plan for enhancing RCBot2's SourceMod integration, enabling greater control and customization of bot behavior through SourcePawn plugins. The integration will maintain backward compatibility while exposing powerful new capabilities to server administrators and plugin developers.

---

## Current State

### Existing SourceMod Natives (7 total)

Located in `sm_ext/bot_sm_natives.cpp` and exposed via `scripting/include/rcbot2.inc`:

```sourcepawn
// Waypoint Management
native bool RCBot2_IsWaypointAvailable();

// Bot Creation & Identification
native int RCBot2_CreateBot(const char[] name);
native int IsRCBot2Client(int client);

// Profile Configuration
native void RCBot2_SetProfileInt(int client, RCBotProfileVar property, int value);
native int RCBot2_GetProfileInt(int client, RCBotProfileVar property);
native void RCBot2_SetProfileFloat(int client, RCBotProfileVar property, float value);
native float RCBot2_GetProfileFloat(int client, RCBotProfileVar property);
```

### Current Profile Properties

```sourcepawn
enum RCBotProfileVar {
    RCBotProfile_iVisionTicks,          // Speed of recognizing NPCs
    RCBotProfile_iPathTicks,            // Speed of pathfinding
    RCBotProfile_iVisionTicksClients,   // Speed of recognizing players
    RCBotProfile_iSensitivity,          // Aim speed (1-20)
    RCBotProfile_fBraveness,            // Danger sensitivity (0.0-1.0)
    RCBotProfile_fAimSkill,             // Prediction ability (0.0-1.0)
    RCBotProfile_iClass,                // Preferred class
};
```

---

## Architecture Overview

### Core Bot Systems

| System | Location | Purpose |
|--------|----------|---------|
| **CBot** | `utils/RCBot2_meta/bot.h` | Individual bot instance management |
| **CBots** | `utils/RCBot2_meta/bot.h` | Bot manager singleton |
| **CBotProfile** | `utils/RCBot2_meta/bot_profile.h` | Bot personality configuration |
| **CBotSchedules** | `utils/RCBot2_meta/bot_schedule.h` | Task-based AI behavior |
| **CBotWeapons** | `utils/RCBot2_meta/bot_weapons.h` | Weapon selection and usage |
| **CBotNavigator** | `utils/RCBot2_meta/bot_navigator.h` | Pathfinding via waypoints/navmesh |
| **CBotVisibles** | `utils/RCBot2_meta/bot_visibles.h` | Enemy/entity tracking |

### Game-Specific Implementations

| Game | Class | Location | Size |
|------|-------|----------|------|
| **Team Fortress 2** | `CBotTF2` | `utils/RCBot2_meta/bot_fortress.h` | ~262KB |
| **Day of Defeat: Source** | `CDODBot` | `utils/RCBot2_meta/bot_dod_bot.h` | ~106KB |
| **Half-Life 2: Deathmatch** | `CHLDMBot` | `utils/RCBot2_meta/bot_hldm_bot.h` | - |
| **Counter-Strike: Source** | `CBotCSS` | `utils/RCBot2_meta/bot_css_bot.h` | - |

---

## Implementation Phases

### Phase 1: Essential Bot Control (High Priority)

**Estimated Timeline**: 2-4 weeks
**Value**: Immediate, significant functionality gains

#### 1.1 Bot Command & Control

```sourcepawn
// Direct bot control
native bool RCBot2_SetBotObjective(int client, RCBotObjective objective, float origin[3]);
native bool RCBot2_SetBotEnemy(int client, int target);
native bool RCBot2_ClearBotEnemy(int client);
native bool RCBot2_ForceBotAction(int client, RCBotAction action);

// Bot state queries
native int RCBot2_GetBotTeam(int client);
native int RCBot2_GetBotHealth(int client);
native int RCBot2_GetBotEnemy(int client);
native bool RCBot2_GetBotOrigin(int client, float origin[3]);
native bool RCBot2_GetBotEyeAngles(int client, float angles[3]);

enum RCBotObjective {
    RCBotObjective_None,
    RCBotObjective_Attack,
    RCBotObjective_Defend,
    RCBotObjective_Goto,
    RCBotObjective_Retreat,
    RCBotObjective_Follow
};

enum RCBotAction {
    RCBotAction_Jump,
    RCBotAction_Crouch,
    RCBotAction_Scope,
    RCBotAction_Reload,
    RCBotAction_UseItem
};
```

**Implementation Files**:
- `sm_ext/bot_sm_natives.cpp` - Native implementations
- `sm_ext/bot_sm_natives.h` - Native declarations
- `utils/RCBot2_meta/bot.h` - Add public accessor methods
- `scripting/include/rcbot2.inc` - Update header

#### 1.2 Weapon & Equipment Management

```sourcepawn
// Weapon control
native bool RCBot2_SelectWeapon(int client, const char[] weapon);
native bool RCBot2_GetCurrentWeapon(int client, char[] weapon, int maxlength);
native bool RCBot2_ForceAttack(int client, bool primary = true);
native bool RCBot2_ForceReload(int client);

// Equipment queries
native int RCBot2_GetWeaponAmmo(int client, const char[] weapon);
native int RCBot2_GetBotArmor(int client);
native int RCBot2_GetBotGrenades(int client);
```

**Implementation Files**:
- `sm_ext/bot_sm_natives.cpp`
- `utils/RCBot2_meta/bot_weapons.h` - Expose weapon accessors

#### 1.3 Task System Queries

```sourcepawn
// Task information
native int RCBot2_GetCurrentTask(int client);
native bool RCBot2_ClearSchedule(int client);
native float RCBot2_GetTaskProgress(int client);
native bool RCBot2_IsTaskComplete(int client, int taskId);

enum RCBotTask {
    RCBotTask_None,
    RCBotTask_FindPath,
    RCBotTask_GetAmmo,
    RCBotTask_GetHealth,
    RCBotTask_Attack,
    RCBotTask_Defend,
    RCBotTask_Build,
    // ... etc
};
```

**Implementation Files**:
- `sm_ext/bot_sm_natives.cpp`
- `utils/RCBot2_meta/bot_task.h` - Expose task enums
- `utils/RCBot2_meta/bot_schedule.h` - Add schedule accessors

---

### Phase 2: Game-Specific Extensions (High Priority)

**Estimated Timeline**: 3-5 weeks
**Value**: Enables game-specific bot customization

#### 2.1 Team Fortress 2 Integration

```sourcepawn
// TF2 Class Management
native bool RCBot2_TF2_SetClass(int client, TFClassType class);
native TFClassType RCBot2_TF2_GetClass(int client);
native bool RCBot2_TF2_CanChangeClass(int client);

// TF2 Engineer Building
native bool RCBot2_TF2_BuildObject(int client, TFObjectType object, float origin[3]);
native bool RCBot2_TF2_UpgradeBuilding(int client, int building);
native int RCBot2_TF2_GetBuilding(int client, TFObjectType type);

// TF2 Medic Functions
native bool RCBot2_TF2_SetHealTarget(int client, int target);
native int RCBot2_TF2_GetHealTarget(int client);
native float RCBot2_TF2_GetUberCharge(int client);
native bool RCBot2_TF2_UseUberCharge(int client);

// TF2 Spy Functions
native bool RCBot2_TF2_SetDisguise(int client, TFClassType class, int team);
native bool RCBot2_TF2_Cloak(int client, bool cloak);
native bool RCBot2_TF2_IsCloaked(int client);
native bool RCBot2_TF2_IsDisguised(int client);

enum TFObjectType {
    TFObject_Sentry,
    TFObject_Dispenser,
    TFObject_Teleporter_Entrance,
    TFObject_Teleporter_Exit
};
```

**Implementation Files**:
- `sm_ext/bot_sm_natives_tf2.cpp` (new file)
- `utils/RCBot2_meta/bot_fortress.h` - Expose TF2-specific methods
- `scripting/include/rcbot2_tf2.inc` (new file)

#### 2.2 Day of Defeat: Source Integration

```sourcepawn
// DOD:S Bomb Objectives
native bool RCBot2_DOD_PlantBomb(int client, int bombTarget);
native bool RCBot2_DOD_DefuseBomb(int client, int bomb);
native bool RCBot2_DOD_IsPlantingBomb(int client);
native bool RCBot2_DOD_IsDefusingBomb(int client);

// DOD:S Capture Points
native bool RCBot2_DOD_CapturePoint(int client, int point);
native int RCBot2_DOD_GetNearestCapturePoint(int client);
```

**Implementation Files**:
- `sm_ext/bot_sm_natives_dod.cpp` (new file)
- `utils/RCBot2_meta/bot_dod_bot.h` - Expose DOD-specific methods
- `scripting/include/rcbot2_dod.inc` (new file)

#### 2.3 Half-Life 2: Deathmatch Integration

```sourcepawn
// HL2:DM Specific Functions
native bool RCBot2_HLDM_UseCharger(int client, int charger);
native bool RCBot2_HLDM_UseSprint(int client, bool enable);
native bool RCBot2_HLDM_UseGravityGun(int client, int target);
```

**Implementation Files**:
- `sm_ext/bot_sm_natives_hldm.cpp` (new file)
- `utils/RCBot2_meta/bot_hldm_bot.h` - Expose HL2DM-specific methods
- `scripting/include/rcbot2_hldm.inc` (new file)

---

### Phase 3: Navigation & Pathfinding (Medium Priority)

**Estimated Timeline**: 2-3 weeks
**Value**: Advanced positioning control

```sourcepawn
// Navigation control
native bool RCBot2_GotoOrigin(int client, float origin[3]);
native bool RCBot2_GotoWaypoint(int client, int waypointId);
native bool RCBot2_StopMovement(int client);
native bool RCBot2_LookAt(int client, float origin[3]);

// Waypoint queries
native int RCBot2_GetNearestWaypoint(int client);
native int RCBot2_GetWaypointCount();
native bool RCBot2_GetWaypointOrigin(int waypointId, float origin[3]);
native int RCBot2_GetWaypointFlags(int waypointId);
native bool RCBot2_IsWaypointVisible(int client, int waypointId);

// Path queries
native bool RCBot2_HasPath(int client);
native int RCBot2_GetPathLength(int client);
native bool RCBot2_IsStuck(int client);
```

**Implementation Files**:
- `sm_ext/bot_sm_natives.cpp`
- `utils/RCBot2_meta/bot_navigator.h`
- `utils/RCBot2_meta/bot_waypoint.h`

---

### Phase 4: Event System & Callbacks (Medium Priority)

**Estimated Timeline**: 3-4 weeks
**Value**: Dynamic behavior modification

```sourcepawn
// Event forwards
forward void RCBot2_OnBotSpawn(int client);
forward void RCBot2_OnBotDeath(int client, int attacker);
forward void RCBot2_OnBotKill(int client, int victim);
forward void RCBot2_OnBotObjectiveComplete(int client, RCBotObjective objective);
forward void RCBot2_OnBotTaskChange(int client, RCBotTask oldTask, RCBotTask newTask);
forward void RCBot2_OnBotThink(int client);
forward void RCBot2_OnBotEnemyFound(int client, int enemy);
forward void RCBot2_OnBotEnemyLost(int client, int enemy);
forward void RCBot2_OnBotDamaged(int client, int attacker, int damage);
forward void RCBot2_OnBotHearSound(int client, float origin[3], int soundEntity);

// TF2-specific forwards
forward void RCBot2_TF2_OnBuildingPlaced(int client, int building, TFObjectType type);
forward void RCBot2_TF2_OnBuildingDestroyed(int client, int building, int destroyer);
forward void RCBot2_TF2_OnUberDeployed(int client, int target);
forward void RCBot2_TF2_OnClassChanged(int client, TFClassType oldClass, TFClassType newClass);

// DOD:S-specific forwards
forward void RCBot2_DOD_OnBombPlanted(int client, int target);
forward void RCBot2_DOD_OnBombDefused(int client, int bomb);
forward void RCBot2_DOD_OnPointCaptured(int client, int point);
```

**Implementation Files**:
- `sm_ext/bot_sm_ext.cpp` - Forward management
- `sm_ext/bot_sm_forwards.h` (new file)
- `utils/RCBot2_meta/bot.cpp` - Hook into bot events
- `utils/RCBot2_meta/bot_events.cpp` - Event propagation

---

### Phase 5: Squad & Team Coordination (Medium Priority)

**Estimated Timeline**: 2-3 weeks
**Value**: Multi-bot coordination

```sourcepawn
// Squad management
native int RCBot2_CreateSquad(const char[] name);
native bool RCBot2_DestroySquad(int squadId);
native bool RCBot2_AddBotToSquad(int client, int squadId);
native bool RCBot2_RemoveBotFromSquad(int client);
native int RCBot2_GetBotSquad(int client);

// Squad queries
native int RCBot2_GetSquadLeader(int squadId);
native bool RCBot2_SetSquadLeader(int squadId, int client);
native int RCBot2_GetSquadMemberCount(int squadId);
native bool RCBot2_GetSquadMembers(int squadId, int[] members, int maxsize);

// Squad commands
native bool RCBot2_SquadAttack(int squadId, int target);
native bool RCBot2_SquadDefend(int squadId, float origin[3]);
native bool RCBot2_SquadFollow(int squadId, int leader);
native bool RCBot2_SquadGoto(int squadId, float origin[3]);
```

**Implementation Files**:
- `sm_ext/bot_sm_natives.cpp`
- `utils/RCBot2_meta/bot_squads.h` - Expose squad interface

---

### Phase 6: Advanced Bot Management (Lower Priority)

**Estimated Timeline**: 2-3 weeks
**Value**: Enhanced control and debugging

```sourcepawn
// Bot management
native bool RCBot2_KickBot(int client);
native int RCBot2_CountBots(int team = -1);
native int RCBot2_GetBotByIndex(int index);
native bool RCBot2_ReloadProfiles();
native bool RCBot2_GetBotName(int client, char[] name, int maxlength);

// Profile management
native bool RCBot2_SaveProfile(int client, const char[] filename);
native bool RCBot2_LoadProfile(int client, const char[] filename);
native bool RCBot2_ResetProfile(int client);

// Debugging
native bool RCBot2_SetDebugLevel(int client, int level);
native bool RCBot2_GetBotStats(int client, RCBotStats stats);
native bool RCBot2_DumpBotState(int client, char[] buffer, int maxlength);

struct RCBotStats {
    int kills;
    int deaths;
    int damageDealt;
    int damageTaken;
    int shotsHit;
    int shotsMissed;
    int waypointsTraversed;
    float timePlayed;
};
```

**Implementation Files**:
- `sm_ext/bot_sm_natives.cpp`
- `utils/RCBot2_meta/bot.h`

---

### Phase 7: Perception & AI Configuration (Lower Priority)

**Estimated Timeline**: 3-5 weeks
**Value**: Fine-tuned behavior control

```sourcepawn
// Perception control
native bool RCBot2_SetBotFOV(int client, float fov);
native bool RCBot2_SetBotViewDistance(int client, float distance);
native int RCBot2_GetVisibleEnemies(int client, int[] enemies, int maxsize);
native int RCBot2_GetNearbyAllies(int client, int[] allies, int maxsize, float radius);

// Condition management
native bool RCBot2_SetCondition(int client, RCBotCondition condition);
native bool RCBot2_RemoveCondition(int client, RCBotCondition condition);
native bool RCBot2_HasCondition(int client, RCBotCondition condition);
native int RCBot2_GetConditions(int client, RCBotCondition[] conditions, int maxsize);

enum RCBotCondition {
    RCBotCond_SeeEnemy,
    RCBotCond_Hurt,
    RCBotCond_NeedHealth,
    RCBotCond_NeedAmmo,
    RCBotCond_TaskComplete,
    RCBotCond_Stuck,
    // ... many more from bot.h
};

// AI configuration
native bool RCBot2_SetUtilityWeight(int client, RCBotUtility utility, float weight);
native float RCBot2_GetUtilityWeight(int client, RCBotUtility utility);
native bool RCBot2_SetWeaponPreference(int client, const char[] weapon, float preference);
```

**Implementation Files**:
- `sm_ext/bot_sm_natives.cpp`
- `utils/RCBot2_meta/bot.h` - Expose condition flags
- `utils/RCBot2_meta/bot_utility.h` - Expose utility system

---

## Technical Implementation Guidelines

### Native Function Standards

All natives should follow these conventions:

1. **Parameter Validation**
```cpp
cell_t sm_RCBotExampleNative(IPluginContext *pContext, const cell_t *params) {
    const int client = params[1];

    // Validate client index
    if (client < 1 || client > gpGlobals->maxClients) {
        return pContext->ThrowNativeError("Invalid client index %d", client);
    }

    // Validate RCBot instance
    const CBot* bot = CBots::getBot(client - 1);
    if (!bot) {
        return pContext->ThrowNativeError("Client index %d is not a RCBot", client);
    }

    // Implementation...
    return 0;
}
```

2. **Error Handling**
- Use `pContext->ThrowNativeError()` for invalid inputs
- Return meaningful error codes (-1 for failure, 0+ for success)
- Document error conditions in SourcePawn header

3. **Memory Safety**
- Validate array sizes before writing
- Use `pContext->LocalToString()` for string parameters
- Check pointer validity before dereferencing

4. **Game Mod Detection**
```cpp
// Only expose TF2 natives when running TF2
#ifdef TF2
    sharesys->AddNatives(myself, g_RCBotNatives_TF2);
#endif
```

### File Organization

```
sm_ext/
├── bot_sm_ext.cpp              # Main extension initialization
├── bot_sm_ext.h                # Extension header
├── bot_sm_natives.cpp          # Core/generic natives
├── bot_sm_natives.h            # Native declarations
├── bot_sm_natives_tf2.cpp      # TF2-specific natives (new)
├── bot_sm_natives_dod.cpp      # DOD:S-specific natives (new)
├── bot_sm_natives_hldm.cpp     # HL2DM-specific natives (new)
└── bot_sm_forwards.h           # Forward declarations (new)

scripting/include/
├── rcbot2.inc                  # Core natives
├── rcbot2_tf2.inc              # TF2 natives (new)
├── rcbot2_dod.inc              # DOD:S natives (new)
└── rcbot2_hldm.inc             # HL2DM natives (new)
```

### Testing Strategy

1. **Unit Tests**: Validate each native with edge cases
2. **Integration Tests**: Test native interactions
3. **Game Tests**: Test in actual game environments
4. **Performance Tests**: Ensure minimal overhead
5. **Compatibility Tests**: Verify backward compatibility

---

## Backward Compatibility

All existing functionality must remain unchanged:
- Existing 7 natives retain current behavior
- Existing RCBotProfileVar enum values preserved
- No breaking changes to public API
- Extension name remains "RCBot2"

---

## Dependencies

- SourceMod 1.11+ (for latest extension API)
- MetaMod:Source 1.11+
- Game-specific SDKs (HL2SDK-TF2, HL2SDK-DODS, etc.)
- AMBuild build system

---

## Documentation Requirements

Each phase must include:
1. Updated `scripting/include/rcbot2*.inc` headers with documentation
2. Sample plugins demonstrating new natives
3. Wiki updates with usage examples
4. Migration guides for plugin developers

---

## Success Metrics

- All phases completed with full test coverage
- At least 3 community plugins utilizing new natives
- No regression in existing bot behavior
- Performance impact <5% in typical scenarios
- Positive community feedback

---

## Future Considerations

### Post-Roadmap Enhancements

1. **Machine Learning Integration**
   - Expose genetic algorithm training interface
   - Allow SourcePawn to influence learning weights
   - Runtime model loading/switching

2. **Advanced Scripting**
   - Lua integration for complex bot behaviors
   - Python bindings for offline analysis
   - Custom task definition system

3. **Multiplayer Coordination**
   - Cross-server bot coordination
   - Centralized bot management
   - Cloud-based profile storage

4. **Performance Optimization**
   - Multi-threaded pathfinding
   - Cached perception queries
   - GPU-accelerated decision making

---

## Contributors & Maintainers

**Primary Maintainers**:
- nosoop (SourceMod integration architect)
- APGRoboCop[CL] (Linux and general development)
- caxanga334 (SourceMod, AMBuild support)

**Community Contributors Welcome**:
- See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines
- Join [Bots United Discord](https://discord.gg/5v5YvKG4Hr)
- File issues on [GitHub](https://github.com/APGRoboCop/rcbot2)

---

## License

All SourceMod integration code follows the same licensing as RCBot2:
- Core: [GNU Affero General Public License v3.0](https://spdx.org/licenses/AGPL-3.0-only.html)
- Logging module: [BSD Zero Clause License](https://spdx.org/licenses/0BSD.html)

---

**Document Version**: 1.0
**Last Updated**: 2025-11-22
**Status**: Planning Phase
