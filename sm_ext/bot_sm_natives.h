#pragma once

#include <IExtensionSys.h>

using namespace SourceMod;
using namespace SourcePawn;

cell_t sm_RCBotIsWaypointAvailable(IPluginContext *pContext, const cell_t *params);

cell_t sm_RCBotCreate(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotIsClientBot(IPluginContext *pContext, const cell_t *params);

cell_t sm_RCBotSetProfileInt(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotGetProfileInt(IPluginContext *pContext, const cell_t *params);

cell_t sm_RCBotSetProfileFloat(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotGetProfileFloat(IPluginContext *pContext, const cell_t *params);

// Phase 1: Bot Command & Control
cell_t sm_RCBotSetBotEnemy(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotGetBotEnemy(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotForceBotAction(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotGetBotTeam(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotGetBotHealth(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotGetBotOrigin(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotGetBotEyeAngles(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotGotoOrigin(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotStopMovement(IPluginContext *pContext, const cell_t *params);

// Phase 1: Weapon & Equipment Management
cell_t sm_RCBotSelectWeapon(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotGetCurrentWeapon(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotForceAttack(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotForceReload(IPluginContext *pContext, const cell_t *params);

// Phase 1: Task System Queries
cell_t sm_RCBotGetCurrentSchedule(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotClearSchedule(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotHasSchedule(IPluginContext *pContext, const cell_t *params);

// Phase 3: Navigation & Pathfinding
cell_t sm_RCBotGetWaypointCount(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotGetNearestWaypoint(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotGetWaypointOrigin(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotGetWaypointFlags(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotHasPath(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotGetGoalOrigin(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotGetCurrentWaypointID(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotGetGoalWaypointID(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotClearPath(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotIsStuck(IPluginContext *pContext, const cell_t *params);

// Phase 5: Squad & Team Coordination
cell_t sm_RCBotCreateSquad(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotDestroySquad(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotAddBotToSquad(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotRemoveBotFromSquad(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotGetBotSquadLeader(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotGetSquadMemberCount(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotGetSquadMembers(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotIsInSquad(IPluginContext *pContext, const cell_t *params);

const sp_nativeinfo_t g_RCBotNatives[] = {
	{ "RCBot2_IsWaypointAvailable", sm_RCBotIsWaypointAvailable },

	{ "RCBot2_CreateBot", sm_RCBotCreate },
	{ "IsRCBot2Client", sm_RCBotIsClientBot },

	{ "RCBot2_SetProfileInt", sm_RCBotSetProfileInt },
	{ "RCBot2_GetProfileInt", sm_RCBotGetProfileInt },

	{ "RCBot2_SetProfileFloat", sm_RCBotSetProfileFloat },
	{ "RCBot2_GetProfileFloat", sm_RCBotGetProfileFloat },

	// Phase 1: Bot Command & Control
	{ "RCBot2_SetBotEnemy", sm_RCBotSetBotEnemy },
	{ "RCBot2_GetBotEnemy", sm_RCBotGetBotEnemy },
	{ "RCBot2_ForceBotAction", sm_RCBotForceBotAction },
	{ "RCBot2_GetBotTeam", sm_RCBotGetBotTeam },
	{ "RCBot2_GetBotHealth", sm_RCBotGetBotHealth },
	{ "RCBot2_GetBotOrigin", sm_RCBotGetBotOrigin },
	{ "RCBot2_GetBotEyeAngles", sm_RCBotGetBotEyeAngles },
	{ "RCBot2_GotoOrigin", sm_RCBotGotoOrigin },
	{ "RCBot2_StopMovement", sm_RCBotStopMovement },

	// Phase 1: Weapon & Equipment Management
	{ "RCBot2_SelectWeapon", sm_RCBotSelectWeapon },
	{ "RCBot2_GetCurrentWeapon", sm_RCBotGetCurrentWeapon },
	{ "RCBot2_ForceAttack", sm_RCBotForceAttack },
	{ "RCBot2_ForceReload", sm_RCBotForceReload },

	// Phase 1: Task System Queries
	{ "RCBot2_GetCurrentSchedule", sm_RCBotGetCurrentSchedule },
	{ "RCBot2_ClearSchedule", sm_RCBotClearSchedule },
	{ "RCBot2_HasSchedule", sm_RCBotHasSchedule },

	// Phase 3: Navigation & Pathfinding
	{ "RCBot2_GetWaypointCount", sm_RCBotGetWaypointCount },
	{ "RCBot2_GetNearestWaypoint", sm_RCBotGetNearestWaypoint },
	{ "RCBot2_GetWaypointOrigin", sm_RCBotGetWaypointOrigin },
	{ "RCBot2_GetWaypointFlags", sm_RCBotGetWaypointFlags },
	{ "RCBot2_HasPath", sm_RCBotHasPath },
	{ "RCBot2_GetGoalOrigin", sm_RCBotGetGoalOrigin },
	{ "RCBot2_GetCurrentWaypointID", sm_RCBotGetCurrentWaypointID },
	{ "RCBot2_GetGoalWaypointID", sm_RCBotGetGoalWaypointID },
	{ "RCBot2_ClearPath", sm_RCBotClearPath },
	{ "RCBot2_IsStuck", sm_RCBotIsStuck },

	// Phase 5: Squad & Team Coordination
	{ "RCBot2_CreateSquad", sm_RCBotCreateSquad },
	{ "RCBot2_DestroySquad", sm_RCBotDestroySquad },
	{ "RCBot2_AddBotToSquad", sm_RCBotAddBotToSquad },
	{ "RCBot2_RemoveBotFromSquad", sm_RCBotRemoveBotFromSquad },
	{ "RCBot2_GetBotSquadLeader", sm_RCBotGetBotSquadLeader },
	{ "RCBot2_GetSquadMemberCount", sm_RCBotGetSquadMemberCount },
	{ "RCBot2_GetSquadMembers", sm_RCBotGetSquadMembers },
	{ "RCBot2_IsInSquad", sm_RCBotIsInSquad },

	{nullptr, nullptr},
};
