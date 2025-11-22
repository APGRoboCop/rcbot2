// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

/**
 * DOD:S-Specific SourceMod Natives for RCBot2
 * Phase 2.2: Game-Specific Extensions - Day of Defeat: Source
 */

#include "bot_sm_natives_dod.h"
#include "bot.h"
#include "bot_dod_bot.h"

// Helper function to get CDODBot from client index
static CDODBot* GetDODBot(int client, IPluginContext *pContext) {
	if (client < 1 || client > gpGlobals->maxClients) {
		pContext->ThrowNativeError("Invalid client index %d", client);
		return nullptr;
	}

	CBot* bot = CBots::getBot(client - 1);
	if (!bot) {
		pContext->ThrowNativeError("Client index %d is not a RCBot", client);
		return nullptr;
	}

	// Verify this is a DOD bot
	if (!bot->isDOD()) {
		pContext->ThrowNativeError("Client index %d is not a DOD:S bot", client);
		return nullptr;
	}

	return static_cast<CDODBot*>(bot);
}

//=============================================================================
// DOD:S Bomb Natives
//=============================================================================

/* native bool RCBot2_DOD_HasBomb(int client); */
cell_t sm_RCBotDOD_HasBomb(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	CDODBot* bot = GetDODBot(client, pContext);
	if (!bot) {
		return 0;
	}

	return bot->hasBomb() ? 1 : 0;
}

/* native bool RCBot2_DOD_RemoveBomb(int client); */
cell_t sm_RCBotDOD_RemoveBomb(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	CDODBot* bot = GetDODBot(client, pContext);
	if (!bot) {
		return 0;
	}

	bot->removeBomb();
	return 1;
}

//=============================================================================
// DOD:S Movement Natives
//=============================================================================

/* native bool RCBot2_DOD_Prone(int client); */
cell_t sm_RCBotDOD_Prone(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	CDODBot* bot = GetDODBot(client, pContext);
	if (!bot) {
		return 0;
	}

	bot->prone();
	return 1;
}

/* native bool RCBot2_DOD_UnProne(int client); */
cell_t sm_RCBotDOD_UnProne(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	CDODBot* bot = GetDODBot(client, pContext);
	if (!bot) {
		return 0;
	}

	bot->unProne();
	return 1;
}

//=============================================================================
// DOD:S Weapon Natives
//=============================================================================

/* native bool RCBot2_DOD_HasMG(int client); */
cell_t sm_RCBotDOD_HasMG(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	CDODBot* bot = GetDODBot(client, pContext);
	if (!bot) {
		return 0;
	}

	return bot->hasMG() ? 1 : 0;
}

/* native bool RCBot2_DOD_HasSniperRifle(int client); */
cell_t sm_RCBotDOD_HasSniperRifle(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	CDODBot* bot = GetDODBot(client, pContext);
	if (!bot) {
		return 0;
	}

	return bot->hasSniperRifle() ? 1 : 0;
}

//=============================================================================
// DOD:S Communication Natives
//=============================================================================

/* native bool RCBot2_DOD_VoiceCommand(int client, int voiceCmd); */
cell_t sm_RCBotDOD_VoiceCommand(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];
	const byte voiceCmd = static_cast<byte>(params[2]);

	CDODBot* bot = GetDODBot(client, pContext);
	if (!bot) {
		return 0;
	}

	bot->voiceCommand(voiceCmd);
	return 1;
}

//=============================================================================
// DOD:S Info Natives
//=============================================================================

/* native float RCBot2_DOD_GetArmorPercent(int client); */
cell_t sm_RCBotDOD_GetArmorPercent(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	CDODBot* bot = GetDODBot(client, pContext);
	if (!bot) {
		return sp_ftoc(0.0f);
	}

	const float armorPercent = bot->getArmorPercent();
	return sp_ftoc(armorPercent);
}
