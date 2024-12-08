/*
 *    This file is part of RCBot.
 *
 *    RCBot by Paul Murphy adapted from Botman's HPB Bot 2 template.
 *
 *    RCBot is free software; you can redistribute it and/or modify it
 *    under the terms of the GNU General Public License as published by the
 *    Free Software Foundation; either version 2 of the License, or (at
 *    your option) any later version.
 *
 *    RCBot is distributed in the hope that it will be useful, but
 *    WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with RCBot; if not, write to the Free Software Foundation,
 *    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *    In addition, as a special exception, the author gives permission to
 *    link the code of this program with the Half-Life Game Engine ("HL
 *    Engine") and Modified Game Libraries ("MODs") developed by Valve,
 *    L.L.C ("Valve").  You must obey the GNU General Public License in all
 *    respects for all of the code used other than the HL Engine and MODs
 *    from Valve.  If you modify this file, you may extend this exception
 *    to your version of the file, but you are not obligated to do so.  If
 *    you do not wish to do so, delete this exception statement from your
 *    version.
 *
 */
#include "engine_wrappers.h"

#include "bot.h"
#include "bot_cvars.h"
#include "ndebugoverlay.h"
#include "bot_squads.h"
#include "bot_synergy.h"
#include "in_buttons.h"
#include "bot_buttons.h"
#include "bot_globals.h"
#include "bot_profile.h"
#include "bot_getprop.h"
#include "bot_mtrand.h"
#include "bot_mods.h"
#include "bot_task.h"
#include "bot_schedule.h"
#include "bot_weapons.h"
#include "bot_waypoint.h"
#include "bot_waypoint_locations.h"
#include "bot_navigator.h"
#include "bot_perceptron.h"
#include "bot_plugin_meta.h"
#include "bot_waypoint_visibility.h"

#include <cstring>

extern IVDebugOverlay* debugoverlay;
extern IServerGameEnts* servergameents; // for accessing the server game entities

void CBotSynergy::init(bool bVarInit)
{
	CBot::init(bVarInit); // call base first
	m_fFov = 110.0f; // Coop mod, give bot larger FOV (default is 75)
}

void CBotSynergy::spawnInit()
{
	CBot::spawnInit();

	if (m_pWeapons) // reset weapons
		m_pWeapons->clearWeapons();

	m_CurrentUtil = BOT_UTIL_MAX;
	m_pNearbyAmmo = nullptr;
	m_pNearbyBattery = nullptr;
	m_pNearbyCrate = nullptr;
	m_pNearbyHealthKit = nullptr;
	m_pNearbyWeapon = nullptr;
	m_pNearbyMine = nullptr;
	m_pNearbyGrenade = nullptr;
	m_pNearbyItemCrate = nullptr;
	m_pCurrentWeapon = nullptr;
	m_flNextSprintTime = engine->Time();
	m_flSuitPower = 0.0f;
	m_flUseCrateTime = engine->Time();
	m_flPickUpTime = engine->Time();
}

void CBotSynergy::died(edict_t* pKiller, const char* pszWeapon)
{
	CBot::died(pKiller, pszWeapon);

	if (pKiller)
	{
		if (CBotGlobals::entityIsValid(pKiller))
		{
			m_pNavigator->belief(CBotGlobals::entityOrigin(pKiller), getEyePosition(), bot_beliefmulti.GetFloat(), distanceFrom(pKiller), BELIEF_DANGER);
		}
	}
}

/**
 * Determines if the bot needs health
 *
 * @return          True if the bot needs health
 **/
bool CBotSynergy::needHealth()
{
	return getHealthPercent() <= 0.7f;
}

/**
 * Determines if the bot needs ammo
 *
 * @return          True if the bot needs ammo
 **/
bool CBotSynergy::needAmmo()
{
	if (m_pCurrentWeapon == nullptr)
	{
		return false;
	}

	const CBotWeapon* weapon = m_pWeapons->getWeapon(CWeapons::getWeapon(m_pCurrentWeapon->GetClassName()));
	if (weapon)
	{
		const int iAmmo = weapon->getAmmo(this); // Current weapon reserve ammo

		switch (weapon->getID())
		{
		case SYN_WEAPON_PISTOL:
		{
			return iAmmo < 30;
			//break;
		}
		case SYN_WEAPON_SHOTGUN:
		case SYN_WEAPON_DESERTEAGLE:
		case SYN_WEAPON_357:
		{
			return iAmmo < 6;
			//break;
		}
		case SYN_WEAPON_SMG1:
		case SYN_WEAPON_MP5K:
		{
			return iAmmo < 75;
			//break;
		}
		case SYN_WEAPON_AR2:
		case SYN_WEAPON_MG1:
		{
			return iAmmo < 20;
			//break;
		}
		case SYN_WEAPON_CROSSBOW:
		{
			return iAmmo < 2;
			//break;
		}
		case SYN_WEAPON_RPG:
		{
			return iAmmo < 2;
			//break;
		}

		default:
		{
			return false;
			//break;
		}
		} //Not required? [APG]RoboCop[CL]
	}

	return false;
}

void CBotSynergy::modThink()
{
	m_fIdealMoveSpeed = CClassInterface::getMaxSpeed(m_pEdict);
	m_pCurrentWeapon = CClassInterface::getCurrentWeapon(m_pEdict);
	m_flSuitPower = CClassInterface::getSynPlayerSuitPower(m_pEdict);

	if (needHealth())
		updateCondition(CONDITION_NEED_HEALTH);
	else
		removeCondition(CONDITION_NEED_HEALTH);

	if (needAmmo())
		updateCondition(CONDITION_NEED_AMMO);
	else
		removeCondition(CONDITION_NEED_AMMO);

	if (onLadder())
	{
		setMoveLookPriority(MOVELOOK_OVERRIDE);
		setLookAtTask(LOOK_WAYPOINT);
		m_pButtons->holdButton(IN_FORWARD, 0, 1, 0);
		setMoveLookPriority(MOVELOOK_MODTHINK);
	}

	if (m_pNearbyGrenade && distanceFrom(m_pNearbyGrenade.get()) <= 200.0f) // Nearby grenade, RUN for cover!
	{
		updateCondition(CONDITION_RUN);
		if (!m_pSchedules->isCurrentSchedule(SCHED_GOOD_HIDE_SPOT))
		{
			m_pSchedules->removeSchedule(SCHED_GOOD_HIDE_SPOT);
			m_pSchedules->addFront(new CGotoHideSpotSched(this, m_pNearbyGrenade, false)); // bIsGrenade is false because when true the bot will do a DoD specific task
			debugMsg(BOT_DEBUG_THINK, "[MOD THINK] Taking cover from grenade");
		}
	}

	if (m_pNearbyMine && distanceFrom(m_pNearbyMine.get()) <= 512.0f && !CSynergyMod::IsCombineMineDisarmed(m_pNearbyMine.get()))
	{
		if (CSynergyMod::IsCombineMinePlayerPlaced(m_pNearbyMine.get()))
		{
			m_pNearbyMine = nullptr; // The mine is friendly now.
		}
		else
		{
			if (m_pWeapons->hasWeapon(SYN_WEAPON_PHYSCANNON) && !CSynergyMod::IsCombineMineHeldByPhysgun(m_pNearbyMine.get()))
			{
				if (!m_pSchedules->isCurrentSchedule(SCHED_SYN_DISARM_MINE))
				{
					m_pSchedules->removeSchedule(SCHED_SYN_DISARM_MINE);
					m_pSchedules->addFront(new CSynDisarmMineSched(m_pNearbyMine.get()));
					debugMsg(BOT_DEBUG_THINK, "[MOD THINK] Disarming combine mine");
				}
			}
			else
			{
				m_pNavigator->belief(CBotGlobals::entityOrigin(m_pNearbyMine.get()), getEyePosition(), MAX_BELIEF, distanceFrom(m_pNearbyMine.get()), BELIEF_DANGER);
			}
		}
	}

	// Pick nearby weapons that the bot doesn't already have
	if (m_pNearbyWeapon && distanceFrom(m_pNearbyWeapon.get()) <= 400.0f && m_flPickUpTime <= engine->Time())
	{
		const edict_t* pOwner = CClassInterface::getOwner(m_pNearbyWeapon);

		if (pOwner != nullptr) // Someone already owns this weapon
		{
			m_pNearbyWeapon = nullptr;
		}
		else
		{
			if (!m_pSchedules->isCurrentSchedule(SCHED_PICKUP))
			{
				m_pSchedules->removeSchedule(SCHED_PICKUP);
				m_pSchedules->addFront(new CBotPickupSched(m_pNearbyWeapon.get()));
				debugMsg(BOT_DEBUG_THINK, "[MOD THINK] Picking up weapon");
				m_flPickUpTime = engine->Time() + randomFloat(5.0f, 10.0f);
			}
		}
	}

	// Checks for nearby item boxes and try to break them
	if (m_pNearbyItemCrate && distanceFrom(m_pNearbyItemCrate.get()) <= 400.0f && m_flPickUpTime <= engine->Time())
	{
		if (!m_pSchedules->isCurrentSchedule(SCHED_SYN_BREAK_ICRATE))
		{
			CBotWeapon* pWeapon = nullptr;

			if (m_pWeapons->hasWeapon(SYN_WEAPON_PHYSCANNON))
			{
				pWeapon = m_pWeapons->getWeapon(CWeapons::getWeapon(SYN_WEAPON_PHYSCANNON));
			}
			else if (m_pWeapons->hasWeapon(SYN_WEAPON_CROWBAR))
			{
				pWeapon = m_pWeapons->getWeapon(CWeapons::getWeapon(SYN_WEAPON_CROWBAR));
			}
			else if (m_pWeapons->hasWeapon(SYN_WEAPON_STUNSTICK))
			{
				pWeapon = m_pWeapons->getWeapon(CWeapons::getWeapon(SYN_WEAPON_STUNSTICK));
			}
			else if (m_pWeapons->hasWeapon(SYN_WEAPON_LEADPIPE))
			{
				pWeapon = m_pWeapons->getWeapon(CWeapons::getWeapon(SYN_WEAPON_LEADPIPE));
			}
			else if (m_pWeapons->hasWeapon(SYN_WEAPON_SHOTGUN))
			{
				pWeapon = m_pWeapons->getWeapon(CWeapons::getWeapon(SYN_WEAPON_SHOTGUN));
			}
			else if (m_pWeapons->hasWeapon(SYN_WEAPON_SMG1))
			{
				pWeapon = m_pWeapons->getWeapon(CWeapons::getWeapon(SYN_WEAPON_SMG1));
			}
			else if (m_pWeapons->hasWeapon(SYN_WEAPON_PISTOL))
			{
				pWeapon = m_pWeapons->getWeapon(CWeapons::getWeapon(SYN_WEAPON_PISTOL));
			}

			m_pSchedules->removeSchedule(SCHED_SYN_BREAK_ICRATE);
			m_pSchedules->addFront(new CSynBreakICrateSched(m_pNearbyItemCrate.get(), pWeapon));
			debugMsg(BOT_DEBUG_THINK, "[MOD THINK] Breaking item crate");
			m_flPickUpTime = engine->Time() + randomFloat(5.0f, 10.0f);
		}
	}

	/**
	 * Bot sprinting logic
	 **/
	if (hasSomeConditions(CONDITION_RUN) && m_flSuitPower > 1.0f && m_flNextSprintTime <= engine->Time()) // The bot wants to sprint
	{
		m_pButtons->holdButton(IN_SPEED, 0.0f, 1.0f, 0.0f);
	}
	else if (m_fCurrentDanger >= 75.0f && m_flSuitPower > 1.0f && !isUnderWater()) // dangerous area, sprint
	{
		m_pButtons->holdButton(IN_SPEED, 0.0f, 1.0f, 0.0f);
	}
	else if (m_flSuitPower < 1.0f) // Low on suit power, don't sprint for a while
	{
		m_flNextSprintTime = engine->Time() + randomFloat(8.0f, 10.0f);
		removeCondition(CONDITION_RUN);
	}
	else if (isUnderWater()) // In Synergy/HL2 suit power is also used for oxygen
	{
		m_flNextSprintTime = engine->Time() + 0.5f;
	}
}

void CBotSynergy::updateConditions()
{
	CBot::updateConditions();

	if (m_pEnemy.get() != nullptr)
	{
		if (CDataInterface::GetEntityHealth(m_pEnemy.get()->GetNetworkable()->GetBaseEntity()) <= 0)
		{
			updateCondition(CONDITION_ENEMY_DEAD);
			m_pNavigator->belief(getOrigin(), CBotGlobals::entityOrigin(m_pEnemy), bot_belief_fade.GetFloat(), distanceFrom(m_pEnemy), BELIEF_SAFETY);
			enemyDown(m_pEnemy);
			m_pEnemy = nullptr;
		}
	}
}

bool CBotSynergy::isEnemy(edict_t* pEdict, bool bCheckWeapons)
{
	if (m_pEdict == pEdict) // Not self
		return false;

	if (ENTINDEX(pEdict) <= CBotGlobals::maxClients()) // Coop mod, don't attack players
		return false;

	const char* szclassname = pEdict->GetClassName();

	// BUGBUG!! Maps can override NPC relationship with the ai_relationship entity, making this classname filter useless
	if (std::strncmp(szclassname, "npc_", 4) == 0) // Attack NPCs
	{
		if (std::strcmp(szclassname, "npc_metropolice") == 0 || std::strcmp(szclassname, "npc_combine_s") == 0 || std::strcmp(szclassname, "npc_manhack") == 0 ||
			std::strcmp(szclassname, "npc_zombie") == 0 || std::strcmp(szclassname, "npc_fastzombie") == 0 || std::strcmp(szclassname, "npc_poisonzombie") == 0 || std::strcmp(szclassname, "npc_zombine") == 0 ||
			std::strcmp(szclassname, "npc_antlionguard") == 0 || std::strcmp(szclassname, "npc_antlion") == 0 || std::strcmp(szclassname, "npc_headcrab") == 0 || std::strcmp(szclassname, "npc_headcrab_fast") == 0 ||
			std::strcmp(szclassname, "npc_headcrab_black") == 0 || std::strcmp(szclassname, "npc_hunter") == 0 || std::strcmp(szclassname, "npc_fastzombie_torso") == 0 || std::strcmp(szclassname, "npc_zombie_torso") == 0 ||
			std::strcmp(szclassname, "npc_barnacle") == 0 || (std::strcmp(szclassname, "npc_combinegunship") == 0) || (std::strcmp(szclassname, "npc_helicopter") == 0)
			|| (std::strcmp(szclassname, "npc_strider") == 0) || (std::strcmp(szclassname, "npc_combinedropship") == 0) || (std::strcmp(szclassname, "npc_clawscanner") == 0)
			|| (std::strcmp(szclassname, "npc_combine_camera") == 0) || (std::strcmp(szclassname, "npc_antlion_worker") == 0) || (std::strcmp(szclassname, "npc_cscanner") == 0))
		{
			return true;
		}
	}

	return false;
}

bool CBotSynergy::setVisible(edict_t* pEntity, bool bVisible)
{
	const bool bValid = CBot::setVisible(pEntity, bVisible);

	static float fDist = distanceFrom(pEntity);
	const Vector entityorigin = CBotGlobals::entityOrigin(pEntity);
	const char* szclassname = pEntity->GetClassName();

	// Is valid and NOT invisible
	if (bValid && bVisible && !(CClassInterface::getEffects(pEntity) & EF_NODRAW))
	{
		if (std::strcmp(szclassname, "item_ammo_crate") == 0 && (!m_pNearbyCrate.get() || fDist < distanceFrom(m_pNearbyCrate.get())))
		{
			m_pNearbyCrate = pEntity;
		}
		else if (std::strncmp(szclassname, "item_ammo", 9) == 0 && (!m_pNearbyAmmo.get() || fDist < distanceFrom(m_pNearbyAmmo.get())))
		{
			if (std::strncmp(szclassname, "item_ammo_crate", 15) == 0)
			{
				m_pNearbyAmmo = nullptr; // Invalidate if this entity is an ammo crate
			}
			else if (std::strncmp(szclassname, "item_ammo_pack", 14) == 0) // Ignore these
			{
				m_pNearbyAmmo = nullptr;
			}
			else if (filterAmmo(pEntity, szclassname))
			{
				m_pNearbyAmmo = pEntity;
			}
		}
		else if (std::strncmp(szclassname, "item_box_buckshot", 17) == 0 && (!m_pNearbyAmmo.get() || fDist < distanceFrom(m_pNearbyAmmo.get())))
		{
			if (filterAmmo(pEntity, szclassname))
			{
				m_pNearbyAmmo = pEntity;
			}
		}
		else if (std::strncmp(szclassname, "item_rpg_round", 14) == 0 && (!m_pNearbyAmmo.get() || fDist < distanceFrom(m_pNearbyAmmo.get())))
		{
			if (filterAmmo(pEntity, szclassname))
			{
				m_pNearbyAmmo = pEntity;
			}
		}
		else if (std::strncmp(szclassname, "item_healthkit", 14) == 0 && (!m_pNearbyHealthKit.get() || fDist < distanceFrom(m_pNearbyHealthKit.get())))
		{
			m_pNearbyHealthKit = pEntity;
		}
		else if (std::strncmp(szclassname, "item_battery", 12) == 0 && (!m_pNearbyBattery.get() || fDist < distanceFrom(m_pNearbyBattery.get())))
		{
			m_pNearbyBattery = pEntity;
		}
		else if (std::strncmp(szclassname, "weapon_", 7) == 0 && (!m_pNearbyWeapon.get() || fDist < distanceFrom(m_pNearbyWeapon.get())))
		{
			const CBotWeapon* pWeapon = m_pWeapons->getWeapon(CWeapons::getWeapon(szclassname));
			if (pWeapon && pWeapon->hasWeapon())
			{
				m_pNearbyWeapon = nullptr; // bot already has this weapon
			}
			else
			{
				const edict_t* pOwner = CClassInterface::getOwner(pEntity);
				if (pOwner == nullptr) // Don't pick weapons owned by someone
				{
					m_pNearbyWeapon = pEntity;
				}
			}
		}
		else if (std::strncmp(szclassname, "npc_grenade_frag", 16) == 0 && (!m_pNearbyGrenade.get() || fDist < distanceFrom(m_pNearbyGrenade.get())))
		{
			const edict_t* pOwner = CClassInterface::getOwner(pEntity);
			const IPlayerInfo* p = playerinfomanager->GetPlayerInfo(pEntity);
			if (pOwner == nullptr || p == nullptr) // Only care about grenades that doesn't have an owner or isn't owned by a player
			{
				m_pNearbyGrenade = pEntity;
				const int iWaypoint = CWaypointLocations::NearestWaypoint(entityorigin, 512.0f, -1);
				if (iWaypoint != -1)
				{
					m_pNavigator->beliefOne(iWaypoint, BELIEF_DANGER, distanceFrom(pEntity));
				}
			}
		}
		else if (std::strncmp(szclassname, "combine_mine", 12) == 0 && (!m_pNearbyMine.get() || fDist < distanceFrom(m_pNearbyMine.get())))
		{
			if (!CSynergyMod::IsCombineMinePlayerPlaced(pEntity)) // Ignore player placed (friendly) mines
			{
				m_pNearbyMine = pEntity;
				const int iWaypoint = CWaypointLocations::NearestWaypoint(entityorigin, 512.0f, -1);
				if (iWaypoint != -1)
				{
					m_pNavigator->beliefOne(iWaypoint, BELIEF_DANGER, distanceFrom(pEntity));
				}
			}
		}
		else if (std::strncmp(szclassname, "item_item_crate", 15) == 0 && (!m_pNearbyItemCrate.get() || fDist < distanceFrom(m_pNearbyItemCrate.get())))
		{
			m_pNearbyItemCrate = pEntity;
		}
		else if (std::strncmp(szclassname, "item_healthcharger", 18) == 0 && (!m_pNearbyHealthCharger.get() || fDist < distanceFrom(m_pNearbyHealthCharger.get())))
		{
			if (CClassInterface::getAnimCycle(pEntity) < 1.0f)
			{
				m_pNearbyHealthCharger = pEntity;
			}
		}
		else if (std::strncmp(szclassname, "item_suitcharger", 16) == 0 && (!m_pNearbyArmorCharger.get() || fDist < distanceFrom(m_pNearbyArmorCharger.get())))
		{
			if (CClassInterface::getAnimCycle(pEntity) < 1.0f)
			{
				m_pNearbyArmorCharger = pEntity;
			}
		}
	}
	else
	{
		if (pEntity == m_pNearbyAmmo.get_old())
			m_pNearbyAmmo = nullptr;
		else if (pEntity == m_pNearbyCrate.get_old())
			m_pNearbyCrate = nullptr;
		else if (pEntity == m_pNearbyHealthKit.get_old())
			m_pNearbyHealthKit = nullptr;
		else if (pEntity == m_pNearbyBattery.get_old())
			m_pNearbyBattery = nullptr;
		else if (pEntity == m_pNearbyWeapon.get_old())
			m_pNearbyWeapon = nullptr;
		else if (pEntity == m_pNearbyMine.get_old())
			m_pNearbyMine = nullptr;
		else if (pEntity == m_pNearbyGrenade.get_old())
			m_pNearbyGrenade = nullptr;
		else if (pEntity == m_pNearbyItemCrate.get_old())
			m_pNearbyItemCrate = nullptr;
		else if (pEntity == m_pNearbyHealthCharger.get_old())
			m_pNearbyHealthCharger = nullptr;
		else if (pEntity == m_pNearbyArmorCharger.get_old())
			m_pNearbyArmorCharger = nullptr;
	}

	return bValid;
}

void CBotSynergy::getTasks(unsigned int iIgnore)
{
	static CBotUtilities utils;
	static CBotUtility* next;
	static bool bCheckCurrent;

	if (!hasSomeConditions(CONDITION_CHANGED) && !m_pSchedules->isEmpty())
		return;

	removeCondition(CONDITION_CHANGED);
	bCheckCurrent = true; // important for checking current schedule

	// Utilities
	ADD_UTILITY(BOT_UTIL_PICKUP_WEAPON, m_pNearbyWeapon.get() != NULL, 0.75f) // New weapons are interesting, high priority
	ADD_UTILITY(BOT_UTIL_GETHEALTHKIT, m_pNearbyHealthKit.get() != NULL, 1.0f - getHealthPercent())	// Pick up health kits
	ADD_UTILITY(BOT_UTIL_HL2DM_FIND_ARMOR, m_pNearbyBattery.get() != NULL, 1.0f - getArmorPercent()) // Pick up armor batteries
	ADD_UTILITY(BOT_UTIL_FIND_NEAREST_HEALTH, hasSomeConditions(CONDITION_NEED_HEALTH), 1.0f - getHealthPercent()) // Search for health kits
	ADD_UTILITY(BOT_UTIL_FIND_NEAREST_AMMO, hasSomeConditions(CONDITION_NEED_AMMO), 0.15f) // Search for ammo
	ADD_UTILITY(BOT_UTIL_ATTACK_POINT, true, 0.01f) // Go to waypoints with 'goal' flag
	ADD_UTILITY(BOT_UTIL_ROAM, true, 0.0001f) // Roam around

	utils.execute();

	while ((next = utils.nextBest()) != nullptr)
	{
		if (!m_pSchedules->isEmpty() && bCheckCurrent)
		{
			if (m_CurrentUtil != next->getId())
				m_pSchedules->freeMemory();
			else
				break;
		}

		bCheckCurrent = false;

		if (executeAction(next->getId()))
		{
			m_CurrentUtil = next->getId();
			m_flInterruptTime = engine->Time() + randomFloat(30.0f, 45.0f);

			if (m_fUtilTimes[next->getId()] < engine->Time())
				m_fUtilTimes[next->getId()] = engine->Time() + randomFloat(0.1f, 2.0f); // saves problems with consistent failing

			if (CClients::clientsDebugging(BOT_DEBUG_UTIL))
			{
				CClients::clientDebugMsg(BOT_DEBUG_UTIL, g_szUtils[next->getId()], this);
			}
			break;
		}
	}

	utils.freeMemory();
}

bool CBotSynergy::executeAction(eBotAction iAction)
{
	switch (iAction)
	{
	case BOT_UTIL_PICKUP_WEAPON:
		m_pSchedules->add(new CBotPickupSched(m_pNearbyWeapon.get()));
		m_fUtilTimes[BOT_UTIL_PICKUP_WEAPON] = engine->Time() + randomFloat(5.0f, 10.0f);
		return true;
		//break;
	case BOT_UTIL_GETHEALTHKIT:
		m_pSchedules->add(new CBotPickupSched(m_pNearbyHealthKit.get()));
		m_fUtilTimes[BOT_UTIL_GETHEALTHKIT] = engine->Time() + randomFloat(5.0f, 10.0f);
		return true;
		//break;
	case BOT_UTIL_HL2DM_FIND_ARMOR:
		m_pSchedules->add(new CBotPickupSched(m_pNearbyBattery.get()));
		m_fUtilTimes[BOT_UTIL_HL2DM_FIND_ARMOR] = engine->Time() + randomFloat(5.0f, 10.0f);
		return true;
		//break;
	case BOT_UTIL_FIND_NEAREST_HEALTH:
	{
		const Vector vOrigin = getOrigin();
		CBotSchedule* pSched = new CBotSchedule();
		pSched->setID(SCHED_GOTO_ORIGIN);
		updateCondition(CONDITION_COVERT); // Pay more attention to danger
		CWaypoint* pWaypoint = CWaypoints::getWaypoint(
			CWaypoints::nearestWaypointGoal(CWaypointTypes::W_FL_HEALTH, vOrigin, 2048.0f));
		m_fUtilTimes[BOT_UTIL_FIND_NEAREST_HEALTH] = engine->Time() + randomFloat(60.0f, 90.0f);

		if (pWaypoint)
		{
			pSched->addTask(new CFindPathTask(CWaypoints::getWaypointIndex(pWaypoint), LOOK_WAYPOINT));
			pSched->addTask(new CMoveToTask(pWaypoint->getOrigin()));
			m_pSchedules->add(pSched);
			return true;
		}
	}
	case BOT_UTIL_FIND_NEAREST_AMMO:
	{
		const Vector vOrigin = getOrigin();
		CBotSchedule* pSched = new CBotSchedule();
		pSched->setID(SCHED_GOTO_ORIGIN);
		CWaypoint* pWaypoint = CWaypoints::getWaypoint(
			CWaypoints::nearestWaypointGoal(CWaypointTypes::W_FL_AMMO, vOrigin, 2048.0f));
		m_fUtilTimes[BOT_UTIL_FIND_NEAREST_AMMO] = engine->Time() + randomFloat(60.0f, 90.0f);

		if (pWaypoint)
		{
			pSched->addTask(new CFindPathTask(CWaypoints::getWaypointIndex(pWaypoint), LOOK_WAYPOINT));
			pSched->addTask(new CMoveToTask(pWaypoint->getOrigin()));
			m_pSchedules->add(pSched);
			return true;
		}
	}
	case BOT_UTIL_ATTACK_POINT:
	{
		// roam
		CBotSchedule* pSched = new CBotSchedule();
		m_fUtilTimes[BOT_UTIL_ATTACK_POINT] = engine->Time() + randomFloat(60.0f, 180.0f);

		pSched->setID(SCHED_ATTACKPOINT);

		// Make the bot more likely to use alternate paths based on their braveness and current health
		if (getHealthPercent() + m_pProfile->m_fBraveness <= 1.0f)
			updateCondition(CONDITION_COVERT);
		else
			removeCondition(CONDITION_COVERT);

		CWaypoint* pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_GOAL);

		if (pWaypoint)
		{
			CBotTask* pFindPath;
			CWaypoint* pRoute = CWaypoints::randomRouteWaypoint(this, getOrigin(), pWaypoint->getOrigin(), 0, 0);
			if ((m_fUseRouteTime <= engine->Time()))
			{
				if (pRoute)
				{
					const int iRoute = CWaypoints::getWaypointIndex(pRoute); // Route waypoint
					pFindPath = new CFindPathTask(iRoute, LOOK_WAYPOINT);
					pFindPath->setInterruptFunction(new CBotSYNRoamInterrupt());
					pSched->addTask(pFindPath);
					pSched->addTask(new CMoveToTask(pRoute->getOrigin()));
					m_pSchedules->add(pSched);
					m_fUseRouteTime = engine->Time() + 30.0f;
				}
			}

			const int iWaypoint = CWaypoints::getWaypointIndex(pWaypoint);
			pFindPath = new CFindPathTask(iWaypoint, LOOK_WAYPOINT);
			pFindPath->setInterruptFunction(new CBotSYNRoamInterrupt());
			pSched->addTask(pFindPath);
			pSched->addTask(new CMoveToTask(pWaypoint->getOrigin()));
			m_pSchedules->add(pSched);

			return true;
		}

		break;
	}
	case BOT_UTIL_ROAM:
	{
		// roam
		CBotSchedule* pSched = new CBotSchedule();

		pSched->setID(SCHED_GOTO_ORIGIN);

		// Make the bot more likely to use alternate paths based on their braveness and current health
		if (getHealthPercent() + m_pProfile->m_fBraveness <= 1.0f)
			updateCondition(CONDITION_COVERT);
		else
			removeCondition(CONDITION_COVERT);

		CWaypoint* pWaypoint = CWaypoints::randomWaypointGoal(-1);

		if (pWaypoint)
		{
			CBotTask* pFindPath;
			CWaypoint* pRoute = CWaypoints::randomRouteWaypoint(this, getOrigin(), pWaypoint->getOrigin(), 0, 0);
			if ((m_fUseRouteTime <= engine->Time()))
			{
				if (pRoute)
				{
					const int iRoute = CWaypoints::getWaypointIndex(pRoute); // Route waypoint
					pFindPath = new CFindPathTask(iRoute, LOOK_WAYPOINT);
					pFindPath->setInterruptFunction(new CBotSYNRoamInterrupt());
					pSched->addTask(pFindPath);
					pSched->addTask(new CMoveToTask(pRoute->getOrigin()));
					m_pSchedules->add(pSched);
					m_fUseRouteTime = engine->Time() + 30.0f;
				}
			}

			const int iWaypoint = CWaypoints::getWaypointIndex(pWaypoint);
			pFindPath = new CFindPathTask(iWaypoint, LOOK_WAYPOINT);
			pFindPath->setInterruptFunction(new CBotSYNRoamInterrupt());
			pSched->addTask(pFindPath);
			pSched->addTask(new CMoveToTask(pWaypoint->getOrigin()));
			m_pSchedules->add(pSched);

			return true;
		}

		break;
	}
	} //Not required? [APG]RoboCop[CL]

	return false;
}

void CBotSynergy::touchedWpt(CWaypoint* pWaypoint, int iNextWaypoint, int iPrevWaypoint)
{
	if (iNextWaypoint != -1 && pWaypoint->hasFlag(CWaypointTypes::W_FL_USE)) // Use waypoint: Check for door
	{
		CWaypoint* pNext = CWaypoints::getWaypoint(iNextWaypoint);
		if (pNext && pNext->hasFlag(CWaypointTypes::W_FL_USE))
		{
			/**
			 * Perform a trace to check if there is something blocking the path between the current waypoint and the next waypoint.
			 * Originally I wanted to use tr->GetEntityIndex() and check if the hit entity is a door
			 * but that function causes link errors when compiling, so I had to fall back to manually searching for door entities.
			**/
			CTraceFilterHitAll filter;
			const trace_t* tr = CBotGlobals::getTraceResult();
			CBotGlobals::traceLine(pWaypoint->getOrigin() + Vector(0, 0, CWaypoint::WAYPOINT_HEIGHT / 2), pNext->getOrigin() + Vector(0, 0, CWaypoint::WAYPOINT_HEIGHT / 2), MASK_PLAYERSOLID, &filter);
			if (tr->fraction < 1.0f)
			{
				if (tr->m_pEnt)
				{
					edict_t* pDoor = servergameents->BaseEntityToEdict(tr->m_pEnt);
					const char* szclassname = pDoor->GetClassName();
					if (std::strncmp(szclassname, "prop_door_rotating", 18) == 0 || std::strncmp(szclassname, "func_door", 9) == 0 || std::strncmp(szclassname, "func_door_rotating", 18) == 0)
					{
						if (!CSynergyMod::IsEntityLocked(pDoor))
						{
							m_pSchedules->addFront(new CSynOpenDoorSched(pDoor));
						}
					}
				}
				/* 				pDoor = CClassInterface::FindEntityByClassnameNearest(getOrigin(), "prop_door_rotating", rcbot_syn_use_search_range.GetFloat());
								if(pDoor != NULL && !CSynergyMod::IsEntityLocked(pDoor))
								{
									m_pSchedules->addFront(new CSynOpenDoorSched(pDoor));
								}
								else
								{
									pDoor = CClassInterface::FindEntityByClassnameNearest(getOrigin(), "func_door", rcbot_syn_use_search_range.GetFloat());
									if(pDoor != NULL && !CSynergyMod::IsEntityLocked(pDoor))
									{
										m_pSchedules->addFront(new CSynOpenDoorSched(pDoor));
									}
									else
									{
										pDoor = CClassInterface::FindEntityByClassnameNearest(getOrigin(), "func_door_rotating", rcbot_syn_use_search_range.GetFloat());
										if(pDoor != NULL && !CSynergyMod::IsEntityLocked(pDoor))
										{
											m_pSchedules->addFront(new CSynOpenDoorSched(pDoor));
										}
									}
								} */
			}
		}
	}
	else // Check for button
	{
		edict_t* pEntity = CClassInterface::FindEntityByClassnameNearest(getOrigin(), "func_button",
			rcbot_syn_use_search_range.GetFloat());
		if (pEntity != nullptr && !CSynergyMod::IsEntityLocked(pEntity))
		{
			CBotSchedule* sched = new CBotSchedule();
			sched->setID(SCHED_GOTO_ORIGIN);
			sched->addTask(new CMoveToTask(pEntity));
			sched->addTask(new CBotHL2DMUseButton(pEntity));
			sched->addTask(new CBotWaitTask(randomFloat(3.0f, 6.0f)));
			m_pSchedules->addFront(sched);
		}
	}

	CBot::touchedWpt(pWaypoint, iNextWaypoint, iPrevWaypoint);
}

bool CBotSynergy::walkingTowardsWaypoint(CWaypoint* pWaypoint, bool* bOffsetApplied, Vector& vOffset)
{
	return CBot::walkingTowardsWaypoint(pWaypoint, bOffsetApplied, vOffset);
}

void CBotSynergy::reachedCoverSpot(int flags)
{
	removeCondition(CONDITION_RUN); // Remove when in cover
}

void CBotSynergy::handleWeapons()
{
	if (m_pEnemy && !hasSomeConditions(CONDITION_ENEMY_DEAD) &&
		hasSomeConditions(CONDITION_SEE_CUR_ENEMY) && wantToShoot() &&
		isVisible(m_pEnemy) && isEnemy(m_pEnemy))
	{
		const char* szclassname = m_pEnemy.get()->GetClassName();
		CBotWeapon* pWeapon;

		if ((std::strncmp(szclassname, "npc_combinegunship", 18) == 0) || (std::strncmp(szclassname, "npc_combinedropship", 19) == 0) || (std::strncmp(szclassname, "npc_strider", 11) == 0) ||
			(std::strncmp(szclassname, "npc_helicopter", 14) == 0))
		{
			pWeapon = m_pWeapons->getWeapon(CWeapons::getWeapon(SYN_WEAPON_RPG));
		}
		else
		{
			pWeapon = getBestWeapon(m_pEnemy, true, true, false, false);
		}

		if (m_bWantToChangeWeapon && (pWeapon != nullptr) && (pWeapon != getCurrentWeapon()) && pWeapon->getWeaponIndex())
		{
			selectBotWeapon(pWeapon);
		}

		setLookAtTask(LOOK_ENEMY);

		if (!handleAttack(pWeapon, m_pEnemy))
		{
			m_pEnemy = nullptr;
			m_pOldEnemy = nullptr;
			wantToShoot(false);
		}
	}
}

bool CBotSynergy::handleAttack(CBotWeapon* pWeapon, edict_t* pEnemy)
{
	const char* szclassname = pEnemy->GetClassName();

	if ((std::strncmp(szclassname, "npc_combinegunship", 18) == 0) || (std::strncmp(szclassname, "npc_combinedropship", 19) == 0) || (std::strncmp(szclassname, "npc_strider", 11) == 0) ||
		(std::strncmp(szclassname, "npc_helicopter", 14) == 0))
	{
		if (!m_pWeapons->hasWeapon(SYN_WEAPON_RPG))
		{
			return false;
		}
	}

	if (pWeapon)
	{
		clearFailedWeaponSelect();

		if (pWeapon->isMelee())
			setMoveTo(CBotGlobals::entityOrigin(pEnemy));

		if (pWeapon->mustHoldAttack())
			primaryAttack(true);
		else
			primaryAttack();
	}
	else
	{
		primaryAttack();
	}

	return true;
}

/**
 * Filters and validates ammo entities
 *
 * @param pAmmo			The ammo entity to validate
 * @param szclassname	The ammo entity's classname
 * @return          	True if the bot should pick up this ammo entity
 **/
bool CBotSynergy::filterAmmo(edict_t* pAmmo, const char* szclassname)
{
	if (std::strncmp(szclassname, "item_ammo_pistol", 16) == 0)
	{
		if (m_pWeapons->hasWeapon(SYN_WEAPON_PISTOL))
		{
			if (m_pWeapons->getWeapon(CWeapons::getWeapon(SYN_WEAPON_PISTOL))->getAmmo(this, AMMO_PRIM) < 36)
			{
				return true;
			}
		}
	}
	else if (std::strncmp(szclassname, "item_ammo_357", 13) == 0)
	{
		if (m_pWeapons->hasWeapon(SYN_WEAPON_357) || m_pWeapons->hasWeapon(SYN_WEAPON_DESERTEAGLE))
		{
			if (m_pWeapons->getWeapon(CWeapons::getWeapon(SYN_WEAPON_357))->getAmmo(this, AMMO_PRIM) < 6 ||
				m_pWeapons->getWeapon(CWeapons::getWeapon(SYN_WEAPON_DESERTEAGLE))->getAmmo(this, AMMO_PRIM) < 6)
			{
				return true;
			}
		}
	}
	else if (std::strncmp(szclassname, "item_ammo_smg1", 14) == 0)
	{
		if (std::strncmp(szclassname, "item_ammo_smg1_grenade", 22) == 0)
		{
			if (m_pWeapons->hasWeapon(SYN_WEAPON_SMG1))
			{
				if (m_pWeapons->getWeapon(CWeapons::getWeapon(SYN_WEAPON_SMG1))->getAmmo(this, AMMO_SEC) < 1)
				{
					return true;
				}
			}
		}
		else
		{
			if (m_pWeapons->hasWeapon(SYN_WEAPON_SMG1) || m_pWeapons->hasWeapon(SYN_WEAPON_MP5K))
			{
				if (m_pWeapons->getWeapon(CWeapons::getWeapon(SYN_WEAPON_SMG1))->getAmmo(this, AMMO_PRIM) < 75 ||
					m_pWeapons->getWeapon(CWeapons::getWeapon(SYN_WEAPON_MP5K))->getAmmo(this, AMMO_PRIM) < 75)
				{
					return true;
				}
			}
		}
	}
	else if (std::strncmp(szclassname, "item_ammo_ar2", 13) == 0)
	{
		if (std::strncmp(szclassname, "item_ammo_ar2_altfire", 21) == 0)
		{
			if (m_pWeapons->hasWeapon(SYN_WEAPON_AR2))
			{
				if (m_pWeapons->getWeapon(CWeapons::getWeapon(SYN_WEAPON_AR2))->getAmmo(this, AMMO_SEC) < 1)
				{
					return true;
				}
			}
		}
		else
		{
			if (m_pWeapons->hasWeapon(SYN_WEAPON_AR2) || m_pWeapons->hasWeapon(SYN_WEAPON_MG1))
			{
				if (m_pWeapons->getWeapon(CWeapons::getWeapon(SYN_WEAPON_AR2))->getAmmo(this, AMMO_PRIM) < 30 ||
					m_pWeapons->getWeapon(CWeapons::getWeapon(SYN_WEAPON_MG1))->getAmmo(this, AMMO_PRIM) < 30)
				{
					return true;
				}
			}
		}
	}
	else if (std::strncmp(szclassname, "item_ammo_crossbow", 18) == 0)
	{
		if (m_pWeapons->hasWeapon(SYN_WEAPON_CROSSBOW))
		{
			if (m_pWeapons->getWeapon(CWeapons::getWeapon(SYN_WEAPON_CROSSBOW))->getAmmo(this, AMMO_PRIM) < 3)
			{
				return true;
			}
		}
	}
	else if (std::strncmp(szclassname, "item_box_buckshot", 17) == 0)
	{
		if (m_pWeapons->hasWeapon(SYN_WEAPON_SHOTGUN))
		{
			if (m_pWeapons->getWeapon(CWeapons::getWeapon(SYN_WEAPON_SHOTGUN))->getAmmo(this, AMMO_PRIM) < 12)
			{
				return true;
			}
		}
	}
	else if (std::strncmp(szclassname, "item_rpg_round", 14) == 0)
	{
		if (m_pWeapons->hasWeapon(SYN_WEAPON_RPG))
		{
			if (m_pWeapons->getWeapon(CWeapons::getWeapon(SYN_WEAPON_RPG))->getAmmo(this, AMMO_PRIM) < 2)
			{
				return true;
			}
		}
	}

	return false;
}

/**
 * This functions is called by task interruptions check to see if the bot should change it's current task
 *
 * @return 		TRUE if the bot should interrupt it's current task
 **/
bool CBotSynergy::wantsToChangeCourseOfAction()
{
	return false; // TO-DO
}