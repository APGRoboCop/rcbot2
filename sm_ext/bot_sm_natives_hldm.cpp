// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

/**
 * HL2DM-Specific SourceMod Natives for RCBot2
 * Phase 2.3: Game-Specific Extensions - Half-Life 2: Deathmatch
 */

#include "bot_sm_natives_hldm.h"
#include "bot.h"
#include "bot_hldm_bot.h"

// Helper function to get CHLDMBot from client index
static CHLDMBot* GetHLDMBot(int client, IPluginContext *pContext) {
	if (client < 1 || client > gpGlobals->maxClients) {
		pContext->ThrowNativeError("Invalid client index %d", client);
		return nullptr;
	}

	CBot* bot = CBots::getBot(client - 1);
	if (!bot) {
		pContext->ThrowNativeError("Client index %d is not a RCBot", client);
		return nullptr;
	}

	// Verify this is an HLDM bot
	if (!bot->isHLDM()) {
		pContext->ThrowNativeError("Client index %d is not an HL2DM bot", client);
		return nullptr;
	}

	return static_cast<CHLDMBot*>(bot);
}

//=============================================================================
// HL2DM Sprint Natives
//=============================================================================

/* native bool RCBot2_HLDM_UseSprint(int client, bool enable); */
cell_t sm_RCBotHLDM_UseSprint(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];
	const bool enable = params[2] != 0;

	CHLDMBot* bot = GetHLDMBot(client, pContext);
	if (!bot) {
		return 0;
	}

	// Set sprint button state
	if (enable) {
		bot->sprint();
	} else {
		bot->letGoSprint();
	}

	return 1;
}

/* native bool RCBot2_HLDM_IsSprinting(int client); */
cell_t sm_RCBotHLDM_IsSprinting(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	CHLDMBot* bot = GetHLDMBot(client, pContext);
	if (!bot) {
		return 0;
	}

	// Check if bot is currently sprinting
	return bot->isSprinting() ? 1 : 0;
}

/* native float RCBot2_HLDM_GetSprintTime(int client); */
cell_t sm_RCBotHLDM_GetSprintTime(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	CHLDMBot* bot = GetHLDMBot(client, pContext);
	if (!bot) {
		return sp_ftoc(0.0f);
	}

	// Get remaining sprint time
	// Note: This would need to be exposed from CHLDMBot if not already public
	// For now, return a placeholder
	return sp_ftoc(0.0f);
}

//=============================================================================
// HL2DM Equipment Natives
//=============================================================================

/* native bool RCBot2_HLDM_UseCharger(int client, int charger); */
cell_t sm_RCBotHLDM_UseCharger(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];
	const int chargerEntity = params[2];

	CHLDMBot* bot = GetHLDMBot(client, pContext);
	if (!bot) {
		return 0;
	}

	// Validate charger entity
	if (chargerEntity < 0 || chargerEntity > gpGlobals->maxEntities) {
		pContext->ThrowNativeError("Invalid charger entity %d", chargerEntity);
		return 0;
	}

	edict_t* pCharger = INDEXENT(chargerEntity);
	if (!pCharger || pCharger->IsFree()) {
		return 0;
	}

	// Set bot to use the charger
	bot->setLookAt(CBotGlobals::entityOrigin(pCharger));
	bot->use();

	return 1;
}

/* native float RCBot2_HLDM_GetArmorPercent(int client); */
cell_t sm_RCBotHLDM_GetArmorPercent(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	CHLDMBot* bot = GetHLDMBot(client, pContext);
	if (!bot) {
		return sp_ftoc(0.0f);
	}

	// Get armor percentage (0.0 - 1.0)
	const float armorPercent = bot->getArmorPercent();
	return sp_ftoc(armorPercent);
}
