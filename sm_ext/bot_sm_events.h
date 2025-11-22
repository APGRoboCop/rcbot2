#pragma once

/**
 * RCBot2 SourceMod Event Integration
 *
 * This header provides functions that the RCBot2 core can call to trigger
 * SourceMod event forwards. Include this in bot.cpp and other bot files
 * to enable event notifications.
 */

#include <edict.h>

// Forward declarations - only include this header when building with SourceMod support
#ifdef ENABLE_SOURCEMOD_INTEGRATION

namespace RCBotEvents {
	/**
	 * Call when a bot spawns
	 * @param pBot Bot edict pointer
	 */
	void OnBotSpawn(edict_t* pBot);

	/**
	 * Call when a bot dies
	 * @param pBot Bot edict pointer
	 * @param pAttacker Attacker edict pointer (can be null)
	 */
	void OnBotDeath(edict_t* pBot, edict_t* pAttacker);

	/**
	 * Call when a bot kills someone
	 * @param pBot Bot edict pointer
	 * @param pVictim Victim edict pointer
	 */
	void OnBotKill(edict_t* pBot, edict_t* pVictim);

	/**
	 * Call during bot think cycle
	 * @param pBot Bot edict pointer
	 */
	void OnBotThink(edict_t* pBot);

	/**
	 * Call when bot acquires new enemy
	 * @param pBot Bot edict pointer
	 * @param pEnemy Enemy edict pointer
	 */
	void OnBotEnemyFound(edict_t* pBot, edict_t* pEnemy);

	/**
	 * Call when bot loses enemy
	 * @param pBot Bot edict pointer
	 * @param pEnemy Enemy edict pointer
	 */
	void OnBotEnemyLost(edict_t* pBot, edict_t* pEnemy);

	/**
	 * Call when bot takes damage
	 * @param pBot Bot edict pointer
	 * @param pAttacker Attacker edict pointer (can be null)
	 * @param damage Damage amount
	 */
	void OnBotDamaged(edict_t* pBot, edict_t* pAttacker, int damage);

	/**
	 * Call when bot task/schedule changes
	 * @param pBot Bot edict pointer
	 * @param oldTask Old schedule ID
	 * @param newTask New schedule ID
	 */
	void OnBotTaskChange(edict_t* pBot, int oldTask, int newTask);

	// HL2DM-specific event namespace
	namespace HLDM {
		/**
		 * Call when bot picks up a weapon
		 * @param pBot Bot edict pointer
		 * @param pWeapon Weapon edict pointer
		 */
		void OnBotWeaponPickup(edict_t* pBot, edict_t* pWeapon);

		/**
		 * Call when bot picks up object with Gravity Gun
		 * @param pBot Bot edict pointer
		 * @param pObject Physics object edict pointer
		 */
		void OnBotGravityGunPickup(edict_t* pBot, edict_t* pObject);

		/**
		 * Call when bot launches object with Gravity Gun
		 * @param pBot Bot edict pointer
		 * @param pObject Physics object edict pointer
		 */
		void OnBotGravityGunLaunch(edict_t* pBot, edict_t* pObject);

		/**
		 * Call when bot drops object from Gravity Gun
		 * @param pBot Bot edict pointer
		 * @param pObject Physics object edict pointer
		 */
		void OnBotGravityGunDrop(edict_t* pBot, edict_t* pObject);

		/**
		 * Call when bot uses a charger (health or suit)
		 * @param pBot Bot edict pointer
		 * @param chargerType 0 = health, 1 = suit/armor
		 * @param pCharger Charger edict pointer
		 */
		void OnBotSuitChargeUsed(edict_t* pBot, int chargerType, edict_t* pCharger);
	}
}

#else

// Stub implementations when SourceMod integration is disabled
namespace RCBotEvents {
	inline void OnBotSpawn(edict_t*) {}
	inline void OnBotDeath(edict_t*, edict_t*) {}
	inline void OnBotKill(edict_t*, edict_t*) {}
	inline void OnBotThink(edict_t*) {}
	inline void OnBotEnemyFound(edict_t*, edict_t*) {}
	inline void OnBotEnemyLost(edict_t*, edict_t*) {}
	inline void OnBotDamaged(edict_t*, edict_t*, int) {}
	inline void OnBotTaskChange(edict_t*, int, int) {}

	namespace HLDM {
		inline void OnBotWeaponPickup(edict_t*, edict_t*) {}
		inline void OnBotGravityGunPickup(edict_t*, edict_t*) {}
		inline void OnBotGravityGunLaunch(edict_t*, edict_t*) {}
		inline void OnBotGravityGunDrop(edict_t*, edict_t*) {}
		inline void OnBotSuitChargeUsed(edict_t*, int, edict_t*) {}
	}
}

#endif
