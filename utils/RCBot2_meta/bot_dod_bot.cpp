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
#include "bot.h"
#include "bot_cvars.h"
#include "ndebugoverlay.h"
#include "bot_squads.h"
#include "bot_dod_bot.h"
#include "in_buttons.h"
#include "bot_buttons.h"
#include "bot_globals.h"
#include "bot_profile.h"
#include "bot_getprop.h"
#include "bot_mtrand.h"
#include "bot_task.h"
#include "bot_schedule.h"
#include "bot_weapons.h"
#include "bot_waypoint.h"
#include "bot_waypoint_locations.h"
#include "bot_navigator.h"
#include "bot_perceptron.h"
#include "bot_waypoint_visibility.h"

#include <cmath>
#include <cstring>

const char *g_DODClassCmd[2][6] = 
{ {"cls_garand","cls_tommy","cls_bar","cls_spring","cls_30cal","cls_bazooka"},
{"cls_k98","cls_mp40","cls_mp44","cls_k98s","cls_mg42","cls_pschreck"} };

// could be a bomb 
void CBroadcastBombEvent :: execute (CBot *pBot) 
{
	CDODBot *pDODBot = static_cast<CDODBot*>(pBot);

	pDODBot->bombEvent(DOD_BOMB_PLANT,m_iCP,m_iTeam);
}

// The lower the better
float CDODBot :: getEnemyFactor ( edict_t *pEnemy )
{
	float fPreFactor = CBot::getEnemyFactor(pEnemy);

	// not a player (breakable)
	if ( ENTINDEX(pEnemy) > gpGlobals->maxClients )
		fPreFactor += 1024.0f;
		
	return fPreFactor;
}


// could be a bomb or flag capture event
void CDODBot :: bombEvent ( int iEvent, int iCP, int iTeam )
{
	const int iWaypoint = CDODMod::m_Flags.getWaypointAtFlag(iCP);

	if ( iTeam && (iWaypoint != -1) )
	{
		m_pNavigator->beliefOne(iWaypoint,(iTeam == m_iTeam) ? BELIEF_SAFETY : BELIEF_DANGER,200.0f);
	}

	// this might be called twice within a second, 
	// make sure we only update tasks once per second at most
	if ( m_fLastCaptureEvent < engine->Time() )
	{
		updateCondition(CONDITION_CHANGED); // find new task

		m_fLastCaptureEvent = engine->Time() + 1.0f;
	}
}

CDODBot :: CDODBot()
{
	CBot();
	CDODBot::init(true);
}

void CDODBot :: init (bool bVarInit)
{
	CBot::init(bVarInit);

	m_iSelectedClass = -1;
	m_bCheckClass = false;
	m_pWantToProne = nullptr;
}

void CDODBot :: setup ()
{
	CBot::setup();

	if ( m_pWantToProne == nullptr)
		m_pWantToProne = new CPerceptron(3); // health , distance from enemy, danger out of 255
}

void CDODBot :: freeMapMemory ()
{
	delete m_pWantToProne;

	m_pWantToProne = nullptr;

	CBot::freeMapMemory();
}

bool CDODBot::canGotoWaypoint(Vector vPrevWaypoint, CWaypoint *pWaypoint, CWaypoint *pPrev )
{
	if ( CBot::canGotoWaypoint(vPrevWaypoint,pWaypoint,pPrev) )
	{
		if ( (m_iTeam == TEAM_ALLIES) && pWaypoint->hasFlag(CWaypointTypes::W_FL_NOALLIES) )
		{
			return false;
		}
		if ( (m_iTeam == TEAM_AXIS) && pWaypoint->hasFlag(CWaypointTypes::W_FL_NOAXIS) )
		{
			return false;
		}
		if ( pWaypoint->hasFlag(CWaypointTypes::W_FL_BREAKABLE) )
		{
			edict_t *pBreakable = CDODMod::getBreakable(pWaypoint);

			if ( CBotGlobals::entityIsValid(pBreakable) )
			{
				if ( !CBotGlobals::isBreakableOpen(pBreakable) )
				{
					return m_pWeapons->hasExplosives();
				}
			}
		}
		else if ( pWaypoint->hasFlag(CWaypointTypes::W_FL_BOMB_TO_OPEN) )
		{
			edict_t *pBombTarget = CDODMod::getBombTarget(pWaypoint);

			if ( CBotGlobals::entityIsValid(pBombTarget) )
			{
				// if its blown we can go there
				if ( CClassInterface::getDODBombState(pBombTarget)==0 )
					return true;
				// if its for our team we can blow it up 
				if ( CClassInterface::getDODBombTeam(pBombTarget) == m_iTeam )
					return m_bHasBomb;

				return false;
			}
			// entity invalid -- maybe blown up and freed by engine
			return true;
		}

		return true;
	}

	return false;
}

#define UPDATE_VISIBLE_OBJECT(visobj,pent) if ( !(visobj).get() || (distanceFrom(pent)<distanceFrom(visobj)) ) { (visobj) = pent; }
#define UPDATE_VISIBLE_OBJECT_CONDITION(visobj,pent,condition)  if ( !(visobj).get() || (distanceFrom(pent)<distanceFrom(visobj)) ) { if ( condition ) { (visobj) = pent; }  }
#define NULLIFY_VISIBLE(visobj,pent,distance)  if ( (visobj) == (pent) ) { if ( !bValid || (distanceFrom(visobj)>(distance)) ) { (visobj) = NULL; } }
#define NULLIFY_VISIBLE_CONDITION(visobj,pent,distance,condition) if ( (visobj) == (pent) ) { if ( !bValid || (distanceFrom(visobj)>(distance)) || (condition) ) { (visobj) = NULL; } }

bool CDODBot :: setVisible ( edict_t *pEntity, bool bVisible )
{
	//static float fDist;
	static const char *szClassname;
	static bool bNoDraw;
	static bool bValid;
	static float fSmokeTime;

	static bool bFriendlyFire;

	bFriendlyFire = mp_friendlyfire.IsValid()? mp_friendlyfire.GetBool() : false;

	bValid = CBot::setVisible(pEntity,bVisible);

	szClassname = pEntity->GetClassName();

	bNoDraw = ((CClassInterface::getEffects(pEntity) & EF_NODRAW) == EF_NODRAW);

	if ( bVisible && !bNoDraw && bValid )
	{
		if ( (m_pNearestDeadTeamMate != pEntity) && (CClassInterface::getTeam(pEntity) == m_iTeam) && !CBotGlobals::entityIsAlive(pEntity) )
		{
			UPDATE_VISIBLE_OBJECT(m_pNearestDeadTeamMate,pEntity)
		}
		else if ( (m_pNearestBreakable != pEntity) && (std::strncmp(pEntity->GetClassName(),"prop_physics",12)==0) )
		{
			UPDATE_VISIBLE_OBJECT_CONDITION(m_pNearestBreakable,pEntity,CClassInterface::getPlayerHealth(pEntity) > 0)
		}
		else if ( (m_pNearestFlag != pEntity) && CDODMod::m_Flags.isFlag(pEntity) )
		{
			UPDATE_VISIBLE_OBJECT(m_pNearestFlag,pEntity)
		}
		else if ( (m_pNearestBomb != pEntity) && CDODMod::m_Flags.isBomb(pEntity) )
		{
			UPDATE_VISIBLE_OBJECT_CONDITION(m_pNearestBomb,pEntity,CClassInterface::getDODBombState(pEntity)!=0)
		}
		// grenade_smoke
		// 012345678
		// don't run away from smoke grenades
		else if ( (pEntity!=m_pEnemyGrenade) && (szClassname[8]!='s') && (std::strncmp(szClassname,"grenade",7) == 0 ) && 
			((CClassInterface::getGrenadeThrower(pEntity) == m_pEdict) || 
			 ((CClassInterface::getTeam(pEntity) == m_iEnemyTeam)||bFriendlyFire)))
		{
			UPDATE_VISIBLE_OBJECT(m_pEnemyGrenade,pEntity)
		}
		else if ( (pEntity!=m_pEnemyRocket) && (std::strncmp(szClassname,"rocket",6) == 0 ) && 
			(CClassInterface::getTeam(pEntity) == m_iEnemyTeam) )
		{
			UPDATE_VISIBLE_OBJECT(m_pEnemyRocket,pEntity)
		}
		else if ( (pEntity!=m_pNearestPathBomb) && CDODMod::isPathBomb(pEntity) && (CClassInterface::getDODBombState(pEntity)!=0) )
		{
			UPDATE_VISIBLE_OBJECT(m_pNearestPathBomb,pEntity)
		}
		else if ( (pEntity!=m_pNearestWeapon) && (std::strncmp(szClassname,"weapon_",7)==0) )
		{
			UPDATE_VISIBLE_OBJECT(m_pNearestWeapon,pEntity)
		}
	}
	else
	{
		NULLIFY_VISIBLE(m_pNearestFlag,pEntity,512.0f)
		NULLIFY_VISIBLE_CONDITION(m_pNearestBreakable,pEntity,CWaypointLocations::REACHABLE_RANGE,CClassInterface::getPlayerHealth(m_pNearestBreakable)<=0)
		NULLIFY_VISIBLE(m_pEnemyGrenade,pEntity,BLAST_RADIUS*2)
		NULLIFY_VISIBLE(m_pEnemyRocket,pEntity,BLAST_RADIUS*2)
		NULLIFY_VISIBLE_CONDITION(m_pNearestBomb,pEntity,BLAST_RADIUS*2,CClassInterface::getDODBombState(m_pNearestBomb)==0)
		NULLIFY_VISIBLE_CONDITION(m_pNearestPathBomb,pEntity,BLAST_RADIUS*2,CClassInterface::getDODBombState(m_pNearestPathBomb)==0)
		NULLIFY_VISIBLE(m_pNearestWeapon,pEntity,CWaypointLocations::REACHABLE_RANGE)
	}

	if ( !bNoDraw && (pEntity != m_pNearestSmokeToEnemy) && (std::strncmp(szClassname,"grenade_smoke",13) == 0) )
	{
		fSmokeTime = gpGlobals->curtime - CClassInterface::getSmokeSpawnTime(pEntity);

		if ( (fSmokeTime >= 1.0f) && (fSmokeTime <= 10.0f) )
		{
			if ( !m_pNearestSmokeToEnemy )
			{
				m_pNearestSmokeToEnemy = pEntity;
			}
			else if ( m_pEnemy && hasSomeConditions(CONDITION_SEE_CUR_ENEMY) )
			{
				if ( (distanceFrom(pEntity) < distanceFrom(m_pNearestSmokeToEnemy)) || 
					(distanceFrom(pEntity) < (distanceFrom(m_pEnemy)+SMOKE_RADIUS)) )
				{
					// math time - good lord!
					// choose the best smoke that is worthwhile for checking enemy
					if ( m_pNearestSmokeToEnemy && (std::fabs(DotProductFromOrigin(CBotGlobals::entityOrigin(pEntity))-
							  DotProductFromOrigin(CBotGlobals::entityOrigin(m_pEnemy))) <=
						 std::fabs(DotProductFromOrigin(CBotGlobals::entityOrigin(m_pNearestSmokeToEnemy))-
							  DotProductFromOrigin(CBotGlobals::entityOrigin(m_pEnemy)))) )
					{
						m_pNearestSmokeToEnemy = pEntity;
					}
				}
			}
		}
	}
	else if ( pEntity == m_pNearestSmokeToEnemy )
	{
		fSmokeTime = gpGlobals->curtime - CClassInterface::getSmokeSpawnTime(pEntity);

		if ( bNoDraw || ((fSmokeTime < 1.0f) || (fSmokeTime > rcbot_smoke_time.GetFloat())) )
			m_pNearestSmokeToEnemy = nullptr;
	}

	return bValid;
}

void CDODBot :: selectedClass ( int iClass )
{
	m_iSelectedClass = iClass;
}

bool CDODBot :: startGame ()
{
	static int iTeam;

	iTeam = m_pPlayerInfo->GetTeamIndex();

	// not joined a team?
	if ( (iTeam != 2) && (iTeam != 3) )
	{
		if ( (m_iDesiredTeam == 2) || (m_iDesiredTeam == 3) )
			m_pPlayerInfo->ChangeTeam(m_iDesiredTeam);
		else
		{
			// manual -- auto team
			if ( CBotGlobals::numPlayersOnTeam(TEAM_ALLIES,false) <= CBotGlobals::numPlayersOnTeam(TEAM_AXIS,false) )
				m_pPlayerInfo->ChangeTeam(TEAM_ALLIES);
			else
				m_pPlayerInfo->ChangeTeam(TEAM_AXIS);
		}

		return false;
	}

	if ( (m_iDesiredClass < 0) || (m_iDesiredClass > 5) )
		chooseClass(false);

	// not the correct class? and desired class is valid?
	if ( (m_iDesiredClass >= 0) && (m_iDesiredClass <= 5) && (m_iDesiredClass != CClassInterface::getPlayerClassDOD(m_pEdict)) )
	{
		kill();
		changeClass();

		return false;
	}

	//else if ( m_pProfile->m_iClass 
	//	engine->ClientCommand(m_pEdict,"joinclass %d\n",m_iDesiredClass); 

	/*if ( CClassInterface::getDesPlayerClassDOD(m_pEdict) == -1 )
	{
		//if ( m_iDesiredClass == 0 )
			engine->ClientCommand(m_pEdict,"joinclass %d",randomInt(0,5)); 
		//else
		//	engine->ClientCommand(m_pEdict,"joinclass %d",m_iDesiredClass);

		switch ( m_iDesiredClass )
		{
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		}
		engine->ClientCommand(m_pEdict,"joinclass %d",m_iDesiredClass); 

		return false;
	}*/

	//if ( m_pPlayerInfo->get

	return true;
}

void CDODBot :: killed ( edict_t *pVictim, char *weapon )
{
	CBot::killed(pVictim,weapon);

	if ( pVictim && CBotGlobals::entityIsValid(pVictim) )
		m_pNavigator->belief(CBotGlobals::entityOrigin(pVictim),getEyePosition(),bot_beliefmulti.GetFloat(),distanceFrom(pVictim),BELIEF_SAFETY);

	if ( (m_pEnemy==pVictim) )
	{
		ga_nn_value inputs[3] = {distanceFrom(m_pEnemy)/1000.0f,getHealthPercent(),m_fCurrentDanger/MAX_BELIEF};
		m_pWantToProne->input(inputs);
		m_pWantToProne->execute();

		if ( m_bProne )
			m_pWantToProne->train(1.0f);
		else
			m_pWantToProne->train(0.0f);
	}

	if ( (m_pLastEnemy == pVictim) && inSquad() && isSquadLeader() )
	{
		addVoiceCommand(DOD_VC_ENEMY_DOWN);
		//addVoiceCommand(DOD_VC_GOGOGO);
	}
}

void CDODBot :: died ( edict_t *pKiller, const char *pszWeapon )
{
	CBot::died(pKiller,pszWeapon);

	// check if I want to change class
	m_bCheckClass = true;

	if ( randomInt(0,1) )
		m_pButtons->attack();

	if ( pKiller )
	{
		if ( CBotGlobals::entityIsValid(pKiller) )
			m_pNavigator->belief(CBotGlobals::entityOrigin(pKiller),getEyePosition(),bot_beliefmulti.GetFloat(),distanceFrom(pKiller),BELIEF_DANGER);

		if ( (m_pEnemy==pKiller) )
		{
			ga_nn_value inputs[3] = {distanceFrom(m_pEnemy)/1000.0f,getHealthPercent(),m_fCurrentDanger/MAX_BELIEF};
			m_pWantToProne->input(inputs);
			m_pWantToProne->execute();

			if ( m_bProne )
				m_pWantToProne->train(0.0f);
			else
				m_pWantToProne->train(1.0f);
		}
	}
}

// TO COMPLETE
void CDODBot :: seeFriendlyDie ( edict_t *pDied, edict_t *pKiller, CWeapon *pWeapon )
{
	static CBotUtilities utils;

	utils.freeMemory();

	if ( (pKiller != m_pEdict) && pKiller && !m_pEnemy && !hasSomeConditions(CONDITION_SEE_CUR_ENEMY) && isEnemy(pKiller,false) )
	{
		static CWaypoint *pWpt;
		//bool bInvestigate = true;
		//bool bFollow = true;
		const Vector vecEnemy = CBotGlobals::entityOrigin(pKiller);
		
		if ( pWeapon )
		{
			const DOD_Class pclass = static_cast<DOD_Class>(CClassInterface::getPlayerClassDOD(pKiller));

			if ( (pclass == DOD_CLASS_SNIPER) && pWeapon->isZoomable() )
			{
				if ( (m_LastHearVoiceCommand == DOD_VC_SNIPER) && m_pWeapons->hasWeapon(DOD_WEAPON_FRAG_US) && !m_pWeapons->hasWeapon(DOD_WEAPON_FRAG_GER) )
					addVoiceCommand(DOD_VC_USE_GRENADE);
				else
					addVoiceCommand(DOD_VC_SNIPER);

				updateCondition(CONDITION_COVERT);
				//m_pNavigator->belief
				m_fCurrentDanger += 100.0f; // sniper danger
				//bInvestigate = false;
				// Find Hide Spot
				ADD_UTILITY_DATA_VECTOR(BOT_UTIL_SNIPE_POINT,!hasEnemy() && (m_iClass == DOD_CLASS_SNIPER) && getSniperRifle() && !getSniperRifle()->outOfAmmo(this),1.0f,reinterpret_cast<unsigned>(pKiller),vecEnemy)
			}
			else if ( (pclass == DOD_CLASS_MACHINEGUNNER) && pWeapon->isDeployable() )
			{
				if ( (m_LastHearVoiceCommand == DOD_VC_MGAHEAD) && m_pWeapons->hasWeapon(DOD_WEAPON_FRAG_US) && !m_pWeapons->hasWeapon(DOD_WEAPON_FRAG_GER) )
					addVoiceCommand(DOD_VC_USE_GRENADE);
				else
					addVoiceCommand(DOD_VC_MGAHEAD);

				updateCondition(CONDITION_COVERT);
				m_pNavigator->belief(CBotGlobals::entityOrigin(pDied),CBotGlobals::entityOrigin(pKiller),100.0f,512.0f,BELIEF_DANGER);
				//updateCondition(CONDITION_CHANGED);
				m_fCurrentDanger = MAX_BELIEF; // machine gun danger
				//bInvestigate = false;

				ADD_UTILITY_DATA_VECTOR(BOT_UTIL_SNIPE_POINT,!hasEnemy() && (m_iClass == DOD_CLASS_SNIPER) && getSniperRifle() && !getSniperRifle()->outOfAmmo(this),1.0f,reinterpret_cast<unsigned>(pKiller),vecEnemy)
				//ADD_UTILITY_DATA_VECTOR(BOT_UTIL_MOVEUP_MG,!hasEnemy() && (m_iClass == DOD_CLASS_MACHINEGUNNER) && getMG() && !getMG()->outOfAmmo(this),1.0f,1,vecEnemy);
			}
			else
			{
				m_fCurrentDanger += 20.0f;
				//ADD_UTILITY_DATA_VECTOR(BOT_UTIL_SNIPE_POINT,m_iClass == DOD_CLASS_SNIPER,1.0f,1,vecEnemy);
			}

			if ( isVisible(pKiller) )
			{
				const CWaypointVisibilityTable *pTable = CWaypoints::getVisiblity();
				const int iCurrentWaypoint = m_pNavigator->getCurrentWaypointID();
				const int iEnemyWaypoint = CWaypointLocations::NearestWaypoint(CBotGlobals::entityOrigin(pKiller),100.0f,-1,true,true);

				if ( (iCurrentWaypoint!=-1) && (iEnemyWaypoint!=-1) && !pTable->GetVisibilityFromTo(iCurrentWaypoint,iEnemyWaypoint) )
				{
					//bFollow = false;

					ADD_UTILITY_DATA_VECTOR(BOT_UTIL_COVER_POINT,m_pCurrentWeapon != NULL,0.8f,(reinterpret_cast<unsigned>(pKiller)),(vecEnemy))
				}
			}
			/*else if ( CBotGlobals::isPlayer(pDied) )
			{
				// make a guess where the bullet came from
				IPlayerInfo *p = playerinfomanager->GetPlayerInfo(pDied);
				Vector v;
				CClassInterface::getVelocity(pDied,&v);
				
				if ( v.Length() > 0 )
					v = v / v.Length();

				Vector vend = (p->GetAbsOrigin() + v * 1024);

				CBotGlobals::quickTraceline(pDied,p->GetAbsOrigin(),vend);

				vend = CBotGlobals::getTraceResult()->endpos;

				int iWpt = CWaypointLocations::NearestWaypoint(vend,1024,-1,false,true);

				if ( iWpt != -1 )
				{
					CWaypoint *pWpt = CWaypoints::getPinchPointFromWaypoint(getOrigin(),CWaypoints::getWaypoint(iWpt)->getOrigin());

					if ( pWpt != NULL )
					{
						m_pSchedules->removeSchedule(SCHED_CROUCH_AND_HIDE);
						m_pSchedules->addFront(new CCrouchHideSched(pKiller));
						bFollow = false;
					}
				}
			}*/
		}

		// encourage bots to snoop out enemy or throw grenades
		m_fLastSeeEnemy = engine->Time();
		m_pLastEnemy = pKiller;
		m_fLastUpdateLastSeeEnemy = 0;
		m_vLastSeeEnemy = CBotGlobals::entityOrigin(m_pLastEnemy);
		m_vLastSeeEnemyBlastWaypoint = m_vLastSeeEnemy;

		pWpt = CWaypoints::getWaypoint(CWaypointLocations::NearestBlastWaypoint(m_vLastSeeEnemy,getOrigin(),1500.0f,-1,true,true,false,false,0,false));
			
		if ( pWpt )
		{
			m_vLastSeeEnemyBlastWaypoint = pWpt->getOrigin();
		}

		if ( inSquad() && isSquadLeader() )
		{
			addVoiceCommand(DOD_VC_HOLD);
		}

		if ( !hasEnemy() )
		{
			m_vListenPosition = CBotGlobals::entityOrigin(pKiller);
			CBotWeapon *pMachineGun = getMG();

			// move up MG
			if ( !rcbot_melee_only.GetBool() && pMachineGun && (m_iClass == DOD_CLASS_MACHINEGUNNER) && !pMachineGun->outOfAmmo(this) && pMachineGun->isDeployable() )
			{
				if ( !m_pSchedules->hasSchedule(SCHED_DEPLOY_MACHINE_GUN) )
				{
					CWaypoint *pWaypoint;
					Vector vSearchForMachineGunPointOrigin = m_vListenPosition-getOrigin();

					vSearchForMachineGunPointOrigin = vSearchForMachineGunPointOrigin/vSearchForMachineGunPointOrigin.Length();

					vSearchForMachineGunPointOrigin = getOrigin() + (vSearchForMachineGunPointOrigin*CWaypointLocations::REACHABLE_RANGE);

						if ( (pWaypoint = CWaypoints::getWaypoint(
						CWaypointLocations::NearestWaypoint(
						vSearchForMachineGunPointOrigin,
						CWaypointLocations::REACHABLE_RANGE,
						-1,
						false,false,
						false, nullptr,
						false,m_iTeam,
						false,false,
						Vector(0,0,0),
						CWaypointTypes::W_FL_MACHINEGUN))) != nullptr)
						{	
							ADD_UTILITY_WEAPON_DATA_VECTOR(BOT_UTIL_MOVEUP_MG,
							                               pMachineGun && !pMachineGun->outOfAmmo(this), 1.0f,
							                               pMachineGun, CWaypoints::getWaypointIndex(pWaypoint),
							                               m_vListenPosition)

							//m_pSchedules->add(new CDeployMachineGunSched(pMachineGun,pWaypoint,m_vListenPosition));

							//bFollow = false;
						}
				}
			}

			if ( !m_pSchedules->isCurrentSchedule(SCHED_INVESTIGATE_NOISE) )
			{
				ADD_UTILITY_DATA_VECTOR(BOT_UTIL_INVESTIGATE_POINT,!m_pSchedules->hasSchedule(SCHED_DEPLOY_MACHINE_GUN)&&!m_pSchedules->hasSchedule(SCHED_SNIPE),0.5f,reinterpret_cast<unsigned>(pDied),m_vListenPosition)

				//m_pSchedules->removeSchedule(SCHED_INVESTIGATE_NOISE);
				//m_pSchedules->addFront(new CBotInvestigateNoiseSched(CBotGlobals::entityOrigin(pDied),m_vListenPosition));
			}

			m_bListenPositionValid = true;
			m_fListenTime = engine->Time() + randomFloat(1.0f,2.0f);
			setLookAtTask(LOOK_NOISE);
			m_fLookSetTime = m_fListenTime;
			
			//listenToPlayer(pDied);
		}

		utils.execute();
		CBotUtility *next;

		while ( (next = utils.nextBest()) != nullptr)
		{
			if ( executeAction(next) )
			{
				m_CurrentUtil = next->getId();

				if ( m_fUtilTimes[next->getId()] < engine->Time() )
					m_fUtilTimes[next->getId()] = engine->Time() + randomFloat(0.1f,2.0f); // saves problems with consistent failing

				if ( CClients::clientsDebugging(BOT_DEBUG_UTIL) )
					CClients::clientDebugMsg(BOT_DEBUG_UTIL,g_szUtils[next->getId()],this);
			
				break;
			}
		}

		utils.freeMemory();

		//if ( !bInvestigate && !bFollow )
		//	updateCondition(CONDITION_CHANGED);

		if ( (m_pEnemy==pKiller) )
		{
			ga_nn_value inputs[3] = {distanceFrom(m_pEnemy)/1000.0f,getHealthPercent(),m_fCurrentDanger/MAX_BELIEF};
			m_pWantToProne->input(inputs);
			m_pWantToProne->execute();

			if ( m_bProne )
				m_pWantToProne->train(0.0f);
			else
				m_pWantToProne->train(1.0f);
		}
	}
}

void CDODBot :: seeFriendlyKill ( edict_t *pTeamMate, edict_t *pDied, CWeapon *pWeapon )
{
	static CWaypoint *pWpt; //Unused? [APG]RoboCop[CL]

	if ( (pDied != m_pEdict) && pTeamMate && !hasSomeConditions(CONDITION_SEE_CUR_ENEMY) && (CClassInterface::getTeam(pDied)!=m_iTeam) )
	{
		m_fLastSeeEnemy = engine->Time();

		if ( pWeapon )
		{
			static CBotWeapon *pCurrentWeapon;
			const DOD_Class pclass = static_cast<DOD_Class>(CClassInterface::getPlayerClassDOD(pTeamMate));
			//DOD_Class pclassdead = (DOD_Class)CClassInterface::getPlayerClassDOD(pDied);

			pCurrentWeapon = getCurrentWeapon();

			if ( pCurrentWeapon && (pWeapon->getID() == pCurrentWeapon->getID()) )
				m_fCurrentDanger -= 50.0f;
			else
				m_fCurrentDanger -= 20.0f;

			if ( m_fCurrentDanger < 0 )
				m_fCurrentDanger = 0;

			if ( (pclass == DOD_CLASS_MACHINEGUNNER) && pWeapon->isDeployable() )
			{
				removeCondition(CONDITION_COVERT);
				m_pNavigator->belief(CBotGlobals::entityOrigin(pTeamMate),CBotGlobals::entityOrigin(pDied),MAX_BELIEF,512.0f,BELIEF_SAFETY);
				updateCondition(CONDITION_CHANGED);
				m_fCurrentDanger = 10; // machine gun danger
			}
		}

		if ( pDied == m_pEnemy )
		{
			if ( ( getHealthPercent() < 0.2f ) && ( randomFloat(0.0f,1.0f) > 0.75f ) )
				addVoiceCommand(DOD_VC_NICE_SHOT);

			ga_nn_value inputs[3] = {distanceFrom(m_pEnemy)/1000.0f,getHealthPercent(),m_fCurrentDanger/MAX_BELIEF};
			m_pWantToProne->input(inputs);
			m_pWantToProne->execute();

			if ( m_bProne )
				m_pWantToProne->train(1.0f);
			else
				m_pWantToProne->train(0.0f);		
		}

		if ( m_pLastEnemy == pDied )
		{
			m_pLastEnemy = nullptr;
			m_fLastSeeEnemy = 0.0f;

			if ( inSquad() && isSquadLeader() )
			{
				addVoiceCommand(DOD_VC_ENEMY_DOWN);
				//addVoiceCommand(DOD_VC_GOGOGO);
			}
		}
	}
}

void CDODBot :: dropAmmo ()
{
	m_bDroppedAmmoThisRound = true;
	helpers->ClientCommand(m_pEdict,"dropammo");
}

// use weapon ID later, use getCurrentWeapon for now
bool CDODBot :: wantToListenToPlayerAttack ( edict_t *pPlayer, int iWeaponID )
{
	const edict_t *pentWeapon = CClassInterface::getCurrentWeapon(pPlayer);

	if ( pentWeapon != nullptr)
	{
		const CWeapon *pWeapon = CWeapons::getWeapon(pentWeapon->GetClassName());

		if ( pWeapon )
		{
			if ( pWeapon->isMelee() )
				return false;
			if ( pWeapon->isGrenade() )
				return false;

			// otherwise just random
			return true;
		}

	}

	return false;
}

void CDODBot :: spawnInit ()
{
	CBot::spawnInit();

	m_bDroppedAmmoThisRound = false;

	m_fNextCheckAlone = 0.0f;
	m_fNextCheckNeedAmmo = 0.0f;

	m_uSquadDetail.b1.said_area_clear = true;

	m_fLastRunForCover = 0.0f;

	m_fLastCaptureEvent = 0.0f;

	m_bHasBomb = false;
	m_pNearestBreakable = nullptr;

	m_pNearestPathBomb = nullptr;

	m_pNearestBomb = nullptr;

	std::memset(m_CheckSmoke,0,sizeof(smoke_t)*RCBOT_MAXPLAYERS);

	while ( !m_nextVoicecmd.empty() )
		m_nextVoicecmd.pop();

	m_fNextVoiceCommand = 0.0f;

	m_fDeployMachineGunTime = 0.0f;
	m_pNearestFlag = nullptr;

	m_fShoutRocket = 0.0f;
	m_fShoutGrenade = 0.0f;
	m_pEnemyRocket = nullptr;
	m_pEnemyGrenade = nullptr;

	m_fShootTime = 0.0f;
	m_fProneTime = 0.0f;
	m_fZoomOrDeployTime = 0.0f;

	if ( m_pWeapons )
		m_pWeapons->clearWeapons();

	m_CurrentUtil = BOT_UTIL_MAX;
	// reset objects
	m_flSprintTime = 0.0f;
	m_pCurrentWeapon = nullptr;
	m_fFixWeaponTime = 0.0f;
	m_LastHearVoiceCommand = DOD_VC_INVALID;
}

bool CDODBot :: isEnemy ( edict_t *pEdict,bool bCheckWeapons )
{
	const int entity_index = ENTINDEX(pEdict);
//#ifdef _DEBUG
//	const char *pszClassname = pEdict->GetClassName();

//#endif

	if ( entity_index == 0 )
		return false; // worldspawn

	if ( entity_index > gpGlobals->maxClients )
	{
		const bool bRegisteredBreakable = CDODMod::isBreakableRegistered(pEdict, m_iTeam);

		if ( !CBotGlobals::isBreakableOpen(pEdict) && ((pEdict == m_pNearestBreakable) || bRegisteredBreakable) )
		{

			/*if ( std::strcmp("dod_ragdoll",pszClassname) == 0 )
			{
				// break;
				return false;
			}*/

			if ( rcbot_shoot_breakables.GetBool() )
			{ 
				if ( bRegisteredBreakable )  // this breakable is registered as explosive only
				{
					return (distanceFrom(pEdict) > BLAST_RADIUS) && m_pWeapons->hasExplosives();
				}
				//else if ( (m_fLastSeeEnemy + 5.0f) > engine->Time() )
				if ( DotProductFromOrigin(CBotGlobals::entityOrigin(pEdict)) > rcbot_shoot_breakable_cos.GetFloat() )
					return ((m_fLastSeeEnemyPlayer+3.0f) < engine->Time()) && (distanceFrom(pEdict) < rcbot_shoot_breakable_dist.GetFloat()) && (CClassInterface::getPlayerHealth(pEdict) > 0);
			}
		}

		return false;
	}

	if ( !pEdict )
		return false;

	if ( !pEdict->GetUnknown() )
		return false; // left the server

	// if no target on - listen sever player is a non target
	if ( rcbot_notarget.GetBool() && (entity_index == 1) )
		return false;

	if ( pEdict == m_pEdict )
		return false;

	// not alive -- false
	if ( !CBotGlobals::entityIsAlive(pEdict) )
		return false;

	if ( CBotGlobals::getTeam(pEdict) == getTeam() )
	{
		if ( rcbot_ffa.GetBool() == false )
			return false;

		// if true continue down -- don't return
	}

	if ( bCheckWeapons && m_pNearestSmokeToEnemy )
	{
		if ( !isVisibleThroughSmoke(m_pNearestSmokeToEnemy,pEdict) )
			return false;
	}

	return true;	
}

void CDODBot :: handleWeapons ()
{
	//	
	// Handle attacking at this point
	//
	if ( m_pEnemy && !hasSomeConditions(CONDITION_ENEMY_DEAD) && 
		hasSomeConditions(CONDITION_SEE_CUR_ENEMY) && wantToShoot() && 
		isVisible(m_pEnemy) && isEnemy(m_pEnemy,true) )
	{
		CBotWeapon* pWeapon = getCurrentWeapon();

		if ( pWeapon && pWeapon->getWeaponEntity() && !rcbot_melee_only.GetBool() && pWeapon->isDeployable() && !pWeapon->outOfAmmo(this) && CClassInterface::isMachineGunDeployed(pWeapon->getWeaponEntity()))
		{
			;// keep current weapon on
		}
		else 
			pWeapon = getBestWeapon(m_pEnemy,true,true,rcbot_melee_only.GetBool(),!rcbot_melee_only.GetBool() && (((m_pEnemy == m_pNearestBreakable) || CDODMod::isBreakableRegistered(m_pEnemy,m_iTeam))));


		if ( m_bWantToChangeWeapon && (pWeapon != nullptr) && (pWeapon != getCurrentWeapon()) && pWeapon->getWeaponIndex() )
		{
			selectBotWeapon(pWeapon);
		}

		setLookAtTask(LOOK_ENEMY);

		if ( !handleAttack ( pWeapon, m_pEnemy ) )
		{
			m_pEnemy = nullptr;
			m_pOldEnemy = nullptr;
			wantToShoot(false);
		}
	}
}

void CDODBot :: touchedWpt ( CWaypoint *pWaypoint, int iNextWaypoint, int iPrevWaypoint )
{
	static int wptindex; //Unused? [APG]RoboCop[CL]

	CBot::touchedWpt(pWaypoint);

	wptindex = CWaypoints::getWaypointIndex(pWaypoint);

	if ( pWaypoint->hasFlag(CWaypointTypes::W_FL_BOMBS_HERE) )
	{
		//removeCondition(CONDITION_NEED_BOMB);
		//m_bHasBomb = true;
	}
	else if ( pWaypoint->hasFlag(CWaypointTypes::W_FL_BOMB_TO_OPEN) )
	{
		edict_t *pBombTarget = CDODMod::getBombTarget(pWaypoint);

		if ( CBotGlobals::entityIsValid(pBombTarget) )
		{
			Vector vBombTarget = CBotGlobals::entityOrigin(pBombTarget);  // `vBombTarget` Unused? [APG]RoboCop[CL]
			const int state = CClassInterface::getDODBombState(pBombTarget);

			if ( state != 0 )
				m_pNearestPathBomb = pBombTarget;

			// find bomb target for this waypoint and place bomb
			if ( pBombTarget && (state != 0) && (CClassInterface::getDODBombTeam(pBombTarget) == m_iTeam) )
			{
				// check if someone isn't bombing already
				// if ( (state == 2) || CDODMod::m_Flags.isTeamMatePlanting(m_pEdict,m_iTeam,pWaypoint->getOrigin()) )
				if ( (state == 2) || CDODFlags::isTeamMatePlanting(m_pEdict,m_iTeam,pWaypoint->getOrigin()) )
				{
					CBotSchedule *bombsched = new CBotSchedule();
					//todob
					m_pSchedules->freeMemory();

					bombsched->setID(SCHED_GOOD_HIDE_SPOT);

					bombsched->addTask(new CDODWaitForBombTask(pBombTarget,pWaypoint));

					m_pSchedules->add(bombsched);
				}
				else if ( m_bHasBomb ) // state must be 1 and no team mate planting
				// plant
				{
					CBotSchedule *bombsched = new CBotSchedule();
					//todob
					m_pSchedules->freeMemory();

					bombsched->setID(SCHED_BOMB);

					bombsched->addTask(new CBotDODBomb(DOD_BOMB_PATH_PLANT,-1,pBombTarget,CBotGlobals::entityOrigin(pBombTarget),-1));
					bombsched->addTask(new CDODWaitForBombTask(pBombTarget,pWaypoint));

					m_pSchedules->add(bombsched);
				}
				else
				{
					updateCondition(CONDITION_NEED_BOMB);
					updateCondition(CONDITION_CHANGED);
				}
			}
		}
	}
	else
	{
		
		// normal waypoint
		// Check if current waypoint has a path which is invisible to my next waypoint
		if ( (iPrevWaypoint != -1) && (iNextWaypoint != -1) && (randomFloat(0,MAX_BELIEF) < m_pNavigator->getBelief(CWaypoints::getWaypointIndex(pWaypoint))) )
		{
			const CWaypointVisibilityTable *pTable = CWaypoints::getVisiblity();
			WaypointList m_InvisPaths;
			const int iThisWaypoint = CWaypoints::getWaypointIndex(pWaypoint);
			//CWaypoint *pNextWaypoint = CWaypoints::getWaypoint(iNextWaypoint);

			for ( int i = 0; i < pWaypoint->numPaths(); i++ )
			{
				int iPath = pWaypoint->getPath(i);

				if ( iPath == iNextWaypoint )
					continue;
				if ( iPath == iPrevWaypoint )
					continue;

				CWaypoint* pPath = CWaypoints::getWaypoint(iPath);

				if ( pPath == pWaypoint )
					continue;

				if ( pPath->hasFlag(CWaypointTypes::W_FL_UNREACHABLE) )
					continue;
				/*
				static Vector vecLOS;
				static float flDot;

				Vector vForward = pNextWaypoint->getOrigin() - pWaypoint->getOrigin();
				vForward = vForward/vForward.Length();

				vecLOS = pWaypoint->getOrigin() - pPath->getOrigin();
				vecLOS = vecLOS/vecLOS.Length();
	
				flDot = DotProduct (vecLOS , vForward );
	
				if ( flDot > rcbot_dod_investigatepath_dp.GetFloat() )
					continue;*/
					
				if (  (pTable->GetVisibilityFromTo(iPrevWaypoint,iPath) == false) && (pTable->GetVisibilityFromTo(iThisWaypoint,iPath) == true) && (pTable->GetVisibilityFromTo(iNextWaypoint,iPath) == false) )
				{
					m_InvisPaths.emplace_back(iPath);

#ifndef __linux__
					if ( CClients::clientsDebugging(BOT_DEBUG_TASK) )
					{
						extern IVDebugOverlay *debugoverlay;
						debugoverlay->AddLineOverlay(CWaypoints::getWaypoint(iNextWaypoint)->getOrigin(),pPath->getOrigin(),255,120,120,false,7.0f);
						debugoverlay->AddLineOverlay(pWaypoint->getOrigin(),pPath->getOrigin(),255,255,255,false,7.0f);
					}

#endif
//					debugoverlay->AddTextOverlayRGB((pWaypoint->getOrigin()+pPath->getOrigin())/2,0,7.0f,255,255,255,255,"Dot: %0.3f",flDot);
				}
				
			}

			if (!m_InvisPaths.empty())
			{
				int iCheck = randomInt(0,m_InvisPaths.size()-1);

				iCheck = m_InvisPaths[iCheck];

				if ( !m_pSchedules->hasSchedule(SCHED_INVESTIGATE_HIDE) )
				{
					// waypoint to check is this one "iCheck"
					CBotSchedule *pSched = new CBotSchedule();

					pSched->setID(SCHED_INVESTIGATE_HIDE);
					pSched->addTask(new CBotInvestigateHidePoint(iCheck,CWaypoints::getWaypointIndex(pWaypoint)));

					m_pSchedules->addFront(pSched);				
				}
			}
		}
	}

	/*else if ( m_pEnemy && hasSomeConditions(CONDITION_SEE_CUR_ENEMY) ) 
	{
		m_pNavigator->beliefOne(wptindex,BELIEF_DANGER,distanceFrom(m_pEnemy));
	}
	else
		m_pNavigator->beliefOne(wptindex,BELIEF_SAFETY,0);*/
}

void CDODBot :: changeClass ()
{
	const int iTeam = getTeam();
	// change class
	//selectClass();
	helpers->ClientCommand(m_pEdict,g_DODClassCmd[iTeam-2][m_iDesiredClass]);

	m_fChangeClassTime = engine->Time() + randomFloat(bot_min_cc_time.GetFloat(),bot_max_cc_time.GetFloat());
}

void CDODBot :: chooseClass ( bool bIsChangingClass )
{
	const int _forcedClass = rcbot_force_class.GetInt();
	if (_forcedClass > 0 && _forcedClass < 10)
	{
		switch (_forcedClass)
		{
		case 1:
			m_iDesiredClass = DOD_CLASS_RIFLEMAN;
			break;
		case 2:
			m_iDesiredClass = DOD_CLASS_ASSAULT;
			break;
		case 3:
			m_iDesiredClass = DOD_CLASS_SUPPORT;
			break;
		case 4:
			m_iDesiredClass = DOD_CLASS_SNIPER;
			break;
		case 5:
			m_iDesiredClass = DOD_CLASS_MACHINEGUNNER;
			break;
		case 6:
			m_iDesiredClass = DOD_CLASS_ROCKET;
			break;
		}
	}
	else
	{
		float fClassFitness[6]; // 6 classes
		float fTotalFitness = 0;

		const int iTeam = getTeam();

		//TODO: allow bots to choose less heavy classes [APG]RoboCop[CL]
		fClassFitness[DOD_CLASS_RIFLEMAN] *= 1.5f;
		fClassFitness[DOD_CLASS_ASSAULT] *= 1.2f;
		fClassFitness[DOD_CLASS_SUPPORT] *= 1.0f;
		fClassFitness[DOD_CLASS_SNIPER] *= 0.6f;
		fClassFitness[DOD_CLASS_MACHINEGUNNER] *= 0.8f;
		fClassFitness[DOD_CLASS_ROCKET] *= 0.5f;

		for (float& fClassFitnes : fClassFitness)
		{
			fClassFitnes = 1.0f;
		}

		if (bIsChangingClass && ((m_iClass >= 0) && (m_iClass < 6)))
			fClassFitness[m_iClass] = 0.1f;

		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			edict_t* pPlayer = INDEXENT(i);

			if (CBotGlobals::entityIsValid(pPlayer) && (CClassInterface::getTeam(pPlayer) == iTeam))
			{
				const int iClass = CClassInterface::getPlayerClassDOD(pPlayer);

				if ((iClass >= 0) && (iClass < 6))
					fClassFitness[iClass] *= 0.6f;
			}
		}

		for (int i = 0; i < 6; i++)
			fTotalFitness += fClassFitness[i];

		const float fRandom = randomFloat(0, fTotalFitness);

		fTotalFitness = 0.0f;

		m_iDesiredClass = 0;

		for (int i = 0; i < 6; i++)
		{
			fTotalFitness += fClassFitness[i];

			if (fRandom <= fTotalFitness)
			{
				m_iDesiredClass = i;
				break;
			}
		}
	}
}

void CDODBot :: prone ()
{
	if ( !hasSomeConditions(CONDITION_RUN) && !m_bProne && (m_fProneTime < engine->Time()) )
	{
		m_pButtons->tap(IN_ALT1);
		m_fProneTime = engine->Time() + randomFloat(4.0f,8.0f);
	}
}
void CDODBot :: unProne()
{
	if ( m_bProne && (hasSomeConditions(CONDITION_RUN) || (m_fProneTime < engine->Time())) )
	{
		m_pButtons->tap(IN_ALT1);
		m_fProneTime = engine->Time() + randomFloat(4.0f,8.0f);
	}
}

void CDODBot :: modThink ()
{
	static float fMaxSpeed;
	static CBotWeapon *pWeapon;

	// when respawned -- check if I should change class
	if ( m_bCheckClass && !m_pPlayerInfo->IsDead())
	{
		m_bCheckClass = false;

		if ( bot_change_class.GetBool() && (m_fChangeClassTime < engine->Time()) )
		{
				// get score for this class
				const float scoreValue = CDODMod::getScore(m_pEdict);

				// if I think I could do better
				if ( randomFloat(0.0f,1.0f) > (scoreValue / CDODMod::getHighestScore()) )
				{
					chooseClass(true);
					changeClass();
				}
		}
	}

	m_fFov = BOT_DEFAULT_FOV;
	fMaxSpeed = CClassInterface::getMaxSpeed(m_pEdict);//*rcbot_speed_boost.GetFloat();

	setMoveSpeed(fMaxSpeed);
	
	m_iClass = static_cast<DOD_Class>(CClassInterface::getPlayerClassDOD(m_pEdict));	
	m_iTeam = getTeam();

	m_bHasBomb = m_pWeapons->hasWeapon(DOD_WEAPON_BOMB);

	if ( m_iTeam == TEAM_ALLIES ) // im allies
		m_iEnemyTeam = TEAM_AXIS;
	else if ( m_iEnemyTeam == TEAM_AXIS ) // im axis and my enemy team is wrong
		m_iEnemyTeam = TEAM_ALLIES;

	m_pCurrentWeapon = CClassInterface::getCurrentWeapon(m_pEdict);
	pWeapon = getCurrentWeapon();

	if ( m_pCurrentWeapon )
	{
		CClassInterface::getWeaponClip(m_pCurrentWeapon,&m_iClip1,&m_iClip2);

		if ( pWeapon && pWeapon->isDeployable() && !pWeapon->outOfAmmo(this) )
		{
			if ( CClassInterface::isMachineGunDeployed(m_pCurrentWeapon) )
				m_fDeployMachineGunTime = engine->Time();
			else if (hasEnemy() && (m_StatsCanUse.stats.m_iEnemiesInRange>0))
			{
				// Not deployed machine gun and within enemies -- run for cover
				// run for cover
				if ( !m_pSchedules->isCurrentSchedule(SCHED_RUN_FOR_COVER) )
				{
					m_pSchedules->removeSchedule(SCHED_RUN_FOR_COVER);
					m_pSchedules->addFront(new CGotoHideSpotSched(this,m_pEnemy.get()));
				}
			}
		}

		if ( pWeapon && pWeapon->isZoomable() && CClassInterface::isSniperWeaponZoomed(m_pCurrentWeapon) )
		{
			m_fFov = 20.0f;
		}
	}


	if ( onLadder() )
	{
		setMoveLookPriority(MOVELOOK_OVERRIDE);
		setLookAtTask(LOOK_WAYPOINT);
		m_pButtons->holdButton(IN_FORWARD,0,1,0);
		setMoveLookPriority(MOVELOOK_MODTHINK);
	}

	CClassInterface::getPlayerInfoDOD(m_pEdict,&m_bProne,&m_flStamina);

	// going prone
	if ( !isUnderWater() && !hasSomeConditions(CONDITION_RUN) && (m_fCurrentDanger >= 50.0f) )
	{
		// not sniper rifle or machine gun but can look down the sights
		if ( hasSomeConditions(CONDITION_COVERT) && m_pCurrentWeapon && pWeapon && (( pWeapon->getID() == DOD_WEAPON_K98 ) || (pWeapon->getID() == DOD_WEAPON_GARAND) ))
		{
			bool bZoomed;

			if ( pWeapon->getID() == DOD_WEAPON_K98 )
				bZoomed = CClassInterface::isK98Zoomed(m_pCurrentWeapon);
			else
				bZoomed = CClassInterface::isGarandZoomed(m_pCurrentWeapon);

			if ( !bZoomed && (m_fZoomOrDeployTime < engine->Time()) )
			{
				secondaryAttack(); // deploy / zoom
				m_fZoomOrDeployTime = engine->Time() + randomFloat(0.1f,0.2f);
			}
		}
		// prone only if has enemy or last seen one a second ago
		// if rcbot_prone_enemy_only is true
		if ( (hasSomeConditions(CONDITION_PRONE) || !rcbot_prone_enemy_only.GetBool() || ((m_pEnemy.get()!= nullptr) || (m_fLastSeeEnemy + 5.0f > engine->Time()))) && (m_fCurrentDanger >= 80.0f) && !m_bProne && ( m_fProneTime < engine->Time() ))
		{
			bool bProne = true;

			if ( rcbot_prone_enemy_only.GetBool() && (m_pEnemy.get()!= nullptr) )
			{
				ga_nn_value inputs[3] = {distanceFrom(m_pEnemy)/1000.0f,getHealthPercent(),m_fCurrentDanger/MAX_BELIEF};
				m_pWantToProne->input(inputs);
				m_pWantToProne->execute();
				bProne = m_pWantToProne->fired();
			}

			if ( bProne )
			{
				prone();
			}
		}

		setMoveSpeed(fMaxSpeed/4);

		// slow down - be careful
	}
	else if ( hasSomeConditions(CONDITION_RUN) || ((m_fCurrentDanger >= 20.0f) && (m_flStamina >= 5.0f ) && (m_flSprintTime < engine->Time())) )
	{
		// unprone
		
		unProne();
		

		setMoveSpeed(fMaxSpeed*1.1f); 
		m_pButtons->holdButton(IN_SPEED,0,1,0);
		m_pButtons->holdButton(IN_FORWARD,0,1,0);
	}
	else if (( m_fCurrentDanger < 1 ) || (m_flStamina < 10.0f ))
	{
		m_flSprintTime = engine->Time() + randomFloat(2.0f,6.0f);
	}

	if ( (pWeapon && pWeapon->needToReload(this)) ||
		(m_fLastSeeEnemy && ((m_fLastSeeEnemy + 5.0f)<engine->Time())) )
	{
		m_fLastSeeEnemy = 0;
		m_pButtons->tap(IN_RELOAD);
	}

	if ( !m_bProne && m_bStatsCanUse )
	{
		if ( m_pSchedules->isEmpty() || (!m_pSchedules->getCurrentSchedule()->isID(SCHED_FOLLOW) && !m_pSchedules->getCurrentSchedule()->isID(SCHED_SNIPE)) )
		{
			if ( m_fNextCheckAlone < engine->Time() )
			{
				m_fNextCheckAlone = engine->Time() + 2.5f;

				if ( !inSquad() && (m_Stats.stats.m_iTeamMatesInRange < 1) && (m_Stats.stats.m_iTeamMatesVisible > 1) )
					addVoiceCommand(DOD_VC_STICK_TOGETHER);
			}
		}
	}

	if ( m_pEnemyRocket )
	{
		m_fShoutRocket = m_fShoutRocket/2 + 0.5f;

		if ( m_fShoutRocket > 0.95f ) 
		{
			addVoiceCommand(DOD_VC_BAZOOKA);

			updateCondition(CONDITION_RUN);

			m_fShoutRocket = 0.0f;

			if ( !m_pSchedules->isCurrentSchedule(SCHED_GOOD_HIDE_SPOT) )
			{
				// don't interrupt current shedule, just add to front
				m_pSchedules->removeSchedule(SCHED_GOOD_HIDE_SPOT);
				m_pSchedules->addFront(new CGotoHideSpotSched(this,m_pEnemyRocket,true));
			}
		}
	}

	// need ammo and see a teammate nearby
	if ( hasSomeConditions(CONDITION_NEED_AMMO) && (m_fNextCheckNeedAmmo < engine->Time()) && (m_StatsCanUse.stats.m_iTeamMatesVisible>0) && (m_StatsCanUse.stats.m_iTeamMatesInRange>0) )
	{
		addVoiceCommand(DOD_VC_NEED_AMMO);
		m_fNextCheckNeedAmmo = engine->Time() + randomFloat(30.0f,60.0f);
	}

	if ( m_pEnemyGrenade )
	{
		m_fShoutGrenade = m_fShoutGrenade/2 + 0.5f;

		if ( m_fShoutGrenade > 0.95f ) 
		{
			if ( distanceFrom(m_pEnemyGrenade) < (BLAST_RADIUS*2) )
			{
				if ( CClassInterface::getGrenadeThrower(m_pEnemyGrenade) != m_pEdict )
				{
					addVoiceCommand(DOD_VC_GRENADE2);
					m_fShoutGrenade = 0.0f;
				}

				updateCondition(CONDITION_RUN);

				if ( !m_pSchedules->isCurrentSchedule(SCHED_GOOD_HIDE_SPOT) )
				{
					// don't interrupt current shedule, just add to front
					m_pSchedules->removeSchedule(SCHED_GOOD_HIDE_SPOT);
					m_pSchedules->addFront(new CGotoHideSpotSched(this,m_pEnemyGrenade,true));
				}
			}
		}
	}

	if ( m_pNearestPathBomb )
	{
		// active bomb
		if ( CClassInterface::getDODBombState(m_pNearestPathBomb) == 2 )
		{
			if ( distanceFrom(m_pNearestPathBomb) < (BLAST_RADIUS*2) )
			{
				// run
				if ( !m_pSchedules->hasSchedule(SCHED_BOMB) && 
					!m_pSchedules->hasSchedule(SCHED_GOOD_HIDE_SPOT) )
				{
					
					CWaypoint *pWaypoint = CDODMod::getBombWaypoint(m_pNearestPathBomb);

					if ( pWaypoint )
					{
						CBotSchedule *runsched = new CBotSchedule(new CDODWaitForBombTask(m_pNearestPathBomb,pWaypoint));
						runsched->setID(SCHED_GOOD_HIDE_SPOT);

						updateCondition(CONDITION_RUN);

						m_pSchedules->freeMemory();
						m_pSchedules->add(runsched);
					}
				}
			}
		}
	}

	if ( m_pNearestBomb )
	{
		const int iBombID = CDODMod::m_Flags.getBombID(m_pNearestBomb);

		if ( CDODMod::m_Flags.canDefuseBomb(m_iTeam,iBombID) )
		{
			if ( !CDODMod::m_Flags.isTeamMateDefusing(m_pEdict,m_iTeam,iBombID) && !m_pSchedules->hasSchedule(SCHED_BOMB) )
			{
				CBotSchedule *attack = new CBotSchedule();

				attack->setID(SCHED_BOMB);
				attack->addTask(new CFindPathTask(m_pNearestBomb));
				attack->addTask(new CBotDODBomb(DOD_BOMB_DEFUSE,iBombID,m_pNearestBomb,CBotGlobals::entityOrigin(m_pNearestBomb),-1));
				// add defend task
				m_pSchedules->freeMemory();
				m_pSchedules->add(attack);

				removeCondition(CONDITION_PUSH);
				updateCondition(CONDITION_RUN);
			}
		}
		else if ( !m_pSchedules->hasSchedule(SCHED_BOMB) && 
			!m_pSchedules->hasSchedule(SCHED_GOOD_HIDE_SPOT) &&
			!CDODMod::m_Flags.isBombBeingDefused(iBombID) &&
			CDODMod::m_Flags.isBombPlanted(iBombID) &&
			CDODMod::m_Flags.isBombExplodeImminent(iBombID) )
		{
			const Vector vNearestBomb = CBotGlobals::entityOrigin(m_pNearestBomb);

			if ( distanceFrom(vNearestBomb) < (BLAST_RADIUS*2) )
			{
				// if ( !CDODMod::m_Flags.isTeamMatePlanting(m_pEdict,m_iTeam,vNearestBomb) )
				if ( !CDODFlags::isTeamMatePlanting(m_pEdict,m_iTeam,vNearestBomb) )
				{
					CWaypoint *pWaypoint = CDODMod::getBombWaypoint(m_pNearestBomb);

					if ( pWaypoint )
					{
						CBotSchedule *runsched = new CBotSchedule(new CDODWaitForBombTask(m_pNearestBomb,pWaypoint));
						runsched->setID(SCHED_GOOD_HIDE_SPOT);

						updateCondition(CONDITION_RUN);

						m_pSchedules->freeMemory();
						m_pSchedules->add(runsched);
					}
				}
			}
		}
	}
}

void CDODBot ::defending()
{
	// check to go prone or not
}

void CDODBot ::voiceCommand ( int cmd )
{
	// find voice command
	extern eDODVoiceCommand_t g_DODVoiceCommands[DOD_VC_INVALID];

	char scmd[64];
	u_VOICECMD vcmd;

	vcmd.voicecmd = cmd; //not used? [APG]RoboCop[CL]
	
	std::sprintf(scmd,"voice_%s",g_DODVoiceCommands[cmd].pcmd);

	helpers->ClientCommand(m_pEdict,scmd);
}

void CDODBot ::signal ( const char *signal ) const
{
	char scmd[64];

	std::sprintf(scmd,"signal_%s",signal);

	helpers->ClientCommand(m_pEdict,scmd);
}

void CDODBot :: friendlyFire ( edict_t *pEdict )
{
	if ( isVisible(pEdict) )
	{
		addVoiceCommand(DOD_VC_CEASEFIRE);
	}
}

#define IF_WANT_TO_LISTEN if ( isVisible(pPlayer) || (inSquad() && (m_pSquad->GetLeader()==pPlayer)) )

void CDODBot :: hearVoiceCommand ( edict_t *pPlayer, byte cmd )
{
	switch ( cmd )
	{
	case DOD_VC_CEASEFIRE:
		IF_WANT_TO_LISTEN
		{
			if ( mp_friendlyfire.IsValid() && mp_friendlyfire.GetBool() )
			{
				wantToShoot(false);  // don't shoot this frame
				m_pButtons->letGo(IN_ATTACK);
			}
		}
		break;
	case DOD_VC_AREA_CLEAR:
		IF_WANT_TO_LISTEN
		{
			// reduce danger
			m_fCurrentDanger = 0.0f;
			removeCondition(CONDITION_DEFENSIVE);
			removeCondition(CONDITION_COVERT);
		}
		break;
	case DOD_VC_NEED_MG:
		
		IF_WANT_TO_LISTEN
		{
			if ( hasMG() )
			{
				// find the nearest objective
				int iFlagID = CDODMod::m_Flags.findNearestObjective(CBotGlobals::entityOrigin(pPlayer));
				int iWaypointAtFlag = CDODMod::m_Flags.getWaypointAtFlag(iFlagID);

				if ( (iFlagID != -1) && (iWaypointAtFlag!=-1) )
				{	
					CWaypoint *pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_MACHINEGUN,m_iTeam,iFlagID,true,this);
					CWaypoint *pWaypointAtFlag = CWaypoints::getWaypoint(iWaypointAtFlag);

					if ( pWaypoint && pWaypointAtFlag )
					{
						CBotSchedule *snipe = new CBotSchedule();
						CBotTask *findpath = new CFindPathTask(CWaypoints::getWaypointIndex(pWaypoint));
						CBotTask *snipetask;
						CBotWeapon *pWeapon = getMG();
						Vector vGoal = pWaypointAtFlag->getOrigin();

						snipetask = new CBotDODSnipe(pWeapon, pWaypoint->getOrigin(), pWaypoint->getAimYaw(),
						                             iFlagID != -1, vGoal.z + 48, pWaypoint->getFlags());

						removeCondition(CONDITION_PUSH);
						findpath->setCompleteInterrupt(CONDITION_PUSH);
						snipetask->setCompleteInterrupt(CONDITION_PUSH);

						snipe->setID(SCHED_DEFENDPOINT);
						snipe->addTask(findpath);
						snipe->addTask(snipetask);
				
						m_pSchedules->freeMemory();
						m_pSchedules->add(snipe);

						addVoiceCommand(DOD_VC_YES);

						break;
					}
					if ( randomFloat(0.0f,1.0f) < 0.8f )
						addVoiceCommand(DOD_VC_NO);
				}
				else if ( randomFloat(0.0f,1.0f) < 0.6f )
					addVoiceCommand(DOD_VC_NO);
			}
			else if ( randomFloat(0.0f,1.0f) < 0.4f )
				addVoiceCommand(DOD_VC_NO);
		}
		else if ( randomFloat(0.0f,1.0f) < 0.2f )
				addVoiceCommand(DOD_VC_NO);
		break;
	case DOD_VC_DISPLACE:
		// leave squad
		if ( inSquad() && (m_pSquad->GetLeader()==pPlayer) )
		{
			if ( randomFloat(0.0f,1.0f) > 0.75f )
				addVoiceCommand(DOD_VC_YES);

			m_pSquad->removeMember(m_pEdict);
			clearSquad();
		}
		break;
	case DOD_VC_USE_GRENADE:
		IF_WANT_TO_LISTEN
		{
			if ( m_pWeapons->hasWeapon(DOD_WEAPON_FRAG_GER) ||  m_pWeapons->hasWeapon(DOD_WEAPON_FRAG_US)  )
			{
				updateCondition(CONDITION_GREN);
				updateCondition(CONDITION_CHANGED);

				if ( randomFloat(0.0f,1.0f) > 0.25f )
					addVoiceCommand(DOD_VC_YES);
			}
		}
		break;
	case DOD_VC_GO_LEFT:
	case DOD_VC_GO_RIGHT:
		IF_WANT_TO_LISTEN
		{
			Vector vOrigin = CBotGlobals::entityOrigin(pPlayer);
			IPlayerInfo *p;
			Vector vForward,vUp,vRight;

			p = playerinfomanager->GetPlayerInfo(pPlayer);

			if ( p )
			{
				Vector vCapSearchOrigin;
				CBotCmd usercmd;
				QAngle angles;
				usercmd = p->GetLastUserCommand();
				angles = usercmd.viewangles;
				AngleVectors(angles,&vForward,&vRight,&vUp);
				// find capture point nearest to where leader is pointing
				
				vCapSearchOrigin = vOrigin+(vForward*CWaypointLocations::BUCKET_SPACING*2);
				int iNearestPoint = CDODMod::m_Flags.findNearestObjective(vCapSearchOrigin);

				if ( cmd == DOD_VC_GO_LEFT )
					vOrigin = vOrigin - (vRight*(CWaypointLocations::BUCKET_SPACING+1)) + (vForward*(CWaypointLocations::BUCKET_SPACING+1));
				else
					vOrigin = vOrigin + (vRight*(CWaypointLocations::BUCKET_SPACING+1)) + (vForward*(CWaypointLocations::BUCKET_SPACING+1));

				// find route waypoint to right of leader
								
				CWaypoint *pWaypoint = CWaypoints::getWaypoint(CWaypointLocations::NearestWaypoint(vOrigin,768.0f,-1,false,false,false, nullptr,false,0,false,false,Vector(0,0,0),CWaypointTypes::W_FL_ROUTE, nullptr));

				if ( pWaypoint != nullptr)
				{
					if ( iNearestPoint != -1 )
					{
						CWaypoint *pCap = CWaypoints::getWaypoint(CDODMod::m_Flags.getWaypointAtFlag(iNearestPoint));

						if ( pCap != nullptr) // go here after going through route
							m_pSchedules->addFront(new CBotGotoOriginSched(pCap->getOrigin()));
					}

					m_pSchedules->addFront(new CBotGotoOriginSched(pWaypoint->getOrigin()));
				}
				else
				{
					// find capture waypoint to right of leader
					CWaypoint *pWaypoint = CWaypoints::getWaypoint(CWaypointLocations::NearestWaypoint(vOrigin,768.0f,-1,false,false,false, nullptr,false,0,false,false,Vector(0,0,0),CWaypointTypes::W_FL_CAPPOINT, nullptr));

					if ( pWaypoint != nullptr)
					{
						m_pSchedules->addFront(new CBotGotoOriginSched(pWaypoint->getOrigin()));
						addVoiceCommand(DOD_VC_YES);
					}					
					else
						addVoiceCommand(DOD_VC_NO);
				}
			}
		}
		break;
	case DOD_VC_ENEMY_DOWN:
	case DOD_VC_GOGOGO:
		IF_WANT_TO_LISTEN
		{
			updateCondition(CONDITION_PUSH);	 
			updateCondition(CONDITION_RUN);
			updateCondition(CONDITION_CHANGED);
			removeCondition(CONDITION_DEFENSIVE);

			if ( hasSomeConditions(CONDITION_DEFENSIVE) || (randomFloat(0.0f,1.0f) > 0.25f) )
				addVoiceCommand(DOD_VC_YES);
		} 
		else if ( randomFloat(0.0f,1.0f) > 0.9f )
			addVoiceCommand(DOD_VC_NO);
		break;
	case DOD_VC_HOLD:
		if ( inSquad() && (m_pSquad->GetLeader() == pPlayer) )
		{
			updateCondition(CONDITION_COVERT);
			removeCondition(CONDITION_PUSH);
			removeCondition(CONDITION_RUN);

			if ( randomFloat(0.0f,1.0f) > 0.25f )
				addVoiceCommand(DOD_VC_YES);

			// to improve
			m_pSquad->setTactic(TACTIC_DEFEND);

			updateCondition(CONDITION_DEFENSIVE);
			updateCondition(CONDITION_CHANGED);
		}
		else if ( isVisible(pPlayer) )
		{
			updateCondition(CONDITION_COVERT);
			removeCondition(CONDITION_PUSH);
			removeCondition(CONDITION_RUN);

			if ( randomFloat(0.0f,1.0f) > 0.75f )
				addVoiceCommand(DOD_VC_YES);
		}
		else if ( randomFloat(0.0f,1.0f) > 0.9f )
			addVoiceCommand(DOD_VC_NO);
		break;
	case DOD_VC_SMOKE:
		IF_WANT_TO_LISTEN
		{
			if ( m_pWeapons->hasWeapon(DOD_WEAPON_SMOKE_US) || m_pWeapons->hasWeapon(DOD_WEAPON_SMOKE_GER) )
			{
				if ( randomFloat(0.0f,1.0f) > 0.75f )
					addVoiceCommand(DOD_VC_YES);

				updateCondition(CONDITION_COVERT);
				updateCondition(CONDITION_CHANGED);
			}
		}
		
		if ( (!m_pWeapons->hasWeapon(DOD_WEAPON_SMOKE_US) && !m_pWeapons->hasWeapon(DOD_WEAPON_SMOKE_GER)) && (randomFloat(0.0f,1.0f) > 0.75f) )
		{
			addVoiceCommand(DOD_VC_NO);
		}
		break;
	case DOD_VC_SNIPER:
	case DOD_VC_MGAHEAD:
		IF_WANT_TO_LISTEN
		{
			updateCondition(CONDITION_COVERT);
			updateCondition(CONDITION_PARANOID);
			m_fCurrentDanger += 50.0f;
		}
		break;
	case DOD_VC_NEED_AMMO:
		// TODO: go to team mate and drop ammo
		// should drop ammo to this person?

		if ( !m_bDroppedAmmoThisRound )
		{
			// if this player is in my squad , do it
			// if this player is not in my squad ill only do it if I see him and he's nearby
			if ( (!inSquad() && (distanceFrom(pPlayer)<512.0f) && isVisible(pPlayer)) || (inSquad()&&m_pSquad->IsMember(pPlayer)) )
			{
				if ( !m_pSchedules->isCurrentSchedule(SCHED_DOD_DROPAMMO) )
				{
					if ( m_pSchedules->hasSchedule(SCHED_DOD_DROPAMMO) )
						m_pSchedules->removeSchedule(SCHED_DOD_DROPAMMO);

					CBotSchedule *pSched = new CBotSchedule();

					pSched->setID(SCHED_DOD_DROPAMMO);

					if ( !isVisible(pPlayer) || (distanceFrom(pPlayer)>200.0f) )
					{				
						CWaypoint *pWpt = CWaypoints::getWaypoint(CWaypointLocations::NearestWaypoint(CBotGlobals::entityOrigin(pPlayer),200.0f,-1));

						if ( pWpt )
						{
							CFindPathTask *findpathtask = new CFindPathTask(pPlayer);

							pSched->addTask(findpathtask);

							findpathtask->completeIfSeeTaskEdict();
							findpathtask->failIfTaskEdictDead();
						}
						else
						{
							if ( randomFloat(0.0f,1.0f) > 0.25f )
							{
								addVoiceCommand(DOD_VC_NO);
							}

							delete pSched;
							return; // can't find the player
						}
					}

					CBotTask *pTask = new CDODDropAmmoTask(pPlayer);
					pSched->addTask(pTask);

					m_pSchedules->addFront(pSched);

					if ( randomFloat(0.0f,1.0f) > (inSquad()?0.25f:0.75f) )
					{
						addVoiceCommand(DOD_VC_YES);
					}
				}
			}
			else if ( randomFloat(0.0f,1.0f) > 0.75f )
			{
				addVoiceCommand(DOD_VC_NO);
			}
		}
		else if ( randomFloat(0.0f,1.0f) > 0.75f )
		{
			addVoiceCommand(DOD_VC_NO);
		}
		break;
	case DOD_VC_STICK_TOGETHER:
		{
			IPlayerInfo *p = playerinfomanager->GetPlayerInfo(pPlayer);

			if ( inSquad() && (m_pSquad->GetLeader() == pPlayer) )
			{
				removeCondition(CONDITION_PUSH);
				removeCondition(CONDITION_COVERT);
				removeCondition(CONDITION_DEFENSIVE);
				updateCondition(CONDITION_CHANGED);
				setSquadIdleTime(0.0f);
			}
			else if ( !inSquad() && isVisible(pPlayer) )
			{
				//int iClass = CClassInterface::getPlayerClassDOD(pPlayer);
				// probablity of joining someone is based on their score and class
				float fProb = 0.0f;
								
				// always join a squad if the leader is human
				if ( p->IsPlayer() && !p->IsFakeClient() )
					fProb = 1.0f;
				else if ( rcbot_bots_form_squads.GetBool() )
				{
					fProb = rcbot_bot_squads_percent.GetFloat()/100;

					/*if ( iClass == DOD_CLASS_SNIPER )
						fProb = 0.2f;
					else if ( iClass == DOD_CLASS_MACHINEGUNNER )
						fProb = 0.3f;*/
				}
				
				if ( randomFloat(0.0f,1.0f) < fProb )
				{
					m_pSquad = CBotSquads::AddSquadMember(pPlayer,m_pEdict);

					if ( m_pSquad )
					{
						updateCondition(CONDITION_PUSH);
						updateCondition(CONDITION_RUN);
						updateCondition(CONDITION_CHANGED);

						addVoiceCommand(DOD_VC_YES);
					}
				}
				else
				{
					if ( randomFloat(0.0f,1.0f) > 0.25f )
						addVoiceCommand(DOD_VC_NO);
				}
			}
			else if ( ( p->IsPlayer() && !p->IsFakeClient() ) || ((randomFloat(0.0f,1.0f) > 0.75f)&&rcbot_bots_form_squads.GetBool() ) ) // already in a squad
			{
				// decide to change squad leader here
				if ( inSquad() && (!hasSomeConditions(CONDITION_SEE_SQUAD_LEADER) || !hasSomeConditions(CONDITION_SQUAD_LEADER_INRANGE)) )
				{
					m_pSquad->removeMember(m_pEdict);
					clearSquad();

					m_pSquad = CBotSquads::AddSquadMember(pPlayer,m_pEdict);

					if ( m_pSquad )
					{
						updateCondition(CONDITION_PUSH);
						updateCondition(CONDITION_RUN);
						updateCondition(CONDITION_CHANGED);

						addVoiceCommand(DOD_VC_YES);
					}
				}
				else 
					addVoiceCommand(DOD_VC_NO);
			}
			else if (randomFloat(0.0f,1.0f) > 0.75f) 
					addVoiceCommand(DOD_VC_NO);
		}
		break;
	case DOD_VC_GRENADE2:
	case DOD_VC_BAZOOKA:
		// if I don't see anythign but I see the player calling this, hide!
		if ( !m_pEnemyGrenade && !m_pEnemyRocket && ((inSquad()&&(m_pSquad->GetLeader()==pPlayer))||isVisible(pPlayer)) && !m_pSchedules->isCurrentSchedule(SCHED_GOOD_HIDE_SPOT) )
		{
			float fDist = distanceFrom(pPlayer);
			updateCondition(CONDITION_COVERT);
			updateCondition(CONDITION_RUN);

			if ( fDist < BLAST_RADIUS )
				m_fCurrentDanger += 50.0f;
			else
				m_fCurrentDanger += 10.0f;

			// don't interrupt current shedule, just add to front
			m_pSchedules->removeSchedule(SCHED_GOOD_HIDE_SPOT);
			m_pSchedules->addFront(new CGotoHideSpotSched(this,pPlayer,false));
		}

		break;
	case DOD_VC_NEED_BACKUP:
		if ( (!inSquad() ||( m_pSquad->GetLeader()==m_pEdict)) && m_pNearestFlag && isVisible(pPlayer) && !rcbot_nocapturing.GetBool() )
		{
			Vector vPoint = CBotGlobals::entityOrigin(m_pNearestFlag);
			Vector vPlayer = CBotGlobals::entityOrigin(pPlayer);

			if ( (vPoint - vPlayer).Length() < 250 )
			{
				CBotSchedule *attack = new CBotSchedule();

				updateCondition(CONDITION_RUN);

				if ( CDODMod::isBombMap() )
				{
					CWaypoint *pWpt = CWaypoints::getWaypoint(CWaypointLocations::NearestBlastWaypoint(vPoint,vPlayer,1000.0f,-1,true,false,true,false,m_iTeam,true));

					attack->setID(SCHED_DEFENDPOINT);
					attack->addTask(new CFindPathTask(pWpt->getOrigin()));
					attack->addTask(new CBotDefendTask(pWpt->getOrigin(),randomFloat(6.0f,12.0f),0,true,vPoint,LOOK_SNIPE,pWpt->getFlags()));

					if ( !inSquad() && rcbot_bots_form_squads.GetBool() )
						attack->addTask(new CBotJoinSquad(pPlayer));

					// add defend task
					m_pSchedules->freeMemory();
					m_pSchedules->add(attack);
				}
				else
				{
					attack->setID(SCHED_ATTACKPOINT);
					attack->addTask(new CFindPathTask(vPoint));
					attack->addTask(new CBotDODAttackPoint(CDODMod::m_Flags.getFlagID(m_pNearestFlag),vPoint,150.0f));

					if ( !inSquad() && rcbot_bots_form_squads.GetBool() )
						attack->addTask(new CBotJoinSquad(pPlayer));

					// add defend task
					m_pSchedules->freeMemory();
					m_pSchedules->add(attack);

				}

				if ( randomFloat(0.0f,1.0f) > 0.75f )
					addVoiceCommand(DOD_VC_YES);
			}
		}
		break;
	default:
		break;
	}

	m_LastHearVoiceCommand = static_cast<eDODVoiceCMD>(cmd);

	// don't say the same command for a second or two
	if ( cmd < MAX_VOICE_CMDS )
		m_fLastVoiceCommand[cmd] = engine->Time() + randomFloat(1.0f,3.0f);
}

void CDODBot :: sayInPosition ( )
{
	signal("yes");
}

void CDODBot :: sayMoveOut ()
{
	signal("moveout");
	// forced squad leader to do something rather than just lark about
	removeCondition(CONDITION_DEFENSIVE);
	updateCondition(CONDITION_PUSH);
	updateCondition(CONDITION_RUN);
	updateCondition(CONDITION_CHANGED);
}

bool CDODBot :: withinTeammate ( ) const
{
	// check if the bot is right next to a team mate (sometimes bots can't deploy if theyr are next to one already)

	for ( int i = 1; i <= gpGlobals->maxClients; i ++ )
	{
		edict_t* pPlayer = INDEXENT(i);

		// skip me
		if ( pPlayer == m_pEdict )
			continue;

		if ( pPlayer && !pPlayer->IsFree() )
		{
			IPlayerInfo* playerinfo = playerinfomanager->GetPlayerInfo(pPlayer);

			if ( playerinfo )
			{
				if ( playerinfo->IsConnected() && !playerinfo->IsDead() )
				{
					// this player is in the game
					if ( playerinfo->GetTeamIndex() == m_iTeam )
					{
						if ( (playerinfo->GetAbsOrigin() - getOrigin()).Length() < 52.0f )
							return true;
					}
				}
			}
		}
	}

	return false;
}

void CDODBot::areaClear()
{
	addVoiceCommand(DOD_VC_AREA_CLEAR);
}

// Listen for players who are shooting --- USE EVENT (CDODFireWeaponEvent)
void CDODBot :: listenForPlayers ()
{
	if ( m_fListenTime > engine->Time() ) // already listening to something ?
	{
		setLookAtTask(LOOK_NOISE);
		return;
	}

	if ( !m_bStatsCanUse || (m_StatsCanUse.stats.m_iTeamMatesVisible>0) /*|| (m_fSeeTeamMateTime ... )*/ )
		return; // not very interested in listening to footsteps with other team-mates around.

	// check for footsteps
		//m_fNextListenTime = engine->Time() + randomFloat(0.5f,2.0f);
	edict_t *pListenNearest = nullptr;
	float fMaxFactor = 0;
	Vector vVelocity;

	m_bListenPositionValid = false;

	for ( int i = 1; i <= gpGlobals->maxClients; i ++ )
	{
		edict_t* pPlayer = INDEXENT(i);

		if ( pPlayer == m_pEdict )
			continue; // don't listen to self

		if ( pPlayer->IsFree() )
			continue;

		// get client network info
		const CClient* pClient = CClients::get(pPlayer);

		if ( !pClient->isUsed() )
			continue;

		IPlayerInfo* p = playerinfomanager->GetPlayerInfo(pPlayer);

		// 05/07/09 fix crash bug
		if ( !p || !p->IsConnected() || p->IsDead() || p->IsObserver() || !p->IsPlayer() )
			continue;

		if ( isVisible(pPlayer) )
			continue; // only listen to footsteps / don't care about people I can already see

		const float fDist = distanceFrom(pPlayer);

		if ( fDist > rcbot_listen_dist.GetFloat() )
			continue;

		float fFactor = (rcbot_listen_dist.GetFloat() - fDist);

		cmd = p->GetLastUserCommand();
		
		CClassInterface::getVelocity(pPlayer,&vVelocity);

		const float fVelocity = vVelocity.Length();

		if (  fVelocity > rcbot_footstep_speed.GetFloat() )
			fFactor += vVelocity.Length();
		else
			continue; // not going fast enough to hear -- can't hear, move on

		if ( fFactor > fMaxFactor )
		{
			fMaxFactor = fFactor;
			pListenNearest = pPlayer;
		}
	}

	if ( pListenNearest != nullptr)
	{
		m_fListenFactor = fMaxFactor;
		listenToPlayer(pListenNearest,false,false);
	}
}

// Successful actions must return true
// unsuccessful return false so that another may be attempted
bool CDODBot :: executeAction ( CBotUtility *util )
{
	int iBombType = 0;
	int id = -1;
	Vector vGoal;
	edict_t *pBombTarget = nullptr;

	switch ( util->getId() )
	{
	case  BOT_UTIL_INVESTIGATE_POINT:
		m_pSchedules->removeSchedule(SCHED_INVESTIGATE_NOISE);
		m_pSchedules->addFront(new CBotInvestigateNoiseSched(
			CBotGlobals::entityOrigin(reinterpret_cast<edict_t*>(util->getIntData())), util->getVectorData()));
		return true;
	case BOT_UTIL_COVER_POINT:
		m_pSchedules->removeSchedule(SCHED_CROUCH_AND_HIDE);
		m_pSchedules->addFront(new CCrouchHideSched(reinterpret_cast<edict_t*>(util->getIntData())));
		return true;
	case BOT_UTIL_SNIPE_POINT:
		// find sniper point facing the enemy
		{
			edict_t *pEnemy = reinterpret_cast<edict_t*>(util->getIntData());

			Vector vEnemyOrigin = CBotGlobals::entityOrigin(pEnemy);

			int iEnemyWpt = CWaypointLocations::NearestWaypoint(CBotGlobals::entityOrigin(pEnemy), 200.0f, -1, true,
			                                                    true, false, nullptr, false, 0, false);

			WaypointList waypoints;

			Vector vOrigin = getOrigin();

			// find all sniper points
			CWaypointLocations::GetAllInArea(vOrigin,&waypoints,iEnemyWpt);

			size_t size = waypoints.size();
			float fDist;
			float fNearest = 1024.0f;
			CWaypoint *pNearest = nullptr;

			for (size_t i = 0; i < size; i++)
			{
				CWaypoint *pWpt = CWaypoints::getWaypoint(waypoints[i]);

				if ( pWpt->hasFlag(CWaypointTypes::W_FL_SNIPER) )
				{
					// Check the yaw
					QAngle eyes;

					float iYaw = pWpt->getAimYaw();

					eyes.x = 0;
					eyes.y = iYaw;
					eyes.z = 0;

					Vector vecLOS;
					float flDot;
	
					Vector vForward;
		
					// in fov? Check angle to edict
					AngleVectors(eyes,&vForward);
	
					vecLOS = vEnemyOrigin - pWpt->getOrigin();
					vecLOS = vecLOS/vecLOS.Length();
	
					flDot = DotProduct (vecLOS , vForward );
	
					if ( flDot > 0 ) // 90 degrees 
					{
						fDist = pWpt->distanceFrom(vOrigin);

						if ( fDist < fNearest )
						{
							pNearest = pWpt;
							fNearest = fDist;
						}
					}
				}
			}

			if ( pNearest != nullptr)
			{
				m_pSchedules->freeMemory();

				CBotSchedule *snipesched = new CBotSchedule();

				snipesched->addTask(new CFindPathTask(CWaypoints::getWaypointIndex(pNearest)));
				snipesched->addTask(new CBotDODSnipe(getSniperRifle(),pNearest->getOrigin(),pNearest->getAimYaw(),true,vEnemyOrigin.z,pNearest->getFlags()));

				snipesched->setID(SCHED_SNIPE);

				m_pSchedules->addFront(snipesched);
				return true;
			}
		}
		break;
	case BOT_UTIL_MOVEUP_MG:
		m_pSchedules->add(new CDeployMachineGunSched(util->getWeaponChoice(),CWaypoints::getWaypoint(util->getIntData()),util->getVectorData()));
		return true;
	case BOT_UTIL_PICKUP_WEAPON:
		m_pSchedules->add(new CBotPickupSchedUse(m_pNearestWeapon.get()));
		return true;
	case BOT_UTIL_MESSAROUND:
		{
			// find a nearby friendly
			int i = 0;
			edict_t *pEdict;
			edict_t *pNearby = nullptr;
			float fMaxDistance = 500.0f;
			float fDistance;

			for ( i = 1; i <= CBotGlobals::maxClients(); i ++ )
			{
				pEdict = INDEXENT(i);

				if ( CBotGlobals::entityIsValid(pEdict) )
				{
					if ( CClassInterface::getTeam(pEdict) == getTeam() )
					{
						if ( (fDistance=distanceFrom(pEdict)) < fMaxDistance )
						{
							if ( isVisible(pEdict) )
							{
								// add a little bit of randomness
								if ( !pNearby || randomInt(0,1) )
								{
									pNearby = pEdict;
									fMaxDistance = fDistance;
								}
							}
						}
					}
				}
			}

			if ( pNearby )
			{
				// this will work in DOD too
				m_pSchedules->add(new CBotTF2MessAroundSched(pNearby,DOD_VC_INVALID));
				return true;
			}

			return false;
		}
	case BOT_UTIL_DEFEND_NEAREST_POINT:
		{
			Vector vGoal;
			int id = util->getIntData();
			bool defend_wpt = true;

			vGoal = util->getVectorData();

			if ( inSquad() && !isSquadLeader() )
			{
				if ( (m_pSquad->GetFormationVector(m_pEdict)-vGoal).Length() > 400.0f )
					return false;
			}

			CWaypoint *pWaypoint;

			pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_DEFEND,m_iTeam,id,true,this);

			if ( pWaypoint == nullptr)
			{			
				defend_wpt = false;

				if ( distanceFrom(vGoal) > 1024 ) // outside waypoint bucket of goal
					pWaypoint = CWaypoints::getPinchPointFromWaypoint(vGoal,vGoal);
				else
					pWaypoint = CWaypoints::getPinchPointFromWaypoint(getOrigin(),vGoal);
			}

			if ( pWaypoint )
			{
				if ( randomFloat(0.0f,1.0f) > 0.9f )
					addVoiceCommand(DOD_VC_ENEMY_BEHIND);
				
				CBotSchedule *defend = new CBotSchedule();
				CBotTask *findpath = new CFindPathTask(pWaypoint->getOrigin());
				CBotTask *deftask = new CBotDefendTask(pWaypoint->getOrigin(),randomFloat(7.5f,12.5f),0,true,vGoal,defend_wpt ? LOOK_SNIPE : LOOK_AROUND,pWaypoint->getFlags());

				removeCondition(CONDITION_PUSH); 
				findpath->setCompleteInterrupt(CONDITION_PUSH);
				deftask->setCompleteInterrupt(CONDITION_PUSH);

				defend->setID(SCHED_DEFENDPOINT);
				defend->addTask(findpath);
				defend->addTask(deftask);
				// add defend task
				m_pSchedules->add(defend);

				return true;

			}
		}
		break;
	case BOT_UTIL_ATTACK_NEAREST_POINT:
		{
			Vector vGoal;

			int iFlagID;
			int iWaypointGoal; 

			iFlagID = util->getIntData();
			iWaypointGoal = CDODMod::m_Flags.getWaypointAtFlag(iFlagID);

			vGoal = util->getVectorData();

			if ( iWaypointGoal == -1 )
				return false;

			if ( inSquad() && !isSquadLeader() )
			{
				if ( (m_pSquad->GetFormationVector(m_pEdict)-vGoal).Length() > 400.0f )
					return false;
			}

			CBotSchedule *attack = new CBotSchedule();
			
			attack->setID(SCHED_ATTACKPOINT);

			attack->addTask(new CFindPathTask(iWaypointGoal));
			attack->addTask(new CBotDODAttackPoint(iFlagID,vGoal,150.0f));
			// add defend task
			m_pSchedules->add(attack);
			removeCondition(CONDITION_PUSH);

			if ( CDODMod::m_Flags.getNumFlagsOwned(m_iTeam) == (CDODMod::m_Flags.getNumFlags()-1) )
			{
				addVoiceCommand(DOD_VC_GOGOGO);
			}

			return true;
		}
		//break;
	case BOT_UTIL_SNIPE: // find snipe or machine gun waypoint
		{
			int iFlagID = -1;
			int iWaypointType = 0;
			Vector vGoal;
			CWaypoint *pWaypoint = nullptr;

			if ( hasMG() )
				iWaypointType = CWaypointTypes::W_FL_MACHINEGUN;
			else
				iWaypointType = CWaypointTypes::W_FL_SNIPER;

			if ( inSquad() && !isSquadLeader() )
			{
				edict_t *pSquadLeader = m_pSquad->GetLeader();
				Vector vSquadLeaderOrigin = CBotGlobals::entityOrigin(pSquadLeader);

				iFlagID = CDODMod::m_Flags.findNearestObjective(vSquadLeaderOrigin);

				if ( iFlagID == -1 )
				{
					pWaypoint = CWaypoints::getWaypoint(CWaypointLocations::NearestWaypoint(
						m_pSquad->GetFormationVector(m_pEdict), 250.0f, -1, false, false, true, nullptr, false, m_iTeam,
						false, false, Vector(0, 0, 0), iWaypointType));					
				}
				else
					pWaypoint = CWaypoints::randomWaypointGoal(iWaypointType,m_iTeam,iFlagID,true,this);
			}
			else
			{
				if ( util->getIntData() ) // attack
				{
					if ( !CDODMod::isBombMap() || !CDODMod::isCommunalBombPoint() )
					{
						if ( CDODMod::m_Flags.getRandomEnemyControlledFlag(this,&vGoal,getTeam(),&iFlagID) )
						{
							pWaypoint = CWaypoints::randomWaypointGoal(iWaypointType,m_iTeam,iFlagID,true,this);
						}
					}
					else
					{
						// attack the bomb point -- less chance if owned many bomb points already
						if ( randomFloat(0.0f,1.0f) < 
							(static_cast<float>(CDODMod::m_Flags.getNumPlantableBombs(m_iTeam))/
							 CDODMod::m_Flags.getNumBombsOnMap(m_iTeam)) ) 
						{
								pWaypoint = CWaypoints::randomWaypointGoal(iWaypointType,m_iTeam,CDODMod::getBombPointArea(m_iTeam),true,this);
						}
						else
						{
							// attack a point
							if ( CDODMod::m_Flags.getRandomEnemyControlledFlag(this,&vGoal,getTeam(),&iFlagID) )
							{
								if ( hasMG() )
									pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_MACHINEGUN,m_iTeam,iFlagID,true,this);
								else
									pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_SNIPER,m_iTeam,iFlagID,true,this);
							}
						}
					}

					if ( pWaypoint ) // attack position -- pushing
						removeCondition(CONDITION_PUSH);
				}
				else // defend
				{
					if ( CDODMod::m_Flags.getRandomTeamControlledFlag(this,&vGoal,getTeam(),&iFlagID) )
					{
						pWaypoint = CWaypoints::randomWaypointGoal(iWaypointType,m_iTeam,iFlagID,true,this);
					}
				}

				if ( pWaypoint == nullptr)
				{
					pWaypoint = CWaypoints::randomWaypointGoal(iWaypointType,m_iTeam);
				}
			}

			if ( pWaypoint )
			{
				CBotSchedule *snipe = new CBotSchedule();
				CBotTask *findpath = new CFindPathTask(CWaypoints::getWaypointIndex(pWaypoint));
				CBotTask *snipetask;
				
				// find Z for goal if no flag id
				if ( (iFlagID == -1) && (pWaypoint->getArea() > 0) && CDODMod::isBombMap() && CDODMod::isCommunalBombPoint() )
				{
					CWaypoint *pBombs = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_BOMBS_HERE,m_iTeam,pWaypoint->getArea(),true);

					if ( pBombs )
					{
						iFlagID = pWaypoint->getArea();
						vGoal = pBombs->getOrigin();
					}
				}

				snipetask = new CBotDODSnipe(util->getWeaponChoice(),pWaypoint->getOrigin(),pWaypoint->getAimYaw(),iFlagID!=-1,vGoal.z+48,pWaypoint->getFlags());

				removeCondition(CONDITION_PUSH);
				findpath->setCompleteInterrupt(CONDITION_PUSH);
				snipetask->setCompleteInterrupt(CONDITION_PUSH);

				snipe->setID(SCHED_DEFENDPOINT);
				snipe->addTask(findpath);
				snipe->addTask(snipetask);
				
				m_pSchedules->add(snipe);

				return true;
			}
		}
		break;
	case BOT_UTIL_DEFEND_NEAREST_BOMB:
		vGoal = util->getVectorData();
	case BOT_UTIL_DEFEND_BOMB: // fall through -- no break
		if ( util->getId() == BOT_UTIL_DEFEND_BOMB )
			CDODMod::m_Flags.getRandomBombToDefend(this,&vGoal,m_iTeam,&pBombTarget,&id);
	case BOT_UTIL_DEFEND_POINT:
		{
			int id = -1;

			if ( util->getId() == BOT_UTIL_DEFEND_POINT )
			{
				if ( !CDODMod::m_Flags.getRandomTeamControlledFlag(this,&vGoal,getTeam(),&id) )
					return false;
			}

			CWaypoint *pWaypoint;

			pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_DEFEND,m_iTeam,id,true,this);

			if ( pWaypoint == nullptr)
			{
				bool defend_wpt = true; //Unused? [APG]RoboCop[CL]
				defend_wpt = false;

				if ( distanceFrom(vGoal) > 1024 ) // outside waypoint bucket of goal
					pWaypoint = CWaypoints::getPinchPointFromWaypoint(vGoal,vGoal);
				else
					pWaypoint = CWaypoints::getPinchPointFromWaypoint(getOrigin(),vGoal);
			}

			if ( pWaypoint )
			{
				CBotSchedule *defend = new CBotSchedule();
				CBotTask *findpath = new CFindPathTask(pWaypoint->getOrigin());
				// fix -- make bots look at yaw
				CBotTask *deftask = new CBotDefendTask(pWaypoint->getOrigin(),randomFloat(7.5f,12.5f),0,false,Vector(0,0,0),LOOK_SNIPE,pWaypoint->getFlags());

				removeCondition(CONDITION_PUSH); 
				findpath->setCompleteInterrupt(CONDITION_PUSH);
				deftask->setCompleteInterrupt(CONDITION_PUSH);

				defend->setID(SCHED_DEFENDPOINT);
				defend->addTask(findpath);
				defend->addTask(deftask);
				// add defend task
				m_pSchedules->add(defend);

				return true;
			}
		}
		break;
	// no breaking -- defusing/planting bomb stuff
	case BOT_UTIL_DEFUSE_NEAREST_BOMB:
		iBombType = DOD_BOMB_DEFUSE;

		if ( CDODMod::m_Flags.isTeamMateDefusing(m_pEdict,m_iTeam,util->getIntData()) )
			return false; // teammate doing the job already
		break;

	case BOT_UTIL_PLANT_NEAREST_BOMB:
		id = util->getIntData();
		vGoal = util->getVectorData();
		pBombTarget = m_pNearestBomb;

		if ( util->getId() == BOT_UTIL_PLANT_NEAREST_BOMB )
			iBombType = DOD_BOMB_PLANT;

		if ( CDODMod::m_Flags.isTeamMatePlanting(m_pEdict,m_iTeam,id) )
			return false; // teammate doing the job already
		break;

	case BOT_UTIL_DEFUSE_BOMB:
		if ( util->getId() == BOT_UTIL_DEFUSE_BOMB )
		{
			if ( !CDODMod::m_Flags.getRandomBombToDefuse(&vGoal,m_iTeam,&pBombTarget,&id) )
				return false;

			if ( CDODMod::m_Flags.isTeamMateDefusing(m_pEdict,m_iTeam,id) )
				return false; // teammate doing the job already

			iBombType = DOD_BOMB_DEFUSE;
		}
		break;
		
	case BOT_UTIL_PLANT_BOMB:
		{
			if ( util->getId() == BOT_UTIL_PLANT_BOMB )
			{
				if ( !CDODMod::m_Flags.getRandomBombToPlant(this,&vGoal,m_iTeam,&pBombTarget,&id) )
					return false;

				if ( CDODMod::m_Flags.isTeamMatePlanting(m_pEdict,m_iTeam,id) )
					return false; // teammate doing the job already here

				iBombType = DOD_BOMB_PLANT;
			}

			int iWptGoal = CDODMod::m_Flags.getWaypointAtFlag(id);
	
			if ( iWptGoal == -1 )
				return false;

			CWaypoint *pWaypoint;

			if ( distanceFrom(vGoal) > 1024 ) // outside waypoint bucket of goal
				pWaypoint = CWaypoints::getPinchPointFromWaypoint(vGoal,vGoal);
			else
				pWaypoint = CWaypoints::getPinchPointFromWaypoint(getOrigin(),vGoal);

			CBotSchedule *attack = new CBotSchedule();

			attack->setID(SCHED_BOMB);

			if ( pWaypoint && (iBombType == DOD_BOMB_PLANT) && (randomFloat(0.0f,200.0f) < m_pNavigator->getBelief(CWaypoints::getWaypointIndex(pWaypoint))) )
			{
				attack->addTask(new CFindPathTask(pWaypoint->getOrigin()));
				attack->addTask(new CBotInvestigateTask(pWaypoint->getOrigin(),250,Vector(0,0,0),false,randomFloat(3.0f,5.0f),CONDITION_SEE_CUR_ENEMY));
			}
			attack->addTask(new CFindPathTask(iWptGoal));
			attack->addTask(new CBotDODBomb(iBombType,id,pBombTarget,vGoal,-1));
			// add defend task
			m_pSchedules->add(attack);

			if ( iBombType == DOD_BOMB_DEFUSE ) 
				updateCondition(CONDITION_RUN);

			removeCondition(CONDITION_PUSH);
			
			return true;
		
		}
		//break;
	case BOT_UTIL_PICKUP_BOMB:
		{
			CWaypoint *pWaypoint;
			Vector vOrigin = getOrigin();

			pWaypoint = CWaypoints::getWaypoint(CWaypoints::nearestWaypointGoal(CWaypointTypes::W_FL_BOMBS_HERE,vOrigin,8192.0f,m_iTeam));

			if ( pWaypoint )
			{
				if ( CDODMod::isCommunalBombPoint() )
				{
					CWaypoint *pWaypointPinch = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_DEFEND|CWaypointTypes::W_FL_SNIPER|CWaypointTypes::W_FL_MACHINEGUN,m_iTeam,pWaypoint->getArea(),true,this);

					if ( !pWaypointPinch )
					{
						if ( distanceFrom(pWaypoint->getOrigin()) > 1024.0f )
							pWaypointPinch = CWaypoints::getPinchPointFromWaypoint(pWaypoint->getOrigin(),pWaypoint->getOrigin());
						else
							pWaypointPinch = CWaypoints::getPinchPointFromWaypoint(getOrigin(),pWaypoint->getOrigin());
					}

					if ( pWaypointPinch && (distanceFrom(pWaypointPinch->getOrigin()) < distanceFrom(pWaypoint->getOrigin())) )
					{
						CBotSchedule *defend = new CBotSchedule();
						CBotTask *findpath1 = new CFindPathTask(pWaypointPinch->getOrigin());
						CBotTask *findpath2 = new CFindPathTask(pWaypoint->getOrigin());
						CBotTask *deftask = new CBotDefendTask(pWaypointPinch->getOrigin(),randomFloat(3.5f,6.5f),0,true,pWaypoint->getOrigin(),LOOK_SNIPE);

						removeCondition(CONDITION_PUSH); 
						deftask->setCompleteInterrupt(CONDITION_PUSH);

						defend->setID(SCHED_DEFENDPOINT);
						defend->addTask(findpath1); // find a spot to look out for enemies
						defend->addTask(deftask);   // look out for enemies
						defend->addTask(findpath2); // pickup bomb
						// add defend task
						m_pSchedules->add(defend);
					}
					else
						m_pSchedules->add(new CBotGotoOriginSched(pWaypoint->getOrigin()));
				}
				else
					m_pSchedules->add(new CBotGotoOriginSched(pWaypoint->getOrigin()));

				return true;
			}
		}
		break;
	case BOT_UTIL_FIND_LAST_ENEMY:
		{
			Vector vVelocity = Vector(0,0,0);
			CClient *pClient = CClients::get(m_pLastEnemy);
			CBotSchedule *pSchedule = new CBotSchedule();
			
			CFindPathTask *pFindPath = new CFindPathTask(m_vLastSeeEnemy,LOOK_LAST_ENEMY);	

			removeCondition(CONDITION_SEE_CUR_ENEMY);
			pFindPath->setCompleteInterrupt(CONDITION_SEE_CUR_ENEMY);
			
			if ( !CClassInterface::getVelocity(m_pLastEnemy,&vVelocity) )
			{
				if ( pClient )
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
			CWaypoint *pWaypoint = CWaypoints::getWaypoint(CWaypointLocations::GetCoverWaypoint(getOrigin(),m_vLastSeeEnemy, nullptr));

			if ( pWaypoint )
			{
				CBotWeapon *pWeapon = util->getWeaponChoice();
				CBotSchedule *pSched = new CBotSchedule();
				CFindPathTask *pathtask = new CFindPathTask(pWaypoint->getOrigin());

				pSched->addTask(new CThrowGrenadeTask(pWeapon,getAmmo(pWeapon->getWeaponInfo()->getAmmoIndex1()),m_vLastSeeEnemyBlastWaypoint)); // first - throw
				pSched->addTask(pathtask); // 2nd -- hide

				pathtask->setNoInterruptions();

				m_pSchedules->add(pSched);

				removeCondition(CONDITION_COVERT);
				removeCondition(CONDITION_GREN);

				return true;
			}

			return true;
		}
		//break;
	case BOT_UTIL_ATTACK_POINT:
		{
			Vector vGoal;
			int iFlagID;

			if ( CDODMod::m_Flags.getRandomEnemyControlledFlag(this,&vGoal,getTeam(),&iFlagID) )
			{
				int iGoalWaypoint = CDODMod::m_Flags.getWaypointAtFlag(iFlagID);

				// no waypoint...
				if ( iGoalWaypoint == -1 )
					return false;
				
				CBotSchedule *attack = new CBotSchedule();

				CWaypoint *pRoute = nullptr;

				if ( (m_fUseRouteTime < engine->Time()) )
				{
				// find random route
					CWaypoint *pGoalWpt = CWaypoints::getWaypoint(iGoalWaypoint);
					pRoute = CWaypoints::randomRouteWaypoint(this,getOrigin(),pGoalWpt->getOrigin(),getTeam(),0);

					if ( pRoute )
					{
						attack->addTask(new CFindPathTask(CWaypoints::getWaypointIndex(pRoute)));
						m_fUseRouteTime = engine->Time() + 30.0f;
					}
				}

				attack->setID(SCHED_ATTACKPOINT);

				if ( (pRoute == nullptr) && (randomFloat(0.0f,MAX_BELIEF) < m_pNavigator->getBelief(iGoalWaypoint)) )
				{
					CWaypoint *pWaypoint;
					if ( distanceFrom(vGoal) > 1024 ) // outside waypoint bucket of goal
						pWaypoint = CWaypoints::getPinchPointFromWaypoint(vGoal,vGoal);
					else
						pWaypoint = CWaypoints::getPinchPointFromWaypoint(getOrigin(),vGoal);

					if ( pWaypoint )
					{
						attack->addTask(new CFindPathTask(CWaypoints::getWaypointIndex(pWaypoint)));
						attack->addTask(new CBotInvestigateTask(pWaypoint->getOrigin(),250,Vector(0,0,0),false,randomFloat(3.0f,5.0f),CONDITION_SEE_CUR_ENEMY));
					}
				}

				attack->addTask(new CFindPathTask(iGoalWaypoint));
				attack->addTask(new CBotDODAttackPoint(iFlagID,vGoal,150.0f));
				// add defend task
				m_pSchedules->add(attack);
				
				// last flag
				//if ( CDODMod::m_Flags.getNumFlagsOwned(m_iTeam) == (CDODMod::m_Flags.getNumFlags()-1) )
				//	addVoiceCommand(DOD_VC_GOGOGO);

				removeCondition(CONDITION_PUSH);

				return true;
		
			}
		}
		break;
	case BOT_UTIL_FIND_SQUAD_LEADER:
		{
			Vector pos = m_pSquad->GetFormationVector(m_pEdict);
			CBotTask *findTask = new CFindPathTask(pos);
			removeCondition(CONDITION_SEE_SQUAD_LEADER);
			removeCondition(CONDITION_SQUAD_LEADER_INRANGE);
			findTask->setCompleteInterrupt(CONDITION_SEE_SQUAD_LEADER|CONDITION_SQUAD_LEADER_INRANGE);

			m_pSchedules->add(new CBotSchedule(findTask));

			return true;
		}
		//break;
	case BOT_UTIL_FOLLOW_SQUAD_LEADER:
		{
			Vector pos = m_pSquad->GetFormationVector(m_pEdict); // `pos` Unused? [APG]RoboCop[CL]

			m_pSchedules->add(new CBotSchedule(new CBotFollowSquadLeader(m_pSquad)));

			return true;
		}
		//break;
	case BOT_UTIL_ROAM:
		{
		// roam			
			CWaypoint *pWaypoint = CWaypoints::getWaypoint(CWaypoints::randomFlaggedWaypoint(getTeam()));

			if ( pWaypoint )
			{
				if ( inSquad() && !isSquadLeader() )
				{
					if ( pWaypoint->distanceFrom(m_pSquad->GetFormationVector(m_pEdict)) > CWaypointLocations::REACHABLE_RANGE )
						return false;
				}

				m_pSchedules->add(new CBotGotoOriginSched(pWaypoint->getOrigin()));
				return true;
			}
		}
		break;
	default:
		break;
	}

	return false;
}

void CDODBot :: reachedCoverSpot (int flags)
{
	// reached cover
	// dont need to run there any more
	removeCondition(CONDITION_RUN);

	if ( !m_pEnemy.get() && !hasSomeConditions(CONDITION_SEE_CUR_ENEMY) )
	{
		// Allow bots to crouch when reloading to take cover
		if ( flags & (CWaypointTypes::W_FL_CROUCH | CWaypointTypes::W_FL_MACHINEGUN) )
			m_pButtons->duck(randomFloat(2.0f,3.5f),engine->Time());

		m_pButtons->tap(IN_RELOAD);
	}

	//m_fFixWeaponTime = engine->Time() + 1.0f;
	//}
	CBotWeapon *pWeapon = getMG();

	if ( pWeapon != nullptr)
	{
		bool bDontCrouchAndHide = false;

		if ( pWeapon && pWeapon->isDeployable() && !pWeapon->outOfAmmo(this) )
		{
			if ( flags & CWaypointTypes::W_FL_MACHINEGUN )
			{
				const CWaypoint *pWpt = CWaypoints::getWaypoint(m_pNavigator->getCurrentGoalID());

				if ( pWpt && (pWpt->getFlags() == flags) )
				{
					m_pSchedules->addFront(new CBotSchedule(new CBotDODSnipe(pWeapon,getOrigin(),pWpt->getAimYaw(),false,false,flags)));
					removeCondition(CONDITION_PRONE);
					bDontCrouchAndHide = true;
				}
			}
			else
			{
				updateCondition(CONDITION_PRONE);
			}
			
			m_fCurrentDanger = MAX_BELIEF;
		}

		if ( !bDontCrouchAndHide && (m_pLastCoverFrom.get() != nullptr) )
		{
			if ( CBotGlobals::entityIsAlive(m_pLastCoverFrom) )
			{
				if ( flags & (CWaypointTypes::W_FL_CROUCH|CWaypointTypes::W_FL_MACHINEGUN|CWaypointTypes::W_FL_COVER_RELOAD) )
				{
					m_pSchedules->removeSchedule(SCHED_CROUCH_AND_HIDE);
					m_pSchedules->addFront(new CCrouchHideSched(m_pLastCoverFrom));
				}
			}
		}
	}
}

bool CDODBot:: checkStuck ()
{
	if ( CBot::checkStuck() )
	{
		const CBotWeapon *pWeapon = getCurrentWeapon();

		if ( pWeapon && (pWeapon->getWeaponEntity()!= nullptr) )
		{
			if ( pWeapon->isZoomable() && CClassInterface::isSniperWeaponZoomed(pWeapon->getWeaponEntity()) )
				secondaryAttack();
			else if ( pWeapon->isDeployable() )
			{
				if ( pWeapon->isExplosive() && CClassInterface::isRocketDeployed(pWeapon->getWeaponEntity()) )
					secondaryAttack();
				else if ( !pWeapon->isExplosive() && CClassInterface::isMachineGunDeployed(pWeapon->getWeaponEntity()) )
					secondaryAttack();
			}
		}

		const edict_t *pGroundEnt = CClassInterface::getGroundEntity(m_pEdict);

		if ( pGroundEnt ) // stuck on furniture? 
		{
			if ( std::strncmp(pGroundEnt->GetClassName(),"prop_physics",12) != 0 )
			{
				// Duck
				if ( randomFloat(0.0f,1.0f) < 0.9f )
					duck();
			}
		}
	}

	return false;
}

bool CDODBot :: handleAttack ( CBotWeapon *pWeapon, edict_t *pEnemy )
{
	static bool bAttack;
	static float fDelay; // delay to reduce recoil

	bAttack = true;
	fDelay = 0;

	if ( pWeapon )
	{
		static float fDist;
		const Vector vEnemyOrigin = CBotGlobals::entityOrigin(pEnemy);

		fDist = distanceFrom(vEnemyOrigin);

		if ( pWeapon->isExplosive() && (DotProductFromOrigin(m_vAimVector) < 0.98f) ) 
			return true; // keep enemy / don't shoot : until angle between enemy is less than 45 degrees

		clearFailedWeaponSelect();

		if ( pWeapon->isMelee() || (pWeapon->isMeleeSecondary()&&(pWeapon->outOfAmmo(this)||rcbot_melee_only.GetBool())) )
		{
			setMoveTo(CBotGlobals::entityOrigin(pEnemy));
			//setLookAt(m_vAimVector);
			setLookAtTask(LOOK_ENEMY);
			// dontAvoid my enemy
			m_fAvoidTime = engine->Time() + 1.0f;
			fDelay = 0;

	// enemy below me!
			if ( pWeapon->isMelee() && (vEnemyOrigin.z < (getOrigin().z - 8)) && (vEnemyOrigin.z > (getOrigin().z-128))  )
				duck();
		}
		else
		{
			edict_t *pWeaponEdict = pWeapon->getWeaponEntity();

			if ( m_iClip1 == 0 )
			{
				if ( !m_pSchedules->isCurrentSchedule(SCHED_GOOD_HIDE_SPOT) )
				{
					if ( pWeapon->isZoomable() && !CClassInterface::isSniperWeaponZoomed(pWeaponEdict) )
						secondaryAttack();

					m_pSchedules->freeMemory();
					m_pSchedules->add(new CGotoHideSpotSched(this,pEnemy));
				}

				if ( pWeapon->outOfAmmo(this) )
					updateCondition(CONDITION_NEED_AMMO);
				else
					m_pButtons->tap(IN_RELOAD);

				return false;
			}
			fDelay = randomFloat(0.05f,0.2f);

			removeCondition(CONDITION_NEED_AMMO);

			if ( !hasSomeConditions(CONDITION_RUN) )
				stopMoving();

			if ( pWeapon->needsDeployedOrZoomed() ) // && pWeapon->getID() ==...
			{
				//stopMoving();

				if ( pWeapon->isZoomable() ) 
				{
					if ( !CClassInterface::isSniperWeaponZoomed(pWeaponEdict) )
						bAttack = false;
					else
						fDelay = randomFloat(0.5f,1.0f);
				}
				else if ( pWeapon->isDeployable() ) 
				{
					if ( pWeapon->isExplosive() )
						bAttack = CClassInterface::isRocketDeployed(pWeaponEdict);
					else
					{
						bAttack = CClassInterface::isMachineGunDeployed(pWeaponEdict);
					}

					if ( !bAttack )
						fDelay = randomFloat(0.7f,1.2f);
					//else
					//	fDelay = 0;
				}

				if ( !bAttack && (m_fZoomOrDeployTime < engine->Time()) )
				{
					secondaryAttack(); // deploy / zoom
					m_fZoomOrDeployTime = engine->Time() + randomFloat(0.5f,1.0f);
				}
			} 
			else if ( pWeapon->isDeployable() )
			{
				if ( pWeapon->hasHighRecoil() && !CClassInterface::isMachineGunDeployed(pWeaponEdict) )
				{
					if ( m_fZoomOrDeployTime < engine->Time() )
					{
						secondaryAttack(); // deploy / zoom
						m_fZoomOrDeployTime = engine->Time() + randomFloat(0.5f,1.0f);
					}

					// not deployed for a while -- go prone to deploy
					if ( !hasSomeConditions(CONDITION_RUN) && (m_fDeployMachineGunTime + 1.0f) < engine->Time() )
					{
						// go prone
						prone();

						if ( fDist > 400.0f )
							return true; // keep enemy but don't shoot yet
					}

					fDelay = randomFloat(0.7f,1.2f);
				}
			}
			else if ( ((pWeapon->getID()==DOD_WEAPON_GARAND)||(pWeapon->getID()==DOD_WEAPON_K98)) && ( distanceFrom(pEnemy) > 1000 ))
			{
				if ( !CClassInterface::isK98Zoomed(pWeaponEdict) && !CClassInterface::isGarandZoomed(pWeaponEdict) )
				{
					if ( m_fZoomOrDeployTime < engine->Time() )
					{
						secondaryAttack(); // deploy / zoom
						m_fZoomOrDeployTime = engine->Time() + randomFloat(0.1f,0.2f);
						fDelay = randomFloat(0.2f,0.4f);
					}
				}
			}
		}

		if ( bAttack && (m_fShootTime < engine->Time()) )
		{
			if ( pWeapon->isMeleeSecondary()&&(pWeapon->outOfAmmo(this)||rcbot_melee_only.GetBool()) )
				secondaryAttack();
			else
				primaryAttack(); // shoot

			m_fShootTime = engine->Time() + fDelay;
		}
	}
	else
		primaryAttack();

	return true;
}

CBotWeapon *CDODBot::getMG () const
{
	return m_pWeapons->hasWeapon(DOD_WEAPON_20CAL) ? m_pWeapons->getWeapon(CWeapons::getWeapon(DOD_WEAPON_20CAL)) : m_pWeapons->getWeapon(CWeapons::getWeapon(DOD_WEAPON_MG42));
}

bool CDODBot :: hasMG () const
{
	return m_pWeapons->hasWeapon(DOD_WEAPON_20CAL) || m_pWeapons->hasWeapon(DOD_WEAPON_MG42);
}

CBotWeapon *CDODBot::getSniperRifle () const
{
	return m_pWeapons->hasWeapon(DOD_WEAPON_K98_SCOPED) ? m_pWeapons->getWeapon(CWeapons::getWeapon(DOD_WEAPON_K98_SCOPED)) : m_pWeapons->getWeapon(CWeapons::getWeapon(DOD_WEAPON_SPRING));
}

bool CDODBot :: hasSniperRifle () const
{
	return m_pWeapons->hasWeapon(DOD_WEAPON_K98_SCOPED) || m_pWeapons->hasWeapon(DOD_WEAPON_SPRING);
}

#define BOT_DEFEND 0
#define BOT_ATTACK 1

void CDODBot :: getTasks (unsigned int iIgnore)
{
	static CBotUtilities utils;
	static CBotUtility *next;
	static CBotWeapon *grenade; //Unused? [APG]RoboCop[CL]
	static CBotWeapon *pWeapon;
	static float fAttackUtil;
	static float fDefendUtil;
	static int numPlayersOnTeam; //Unused? [APG]RoboCop[CL]
	static int numClassOnTeam; //Unused? [APG]RoboCop[CL]
	static bool bCheckCurrent;
	static int iFlagID;
	static int iFlagsOwned;
	static int iEnemyFlagsOwned; //Unused? [APG]RoboCop[CL]
	static int iNumFlags;
	static bool bCanMessAround;

	// if condition has changed or no tasks
	if ( !hasSomeConditions(CONDITION_CHANGED) && !m_pSchedules->isEmpty() )
		return;

	//squads
	ADD_UTILITY(BOT_UTIL_FOLLOW_SQUAD_LEADER,!hasSomeConditions(CONDITION_DEFENSIVE) && hasSomeConditions(CONDITION_SQUAD_LEADER_INRANGE) && inSquad() && (m_pSquad->GetLeader()!=m_pEdict) && hasSomeConditions(CONDITION_SEE_SQUAD_LEADER),hasSomeConditions(CONDITION_SQUAD_IDLE)?0.1f:2.0f)
	ADD_UTILITY(BOT_UTIL_FIND_SQUAD_LEADER,!hasSomeConditions(CONDITION_DEFENSIVE) && inSquad() && (m_pSquad->GetLeader()!=m_pEdict) && (!hasSomeConditions(CONDITION_SEE_SQUAD_LEADER) || !hasSomeConditions(CONDITION_SQUAD_LEADER_INRANGE)),hasSomeConditions(CONDITION_SQUAD_IDLE)?0.1f:2.0f)

	bCheckCurrent = true;
	m_iTeam = getTeam(); // update team
	bCanMessAround = bot_messaround.GetBool();

	fAttackUtil = 0.5f;
	fDefendUtil = 0.4f;

	numClassOnTeam = CDODMod::numClassOnTeam(m_iTeam,m_iClass);
	numPlayersOnTeam = CBotGlobals::numPlayersOnTeam(m_iTeam,false);

	pWeapon = nullptr;

	removeCondition(CONDITION_CHANGED);

	// always able to roam around -- low utility
	ADD_UTILITY(BOT_UTIL_ROAM,true,0.01f)

	// I had an enemy a minute ago
	ADD_UTILITY(BOT_UTIL_FIND_LAST_ENEMY,wantToFollowEnemy() && !m_bLookedForEnemyLast && (m_pLastEnemy.get()!=NULL) && CBotGlobals::entityIsValid(m_pLastEnemy) && CBotGlobals::entityIsAlive(m_pLastEnemy),getHealthPercent()*0.89f)

	// flag capture map
	if ( CDODMod::isFlagMap() && (CDODMod::m_Flags.getNumFlags() > 0) && !rcbot_nocapturing.GetBool() )
	{
		bool bAttackNearestFlag = false;

		bCanMessAround = false;

		if ( inSquad() )
		{
			//caxanga334: SDK 2013 doesn't like to create a Vector from an int
			//TODO: Proper fix
			#if SOURCE_ENGINE == SE_SDK2013 || SOURCE_ENGINE == SE_BMS
			iFlagID = CDODMod::m_Flags.findNearestObjective(Vector(m_pSquad->GetFormationPosition(m_pEdict)));
			#else
			iFlagID = CDODMod::m_Flags.findNearestObjective(m_pSquad->GetFormationPosition(m_pEdict));
			#endif
			m_pNearestFlag = CDODMod::m_Flags.getFlagByID(iFlagID);

			if ( hasSomeConditions(CONDITION_DEFENSIVE) )
			{
				CWaypoint *pWaypoint = CWaypoints::getWaypoint(CDODMod::m_Flags.getWaypointAtFlag(iFlagID));

				if ( pWaypoint )
				{
					// just following orders
					ADD_UTILITY_DATA_VECTOR(BOT_UTIL_DEFEND_NEAREST_POINT,true,1.5f,iFlagID,pWaypoint->getOrigin())
				}
			}
		}

		if ( m_pNearestFlag )
		{
			iFlagID = CDODMod::m_Flags.getFlagID(m_pNearestFlag);

			bAttackNearestFlag = !CDODMod::m_Flags.ownsFlag(iFlagID,m_iTeam) && ((CDODMod::m_Flags.numCappersRequired(iFlagID,m_iTeam)-CDODMod::m_Flags.numFriendliesAtCap(iFlagID,m_iTeam))>0);
		}

		if ( (m_pNearestFlag==NULL) || !bAttackNearestFlag )
		{
			if ( !hasSomeConditions(CONDITION_DEFENSIVE) && CDODMod::shouldAttack(m_iTeam) )
			{
				fAttackUtil = 0.6f;
				fDefendUtil = 0.3f;
			}
			else
			{
				fAttackUtil = 0.3f;
				fDefendUtil = 0.5f;
			}

			if ( hasSomeConditions(CONDITION_PUSH) )
				fAttackUtil *= 2;
			if ( hasSomeConditions(CONDITION_DEFENSIVE) )
				fDefendUtil *= 2;

			// if we see a flag and we own it, don't worry about it
			ADD_UTILITY(BOT_UTIL_ATTACK_POINT,true,fAttackUtil)
			ADD_UTILITY(BOT_UTIL_DEFEND_POINT,true,fDefendUtil)
		}

		if ( m_pNearestFlag && (!inSquad() || (isSquadLeader() || (hasSomeConditions(CONDITION_SQUAD_IDLE) && (distanceFrom(m_pNearestFlag)<CWaypointLocations::REACHABLE_RANGE))) ) )
		{
			// attack the flag if I've reached the last one
			ADD_UTILITY_DATA_VECTOR(BOT_UTIL_ATTACK_NEAREST_POINT,
				bAttackNearestFlag,(iFlagsOwned == (iNumFlags-1)) ? 0.9f : 0.75f,iFlagID,CBotGlobals::entityOrigin(m_pNearestFlag))

			ADD_UTILITY_DATA_VECTOR(BOT_UTIL_DEFEND_NEAREST_POINT,
				CDODMod::m_Flags.ownsFlag(iFlagID,m_iTeam) && ((CDODMod::m_Flags.numEnemiesAtCap(iFlagID,m_iTeam)>0)||hasEnemy()),
				(iFlagsOwned == 1) ? 0.9f : 0.75f,iFlagID,CBotGlobals::entityOrigin(m_pNearestFlag))
		}

		if ( CDODMod::mapHasBombs() )
		{
			ADD_UTILITY(BOT_UTIL_PICKUP_BOMB,!m_bHasBomb,hasSomeConditions(CONDITION_NEED_BOMB) ? 1.0f : 0.75f)
		}
	}
	// bomb map
	
	if ( CDODMod::isBombMap() && (CDODMod::m_Flags.getNumFlags() > 0) && !rcbot_nocapturing.GetBool() )
	{
		static int iEnemyTeam;
		static float fPlantUtil;
		static float fDefuseBombUtil;
		static float fDefendBombUtil;
		static int iNumEnemyBombsStillToPlant;
		static int iNumBombsToDefend;
		static int iNumBombsToDefuse;
		static int iNumEnemyBombsOnMap;
		static int iNumBombsOnMap;
		static int iNumBombsToPlant;
		static float fDefRate;
		bCanMessAround = false;
		// same thing as above except with bombs
		iFlagID = -1;
		iEnemyTeam = m_iTeam==TEAM_ALLIES ? TEAM_AXIS : TEAM_ALLIES;
		iFlagsOwned = CDODMod::m_Flags.getNumFlagsOwned(m_iTeam);
		iNumFlags = CDODMod::m_Flags.getNumFlags();

		iNumEnemyBombsOnMap = CDODMod::m_Flags.getNumBombsOnMap(iEnemyTeam);
		iNumBombsOnMap = CDODMod::m_Flags.getNumBombsOnMap(m_iTeam);
		iNumBombsToPlant = CDODMod::m_Flags.getNumBombsToPlant(m_iTeam);
		iNumBombsToDefuse = CDODMod::m_Flags.getNumBombsToDefuse(m_iTeam);
		iNumBombsToDefend = CDODMod::m_Flags.getNumBombsToDefend(m_iTeam);
		iNumEnemyBombsStillToPlant = CDODMod::m_Flags.getNumBombsToPlant(iEnemyTeam);

		// different defend util here
		fDefendUtil = 0.4f;

		if ( iNumEnemyBombsOnMap > 0 )
			fDefendUtil = 0.8f - (static_cast<float>(iNumEnemyBombsStillToPlant)/iNumEnemyBombsOnMap)*0.4f;

		if ( CDODMod::isFlagMap() && (iNumBombsToPlant>0) )
			fPlantUtil = 0.3f + (((static_cast<float>(iFlagsOwned)/iNumBombsToPlant)*0.6f)/iNumFlags);
		else
			fPlantUtil = 0.4f + ((static_cast<float>(iNumBombsToPlant)/iNumBombsOnMap)*0.4f);
		
		fDefuseBombUtil = fDefendUtil * 2;
		fDefendBombUtil = 0.8f - ((static_cast<float>(iNumBombsToDefend)/iNumBombsOnMap)*0.8f);

		
		fPlantUtil += randomFloat(-0.25f,0.25f); // add some fuzz
		fAttackUtil = fPlantUtil;
		fDefRate = bot_defrate.GetFloat();
		fDefendUtil += randomFloat(-fDefRate,fDefRate);

		// bot is ... go go go!
		if ( hasSomeConditions(CONDITION_PUSH) )
		{
			fPlantUtil *= 2.0f;
			fDefuseBombUtil *= 2.0f;
			fDefendBombUtil *= 0.75f;
			fDefendUtil *= 0.75f;
		}

		if ( hasSomeConditions(CONDITION_DEFENSIVE) )
		{
			fDefendBombUtil *= 2.0f;
			fDefendUtil *= 2.0f;
		}

		ADD_UTILITY(BOT_UTIL_PLANT_BOMB,m_bHasBomb && (iNumBombsToPlant>0),fPlantUtil )
		ADD_UTILITY(BOT_UTIL_DEFUSE_BOMB,(iNumBombsToDefuse>0), fDefuseBombUtil)
		ADD_UTILITY(BOT_UTIL_DEFEND_BOMB,(iNumBombsToDefend>0), fDefendBombUtil)

		ADD_UTILITY(BOT_UTIL_PICKUP_BOMB,!m_bHasBomb && (iNumBombsToPlant>0),hasSomeConditions(CONDITION_NEED_BOMB) ? 0.9f : fPlantUtil)

		if ( iNumEnemyBombsOnMap > 0 )
		{
			ADD_UTILITY(BOT_UTIL_DEFEND_POINT,(iFlagsOwned>0)&&(m_pNearestFlag==NULL)||CDODMod::m_Flags.ownsFlag(iFlagID,m_iTeam),fDefendUtil)

			/*if ( m_pNearestFlag )
			{
				// attack the flag if I've reached the last one
				ADD_UTILITY_DATA_VECTOR(BOT_UTIL_ATTACK_NEAREST_POINT,
					!CDODMod::m_Flags.ownsFlag(iFlagID,m_iTeam) && (CDODMod::m_Flags.numCappersRequired(iFlagID,m_iTeam)-
					CDODMod::m_Flags.numFriendliesAtCap(iFlagID,m_iTeam))<=1,(iFlagsOwned == (iNumFlags-1)) ? 0.9f : 0.75f,iFlagID,CBotGlobals::entityOrigin(m_pNearestFlag));
			}*/
		}

		if ( m_pNearestBomb && (!inSquad() || (isSquadLeader() || (hasSomeConditions(CONDITION_SQUAD_IDLE) && (distanceFrom(m_pNearestBomb)<CWaypointLocations::REACHABLE_RANGE))) ) )
		{
			const Vector vBomb = CBotGlobals::entityOrigin(m_pNearestBomb);

			iFlagID = CDODMod::m_Flags.getBombID(m_pNearestBomb);

			// attack the flag if I've reached the last one
			ADD_UTILITY_DATA_VECTOR(BOT_UTIL_PLANT_NEAREST_BOMB,
				CDODMod::m_Flags.canPlantBomb(m_iTeam,iFlagID),fPlantUtil+randomFloat(-0.05f,0.1f),iFlagID,vBomb)
			// attack the flag if I've reached the last one
			ADD_UTILITY_DATA_VECTOR(BOT_UTIL_DEFEND_NEAREST_BOMB,
				CDODMod::m_Flags.canDefendBomb(m_iTeam,iFlagID),fDefendBombUtil+randomFloat(-0.05f,0.1f),iFlagID,vBomb)
			// attack the flag if I've reached the last one
			ADD_UTILITY_DATA_VECTOR(BOT_UTIL_DEFUSE_NEAREST_BOMB,
				CDODMod::m_Flags.canDefuseBomb(m_iTeam,iFlagID),fDefuseBombUtil+randomFloat(-0.05f,0.1f),iFlagID,vBomb)
		}
	}
	
	if ( bCanMessAround )
	{
		ADD_UTILITY(BOT_UTIL_MESSAROUND,(getHealthPercent()>0.75f), fAttackUtil )
	}

	if ( !rcbot_melee_only.GetBool() && (m_pNearestWeapon.get() != nullptr) && hasSomeConditions(CONDITION_NEED_AMMO) )
	{
		CWeapon *pNearestWeapon = CWeapons::getWeapon(m_pNearestWeapon.get()->GetClassName());
		const CBotWeapon *pHaveWeapon = (pNearestWeapon== nullptr)? nullptr :(m_pWeapons->getWeapon(pNearestWeapon));

		if ( pNearestWeapon && (!pHaveWeapon || !pHaveWeapon->hasWeapon() || pHaveWeapon->outOfAmmo(this) ) )
		{
			ADD_UTILITY(BOT_UTIL_PICKUP_WEAPON, true, 0.6f + pNearestWeapon->getPreference()*0.1f)
		}
		//BOT_UTIL_DOD_PICKUP_OBJ
	}
	// sniping or machinegunning
	if ( !rcbot_melee_only.GetBool() && hasSniperRifle() )
	{
		// perturbation = (numberofclassonteam/numberofplayersonteam)

		pWeapon = getSniperRifle();

		if ( pWeapon )
			ADD_UTILITY_WEAPON_DATA(BOT_UTIL_SNIPE,pWeapon->getAmmo(this)>0,hasSomeConditions(CONDITION_DEFENSIVE) ? 1.6f : (getHealthPercent() * randomFloat(0.75f,1.0f)),pWeapon,hasSomeConditions(CONDITION_DEFENSIVE) ? false : (fAttackUtil>fDefendUtil))
	}
	else if ( !rcbot_melee_only.GetBool() && hasMG() )
	{
		pWeapon = getMG();

		if ( pWeapon )
			ADD_UTILITY_WEAPON_DATA(BOT_UTIL_SNIPE,pWeapon->getAmmo(this)>0,hasSomeConditions(CONDITION_DEFENSIVE) ? 1.6f : (getHealthPercent()),pWeapon,hasSomeConditions(CONDITION_DEFENSIVE) ? false : (fAttackUtil>fDefendUtil))
	}

	// grenades
	if ( !rcbot_melee_only.GetBool() && !hasSomeConditions(CONDITION_SEE_CUR_ENEMY) && hasSomeConditions(CONDITION_SEE_LAST_ENEMY_POS) && m_pLastEnemy && m_fLastSeeEnemy && ((m_fLastSeeEnemy + 10.0f) > engine->Time()) && 
		(m_pWeapons->hasWeapon(DOD_WEAPON_FRAG_US) || m_pWeapons->hasWeapon(DOD_WEAPON_FRAG_GER) || m_pWeapons->hasWeapon(DOD_WEAPON_SMOKE_US) || m_pWeapons->hasWeapon(DOD_WEAPON_SMOKE_GER)) )
	{
		const float fDistance = distanceFrom(m_vLastSeeEnemyBlastWaypoint);
		const float fGrenUtil =  0.85f + ( (1.0f - getHealthPercent()) * 0.15f);

		CBotWeapon *pBotWeapon = nullptr;
		CBotWeapon* pBotSmokeGren = m_pWeapons->hasWeapon(DOD_WEAPON_SMOKE_US)
			                            ? m_pWeapons->getWeapon(CWeapons::getWeapon(DOD_WEAPON_SMOKE_US))
			                            : m_pWeapons->getWeapon(CWeapons::getWeapon(DOD_WEAPON_SMOKE_GER));

		if ( (hasSomeConditions(CONDITION_COVERT)||(m_fCurrentDanger >= 25.0f)) && pBotSmokeGren && pBotSmokeGren->hasWeapon() )
			pBotWeapon = pBotSmokeGren;
		else if ( m_pWeapons->hasWeapon(DOD_WEAPON_FRAG_US) )
			pBotWeapon = m_pWeapons->getWeapon(CWeapons::getWeapon(DOD_WEAPON_FRAG_US));
		else if ( m_pWeapons->hasWeapon(DOD_WEAPON_FRAG_GER) )
			pBotWeapon = m_pWeapons->getWeapon(CWeapons::getWeapon(DOD_WEAPON_FRAG_GER));
				
		// if within throw distance and outside balst radius, I can throw it
		if ( pBotWeapon && (!pBotWeapon->isExplosive() || (fDistance > BLAST_RADIUS)) && ( fDistance < (MAX_GREN_THROW_DIST+BLAST_RADIUS) ) )
		{
			ADD_UTILITY_WEAPON(BOT_UTIL_THROW_GRENADE, pBotWeapon && (pBotWeapon->getAmmo(this) > 0),
			                   hasSomeConditions(CONDITION_GREN) ? fGrenUtil*2 : fGrenUtil, pBotWeapon)
		}
	}

	// this is commented out for a future feature
	/*if ( m_pNearbyWeapon.get() )
	{
		pWeapon = CWeapons::getWeapon(m_pNearbyWeapon.get()->GetClassName());

		if ( pWeapon && !m_pWeapons->hasWeapon(pWeapon->getID()) )
		{
			ADD_UTILITY(BOT_UTIL_PICKUP_WEAPON, true , 0.6f + pWeapon->getPreference()*0.1f);
		}
	}*/

	utils.execute();

	while ( (next = utils.nextBest()) != nullptr)
	{
		if ( !m_pSchedules->isEmpty() && bCheckCurrent )
		{
			if ( m_CurrentUtil != next->getId() )
				m_pSchedules->freeMemory();
			else
			{
				removeCondition(CONDITION_DEFENSIVE);
				break;
			}
		} 

		bCheckCurrent = false;

		if ( executeAction(next) )
		{
			m_CurrentUtil = next->getId();

			removeCondition(CONDITION_DEFENSIVE);

			if ( m_fUtilTimes[next->getId()] < engine->Time() )
				m_fUtilTimes[next->getId()] = engine->Time() + randomFloat(0.1f,2.0f); // saves problems with consistent failing

			if ( CClients::clientsDebugging(BOT_DEBUG_UTIL) )
				CClients::clientDebugMsg(BOT_DEBUG_UTIL,g_szUtils[next->getId()],this);
			
			break;
		}
	}

	utils.freeMemory();
}

bool CDODBot :: select_CWeapon ( CWeapon *pWeapon )
{
	char cmd[128];

	std::sprintf(cmd,"use %s\n",pWeapon->getWeaponName());

	helpers->ClientCommand(m_pEdict,cmd);

	return true;
}

bool CDODBot :: selectBotWeapon ( CBotWeapon *pBotWeapon )
{
	const CWeapon *pSelect = pBotWeapon->getWeaponInfo();

	if ( pSelect )
	{
		//int id = pSelect->getWeaponIndex();
		char cmd[128];

		std::sprintf(cmd,"use %s\n",pSelect->getWeaponName());

		helpers->ClientCommand(m_pEdict,cmd);

		return true;
	}
	failWeaponSelect();

	return false;
}

void CDODBot :: updateConditions ()
{
	CBot::updateConditions();

	if ( m_pPrimaryWeapon != nullptr)
	{
		if ( m_pPrimaryWeapon->outOfAmmo(this) )
		{
			updateCondition(CONDITION_NEED_AMMO);
		}
	}
}

bool CDODBot :: walkingTowardsWaypoint ( CWaypoint *pWaypoint, bool *bOffsetApplied, Vector &vOffset )
{
	if ( pWaypoint->hasFlag(CWaypointTypes::W_FL_PRONE) )
	{
		m_fCurrentDanger = 80;
		removeCondition(CONDITION_RUN);
		updateCondition(CONDITION_PRONE);
	}
	else
		removeCondition(CONDITION_PRONE);

	if ( CBot::walkingTowardsWaypoint(pWaypoint,bOffsetApplied,vOffset) )
	{
		if ( pWaypoint->hasFlag(CWaypointTypes::W_FL_BOMB_TO_OPEN) )
		{	
			vOffset += (CDODMod::getGround(pWaypoint) - pWaypoint->getOrigin());
		}

		return true;
	}

	return false;
}


void CDODBot :: modAim ( edict_t *pEntity, Vector &v_origin, 
						Vector *v_desired_offset, Vector &v_size, 
						float fDist, float fDist2D )
{
	//static Vector vAim;
	//static bool bProne;

	static float fStamina; //Unused? [APG]RoboCop[CL]
	static CBotWeapon *pWp;
	static Vector vel;
	static int index;
	bool bIsEnemyProne;
	float fEnemyStamina;

	bool bAddHeadHeight = false; //Unused? [APG]RoboCop[CL]

	CBot::modAim(pEntity,v_origin,v_desired_offset,v_size,fDist,fDist2D);

	pWp = getCurrentWeapon();

	// CRASH fix
	bIsEnemyProne = false;
	index = ENTINDEX(pEntity);
	// for some reason, prone does not change collidable size
	if ( (index > 0) && (index <= gpGlobals->maxClients) )
	{
		CClassInterface::getPlayerInfoDOD(pEntity,&bIsEnemyProne,&fEnemyStamina);
			// .. so update 
		if ( bIsEnemyProne )
		{
			v_desired_offset->z -= randomFloat(0.0f,8.0f);
		}
		// aiming for head done in Cbot::getaimVector
		/*else if ( hasSomeConditions(CONDITION_SEE_ENEMY_HEAD) )
		{
			// add head height (body height already added)
			bAddHeadHeight = true; 
		}*/

	}

	// weapon is known
	if ( pWp != nullptr)
	{
		if ( pWp->isProjectile() )
		{
			if ( CClassInterface::getVelocity(pEntity,&vel) )
				*v_desired_offset = *v_desired_offset + (vel * randomFloat(m_pProfile->m_fAimSkill-0.1f,m_pProfile->m_fAimSkill+0.1f));

			if ( v_origin.z <= getOrigin().z )
			{
				// shoot the ground
				*v_desired_offset = *v_desired_offset - Vector(0,0,randomFloat(16.0f,32.0f));
			}

			if ( pWp->getProjectileSpeed() > 0 && sv_gravity.IsValid() )
				{
					const float fTime = fDist2D/pWp->getProjectileSpeed();
				//TODO: Improve on the floating point precision conversion [APG]RoboCop[CL]
				v_desired_offset->z = (std::pow(2, fTime) * (sv_gravity.GetFloat() * rcbot_projectile_tweak.GetFloat()));// - (getOrigin().z - v_origin.z);
			}
			//v_desired_offset->z += (distanceFrom(pEntity) * (randomFloat(0.05,0.15)*m_pProfile->m_fAimSkill));
		}
	}

	// if I know the enemy is near a smoke grenade i'll fire randomly into the cloud
	if ( m_pNearestSmokeToEnemy )
	{
		static short int iSlot;
		iSlot = ENTINDEX(pEntity)-1;

		if (( iSlot >= 0 ) && ( iSlot < RCBOT_MAXPLAYERS ))
		{
			static smoke_t *smokeinfo;
			smokeinfo = &(m_CheckSmoke[iSlot]);

			if ( smokeinfo->bInSmoke ) 
			{
				*v_desired_offset = *v_desired_offset + Vector(randomFloat(-SMOKE_RADIUS,SMOKE_RADIUS)*smokeinfo->fProb,randomFloat(-SMOKE_RADIUS,SMOKE_RADIUS)*smokeinfo->fProb,randomFloat(-SMOKE_RADIUS,SMOKE_RADIUS)*smokeinfo->fProb);
				//return vAim = vAim + Vector(randomFloat(-SMOKE_RADIUS,SMOKE_RADIUS)*smokeinfo->fProb,randomFloat(-SMOKE_RADIUS,SMOKE_RADIUS)*smokeinfo->fProb,randomFloat(-SMOKE_RADIUS,SMOKE_RADIUS)*smokeinfo->fProb);
			}
		}
	}

	//return vAim;
}

bool CDODBot :: isVisibleThroughSmoke ( edict_t *pSmoke, edict_t *pCheck )
{
	static float fTime = 0.0f; //Unused? [APG]RoboCop[CL]
	static float fProb = 0.0f; //Unused? [APG]RoboCop[CL]

	static short int iSlot;

	iSlot = ENTINDEX(pCheck)-1;

	// if pCheck is a player
	if (( iSlot >= 0 ) && ( iSlot < RCBOT_MAXPLAYERS ))
	{
		static smoke_t *smokeinfo;
		smokeinfo = &(m_CheckSmoke[iSlot]);

		// last time i checked was long enough ago
		if ( smokeinfo->fLastTime < engine->Time() )
		{
			static float fDist;
			static float fSmokeDist;
			static Vector vSmoke;
			smokeinfo->bVisible = true;
			smokeinfo->bInSmoke = false;

			vSmoke = CBotGlobals::entityOrigin(pSmoke);
			fSmokeDist = distanceFrom(vSmoke);

			// I'm outside smoke -- but maybe the enemy is inside the smoke
			if ( fSmokeDist > SMOKE_RADIUS )
			{
				fDist = (CBotGlobals::entityOrigin(pCheck) - vSmoke).Length();

				// enemy outside the smoke radius
				if ( fDist > SMOKE_RADIUS )
				{
					static Vector vCheckComp;
					// check if enemy is behind the smoke from my perspective
					vCheckComp = CBotGlobals::entityOrigin(pCheck) - getOrigin();
					vCheckComp = (vCheckComp / vCheckComp.Length())*fSmokeDist;

					fDist = (vSmoke - (getOrigin() + vCheckComp)).Length();
				}
			}
			else
				fDist = fSmokeDist;

			if ( fDist <= SMOKE_RADIUS ) 
				// smoke gets heavy at 1.0 seconds and diminishes at 10 secs
			{
				smokeinfo->fProb = 1.0f-(fDist/SMOKE_RADIUS);
				smokeinfo->bVisible = (randomFloat(0.0f,0.33f) > smokeinfo->fProb ); 
				// smoke gets pretty heavy half way into the smoke grenade
				smokeinfo->bInSmoke = true;
			}
				
			#ifdef _DEBUG
				if ( CClients::clientsDebugging(BOT_DEBUG_THINK) )
					CClients::clientDebugMsg(this,BOT_DEBUG_THINK,"Smoke Test (%s to %s) = %s",m_szBotName,engine->GetPlayerNetworkIDString(pCheck),smokeinfo->bVisible ? "visible" : "invisible");
			#endif

		}

		// check again soon (typical reaction time delay)
		smokeinfo->fLastTime = engine->Time() + randomFloat(0.15f,0.3f);

		return smokeinfo->bVisible;

	}

	return true;
//}

	//return false;
}
