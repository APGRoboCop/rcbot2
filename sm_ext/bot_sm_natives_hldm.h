#pragma once

/**
 * HL2DM-Specific SourceMod Natives for RCBot2
 * Phase 2.3: Game-Specific Extensions - Half-Life 2: Deathmatch
 */

#include <IExtensionSys.h>

using namespace SourceMod;
using namespace SourcePawn;

// HL2DM-Specific Natives
cell_t sm_RCBotHLDM_UseSprint(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotHLDM_IsSprinting(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotHLDM_GetSprintTime(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotHLDM_UseCharger(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotHLDM_GetArmorPercent(IPluginContext *pContext, const cell_t *params);

const sp_nativeinfo_t g_RCBotNatives_HLDM[] = {
	{ "RCBot2_HLDM_UseSprint", sm_RCBotHLDM_UseSprint },
	{ "RCBot2_HLDM_IsSprinting", sm_RCBotHLDM_IsSprinting },
	{ "RCBot2_HLDM_GetSprintTime", sm_RCBotHLDM_GetSprintTime },
	{ "RCBot2_HLDM_UseCharger", sm_RCBotHLDM_UseCharger },
	{ "RCBot2_HLDM_GetArmorPercent", sm_RCBotHLDM_GetArmorPercent },
	{ nullptr, nullptr }
};
