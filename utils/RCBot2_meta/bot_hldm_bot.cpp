// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
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

#include "in_buttons.h"
#include "bot.h"
#include "bot_cvars.h"
#include "bot_hldm_bot.h"
#include "bot_client.h"
#include "bot_buttons.h"
#include "bot_globals.h"
//#include "bot_profile.h"
#include "bot_navigator.h"
#include "bot_waypoint.h"
#include "bot_utility.h"
#include "bot_task.h"
#include "bot_schedule.h"
//#include "bot_waypoint.h"
#include "bot_weapons.h"
#include "bot_mtrand.h"
#include "bot_waypoint_locations.h"
#include "bot_getprop.h"
#include "rcbot/logging.h"

#ifdef ENABLE_SOURCEMOD_INTEGRATION
#include "../../sm_ext/bot_sm_events.h"
#endif

#include <cstring>

// initialise , i.e. set everything to a default value
void CHLDMBot :: init (const bool bVarInit)
{
	CBot::init(bVarInit);
}

// initialize structures and classes used be the bot
// i.e. create 'new' classes
void CHLDMBot :: setup ()
{
	CBot::setup();
}

// the bot doesn't need to do anything to start a game in HL2DM
bool CHLDMBot :: startGame ()
{
	return true;
}

// the bot killed pVictim
void CHLDMBot :: killed ( edict_t *pVictim, char *weapon )
{
    CBot::killed(pVictim, weapon);

	// update belief around this waypoint
    if (pVictim && CBotGlobals::entityIsValid(pVictim))
    {
        m_pNavigator->belief(CBotGlobals::entityOrigin(pVictim), getEyePosition(), bot_beliefmulti.GetFloat(), distanceFrom(pVictim), BELIEF_SAFETY);
	}
}

// the bot was killed by pKiller
void CHLDMBot :: died ( edict_t *pKiller, const char *pszWeapon )
{
	// re-initialize stuff per life
	CBot::died(pKiller, pszWeapon);

	//if ( randomInt(0,1) )
	m_pButtons->attack(); // respawn

	if ( pKiller )
	{
		if ( CBotGlobals::entityIsValid(pKiller) )
		{
			m_pNavigator->belief(CBotGlobals::entityOrigin(pKiller),getEyePosition(),bot_beliefmulti.GetFloat(),distanceFrom(pKiller),BELIEF_DANGER);
		}
	}
}

void CHLDMBot :: touchedWpt (CWaypoint* pWaypoint, const int iNextWaypoint, const int iPrevWaypoint)
{
	CBot::touchedWpt(pWaypoint, iNextWaypoint, iPrevWaypoint);

	if ( pWaypoint->hasFlag(CWaypointTypes::W_FL_LIFT) )
	{
		if ( m_fUseButtonTime < engine->Time() )
		{
			if ( edict_t *pButton = CHalfLifeDeathmatchMod::getButtonAtWaypoint(pWaypoint) )
			{
				m_fUseButtonTime = engine->Time() + randomFloat(5.0f,10.0f);

				m_pSchedules->addFront(new CBotSchedule(new CBotHL2DMUseButton(pButton)));

			}
		}
	}
}

// new life
void CHLDMBot :: spawnInit ()
{
	CBot::spawnInit();

	if ( m_pWeapons )
		m_pWeapons->clearWeapons();

	m_CurrentUtil = BOT_UTIL_MAX;
	// reset objects
	m_pNearbyWeapon = nullptr;
	m_FailedPhysObj = nullptr;
	m_fSprintTime = 0.0f;
	m_NearestPhysObj = nullptr;
	
	m_pBattery = nullptr;
	m_pHealthKit = nullptr;
	m_pAmmoKit = nullptr;
	m_pCurrentWeapon = nullptr;
	m_pCharger = nullptr;
	
	m_fFixWeaponTime = 0.0f;
	m_fUseButtonTime = 0.0f;
	m_fUseCrateTime = 0.0f;

	ConVarRef hl2_normspeed("hl2_normspeed");

	if (!hl2_normspeed.IsValid())
	{
		logger->Log(LogLevel::ERROR, "Unable to find \"hl2_normspeed\" ConVar!");
		m_fCachedNormSpeed = 190.0f; // hl2_normspeed default value -caxanga334
	}
	else
	{
		m_fCachedNormSpeed = hl2_normspeed.GetFloat();
	}
}

// Is pEdict an enemy? return true if enemy / false if not
// if checkWeapons is true, check if current weapon can attack enemy 
//							return false if not 
bool CHLDMBot::isEnemy(edict_t* pEdict, bool bCheckWeapons)
{
	static int entity_index;

	entity_index = ENTINDEX(pEdict);

	// if no target on - listen sever player is a non target
	if (rcbot_notarget.GetBool() && entity_index == 1)
		return false;

	// not myself
	if (pEdict == m_pEdict)
		return false;

	// not a player - false
	if (!entity_index || entity_index > CBotGlobals::maxClients())
	{
		if (!m_pCarryingObject && pEdict->GetUnknown() && pEdict == m_NearestBreakable && CClassInterface::getPlayerHealth(pEdict) > 0)
		{
			if (distanceFrom(CBotGlobals::entityOrigin(pEdict)) < rcbot_jump_obst_dist.GetFloat())
			{
				if (BotFunc_BreakableIsEnemy(m_NearestBreakable, m_pEdict) || (CBotGlobals::entityOrigin(pEdict) - m_vMoveTo).Length() + 48 < (getOrigin() - m_vMoveTo).Length())
					return true;
			}
		}

		return false;
	}

	// not alive -- false
	if (!CBotGlobals::entityIsAlive(pEdict))
		return false;

	// team game?
	if (CBotGlobals::getTeamplayOn())
	{
		// same team ? false
		if (CBotGlobals::getTeam(pEdict) == getTeam())
			return false;
	}

	return true;
}

// from the bots UTILITIES , execute the given action
bool CHLDMBot :: executeAction (const eBotAction iAction)
{
	switch ( iAction )
	{
	case BOT_UTIL_HL2DM_USE_CRATE:
		// check if it is worth it first
		{
			const CBotWeapon *pWeapon = nullptr;

			/*
			possible models
			0000000000111111111122222222223333
			0123456789012345678901234567890123
			models/items/ammocrate_ar2.mdl
			models/items/ammocrate_grenade.mdl
			models/items/ammocrate_rockets.mdl
			models/items/ammocrate_smg1.mdl
			*/

			const char* szModel = m_pAmmoCrate.get()->GetIServerEntity()->GetModelName().ToCStr();
			const char type = szModel[23];

			if ( type == 'a' ) // ar2
			{
				pWeapon = m_pWeapons->getWeapon(CWeapons::getWeapon(HL2DM_WEAPON_AR2));
			}
			else if ( type == 'g' ) // grenade
			{
				pWeapon = m_pWeapons->getWeapon(CWeapons::getWeapon(HL2DM_WEAPON_FRAG));
			}
			else if ( type == 'r' ) // rocket
			{
				pWeapon = m_pWeapons->getWeapon(CWeapons::getWeapon(HL2DM_WEAPON_RPG));
			}
			else if ( type == 's' ) // smg
			{
				pWeapon = m_pWeapons->getWeapon(CWeapons::getWeapon(HL2DM_WEAPON_SMG1));
			}

			if ( pWeapon && pWeapon->getAmmo(this) < 1 )
			{
				CBotSchedule *pSched = new CBotSchedule();
				
				pSched->addTask(new CFindPathTask(m_pAmmoCrate));
				pSched->addTask(new CBotHL2DMUseButton(m_pAmmoCrate));

				m_pSchedules->add(pSched);

				m_fUtilTimes[iAction] = engine->Time() + randomFloat(5.0f,10.0f);
				return true;
			}
		}
		return false;
	case BOT_UTIL_PICKUP_WEAPON:
		m_pSchedules->add(new CBotPickupSched(m_pNearbyWeapon.get()));
#ifdef ENABLE_SOURCEMOD_INTEGRATION
		// Notify SourceMod of weapon pickup
		if (m_pNearbyWeapon.get())
		{
			RCBotEvents::HLDM::OnBotWeaponPickup(m_pEdict, m_pNearbyWeapon.get());
		}
#endif
		return true;
	case BOT_UTIL_FIND_NEAREST_HEALTH:
		m_pSchedules->add(new CBotPickupSched(m_pHealthKit.get()));
		return true;
	case BOT_UTIL_HL2DM_FIND_ARMOR:
		m_pSchedules->add(new CBotPickupSched(m_pBattery.get()));
		return true;
	case BOT_UTIL_FIND_NEAREST_AMMO:
		m_pSchedules->add(new CBotPickupSched(m_pAmmoKit.get()));
		m_fUtilTimes[iAction] = engine->Time() + randomFloat(5.0f,10.0f);
		return true;
	case BOT_UTIL_HL2DM_USE_HEALTH_CHARGER:
		{
			CBotSchedule *pSched = new CBotSchedule();

			pSched->addTask(new CFindPathTask(m_pHealthCharger));
			pSched->addTask(new CBotHL2DMUseCharger(m_pHealthCharger,CHARGER_HEALTH));

			m_pSchedules->add(pSched);

			m_fUtilTimes[BOT_UTIL_HL2DM_USE_HEALTH_CHARGER] = engine->Time() + randomFloat(5.0f,10.0f);

#ifdef ENABLE_SOURCEMOD_INTEGRATION
			// Notify SourceMod of health charger usage
			if (m_pHealthCharger.get())
			{
				RCBotEvents::HLDM::OnBotSuitChargeUsed(m_pEdict, 0, m_pHealthCharger.get());
			}
#endif
			return true;
		}
	case BOT_UTIL_HL2DM_USE_CHARGER:
		{
			CBotSchedule *pSched = new CBotSchedule();

			pSched->addTask(new CFindPathTask(m_pCharger));
			pSched->addTask(new CBotHL2DMUseCharger(m_pCharger,CHARGER_ARMOR));

			m_pSchedules->add(pSched);

			m_fUtilTimes[BOT_UTIL_HL2DM_USE_CHARGER] = engine->Time() + randomFloat(5.0f,10.0f);

#ifdef ENABLE_SOURCEMOD_INTEGRATION
			// Notify SourceMod of armor charger usage
			if (m_pCharger.get())
			{
				RCBotEvents::HLDM::OnBotSuitChargeUsed(m_pEdict, 1, m_pCharger.get());
			}
#endif
			return true;
		}
	case BOT_UTIL_HL2DM_GRAVIGUN_PICKUP:
		{
			CBotSchedule *pSched = new CBotSchedule(new CBotGravGunPickup(m_pCurrentWeapon,m_NearestPhysObj));
			pSched->setID(SCHED_GRAVGUN_PICKUP);
			m_pSchedules->add(pSched);
			return true;
		}
	case BOT_UTIL_FIND_LAST_ENEMY:
		{
			Vector vVelocity = Vector(0,0,0);
			CClient *pClient = CClients::get(m_pLastEnemy);
			CBotSchedule *pSchedule = new CBotSchedule();
			
			CFindPathTask *pFindPath = new CFindPathTask(m_vLastSeeEnemy);	

			pFindPath->setCompleteInterrupt(CONDITION_SEE_CUR_ENEMY);
			
			assert(pClient != nullptr); // Ensure pClient is not null - [APG]RoboCop[CL]
			if (!CClassInterface::getVelocity(m_pLastEnemy, &vVelocity))
			{
				vVelocity = pClient->getVelocity();
			}

			pSchedule->addTask(pFindPath);
			pSchedule->addTask(new CFindLastEnemy(m_vLastSeeEnemy,vVelocity));

			//////////////
			pFindPath->setNoInterruptions();

			m_pSchedules->add(pSchedule);

			m_bLookedForEnemyLast = true;

			return true;
		}
	case BOT_UTIL_THROW_GRENADE:
		{
		// find hide waypoint

		if ( CWaypoint *pWaypoint = CWaypoints::getWaypoint(CWaypointLocations::GetCoverWaypoint(getOrigin(),m_vLastSeeEnemy, nullptr)) )
			{
				CBotSchedule *pSched = new CBotSchedule();
				pSched->addTask(new CThrowGrenadeTask(
					m_pWeapons->getWeapon(CWeapons::getWeapon(HL2DM_WEAPON_FRAG)),
					getAmmo(CWeapons::getWeapon(HL2DM_WEAPON_FRAG)->getAmmoIndex1()),
					m_vLastSeeEnemyBlastWaypoint
				));

				pSched->addTask(new CFindPathTask(pWaypoint->getOrigin())); // 2nd -- hide
				m_pSchedules->add(pSched);
				return true;
			}

		}
	case BOT_UTIL_SNIPE:
		{
			// roam

			if ( CWaypoint *pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_SNIPER) )
			{
				CBotSchedule* snipe = new CBotSchedule();
				CBotTask *findpath = new CFindPathTask(CWaypoints::getWaypointIndex(pWaypoint));

				// use DOD task  //Unstable? [APG]RoboCop[CL]
				CBotTask* snipetask = new CBotHL2DMSnipe(m_pWeapons->getWeapon(CWeapons::getWeapon(HL2DM_WEAPON_CROSSBOW)),
				                                         pWaypoint->getOrigin(), pWaypoint->getAimYaw(), false, 0);

				findpath->setCompleteInterrupt(CONDITION_PUSH);
				snipetask->setCompleteInterrupt(CONDITION_PUSH);

				snipe->setID(SCHED_DEFENDPOINT);
				snipe->addTask(findpath);
				snipe->addTask(snipetask);
				
				m_pSchedules->add(snipe);

				return true;
			}

			break;
		}
	case BOT_UTIL_ROAM:
		// roam
		CWaypoint *pWaypoint = CWaypoints::getWaypoint(CWaypoints::randomFlaggedWaypoint(getTeam()));

		if ( pWaypoint )
		{
			m_pSchedules->add(new CBotGotoOriginSched(pWaypoint->getOrigin()));
			return true;
		}

		break;
	}

	return false;
}

// deal with attacking code here
// return false if it is impossible to shoot this enemy; i.e. change enemy
// return true: if it is possible to shoot this enemy
// decide whether or not the bot shoot attack by calling primaryAttack or secondaryAttack
bool CHLDMBot :: handleAttack ( CBotWeapon *pWeapon, edict_t *pEnemy )
{
	if ( pWeapon )
	{
		static float fDistance;

		fDistance = distanceFrom(pEnemy);

		clearFailedWeaponSelect();

		if ( pWeapon->isMelee() )
			setMoveTo(CBotGlobals::entityOrigin(pEnemy));

		if ( pWeapon->getID() == HL2DM_WEAPON_PHYSCANNON && DotProductFromOrigin(m_vAimVector) < rcbot_enemyshoot_gravgun_fov.GetFloat() ) 
			return true; // keep enemy / don't shoot : until angle between enemy is less than 20 degrees

		if ( pWeapon->canUseSecondary() && pWeapon->getAmmo(this,2) && pWeapon->secondaryInRange(fDistance) )
		{
			if ( randomInt(0,1) )
			{
				secondaryAttack();
				return true;
			}
		}

		// can use primary
		if ( pWeapon->canAttack() )
		{
			if ( pWeapon->mustHoldAttack() )
				primaryAttack(true);
			else
				primaryAttack();

			return true;
		}
	}
	else
	{
		primaryAttack();
		return true;
	}

	return false;
}
// time to think about something new to do
void CHLDMBot :: getTasks (unsigned iIgnore)
{
	static CBotUtilities utils;
	static CBotUtility *next;
	static CBotWeapon *gravgun;
	static CBotWeapon *crossbow;
	static bool bCheckCurrent;

	if ( !hasSomeConditions(CONDITION_CHANGED) && !m_pSchedules->isEmpty() )
		return;

	removeCondition(CONDITION_CHANGED);

	bCheckCurrent = true; // important for checking current schedule

	gravgun = m_pWeapons->getWeapon(CWeapons::getWeapon(HL2DM_WEAPON_PHYSCANNON));

	// If I have the grav gun, think about picking something up
	if ( gravgun )
	{
		edict_t *pent = INDEXENT(gravgun->getWeaponIndex());

		if (CBotGlobals::entityIsValid(pent))
		{
			ADD_UTILITY(BOT_UTIL_HL2DM_GRAVIGUN_PICKUP,
				(!m_pEnemy || (m_pCurrentWeapon && std::strcmp("weapon_physcannon", m_pCurrentWeapon->GetClassName()) == 0)) &&
				gravgun && gravgun->hasWeapon() && (m_NearestPhysObj.get() != nullptr) && gravgun->getWeaponIndex() > 0 &&
				(CClassInterface::gravityGunObject(INDEXENT(gravgun->getWeaponIndex())) == nullptr), 0.9f)
		}
	}

	if ( (crossbow = m_pWeapons->getWeapon(CWeapons::getWeapon(HL2DM_WEAPON_CROSSBOW))) != nullptr)
	{
		if ( crossbow->hasWeapon() && !crossbow->outOfAmmo(this) )
			ADD_UTILITY(BOT_UTIL_SNIPE,true,0.91f)
	}

	// low on health? Pick some up if there's any near by
	ADD_UTILITY(BOT_UTIL_HL2DM_USE_HEALTH_CHARGER,
	            (m_pHealthCharger.get() != nullptr) && CClassInterface::getAnimCycle(m_pHealthCharger)<1.0f &&
	            getHealthPercent()<1.0f, 1.0f-getHealthPercent())
	ADD_UTILITY(BOT_UTIL_FIND_NEAREST_HEALTH,(m_pHealthKit.get()!=nullptr) && getHealthPercent()<1.0f,1.0f-getHealthPercent())

	// low on armor?
	ADD_UTILITY(BOT_UTIL_HL2DM_FIND_ARMOR,(m_pBattery.get() !=nullptr) && getArmorPercent()<1.0f,(1.0f-getArmorPercent())*0.75f)
	ADD_UTILITY(BOT_UTIL_HL2DM_USE_CHARGER,
	            (m_pCharger.get() !=nullptr) && CClassInterface::getAnimCycle(m_pCharger)<1.0f && getArmorPercent()<1.0f,
	            (1.0f-getArmorPercent())*0.75f)

	ADD_UTILITY(BOT_UTIL_HL2DM_USE_CRATE,(m_pAmmoCrate.get()!=nullptr) && m_fUseCrateTime < engine->Time(),1.0f)
	// low on ammo? ammo nearby?
	ADD_UTILITY(BOT_UTIL_FIND_NEAREST_AMMO,(m_pAmmoKit.get() !=nullptr) && getAmmo(0)<5,0.01f*(100-getAmmo(0)))

	// always able to roam around
	ADD_UTILITY(BOT_UTIL_ROAM,true,0.01f)

	// I have an enemy 
	ADD_UTILITY(BOT_UTIL_FIND_LAST_ENEMY,
	            wantToFollowEnemy() && !m_bLookedForEnemyLast && m_pLastEnemy && CBotGlobals::entityIsValid(m_pLastEnemy
	            ) && CBotGlobals::entityIsAlive(m_pLastEnemy), getHealthPercent()*(getArmorPercent()+0.1f))

	if (!hasSomeConditions(CONDITION_SEE_CUR_ENEMY) && hasSomeConditions(CONDITION_SEE_LAST_ENEMY_POS) && m_pLastEnemy
		&& m_fLastSeeEnemy && m_fLastSeeEnemy + 10.0f > engine->Time() && m_pWeapons->hasWeapon(HL2DM_WEAPON_FRAG))
	{
		const float fDistance = distanceFrom(m_vLastSeeEnemyBlastWaypoint);

		if ( fDistance > BLAST_RADIUS && fDistance < 1500 )
		{
			const CWeapon *pWeapon = CWeapons::getWeapon(HL2DM_WEAPON_FRAG);
			const CBotWeapon *pBotWeapon = m_pWeapons->getWeapon(pWeapon);

			ADD_UTILITY(BOT_UTIL_THROW_GRENADE, pBotWeapon && pBotWeapon->getAmmo(this) > 0,1.0f- getHealthPercent()*0.2f)
		}
	}

	if ( m_pNearbyWeapon.get() )
	{
		static CWeapon *pWeapon;
		pWeapon = CWeapons::getWeapon(m_pNearbyWeapon.get()->GetClassName());

		if ( pWeapon && !m_pWeapons->hasWeapon(pWeapon->getID()) )
		{
			ADD_UTILITY(BOT_UTIL_PICKUP_WEAPON, true , 0.6f + pWeapon->getPreference()*0.1f)
		}
	}


	utils.execute();

	while ( (next = utils.nextBest()) != nullptr)
	{
		if ( !m_pSchedules->isEmpty() && bCheckCurrent )
		{
			if ( m_CurrentUtil != next->getId() )
				m_pSchedules->freeMemory();
			else
				break;
		} 

		bCheckCurrent = false;

		if ( executeAction(next->getId()) )
		{
			m_CurrentUtil = next->getId();

			if ( m_fUtilTimes[next->getId()] < engine->Time() )
				m_fUtilTimes[next->getId()] = engine->Time() + randomFloat(0.1f,2.0f); // saves problems with consistent failing

			if ( CClients::clientsDebugging(BOT_DEBUG_UTIL) )
			{
				CClients::clientDebugMsg(BOT_DEBUG_UTIL,g_szUtils[next->getId()],this);
			}
			break;
		}
	}

	utils.freeMemory();
}

void CHLDMBot :: modThink ()
{
	m_fIdealMoveSpeed = m_fCachedNormSpeed;

	// update hitbox hull
	//m_pEdict->GetCollideable()->GetCollisionOrigin();

	if ( !CBotGlobals::entityIsValid(m_NearestPhysObj) )
		m_NearestPhysObj = nullptr;

	if ( !CBotGlobals::entityIsValid(m_FailedPhysObj) )
		m_FailedPhysObj = nullptr;

	m_pCurrentWeapon = CClassInterface::getCurrentWeapon(m_pEdict);


	if ( m_pCurrentWeapon )
		CClassInterface::getWeaponClip(m_pCurrentWeapon,&m_iClip1,&m_iClip2);

	if ( CClassInterface::onLadder(m_pEdict) != nullptr)
	{
		setMoveLookPriority(MOVELOOK_OVERRIDE);
		setLookAtTask(LOOK_WAYPOINT);
		m_pButtons->holdButton(IN_FORWARD,0,1,0);
		setMoveLookPriority(MOVELOOK_MODTHINK);
	}

	if ( m_fCurrentDanger >= 20.0f && CClassInterface::auxPower(m_pEdict) > 90.0f && m_fSprintTime < engine->Time())
	{
		m_pButtons->holdButton(IN_SPEED,0,1,0);
	}
	else if (m_fCurrentDanger < 1 || CClassInterface::auxPower(m_pEdict) < 5.0f)
	{
		m_fSprintTime = engine->Time() + randomFloat(5.0f,20.0f);
	}

	if ( m_fLastSeeEnemy && m_fLastSeeEnemy + 5.0f<engine->Time() )
	{
		const CBotWeapon *pWeapon = getCurrentWeapon();

		if ( pWeapon && pWeapon->getClip1(this)==0 && pWeapon->getAmmo(this) > 0 )
		{
			m_fLastSeeEnemy = 0.0f;
			m_pButtons->tap(IN_RELOAD);
		}
	}

	if ( m_NearestPhysObj.get() )
	{
		bool bCarry = false;
		edict_t *pEntity = m_NearestPhysObj.get();

		if ( m_pCurrentWeapon && !std::strcmp("weapon_physcannon",m_pCurrentWeapon->GetClassName()) )
		{
			edict_t* pPrevCarrying = m_pCarryingObject;
			m_pCarryingObject = CClassInterface::gravityGunObject(m_pCurrentWeapon);
			bCarry = m_pCarryingObject == m_NearestPhysObj.get();

#ifdef ENABLE_SOURCEMOD_INTEGRATION
			// Track gravity gun pickup events
			if (!pPrevCarrying && m_pCarryingObject)
			{
				// Bot picked up an object
				RCBotEvents::HLDM::OnBotGravityGunPickup(m_pEdict, m_pCarryingObject);
			}
			else if (pPrevCarrying && !m_pCarryingObject)
			{
				// Bot dropped/launched object - check if it was launched or dropped
				// If bot is attacking, it was launched; otherwise dropped
				if (m_pButtons->holdingButton(IN_ATTACK))
				{
					RCBotEvents::HLDM::OnBotGravityGunLaunch(m_pEdict, pPrevCarrying);
				}
				else
				{
					RCBotEvents::HLDM::OnBotGravityGunDrop(m_pEdict, pPrevCarrying);
				}
			}
#endif
		}

		if ( !bCarry && distanceFrom(pEntity) < rcbot_jump_obst_dist.GetFloat() )
		{
			bool bCanJump = false;
			float fTime = 0;

			if ( willCollide(pEntity,&bCanJump,&fTime) )
			{
				if ( bCanJump && fTime < 1.5f ) // one second to jump
				{
					if ( randomInt(0,1) )
						jump();
				}
			}
		}
	}
}

bool CHLDMBot::checkStuck()
{
	static bool bStuck;

	if ( (bStuck = CBot::checkStuck()) == true )
	{
		if ( m_pWeapons->hasWeapon(HL2DM_WEAPON_PHYSCANNON) )
		{// check stuck on object

			/*const CBotWeapon* currentWeapon = getCurrentWeapon(); //Unstable? [APG]RoboCop[CL]

			if (currentWeapon->getID() == HL2DM_WEAPON_PHYSCANNON && m_pCarryingObject)
			{
				primaryAttack();
			}
			else */if ( m_NearestPhysObj && distanceFrom(m_NearestPhysObj)<100 )
			{
				if ( !m_pSchedules->hasSchedule(SCHED_GRAVGUN_PICKUP) )
				{
					m_pSchedules->freeMemory();
					CBotSchedule* const pSched = new CBotSchedule(
						new CBotGravGunPickup(m_pCurrentWeapon, m_NearestPhysObj));
					pSched->setID(SCHED_GRAVGUN_PICKUP);
					m_pSchedules->add(pSched);
				}
			}
		}
	}

	return bStuck;
}

bool CHLDMBot :: willCollide ( edict_t *pEntity, bool *bCanJump, float *fTime ) const
{
	static Vector vel;

	if ( CClassInterface::getVelocity(m_pEdict,&vel) )
	{
		static Vector v_size;
		static float fDistance;
		static Vector vOrigin;

		static float fSpeed;
		static Vector v_min;
		static Vector v_max;

		v_min = pEntity->GetCollideable()->OBBMins();
		v_max = pEntity->GetCollideable()->OBBMaxs();
		v_size = v_max - v_min;

		fDistance = distanceFrom(pEntity);
		vOrigin = CBotGlobals::entityOrigin(pEntity);
		fSpeed = vel.Length();

		// speed = dist/time  --- time = dist/speed
		if ( fSpeed > 0 )
		{
			static Vector v_dest;
			*fTime = fDistance / fSpeed;

			vel = vel / fSpeed; // normalize
			v_dest = getOrigin() + vel*fDistance;

			if ( v_size.z <= 48 ) // jump height
				*bCanJump = true;

			return (vOrigin - v_dest).Length() < v_size.Length()/2;
		}
	}

	return false;
}

void CHLDMBot :: handleWeapons ()
{
	//
	// Handle attacking at this point
	//
	if ( m_pEnemy && !hasSomeConditions(CONDITION_ENEMY_DEAD) && 
		hasSomeConditions(CONDITION_SEE_CUR_ENEMY) && wantToShoot() && 
		isVisible(m_pEnemy) && isEnemy(m_pEnemy) )
	{
		CBotWeapon* pWeapon = getBestWeapon(m_pEnemy, true, true,
		                                    m_pEnemy == m_NearestBreakable && !rcbot_melee_only.GetBool());

		if ( m_bWantToChangeWeapon && pWeapon != nullptr && pWeapon != getCurrentWeapon() && pWeapon->getWeaponIndex() )
		{
			selectWeapon(pWeapon->getWeaponIndex());
		}

		setLookAtTask(LOOK_ENEMY);

		///battack = true;

		if ( !handleAttack ( pWeapon, m_pEnemy ) )
		{
			m_pEnemy = nullptr;
			m_pOldEnemy = nullptr;
			wantToShoot(false);
		}
	}
}

// update some edicts in my memory if I see them or not
bool CHLDMBot :: setVisible ( edict_t *pEntity, const bool bVisible )
{
	static float fDist;

	const bool bValid = CBot::setVisible(pEntity,bVisible);

	fDist = distanceFrom(pEntity);

	// if no draw effect it is invisible
	if ( bValid && bVisible && !(CClassInterface::getEffects(pEntity)&EF_NODRAW) )
	{
		const char* szClassname = pEntity->GetClassName();

		if ( std::strncmp(szClassname,"item_ammo",9) == 0 )
		{
			if ( !m_pAmmoKit.get() || fDist < distanceFrom(m_pAmmoKit.get()) )
				m_pAmmoKit = pEntity;
		}
		else if ( std::strncmp(szClassname,"item_health",11) == 0 )
		{
			if ( !m_pHealthKit.get() || fDist < distanceFrom(m_pHealthKit.get()) )
				m_pHealthKit = pEntity;
		}
		else if ( std::strcmp(szClassname,"item_battery") == 0 )
		{
			if ( !m_pBattery.get() || fDist < distanceFrom(m_pBattery.get()) )
				m_pBattery = pEntity;
		}
		else if ( ( std::strcmp(szClassname,"func_breakable") == 0 || std::strncmp(szClassname,"prop_physics",12)==0 ) && CClassInterface::getPlayerHealth(pEntity)>0 )
		{
			if ( !m_NearestBreakable.get() || fDist < distanceFrom(m_NearestBreakable.get()) )
				m_NearestBreakable = pEntity;
		}
		else if ( pEntity != m_pNearestButton && std::strcmp(szClassname,"func_button") == 0 )
		{
			if ( !m_pNearestButton.get() || fDist < distanceFrom(m_pNearestButton.get()) )
				m_pNearestButton = pEntity;
		}
		// covered above
		/*else if ( (pEntity != m_pNearestBreakable) && ( std::strcmp(szClassname,"func_breakable")==0 ) )
		{
			if ( !m_pNearestBreakable.get() || (fDist<distanceFrom(m_pNearestBreakable.get())) )
				m_pNearestBreakable = pEntity;
		}*/
		else if ( pEntity != m_pAmmoCrate && std::strcmp(szClassname,"item_ammo_crate") == 0 )
		{
			if ( !m_pAmmoCrate.get() || fDist < distanceFrom(m_pAmmoCrate.get()) )
				m_pAmmoCrate = pEntity;
		}
		else if ( pEntity != m_FailedPhysObj && std::strncmp(szClassname,"prop_physics",12) == 0 )
		{
			if ( !m_NearestPhysObj.get() || fDist < distanceFrom(m_NearestPhysObj.get()) )
				m_NearestPhysObj = pEntity;
		}
		else if ( std::strncmp(szClassname,"item_suitcharger",16) == 0 )
		{
			if ( !m_pCharger.get() || fDist < distanceFrom(m_pCharger.get()) )
			{
				if ( m_pCharger.get() )
				{
					// less juice than the current one I see
					if ( CClassInterface::getAnimCycle(m_pCharger) < CClassInterface::getAnimCycle(pEntity) )
					{
						return bValid;
					}
				}

				if ( m_pPlayerInfo->GetArmorValue() < 50 )
					updateCondition(CONDITION_CHANGED);

				m_pCharger = pEntity;
			}
		}
		else if ( std::strncmp(szClassname,"item_healthcharger",18) == 0 )
		{
			if ( !m_pHealthCharger.get() || fDist < distanceFrom(m_pHealthCharger.get()) )
			{
				if ( m_pHealthCharger.get() )
				{
					// less juice than the current one I see - forget it
					if ( CClassInterface::getAnimCycle(m_pHealthCharger) < CClassInterface::getAnimCycle(pEntity) )
					{
						return bValid;
					}
				}

				if ( getHealthPercent() < 1.0f )
					updateCondition(CONDITION_CHANGED); // update tasks

				m_pHealthCharger = pEntity;
			}
		}
		else if ( std::strncmp(szClassname,"weapon_",7) == 0 )
		{
			if ( !m_pNearbyWeapon.get() || fDist < distanceFrom(m_pNearbyWeapon.get()) )
				m_pNearbyWeapon = pEntity;
		}
	}
	else
	{
		if ( m_pAmmoKit == pEntity )
			m_pAmmoKit = nullptr;
		else if ( m_pAmmoCrate == pEntity )
			m_pAmmoCrate = nullptr;
		else if ( m_pHealthKit == pEntity )
			m_pHealthKit = nullptr;
		else if ( m_pBattery == pEntity )
			m_pBattery = nullptr;
		else if ( m_NearestPhysObj == pEntity )
			m_NearestPhysObj = nullptr;
		else if ( m_pCharger == pEntity )
			m_pCharger = nullptr;
		else if ( m_pHealthCharger == pEntity )
			m_pHealthCharger = nullptr;
		else if ( m_NearestBreakable == pEntity )
			m_NearestBreakable = nullptr;
		else if ( m_pNearbyWeapon == pEntity )
			m_pNearbyWeapon = nullptr;
		else if ( m_pNearestButton == pEntity )
			m_pNearestButton = nullptr;
		//else if ( m_pNearestBreakable == pEntity )
		//	m_pNearestBreakable = nullptr;
	}

	return bValid;
}

// lost my enemy - rethink my next move by flushiing schedules
void CHLDMBot :: enemyLost (edict_t *pEnemy)
{
	updateCondition(CONDITION_CHANGED);
	//m_pSchedules->freeMemory();
}