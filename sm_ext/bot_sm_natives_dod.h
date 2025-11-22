#pragma once

/**
 * DOD:S-Specific SourceMod Natives for RCBot2
 * Phase 2.2: Game-Specific Extensions - Day of Defeat: Source
 */

#include <IExtensionSys.h>

using namespace SourceMod;
using namespace SourcePawn;

// DOD:S-Specific Natives
cell_t sm_RCBotDOD_HasBomb(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotDOD_RemoveBomb(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotDOD_Prone(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotDOD_UnProne(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotDOD_HasMG(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotDOD_HasSniperRifle(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotDOD_VoiceCommand(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotDOD_GetArmorPercent(IPluginContext *pContext, const cell_t *params);

const sp_nativeinfo_t g_RCBotNatives_DOD[] = {
	{ "RCBot2_DOD_HasBomb", sm_RCBotDOD_HasBomb },
	{ "RCBot2_DOD_RemoveBomb", sm_RCBotDOD_RemoveBomb },
	{ "RCBot2_DOD_Prone", sm_RCBotDOD_Prone },
	{ "RCBot2_DOD_UnProne", sm_RCBotDOD_UnProne },
	{ "RCBot2_DOD_HasMG", sm_RCBotDOD_HasMG },
	{ "RCBot2_DOD_HasSniperRifle", sm_RCBotDOD_HasSniperRifle },
	{ "RCBot2_DOD_VoiceCommand", sm_RCBotDOD_VoiceCommand },
	{ "RCBot2_DOD_GetArmorPercent", sm_RCBotDOD_GetArmorPercent },
	{ nullptr, nullptr }
};
