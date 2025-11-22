// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

/**
 * RCBot2 SourceMod Event Integration Implementation
 *
 * Bridges bot events from the core bot code to SourceMod forwards
 */

#include "bot_sm_events.h"
#include "bot_sm_forwards.h"
#include <eiface.h>

extern IVEngineServer* engine;

namespace RCBotEvents {
	// Helper function to convert edict_t* to client index
	static int EdictToClientIndex(edict_t* pEdict) {
		if (!pEdict || pEdict->IsFree()) {
			return 0;
		}
		return engine->IndexOfEdict(pEdict);
	}

	void OnBotSpawn(edict_t* pBot) {
		if (!pBot) return;
		const int client = EdictToClientIndex(pBot);
		if (client > 0) {
			RCBotForwards::OnBotSpawn(client);
		}
	}

	void OnBotDeath(edict_t* pBot, edict_t* pAttacker) {
		if (!pBot) return;
		const int client = EdictToClientIndex(pBot);
		const int attacker = pAttacker ? EdictToClientIndex(pAttacker) : 0;
		if (client > 0) {
			RCBotForwards::OnBotDeath(client, attacker);
		}
	}

	void OnBotKill(edict_t* pBot, edict_t* pVictim) {
		if (!pBot || !pVictim) return;
		const int client = EdictToClientIndex(pBot);
		const int victim = EdictToClientIndex(pVictim);
		if (client > 0 && victim > 0) {
			RCBotForwards::OnBotKill(client, victim);
		}
	}

	void OnBotThink(edict_t* pBot) {
		if (!pBot) return;
		const int client = EdictToClientIndex(pBot);
		if (client > 0) {
			RCBotForwards::OnBotThink(client);
		}
	}

	void OnBotEnemyFound(edict_t* pBot, edict_t* pEnemy) {
		if (!pBot || !pEnemy) return;
		const int client = EdictToClientIndex(pBot);
		const int enemy = EdictToClientIndex(pEnemy);
		if (client > 0 && enemy > 0) {
			RCBotForwards::OnBotEnemyFound(client, enemy);
		}
	}

	void OnBotEnemyLost(edict_t* pBot, edict_t* pEnemy) {
		if (!pBot || !pEnemy) return;
		const int client = EdictToClientIndex(pBot);
		const int enemy = EdictToClientIndex(pEnemy);
		if (client > 0 && enemy > 0) {
			RCBotForwards::OnBotEnemyLost(client, enemy);
		}
	}

	void OnBotDamaged(edict_t* pBot, edict_t* pAttacker, const int damage) {
		if (!pBot) return;
		const int client = EdictToClientIndex(pBot);
		const int attacker = pAttacker ? EdictToClientIndex(pAttacker) : 0;
		if (client > 0) {
			RCBotForwards::OnBotDamaged(client, attacker, damage);
		}
	}

	void OnBotTaskChange(edict_t* pBot, const int oldTask, const int newTask) {
		if (!pBot) return;
		const int client = EdictToClientIndex(pBot);
		if (client > 0) {
			RCBotForwards::OnBotTaskChange(client, oldTask, newTask);
		}
	}
}
