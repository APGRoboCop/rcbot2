#pragma once

#include <IExtensionSys.h>

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
}
