// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

/**
 * TF2-Specific SourceMod Natives for RCBot2
 * Phase 2: Game-Specific Extensions - Team Fortress 2
 */

#include "bot_sm_natives_tf2.h"
#include "bot.h"
#include "bot_fortress.h"
#include "bot_profile.h"

// TF2 Class enum mapping (from rcbot2_tf2.inc to bot_fortress.h)
enum : std::uint8_t {
	TFCLASS_UNKNOWN = 0,
	TFCLASS_SCOUT,
	TFCLASS_SNIPER,
	TFCLASS_SOLDIER,
	TFCLASS_DEMOMAN,
	TFCLASS_MEDIC,
	TFCLASS_HEAVY,
	TFCLASS_PYRO,
	TFCLASS_SPY,
	TFCLASS_ENGINEER
};

// TF2 Object enum mapping
enum : std::uint8_t {
	TFOBJECT_DISPENSER = 0,
	TFOBJECT_TELEPORTER,
	TFOBJECT_SENTRY,
	TFOBJECT_SAPPER,
	TFOBJECT_TELE_EXIT,
	TFOBJECT_TELE_ENTRANCE
};

// Helper function to get CBotTF2 from client index
static CBotTF2* GetTF2Bot(int client, IPluginContext *pContext) {
	if (client < 1 || client > gpGlobals->maxClients) {
		pContext->ThrowNativeError("Invalid client index %d", client);
		return nullptr;
	}

	CBot* bot = CBots::getBot(client - 1);
	if (!bot) {
		pContext->ThrowNativeError("Client index %d is not a RCBot", client);
		return nullptr;
	}

	if (!bot->isTF2()) {
		pContext->ThrowNativeError("Client index %d is not a TF2 bot", client);
		return nullptr;
	}

	return static_cast<CBotTF2*>(bot);
}

// Convert SourcePawn TFClass to C++ TF_Class
static TF_Class SPClassToTFClass(int spClass) {
	switch (spClass) {
		case TFCLASS_SCOUT: return TF_CLASS_SCOUT;
		case TFCLASS_SNIPER: return TF_CLASS_SNIPER;
		case TFCLASS_SOLDIER: return TF_CLASS_SOLDIER;
		case TFCLASS_DEMOMAN: return TF_CLASS_DEMOMAN;
		case TFCLASS_MEDIC: return TF_CLASS_MEDIC;
		case TFCLASS_HEAVY: return TF_CLASS_HWGUY;
		case TFCLASS_PYRO: return TF_CLASS_PYRO;
		case TFCLASS_SPY: return TF_CLASS_SPY;
		case TFCLASS_ENGINEER: return TF_CLASS_ENGINEER;
		default: return TF_CLASS_UNDEFINED;
	}
}

// Convert C++ TF_Class to SourcePawn TFClass
static int TFClassToSPClass(TF_Class tfClass) {
	switch (tfClass) {
		case TF_CLASS_SCOUT: return TFCLASS_SCOUT;
		case TF_CLASS_SNIPER: return TFCLASS_SNIPER;
		case TF_CLASS_SOLDIER: return TFCLASS_SOLDIER;
		case TF_CLASS_DEMOMAN: return TFCLASS_DEMOMAN;
		case TF_CLASS_MEDIC: return TFCLASS_MEDIC;
		case TF_CLASS_HWGUY: return TFCLASS_HEAVY;
		case TF_CLASS_PYRO: return TFCLASS_PYRO;
		case TF_CLASS_SPY: return TFCLASS_SPY;
		case TF_CLASS_ENGINEER: return TFCLASS_ENGINEER;
		default: return TFCLASS_UNKNOWN;
	}
}

// Convert SourcePawn object type to eEngiBuild
static eEngiBuild SPObjectToEngiBuild(int spObject) {
	switch (spObject) {
		case TFOBJECT_DISPENSER: return ENGI_DISP;
		case TFOBJECT_TELEPORTER:
		case TFOBJECT_TELE_ENTRANCE: return ENGI_ENTRANCE;
		case TFOBJECT_TELE_EXIT: return ENGI_EXIT;
		case TFOBJECT_SENTRY: return ENGI_SENTRY;
		case TFOBJECT_SAPPER: return ENGI_SAPPER;
		default: return ENGI_DISP;
	}
}

//=============================================================================
// TF2 Class Management Natives
//=============================================================================

/* native bool RCBot2_TF2_SetClass(int client, TFClassType class); */
cell_t sm_RCBotTF2SetClass(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];
	const int classType = params[2];

	CBotTF2* bot = GetTF2Bot(client, pContext);
	if (!bot) {
		return 0;
	}

	TF_Class tfClass = SPClassToTFClass(classType);
	if (tfClass == TF_CLASS_UNDEFINED) {
		return pContext->ThrowNativeError("Invalid TF2 class %d", classType);
	}

	bot->setClass(tfClass);
	return 1;
}

/* native TFClassType RCBot2_TF2_GetClass(int client); */
cell_t sm_RCBotTF2GetClass(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	CBotTF2* bot = GetTF2Bot(client, pContext);
	if (!bot) {
		return 0;
	}

	TF_Class tfClass = bot->getClass();
	return TFClassToSPClass(tfClass);
}

//=============================================================================
// TF2 Engineer Natives
//=============================================================================

/* native bool RCBot2_TF2_EngineerBuild(int client, TFObjectType object, TFEngiCmd command); */
cell_t sm_RCBotTF2EngineerBuild(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];
	const int objectType = params[2];
	const int command = params[3];

	CBotTF2* bot = GetTF2Bot(client, pContext);
	if (!bot) {
		return 0;
	}

	// Verify bot is an engineer
	if (bot->getClass() != TF_CLASS_ENGINEER) {
		return pContext->ThrowNativeError("Client %d is not an Engineer", client);
	}

	eEngiBuild building = SPObjectToEngiBuild(objectType);
	eEngiCmd cmd = static_cast<eEngiCmd>(command);

	bot->engineerBuild(building, cmd);
	return 1;
}

/* native bool RCBot2_TF2_HasBuilding(int client, TFObjectType object); */
cell_t sm_RCBotTF2HasBuilding(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];
	const int objectType = params[2];

	CBotTF2* bot = GetTF2Bot(client, pContext);
	if (!bot) {
		return 0;
	}

	eEngiBuild building = SPObjectToEngiBuild(objectType);
	return bot->hasEngineerBuilt(building);
}

/* native int RCBot2_TF2_GetBuilding(int client, TFObjectType object); */
cell_t sm_RCBotTF2GetBuilding(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];
	const int objectType = params[2];

	CBotTF2* bot = GetTF2Bot(client, pContext);
	if (!bot) {
		return -1;
	}

	eEngiBuild building = SPObjectToEngiBuild(objectType);
	edict_t* pBuilding = bot->findEngineerBuiltObject(building, 0);

	if (!pBuilding || pBuilding->IsFree()) {
		return -1;
	}

	return engine->IndexOfEdict(pBuilding);
}

//=============================================================================
// TF2 Medic Natives
//=============================================================================

/* native bool RCBot2_TF2_SetHealTarget(int client, int target); */
cell_t sm_RCBotTF2SetHealTarget(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];
	const int target = params[2];

	CBotTF2* bot = GetTF2Bot(client, pContext);
	if (!bot) {
		return 0;
	}

	// Verify bot is a medic
	if (bot->getClass() != TF_CLASS_MEDIC) {
		return pContext->ThrowNativeError("Client %d is not a Medic", client);
	}

	// Clear heal target if -1
	if (target == -1) {
		bot->clearHealingEntity();
		return 1;
	}

	// Validate target
	if (target < 1 || target > gpGlobals->maxClients) {
		return pContext->ThrowNativeError("Invalid target index %d", target);
	}

	edict_t* pTarget = engine->PEntityOfEntIndex(target);
	if (!pTarget || pTarget->IsFree()) {
		return 0;
	}

	// Use healPlayer to set the healing target
	bot->healPlayer(pTarget, bot->getHealingEntity());
	return 1;
}

/* native int RCBot2_TF2_GetHealTarget(int client); */
cell_t sm_RCBotTF2GetHealTarget(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	CBotTF2* bot = GetTF2Bot(client, pContext);
	if (!bot) {
		return -1;
	}

	edict_t* pHealTarget = bot->getHealingEntity();
	if (!pHealTarget || pHealTarget->IsFree()) {
		return -1;
	}

	return engine->IndexOfEdict(pHealTarget);
}

/* native float RCBot2_TF2_GetUberCharge(int client); */
cell_t sm_RCBotTF2GetUberCharge(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	CBotTF2* bot = GetTF2Bot(client, pContext);
	if (!bot) {
		return sp_ftoc(-1.0f);
	}

	// Verify bot is a medic
	if (bot->getClass() != TF_CLASS_MEDIC) {
		return sp_ftoc(-1.0f);
	}

	// Get uber charge from entity property
	// This requires accessing the TF2 player's m_flCharge property
	edict_t* pEdict = bot->getEdict();
	if (!pEdict || pEdict->IsFree()) {
		return sp_ftoc(-1.0f);
	}

	// Try to get ubercharge via CClassInterface or entity properties
	// For now, return 0.0 as placeholder - would need proper entity property access
	// TODO: Implement proper ubercharge reading via CClassInterface::getUberChargeLevel()
	return sp_ftoc(0.0f);
}

//=============================================================================
// TF2 Spy Natives
//=============================================================================

/* native bool RCBot2_TF2_Disguise(int client, int team, TFClassType class); */
cell_t sm_RCBotTF2Disguise(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];
	const int team = params[2];
	const int classType = params[3];

	CBotTF2* bot = GetTF2Bot(client, pContext);
	if (!bot) {
		return 0;
	}

	// Verify bot is a spy
	if (bot->getClass() != TF_CLASS_SPY) {
		return pContext->ThrowNativeError("Client %d is not a Spy", client);
	}

	// Convert to TF_Class
	TF_Class disguiseClass = SPClassToTFClass(classType);
	if (disguiseClass == TF_CLASS_UNDEFINED) {
		return pContext->ThrowNativeError("Invalid disguise class %d", classType);
	}

	// Call spy disguise
	bot->spyDisguise(team, static_cast<byte>(disguiseClass));
	return 1;
}

/* native bool RCBot2_TF2_IsCloaked(int client); */
cell_t sm_RCBotTF2IsCloaked(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	CBotTF2* bot = GetTF2Bot(client, pContext);
	if (!bot) {
		return 0;
	}

	return bot->isCloaked();
}

/* native bool RCBot2_TF2_IsDisguised(int client); */
cell_t sm_RCBotTF2IsDisguised(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	CBotTF2* bot = GetTF2Bot(client, pContext);
	if (!bot) {
		return 0;
	}

	return bot->isDisguised();
}

/* native bool RCBot2_TF2_ResetCloakTime(int client); */
cell_t sm_RCBotTF2ResetCloakTime(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	CBotTF2* bot = GetTF2Bot(client, pContext);
	if (!bot) {
		return 0;
	}

	// Verify bot is a spy
	if (bot->getClass() != TF_CLASS_SPY) {
		return pContext->ThrowNativeError("Client %d is not a Spy", client);
	}

	bot->resetCloakTime();
	return 1;
}

//=============================================================================
// TF2 General Natives
//=============================================================================

/* native bool RCBot2_TF2_CallMedic(int client); */
cell_t sm_RCBotTF2CallMedic(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	CBotTF2* bot = GetTF2Bot(client, pContext);
	if (!bot) {
		return 0;
	}

	bot->callMedic();
	return 1;
}

/* native bool RCBot2_TF2_Taunt(int client, bool force); */
cell_t sm_RCBotTF2Taunt(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];
	const bool force = params[2] != 0;

	CBotTF2* bot = GetTF2Bot(client, pContext);
	if (!bot) {
		return 0;
	}

	bot->taunt(force);
	return 1;
}
