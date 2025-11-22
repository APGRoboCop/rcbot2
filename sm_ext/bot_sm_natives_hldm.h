#pragma once

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

#include <IExtensionSys.h>

using namespace SourceMod;
using namespace SourcePawn;

// Sprint Natives
cell_t sm_RCBotHLDM_UseSprint(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotHLDM_IsSprinting(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotHLDM_GetSprintTime(IPluginContext *pContext, const cell_t *params);

// Equipment Natives
cell_t sm_RCBotHLDM_UseCharger(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotHLDM_UseHealthCharger(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotHLDM_GetArmorPercent(IPluginContext *pContext, const cell_t *params);

// Gravity Gun Natives
cell_t sm_RCBotHLDM_IsCarryingObject(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotHLDM_GetCarriedObject(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotHLDM_PickupObject(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotHLDM_DropObject(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotHLDM_LaunchObject(IPluginContext *pContext, const cell_t *params);

// Physics & Breakables
cell_t sm_RCBotHLDM_GetNearestPhysicsObject(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotHLDM_GetNearestBreakable(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotHLDM_SetFailedObject(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotHLDM_GetFailedObject(IPluginContext *pContext, const cell_t *params);

// Item/Pickup Natives
cell_t sm_RCBotHLDM_GetNearestWeapon(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotHLDM_GetNearestHealthKit(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotHLDM_GetNearestBattery(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotHLDM_GetNearestAmmoCrate(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotHLDM_UseAmmoCrate(IPluginContext *pContext, const cell_t *params);

// Weapon Management
cell_t sm_RCBotHLDM_GetCurrentWeapon(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotHLDM_GetWeaponClip1(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotHLDM_GetWeaponClip2(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotHLDM_HasGravityGun(IPluginContext *pContext, const cell_t *params);

// Combat Natives
cell_t sm_RCBotHLDM_ThrowGrenade(IPluginContext *pContext, const cell_t *params);
cell_t sm_RCBotHLDM_CanThrowGrenade(IPluginContext *pContext, const cell_t *params);

const sp_nativeinfo_t g_RCBotNatives_HLDM[] = {
	// Sprint
	{ "RCBot2_HLDM_UseSprint", sm_RCBotHLDM_UseSprint },
	{ "RCBot2_HLDM_IsSprinting", sm_RCBotHLDM_IsSprinting },
	{ "RCBot2_HLDM_GetSprintTime", sm_RCBotHLDM_GetSprintTime },

	// Equipment
	{ "RCBot2_HLDM_UseCharger", sm_RCBotHLDM_UseCharger },
	{ "RCBot2_HLDM_UseHealthCharger", sm_RCBotHLDM_UseHealthCharger },
	{ "RCBot2_HLDM_GetArmorPercent", sm_RCBotHLDM_GetArmorPercent },

	// Gravity Gun
	{ "RCBot2_HLDM_IsCarryingObject", sm_RCBotHLDM_IsCarryingObject },
	{ "RCBot2_HLDM_GetCarriedObject", sm_RCBotHLDM_GetCarriedObject },
	{ "RCBot2_HLDM_PickupObject", sm_RCBotHLDM_PickupObject },
	{ "RCBot2_HLDM_DropObject", sm_RCBotHLDM_DropObject },
	{ "RCBot2_HLDM_LaunchObject", sm_RCBotHLDM_LaunchObject },

	// Physics & Breakables
	{ "RCBot2_HLDM_GetNearestPhysicsObject", sm_RCBotHLDM_GetNearestPhysicsObject },
	{ "RCBot2_HLDM_GetNearestBreakable", sm_RCBotHLDM_GetNearestBreakable },
	{ "RCBot2_HLDM_SetFailedObject", sm_RCBotHLDM_SetFailedObject },
	{ "RCBot2_HLDM_GetFailedObject", sm_RCBotHLDM_GetFailedObject },

	// Items/Pickups
	{ "RCBot2_HLDM_GetNearestWeapon", sm_RCBotHLDM_GetNearestWeapon },
	{ "RCBot2_HLDM_GetNearestHealthKit", sm_RCBotHLDM_GetNearestHealthKit },
	{ "RCBot2_HLDM_GetNearestBattery", sm_RCBotHLDM_GetNearestBattery },
	{ "RCBot2_HLDM_GetNearestAmmoCrate", sm_RCBotHLDM_GetNearestAmmoCrate },
	{ "RCBot2_HLDM_UseAmmoCrate", sm_RCBotHLDM_UseAmmoCrate },

	// Weapons
	{ "RCBot2_HLDM_GetCurrentWeapon", sm_RCBotHLDM_GetCurrentWeapon },
	{ "RCBot2_HLDM_GetWeaponClip1", sm_RCBotHLDM_GetWeaponClip1 },
	{ "RCBot2_HLDM_GetWeaponClip2", sm_RCBotHLDM_GetWeaponClip2 },
	{ "RCBot2_HLDM_HasGravityGun", sm_RCBotHLDM_HasGravityGun },

	// Combat
	{ "RCBot2_HLDM_ThrowGrenade", sm_RCBotHLDM_ThrowGrenade },
	{ "RCBot2_HLDM_CanThrowGrenade", sm_RCBotHLDM_CanThrowGrenade },

	{ nullptr, nullptr }
};
