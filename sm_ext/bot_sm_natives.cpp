// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include "bot_sm_natives.h"
#include "bot_sm_ext.h"

#include "bot.h"
#include "bot_profile.h"
#include "bot_waypoint.h"
#include "bot_waypoint_locations.h"
#include "bot_navigator.h"
#include "bot_squads.h"
#include "bot_globals.h"

enum RCBotProfileVar : std::uint8_t {
	RCBotProfile_iVisionTicks,
	RCBotProfile_iPathTicks,
	RCBotProfile_iVisionTicksClients,
	RCBotProfile_iSensitivity,
	RCBotProfile_fBraveness,
	RCBotProfile_fAimSkill,
	RCBotProfile_iClass,
};

int* GetIntProperty(CBotProfile* profile, RCBotProfileVar profileVar);
float* GetFloatProperty(CBotProfile* profile, RCBotProfileVar profileVar);

cell_t sm_RCBotIsWaypointAvailable(IPluginContext *pContext, const cell_t *params) {
	return CWaypoints::numWaypoints() > 0;
}

cell_t sm_RCBotCreate(IPluginContext *pContext, const cell_t *params) {
	char *name;
	pContext->LocalToString(params[1], &name);

	const int slot = CBots::createDefaultBot(name);
	
	// player slots are off-by-one (though this calculation is performed in the function)
	return (slot != -1)? slot + 1 : -1;
}

/* native void RCBot2_SetProfileInt(int client, RCBotProfileVar property, int value); */
cell_t sm_RCBotSetProfileInt(IPluginContext* pContext, const cell_t* params) {
	const int client = params[1];
	const RCBotProfileVar profileVar = static_cast<RCBotProfileVar>(params[2]);
	if (client < 1 || client > gpGlobals->maxClients) {
		pContext->ThrowNativeError("Invalid client index %d", client);
		return 0; // Ensure the function exits
	}

	const CBot* bot = CBots::getBot(client - 1);
	if (!bot) {
		pContext->ThrowNativeError("Client index %d is not a RCBot", client);
		return 0; // Ensure the function exits
	}

	CBotProfile* profile = bot->getProfile();
	int* value = GetIntProperty(profile, profileVar);
	if (!value) {
		pContext->ThrowNativeError("RCBot property %d is not an integer property", profileVar);
		return 0;
	}
	*value = params[3];

	return 0;
}

/* native void RCBot2_SetProfileFloat(int client, RCBotProfileVar property, float value); */
cell_t sm_RCBotSetProfileFloat(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];
	const RCBotProfileVar profileVar = static_cast<RCBotProfileVar>(params[2]);
	if (client < 1 || client > gpGlobals->maxClients) {
		pContext->ThrowNativeError("Invalid client index %d", client);
	}

	const CBot* bot = CBots::getBot(client - 1);
	if (!bot) {
		return pContext->ThrowNativeError("Client index %d is not a RCBot", client);
	}

	CBotProfile* profile = bot->getProfile();
	float* value = GetFloatProperty(profile, profileVar);
	if (!value) {
		pContext->ThrowNativeError("RCBot property %d is not a float property", profileVar);
		return 0;
	}
	*value = sp_ctof(params[3]);
	
	return 0;
}

/* native int RCBot2_GetProfileInt(int client, RCBotProfileVar property); */
cell_t sm_RCBotGetProfileInt(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];
	const RCBotProfileVar profileVar = static_cast<RCBotProfileVar>(params[2]);
	if (client < 1 || client > gpGlobals->maxClients) {
		pContext->ThrowNativeError("Invalid client index %d", client);
		return 0;
	}

	const CBot* bot = CBots::getBot(client - 1);
	if (!bot) {
		pContext->ThrowNativeError("Client index %d is not a RCBot", client);
		return 0;
	}
	
	CBotProfile* profile = bot->getProfile();
	const int* value = GetIntProperty(profile, profileVar);
	if (!value) {
		pContext->ThrowNativeError("RCBot property %d is not an integer property", profileVar);
		return 0;
	}
	return *value;
}

/* native float RCBot2_GetProfileFloat(int client, RCBotProfileVar property); */
cell_t sm_RCBotGetProfileFloat(IPluginContext* pContext, const cell_t* params) {
	const int client = params[1];
	const RCBotProfileVar profileVar = static_cast<RCBotProfileVar>(params[2]);
	if (client < 1 || client > gpGlobals->maxClients) {
		pContext->ThrowNativeError("Invalid client index %d", client);
		return 0;
	}

	const CBot* bot = CBots::getBot(client - 1);
	if (!bot) {
		pContext->ThrowNativeError("Client index %d is not a RCBot", client);
		return 0;
	}

	CBotProfile* profile = bot->getProfile();
	const float* value = GetFloatProperty(profile, profileVar);
	if (!value) {
		pContext->ThrowNativeError("RCBot property %d is not a float property", profileVar);
		return 0;
	}
	return sp_ftoc(*value);
}

cell_t sm_RCBotIsClientBot(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];
	if (client < 1 || client > gpGlobals->maxClients) {
		pContext->ThrowNativeError("Invalid client index %d", client);
		return 0;
	}
	return CBots::getBot(client - 1) != nullptr;
}

int* GetIntProperty(CBotProfile* profile, const RCBotProfileVar profileVar) {
	switch (profileVar) {
		case RCBotProfile_iVisionTicks:
			return &profile->m_iVisionTicks;
		case RCBotProfile_iPathTicks:
			return &profile->m_iPathTicks;
		case RCBotProfile_iClass:
			return &profile->m_iClass;
		case RCBotProfile_iVisionTicksClients:
			return &profile->m_iVisionTicksClients;
		case RCBotProfile_iSensitivity:
			return &profile->m_iSensitivity;
	}
	return nullptr;
}

float* GetFloatProperty(CBotProfile* profile, const RCBotProfileVar profileVar) {
	switch (profileVar) {
		case RCBotProfile_fBraveness:
			return &profile->m_fBraveness;
		case RCBotProfile_fAimSkill:
			return &profile->m_fAimSkill;
	}
	return nullptr;
}

//=============================================================================
// Phase 1: Bot Command & Control
//=============================================================================

/* native bool RCBot2_SetBotEnemy(int client, int target); */
cell_t sm_RCBotSetBotEnemy(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];
	const int target = params[2];

	// Validate client index
	if (client < 1 || client > gpGlobals->maxClients) {
		return pContext->ThrowNativeError("Invalid client index %d", client);
	}

	// Get bot instance
	CBot* bot = CBots::getBot(client - 1);
	if (!bot) {
		return pContext->ThrowNativeError("Client index %d is not a RCBot", client);
	}

	// Clear enemy if target is -1
	if (target == -1) {
		bot->setEnemy(nullptr);
		return 1;
	}

	// Validate target index
	if (target < 1 || target > gpGlobals->maxClients) {
		return pContext->ThrowNativeError("Invalid target index %d", target);
	}

	// Get target edict
	edict_t *pTarget = engine->PEntityOfEntIndex(target);
	if (!pTarget || pTarget->IsFree()) {
		return 0;
	}

	// Set enemy
	bot->setEnemy(pTarget);
	return 1;
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

/* native bool RCBot2_ForceBotAction(int client, RCBotAction action); */
cell_t sm_RCBotForceBotAction(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];
	const int action = params[2];

	if (client < 1 || client > gpGlobals->maxClients) {
		return pContext->ThrowNativeError("Invalid client index %d", client);
	}

	CBot* bot = CBots::getBot(client - 1);
	if (!bot) {
		return pContext->ThrowNativeError("Client index %d is not a RCBot", client);
	}

	// Force specific actions via bot buttons
	switch (action) {
		case 0: // Jump
			bot->jump();
			return 1;
		case 1: // Crouch
			bot->duck(true);
			return 1;
		case 2: // Attack1
			bot->primaryAttack();
			return 1;
		case 3: // Attack2
			bot->secondaryAttack();
			return 1;
		case 4: // Reload
			bot->reload();
			return 1;
		case 5: // Use
			bot->use();
			return 1;
		default:
			return 0;
	}
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

	return bot->getHealth();
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

/* native bool RCBot2_GetBotEyeAngles(int client, float angles[3]); */
cell_t sm_RCBotGetBotEyeAngles(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	if (client < 1 || client > gpGlobals->maxClients) {
		return pContext->ThrowNativeError("Invalid client index %d", client);
	}

	const CBot* bot = CBots::getBot(client - 1);
	if (!bot) {
		return pContext->ThrowNativeError("Client index %d is not a RCBot", client);
	}

	QAngle vAngles = bot->eyeAngles();

	cell_t *addr;
	pContext->LocalToPhysAddr(params[2], &addr);
	addr[0] = sp_ftoc(vAngles.x);
	addr[1] = sp_ftoc(vAngles.y);
	addr[2] = sp_ftoc(vAngles.z);

	return 1;
}

/* native bool RCBot2_GotoOrigin(int client, const float origin[3]); */
cell_t sm_RCBotGotoOrigin(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	if (client < 1 || client > gpGlobals->maxClients) {
		return pContext->ThrowNativeError("Invalid client index %d", client);
	}

	CBot* bot = CBots::getBot(client - 1);
	if (!bot) {
		return pContext->ThrowNativeError("Client index %d is not a RCBot", client);
	}

	// Get origin parameter
	cell_t *addr;
	pContext->LocalToPhysAddr(params[2], &addr);
	Vector vOrigin(sp_ctof(addr[0]), sp_ctof(addr[1]), sp_ctof(addr[2]));

	// Use bot's navigator to go to position
	bot->getNavigator()->gotoOrigin(vOrigin);

	return 1;
}

/* native bool RCBot2_StopMovement(int client); */
cell_t sm_RCBotStopMovement(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	if (client < 1 || client > gpGlobals->maxClients) {
		return pContext->ThrowNativeError("Invalid client index %d", client);
	}

	CBot* bot = CBots::getBot(client - 1);
	if (!bot) {
		return pContext->ThrowNativeError("Client index %d is not a RCBot", client);
	}

	// Clear navigator goals
	bot->getNavigator()->clear();
	bot->stopMoving();

	return 1;
}

//=============================================================================
// Phase 1: Weapon & Equipment Management
//=============================================================================

/* native bool RCBot2_SelectWeapon(int client, const char[] weapon); */
cell_t sm_RCBotSelectWeapon(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	if (client < 1 || client > gpGlobals->maxClients) {
		return pContext->ThrowNativeError("Invalid client index %d", client);
	}

	CBot* bot = CBots::getBot(client - 1);
	if (!bot) {
		return pContext->ThrowNativeError("Client index %d is not a RCBot", client);
	}

	// Get weapon name parameter
	char *weaponName;
	pContext->LocalToString(params[2], &weaponName);

	// Find weapon by name and select it
	CBotWeapon *pWeapon = bot->getWeapons()->getWeapon(CWeapons::getWeapon(weaponName));
	if (pWeapon) {
		bot->selectBotWeapon(pWeapon);
		return 1;
	}

	return 0;
}

/* native bool RCBot2_GetCurrentWeapon(int client, char[] weapon, int maxlength); */
cell_t sm_RCBotGetCurrentWeapon(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];
	const int maxlength = params[3];

	if (client < 1 || client > gpGlobals->maxClients) {
		return pContext->ThrowNativeError("Invalid client index %d", client);
	}

	const CBot* bot = CBots::getBot(client - 1);
	if (!bot) {
		return pContext->ThrowNativeError("Client index %d is not a RCBot", client);
	}

	CBotWeapon *pWeapon = bot->getCurrentWeapon();
	if (!pWeapon) {
		return 0;
	}

	const char *weaponName = pWeapon->getWeaponName();
	if (!weaponName) {
		return 0;
	}

	pContext->StringToLocal(params[2], maxlength, weaponName);
	return 1;
}

/* native bool RCBot2_ForceAttack(int client, bool primary, float duration); */
cell_t sm_RCBotForceAttack(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];
	const bool primary = params[2] != 0;
	const float duration = sp_ctof(params[3]);

	if (client < 1 || client > gpGlobals->maxClients) {
		return pContext->ThrowNativeError("Invalid client index %d", client);
	}

	CBot* bot = CBots::getBot(client - 1);
	if (!bot) {
		return pContext->ThrowNativeError("Client index %d is not a RCBot", client);
	}

	// Set attack button and hold time
	if (primary) {
		bot->primaryAttack(true, duration);
	} else {
		bot->secondaryAttack(true, duration);
	}

	return 1;
}

/* native bool RCBot2_ForceReload(int client); */
cell_t sm_RCBotForceReload(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	if (client < 1 || client > gpGlobals->maxClients) {
		return pContext->ThrowNativeError("Invalid client index %d", client);
	}

	CBot* bot = CBots::getBot(client - 1);
	if (!bot) {
		return pContext->ThrowNativeError("Client index %d is not a RCBot", client);
	}

	bot->reload();
	return 1;
}

//=============================================================================
// Phase 1: Task System Queries
//=============================================================================

/* native RCBotSchedule RCBot2_GetCurrentSchedule(int client); */
cell_t sm_RCBotGetCurrentSchedule(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	if (client < 1 || client > gpGlobals->maxClients) {
		return pContext->ThrowNativeError("Invalid client index %d", client);
	}

	const CBot* bot = CBots::getBot(client - 1);
	if (!bot) {
		return pContext->ThrowNativeError("Client index %d is not a RCBot", client);
	}

	CBotSchedules *pSchedules = bot->getSchedule();
	if (!pSchedules) {
		return 0; // RCBotSchedule_None
	}

	// Get current schedule from front of queue
	if (pSchedules->isEmpty()) {
		return 0;
	}

	return static_cast<int>(pSchedules->getCurrentSchedule()->getID());
}

/* native bool RCBot2_ClearSchedule(int client); */
cell_t sm_RCBotClearSchedule(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	if (client < 1 || client > gpGlobals->maxClients) {
		return pContext->ThrowNativeError("Invalid client index %d", client);
	}

	CBot* bot = CBots::getBot(client - 1);
	if (!bot) {
		return pContext->ThrowNativeError("Client index %d is not a RCBot", client);
	}

	CBotSchedules *pSchedules = bot->getSchedule();
	if (pSchedules) {
		pSchedules->freeMemory();
		return 1;
	}

	return 0;
}

/* native bool RCBot2_HasSchedule(int client, RCBotSchedule schedule); */
cell_t sm_RCBotHasSchedule(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];
	const int schedule = params[2];

	if (client < 1 || client > gpGlobals->maxClients) {
		return pContext->ThrowNativeError("Invalid client index %d", client);
	}

	const CBot* bot = CBots::getBot(client - 1);
	if (!bot) {
		return pContext->ThrowNativeError("Client index %d is not a RCBot", client);
	}

	CBotSchedules *pSchedules = bot->getSchedule();
	if (!pSchedules) {
		return 0;
	}

	return pSchedules->hasSchedule(static_cast<eBotSchedule>(schedule));
}

//=============================================================================
// Phase 3: Navigation & Pathfinding Natives
//=============================================================================

/* native int RCBot2_GetWaypointCount(); */
cell_t sm_RCBotGetWaypointCount(IPluginContext *pContext, const cell_t *params) {
	return CWaypoints::numWaypoints();
}

/* native int RCBot2_GetNearestWaypoint(const float origin[3], float maxdist = 0.0); */
cell_t sm_RCBotGetNearestWaypoint(IPluginContext *pContext, const cell_t *params) {
	cell_t *addr;
	pContext->LocalToPhysAddr(params[1], &addr);

	const Vector vOrigin(sp_ctof(addr[0]), sp_ctof(addr[1]), sp_ctof(addr[2]));
	const float fMaxDist = sp_ctof(params[2]);

	// Use CWaypointLocations::NearestWaypoint for efficient nearest waypoint search
	// Parameters: origin, maxdist, ignore_wpt, getVisible, getUnreachable, isBot, failedWpts, nearestAimingOnly, team, checkArea, getVisibleFromOther, vOther, flagsOnly
	const int iWaypoint = CWaypointLocations::NearestWaypoint(vOrigin, fMaxDist, -1, true, false, false, nullptr, false, 0, false, false, Vector(0,0,0), 0);

	return iWaypoint;
}

/* native bool RCBot2_GetWaypointOrigin(int waypointid, float origin[3]); */
cell_t sm_RCBotGetWaypointOrigin(IPluginContext *pContext, const cell_t *params) {
	const int waypointId = params[1];

	if (!CWaypoints::validWaypointIndex(waypointId)) {
		return 0;
	}

	CWaypoint* pWaypoint = CWaypoints::getWaypoint(waypointId);
	if (!pWaypoint) {
		return 0;
	}

	const Vector vOrigin = pWaypoint->getOrigin();

	cell_t *addr;
	pContext->LocalToPhysAddr(params[2], &addr);
	addr[0] = sp_ftoc(vOrigin.x);
	addr[1] = sp_ftoc(vOrigin.y);
	addr[2] = sp_ftoc(vOrigin.z);

	return 1;
}

/* native int RCBot2_GetWaypointFlags(int waypointid); */
cell_t sm_RCBotGetWaypointFlags(IPluginContext *pContext, const cell_t *params) {
	const int waypointId = params[1];

	if (!CWaypoints::validWaypointIndex(waypointId)) {
		return 0;
	}

	CWaypoint* pWaypoint = CWaypoints::getWaypoint(waypointId);
	if (!pWaypoint) {
		return 0;
	}

	return pWaypoint->getFlags();
}

/* native bool RCBot2_HasPath(int client); */
cell_t sm_RCBotHasPath(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	if (client < 1 || client > gpGlobals->maxClients) {
		return pContext->ThrowNativeError("Invalid client index %d", client);
	}

	const CBot* bot = CBots::getBot(client - 1);
	if (!bot) {
		return pContext->ThrowNativeError("Client index %d is not a RCBot", client);
	}

	IBotNavigator* pNav = bot->getNavigator();
	if (!pNav) {
		return 0;
	}

	return pNav->hasNextPoint();
}

/* native bool RCBot2_GetGoalOrigin(int client, float origin[3]); */
cell_t sm_RCBotGetGoalOrigin(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	if (client < 1 || client > gpGlobals->maxClients) {
		return pContext->ThrowNativeError("Invalid client index %d", client);
	}

	const CBot* bot = CBots::getBot(client - 1);
	if (!bot) {
		return pContext->ThrowNativeError("Client index %d is not a RCBot", client);
	}

	IBotNavigator* pNav = bot->getNavigator();
	if (!pNav) {
		return 0;
	}

	const Vector vGoal = pNav->getGoalOrigin();

	// Check if goal is valid (not zero vector)
	if (vGoal.x == 0.0f && vGoal.y == 0.0f && vGoal.z == 0.0f) {
		return 0;
	}

	cell_t *addr;
	pContext->LocalToPhysAddr(params[2], &addr);
	addr[0] = sp_ftoc(vGoal.x);
	addr[1] = sp_ftoc(vGoal.y);
	addr[2] = sp_ftoc(vGoal.z);

	return 1;
}

/* native int RCBot2_GetCurrentWaypointID(int client); */
cell_t sm_RCBotGetCurrentWaypointID(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	if (client < 1 || client > gpGlobals->maxClients) {
		return pContext->ThrowNativeError("Invalid client index %d", client);
	}

	const CBot* bot = CBots::getBot(client - 1);
	if (!bot) {
		return pContext->ThrowNativeError("Client index %d is not a RCBot", client);
	}

	IBotNavigator* pNav = bot->getNavigator();
	if (!pNav) {
		return -1;
	}

	return pNav->getCurrentWaypointID();
}

/* native int RCBot2_GetGoalWaypointID(int client); */
cell_t sm_RCBotGetGoalWaypointID(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	if (client < 1 || client > gpGlobals->maxClients) {
		return pContext->ThrowNativeError("Invalid client index %d", client);
	}

	const CBot* bot = CBots::getBot(client - 1);
	if (!bot) {
		return pContext->ThrowNativeError("Client index %d is not a RCBot", client);
	}

	IBotNavigator* pNav = bot->getNavigator();
	if (!pNav) {
		return -1;
	}

	return pNav->getCurrentGoalID();
}

/* native bool RCBot2_ClearPath(int client); */
cell_t sm_RCBotClearPath(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	if (client < 1 || client > gpGlobals->maxClients) {
		return pContext->ThrowNativeError("Invalid client index %d", client);
	}

	CBot* bot = CBots::getBot(client - 1);
	if (!bot) {
		return pContext->ThrowNativeError("Client index %d is not a RCBot", client);
	}

	IBotNavigator* pNav = bot->getNavigator();
	if (!pNav) {
		return 0;
	}

	pNav->clear();
	return 1;
}

/* native bool RCBot2_IsStuck(int client); */
cell_t sm_RCBotIsStuck(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	if (client < 1 || client > gpGlobals->maxClients) {
		return pContext->ThrowNativeError("Invalid client index %d", client);
	}

	CBot* bot = CBots::getBot(client - 1);
	if (!bot) {
		return pContext->ThrowNativeError("Client index %d is not a RCBot", client);
	}

	return bot->checkStuck();
}

//=============================================================================
// Phase 5: Squad & Team Coordination Natives
//=============================================================================

/* native int RCBot2_CreateSquad(int leader); */
cell_t sm_RCBotCreateSquad(IPluginContext *pContext, const cell_t *params) {
	const int leader = params[1];

	if (leader < 1 || leader > gpGlobals->maxClients) {
		return pContext->ThrowNativeError("Invalid leader index %d", leader);
	}

	CBot* pLeaderBot = CBots::getBot(leader - 1);
	if (!pLeaderBot) {
		return pContext->ThrowNativeError("Leader index %d is not a RCBot", leader);
	}

	edict_t* pLeaderEdict = pLeaderBot->getEdict();
	if (!pLeaderEdict || pLeaderEdict->IsFree()) {
		return 0;
	}

	// Check if already a leader
	CBotSquad* existingSquad = CBotSquads::FindSquadByLeader(pLeaderEdict);
	if (existingSquad) {
		return leader;
	}

	// Create new squad with leader as first member
	CBotSquad* pSquad = CBotSquads::AddSquadMember(pLeaderEdict, pLeaderEdict);
	if (pSquad) {
		pLeaderBot->setSquad(pSquad);
		return leader;
	}

	return -1;
}

/* native bool RCBot2_DestroySquad(int leader); */
cell_t sm_RCBotDestroySquad(IPluginContext *pContext, const cell_t *params) {
	const int leader = params[1];

	if (leader < 1 || leader > gpGlobals->maxClients) {
		return pContext->ThrowNativeError("Invalid leader index %d", leader);
	}

	CBot* pLeaderBot = CBots::getBot(leader - 1);
	if (!pLeaderBot) {
		return 0;
	}

	edict_t* pLeaderEdict = pLeaderBot->getEdict();
	if (!pLeaderEdict || pLeaderEdict->IsFree()) {
		return 0;
	}

	CBotSquad* pSquad = CBotSquads::FindSquadByLeader(pLeaderEdict);
	if (pSquad) {
		CBotSquads::RemoveSquad(pSquad);
		return 1;
	}

	return 0;
}

/* native bool RCBot2_AddBotToSquad(int client, int leader); */
cell_t sm_RCBotAddBotToSquad(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];
	const int leader = params[2];

	if (client < 1 || client > gpGlobals->maxClients) {
		return pContext->ThrowNativeError("Invalid client index %d", client);
	}

	if (leader < 1 || leader > gpGlobals->maxClients) {
		return pContext->ThrowNativeError("Invalid leader index %d", leader);
	}

	CBot* pBot = CBots::getBot(client - 1);
	if (!pBot) {
		return pContext->ThrowNativeError("Client index %d is not a RCBot", client);
	}

	CBot* pLeaderBot = CBots::getBot(leader - 1);
	if (!pLeaderBot) {
		return pContext->ThrowNativeError("Leader index %d is not a RCBot", leader);
	}

	edict_t* pBotEdict = pBot->getEdict();
	edict_t* pLeaderEdict = pLeaderBot->getEdict();

	if (!pBotEdict || pBotEdict->IsFree() || !pLeaderEdict || pLeaderEdict->IsFree()) {
		return 0;
	}

	CBotSquad* pSquad = CBotSquads::AddSquadMember(pLeaderEdict, pBotEdict);
	if (pSquad) {
		pBot->setSquad(pSquad);
		return 1;
	}

	return 0;
}

/* native bool RCBot2_RemoveBotFromSquad(int client); */
cell_t sm_RCBotRemoveBotFromSquad(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	if (client < 1 || client > gpGlobals->maxClients) {
		return pContext->ThrowNativeError("Invalid client index %d", client);
	}

	CBot* pBot = CBots::getBot(client - 1);
	if (!pBot) {
		return pContext->ThrowNativeError("Client index %d is not a RCBot", client);
	}

	if (!pBot->inSquad()) {
		return 0;
	}

	edict_t* pBotEdict = pBot->getEdict();
	if (!pBotEdict || pBotEdict->IsFree()) {
		return 0;
	}

	CBotSquad* pSquad = pBot->getSquad();
	if (pSquad) {
		CBotSquads::removeSquadMember(pSquad, pBotEdict);
		pBot->setSquad(nullptr);
		return 1;
	}

	return 0;
}

/* native int RCBot2_GetBotSquadLeader(int client); */
cell_t sm_RCBotGetBotSquadLeader(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	if (client < 1 || client > gpGlobals->maxClients) {
		return pContext->ThrowNativeError("Invalid client index %d", client);
	}

	CBot* pBot = CBots::getBot(client - 1);
	if (!pBot) {
		return pContext->ThrowNativeError("Client index %d is not a RCBot", client);
	}

	if (!pBot->inSquad()) {
		return -1;
	}

	CBotSquad* pSquad = pBot->getSquad();
	if (pSquad) {
		edict_t* pLeader = pSquad->GetLeader();
		if (pLeader && !pLeader->IsFree()) {
			return engine->IndexOfEdict(pLeader);
		}
	}

	return -1;
}

/* native int RCBot2_GetSquadMemberCount(int leader); */
cell_t sm_RCBotGetSquadMemberCount(IPluginContext *pContext, const cell_t *params) {
	const int leader = params[1];

	if (leader < 1 || leader > gpGlobals->maxClients) {
		return 0;
	}

	CBot* pLeaderBot = CBots::getBot(leader - 1);
	if (!pLeaderBot) {
		return 0;
	}

	edict_t* pLeaderEdict = pLeaderBot->getEdict();
	if (!pLeaderEdict || pLeaderEdict->IsFree()) {
		return 0;
	}

	CBotSquad* pSquad = CBotSquads::FindSquadByLeader(pLeaderEdict);
	if (pSquad) {
		return static_cast<int>(pSquad->numMembers());
	}

	return 0;
}

/* native int RCBot2_GetSquadMembers(int leader, int[] members, int maxsize); */
cell_t sm_RCBotGetSquadMembers(IPluginContext *pContext, const cell_t *params) {
	const int leader = params[1];
	const int maxsize = params[3];

	if (leader < 1 || leader > gpGlobals->maxClients) {
		return 0;
	}

	if (maxsize <= 0) {
		return 0;
	}

	CBot* pLeaderBot = CBots::getBot(leader - 1);
	if (!pLeaderBot) {
		return 0;
	}

	edict_t* pLeaderEdict = pLeaderBot->getEdict();
	if (!pLeaderEdict || pLeaderEdict->IsFree()) {
		return 0;
	}

	CBotSquad* pSquad = CBotSquads::FindSquadByLeader(pLeaderEdict);
	if (!pSquad) {
		return 0;
	}

	cell_t *addr;
	pContext->LocalToPhysAddr(params[2], &addr);

	const std::size_t numMembers = pSquad->numMembers();
	int count = 0;

	for (std::size_t i = 0; i < numMembers && count < maxsize; i++) {
		edict_t* pMember = pSquad->getMember(i);
		if (pMember && !pMember->IsFree()) {
			addr[count++] = engine->IndexOfEdict(pMember);
		}
	}

	return count;
}

/* native bool RCBot2_IsInSquad(int client); */
cell_t sm_RCBotIsInSquad(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	if (client < 1 || client > gpGlobals->maxClients) {
		return pContext->ThrowNativeError("Invalid client index %d", client);
	}

	CBot* pBot = CBots::getBot(client - 1);
	if (!pBot) {
		return pContext->ThrowNativeError("Client index %d is not a RCBot", client);
	}

	return pBot->inSquad();
}

//=============================================================================
// Phase 6: Advanced Bot Management Natives
//=============================================================================

/* native bool RCBot2_KickBot(int client); */
cell_t sm_RCBotKickBot(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	if (client < 1 || client > gpGlobals->maxClients) {
		return pContext->ThrowNativeError("Invalid client index %d", client);
	}

	CBot* pBot = CBots::getBot(client - 1);
	if (!pBot) {
		return pContext->ThrowNativeError("Client index %d is not a RCBot", client);
	}

	edict_t* pEdict = pBot->getEdict();
	if (!pEdict || pEdict->IsFree()) {
		return 0;
	}

	// Kick the bot using engine command
	engine->ServerCommand(UTIL_VarArgs("kickid %d\n", engine->GetPlayerUserId(pEdict)));
	return 1;
}

/* native int RCBot2_CountBots(int team = -1); */
cell_t sm_RCBotCountBots(IPluginContext *pContext, const cell_t *params) {
	const int team = params[1];

	if (team == -1) {
		// Count all bots
		return CBots::numBots();
	}

	// Count bots on specific team
	return CBotGlobals::numBotsOnTeam(team, false);
}

/* native int RCBot2_GetBotByIndex(int index); */
cell_t sm_RCBotGetBotByIndex(IPluginContext *pContext, const cell_t *params) {
	const int index = params[1];

	if (index < 0 || index >= gpGlobals->maxClients) {
		return -1;
	}

	int currentIndex = 0;
	for (int i = 0; i < gpGlobals->maxClients; i++) {
		CBot* pBot = CBots::getBot(i);
		if (pBot && pBot->inUse()) {
			if (currentIndex == index) {
				return i + 1; // Return client index (1-based)
			}
			currentIndex++;
		}
	}

	return -1;
}

/* native bool RCBot2_GetBotName(int client, char[] name, int maxlength); */
cell_t sm_RCBotGetBotName(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];
	const int maxlength = params[3];

	if (client < 1 || client > gpGlobals->maxClients) {
		return pContext->ThrowNativeError("Invalid client index %d", client);
	}

	CBot* pBot = CBots::getBot(client - 1);
	if (!pBot) {
		return pContext->ThrowNativeError("Client index %d is not a RCBot", client);
	}

	edict_t* pEdict = pBot->getEdict();
	if (!pEdict || pEdict->IsFree()) {
		return 0;
	}

	const char* botName = engine->GetClientConVarValue(engine->IndexOfEdict(pEdict), "name");
	if (!botName) {
		return 0;
	}

	pContext->StringToLocal(params[2], maxlength, botName);
	return 1;
}
