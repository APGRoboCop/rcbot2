// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

/**
 * HL2DM-Specific SourceMod Natives for RCBot2
 * Phase 2.3: Game-Specific Extensions - Half-Life 2: Deathmatch
 *
 * Comprehensive HL2DM API including:
 * - Sprint mechanics
 * - Suit power and chargers (health/armor)
 * - Gravity Gun physics manipulation
 * - Weapon and ammo management
 * - Grenade throwing
 * - Equipment interaction (crates, batteries, health kits)
 */

#include "bot_sm_natives_hldm.h"
#include "bot.h"
#include "bot_hldm_bot.h"
#include "bot_globals.h"
#include "bot_buttons.h"
#include "bot_weapons.h"
#include "bot_utility.h"
#include "bot_schedule.h"
#include "in_buttons.h"

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

// Helper function to convert MyEHandle to entity index
static int EHandleToIndex(const MyEHandle& handle) {
	edict_t* pEntity = handle.get();
	if (!pEntity || pEntity->IsFree()) {
		return -1;
	}
	return ENTINDEX(pEntity);
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
		bot->m_pButtons->holdButton(IN_SPEED, 0.0f, 1.0f, 0.0f);
	} else {
		bot->m_pButtons->letGo(IN_SPEED);
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

	// Check if sprint button is held
	return bot->m_pButtons->holdingButton(IN_SPEED) ? 1 : 0;
}

/* native float RCBot2_HLDM_GetSprintTime(int client); */
cell_t sm_RCBotHLDM_GetSprintTime(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	CHLDMBot* bot = GetHLDMBot(client, pContext);
	if (!bot) {
		return sp_ftoc(0.0f);
	}

	// Get sprint time remaining (m_fSprintTime is when sprint can be used again)
	const float currentTime = engine->Time();
	const float sprintCooldown = bot->m_fSprintTime;
	const float timeRemaining = (sprintCooldown > currentTime) ? (sprintCooldown - currentTime) : 0.0f;

	return sp_ftoc(timeRemaining);
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
	bot->m_pButtons->holdButton(IN_USE, 0.0f, 1.0f, 0.0f);

	return 1;
}

/* native bool RCBot2_HLDM_UseHealthCharger(int client, int charger); */
cell_t sm_RCBotHLDM_UseHealthCharger(IPluginContext *pContext, const cell_t *params) {
	// Same implementation as armor charger
	return sm_RCBotHLDM_UseCharger(pContext, params);
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

//=============================================================================
// HL2DM Gravity Gun Natives
//=============================================================================

/* native bool RCBot2_HLDM_IsCarryingObject(int client); */
cell_t sm_RCBotHLDM_IsCarryingObject(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	CHLDMBot* bot = GetHLDMBot(client, pContext);
	if (!bot) {
		return 0;
	}

	// Check if bot is carrying an object with gravity gun
	return (bot->m_pCarryingObject != nullptr && !bot->m_pCarryingObject->IsFree()) ? 1 : 0;
}

/* native int RCBot2_HLDM_GetCarriedObject(int client); */
cell_t sm_RCBotHLDM_GetCarriedObject(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	CHLDMBot* bot = GetHLDMBot(client, pContext);
	if (!bot) {
		return -1;
	}

	// Return entity index of carried object, or -1 if none
	if (bot->m_pCarryingObject && !bot->m_pCarryingObject->IsFree()) {
		return ENTINDEX(bot->m_pCarryingObject);
	}

	return -1;
}

/* native bool RCBot2_HLDM_PickupObject(int client, int object); */
cell_t sm_RCBotHLDM_PickupObject(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];
	const int objectEntity = params[2];

	CHLDMBot* bot = GetHLDMBot(client, pContext);
	if (!bot) {
		return 0;
	}

	// Validate object entity
	if (objectEntity < 0 || objectEntity > gpGlobals->maxEntities) {
		pContext->ThrowNativeError("Invalid object entity %d", objectEntity);
		return 0;
	}

	edict_t* pObject = INDEXENT(objectEntity);
	if (!pObject || pObject->IsFree()) {
		return 0;
	}

	// Add schedule to pick up object with gravity gun
	CBotSchedule* pSchedule = new CBotSchedule(new CBotGravGunPickup(bot->m_pCurrentWeapon, pObject));
	pSchedule->setID(SCHED_GRAVGUN_PICKUP);
	bot->m_pSchedules->addFront(pSchedule);

	return 1;
}

/* native bool RCBot2_HLDM_DropObject(int client); */
cell_t sm_RCBotHLDM_DropObject(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	CHLDMBot* bot = GetHLDMBot(client, pContext);
	if (!bot) {
		return 0;
	}

	// Drop currently carried object (secondary fire)
	if (bot->m_pCarryingObject && !bot->m_pCarryingObject->IsFree()) {
		bot->m_pButtons->tap(IN_ATTACK2);
		return 1;
	}

	return 0;
}

/* native bool RCBot2_HLDM_LaunchObject(int client); */
cell_t sm_RCBotHLDM_LaunchObject(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	CHLDMBot* bot = GetHLDMBot(client, pContext);
	if (!bot) {
		return 0;
	}

	// Launch currently carried object (primary fire)
	if (bot->m_pCarryingObject && !bot->m_pCarryingObject->IsFree()) {
		bot->m_pButtons->attack(0.1f);
		return 1;
	}

	return 0;
}

//=============================================================================
// HL2DM Physics & Breakables Natives
//=============================================================================

/* native int RCBot2_HLDM_GetNearestPhysicsObject(int client); */
cell_t sm_RCBotHLDM_GetNearestPhysicsObject(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	CHLDMBot* bot = GetHLDMBot(client, pContext);
	if (!bot) {
		return -1;
	}

	// Return entity index of nearest physics object
	return EHandleToIndex(bot->m_NearestPhysObj);
}

/* native int RCBot2_HLDM_GetNearestBreakable(int client); */
cell_t sm_RCBotHLDM_GetNearestBreakable(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	CHLDMBot* bot = GetHLDMBot(client, pContext);
	if (!bot) {
		return -1;
	}

	// Return entity index of nearest breakable object
	return EHandleToIndex(bot->m_NearestBreakable);
}

/* native bool RCBot2_HLDM_SetFailedObject(int client, int object); */
cell_t sm_RCBotHLDM_SetFailedObject(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];
	const int objectEntity = params[2];

	CHLDMBot* bot = GetHLDMBot(client, pContext);
	if (!bot) {
		return 0;
	}

	// Validate object entity
	if (objectEntity < 0 || objectEntity > gpGlobals->maxEntities) {
		pContext->ThrowNativeError("Invalid object entity %d", objectEntity);
		return 0;
	}

	edict_t* pObject = INDEXENT(objectEntity);
	if (!pObject || pObject->IsFree()) {
		return 0;
	}

	// Mark this object as failed (bot will avoid it)
	bot->setFailedObject(pObject);
	return 1;
}

/* native int RCBot2_HLDM_GetFailedObject(int client); */
cell_t sm_RCBotHLDM_GetFailedObject(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	CHLDMBot* bot = GetHLDMBot(client, pContext);
	if (!bot) {
		return -1;
	}

	// Return entity index of failed object
	edict_t* pFailed = bot->getFailedObject();
	if (pFailed && !pFailed->IsFree()) {
		return ENTINDEX(pFailed);
	}

	return -1;
}

//=============================================================================
// HL2DM Item/Pickup Natives
//=============================================================================

/* native int RCBot2_HLDM_GetNearestWeapon(int client); */
cell_t sm_RCBotHLDM_GetNearestWeapon(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	CHLDMBot* bot = GetHLDMBot(client, pContext);
	if (!bot) {
		return -1;
	}

	// Return entity index of nearest weapon
	return EHandleToIndex(bot->m_pNearbyWeapon);
}

/* native int RCBot2_HLDM_GetNearestHealthKit(int client); */
cell_t sm_RCBotHLDM_GetNearestHealthKit(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	CHLDMBot* bot = GetHLDMBot(client, pContext);
	if (!bot) {
		return -1;
	}

	// Return entity index of nearest health kit
	return EHandleToIndex(bot->m_pHealthKit);
}

/* native int RCBot2_HLDM_GetNearestBattery(int client); */
cell_t sm_RCBotHLDM_GetNearestBattery(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	CHLDMBot* bot = GetHLDMBot(client, pContext);
	if (!bot) {
		return -1;
	}

	// Return entity index of nearest battery (armor)
	return EHandleToIndex(bot->m_pBattery);
}

/* native int RCBot2_HLDM_GetNearestAmmoCrate(int client); */
cell_t sm_RCBotHLDM_GetNearestAmmoCrate(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	CHLDMBot* bot = GetHLDMBot(client, pContext);
	if (!bot) {
		return -1;
	}

	// Return entity index of nearest ammo crate
	return EHandleToIndex(bot->m_pAmmoCrate);
}

/* native bool RCBot2_HLDM_UseAmmoCrate(int client); */
cell_t sm_RCBotHLDM_UseAmmoCrate(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	CHLDMBot* bot = GetHLDMBot(client, pContext);
	if (!bot) {
		return 0;
	}

	// Use nearest ammo crate
	edict_t* pCrate = bot->m_pAmmoCrate.get();
	if (pCrate && !pCrate->IsFree()) {
		bot->setLookAt(CBotGlobals::entityOrigin(pCrate));
		bot->m_pButtons->attack(0.5f);
		return 1;
	}

	return 0;
}

//=============================================================================
// HL2DM Weapon Management Natives
//=============================================================================

/* native int RCBot2_HLDM_GetCurrentWeapon(int client); */
cell_t sm_RCBotHLDM_GetCurrentWeapon(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	CHLDMBot* bot = GetHLDMBot(client, pContext);
	if (!bot) {
		return -1;
	}

	// Return entity index of current weapon
	if (bot->m_pCurrentWeapon && !bot->m_pCurrentWeapon->IsFree()) {
		return ENTINDEX(bot->m_pCurrentWeapon);
	}

	return -1;
}

/* native int RCBot2_HLDM_GetWeaponClip1(int client); */
cell_t sm_RCBotHLDM_GetWeaponClip1(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	CHLDMBot* bot = GetHLDMBot(client, pContext);
	if (!bot) {
		return -1;
	}

	// Return primary clip ammo
	return bot->m_iClip1;
}

/* native int RCBot2_HLDM_GetWeaponClip2(int client); */
cell_t sm_RCBotHLDM_GetWeaponClip2(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	CHLDMBot* bot = GetHLDMBot(client, pContext);
	if (!bot) {
		return -1;
	}

	// Return secondary clip ammo
	return bot->m_iClip2;
}

/* native bool RCBot2_HLDM_HasGravityGun(int client); */
cell_t sm_RCBotHLDM_HasGravityGun(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	CHLDMBot* bot = GetHLDMBot(client, pContext);
	if (!bot) {
		return 0;
	}

	// Check if bot has gravity gun weapon
	const CWeapon* pGravGun = CWeapons::getWeapon(HL2DM_WEAPON_GRAVITYGUN);
	if (pGravGun) {
		const CBotWeapon* pBotWeapon = bot->m_pWeapons->getWeapon(pGravGun);
		return (pBotWeapon && pBotWeapon->hasWeapon()) ? 1 : 0;
	}

	return 0;
}

//=============================================================================
// HL2DM Combat Natives
//=============================================================================

/* native bool RCBot2_HLDM_ThrowGrenade(int client); */
cell_t sm_RCBotHLDM_ThrowGrenade(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	CHLDMBot* bot = GetHLDMBot(client, pContext);
	if (!bot) {
		return 0;
	}

	// Check if bot has grenades
	const CWeapon* pGrenade = CWeapons::getWeapon(HL2DM_WEAPON_FRAG);
	if (!pGrenade) {
		return 0;
	}

	const CBotWeapon* pBotGrenade = bot->m_pWeapons->getWeapon(pGrenade);
	if (!pBotGrenade || pBotGrenade->getAmmo(bot) <= 0) {
		return 0;
	}

	// Execute throw grenade utility
	bot->executeAction(BOT_UTIL_THROW_GRENADE);
	return 1;
}

/* native bool RCBot2_HLDM_CanThrowGrenade(int client); */
cell_t sm_RCBotHLDM_CanThrowGrenade(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];

	CHLDMBot* bot = GetHLDMBot(client, pContext);
	if (!bot) {
		return 0;
	}

	// Check if bot has grenades available
	const CWeapon* pGrenade = CWeapons::getWeapon(HL2DM_WEAPON_FRAG);
	if (!pGrenade) {
		return 0;
	}

	const CBotWeapon* pBotGrenade = bot->m_pWeapons->getWeapon(pGrenade);
	return (pBotGrenade && pBotGrenade->getAmmo(bot) > 0) ? 1 : 0;
}
