#pragma once

#include <IExtensionSys.h>

using namespace SourceMod;
using namespace SourcePawn;

// Existing natives
cell_t sm_RCBotIsWaypointAvailable(IPluginContext *pContext, const cell_t *params);

cell_t sm_RCBotCreate(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotIsClientBot(IPluginContext *pContext, const cell_t *params);

cell_t sm_RCBotSetProfileInt(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotGetProfileInt(IPluginContext *pContext, const cell_t *params);

cell_t sm_RCBotSetProfileFloat(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotGetProfileFloat(IPluginContext *pContext, const cell_t *params);

// Enhanced natives - Roadmap Medium-Term Goals: Extended SourceMod Natives
cell_t sm_RCBotKickBot(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotKickAll(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotGetBotCount(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotSetSkill(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotGetSkill(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotGetTeam(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotGetClass(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotSetClass(IPluginContext *pContext, const cell_t *params);

const sp_nativeinfo_t g_RCBotNatives[] = {
	// Waypoint natives
	{ "RCBot2_IsWaypointAvailable", sm_RCBotIsWaypointAvailable },

	// Bot management natives
	{ "RCBot2_CreateBot", sm_RCBotCreate },
	{ "IsRCBot2Client", sm_RCBotIsClientBot },
	{ "RCBot2_KickBot", sm_RCBotKickBot },
	{ "RCBot2_KickAll", sm_RCBotKickAll },
	{ "RCBot2_GetBotCount", sm_RCBotGetBotCount },

	// Profile customization natives
	{ "RCBot2_SetProfileInt", sm_RCBotSetProfileInt },
	{ "RCBot2_GetProfileInt", sm_RCBotGetProfileInt },
	{ "RCBot2_SetProfileFloat", sm_RCBotSetProfileFloat },
	{ "RCBot2_GetProfileFloat", sm_RCBotGetProfileFloat },

	// Skill and class control natives
	{ "RCBot2_SetSkill", sm_RCBotSetSkill },
	{ "RCBot2_GetSkill", sm_RCBotGetSkill },
	{ "RCBot2_GetTeam", sm_RCBotGetTeam },
	{ "RCBot2_GetClass", sm_RCBotGetClass },
	{ "RCBot2_SetClass", sm_RCBotSetClass },

	{nullptr, nullptr},
};
