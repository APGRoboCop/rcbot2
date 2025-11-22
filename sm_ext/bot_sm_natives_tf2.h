#pragma once

#include <IExtensionSys.h>

using namespace SourceMod;
using namespace SourcePawn;

//=============================================================================
// TF2-Specific Native Declarations
//=============================================================================

// TF2 Class Management
cell_t sm_RCBotTF2SetClass(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotTF2GetClass(IPluginContext *pContext, const cell_t *params);

// TF2 Engineer
cell_t sm_RCBotTF2EngineerBuild(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotTF2HasBuilding(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotTF2GetBuilding(IPluginContext *pContext, const cell_t *params);

// TF2 Medic
cell_t sm_RCBotTF2SetHealTarget(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotTF2GetHealTarget(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotTF2GetUberCharge(IPluginContext *pContext, const cell_t *params);

// TF2 Spy
cell_t sm_RCBotTF2Disguise(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotTF2IsCloaked(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotTF2IsDisguised(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotTF2ResetCloakTime(IPluginContext *pContext, const cell_t *params);

// TF2 General
cell_t sm_RCBotTF2CallMedic(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotTF2Taunt(IPluginContext *pContext, const cell_t *params);

// Native registration array for TF2-specific natives
const sp_nativeinfo_t g_RCBotNativesTF2[] = {
	// TF2 Class Management
	{ "RCBot2_TF2_SetClass", sm_RCBotTF2SetClass },
	{ "RCBot2_TF2_GetClass", sm_RCBotTF2GetClass },

	// TF2 Engineer
	{ "RCBot2_TF2_EngineerBuild", sm_RCBotTF2EngineerBuild },
	{ "RCBot2_TF2_HasBuilding", sm_RCBotTF2HasBuilding },
	{ "RCBot2_TF2_GetBuilding", sm_RCBotTF2GetBuilding },

	// TF2 Medic
	{ "RCBot2_TF2_SetHealTarget", sm_RCBotTF2SetHealTarget },
	{ "RCBot2_TF2_GetHealTarget", sm_RCBotTF2GetHealTarget },
	{ "RCBot2_TF2_GetUberCharge", sm_RCBotTF2GetUberCharge },

	// TF2 Spy
	{ "RCBot2_TF2_Disguise", sm_RCBotTF2Disguise },
	{ "RCBot2_TF2_IsCloaked", sm_RCBotTF2IsCloaked },
	{ "RCBot2_TF2_IsDisguised", sm_RCBotTF2IsDisguised },
	{ "RCBot2_TF2_ResetCloakTime", sm_RCBotTF2ResetCloakTime },

	// TF2 General
	{ "RCBot2_TF2_CallMedic", sm_RCBotTF2CallMedic },
	{ "RCBot2_TF2_Taunt", sm_RCBotTF2Taunt },

	{nullptr, nullptr},
};
