#pragma once

#include <IExtensionSys.h>
#include <IForwardSys.h>

using namespace SourceMod;

//=============================================================================
// Phase 4: Event System - Global Forward Declarations
//=============================================================================

namespace RCBotForwards {
	// Core bot event forwards
	extern IForward* g_pOnBotSpawn;
	extern IForward* g_pOnBotDeath;
	extern IForward* g_pOnBotKill;
	extern IForward* g_pOnBotThink;
	extern IForward* g_pOnBotEnemyFound;
	extern IForward* g_pOnBotEnemyLost;
	extern IForward* g_pOnBotDamaged;
	extern IForward* g_pOnBotTaskChange;

	// TF2-specific event forwards
	extern IForward* g_pOnBotBuildingPlaced;
	extern IForward* g_pOnBotBuildingDestroyed;
	extern IForward* g_pOnBotUberDeployed;
	extern IForward* g_pOnBotClassChanged;

	// HL2DM-specific event forwards
	extern IForward* g_pOnBotWeaponPickup;
	extern IForward* g_pOnBotGravityGunPickup;
	extern IForward* g_pOnBotGravityGunLaunch;
	extern IForward* g_pOnBotGravityGunDrop;
	extern IForward* g_pOnBotSuitChargeUsed;

	// Forward initialization and cleanup
	void CreateForwards();
	void DestroyForwards();

	// Forward firing functions
	void OnBotSpawn(int client);
	void OnBotDeath(int client, int attacker);
	void OnBotKill(int client, int victim);
	void OnBotThink(int client);
	void OnBotEnemyFound(int client, int enemy);
	void OnBotEnemyLost(int client, int enemy);
	void OnBotDamaged(int client, int attacker, int damage);
	void OnBotTaskChange(int client, int oldTask, int newTask);

	// TF2-specific forward firing functions
	void OnBotBuildingPlaced(int client, int buildingType, int entity);
	void OnBotBuildingDestroyed(int client, int buildingType, int attacker);
	void OnBotUberDeployed(int client, int target);
	void OnBotClassChanged(int client, int oldClass, int newClass);

	// HL2DM-specific forward firing functions
	void OnBotWeaponPickup(int client, int weapon);
	void OnBotGravityGunPickup(int client, int object);
	void OnBotGravityGunLaunch(int client, int object);
	void OnBotGravityGunDrop(int client, int object);
	void OnBotSuitChargeUsed(int client, int chargerType, int chargerEntity);
}
