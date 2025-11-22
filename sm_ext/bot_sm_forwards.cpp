// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

/**
 * RCBot2 SourceMod Event Forwards Implementation
 * Phase 4: Event System & Callbacks
 */

#include "bot_sm_forwards.h"
#include "bot_sm_ext.h"

namespace RCBotForwards {
	// Global forward handles
	IForward* g_pOnBotSpawn = nullptr;
	IForward* g_pOnBotDeath = nullptr;
	IForward* g_pOnBotKill = nullptr;
	IForward* g_pOnBotThink = nullptr;
	IForward* g_pOnBotEnemyFound = nullptr;
	IForward* g_pOnBotEnemyLost = nullptr;
	IForward* g_pOnBotDamaged = nullptr;
	IForward* g_pOnBotTaskChange = nullptr;

	// TF2-specific forward handles
	IForward* g_pOnBotBuildingPlaced = nullptr;
	IForward* g_pOnBotBuildingDestroyed = nullptr;
	IForward* g_pOnBotUberDeployed = nullptr;
	IForward* g_pOnBotClassChanged = nullptr;

	void CreateForwards() {
		// Create core bot event forwards
		// forward void RCBot2_OnBotSpawn(int client);
		g_pOnBotSpawn = forwards->CreateForward("RCBot2_OnBotSpawn", ET_Ignore, 1, nullptr, Param_Cell);

		// forward void RCBot2_OnBotDeath(int client, int attacker);
		g_pOnBotDeath = forwards->CreateForward("RCBot2_OnBotDeath", ET_Ignore, 2, nullptr, Param_Cell, Param_Cell);

		// forward void RCBot2_OnBotKill(int client, int victim);
		g_pOnBotKill = forwards->CreateForward("RCBot2_OnBotKill", ET_Ignore, 2, nullptr, Param_Cell, Param_Cell);

		// forward void RCBot2_OnBotThink(int client);
		g_pOnBotThink = forwards->CreateForward("RCBot2_OnBotThink", ET_Ignore, 1, nullptr, Param_Cell);

		// forward void RCBot2_OnBotEnemyFound(int client, int enemy);
		g_pOnBotEnemyFound = forwards->CreateForward("RCBot2_OnBotEnemyFound", ET_Ignore, 2, nullptr, Param_Cell, Param_Cell);

		// forward void RCBot2_OnBotEnemyLost(int client, int enemy);
		g_pOnBotEnemyLost = forwards->CreateForward("RCBot2_OnBotEnemyLost", ET_Ignore, 2, nullptr, Param_Cell, Param_Cell);

		// forward void RCBot2_OnBotDamaged(int client, int attacker, int damage);
		g_pOnBotDamaged = forwards->CreateForward("RCBot2_OnBotDamaged", ET_Ignore, 3, nullptr, Param_Cell, Param_Cell, Param_Cell);

		// forward void RCBot2_OnBotTaskChange(int client, int oldTask, int newTask);
		g_pOnBotTaskChange = forwards->CreateForward("RCBot2_OnBotTaskChange", ET_Ignore, 3, nullptr, Param_Cell, Param_Cell, Param_Cell);

		// Create TF2-specific event forwards
		// forward void RCBot2_TF2_OnBotBuildingPlaced(int client, int buildingType, int entity);
		g_pOnBotBuildingPlaced = forwards->CreateForward("RCBot2_TF2_OnBotBuildingPlaced", ET_Ignore, 3, nullptr, Param_Cell, Param_Cell, Param_Cell);

		// forward void RCBot2_TF2_OnBotBuildingDestroyed(int client, int buildingType, int attacker);
		g_pOnBotBuildingDestroyed = forwards->CreateForward("RCBot2_TF2_OnBotBuildingDestroyed", ET_Ignore, 3, nullptr, Param_Cell, Param_Cell, Param_Cell);

		// forward void RCBot2_TF2_OnBotUberDeployed(int client, int target);
		g_pOnBotUberDeployed = forwards->CreateForward("RCBot2_TF2_OnBotUberDeployed", ET_Ignore, 2, nullptr, Param_Cell, Param_Cell);

		// forward void RCBot2_TF2_OnBotClassChanged(int client, int oldClass, int newClass);
		g_pOnBotClassChanged = forwards->CreateForward("RCBot2_TF2_OnBotClassChanged", ET_Ignore, 3, nullptr, Param_Cell, Param_Cell, Param_Cell);
	}

	void DestroyForwards() {
		if (g_pOnBotSpawn) {
			forwards->ReleaseForward(g_pOnBotSpawn);
			g_pOnBotSpawn = nullptr;
		}
		if (g_pOnBotDeath) {
			forwards->ReleaseForward(g_pOnBotDeath);
			g_pOnBotDeath = nullptr;
		}
		if (g_pOnBotKill) {
			forwards->ReleaseForward(g_pOnBotKill);
			g_pOnBotKill = nullptr;
		}
		if (g_pOnBotThink) {
			forwards->ReleaseForward(g_pOnBotThink);
			g_pOnBotThink = nullptr;
		}
		if (g_pOnBotEnemyFound) {
			forwards->ReleaseForward(g_pOnBotEnemyFound);
			g_pOnBotEnemyFound = nullptr;
		}
		if (g_pOnBotEnemyLost) {
			forwards->ReleaseForward(g_pOnBotEnemyLost);
			g_pOnBotEnemyLost = nullptr;
		}
		if (g_pOnBotDamaged) {
			forwards->ReleaseForward(g_pOnBotDamaged);
			g_pOnBotDamaged = nullptr;
		}
		if (g_pOnBotTaskChange) {
			forwards->ReleaseForward(g_pOnBotTaskChange);
			g_pOnBotTaskChange = nullptr;
		}

		// Destroy TF2-specific forwards
		if (g_pOnBotBuildingPlaced) {
			forwards->ReleaseForward(g_pOnBotBuildingPlaced);
			g_pOnBotBuildingPlaced = nullptr;
		}
		if (g_pOnBotBuildingDestroyed) {
			forwards->ReleaseForward(g_pOnBotBuildingDestroyed);
			g_pOnBotBuildingDestroyed = nullptr;
		}
		if (g_pOnBotUberDeployed) {
			forwards->ReleaseForward(g_pOnBotUberDeployed);
			g_pOnBotUberDeployed = nullptr;
		}
		if (g_pOnBotClassChanged) {
			forwards->ReleaseForward(g_pOnBotClassChanged);
			g_pOnBotClassChanged = nullptr;
		}
	}

	void OnBotSpawn(const int client) {
		if (g_pOnBotSpawn) {
			g_pOnBotSpawn->PushCell(client);
			g_pOnBotSpawn->Execute(nullptr);
		}
	}

	void OnBotDeath(const int client, const int attacker) {
		if (g_pOnBotDeath) {
			g_pOnBotDeath->PushCell(client);
			g_pOnBotDeath->PushCell(attacker);
			g_pOnBotDeath->Execute(nullptr);
		}
	}

	void OnBotKill(const int client, const int victim) {
		if (g_pOnBotKill) {
			g_pOnBotKill->PushCell(client);
			g_pOnBotKill->PushCell(victim);
			g_pOnBotKill->Execute(nullptr);
		}
	}

	void OnBotThink(const int client) {
		if (g_pOnBotThink) {
			g_pOnBotThink->PushCell(client);
			g_pOnBotThink->Execute(nullptr);
		}
	}

	void OnBotEnemyFound(const int client, const int enemy) {
		if (g_pOnBotEnemyFound) {
			g_pOnBotEnemyFound->PushCell(client);
			g_pOnBotEnemyFound->PushCell(enemy);
			g_pOnBotEnemyFound->Execute(nullptr);
		}
	}

	void OnBotEnemyLost(const int client, const int enemy) {
		if (g_pOnBotEnemyLost) {
			g_pOnBotEnemyLost->PushCell(client);
			g_pOnBotEnemyLost->PushCell(enemy);
			g_pOnBotEnemyLost->Execute(nullptr);
		}
	}

	void OnBotDamaged(const int client, const int attacker, const int damage) {
		if (g_pOnBotDamaged) {
			g_pOnBotDamaged->PushCell(client);
			g_pOnBotDamaged->PushCell(attacker);
			g_pOnBotDamaged->PushCell(damage);
			g_pOnBotDamaged->Execute(nullptr);
		}
	}

	void OnBotTaskChange(const int client, const int oldTask, const int newTask) {
		if (g_pOnBotTaskChange) {
			g_pOnBotTaskChange->PushCell(client);
			g_pOnBotTaskChange->PushCell(oldTask);
			g_pOnBotTaskChange->PushCell(newTask);
			g_pOnBotTaskChange->Execute(nullptr);
		}
	}
}

	//=============================================================================
	// TF2-Specific Forward Firing Functions
	//=============================================================================

	void OnBotBuildingPlaced(const int client, const int buildingType, const int entity) {
		if (g_pOnBotBuildingPlaced) {
			g_pOnBotBuildingPlaced->PushCell(client);
			g_pOnBotBuildingPlaced->PushCell(buildingType);
			g_pOnBotBuildingPlaced->PushCell(entity);
			g_pOnBotBuildingPlaced->Execute(nullptr);
		}
	}

	void OnBotBuildingDestroyed(const int client, const int buildingType, const int attacker) {
		if (g_pOnBotBuildingDestroyed) {
			g_pOnBotBuildingDestroyed->PushCell(client);
			g_pOnBotBuildingDestroyed->PushCell(buildingType);
			g_pOnBotBuildingDestroyed->PushCell(attacker);
			g_pOnBotBuildingDestroyed->Execute(nullptr);
		}
	}

	void OnBotUberDeployed(const int client, const int target) {
		if (g_pOnBotUberDeployed) {
			g_pOnBotUberDeployed->PushCell(client);
			g_pOnBotUberDeployed->PushCell(target);
			g_pOnBotUberDeployed->Execute(nullptr);
		}
	}

	void OnBotClassChanged(const int client, const int oldClass, const int newClass) {
		if (g_pOnBotClassChanged) {
			g_pOnBotClassChanged->PushCell(client);
			g_pOnBotClassChanged->PushCell(oldClass);
			g_pOnBotClassChanged->PushCell(newClass);
			g_pOnBotClassChanged->Execute(nullptr);
		}
	}
}
