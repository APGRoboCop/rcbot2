/*
 *    part of https://rcbot2.svn.sourceforge.net/svnroot/rcbot2
 *
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

#include "bot_mtrand.h"
#include "bot.h"
#include "bot_cvars.h"
#include "bot_schedule.h"
#include "bot_task.h"
#include "bot_navigator.h"
#include "bot_waypoint_locations.h"
#include "bot_globals.h"
#include "in_buttons.h"
#include "bot_weapons.h"
#include "bot_hldm_bot.h"
#include "bot_fortress.h"
#include "bot_css_bot.h"
#include "bot_profiling.h"
#include "bot_getprop.h"
#include "bot_dod_bot.h"
#include "bot_squads.h"
#include "bot_waypoint_visibility.h"
#include "bot_synergy.h"

#ifdef WIN32
#undef min
#undef max
#endif

// desx and desy must be normalized
// desx = distance (should be 2d)
// desy = height offset
void getGrenadeAngle ( double v, double g, double desx, double desy, float *fa1, float *fa2 )
{
	//normalize
	const float fmax = MAX(v, MAX(g, MAX(desx, desy)));

	v /= fmax;
	g /= fmax;
	desx /= fmax;
	desy /= fmax;
	const double vsquared = v * v;
	const double vfourth = vsquared * vsquared;
	const double x2 = desx * desx;
	const double gx2 = g * x2;
	const double twoyv2 = 2 * desy * vsquared;
	const double fourabplusa = vfourth - g * (gx2 + twoyv2);
	const double topplus = vsquared + std::sqrt(fourabplusa);
	const double topminus = vsquared - std::sqrt(fourabplusa);
	const double bottom = g * desx;

	*fa1 = static_cast<float>(std::atan2(topplus, bottom));
	*fa2 = static_cast<float>(std::atan2(topminus, bottom));

	*fa1 = RAD2DEG(*fa1);
	*fa2 = RAD2DEG(*fa2);
}

float getGrenadeZ ( edict_t *pShooter, edict_t *pTarget, const Vector& vOrigin, const Vector& vTarget, float fInitialSpeed ) 
{
	float fAngle1,fAngle2;

	Vector vForward;

	const Vector v_comp = vTarget-vOrigin;

	const float fDist2D = v_comp.Length2D();
	const float fDist3D = v_comp.Length();

	getGrenadeAngle(fInitialSpeed,sv_gravity.GetFloat(),fDist2D,vTarget.z-vOrigin.z,&fAngle1,&fAngle2);

	QAngle angles = QAngle(0, 0, 0);
	// do a quick traceline to check which angle to choose (minimum angle = straight)

	if (CBotGlobals::isShotVisible(pShooter,vOrigin,vTarget,pTarget))
		angles.x = -MIN(fAngle1,fAngle2);
	else
		angles.x = -MAX(fAngle1,fAngle2);

	AngleVectors(angles,&vForward);
	
	// add gravity height
	return (vOrigin + vForward*fDist3D).z;
}
////////////////////////////////////////////////////////////////////////////////////////////
// Tasks

CBotTF2MedicHeal :: CBotTF2MedicHeal ()
{
	m_pHeal = nullptr;
	m_bHealerJumped = false;
}

void CBotTF2MedicHeal::execute(CBot *pBot,CBotSchedule *pSchedule)
{
	pBot->wantToShoot(false);
	pBot->wantToListen(false);

	if ( !pBot->isTF2() )
	{
		fail();
		return;
	}

	CBotTF2* pBotTF2 = static_cast<CBotTF2*>(pBot);

	edict_t* pHeal = pBotTF2->getHealingEntity();

	if ( pHeal && !m_bHealerJumped )
	{
		Vector vVelocity;

		if ( !CClassInterface::getVelocity(pHeal,&vVelocity) || vVelocity.Length2D() > 1.0f )
		{
			IPlayerInfo *p = playerinfomanager->GetPlayerInfo(pHeal);
		
			if ( p )
			{
				if ( p->GetLastUserCommand().buttons & IN_JUMP )
				{
					m_bHealerJumped = true;
					m_vJump = CBotGlobals::entityOrigin(pHeal);
				}
			}
		}
	}

	if ( !pHeal )
	{
		// because the medic would have followed this guy, he would have lost his own waypoint
		pBot->getNavigator()->rollBackPosition();
		fail();
	}
	else if (pBot->getCurrentWeapon() == nullptr) //Revert fix by caxanga334
	{
		pBotTF2->clearHealingEntity();
		pBot->getNavigator()->rollBackPosition();
		fail();
	}
	else if (pBotTF2->getHealFactor(pHeal) == 0.0f)
	{
		pBotTF2->clearHealingEntity();
		pBot->getNavigator()->rollBackPosition();
		fail();
	}/*
	else if ( !pBot->isVisible(pHeal) )
	{
		pBot->getNavigator()->rollBackPosition();
		((CBotFortress*)pBot)->clearHealingEntity();
		fail();
	}*/
	else if ( pBot->distanceFrom(pHeal) > 416 )
	{
		pBotTF2->clearHealingEntity();
		pBot->getNavigator()->rollBackPosition();
		fail();
	}
	else if ( pBot->getCurrentWeapon()->getWeaponInfo()->getID() != TF2_WEAPON_MEDIGUN )
	{
		pBot->select_CWeapon( CWeapons::getWeapon(TF2_WEAPON_MEDIGUN) );
	}
	else 
	{
		Vector vVelocity;

		CClassInterface::getVelocity(pBot->getEdict(),&vVelocity);

		pBot->clearFailedWeaponSelect();

		if ( m_bHealerJumped && pBot->distanceFrom(m_vJump) < 64.0f && vVelocity.Length()>1.0f )
		{
			pBot->jump();
			m_bHealerJumped = false;
		}

		if ( !pBotTF2->healPlayer() )
		{
			pBot->getNavigator()->rollBackPosition();
			pBotTF2->clearHealingEntity();
			fail();
		}
	}
}

///////////

CBotTF2ShootLastEnemyPosition :: CBotTF2ShootLastEnemyPosition  (const Vector& vPosition, edict_t *pEnemy, const Vector& m_vVelocity )
{
	const float len = m_vVelocity.Length();

	m_vPosition = vPosition;

	if ( len > 0 )
		m_vPosition = m_vPosition - m_vVelocity/m_vVelocity.Length()*16;
	
	m_pEnemy = pEnemy;
	m_fTime = 0.0f;
}

void CBotTF2ShootLastEnemyPosition ::  execute (CBot *pBot,CBotSchedule *pSchedule)
{
	const CBotWeapon *pWeapon = pBot->getCurrentWeapon();
	CBotTF2 *pTF2Bot = static_cast<CBotTF2*>(pBot);
	CWeapon *pChange = nullptr;

	if ( m_fTime == 0.0f )
		m_fTime = engine->Time() + randomFloat(2.0f,4.5f);

	if ( m_fTime < engine->Time() )
	{
		complete();
		return;
	}

	if ( !CBotGlobals::entityIsValid(m_pEnemy) || !CBotGlobals::entityIsAlive(m_pEnemy) )
	{
		complete();
		return;
	}

	if ( pBot->getEnemy() && pBot->hasSomeConditions(CONDITION_SEE_CUR_ENEMY) )
	{
		fail();
		return;
	}

	pBot->wantToShoot(false);
	pBot->wantToChangeWeapon(false);
	pBot->wantToListen(false);

	if ( pTF2Bot->getClass()  == TF_CLASS_SOLDIER )
	{
		pChange = CWeapons::getWeapon(TF2_WEAPON_ROCKETLAUNCHER);
	}
	else if ( pTF2Bot->getClass() == TF_CLASS_DEMOMAN )
	{
		pChange = CWeapons::getWeapon(TF2_WEAPON_GRENADELAUNCHER);
	}

	if ( !pChange )
	{
		fail();
		return;
	}

	CBotWeapon* pChangeTo = pBot->getWeapons()->getWeapon(pChange);
	
	if ( pChangeTo->getAmmo(pBot) < 1 )
	{
		complete();
		return;
	}

	if ( pChangeTo != pWeapon )
	{
		pBot->selectBotWeapon(pChangeTo);
	}
	else
	{
		if ( randomInt(0,1) )
			pBot->primaryAttack(false);
	}

	pBot->setLookVector(m_vPosition);
	pBot->setLookAtTask(LOOK_VECTOR);

}

void CBotTF2ShootLastEnemyPosition :: debugString ( char *string )
{
	std::sprintf(string,"CBotTF2ShootLastEnemyPosition\nm_vPosition = (%0.4f,%0.4f,%0.4f)",m_vPosition.x,m_vPosition.y,m_vPosition.z);
}

/////////////

CBotTF2WaitHealthTask :: CBotTF2WaitHealthTask (const Vector& vOrigin )
{
	m_vOrigin = vOrigin;
	m_fWaitTime = 0.0f;
}

void CBotTF2WaitHealthTask :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	if ( !m_fWaitTime )
		m_fWaitTime = engine->Time() + randomFloat(5.0f,10.0f);

	 if ( !pBot->hasSomeConditions(CONDITION_NEED_HEALTH) )
		complete();
	 else if ( m_fWaitTime < engine->Time() )
		 fail();
	else
	{
		// TODO:
		/*edict_t *pOtherPlayer = CBotGlobals::findNearestPlayer(m_vOrigin,50.0,pBot->getEdict());

		if ( pOtherPlayer )
		{
			fail();
			return;
		}*/

		pBot->setLookAtTask(LOOK_AROUND);

		if ( pBot->distanceFrom(m_vOrigin) > 50 )
			pBot->setMoveTo(m_vOrigin);
		else
			pBot->stopMoving();

		if ( pBot->isTF() )
		{
			static_cast<CBotTF2*>(pBot)->taunt();

			if ( static_cast<CBotTF2*>(pBot)->isBeingHealed() )
				complete();
		}
	}
}

void CBotTF2WaitHealthTask :: debugString ( char *string )
{
	std::sprintf(string,"CBotTF2WaitHealthTask\nm_vOrigin = (%0.4f,%0.4f,%0.4f)",m_vOrigin.x,m_vOrigin.y,m_vOrigin.z);
}


CBotTF2WaitFlagTask :: CBotTF2WaitFlagTask (const Vector& vOrigin, bool bFind )
{
	m_vOrigin = vOrigin;
	m_fWaitTime = 0.0f;
	m_bFind = bFind;
}

void CBotTF2WaitFlagTask :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	if ( !m_fWaitTime )
	{
		if ( m_bFind )
			m_fWaitTime = engine->Time() + 5.0f;
		else
			m_fWaitTime = engine->Time() + 10.0f;
	}

	if ( static_cast<CBotTF2*>(pBot)->hasFlag() )
		complete();
	else if ( pBot->getHealthPercent() < 0.2f )
	{
		fail();
	}
	else if ( m_fWaitTime < engine->Time() )
	{
		static_cast<CBotFortress*>(pBot)->flagReset();
		fail();
	}
	else if ( !pBot->isTF() )
	{
		fail();
	}
	else
	{		
		if ( !static_cast<CBotFortress*>(pBot)->waitForFlag(&m_vOrigin,&m_fWaitTime,m_bFind) )
		{
			fail();
		}
	}
}

void CBotTF2WaitFlagTask :: debugString ( char *string )
{
	std::sprintf(string,"CBotTF2WaitFlagTask\nm_vOrigin = (%0.4f,%0.4f,%0.4f)",m_vOrigin.x,m_vOrigin.y,m_vOrigin.z);
}
//////////
CBotDODBomb :: CBotDODBomb ( int iBombType, int iBombID, edict_t *pBomb, const Vector& vPosition, int iPrevOwner )
{
	m_iType = iBombType;
	m_iBombID = iBombID; 
	m_fTime = 0.0f;

	if ( m_iBombID == -1 )
		m_iBombID = CDODMod::m_Flags.getBombID(pBomb);

	m_pBombTarget = pBomb;
	m_vOrigin = vPosition;
	m_iPrevTeam = iPrevOwner;
}

void CBotDODBomb :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	bool bWorking = false;

	pBot->wantToShoot(false);

	if ( m_fTime == 0.0f )
	{
		m_fTime = engine->Time() + randomFloat(8.0f,12.0f);
		static_cast<CDODBot*>(pBot)->setNearestBomb(m_pBombTarget);

		if ( m_iType == DOD_BOMB_PLANT || m_iType == DOD_BOMB_PATH_PLANT )
		{
			 if ( CDODMod::m_Flags.isTeamMatePlanting(pBot->getEdict(),pBot->getTeam(),m_iBombID) )
				 fail();
		}
		else
		{
			 if ( CDODMod::m_Flags.isTeamMateDefusing(pBot->getEdict(),pBot->getTeam(),m_iBombID) )
				 fail();
		}
	}
	else if ( m_fTime < engine->Time() )
	{
		fail();
	}

	if ( m_iType == DOD_BOMB_PLANT) 
	{
		bWorking = CClassInterface::isPlayerPlantingBomb_DOD(pBot->getEdict());

		if ( CDODMod::m_Flags.isBombPlanted(m_iBombID) )
		{
			complete();
		}
		else if ( CDODMod::m_Flags.isTeamMatePlanting(pBot->getEdict(),pBot->getTeam(),m_iBombID) )
			complete(); // team mate doing my job
			
		//else if ( !CClassInterface::isPlayerPlantingBomb_DOD(pBot->getEdict()) )// it is still planted
		//	complete(); // bomb is being defused by someone else - give up
	}
	else if ( m_iType == DOD_BOMB_DEFUSE)
	{
		bWorking = CClassInterface::isPlayerDefusingBomb_DOD(pBot->getEdict());

		if ( !CDODMod::m_Flags.isBombPlanted(m_iBombID) )
			complete();
		else if ( CDODMod::m_Flags.isTeamMateDefusing(pBot->getEdict(),pBot->getTeam(),m_iBombID) )
			complete(); // team mate doing my job

		//else if ( CDODMod::m_Flags.isBombBeingDefused(m_iBombID) && !CClassInterface::isPlayerDefusingBomb_DOD(pBot->getEdict()) )// it is still planted
		//	complete(); // bomb is being defused by someone else - give up
	}
	else if ( m_iType == DOD_BOMB_PATH_PLANT ) // a bomb that's in the way 
	{
		bWorking = CClassInterface::isPlayerPlantingBomb_DOD(pBot->getEdict());

		if ( CClassInterface::getDODBombState(m_pBombTarget) != DOD_BOMB_STATE_AVAILABLE )
		{
			//CDODBot *pDODBot = (CDODBot*)pBot;

			//pDODBot->removeBomb();

			complete();
		}
		else if ( CDODFlags::isTeamMatePlanting(pBot->getEdict(),pBot->getTeam(),CBotGlobals::entityOrigin(m_pBombTarget)) )
			complete(); // team mate doing my job

	}
	else if ( m_iType == DOD_BOMB_PATH_DEFUSE ) // a bomb that's in the way 
	{
		bWorking = CClassInterface::isPlayerDefusingBomb_DOD(pBot->getEdict());

		if ( CClassInterface::getDODBombState(m_pBombTarget) == DOD_BOMB_STATE_AVAILABLE )
			complete();
		else if ( CDODFlags::isTeamMateDefusing(pBot->getEdict(),pBot->getTeam(),CBotGlobals::entityOrigin(m_pBombTarget)) )
			complete(); // team mate doing my job
	}

	pBot->setLookVector(m_vOrigin);
	pBot->setLookAtTask(LOOK_VECTOR);

	pBot->use();
	
	if ( !bWorking )
	{
		pBot->setMoveTo(m_vOrigin);
		pBot->setMoveSpeed(CClassInterface::getMaxSpeed(pBot->getEdict())/4);
	}
	else
	{
		pBot->stopMoving();
	}
}

void CBotDODBomb :: debugString ( char *string )
{
	std::sprintf(string,"CBotDODBomb\nm_iType = %d\nm_iBombID = %d\nm_fTime = %0.2f\nm_iPrevTeam = %d",m_iType, m_iBombID,m_fTime,m_iPrevTeam);
}

//////
void CDODWaitForGrenadeTask :: execute (CBot *pBot,CBotSchedule *pSchedule)
{

	if ( m_pGrenade.get() == nullptr)
	{
		complete();
	}
	else if ( !CBotGlobals::entityIsAlive(m_pGrenade) )
		complete();
	else if ( m_fTime == 0.0f )
	{
		CDODBot *pDODBot = static_cast<CDODBot*>(pBot);

		m_fTime = engine->Time() + randomFloat(3.0f,5.0f);

		pDODBot->prone();
	}
	else if ( m_fTime < engine->Time() )
	{
		CDODBot *pDODBot = static_cast<CDODBot*>(pBot);

		complete();

		pDODBot->unProne();
	}
}

void CDODWaitForGrenadeTask :: debugString ( char *string )
{
	std::sprintf(string,"CDODWaitForGrenadeTask");
}
//////////

void CDODWaitForBombTask :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	if ( m_fTime == 0.0f )
	{
		pBot->updateCondition(CONDITION_RUN);
		CWaypoint* pCurrent = CWaypoints::getWaypoint(CWaypointLocations::NearestWaypoint(pBot->getOrigin(), 400.0f,
			CWaypoints::getWaypointIndex(m_pBlocking), true, false, true));

		if ( pCurrent == nullptr)
			pCurrent = m_pBlocking;

		m_fTime = engine->Time() + randomFloat(2.0f,5.0f);
		m_pRunTo = CWaypoints::getNextCoverPoint(pBot,pCurrent,m_pBlocking) ;
	}

	if ( m_pBombTarget.get() == nullptr)
	{
		complete();
		return;
	}

	if ( m_pBombTarget.get()->GetUnknown() == nullptr)
	{
		complete();
		return;
	}

	pBot->updateCondition(CONDITION_RUN);

	if ( m_pRunTo )
	{
		if (m_pRunTo->touched(pBot->getOrigin(),Vector(0,0,0),48.0f) )
		{
			if ( pBot->distanceFrom(m_pBombTarget) > BLAST_RADIUS*2 )
				pBot->stopMoving();
			else
				m_pRunTo = CWaypoints::getNextCoverPoint(pBot,m_pRunTo,m_pBlocking) ;
		}
		else
			pBot->setMoveTo(m_pRunTo->getOrigin());
	}

	if ( m_fTime < engine->Time() )
	{
		complete();
		return;
	}

	if ( CClassInterface::getDODBombState (m_pBombTarget) != 2 )
	{
		complete();
		return;
	}

	pBot->lookAtEdict(m_pBombTarget);
	pBot->setLookAtTask(LOOK_EDICT);
}

void CDODWaitForBombTask :: debugString ( char *string )
{
	std::sprintf(string,"CDODWaitForBombTask");
}

//////////

CBotDODAttackPoint :: CBotDODAttackPoint ( int iFlagID, const Vector& vOrigin, float fRadius )
{
	m_bProne = false;
	m_vOrigin = vOrigin;
	m_fAttackTime = 0.0f;
	m_fTime = 0.0f;
	m_iFlagID = iFlagID;
	m_fRadius = fRadius;
}

void CBotDODAttackPoint :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	static int iTeam;

	iTeam = pBot->getTeam();

	if ( CDODMod::m_Flags.ownsFlag(m_iFlagID,iTeam) )
	{
		complete();

		pBot->resetAreaClear();

		if ( pBot->inSquad() && pBot->isSquadLeader() )
		{
			IBotNavigator *pNav = pBot->getNavigator();

			const float fDanger = pNav->getBelief(CDODMod::m_Flags.getWaypointAtFlag(m_iFlagID));

			if ( randomFloat(0.0f,MAX_BELIEF) < fDanger )
			{
				pBot->addVoiceCommand(DOD_VC_HOLD);
				pBot->updateCondition(CONDITION_DEFENSIVE);
			}
		}

		return;
	}
	if ( m_fAttackTime == 0.0f )
	{
		m_fAttackTime = engine->Time() + randomFloat(30.0f,60.0f);
	}
	else if ( m_fAttackTime < engine->Time() )
	{
		complete();
	}
	else
	{
		if ( m_fTime == 0.0f )
		{
			m_fTime = engine->Time() + randomFloat(2.0f,4.0f);
			m_vMoveTo = m_vOrigin + Vector(randomFloat(-m_fRadius,m_fRadius),randomFloat(-m_fRadius,m_fRadius),0);
			m_bProne = randomFloat(0,1) * (1.0f-pBot->getHealthPercent()) > 0.75f;

			if (  CDODMod::m_Flags.numFriendliesAtCap(m_iFlagID,iTeam) < CDODMod::m_Flags.numCappersRequired(m_iFlagID,iTeam) )
			{
				// count players I see
				CDODBot *pDODBot = static_cast<CDODBot*>(pBot);

				pDODBot->addVoiceCommand(DOD_VC_NEED_BACKUP);
			}
		}
		else if ( m_fTime < engine->Time() )
		{
			m_fTime = 0.0f;
		}
		else
		{
			static float fdist;

			fdist = pBot->distanceFrom(m_vMoveTo);

			if ( m_bProne && !pBot->hasSomeConditions(CONDITION_RUN) )
				pBot->duck();

			if ( fdist < m_fRadius )
			{
				pBot->stopMoving();
				pBot->setLookAtTask(LOOK_AROUND);
			}
			else if ( fdist > 400 )
				fail();
			else
			{				
				pBot->setMoveTo(m_vMoveTo);
			}
		}
	}
}

void CBotDODAttackPoint :: debugString ( char *string )
{
	std::sprintf(string, "CBotDODAttackPoint\nm_iFlagID = %d\n m_vOrigin = (%0.1f,%0.1f,%0.1f,radius = %0.1f)", m_iFlagID,
			m_vOrigin.x, m_vOrigin.y, m_vOrigin.z, m_fRadius);
}

///////////

CBotTF2AttackPoint :: CBotTF2AttackPoint ( int iArea, const Vector& vOrigin, int iRadius )
{
	m_vOrigin = vOrigin;
	m_fAttackTime = 0.0f;
	m_fTime = 0.0f;
	m_iArea = iArea;
	m_iRadius = iRadius;
}

void CBotTF2AttackPoint :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	const int iCpIndex = CTeamFortress2Mod::m_ObjectiveResource.m_WaypointAreaToIndexTranslation[m_iArea];
	const int iTeam = pBot->getTeam();
	CBotTF2 *pTF2Bot = static_cast<CBotTF2*>(pBot);

	if ( CTeamFortress2Mod::TF2_IsPlayerInvuln(pBot->getEdict()) )
		fail();

	pBot->wantToInvestigateSound(false);

	if ( m_iArea && CTeamFortress2Mod::m_ObjectiveResource.GetOwningTeam(iCpIndex) == iTeam )
	{
		complete(); // done
		pTF2Bot->updateAttackDefendPoints();
	}
	else if ( m_iArea && !CTeamFortress2Mod::m_ObjectiveResource.isCPValid(iCpIndex,iTeam,TF2_POINT_ATTACK) )
	{
		fail(); // too slow
		pTF2Bot->updateAttackDefendPoints();
	}
	else if ( m_fAttackTime == 0.0f )
		m_fAttackTime = engine->Time() + randomFloat(30.0f,60.0f);
	else if ( m_fAttackTime < engine->Time() )
		complete();
	else
	{
		if ( m_fTime == 0.0f )
		{
			
			m_fTime = engine->Time() + randomFloat(5.0f,10.0f);
			m_vMoveTo = m_vOrigin + Vector(randomFloat(-m_iRadius,m_iRadius),randomFloat(-m_iRadius,m_iRadius),0);
		}
		else if ( m_fTime < engine->Time() )
		{
			m_fTime = 0.0f;
		}
		else
		{
			static float fdist;

			fdist = pBot->distanceFrom(m_vMoveTo);

			if ( pTF2Bot->getClass() == TF_CLASS_SPY )
			{
				if ( pTF2Bot->isDisguised() )
					pBot->primaryAttack(); // remove disguise to capture

				pTF2Bot->wantToDisguise(false);

				// block cloaking
				if ( pTF2Bot->isCloaked() )
				{
					// uncloak
					pTF2Bot->spyUnCloak();
				}
				else 
				{
					pBot->letGoOfButton(IN_ATTACK2);
				}

				pTF2Bot->waitCloak();

			}

			if ( fdist < 52 )
			{
				pBot->stopMoving();
			}
			else if ( fdist > 400 )
				fail();
			else
			{				
				pBot->setMoveTo(m_vMoveTo);
			}

			pBot->setLookAtTask(LOOK_AROUND);

			if ( static_cast<CBotTF2*>(pBot)->checkAttackPoint() )
				complete();
		}
	}
}

void CBotTF2AttackPoint :: debugString ( char *string )
{
	std::sprintf(string,"CBotTF2AttackPoint (%d,%0.1f,%0.1f,%0.1f,%d)",m_iArea,m_vOrigin.x,m_vOrigin.y,m_vOrigin.z,m_iRadius);
}

////////////////////////
void CPrimaryAttack:: execute ( CBot *pBot, CBotSchedule *pSchedule )
{
	pBot->primaryAttack();
	complete();
}

void CBotWaitTask::execute ( CBot *pBot, CBotSchedule *pSchedule )
{
	pBot->stopMoving();

	if(m_bAimSet)
	{
		pBot->setLookVector(m_vAim);
		pBot->setLookAtTask(LOOK_VECTOR);
	}

	if(engine->Time() >= m_ftime)
	{
		complete();
	}
}

void CBotWaitTask::debugString(char *string)
{
	std::sprintf(string,"CBotWaitTask (%.1f)",m_ftime - engine->Time());
}

////////////////////////////

CBotTF2PushPayloadBombTask :: CBotTF2PushPayloadBombTask (edict_t * pPayloadBomb)
{
	m_pPayloadBomb = pPayloadBomb;
	m_fPushTime = 0.0f;
	m_fTime = 0.0f;
	m_vRandomOffset = Vector(0,0,0);
}

void CBotTF2PushPayloadBombTask :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	pBot->wantToInvestigateSound(false);

	if ( m_fPushTime == 0.0f )
	{
		m_fPushTime = engine->Time() + randomFloat(10.0f,30.0f);
		m_vRandomOffset = Vector(randomFloat(-50,50),randomFloat(-50,50),0);
	}
	else if ( m_fPushTime < engine->Time() )
	{
		complete();
	}
	else if(m_pPayloadBomb.get() == nullptr)
	{
		complete();
	}
	else 
	{

		m_vOrigin = CBotGlobals::entityOrigin(m_pPayloadBomb);
		//m_vMoveTo = m_vOrigin + Vector(randomFloat(-10,10),randomFloat(-10,10),0);
		m_vMoveTo = m_vOrigin + m_vRandomOffset;

		if ( pBot->distanceFrom(m_vMoveTo) < 100 )
		{	
			if ( static_cast<CBotTF2*>(pBot)->getClass() == TF_CLASS_SPY && static_cast<CBotTF2*>(pBot)->isDisguised())
				pBot->primaryAttack(); // remove disguise to capture

			static_cast<CBotFortress*>(pBot)->wantToDisguise(false);

		}
		else
			pBot->setMoveTo(m_vMoveTo);

		pBot->setLookAtTask(LOOK_AROUND);
	}
}

void CBotTF2PushPayloadBombTask :: debugString ( char *string )
{
	std::sprintf(string,"CBotTF2PushPayloadBombTask (%0.1f,%0.1f,%0.1f)",m_vOrigin.x,m_vOrigin.y,m_vOrigin.z);
}
////////////////////////////////////////////////////////////////////////

CBotTF2DefendPayloadBombTask :: CBotTF2DefendPayloadBombTask (edict_t * pPayloadBomb)
{
	m_pPayloadBomb = pPayloadBomb;
	m_fDefendTime = 0.0f;
	m_fTime = 0.0f;
	m_vRandomOffset = Vector(0,0,0);
}

void CBotTF2DefendPayloadBombTask :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	if ( m_fDefendTime == 0.0f )
	{
		m_fDefendTime = engine->Time() + randomFloat(10.0f,30.0f);
		m_vRandomOffset = Vector(randomFloat(-150.0f,150.0f),randomFloat(-150.0f,150.0f),0);
	}
	else if ( m_fDefendTime < engine->Time() )
	{
		complete();
	}
	else if(m_pPayloadBomb.get() == nullptr)
	{
		complete();
	}
	else
	{
		m_vOrigin = CBotGlobals::entityOrigin(m_pPayloadBomb);
		m_vMoveTo = m_vOrigin + m_vRandomOffset;

		if ( pBot->distanceFrom(m_vMoveTo) > 200 )
			pBot->setMoveTo(m_vMoveTo);
		else
			pBot->stopMoving();

		pBot->setLookAtTask(LOOK_EDICT);
		pBot->lookAtEdict(m_pPayloadBomb);
	}

}

void CBotTF2DefendPayloadBombTask :: debugString ( char *string )
{
	std::sprintf(string,"CBotTF2DefendPayloadBombTask (%0.1f,%0.1f,%0.1f)",m_vOrigin.x,m_vOrigin.y,m_vOrigin.z);
}
//////////////////////
CBotTF2DefendPoint :: CBotTF2DefendPoint ( int iArea, const Vector& vOrigin, int iRadius )
{
	m_vOrigin = vOrigin;
	m_fDefendTime = 0.0f;
	m_fTime = 0.0f;
	m_iArea = iArea;
	m_iRadius = iRadius;
}

void CBotTF2DefendPoint :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	const int iCpIndex = CTeamFortress2Mod::m_ObjectiveResource.m_WaypointAreaToIndexTranslation[m_iArea];
	const int iTeam = pBot->getTeam();

	if ( m_iArea && CTeamFortress2Mod::m_ObjectiveResource.GetOwningTeam(iCpIndex) != iTeam )
	{
		// doesn't belong to us/can't defend anymore
		static_cast<CBotTF2*>(pBot)->updateAttackDefendPoints();
		complete(); // done
	}
	else if ( m_iArea && !CTeamFortress2Mod::m_ObjectiveResource.isCPValid(iCpIndex,iTeam,TF2_POINT_DEFEND) )
	{
		static_cast<CBotTF2*>(pBot)->updateAttackDefendPoints();
		fail(); // too slow
	}
	else if ( m_fDefendTime == 0.0f )
	{
		m_fDefendTime = engine->Time() + randomFloat(30.0f,60.0f);
		pBot->resetLookAroundTime();
	}
	else if ( m_fDefendTime < engine->Time() )
		complete();
	else
	{
		if ( m_fTime == 0.0f )
		{
			m_fTime = engine->Time() + randomFloat(5.0f,10.0f);
			m_vMoveTo = m_vOrigin + Vector(randomFloat(-m_iRadius,m_iRadius),randomFloat(-m_iRadius,m_iRadius),0);
			const float fdist = pBot->distanceFrom(m_vMoveTo);

			if ( fdist < 32 )
				pBot->stopMoving();
			else if ( fdist > 400 )
				fail();
			else
			{				
				pBot->setMoveTo(m_vMoveTo);
			}		
		}
		else if ( m_fTime < engine->Time() )
		{
			m_fTime = 0.0f;
		}
		pBot->setLookAtTask(LOOK_SNIPE);
	}
}

void CBotTF2DefendPoint :: debugString ( char *string )
{
	std::sprintf(string, "CBotTF2DefendPoint\nm_iArea=%d\nm_vOrigin=(%0.1f,%0.1f,%0.1f,%d)", m_iArea, 
		m_vOrigin.x, m_vOrigin.y, m_vOrigin.z, m_iRadius);
}

///////////
CBotTF2UpgradeBuilding :: CBotTF2UpgradeBuilding ( edict_t *pBuilding )
{
	m_pBuilding = pBuilding;
	m_fTime = 0.0f;
}

void CBotTF2UpgradeBuilding :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	edict_t *pBuilding = m_pBuilding.get();
	edict_t *pOwner;
	pBot->wantToShoot(false);
	pBot->wantToInvestigateSound(false);
	pBot->wantToListen(false);

	if (!m_fTime )
		m_fTime = engine->Time() + randomFloat(9.0f,11.0f);
	
	if ( m_fTime<engine->Time() )
	{
		complete();
	}// Fix 16/07/09
	else if ( pBuilding == nullptr)
	{
		fail();
	}
	else if ( (pOwner = CTeamFortress2Mod::getSentryOwner(pBuilding)) != nullptr && 
		CClassInterface::isCarryingObj(pOwner) && CClassInterface::getCarriedObj(pOwner)==pBuilding)
	{
		// Owner is carrying it
		fail();
	}
	else if ( !pBot->isVisible(pBuilding) )
	{
		if ( pBot->distanceFrom(pBuilding) > 200 )
			fail();
		else if ( pBot->distanceFrom(pBuilding) > 100 )
			pBot->setMoveTo(CBotGlobals::entityOrigin(pBuilding));
		
		pBot->setLookAtTask(LOOK_EDICT,0.15f);
		pBot->lookAtEdict(pBuilding);
	}
	else if ( CBotGlobals::entityIsValid(pBuilding) && CBotGlobals::entityIsAlive(pBuilding) )
	{
		if ( !static_cast<CBotFortress*>(pBot)->upgradeBuilding(pBuilding) )
			fail();		
	}
	else
		fail();
}

void CBotTF2UpgradeBuilding:: debugString ( char *string )
{
	std::sprintf(string,"CBotTF2UpgradeBuilding");
}

void CBotHL2DMUseButton :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	static Vector vOrigin;
	
	if ( m_pButton.get() == nullptr)
	{
		fail();
		return;
	}

	vOrigin = CBotGlobals::entityOrigin(m_pButton);

	if ( m_fTime == 0.0f )
	{
		m_fTime = engine->Time() + randomFloat(4.0f,6.0f);
	}

	if ( m_fTime < engine->Time() )
		complete();

	//if ( CClassInterface::getAnimCycle(m_pCharger) == 1.0f )
	//	complete();

	if(m_bOverrideLook)
	{
		pBot->setMoveLookPriority(MOVELOOK_OVERRIDE);
		pBot->setLookVector(vOrigin);
		pBot->setLookAtTask(LOOK_VECTOR);
		pBot->setMoveLookPriority(MOVELOOK_TASK);
	}
	else
	{
		pBot->setLookVector(vOrigin);
		pBot->setLookAtTask(LOOK_VECTOR);		
	}


	if ( pBot->distanceFrom(m_pButton) > 96 )
	{
		pBot->setMoveTo(vOrigin);
	}
	else if ( pBot->isFacing(vOrigin) )
	{
		pBot->use();
		complete();
	}
}

void CBotHL2DMUseCharger :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	static Vector vOrigin;
	
	if ( m_pCharger.get() == nullptr)
	{
		fail();
		return;
	}

	vOrigin = CBotGlobals::entityOrigin(m_pCharger);

	if ( m_fTime == 0.0f )
	{
		m_fTime = engine->Time() + randomFloat(4.0f,6.0f);
	}

	if ( m_fTime < engine->Time() )
		complete();

	if ( CClassInterface::getAnimCycle(m_pCharger) == 1.0f )
		complete();

	if ( m_iType == CHARGER_HEALTH && pBot->getHealthPercent() >= 0.99f )
		complete();
	else if ( m_iType == CHARGER_ARMOR && static_cast<CHLDMBot*>(pBot)->getArmorPercent() >= 0.99f )
		complete();

	pBot->setLookVector(vOrigin);
	pBot->setLookAtTask(LOOK_VECTOR);

	if ( pBot->distanceFrom(m_pCharger) > 96 )
	{
		pBot->setMoveTo(vOrigin);
	}
	else if ( pBot->isFacing(vOrigin) )
	{
		pBot->use();
	}
}

void CBotGravGunPickup :: execute(CBot *pBot,CBotSchedule *pSchedule)
{
	static Vector vOrigin;
	static Vector vBotOrigin;

	if ( m_fTime == 0.0f )
	{
		m_fSecAttTime = 0.0f;
		m_fTime = engine->Time() + randomFloat(2.0f,4.0f);
	}

	if ( m_fTime < engine->Time() )
	{
		const auto HL2DMBot = static_cast<CHLDMBot*>(pBot);

		if (HL2DMBot->getFailedObject() && HL2DMBot->distanceFrom(HL2DMBot->getFailedObject())<=pBot->distanceFrom(m_Prop)+48 )
			pBot->primaryAttack();

		HL2DMBot->setFailedObject(m_Prop);

		fail();
		return;
	}

	if ( !CBotGlobals::entityIsValid(m_Prop) || !pBot->isVisible(m_Prop) )
	{
		static_cast<CHLDMBot*>(pBot)->setFailedObject(m_Prop);
		fail();
		return;
	}

	pBot->wantToChangeWeapon(false);

	vBotOrigin = pBot->getOrigin();
	vOrigin = CBotGlobals::entityOrigin(m_Prop);

	const CBotWeapon *pWeapon = pBot->getCurrentWeapon();

	if ( !pWeapon || pWeapon->getID() != HL2DM_WEAPON_PHYSCANNON  )
	{
		if ( !pBot->select_CWeapon(CWeapons::getWeapon(HL2DM_WEAPON_PHYSCANNON)) )
		{
			fail();
			return;
		}
	}
	else if ( pBot->distanceFrom(vOrigin) > 100 )
		pBot->setMoveTo(vOrigin);
	else if ( (vOrigin-vBotOrigin).Length2D() < 16 && vOrigin.z < vBotOrigin.z )
		pBot->setMoveTo(vBotOrigin + (vBotOrigin-vOrigin)*100);
	else
		pBot->stopMoving();

	if ( pWeapon )
		m_Weapon = INDEXENT(pWeapon->getWeaponIndex());
	else
	{
		fail();
		return;
	}

	pBot->setMoveLookPriority(MOVELOOK_OVERRIDE);
	pBot->setLookVector(vOrigin);
	pBot->setLookAtTask(LOOK_VECTOR);
	pBot->setMoveLookPriority(MOVELOOK_TASK);

	if ( pBot->isFacing(vOrigin) )
	{
		const edict_t *pPhys = CClassInterface::gravityGunObject(m_Weapon);

		if ( pPhys == m_Prop.get() )
			complete();
		else if ( pPhys || CClassInterface::gravityGunOpen(m_Weapon) )
		{
			if ( m_fSecAttTime < engine->Time() )
			{
				pBot->secondaryAttack();
				m_fSecAttTime = engine->Time() + randomFloat(0.25,0.75);
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////
/*
// Protect SG from Enemy
CBotTFEngiTankSentry :: CBotTFEngiTankSentry ( edict_t *pSentry, edict_t *pEnemy )
{
	m_pEnemy = pEnemy;
	m_pSentry = pSentry;
}

void CBotTFEngiTankSentry :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	
	CBotFortress *tfBot;

	if ( !pBot->isTF() )
	{
		fail();
		return;
	}
	
	tfBot = (CBotFortress*)pBot;


	if ( !CBotGlobals::entityIsAlive(m_pEnemy) )
		complete();
	else if ( !CBotGlobals::entityIsAlive(m_pSentry) || !CBotGlobals::entityIsValid(m_pSentry) || !CTeamFortress2Mod::isSentry(m_pSentry) )
		fail();
	else
	{
		Vector vOrigin;
		Vector vComp;

		vComp = CBotGlobals::entityOrigin(m_pEnemy) - CBotGlobals::entityOrigin(m_pSentry);
		vComp = vComp / vComp.Length(); // Normalise

		// find task position behind sentry
		vOrigin = CBotGlobals::entityOrigin(m_pSentry) - (vComp*80);

		if ( pBot->distanceFrom(vOrigin) > 32  )
		{
			// get into position!
			pBot->setMoveTo((vOrigin));
		}
		else
		{
			if ( !pBot->currentWeapon("tf_wrench") )
				pBot->selectWeaponName("tf_wrench");
			else
			{
				setTaskEntity();
				pBot->setLookAt(TSK_ENTITY,2);

				// Tank!!!
				pBot->duck();
				pBot->tapButton(IN_ATTACK);
			}

		}
	}
		
}
*/

////////////////////////

////////////////////////


CBotTF2WaitAmmoTask :: CBotTF2WaitAmmoTask (const Vector& vOrigin )
{
	m_vOrigin = vOrigin;
	m_fWaitTime = 0.0f;
}
	
void CBotTF2WaitAmmoTask :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	if ( !m_fWaitTime )
		m_fWaitTime = engine->Time() + randomFloat(5.0f,10.0f);

	if ( !pBot->hasSomeConditions(CONDITION_NEED_AMMO) )
	{
		complete();
	}
	else if ( m_fWaitTime < engine->Time() )
		fail();
	else if ( pBot->distanceFrom(m_vOrigin) > 100 )
	{
		pBot->setMoveTo(m_vOrigin);
	}
	else
	{
		pBot->stopMoving();
	}
}

void CBotTF2WaitAmmoTask :: debugString ( char *string )
{
	std::sprintf(string,"CBotTF2WaitAmmoTask");
}
///////////////////////////
CBotTaskEngiPickupBuilding :: CBotTaskEngiPickupBuilding ( edict_t *pBuilding )
{
	m_pBuilding = pBuilding;
	m_fTime = 0.0f;
}
// move building / move sentry / move disp / move tele
void CBotTaskEngiPickupBuilding :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	const CBotWeapon *pWeapon = pBot->getCurrentWeapon();

	if ( m_fTime == 0.0f )
		m_fTime = engine->Time() + 6.0f;

	if ( !m_pBuilding.get() )
	{
		fail();
		return;
	}

	// don't pick up sentry now because my sentry is in good use!!!
	if ( static_cast<CBotTF2*>(pBot)->sentryRecentlyHadEnemy() )
	{
		fail();
		return;
	}

	pBot->wantToShoot(false);
	pBot->lookAtEdict(m_pBuilding.get());
	pBot->setLookAtTask(LOOK_EDICT);

	static_cast<CBotTF2*>(pBot)->updateCarrying();

	if ( static_cast<CBotTF2*>(pBot)->isCarrying() ) //if ( CBotGlobals::entityOrigin(m_pBuilding) == CBotGlobals::entityOrigin(pBot->getEdict()) )
		complete();
	else if ( m_fTime < engine->Time() )
		fail();
	else if ( !pWeapon )
		fail();
	else if ( pWeapon->getID() != TF2_WEAPON_WRENCH )
	{
		if ( !pBot->select_CWeapon(CWeapons::getWeapon(TF2_WEAPON_WRENCH)) )
			fail();
	}
	else if ( pBot->distanceFrom(m_pBuilding) < 100 )
	{
		if ( CBotGlobals::yawAngleFromEdict(pBot->getEdict(),CBotGlobals::entityOrigin(m_pBuilding)) < 25 )
		{	
			if ( randomInt(0,1) )
				pBot->secondaryAttack();
			else
				pBot->letGoOfButton(IN_ATTACK2);

			static_cast<CBotTF2*>(pBot)->resetCarryTime();
		}
	}
	else
		pBot->setMoveTo(CBotGlobals::entityOrigin(m_pBuilding));
}
void CBotTaskEngiPickupBuilding :: debugString ( char *string )
{
	std::sprintf(string,"CBotTaskEngiPickupBuilding");
}

/////////////////
CBotTaskEngiPlaceBuilding :: CBotTaskEngiPlaceBuilding ( eEngiBuild iObject, const Vector& vOrigin )
{
	m_vOrigin = vOrigin;
	m_fTime = 0.0f;
	m_iState = 1; // BEGIN HERE , otherwise bot will try to destroy the building
	m_iObject = iObject;
	m_iTries = 0;
}

// unused
void CBotTaskEngiPlaceBuilding :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	pBot->wantToInvestigateSound(false);

	if ( m_fTime == 0.0f )
	{
		m_fTime = engine->Time() + 6.0f;
		pBot->resetLookAroundTime();

		if ( CTeamFortress2Mod::buildingNearby(pBot->getTeam(),m_vOrigin) )
			m_vOrigin = m_vOrigin + Vector(randomFloat(-200.0f,200.0f),randomFloat(-200.0f,200.0f),0);
	}

	pBot->setLookVector(m_vOrigin);
	pBot->setLookAtTask(LOOK_VECTOR);

	static_cast<CBotTF2*>(pBot)->updateCarrying();

	if ( !static_cast<CBotTF2*>(pBot)->isCarrying() ) 
		complete();
	else if ( m_fTime < engine->Time() )
		fail();
	else if ( pBot->distanceFrom(m_vOrigin) < 100 )
	{		
		if ( CBotGlobals::yawAngleFromEdict(pBot->getEdict(),m_vOrigin) < 25 )
		{
			const int state = static_cast<CBotTF2*>(pBot)->engiBuildObject(&m_iState,m_iObject,&m_fTime,&m_iTries);

			if ( state == 1 )
				complete();
			else if ( state == 0 )
				fail();
			
			//pBot->primaryAttack();
		}
	}
	else
		pBot->setMoveTo(m_vOrigin);
	
	if ( pBot->hasEnemy() )
	{
		if ( randomInt(0,1) )
			pBot->primaryAttack();
	}
}
void CBotTaskEngiPlaceBuilding :: debugString ( char *string )
{
	std::sprintf(string,"CBotTaskEngiPlaceBuilding");
}
/////////////////////////////
CBotUseLunchBoxDrink::CBotUseLunchBoxDrink()
{
	m_fTime = 0.0f;
}
void CBotUseBuffItem::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	pBot->wantToShoot(false);
	pBot->wantToChangeWeapon(false);

	if (m_fTime == 0.0f)
		m_fTime = engine->Time() + 5.0f;
	else if (m_fTime < engine->Time())
		fail();

	const CBotWeapon* pWeapon = pBot->getCurrentWeapon();

	if (CClassInterface::getRageMeter(pBot->getEdict()) < 100.0f)
	{
		fail();
		return;
	}

	if (!pWeapon || pWeapon->getID() != TF2_WEAPON_BUFF_ITEM)
	{
		if (!pBot->select_CWeapon(CWeapons::getWeapon(TF2_WEAPON_BUFF_ITEM)))
			fail();
	}
	else
	{
		if (randomInt(0, 1))
			pBot->primaryAttack();
	}
}

CBotUseBuffItem :: CBotUseBuffItem ()
{
	m_fTime = 0.0f;
}
void CBotUseLunchBoxDrink::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	pBot->wantToShoot(false);
	pBot->wantToChangeWeapon(false);

	if ( m_fTime == 0.0f )
		m_fTime = engine->Time() + 5.0f;
	else if ( m_fTime < engine->Time() )
		fail();

	const CBotWeapon* pWeapon = pBot->getCurrentWeapon();

	if ( CClassInterface::TF2_getEnergyDrinkMeter(pBot->getEdict()) < 100.0f )
	{
		fail();
		return;
	}

	if ( !pWeapon || pWeapon->getID() != TF2_WEAPON_LUNCHBOX_DRINK )
	{
		if (!pBot->select_CWeapon(CWeapons::getWeapon(TF2_WEAPON_LUNCHBOX_DRINK)))
			fail();
	}
	else
	{
		if( randomInt(0,1) )
			pBot->primaryAttack();
	}
}
/////////////////////////////
CBotBackstab :: CBotBackstab (edict_t *_pEnemy)
{
	m_fTime = 0.0f;
	pEnemy = _pEnemy;
}
	
void CBotBackstab ::execute (CBot *pBot,CBotSchedule *pSchedule)
{
	Vector vangles;
	CBotTF2 *pTF2Bot = static_cast<CBotTF2*>(pBot);

	pBot->wantToChangeWeapon(false);
	pBot->wantToShoot(false);
	pBot->wantToListen(false);

	CBotWeapon* pBotWeapon = pBot->getCurrentWeapon();

	if ( !pBotWeapon )
	{
		fail();
		pTF2Bot->waitBackstab();
		return;
	}

	const CWeapon* pWeapon = pBotWeapon->getWeaponInfo();

	if (pWeapon == nullptr)
	{
		fail();
		pTF2Bot->waitBackstab();
		return;
	}

	if ( !CBotGlobals::isAlivePlayer(pEnemy) )
		fail();

	if ( !m_fTime )
		m_fTime = engine->Time() + randomFloat(5.0f,10.0f);

	pBot->setLookAtTask(LOOK_EDICT);
	pBot->lookAtEdict(pEnemy);

	if ( m_fTime < engine->Time() )
	{
		fail();
		pTF2Bot->waitBackstab();
		return;
	}
	if ( !pEnemy || !CBotGlobals::entityIsValid(pEnemy) || !CBotGlobals::entityIsAlive(pEnemy) )
	{
		if ( pBot->getEnemy() && pEnemy != pBot->getEnemy() && pBot->hasSomeConditions(CONDITION_SEE_CUR_ENEMY) && CBotGlobals::isAlivePlayer(pBot->getEnemy()) )
		{
			pEnemy = pBot->getEnemy();
		}
		else
		{
			fail();
		}

		pTF2Bot->waitBackstab();
		return;
	}
	if ( !pBot->isVisible(pEnemy) )
	{
		// this guy will do
		if ( pBot->getEnemy() && pEnemy != pBot->getEnemy() && pBot->hasSomeConditions(CONDITION_SEE_CUR_ENEMY) && CBotGlobals::isAlivePlayer(pBot->getEnemy()) )
		{
			pEnemy = pBot->getEnemy();
		}
		else
		{
			fail();
		}

		pTF2Bot->waitBackstab();
		return;
	}
	if (pWeapon->getID() != TF2_WEAPON_KNIFE)
	{
		if ( !pBot->select_CWeapon(CWeapons::getWeapon(TF2_WEAPON_KNIFE)) )
		{
			fail();
			pTF2Bot->waitBackstab();
			return;
		}
	}

	AngleVectors(CBotGlobals::entityEyeAngles(pEnemy),&vangles);
	const Vector vrear = CBotGlobals::entityOrigin(pEnemy) - vangles * 45 + Vector(0, 0, 32);

	pTF2Bot->resetAttackingEnemy();

	if ( pBot->distanceFrom(vrear) > 40 ) 
	{
		pBot->setMoveTo(vrear);
	}
	else
	{
		// uncloak
		if ( CTeamFortress2Mod::TF2_IsPlayerCloaked(pBot->getEdict()) )
			pTF2Bot->spyUnCloak();
		else
			pTF2Bot->handleAttack(pBotWeapon,pEnemy);
	}
}

void CBotInvestigateTask :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	if ( m_fTime == 0.0f )
	{
		const CWaypoint *pWaypoint = CWaypoints::getWaypoint(CWaypointLocations::NearestWaypoint(m_vOrigin,m_fRadius,-1));

		if ( pWaypoint == nullptr)
		{
			// can't investigate
			// but other tasks depend on this, so complete it
			complete();
			return;
		}

		if ( pWaypoint->numPaths() > 0 )
		{
			for ( int i = 0; i < pWaypoint->numPaths(); i ++ )
				m_InvPoints.emplace_back(CWaypoints::getWaypoint(pWaypoint->getPath(i))->getOrigin());	

			m_iCurPath = randomInt(0,pWaypoint->numPaths()-1);
		}

		m_fTime = engine->Time() + m_fMaxTime;
	}

	if ( m_fTime < engine->Time() )
		complete();

	if (!m_InvPoints.empty())
	{
		Vector vPoint;

		if ( m_iState == 0 ) // goto inv point
			vPoint = m_InvPoints[m_iCurPath];
		else if ( m_iState == 1 ) // goto origin
			vPoint = m_vOrigin;

		if ( pBot->distanceFrom(vPoint) < 80 || m_iState==0&&pBot->distanceFrom(m_vOrigin)>m_fRadius )
		{
			m_iState = !m_iState ? 1 : 0;

			if ( m_iState == 0 )
				m_iCurPath = randomInt(0,m_InvPoints.size()-1);
		}
		else
			pBot->setMoveTo(vPoint);
	}
	else
		pBot->stopMoving();
	// walk
	pBot->setMoveSpeed(CClassInterface::getMaxSpeed(pBot->getEdict())/8);
	//pBot->setLookVector();

	if ( m_bHasPOV )
	{
		pBot->setLookVector(m_vPOV);
		pBot->setLookAtTask(LOOK_VECTOR);
	}
	else
		pBot->setLookAtTask(LOOK_AROUND);

}
///////////////////

void CBotDefendTask :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	static float fDist;

	if ( m_fTime == 0.0f )
	{
		if ( pBot->inSquad() && pBot->isSquadLeader() )
		{
			m_fTime = engine->Time() + randomFloat(15.0f,45.0f);
		} 
		else if ( m_fMaxTime > 0.0f )
			m_fTime = engine->Time() + m_fMaxTime;
		else
		{
			m_fTime = engine->Time() + randomFloat(20.0f,90.0f);

			if ( pBot->isTF2() )
			{		
				if ( CTeamFortress2Mod::isMapType(TF_MAP_MVM) )
				{
					Vector vFlag;

					if ( CTeamFortress2Mod::getFlagLocation(TF2_TEAM_BLUE,&vFlag) )
					{
						// FOR DEBUGGING
						const float fDist = (vFlag - m_vOrigin).Length();
						const float fWaitTime = 90.0f - fDist/45; // MAX Dist 4050
						//
						m_fTime = engine->Time() + fWaitTime;
					}
				}
			}
		}
	}

	fDist = pBot->distanceFrom(m_vOrigin);

	if ( fDist > 200.0f )
		fail(); // too far -- bug
	else if ( fDist > 100.0f )
		pBot->setMoveTo(m_vOrigin);
	else
	{
		pBot->defending();
		
		pBot->stopMoving();

		if ( m_iWaypointType & CWaypointTypes::W_FL_CROUCH )
			pBot->duck();

		if ( m_bDefendOrigin )
		{
			pBot->setAiming(m_vDefendOrigin);
			pBot->setLookVector(m_vDefendOrigin);
		}

		pBot->setLookAtTask(m_LookTask);

		//pBot->setAiming(m_vDefendOrigin);

		/*if ( m_bDefendOrigin )
		{
			pBot->setLookAtTask(LOOK_AROUND);
			//pBot->setAiming(m_vDefendOrigin);
		}
		else
			pBot->setLookAtTask(LOOK_SNIPE);*/
	}

	if ( m_fTime < engine->Time() )
	{
		if ( pBot->inSquad() && pBot->isSquadLeader() )
			pBot->addVoiceCommand(DOD_VC_GOGOGO);

		complete();
	}
}

//////////////////////
void CBotTF2EngiLookAfter :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	CBotFortress *tfBot = static_cast<CBotFortress*>(pBot);

	if ( !m_fTime )
	{
		m_fTime = engine->Time() + randomFloat(21.0f,60.0f);
		m_fHitSentry = engine->Time() + randomFloat(1.0f,3.0f);
	}
	else if ( m_fTime < engine->Time() )
	{
		tfBot->nextLookAfterSentryTime(engine->Time()+randomFloat(20.0f,50.0f));
		complete();
	}
	else 
	{
		const CBotWeapon *pWeapon = pBot->getCurrentWeapon();
		edict_t *pSentry = m_pSentry.get();

		pBot->wantToListen(false);

		pBot->setLookAtTask(LOOK_AROUND);

		if ( !pWeapon )
		{
			fail();
			return;
		}
		if ( pWeapon->getID() != TF2_WEAPON_WRENCH )
		{
			if ( !pBot->select_CWeapon(CWeapons::getWeapon(TF2_WEAPON_WRENCH)) )
			{
				fail();
				return;
			}
		}

		if ( pSentry != nullptr)
		{
			if ( pBot->distanceFrom(pSentry) > 100 )
				pBot->setMoveTo(CBotGlobals::entityOrigin(pSentry));
			else
			{
				pBot->stopMoving();
					
				if ( CClassInterface::getSentryEnemy(pSentry)!= nullptr || 
					CClassInterface::getSentryHealth(pSentry) < CClassInterface::getTF2GetBuildingMaxHealth(pSentry) || 
					CClassInterface::getTF2SentryRockets(pSentry)<20 || 
					CClassInterface::getTF2SentryShells(pSentry)<150)
				{
					pBot->primaryAttack();
				}

			
				pBot->duck(true); // crouch too

			}

			pBot->lookAtEdict(pSentry);
			pBot->setLookAtTask(LOOK_EDICT); // LOOK_EDICT fix engineers not looking at their sentry

		}
		else
			fail();
	}
}

////////////////////////
CBotTFEngiBuildTask :: CBotTFEngiBuildTask ( eEngiBuild iObject, CWaypoint *pWaypoint )
{
	m_iObject = iObject;
	m_vOrigin = pWaypoint->getOrigin()+pWaypoint->applyRadius();
	m_iState = 0;
	m_fTime = 0.0f;
	m_iTries = 0;
	m_fNextUpdateAngle = 0.0f;
	const QAngle ang = QAngle(0,pWaypoint->getAimYaw(),0);
	Vector vForward;
	AngleVectors(ang,&vForward);
	m_vAimingVector = m_vOrigin + vForward*100.0f;
	m_iArea = pWaypoint->getArea();
	m_vBaseOrigin = m_vOrigin;
	m_fRadius = pWaypoint->getRadius();
}
	
void CBotTFEngiBuildTask :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	pBot->wantToInvestigateSound(false);

	//if ( !pBot->isTF() ) // shouldn't happen ever
	//	fail();

	if ( !CTeamFortress2Mod::m_ObjectiveResource.isWaypointAreaValid(m_iArea) )
		fail();

	pBot->wantToShoot(false); // don't shoot enemies , want to build the damn thing
	pBot->wantToChangeWeapon(false); // if enemy just strike them with wrench
	pBot->wantToListen(false); // sometimes bots dont place sentries because they are looking the wrong way due to a noise

	CBotFortress* tfBot = static_cast<CBotFortress*>(pBot);

	if ( tfBot->getClass() != TF_CLASS_ENGINEER )
	{
		fail();
		return;
	}
	/*else if ( pBot->hasEnemy() && pBot->hasSomeConditions(CONDITION_SEE_CUR_ENEMY) )
	{
		fail();
		return;
	}*/

	if ( m_fTime == 0.0f )
	{
		pBot->resetLookAroundTime();

		if ( CTeamFortress2Mod::buildingNearby(pBot->getTeam(),m_vOrigin) )
		{
			if (m_fRadius > 0.0f)
				m_vOrigin = m_vBaseOrigin + Vector(randomFloat(-m_fRadius,m_fRadius),randomFloat(-m_fRadius,m_fRadius),0);
			else
			{
				// can't build here
				fail();
				return; 
			}
		}

		m_fTime = engine->Time() + randomFloat(4.0f,8.0f);

		m_fNextUpdateAngle = engine->Time() + 0.5f;
	}
	else if ( m_fTime < engine->Time() )
		fail();

	if ( m_iObject == ENGI_DISP )
	{
		edict_t *pSentry = tfBot->getSentry();

		if ( pSentry && CBotGlobals::entityIsValid(pSentry) )
		{
			const Vector vSentry = CBotGlobals::entityOrigin(pSentry);
			const Vector vOrigin = pBot->getOrigin();
			const Vector vLookAt = vOrigin - (vSentry - vOrigin);

			m_vAimingVector = vLookAt;

			//pBot->setLookVector(vLookAt);
			//pBot->setLookAtTask(LOOK_VECTOR);
			
			//LOOK_VECTOR,11);
			//bAimingOk = pBot->DotProductFromOrigin(vLookAt) > 0.965925f; // 15 degrees // < CBotGlobals::yawAngleFromEdict(pBot->getEdict(),pBot->getLookVector()) < 15;
		}
		else
		{
			const Vector vSentry = pBot->getAiming();
			const Vector vOrigin = pBot->getOrigin();
			const Vector vLookAt = vOrigin - (vSentry - vOrigin);

			m_vAimingVector = vLookAt;

			/*pBot->setLookVector(pBot->getAiming());
			pBot->setLookAtTask((LOOK_VECTOR));*/
			//bAimingOk = pBot->DotProductFromOrigin(vLookAt) > 0.965925f;
		}
	}

	if ( m_iTries > 1 && m_fNextUpdateAngle < engine->Time() )
	{
		QAngle angles;
		Vector vforward = m_vAimingVector - pBot->getOrigin();
		vforward = vforward/vforward.Length(); // normalize

		VectorAngles(vforward,angles);

		angles.y += randomFloat(-60.0f,60.0f); // yaw
		CBotGlobals::fixFloatAngle(&angles.y);

		AngleVectors(angles,&vforward);

		vforward = vforward/vforward.Length();

		m_vAimingVector = pBot->getOrigin()+vforward*100.0f;
		
		m_fNextUpdateAngle = engine->Time() + 1.5f;
	}

	pBot->setLookAtTask(LOOK_VECTOR,0.2f);
	pBot->setLookVector(m_vAimingVector);

	const bool bAimingOk = pBot->isFacing(m_vAimingVector); // 15 degrees

	if ( pBot->distanceFrom(m_vOrigin) > 70.0f )
	{
		if ( !CBotGlobals::isVisible(pBot->getEdict(),pBot->getEyePosition(),m_vOrigin) )
			fail();
		else
			pBot->setMoveTo(m_vOrigin);
	}
	else if ( bAimingOk || m_iTries > 1 )
	{
		const int state = tfBot->engiBuildObject(&m_iState,m_iObject,&m_fTime,&m_iTries);

		if ( state == 1 )
			complete();
		else if ( state == 0 )
			fail();
		else if ( state == 3 ) // failed , try again 
		{
			if (m_fRadius > 0.0f)
				m_vOrigin = m_vBaseOrigin + Vector(randomFloat(-m_fRadius,m_fRadius),randomFloat(-m_fRadius,m_fRadius),0);
			else
			{
				// can't build here
				fail();
			}
		}
	}
}

void CBotTFEngiBuildTask :: debugString ( char *string )
{
	std::sprintf(string,"CBotTFEngiBuildTask (%d,%0.4f,%0.4f,%0.4f)",m_iObject,m_vOrigin.x,m_vOrigin.y,m_vOrigin.z);
}


///////////////////////////////////////////////////////

CFindGoodHideSpot :: CFindGoodHideSpot ( edict_t *pEntity )		
{
	m_vHideFrom = CBotGlobals::entityOrigin(pEntity);
}

CFindGoodHideSpot :: CFindGoodHideSpot (const Vector& vec )
{
	m_vHideFrom = vec;
}

void CFindGoodHideSpot :: init ()
{
	// not required, should have been constructed properly
	//m_vHideFrom = Vector(0,0,0);
}

void CFindGoodHideSpot :: execute ( CBot *pBot, CBotSchedule *pSchedule )
{
	Vector vFound;

	if ( !pBot->getNavigator()->getHideSpotPosition(m_vHideFrom,&vFound) )
		fail();
	else
	{		
		pSchedule->passVector( vFound );
		complete();
	}
}

CFindPathTask :: CFindPathTask ( int iWaypointId, eLookTask looktask )
{
	m_iInt = 0;
	m_iWaypointId = iWaypointId;
	m_LookTask = looktask;
	m_vVector = CWaypoints::getWaypoint(iWaypointId)->getOrigin();
	m_flags.m_data = 0;
	m_fRange = 0.0f;
	m_iDangerPoint = -1;
	m_bGetPassedIntAsWaypointId = false;
}

void CFindPathTask :: init ()
{
	m_flags.m_data = 0;
	m_iInt = 0;
	m_fRange = 0.0f;
	m_iDangerPoint = -1;
	m_bGetPassedIntAsWaypointId = false;
	//setFailInterrupt(CONDITION_SEE_CUR_ENEMY);
}

CFindPathTask :: CFindPathTask ( edict_t *pEdict )
{
	m_iWaypointId = -1;
	m_pEdict = pEdict;
	m_vVector = CBotGlobals::entityOrigin(pEdict);
	m_LookTask = LOOK_WAYPOINT;
	m_flags.m_data = 0;
	m_fRange = 0.0f;
	m_iInt = 0;
	m_iDangerPoint = -1;
	m_bGetPassedIntAsWaypointId = false;
}

void CFindPathTask :: debugString ( char *string )
{
	std::sprintf(string,"CFindPathTask\n m_iInt = %d\n m_vVector = (%0.4f,%0.4f,%0.4f)",m_iInt,m_vVector.x,m_vVector.y,m_vVector.z);
}

void CFindPathTask :: execute ( CBot *pBot, CBotSchedule *pSchedule )
{	
	bool bFail = false;

	if ( m_LookTask == LOOK_NOISE )
		pBot->wantToListen(false); // vector already set before find path task

	if ( m_bGetPassedIntAsWaypointId )
	{
		m_iWaypointId = pSchedule->passedInt();

		if ( m_iWaypointId == -1 )
		{
			fail();
			return;
		}

		m_vVector = CWaypoints::getWaypoint(m_iWaypointId)->getOrigin();
	}
	else if ( pSchedule->hasPassVector() )
	{
		m_vVector = pSchedule->passedVector();
		pSchedule->clearPass();
	}

	if ( m_iInt == 0 || m_iInt == 1 )
	{
		IBotNavigator *pNav = pBot->getNavigator();

		pBot->m_fWaypointStuckTime = 0;

#ifdef _DEBUG
		CProfileTimer *timer = CProfileTimers::getTimer(BOT_ROUTE_TIMER);

		if ( CClients::clientsDebugging(BOT_DEBUG_PROFILE) )
		{
			timer->Start();
		}
#endif

		if ( pNav->workRoute( pBot->getOrigin(),
							   m_vVector,
							   &bFail,
							   m_iInt==0,
							   m_flags.bits.m_bNoInterruptions, 
							   m_iWaypointId,
							   pBot->getConditions(), m_iDangerPoint ) )
		{
			pBot->m_fWaypointStuckTime = engine->Time() + randomFloat(10.0f,15.0f);
			pBot->moveFailed(); // reset
			m_iInt = 2;
		}
		else
			m_iInt = 1;

#ifdef _DEBUG
		if ( CClients::clientsDebugging(BOT_DEBUG_PROFILE) )
		{
			timer->Stop();
		}
#endif

		pBot->debugMsg(BOT_DEBUG_NAV,"Trying to work out route");
	}

	if ( bFail )
	{
		pBot->debugMsg(BOT_DEBUG_NAV,"Route failed");
		fail();
	}
	else if ( m_iInt == 2 )
	{
		if ( pBot->m_fWaypointStuckTime == 0.0f )
			pBot->m_fWaypointStuckTime = engine->Time() + randomFloat(5.0f,10.0f);

		//if ( m_bNoInterruptions )
		//{
			//pBot->debugMsg(BOT_DEBUG_NAV,"Found route");
			//complete(); // ~fin~
		//}

		if ( !pBot->getNavigator()->hasNextPoint() )
		{
			pBot->debugMsg(BOT_DEBUG_NAV,"Nowhere to go");
			complete(); // reached goal
		}
		else
		{			
			if ( pBot->moveFailed() )
			{
				pBot->debugMsg(BOT_DEBUG_NAV,"moveFailed() == true");
				fail();
				pBot->getNavigator()->failMove();
			}

			if (m_pEdict)
			{
				if (CBotGlobals::entityIsValid(m_pEdict))
				{
					pBot->lookAtEdict(m_pEdict);

					if (m_flags.bits.m_bCompleteInRangeOfEdict && m_flags.bits.m_bCompleteSeeTaskEdict)
					{
						// complete if inrange AND see edict
						if (pBot->distanceFrom(m_pEdict) < m_fRange && pBot->isVisible(m_pEdict))
							complete();
					}
					else if (!m_flags.bits.m_bDontGoToEdict && pBot->isVisible(m_pEdict))
					{
						if (pBot->distanceFrom(m_pEdict) < pBot->distanceFrom(pBot->getNavigator()->getNextPoint()))
							complete();
					}
					else if (m_flags.bits.m_bCompleteOutOfRangeEdict && pBot->distanceFrom(m_pEdict) > m_fRange)
						complete();
					else if (m_flags.bits.m_bCompleteInRangeOfEdict && pBot->distanceFrom(m_pEdict) < m_fRange)
						complete();
				}
				else
					fail();
			}

			//// running path
			//if ( !pBot->hasEnemy() && !pBot->hasSomeConditions(CONDITION_SEE_CUR_ENEMY) )
			
			pBot->setLookAtTask(m_LookTask);
		}
	}

	if ( m_pEdict.get() != nullptr) // task edict
	{
		if ( m_flags.bits.m_bCompleteSeeTaskEdict )
		{
			if ( pBot->isVisible(m_pEdict.get()) && pBot->distanceFrom(CBotGlobals::entityOrigin(m_pEdict)) < CWaypointLocations::REACHABLE_RANGE )
				complete();
		}

		if ( m_flags.bits.m_bFailTaskEdictDied )
		{
			if ( m_pEdict == NULL || !CBotGlobals::entityIsAlive(m_pEdict) )
			{
				fail();
			}
		}
	}
	else if ( m_flags.bits.m_bFailTaskEdictDied )
		fail();
}

CBotTF2FindPipeWaypoint:: CBotTF2FindPipeWaypoint (const Vector& vOrigin, const Vector& vTarget )
{
	m_vOrigin = vOrigin;
	m_vTarget = vTarget;

	m_i = 0;
	m_j = 0;
	m_iters = 0;
	m_iNearesti = -1;
	m_iNearestj = -1;
	m_fNearesti = 2048.0f;
	m_fNearestj = 4096.0f;
	m_iTargetWaypoint = static_cast<short>(CWaypointLocations::NearestWaypoint(m_vTarget,BLAST_RADIUS, -1, true, true));
		
	m_pTable = CWaypoints::getVisiblity();	

	if ( m_iTargetWaypoint != -1 )
	{
		// first find the waypoint nearest the target
		Vector vComp = vOrigin - vTarget;
		vComp = vComp/vComp.Length();
		vComp = vTarget + vComp*256; // get into a better area
		m_pTarget = CWaypoints::getWaypoint(m_iTargetWaypoint);
		CWaypointLocations::GetAllInArea(vComp,&m_WaypointsI,m_iTargetWaypoint);
	}

	
}
// a concurrentish pipe waypoint search
void CBotTF2FindPipeWaypoint :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	int maxiters = 128;

	// push!
	if( CTeamFortress2Mod::TF2_IsPlayerInvuln(pBot->getEdict()) )
		fail();
	else if ( CTeamFortress2Mod::TF2_IsPlayerKrits(pBot->getEdict()) )
		maxiters = 256; // speed up search


	pBot->setLookAtTask(LOOK_AROUND);
	pBot->stopMoving();

	if ( m_pTarget == nullptr)
	{
		fail();
		return;
	}

	m_iters = 0;

	// loop through every visible waypoint to target (can be unreachable)
	while (m_i < m_WaypointsI.size()&&m_iters<maxiters)
	{	

		CWaypoint* pTempi = CWaypoints::getWaypoint(m_WaypointsI[m_i]);
		const float fidist = m_pTarget->distanceFrom(pTempi->getOrigin());
			
		if ( fidist > m_fNearesti )
		{
			m_i++;
			m_iters++;
			continue;
		}

		Vector vTempi = pTempi->getOrigin();

		CWaypointLocations::GetAllInArea(vTempi,&m_WaypointsJ,m_WaypointsI[m_i]);

		// loop through every visible waypoint to intermediatery waypoint (cannot be unreachable)
			while (m_j < m_WaypointsJ.size()&&m_iters<maxiters)
			{
				if ( m_WaypointsJ[m_j] == m_iTargetWaypoint )
				{
					m_j++;
					continue;
				}

				CWaypoint* pTempj = CWaypoints::getWaypoint(m_WaypointsJ[m_j]);

				// must be reachable as I want to go here
				if ( !pBot->canGotoWaypoint(pTempj->getOrigin(),pTempj) )
				{
					m_j++;
					continue;
				}

				// only remember the nearest waypoints - the nearest is updated when both I and J are found
				const float fjdist = pTempj->distanceFrom(m_vOrigin) + pTempj->distanceFrom(pTempi->getOrigin());

				if ( fjdist > m_fNearestj )
				{
					m_j++;
					m_iters++;
					continue;
				}

				// If this waypoint is NOT visible to target it is good
				if ( !m_pTable->GetVisibilityFromTo(m_iTargetWaypoint,m_WaypointsJ[m_j]) )
				{			
					m_fNearesti = fidist;
					m_fNearestj = fjdist;
					m_iNearesti = static_cast<short>(m_WaypointsI[m_i]);
					m_iNearestj = static_cast<short>(m_WaypointsJ[m_j]);
				}

				m_iters++;
				m_j++;

			}
			

			if ( m_j == m_WaypointsJ.size() )
				m_i++;

	}

	if ( m_i == m_WaypointsI.size() )
	{		
		if ( m_iNearesti == -1 )
			fail();
		else
		{
			pSchedule->passInt(m_iNearestj);
			pSchedule->passVector(CWaypoints::getWaypoint(m_iNearesti)->getOrigin());

			complete();
		}
	}
}
//////////////////////////////////////

void CTF2_TauntTask :: init ()
{
	m_fTime = 0;
}

void CTF2_TauntTask :: execute ( CBot *pBot, CBotSchedule *pSchedule )
{
	if ( m_fTime == 0.0f )
		m_fTime = engine->Time() + randomFloat(2.5f,5.0f);
	else if ( m_fTime < engine->Time() )
	{
		fail();
		return;
	}

	pBot->setLookVector(m_vPlayer);
	pBot->setLookAtTask(LOOK_VECTOR);

	if ( pBot->distanceFrom(m_vOrigin) > m_fDist )
		pBot->setMoveTo(m_vOrigin);
	else
	{
		if ( pBot->DotProductFromOrigin(m_vPlayer) > 0.95f )
		{
			static_cast<CBotTF2*>(pBot)->taunt(true);
			complete();
		}
	}
}

void CTF2_TauntTask :: debugString ( char *string )
{
	std::sprintf(string,"CTF2_TauntTask");
}

//////////////////////////////////////
void CMoveToTask :: init () 
{ 
	fPrevDist = 0.0f;
	//m_vVector = Vector(0,0,0);
	//m_pEdict = NULL;
}

void CMoveToTask :: debugString ( char *string )
{
	std::sprintf(string,"CMoveToTask\nm_vVector =(%0.4f,%0.4f,%0.4f)",m_vVector.x,m_vVector.y,m_vVector.z);	
}

CMoveToTask :: CMoveToTask ( edict_t *pEdict )
{
	fPrevDist = 0.0f;
	m_pEdict = pEdict;
	m_vVector = CBotGlobals::entityOrigin(m_pEdict);

	//setFailInterrupt(CONDITION_SEE_CUR_ENEMY);
}

void CMoveToTask :: execute ( CBot *pBot, CBotSchedule *pSchedule )
{
	const float fDistance = pBot->distanceFrom(m_vVector);

	// sort out looping move to origins by using previous distance check
	if ( fDistance < 64 || fPrevDist&&fPrevDist < fDistance )
	{
		complete();
		return;
	}
	pBot->setMoveTo(m_vVector);

	if ( pBot->moveFailed() )
		fail();

	fPrevDist = fDistance;
}
////////////////////////////////////////////////////



///////////////////////////////////////////////////

CBotTFRocketJump :: CBotTFRocketJump ()
{
	m_fTime = 0.0f;
	m_fJumpTime = 0.0f;
	m_iState = 0;
}

void CBotTFRocketJump :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	pBot->wantToListen(false);

	const CBotWeapon* pBotWeapon = pBot->getCurrentWeapon();

	if ( !pBotWeapon )
	{
		fail();
		return;
	}

	const CWeapon* pWeapon = pBotWeapon->getWeaponInfo();

	if (pWeapon == nullptr)
	{
		fail();
		return;
	}

	if ( !pBot->isTF() || static_cast<CBotFortress*>(pBot)->getClass() != TF_CLASS_SOLDIER || pBot->getHealthPercent() < 0.3f )
	{
		fail();
	}
	else if (pWeapon->getID() != TF2_WEAPON_ROCKETLAUNCHER )
	{
		if ( !pBot->select_CWeapon(CWeapons::getWeapon(TF2_WEAPON_ROCKETLAUNCHER)) )
		{
			fail();
		}
	}
	else
	{
		CBotTF2 *tf2Bot = static_cast<CBotTF2*>(pBot);

		if ( !m_fTime )
		{
			m_fTime = engine->Time()+randomFloat(4.0f,5.0f);
		}

		if ( tf2Bot->rocketJump(&m_iState,&m_fJumpTime) == BOT_FUNC_COMPLETE )
			complete();
		else if ( m_fTime < engine->Time() )
		{
			fail();
		}
	}
}

void CBotTFRocketJump :: debugString ( char *string )
{
	std::sprintf(string,"CBotTFRocketJump");
}


//////////////////////////////////////////////////////

CBotTFDoubleJump :: CBotTFDoubleJump ()
{
	m_fTime = 0.0f;
}

void CBotTFDoubleJump ::execute (CBot *pBot,CBotSchedule *pSchedule)
{
	pBot->wantToListen(false);

	if ( m_fTime == 0.0f )
	{
		pBot->tapButton(IN_JUMP);

		m_fTime = engine->Time() + 0.4f;
	}
	else if ( m_fTime < engine->Time() )
	{
		pBot->jump();
		complete();
	}
}

void CBotTFDoubleJump :: debugString ( char *string )
{
	std::sprintf(string,"CbotTFDoublejump");
}
///////////////////////////////////////////////
void CSpyCheckAir :: execute ( CBot *pBot, CBotSchedule *pSchedule )
{
	CBotTF2 *pBotTF2 = static_cast<CBotTF2*>(pBot);
	CBotWeapon *pChooseWeapon;
	CBotWeapons *pWeaponList;
	static int iAttackProb;

	if ( CTeamFortress2Mod::TF2_IsPlayerInvuln(pBot->getEdict()) )
		complete(); // don't waste uber spy checking

	if ( m_fTime == 0.0f )
	{
		/*		edict_t *pDisguised;

		int iClass;
		int iTeam;
		int iIndex;
		int iHealth;
*/
		seenlist = 0;		
		m_bHitPlayer = false;

		for ( int i = 1; i <= gpGlobals->maxClients; i ++ )
		{
			edict_t* pPlayer = INDEXENT(i);

			if ( pPlayer == pBot->getEdict() )
				continue;

			if ( !CBotGlobals::entityIsValid(pPlayer) )
				continue;

			IPlayerInfo* p = playerinfomanager->GetPlayerInfo(pPlayer);

			if ( p->IsDead() || p->IsObserver() )
				continue;

			if ( CClassInterface::getTF2Class(pPlayer) == TF_CLASS_SPY )
			{

				if ( CTeamFortress2Mod::TF2_IsPlayerCloaked(pPlayer) )
					continue; // can't see
			}

			if ( pBot->isVisible(pPlayer) )
				seenlist |= (1 << (i - 1));
		}
		
		m_fTime = engine->Time() + randomFloat(2.0f,5.0f);
		m_fNextCheckUnseen = engine->Time() + 0.1f;
	}

	if ( m_fTime < engine->Time() )
		complete();

	if ( pBot->hasEnemy() )
		complete();

	if ( m_pUnseenBefore== nullptr && m_fNextCheckUnseen < engine->Time() )
	{
		seenlist = 0;

		for ( int i = 1; i <= gpGlobals->maxClients; i ++ )
		{
			edict_t* pPlayer = INDEXENT(i);

			if ( pPlayer == pBot->getEdict() )
				continue;

			if ( !CBotGlobals::entityIsValid(pPlayer) )
				continue;

			IPlayerInfo* p = playerinfomanager->GetPlayerInfo(pPlayer);

			if ( p->IsDead() || p->IsObserver() )
				continue;

			if ( CClassInterface::getTF2Class(pPlayer) == TF_CLASS_SPY )
			{
					//CClassInterface::getTF2SpyDisguised(pPlayer,&iClass,&iTeam,&iIndex,&iHealth);

					//if ( (iIndex > 0) && (iIndex <= gpGlobals->maxClients) )

				if ( CTeamFortress2Mod::TF2_IsPlayerCloaked(pPlayer) )
					continue; // can't see but may still be in visible list
			}

			if ( pBot->isVisible(pPlayer) )
			{
				if ( CTeamFortress2Mod::isFlagCarrier(pPlayer) )
					continue; // spies can't hold flag unless not disguised

				if (!(seenlist & (1 << (i - 1))))
				{
					m_pUnseenBefore = pPlayer;
					seenlist |= (1 << (i - 1)); //add to list
					break;
				}
			}
			
		}

		if ( m_pUnseenBefore != nullptr)
		{
			// add more time
			m_fTime = engine->Time() + randomFloat(2.0f,5.0f);
		}

		m_fNextCheckUnseen = engine->Time() + 0.1f;
	}
	else if ( m_pUnseenBefore && (!CBotGlobals::entityIsAlive(m_pUnseenBefore) || !CBotGlobals::entityIsValid(m_pUnseenBefore) || !pBot->isVisible(m_pUnseenBefore) ) ) 
	{
		m_pUnseenBefore = nullptr;
		m_fNextCheckUnseen = 0.0f;
	}

	if ( m_pUnseenBefore == nullptr)
	{
		// automatically look at danger points
		pBot->updateDanger(50.0f);

		pBot->setLookAtTask(LOOK_WAYPOINT);
	}
	else
	{
		// smack him
		pBot->lookAtEdict(m_pUnseenBefore);
		pBot->setLookAtTask(LOOK_EDICT);
		pBot->setMoveTo(CBotGlobals::entityOrigin(m_pUnseenBefore));
	}
/*
	TF_CLASS_UNDEFINED = 0,
	TF_CLASS_SCOUT,
	TF_CLASS_SNIPER,
	TF_CLASS_SOLDIER,
	TF_CLASS_DEMOMAN,
	TF_CLASS_MEDIC,
	TF_CLASS_HWGUY,
	TF_CLASS_PYRO,
	TF_CLASS_SPY,
	TF_CLASS_ENGINEER,
	TF_CLASS_MAX*/

	const CBotWeapon* pWeapon = pBot->getCurrentWeapon();

	switch ( pBotTF2->getClass() )
	{
	case TF_CLASS_PYRO:

		pWeaponList = pBot->getWeapons();

		pChooseWeapon = pWeaponList->getWeapon(CWeapons::getWeapon(TF2_WEAPON_FLAMETHROWER));
		

		if (pChooseWeapon && pChooseWeapon->hasWeapon() && !pChooseWeapon->outOfAmmo(pBot))
		{
			// use flamethrower
			iAttackProb = 90;
			break;
		}
		// move down to melee if out of ammo
	default:
		iAttackProb = 75;
		pChooseWeapon = pBot->getBestWeapon(nullptr,true,true,true);
		break;
	}

	if ( m_pUnseenBefore )
	{
		pBot->setMoveTo(CBotGlobals::entityOrigin(m_pUnseenBefore));
	}

	if ( pChooseWeapon && pWeapon != pChooseWeapon )
	{
		if ( !pBot->selectBotWeapon(pChooseWeapon) )
		{
			fail();
		}
	}
	else
	{
		if ( randomInt(0,100) < iAttackProb )
		{
			pBot->primaryAttack();

			if ( m_pUnseenBefore != nullptr)
			{
				if ( !m_bHitPlayer && pBot->distanceFrom(CBotGlobals::entityOrigin(m_pUnseenBefore)) < 80.0f )
				{
					m_bHitPlayer = true;
					m_fTime = engine->Time()+randomFloat(0.5f,1.5f);
				}
			}
			
		}

		if ( m_pUnseenBefore && pBot->isVisible(m_pUnseenBefore) )
		{
			IPlayerInfo *p = playerinfomanager->GetPlayerInfo(m_pUnseenBefore);

			// stop if I see the player im hitting attacking
			if ( p->GetLastUserCommand().buttons & IN_ATTACK )
			{
				complete();
			}
		}
	}
	
}

void CSpyCheckAir :: debugString (char *string)
{
	std::sprintf(string,"CSpyCheckAir: checking for spies");
}

/////////////////////////////////////////////
CBotRemoveSapper :: CBotRemoveSapper ( edict_t *pBuilding, eEngiBuild id )
{
	m_fTime = 0.0f;
	m_pBuilding = MyEHandle(pBuilding);
	m_id = id;
	m_fHealTime = 0.0f;
}
	
void CBotRemoveSapper :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	CBotTF2 *pTF2Bot = static_cast<CBotTF2*>(pBot);

	pBot->wantToShoot(false);
	pBot->wantToChangeWeapon(false);

	if ( m_fTime == 0.0f )
	{
		m_fTime = engine->Time() + randomFloat(8.0f,12.0f);
		pBot->removeCondition(CONDITION_COVERT);
	}

	edict_t* pBuilding = m_pBuilding.get();

	if ( !pBuilding )
	{
		fail();
		return;
	}

	if ( m_id == ENGI_DISP )
	{
		if ( !CTeamFortress2Mod::isDispenserSapped(pBuilding) )
		{
			if ( m_fHealTime == 0.0f )
				m_fHealTime = engine->Time() + randomFloat(2.0f,3.0f);
			else if ( m_fHealTime < engine->Time() )
			{
				complete();
				return;
			}
		}
		else if ( m_fHealTime > 0.0f )
		{
			fail();
			pTF2Bot->waitRemoveSap();
			return;
		}
	}
	else if ( m_id == ENGI_TELE )
	{
		if ( !CTeamFortress2Mod::isTeleporterSapped(pBuilding) )
		{
			if ( m_fHealTime == 0.0f )
				m_fHealTime = engine->Time() + randomFloat(2.0f,3.0f);
			else if ( m_fHealTime < engine->Time() )
			{
				complete();
				return;
			}
		}
		else if ( m_fHealTime > 0.0f )
		{
			fail();
			pTF2Bot->waitRemoveSap();
			return;
		}
	}
	else if ( m_id == ENGI_SENTRY )
	{
		if ( !CTeamFortress2Mod::isSentrySapped(pBuilding) )
		{
			if ( m_fHealTime == 0.0f )
				m_fHealTime = engine->Time() + randomFloat(2.0f,3.0f);
			else if ( m_fHealTime < engine->Time() )
			{
				complete();
				return;
			}
		}
		else if ( m_fHealTime > 0.0f )
		{
			fail();
			pTF2Bot->waitRemoveSap();
			return;
		}
	}
	
	if ( m_fTime<engine->Time() )
	{
		fail();
	}// Fix 16/07/09
	else if ( !pBot->isVisible(pBuilding) )
	{
		if ( pBot->distanceFrom(pBuilding) > 200 )
			fail();
		else if ( pBot->distanceFrom(pBuilding) > 100 )
			pBot->setMoveTo(CBotGlobals::entityOrigin(pBuilding));
		
		pBot->setLookAtTask(LOOK_EDICT,0.15f);
		pBot->lookAtEdict(pBuilding);
	}
	else
	{
		if ( !static_cast<CBotFortress*>(pBot)->upgradeBuilding(pBuilding,true) )
			fail();
	}
}

////////////////////////////////////////////////////

CBotTF2SnipeCrossBow::CBotTF2SnipeCrossBow(const Vector& vOrigin, int iWpt)
{
	const CWaypoint *pWaypoint = CWaypoints::getWaypoint(iWpt);
	m_iHideWaypoint = 0;
	m_fHideTime = 0.0f;
	m_iPrevClip = 0;
	m_fCheckTime = 0.0f;
	m_fOriginDistance = 0.0f;
	
	m_iSnipeWaypoint = iWpt;
	m_fAimTime = 0.0f;
	m_fTime = 0.0f;
	const QAngle angle = QAngle(0, pWaypoint->getAimYaw(), 0);
	AngleVectors(angle, &m_vAim);
	
	m_vAim = vOrigin + m_vAim * 4096;
	m_vOrigin = vOrigin;
	m_fEnemyTime = 0.0f;
	m_vEnemy = m_vAim;
	m_iArea = pWaypoint->getArea();
}

void CBotTF2SnipeCrossBow::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	CBotTF2* pBotTF2 = static_cast<CBotTF2*>(pBot);
	// Sniper should move if the point has changed, so he's not wasting time
	if (!CTeamFortress2Mod::m_ObjectiveResource.isWaypointAreaValid(m_iArea))
		fail(); // move up
	else if (m_iArea > 0)
	{
		if (CTeamFortress2Mod::isAttackDefendMap())
		{
			if (pBotTF2->getTeam() == TF2_TEAM_BLUE)
			{
				if (m_iArea != pBotTF2->getCurrentAttackArea())
					complete();
			}
			else if (m_iArea != pBotTF2->getCurrentDefendArea())
				complete();
		}
		else if (m_iArea != pBotTF2->getCurrentAttackArea() && m_iArea != pBotTF2->getCurrentDefendArea())
		{
			complete(); // move up
		}

	}

	// disable normal attack functions
	pBot->wantToShoot(false);
	pBotTF2->resetAttackingEnemy();
	// disable listening functions
	pBot->wantToListen(false);
	pBot->wantToInvestigateSound(false);

	// change weapon to sniper if not already
	CBotWeapon* pBotWeapon = pBot->getCurrentWeapon();

	if (!pBotWeapon)
	{
		fail();
		return;
	}

	const CWeapon* pWeapon = pBotWeapon->getWeaponInfo();

	if (pWeapon == nullptr)
	{
		fail();
		return;
	}

	if (pWeapon->getID() != TF2_WEAPON_BOW)
	{
		if (!pBot->select_CWeapon(CWeapons::getWeapon(TF2_WEAPON_BOW)))
		{
			fail();
			return;
		}
	}

	// initialize
	if (m_fTime == 0.0f)
	{
		// distance from sniper point default
		m_fOriginDistance = 100.0f;
		m_fHideTime = 0.0f;
		m_fEnemyTime = 0.0f;
		m_fAimTime = 0.0f;
		m_fTime = engine->Time() + randomFloat(40.0f, 90.0f);
		//pBot->secondaryAttack();
		pBot->resetLookAroundTime();
		m_iPrevClip = pBotWeapon->getAmmo(pBot);

		CBotGlobals::quickTraceline(pBot->getEdict(), m_vOrigin, m_vAim);

		const int iAimWpt = CWaypointLocations::NearestWaypoint(CBotGlobals::getTraceResult()->endpos, 400.0f, -1, true, true);

		if (iAimWpt != -1)
		{
			const CWaypoint *pWaypoint = CWaypoints::getWaypoint(m_iSnipeWaypoint);
			const CWaypointVisibilityTable *pTable = CWaypoints::getVisiblity();

			for (int i = 0; i < pWaypoint->numPaths(); i++)
			{
				const int iPathId = pWaypoint->getPath(i);

				// isn't visible to the target
				if (!pTable->GetVisibilityFromTo(iAimWpt, iPathId))
				{
					m_iHideWaypoint = iPathId;
					m_vHideOrigin = CWaypoints::getWaypoint(iPathId)->getOrigin();
					break;
				}
			}

			if (m_iHideWaypoint == -1)
			{
				// can't find a useful hide waypoint -- choose a random one
				m_iHideWaypoint = pWaypoint->getPath(randomInt(0, pWaypoint->numPaths()));

				if (m_iHideWaypoint != -1)
				{
					CWaypoint *pHideWaypoint = CWaypoints::getWaypoint(m_iHideWaypoint);

					if (pHideWaypoint != nullptr)
					{
						m_vHideOrigin = pHideWaypoint->getOrigin();
					}
					else
						m_iHideWaypoint = -1;
				}
			}
		}

		// check wall time
		m_fCheckTime = engine->Time() + 0.15f;
	}
	else if (m_fTime<engine->Time())
	{
		//if (CTeamFortress2Mod::TF2_IsPlayerZoomed(pBot->getEdict()))
		//	pBot->secondaryAttack();

		complete();
		return;
	}

	// look at waypoint yaw and have random updates
	pBot->setLookAtTask(LOOK_SNIPE);

	// saw an enemy less than 5 secs ago
	if (m_fEnemyTime + 5.0f > engine->Time())
		pBot->setAiming(m_vEnemy);
	else
		pBot->setAiming(m_vAim);

	if (!pBot->isTF() || static_cast<CBotFortress*>(pBot)->getClass() != TF_CLASS_SNIPER || pBot->getHealthPercent() < 0.2f)
	{
		// out of health -- finish
		//if (CTeamFortress2Mod::TF2_IsPlayerZoomed(pBot->getEdict()))
		//	pBot->secondaryAttack();

		fail();
		return;
	}
	if (pBotWeapon->getAmmo(pBot) < 1)
	{
		// out of ammo -- finish
		//if (CTeamFortress2Mod::TF2_IsPlayerZoomed(pBot->getEdict()))
		//	pBot->secondaryAttack();

		complete();
	}
	else if (pBot->distanceFrom(m_vOrigin) > 400)
	{
		//if (CTeamFortress2Mod::TF2_IsPlayerZoomed(pBot->getEdict()))
		//	pBot->secondaryAttack();
		// too far away
		fail();
	}
	else if (m_iHideWaypoint != -1 && (static_cast<CBotFortress*>(pBot)->incomingRocket(512.0f) || m_fHideTime > engine->Time()))
	{
		// hide time -- move to hide origin
		pBot->setMoveTo(m_vHideOrigin);

		if (pBot->distanceFrom(m_vHideOrigin) < 100)
		{
			pBot->stopMoving();

			//if (pBotWeapon->needToReload(pBot))
			//	pBot->reload();
		}
	}
	else if (pBot->distanceFrom(m_vOrigin) > m_fOriginDistance)
	{
		// is too far from origin -- use normal attack code at this time otherwise bot will be a sitting duck
		pBot->setMoveTo(m_vOrigin);
		pBot->wantToShoot(true);
	}
	else
	{
		pBot->stopMoving();

		if (pBot->isHoldingPrimaryAttack() && pBot->hasEnemy())
		{
			if (pBot->hasSomeConditions(CONDITION_SEE_CUR_ENEMY))
			{
				m_vEnemy = CBotGlobals::entityOrigin(pBot->getEnemy());
				m_fEnemyTime = engine->Time();

				if (m_fAimTime == 0.0f)
					m_fAimTime = engine->Time() + randomFloat(0.1f, 0.3f);

				// too close for sniper rifle
				if (pBot->distanceFrom(pBot->getEnemy()) < 200.0f)
				{
					complete();
					return;
				}
			}

			// careful that the round may have not started yet
			if (CTeamFortress2Mod::hasRoundStarted())
			{
				pBot->setMoveLookPriority(MOVELOOK_ATTACK);

				pBot->setLookAtTask(LOOK_ENEMY);

				const float angle = pBot->DotProductFromOrigin(CBotGlobals::entityOrigin(pBot->getEnemy()));

				if (angle > 0.96f) // 16 degrees
				{
					if (m_fAimTime < engine->Time())
						pBot->handleAttack(pBotWeapon, pBot->getEnemy());

					if (m_iPrevClip > pBotWeapon->getAmmo(pBot))
					{
						bool bhide;

						// hide if player can see me, otherwise if not player hide if low health
						if (CBotGlobals::isPlayer(pBot->getEnemy()))
							bhide = CBotGlobals::DotProductFromOrigin(pBot->getEnemy(), pBot->getOrigin()) > 0.96f;
						else
							bhide = randomFloat(0.0f, 2.0f) <= angle + (1.0f - pBot->getHealthPercent());

						if (bhide)
							m_fHideTime = engine->Time() + randomFloat(1.5f, 2.5f);

						m_fAimTime = 0.0f;
						m_iPrevClip = pBotWeapon->getAmmo(pBot);
					}
				}

				pBot->setMoveLookPriority(MOVELOOK_TASK);
			}


		}
		else
		{
			// time to check if bot is just looking at a wall or not
			if (m_fCheckTime < engine->Time())
			{
				CBotGlobals::quickTraceline(pBot->getEdict(), pBot->getOrigin(), pBot->getAiming());

				if (CBotGlobals::getTraceResult()->fraction < 0.1f)
				{
					m_fOriginDistance = 40.0f;
					m_fCheckTime = engine->Time() + 0.3f;
				}
				else
				{
					m_fOriginDistance = 100.0f;
					m_fCheckTime = engine->Time() + 0.15f;
				}
			}

			if (pBot->isHoldingPrimaryAttack())
			{
				// zoom in
				//if (!CTeamFortress2Mod::TF2_IsPlayerZoomed(pBot->getEdict()))
				//	pBot->secondaryAttack();
				//else
				m_fHideTime = 0.0f;
			}
		}
	}
}
///////////////////////////////////////////
CBotTF2Snipe :: CBotTF2Snipe (const Vector& vOrigin, int iWpt )
{
	m_iHideWaypoint = 0;
	m_fHideTime = 0.0f;
	m_iPrevClip = 0;
	m_fCheckTime = 0.0f;
	m_fOriginDistance = 0.0f;

	const CWaypoint *pWaypoint = CWaypoints::getWaypoint(iWpt);
	m_iSnipeWaypoint = iWpt;
	m_fAimTime = 0.0f;
	m_fTime = 0.0f;
	const QAngle angle = QAngle(0, pWaypoint->getAimYaw(), 0);
	
	AngleVectors(angle,&m_vAim);
	m_vAim = vOrigin + m_vAim*4096;
	m_vOrigin = vOrigin;
	m_fEnemyTime = 0.0f;
	m_vEnemy = m_vAim;
	m_iArea = pWaypoint->getArea();
}
	
void CBotTF2Snipe :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	// Sniper should move if the point has changed, so he's not wasting time
	if ( !CTeamFortress2Mod::m_ObjectiveResource.isWaypointAreaValid(m_iArea) )
		fail(); // move up
	else if ( m_iArea > 0 )
	{
		CBotTF2 *pBotTF2 = static_cast<CBotTF2*>(pBot);

		if ( CTeamFortress2Mod::isAttackDefendMap() )
		{
			if ( pBotTF2->getTeam() == TF2_TEAM_BLUE )
			{
				if ( m_iArea != pBotTF2->getCurrentAttackArea() )
					complete();
			}
			else if ( m_iArea != pBotTF2->getCurrentDefendArea() )
				complete();
		}
		else if (m_iArea != pBotTF2->getCurrentAttackArea() && m_iArea != pBotTF2->getCurrentDefendArea())
		{
			complete(); // move up
		}

	}

	// disable normal attack functions
	pBot->wantToShoot(false);

	// disable listening functions
	pBot->wantToListen(false);
	pBot->wantToInvestigateSound(false);

	// change weapon to sniper if not already
	CBotWeapon* pBotWeapon = pBot->getCurrentWeapon();

	if ( !pBotWeapon )
	{
		fail();
		return;
	}

	const CWeapon* pWeapon = pBotWeapon->getWeaponInfo();

	if (pWeapon == nullptr)
	{
		fail();
		return;
	}

	if (pWeapon->getID() != TF2_WEAPON_SNIPERRIFLE )
	{
		if ( !pBot->select_CWeapon(CWeapons::getWeapon(TF2_WEAPON_SNIPERRIFLE)) )
		{
			fail();
			return;
		}
	}

	// initialize
	if ( m_fTime == 0.0f )
	{
		// distance from sniper point default
		m_fOriginDistance = 100.0f;
		m_fHideTime = 0.0f;
		m_fEnemyTime = 0.0f;
		m_fAimTime = 0.0f;
		m_fTime = engine->Time() + randomFloat(40.0f,90.0f);
		//pBot->secondaryAttack();
		pBot->resetLookAroundTime();
		m_iPrevClip = pBotWeapon->getAmmo(pBot);

		CBotGlobals::quickTraceline(pBot->getEdict(),m_vOrigin,m_vAim);

		const int iAimWpt = CWaypointLocations::NearestWaypoint(CBotGlobals::getTraceResult()->endpos,400.0f,-1,true,true);

		if ( iAimWpt != -1 )
		{
			const CWaypoint *pWaypoint = CWaypoints::getWaypoint(m_iSnipeWaypoint);
			const CWaypointVisibilityTable *pTable = CWaypoints::getVisiblity();

			for ( int i = 0; i < pWaypoint->numPaths(); i ++ )
			{
				const int iPathId = pWaypoint->getPath(i);

				// isn't visible to the target
				if ( !pTable->GetVisibilityFromTo(iAimWpt,iPathId) )
				{
					m_iHideWaypoint = iPathId;
					m_vHideOrigin = CWaypoints::getWaypoint(iPathId)->getOrigin();
					break;
				}
			}

			if (m_iHideWaypoint == -1)
			{
				// can't find a useful hide waypoint -- choose a random one
				const int pathid = randomInt(0, pWaypoint->numPaths());
				m_iHideWaypoint = pWaypoint->getPath(pathid);

				if (m_iHideWaypoint != -1)
				{
					CWaypoint *pHideWaypoint = CWaypoints::getWaypoint(m_iHideWaypoint);

					if (pHideWaypoint != nullptr)
					{
						m_vHideOrigin = pHideWaypoint->getOrigin();
					}
					else
					{
						//detected a PATH problem
						//pWaypoint->removePathTo(pathid);
						m_iHideWaypoint = -1;
					}
				}
			}
		}
		
		// check wall time
		m_fCheckTime = engine->Time() + 0.15f;
	}
	else if (m_fTime<engine->Time() )
	{
		if ( CTeamFortress2Mod::TF2_IsPlayerZoomed(pBot->getEdict()) )
			pBot->secondaryAttack();

		complete();
		return;
	}

	// look at waypoint yaw and have random updates
	pBot->setLookAtTask(LOOK_SNIPE);

	// saw an enemy less than 5 secs ago
	if ( m_fEnemyTime + 5.0f > engine->Time() )
		pBot->setAiming(m_vEnemy);
	else
		pBot->setAiming(m_vAim);

	if ( !pBot->isTF() || static_cast<CBotFortress*>(pBot)->getClass() != TF_CLASS_SNIPER || pBot->getHealthPercent() < 0.2f )
	{
		// out of health -- finish
		if ( CTeamFortress2Mod::TF2_IsPlayerZoomed(pBot->getEdict()) )
			pBot->secondaryAttack();

		fail();
		return;
	}
	if ( pBotWeapon->getAmmo(pBot) < 1 )
	{
		// out of ammo -- finish
		if ( CTeamFortress2Mod::TF2_IsPlayerZoomed(pBot->getEdict()) )
			pBot->secondaryAttack();

		complete();
	}
	else if ( pBot->distanceFrom(m_vOrigin) > 400 )
	{
		if ( CTeamFortress2Mod::TF2_IsPlayerZoomed(pBot->getEdict()) )
			pBot->secondaryAttack();
		// too far away
		fail();
	}
	else if ( m_iHideWaypoint != -1 && (static_cast<CBotFortress*>(pBot)->incomingRocket(512.0f) || m_fHideTime > engine->Time()) )
	{
		// hide time -- move to hide origin
		pBot->setMoveTo(m_vHideOrigin);

		if ( pBot->distanceFrom(m_vHideOrigin) < 100 )
		{
			pBot->stopMoving();

			if ( pBotWeapon->needToReload(pBot) )
				pBot->reload();
		}
	}
	else if ( pBot->distanceFrom(m_vOrigin) > m_fOriginDistance )
	{
		// is too far from origin -- use normal attack code at this time otherwise bot will be a sitting duck
		pBot->setMoveTo(m_vOrigin);
		pBot->wantToShoot(true);
	}
	else
	{
		pBot->stopMoving();

		if ( CTeamFortress2Mod::TF2_IsPlayerZoomed(pBot->getEdict()) && pBot->hasEnemy() )
		{
			if ( pBot->hasSomeConditions(CONDITION_SEE_CUR_ENEMY) )
			{
				m_vEnemy = CBotGlobals::entityOrigin(pBot->getEnemy());
				m_fEnemyTime = engine->Time();

				if ( m_fAimTime == 0.0f )
					m_fAimTime = engine->Time() + randomFloat(0.1f,0.3f);

				// too close for sniper rifle
				if ( pBot->distanceFrom(pBot->getEnemy()) < 200.0f )
				{
					complete();
					return;
				}
			}

			// careful that the round may have not started yet
			if ( CTeamFortress2Mod::hasRoundStarted() )
			{
				pBot->setMoveLookPriority(MOVELOOK_ATTACK);

				pBot->setLookAtTask(LOOK_ENEMY);

				const float angle = pBot->DotProductFromOrigin(CBotGlobals::entityOrigin(pBot->getEnemy()));

				if ( angle > 0.96f ) // 16 degrees
				{
					if ( m_fAimTime < engine->Time() )
						pBot->handleAttack(pBotWeapon,pBot->getEnemy());

					if ( m_iPrevClip > pBotWeapon->getAmmo(pBot) )
					{
						bool bhide;

						// hide if player can see me, otherwise if not player hide if low health
						if ( CBotGlobals::isPlayer(pBot->getEnemy()) )						
							bhide = CBotGlobals::DotProductFromOrigin(pBot->getEnemy(),pBot->getOrigin()) > 0.96f;						
						else
							bhide = randomFloat(0.0f,2.0f) <= angle+(1.0f-pBot->getHealthPercent());
						
						if ( bhide )
							m_fHideTime = engine->Time() + randomFloat(1.5f,2.5f);

						m_fAimTime = 0.0f;
						m_iPrevClip = pBotWeapon->getAmmo(pBot);
					}
				}

				pBot->setMoveLookPriority(MOVELOOK_TASK);
			}


		}
		else
		{
			// time to check if bot is just looking at a wall or not
			if( m_fCheckTime < engine->Time() )
			{
				CBotGlobals::quickTraceline(pBot->getEdict(),pBot->getOrigin(),pBot->getAiming());
				
				if ( CBotGlobals::getTraceResult()->fraction < 0.1f ) 
				{
					m_fOriginDistance = 40.0f;
					m_fCheckTime = engine->Time() + 0.3f;
				}
				else
				{
					m_fOriginDistance = 100.0f;
					m_fCheckTime = engine->Time() + 0.15f;
				}
			}
			// zoom in
			if ( !CTeamFortress2Mod::TF2_IsPlayerZoomed(pBot->getEdict()) )
				pBot->secondaryAttack();
			else
				m_fHideTime = 0.0f;
		}
	}
}

/////////////////////////////////////////////////////

CBotTF2SpySap :: CBotTF2SpySap ( edict_t *pBuilding, eEngiBuild id )
{
	m_pBuilding = MyEHandle(pBuilding);
	m_fTime = 0.0f;
	m_id = id;
}

void CBotTF2SpySap :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	CBotTF2 *tf2Bot = static_cast<CBotTF2*>(pBot);

	if ( !pBot->isTF() )
	{
		fail();
		return;
	}

	if ( m_fTime == 0.0f )
	{
		m_fTime = engine->Time() + randomFloat(4.0f,6.0f);
		tf2Bot->resetCloakTime();
	}


	pBot->wantToShoot(false);

	edict_t* pBuilding = m_pBuilding.get();

	if ( !pBuilding )
	{
		complete();
		return;
	}

	if ( m_id == ENGI_SENTRY )
	{
		if ( CTeamFortress2Mod::isSentrySapped(pBuilding) )
		{
			complete();
			return;
		}
	}
	else if ( m_id == ENGI_DISP )
	{
		if ( CTeamFortress2Mod::isDispenserSapped(pBuilding) )
		{
			complete();
			return;
		}
	}
	else if ( m_id == ENGI_TELE ) 
	{
		if ( CTeamFortress2Mod::isTeleporterSapped(pBuilding) )
		{
			complete();
			return;
		}
	}

	pBot->lookAtEdict(pBuilding);
	pBot->setLookAtTask(LOOK_EDICT,0.2f);
	const CBotWeapon* weapon = tf2Bot->getCurrentWeapon();

	// time out
	if ( m_fTime < engine->Time() )
		fail();
	else if ( !weapon || weapon->getID() != TF2_WEAPON_BUILDER )
	{
		helpers->ClientCommand(pBot->getEdict(),"build 3 0");
	}
	else 
	{
		if ( pBot->distanceFrom(pBuilding) > 100 )
		{
			pBot->setMoveTo(CBotGlobals::entityOrigin(pBuilding));
		}
		else
		{
			if ( CTeamFortress2Mod::TF2_IsPlayerCloaked(pBot->getEdict()) )
			{
				tf2Bot->spyUnCloak();
			}
			else if ( randomInt(0,1) )
				pBot->tapButton(IN_ATTACK);
			//complete();
		}
	}

}

void CBotTF2SpySap :: debugString ( char *string )
{
	std::sprintf(string,"CBotTF2SpySap");
}


/////////////////////////////////////////////////////
CBotTFUseTeleporter :: CBotTFUseTeleporter ( edict_t *pTele )
{// going to use this 
	
	m_pTele = pTele;
	m_fTime = 0.0f;
}

void CBotTFUseTeleporter :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	if ( !m_pTele || !CBotGlobals::entityIsValid(m_pTele) )
	{
		fail();
		return;
	}

	if ( !pBot->isTF() )
	{
		if ( static_cast<CBotFortress*>(pBot)->hasFlag() )
		{
			fail();
			return;
		}
	}

	if ( !m_fTime )
	{
		// initialize
		m_fTime = engine->Time() + 13.0f;
		m_vLastOrigin = pBot->getOrigin();
	}

	// FIX BUG
	//if ( !((CBotFortress*)pBot)->isTeleporterUseful(m_pTele) )
	//	fail();

	if ( m_fTime < engine->Time() )
	{
		if ( CClients::clientsDebugging(BOT_DEBUG_TASK) )
			CClients::clientDebugMsg(BOT_DEBUG_TASK,"TELEPORT: TIMEOUT",pBot);

		fail();

	}
	else
	{
		if ( CTeamFortress2Mod::getTeleporterExit(m_pTele) ) // exit is still alive?
		{
			const Vector vTele = CBotGlobals::entityOrigin(m_pTele);		

			if ( pBot->distanceFrom(vTele) > 48 || CClassInterface::getGroundEntity(pBot->getEdict())!=m_pTele.get() )
			{
				pBot->setMoveTo(vTele);		
				
				if ( (m_vLastOrigin - pBot->getOrigin()).Length() > 50 )
				{
					pBot->getNavigator()->freeMapMemory(); // restart navigator
				
					complete(); // finished
				}
			}
			else
			{
				pBot->stopMoving();
			}
		
			m_vLastOrigin = pBot->getOrigin();

		}
		else
			fail();
	}
}

void CBotTFUseTeleporter :: debugString ( char *string )
{
	std::sprintf(string, "CBotTFUseTeleporter\nm_pTele = %p", m_pTele.get());
}

///////////////////////////////////////////////////

CAttackEntityTask :: CAttackEntityTask ( edict_t *pEdict )
{
	m_pEdict = pEdict;
}

void CAttackEntityTask :: debugString ( char *string )
{
	int id = -1;

	if ( m_pEdict )
	  id = ENTINDEX(m_pEdict);

	std::sprintf(string,"CAttackEntityTask (%d)",id);	
}

void CAttackEntityTask :: init ()
{
	//setFailInterrupt ( CONDITION_ENEMY_OBSCURED );
	//setCompleteInterrupt ( CONDITION_ENEMY_DEAD );
}

void CAttackEntityTask :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	if ( m_pEdict.get() == nullptr)
	{
		fail();
		return;
	}

	if ( !pBot->isEnemy(m_pEdict) )
	{
		complete();
		return;
	}

	if ( !pBot->isVisible(m_pEdict) )
	{
		fail();
		return;
	}

	if ( pBot->hasSomeConditions(CONDITION_ENEMY_DEAD) )
	{
		fail();
		return;
	}

	CBotWeapon* pWeapon = pBot->getBestWeapon(m_pEdict);

	if ( pWeapon != nullptr && pWeapon != pBot->getCurrentWeapon() && pWeapon->getWeaponIndex() )
	{
		pBot->selectWeapon(pWeapon->getWeaponIndex());
	}

	pBot->setEnemy(m_pEdict);

	pBot->setLookAtTask(LOOK_ENEMY);

	if ( !pBot->handleAttack ( pWeapon, m_pEdict ) )
		fail();
}

///
CThrowGrenadeTask :: CThrowGrenadeTask (CBotWeapon *pWeapon, int ammo, const Vector& vLoc )
{
	m_pWeapon = pWeapon;
	m_fTime = 0.0f;
	m_vLoc = vLoc;

	m_fHoldAttackTime = 0.0f;
	m_iAmmo = ammo;
}

void CThrowGrenadeTask ::init()
{
	m_fTime = 0.0f;
}

void CThrowGrenadeTask::debugString(char *string)
{
	std::sprintf(string,"CThrowGrenadeTask\nm_vLoc =(%0.4f,%0.4f,%0.4f)\nfTime = %0.4f",m_vLoc.x,m_vLoc.y,m_vLoc.z,m_fTime);	
}

void CThrowGrenadeTask ::execute (CBot *pBot,CBotSchedule *pSchedule)
{
	if ( m_fTime == 0.0f )
	{
		m_fTime = engine->Time() + 2.5f;

		if ( sv_gravity.IsValid() )
		{
			const float fFraction = pBot->distanceFrom(m_vLoc)/MAX_GREN_THROW_DIST;
			//m_vLoc.z = m_vLoc.z + getGrenadeZ(pBot->getOrigin(),m_vLoc,m_pWeapon->getProjectileSpeed());
			m_vLoc.z = m_vLoc.z + (sv_gravity.GetFloat() * randomFloat(1.5f,2.5f) * fFraction);
		}
	}

	// time out
	if (m_fTime < engine->Time())
	{
		fail();
		return;
	}
	// used all grenades Updated: 2017-05-22 Created: 2017 - 05 - 22 Creator : Gabor Szuromi
	if (!m_pWeapon || !m_pWeapon->hasWeapon())
	{
		complete();
		return;
	}

	if ( m_pWeapon->getAmmo(pBot) < m_iAmmo )
	{
		pBot->grenadeThrown();
		complete();
		return;
	}

	if ( !m_pWeapon )
	{
		fail();
		return;
	}

	const CBotWeapon* pWeapon = pBot->getCurrentWeapon();
	pBot->wantToChangeWeapon(false);

	if ( pWeapon && pWeapon->isGravGun() && CClassInterface::gravityGunObject(INDEXENT(pWeapon->getWeaponIndex())) )
	{
		// drop it
		if ( randomInt(0,1) )
			pBot->primaryAttack();	
		else
			pBot->secondaryAttack();
	}
	else if ( pWeapon != m_pWeapon )
	{
		pBot->selectBotWeapon(m_pWeapon);
	}
	else
	{
		pBot->setLookVector(m_vLoc);
		pBot->setLookAtTask(LOOK_VECTOR);

		if ( pBot->isFacing(m_vLoc) )
		{
			if ( randomInt(0,1) )
				pBot->primaryAttack();
		}
		else if ( pBot->hasSomeConditions(CONDITION_SEE_CUR_ENEMY) )
		{
			pBot->primaryAttack();
			fail();
		}
	}
}
///
CBotInvestigateHidePoint :: CBotInvestigateHidePoint ( int iWaypointIndexToInvestigate, int iOriginalWaypointIndex )
{
	CWaypoint *pWaypoint = CWaypoints::getWaypoint(iWaypointIndexToInvestigate);
	CWaypoint *pOriginalWpt = CWaypoints::getWaypoint(iOriginalWaypointIndex);
	m_vOrigin = pOriginalWpt->getOrigin();
	m_vMoveTo = pWaypoint->getOrigin();
	m_fTime = 0.0f;
	m_fInvestigateTime = 0.0f;
	m_iState = 0;

	for ( int i = 0; i < pWaypoint->numPaths(); i ++ )
	{
		CWaypoint *pWaypointOther = CWaypoints::getWaypoint(pWaypoint->getPath(i));

		if ( pWaypointOther == pWaypoint )
			continue;
		if ( pWaypointOther == pOriginalWpt )
			continue;

		m_CheckPoints.emplace_back(pWaypointOther->getOrigin());
	}

	m_iCurrentCheckPoint = 0;
}

void CBotInvestigateHidePoint:: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	if ( m_fTime == 0.0f )
	{
		m_fTime = engine->Time() + randomFloat(6.0f, 8.0f);
		m_fInvestigateTime = randomFloat(0.3f,1.0f);
	}
	else if ( m_fTime < engine->Time() )
	{
		if ( m_iState == 2 )
			complete();
		else if ( m_iState != 2 ) // go back to origin
		{
			m_fTime = engine->Time() + randomFloat(1.0f,2.0f);
			m_iState = 2;
		}
	}

	if (m_CheckPoints.empty())
	{
		// don't move, just look
		pBot->setLookVector(m_vMoveTo);
		pBot->setLookAtTask(LOOK_VECTOR);

		if ( m_fInvestigateTime < engine->Time() )
			complete();
	}
	else
	{

		switch ( m_iState )
		{
		case 0: // goto m_vMoveTo
			if ( pBot->distanceFrom(m_vMoveTo) > 70 )
				pBot->setMoveTo(m_vMoveTo);
			else
			{
				m_iState = 1;
				m_fInvestigateTime = engine->Time() + randomFloat(0.3f,1.3f);
			}
			pBot->setLookVector(m_vMoveTo);
			pBot->setLookAtTask(LOOK_VECTOR);
			break;
		case 1:
			pBot->stopMoving();
			if ( m_iCurrentCheckPoint < m_CheckPoints.size() )
			{
				if ( m_fInvestigateTime < engine->Time() )
				{
					m_iCurrentCheckPoint++;
					m_fInvestigateTime = engine->Time() + randomFloat(0.3f,1.3f);
				}
				else
				{
					pBot->setLookVector(m_CheckPoints[m_iCurrentCheckPoint]);
					pBot->setLookAtTask(LOOK_VECTOR);
				}
			}
			else
				m_iState = 2;
			break;
		case 2:
			// go back to origin
			if ( pBot->distanceFrom(m_vOrigin) > 70 )
				pBot->setMoveTo(m_vOrigin);
			else
			{
				complete();
			}

			pBot->setLookVector(m_vOrigin);
			pBot->setLookAtTask(LOOK_VECTOR);

			break;
		}

	}
}

void CBotInvestigateHidePoint::debugString ( char *string )
{
	std::sprintf(string,"CBotInvestigateHidePoint");
}
//////

void CAutoBuy :: init () 
{ 
	m_bTimeset = false;
}

void CAutoBuy :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	if ( !m_bTimeset )
	{
		m_bTimeset = true;
		m_fTime = engine->Time() + randomFloat(2.0f,4.0f);
	}
	else if ( m_fTime < engine->Time() )
	{
		engine->SetFakeClientConVarValue(pBot->getEdict(),"cl_autobuy","m4a1 ak47 famas galil p90 mp5 primammo secammo defuser vesthelm vest");
		//helpers->ClientCommand(pBot->getEdict(),"setinfo cl_autobuy \"m4a1 ak47 famas galil p90 mp5 primammo secammo defuser vesthelm vest\"\n");
		helpers->ClientCommand(pBot->getEdict(),"autobuy\n");	
		complete();
	}
}

void CCSSPerformBuyTask::init()
{
	m_fDelay = engine->Time() + randomFloat(2.0f, 5.0f);
}

void CCSSPerformBuyTask::execute(CBot *pBot,CBotSchedule *pSchedule)
{
	if(m_fDelay <= engine->Time())
	{
		if(!CClassInterface::isCSPlayerInBuyZone(pBot->getEdict()))
		{
			fail(); // outside buy zone
			return;
		}
		CCSSBot *pCSSBot = static_cast<CCSSBot*>(pBot);
		pCSSBot->runBuy();
		complete();
	}
	else
	{
		pBot->stopMoving();
		pBot->wantToShoot(false);
		pBot->setMoveLookPriority(MOVELOOK_OVERRIDE);
		pBot->setLookAtTask(LOOK_AROUND);
		pBot->setMoveLookPriority(MOVELOOK_TASK);
	}
}

void CCSSPlantTheBombTask::execute(CBot *pBot,CBotSchedule *pSchedule)
{
	pBot->stopMoving();
	pBot->wantToShoot(false);
	pBot->wantToChangeWeapon(false);
	pBot->setMoveLookPriority(MOVELOOK_OVERRIDE);
	pBot->setLookAtTask(LOOK_AROUND);
	pBot->setMoveLookPriority(MOVELOOK_TASK);

	if(!CCounterStrikeSourceMod::isBombCarrier(pBot))
	{
		complete();
		pBot->letGoOfButton(IN_ATTACK);
		return;
	}

	if(CClassInterface::isCSPlayerInBombZone(pBot->getEdict()))
	{
		if(pBot->getCurrentWeapon() != pBot->getWeapons()->getWeapon(CWeapons::getWeapon(CS_WEAPON_C4)))
		{
			pBot->selectBotWeapon(pBot->getWeapons()->getWeapon(CWeapons::getWeapon(CS_WEAPON_C4)));
		}
		else
		{
			pBot->primaryAttack(true);
		}
	}
	else
	{
		if(CClassInterface::getGroundEntity(pBot->getEdict()) != nullptr)
		{ // Must be on ground to fail since you can't plant while in the air and the bot might have to jump to reach some plant spots
			fail();
		}
	}
}

void CCSSEngageEnemyTask::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	edict_t *pEnemy = engine->PEntityOfEntIndex(m_hEnemy.GetEntryIndex());
	const edict_t *pWeapon = CClassInterface::getCurrentWeapon(pBot->getEdict());
	isBrush = CBotGlobals::isBrushEntity(pEnemy);

	if(pBot->hasSomeConditions(CONDITION_OUT_OF_AMMO))
	{
		fail();
		return;
	}

	if(pBot->getEnemy() == nullptr || pBot->getEnemy() != pEnemy)
	{
		fail();
		return;
	}

	if(pEnemy)
	{
		if(pWeapon)
		{
			const CBotWeapon *weapon = pBot->getWeapons()->getWeapon(CWeapons::getWeaponByShortName(pWeapon->GetClassName()));
			if(weapon)
			{
				if(weapon->isMelee())
				{
					if(isBrush)
						pBot->setMoveTo(CBotGlobals::worldCenter(pEnemy));
					else
					pBot->setMoveTo(CBotGlobals::entityOrigin(pEnemy));
				}
				else
				{
					pBot->stopMoving();
				}
			}
		}
		else
		{
			pBot->stopMoving();
		}

		if(!CBotGlobals::entityIsAlive(pEnemy))
		{
			complete();
		}

		pBot->setLookAtTask(LOOK_ENEMY);
		//pBot->handleAttack(pBot->getCurrentWeapon(), pBot->getEnemy());
	}
	else
	{
		complete();
	}
}

void CCSSEngageEnemyTask::debugString(char *string)
{
	const edict_t *pEnemy = engine->PEntityOfEntIndex(m_hEnemy.GetEntryIndex());
	std::sprintf(string,"CSS Engage Enemy\n%s", pEnemy ? pEnemy->GetClassName() : "null");
}

void CCSSDefuseTheBombTask::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	pBot->stopMoving();
	pBot->setMoveLookPriority(MOVELOOK_OVERRIDE);
	pBot->setLookVector(m_vBomb);
	pBot->setLookAtTask(LOOK_VECTOR);
	pBot->setMoveLookPriority(MOVELOOK_TASK);

	if(pBot->distanceFrom(m_vBomb) >= 75.0f)
	{
		fail();
	}

	pBot->use();

	if(CCounterStrikeSourceMod::isBombDefused())
	{
		complete();
	}
}

void CCSSGuardTask::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	static CBotWeapon *pCurrentWeapon;
	static CWeapon *pWeapon;

	static float fDist;

	pBot->wantToShoot(false);
	pBot->wantToListen(false);

	if ( m_fTime == 0.0f )
	{
		m_fEnemyTime = engine->Time();
		m_fTime = randomFloat(15.0f,30.0f);
		pBot->resetLookAroundTime();
	}

	pCurrentWeapon = pBot->getCurrentWeapon();

	if ( !pCurrentWeapon )
	{
		fail();
		return;
	}

	pWeapon = pCurrentWeapon->getWeaponInfo();

	if (pWeapon == nullptr)
	{
		fail();
		return;
	}

	if ( m_pWeaponToUse && pCurrentWeapon != m_pWeaponToUse )
	{
		if ( !pBot->selectBotWeapon(m_pWeaponToUse) )
		{
			fail();
		}

		return;
	}

	if ( pCurrentWeapon->getAmmo(pBot) < 1 )
	{
		complete();
	}
	else if ( pBot->distanceFrom(m_vOrigin) > 200 ) // too far from sniper point
	{
		// too far away
		fail();
	}

	if ( m_bUseZ )
	{
		const Vector vAim = Vector(m_vAim.x,m_vAim.y,m_z);
		pBot->setLookAtTask(LOOK_VECTOR);
		pBot->setLookVector(pBot->snipe(vAim));
	}
	else
	{
		pBot->setLookAtTask(LOOK_SNIPE);
		pBot->setLookVector(pBot->snipe(m_vAim));
	}

	fDist = (m_vOrigin - pBot->getOrigin()).Length2D();

	if ( fDist > 32 )
	{
		pBot->setMoveTo(m_vOrigin);
		pBot->setMoveSpeed(CClassInterface::getMaxSpeed(pBot->getEdict())/2);
	}
	else
	{
		pBot->stopMoving();

		if ( m_iWaypointType & CWaypointTypes::W_FL_CROUCH )
			pBot->duck();

		// no enemy for a while
		if ( m_fEnemyTime + m_fTime < engine->Time() )
		{
			complete();
		}

		if( pCurrentWeapon->isZoomable() && !CCounterStrikeSourceMod::isScoped(pBot) )
		{
			pBot->secondaryAttack(false);
		}
	}

	if ( pBot->hasEnemy() )
	{
		pBot->setMoveLookPriority(MOVELOOK_ATTACK);
		pBot->setLookAtTask(LOOK_ENEMY);
		pBot->handleAttack(pCurrentWeapon,pBot->getEnemy());
		pBot->setMoveLookPriority(MOVELOOK_TASK);

		// havin' fun
		m_fEnemyTime = engine->Time();
	}
}

CFindLastEnemy::CFindLastEnemy (const Vector& vLast, const Vector& vVelocity)
{
	setCompleteInterrupt(CONDITION_SEE_CUR_ENEMY);
	m_vLast = vLast+vVelocity*10;
	m_fTime = 0.0f;
}

void CFindLastEnemy::execute ( CBot *pBot, CBotSchedule *pSchedule )
{
	if ( m_fTime == 0.0f )
		m_fTime = engine->Time() + randomFloat(2.0f,4.0f);

	if ( !pBot->moveToIsValid() || pBot->moveFailed() )
		fail();
	if ( pBot->distanceFrom(m_vLast) > 80 )
		pBot->setMoveTo(m_vLast);
	else
		pBot->stopMoving();

	pBot->setLookAtTask(LOOK_AROUND);

	if ( m_fTime < engine->Time() )
		complete();
}
////////////////////////
CFollowTask :: CFollowTask ( edict_t *pFollow ) 
{
	m_pFollow = pFollow;
	m_fFollowTime = 0;
	m_vLastSeeVector = CBotGlobals::entityOrigin(pFollow);
	CClassInterface::getVelocity(pFollow,&m_vLastSeeVelocity);
}

void CFollowTask :: init ()
{

}

void CFollowTask::execute ( CBot *pBot, CBotSchedule *pSchedule )
{
	if ( m_pFollow.get() == nullptr)
	{
		fail();
		return;
	}

	if ( !CBotGlobals::entityIsAlive(m_pFollow.get()) )
	{
		fail();
		return;
	}

	pBot->setLookVector(m_vLastSeeVector);
	pBot->setLookAtTask(LOOK_VECTOR);

	if ( pBot->isVisible(m_pFollow) )
	{
		m_vLastSeeVector = CBotGlobals::entityOrigin(m_pFollow);

		if ( pBot->distanceFrom(m_pFollow) > 150.0f )
			pBot->setMoveTo(m_vLastSeeVector);
		else
			pBot->stopMoving();

		CClassInterface::getVelocity(m_pFollow,&m_vLastSeeVelocity);

		m_vLastSeeVector = m_vLastSeeVector + m_vLastSeeVelocity;

		m_fFollowTime = engine->Time() + 1.0f;
	}

	if ( m_fFollowTime < engine->Time() )
		complete();
}

void CFollowTask::debugString ( char *string )
{
	std::sprintf(string,"CFollowTask\nm_pFollow =(%s)",engine->GetPlayerNetworkIDString(m_pFollow));
}
////////////////////////////////////////////////

void CDODDropAmmoTask :: debugString ( char *string )
{
	std::sprintf(string,"CDODDropAmmoTask");
}

void CDODDropAmmoTask :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	const Vector vOrigin = CBotGlobals::entityOrigin(m_pPlayer.get());

	if ( m_pPlayer.get() == nullptr)
	{
		fail();
		return;
	}

	if ( !CBotGlobals::entityIsAlive(m_pPlayer) )
	{
		fail();
		return;
	}

	pBot->setLookAtTask(LOOK_VECTOR);
	pBot->setLookVector(vOrigin);
	pBot->setMoveTo(vOrigin);

	if ( pBot->isFacing(vOrigin) )
	{
		if ( !pBot->isVisible(m_pPlayer) )
		{
			fail();
			return;
		}
	}

	if ( pBot->distanceFrom(m_pPlayer) < 200.0f && pBot->isFacing(vOrigin) )
	{
		CDODBot *pDODBot = static_cast<CDODBot*>(pBot);
		pDODBot->dropAmmo();
		complete();
	}
		
}

////////////////////////////////////////////
CCrouchHideTask :: CCrouchHideTask( edict_t *pHideFrom )
{
	m_pHideFrom = pHideFrom;
	m_vLastSeeVector = CBotGlobals::entityOrigin(pHideFrom);
	m_bCrouching = true; // duck
	m_fChangeTime = 0.0f;
	m_fHideTime = 0.0f;
}

void CCrouchHideTask :: init ()
{
	m_bCrouching = true; // duck
	m_fChangeTime = 0.0f;
	m_fHideTime = 0.0f;
}

void CCrouchHideTask :: debugString ( char *string )
{
	std::sprintf(string,"CCrouchHideTask\nm_pHideFrom =(%s)",engine->GetPlayerNetworkIDString(m_pHideFrom));
}

void CCrouchHideTask :: execute ( CBot *pBot, CBotSchedule *pSchedule )
{
	pBot->wantToListen(false);

	if ( m_pHideFrom.get() == nullptr)
	{
		complete();
		return;
	}
	if ( !CBotGlobals::entityIsAlive(m_pHideFrom) )
	{
		complete();
		return;
	}

	if ( m_fHideTime == 0.0f )
		m_fHideTime = engine->Time() + randomFloat(7.0f,14.0f);

	if ( m_fChangeTime == 0.0f )
		m_fChangeTime = engine->Time() + randomFloat(0.9f,2.9f);

	pBot->stopMoving();	

	if ( pBot->isVisible(m_pHideFrom) )
		m_vLastSeeVector = CBotGlobals::entityOrigin(m_pHideFrom);

	pBot->setLookVector(m_vLastSeeVector);

	pBot->setLookAtTask(LOOK_VECTOR);

	if ( m_fChangeTime < engine->Time() )
	{
		m_bCrouching = !m_bCrouching;
		m_fChangeTime = engine->Time() + randomFloat(1.0f,3.0f);
	}

	if ( m_bCrouching )
	{
		pBot->duck(true);
		pBot->wantToShoot(false);
	}
	else if ( pBot->hasEnemy() )
	{
		m_fHideTime = engine->Time() + 10.0f;
	}

	// refrain from proning
	pBot->updateCondition(CONDITION_RUN);
	pBot->removeCondition(CONDITION_PRONE);

	if ( m_fHideTime < engine->Time() )
		complete();

}

////////////////////////////////////////////////////////
CHideTask :: CHideTask(const Vector& vHideFrom )
{
	m_fHideTime = 0.0f;
	m_vHideFrom = vHideFrom;
}

void CHideTask :: debugString ( char *string )
{
	std::sprintf(string,"CHideTask\nm_vHideFrom =(%0.4f,%0.4f,%0.4f)",m_vHideFrom.x,m_vHideFrom.y,m_vHideFrom.z);	
}

void CHideTask :: init ()
{
	m_fHideTime = 0.0f;	
}

void CHideTask :: execute ( CBot *pBot, CBotSchedule *pSchedule )
{
	pBot->stopMoving();	
	pBot->setLookVector(m_vHideFrom);
	pBot->setLookAtTask(LOOK_VECTOR);
	pBot->duck(true);

	if ( m_fHideTime == 0.0f )
		m_fHideTime = engine->Time() + randomFloat(5.0f,10.0f);

	if ( m_fHideTime < engine->Time() )
		complete();
}
///////////////////////////////////////////
CBotTF2DemomanPipeJump :: CBotTF2DemomanPipeJump ( CBot *pBot, const Vector& vWaypointGround,
												   const Vector& vWaypointNext, 
						CBotWeapon *pWeapon )
{
	m_iStartingAmmo = pWeapon->getClip1(pBot);
	m_vStart = vWaypointGround - Vector(0,0,48.0f);
	m_vEnd = vWaypointNext;
	m_pPipeBomb = nullptr;
	m_fTime = 0.0f;
	m_iState = 0;
	m_pWeapon = pWeapon;
	m_bFired = false;
}

void CBotTF2DemomanPipeJump :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	pBot->wantToListen(false);

	if ( m_fTime == 0.0f )
	{
		// init
		m_fTime = engine->Time() + randomFloat(5.0f,10.0f);
	}

	if ( m_fTime < engine->Time() )
		fail(); // time out

	if ( pBot->getCurrentWeapon() != m_pWeapon )
	{
		pBot->selectBotWeapon(m_pWeapon);
		return;
	}

	if ( m_pPipeBomb )
	{
		if ( !CBotGlobals::entityIsValid(m_pPipeBomb) )
		{
			fail();
			return;
		}
	}

	switch ( m_iState )
	{
	case 0:
		if ( m_pWeapon->getClip1(pBot) == 0 )
		{
			if ( m_pWeapon->getAmmo(pBot) == 0 )
				fail();
			else if ( randomInt(0,1) )
				pBot->tapButton(IN_RELOAD);
		}
		else
		{
			if ( !m_bFired && !m_iStartingAmmo )
				m_iStartingAmmo = m_pWeapon->getClip1(pBot);
			else if ( m_bFired && m_iStartingAmmo > m_pWeapon->getClip1(pBot) )
			{
				// find pipe bomb
				m_pPipeBomb = CClassInterface::FindEntityByClassnameNearest(pBot->getOrigin(),"tf_projectile_pipe_remote",150.0f, nullptr);

				if ( m_pPipeBomb )
				{
					// set this up incase of fail, the bot knows he has a sticky there
					static_cast<CBotTF2*>(pBot)->setStickyTrapType(m_vStart,TF_TRAP_TYPE_ENEMY);
					m_iState++;
				}
				else
					fail();
			}
			else
			{
				pBot->setLookVector(m_vStart);
				pBot->setLookAtTask(LOOK_VECTOR);

				if ( pBot->distanceFrom(m_vStart) < 150 )
				{
					if ( pBot->DotProductFromOrigin(m_vStart) > 0.99f )
					{
						if ( randomInt(0,1) )
						{
							pBot->primaryAttack();
							m_bFired = true;
						}
					}
				}
				else
				{
					pBot->setMoveTo(m_vStart);
				}
			}
		}
		break;
	case 1:
		{
			Vector vel;

			if ( CClassInterface::getVelocity(m_pPipeBomb,&vel) )
			{
				if ( vel.Length() > 1.0f )
					break; // wait until the pipe bomb has rested
			}

			Vector v_comp = m_vEnd - m_vStart;
			v_comp = v_comp / v_comp.Length();

			const Vector v_pipe = CBotGlobals::entityOrigin(m_pPipeBomb);
			Vector v_startrunup = v_pipe - v_comp * rcbot_demo_runup_dist.GetFloat();
			v_startrunup.z = v_pipe.z;

			pBot->lookAtEdict(m_pPipeBomb);
			pBot->setLookAtTask(LOOK_EDICT);

			// m_pPipeBomb != NULL

			// run up and jump time

			if ( pBot->distanceFrom(v_startrunup) < 52.0f )
				m_iState++;
			
			pBot->setMoveTo(v_startrunup);
		}
		break;
	case 2:
		{
			Vector v_comp = m_vEnd - m_vStart;
			v_comp = v_comp / v_comp.Length();
			const Vector v_pipe = CBotGlobals::entityOrigin(m_pPipeBomb);

			Vector v_endrunup = v_pipe + v_comp * rcbot_demo_runup_dist.GetFloat();
			v_endrunup.z = v_pipe.z;

			pBot->setLookVector(m_vEnd);
			pBot->setLookAtTask(LOOK_VECTOR);

			// m_pPipeBomb != NULL

			// run up and jump time

			if ( pBot->distanceFrom(v_endrunup) < 48.0f )
			{
				m_iState++;
			}
			
			pBot->setMoveTo(v_endrunup);
		}
		break;
	case 3:
		pBot->jump();
		m_iState++;
		break;
	case 4:
		{
			Vector vel;
		
			if ( CClassInterface::getVelocity(pBot->getEdict(),&vel) )
			{
				if ( vel.z > 10 )
				{
					static_cast<CBotTF2*>(pBot)->detonateStickies(true);
					complete();
				}
			}
			else
			{
				static_cast<CBotTF2*>(pBot)->detonateStickies(true);
				complete();
			}
		}
		break;
	default:
		fail();
	}
}

//////////////////////////////////////////
CBotTF2DemomanPipeEnemy :: CBotTF2DemomanPipeEnemy ( CBotWeapon *pPipeLauncher, const Vector& vEnemy, edict_t *pEnemy )
{
	m_vEnemy = vEnemy;
	m_pEnemy = MyEHandle(pEnemy);
	m_fTime = 0.0f;
	m_vAim = vEnemy;
	m_pPipeLauncher = pPipeLauncher;
	m_fHoldAttackTime = 0.0f;
	m_fHeldAttackTime = 0.0f;
}

void CBotTF2DemomanPipeEnemy :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	if ( m_pEnemy.get() == nullptr)
	{
		fail();
		return;
	}

	if ( pBot->recentlyHurt(1.0f) )
	{
		// taking damage
		fail();
		pBot->updateUtilTime(pBot->getCurrentUtil());
		return;
	}

	if( CTeamFortress2Mod::TF2_IsPlayerInvuln(pBot->getEdict()) )
		fail();

	pBot->wantToListen(false);
	pBot->wantToInvestigateSound(false);

	if ( m_fTime == 0.0f )
	{

		m_vStand = CWaypoints::getWaypoint(pSchedule->passedInt())->getOrigin();
		const Vector vOtherWaypoint = pSchedule->passedVector();
		static_cast<CBotTF2*>(pBot)->setStickyTrapType(m_vEnemy,TF_TRAP_TYPE_ENEMY);

		// Need to Lob my pipes
		if ( vOtherWaypoint.z > m_vEnemy.z+32.0f )
		{
			m_vAim = m_vEnemy; //(m_vEnemy - pBot->getOrigin())/2;
			m_vAim.z = m_vEnemy.z + getGrenadeZ(pBot->getEdict(),m_pEnemy,pBot->getOrigin(),m_vEnemy,TF2_GRENADESPEED);//();//(sv_gravity->GetFloat() * randomFloat(0.9f,1.1f) * fFraction);
		}
		else
		{
			// otherwise just aim at the closest waypoint
			m_vAim = (vOtherWaypoint+m_vEnemy)/2;
		}

		m_fHoldAttackTime = pBot->distanceFrom(m_vEnemy)/512.0f - 1.0f;

		if ( m_fHoldAttackTime < 0.0f )
			m_fHoldAttackTime = 0.0f;

		/*
		if ( sv_gravity )
		{
			float fFraction = ((m_vAim-m_vStand).Length())/MAX_GREN_THROW_DIST;

			m_vAim.z = m_vAim.z + (sv_gravity->GetFloat() * randomFloat(1.5f,2.5f) * fFraction);
		}*/

		m_fTime = engine->Time() + randomFloat(5.0f,10.0f);
	}

	if ( pBot->distanceFrom(m_vStand) > 200 )
	{
		fail();
		pBot->setLastEnemy(nullptr);
	}


	if ( !CBotGlobals::entityIsValid(m_pEnemy) || !CBotGlobals::entityIsAlive(m_pEnemy) || m_fTime < engine->Time() )
	{
		// blow up any grens before we finish
		//if ( m_pEnemy.get() && pBot->isVisible(m_pEnemy.get()) )
		static_cast<CBotTF2*>(pBot)->detonateStickies(true);

		complete();
		pBot->setLastEnemy(nullptr);
		return;
	}
	
	pBot->wantToShoot(false);

	if ( m_pPipeLauncher->getAmmo(pBot) + m_pPipeLauncher->getClip1(pBot) == 0 )
	{
		if ( pBot->isVisible(m_pEnemy.get()) )
			static_cast<CBotTF2*>(pBot)->detonateStickies(true);

		complete();
		pBot->setLastEnemy(nullptr);
		return;
	}
	
	if ( pBot->getCurrentWeapon() != m_pPipeLauncher )
	{
		pBot->selectBotWeapon(m_pPipeLauncher);
		pBot->setLastEnemy(nullptr);
		return;
	}

	pBot->wantToChangeWeapon(false);

	if ( m_pPipeLauncher->getClip1(pBot) == 0 )
	{
		if ( randomInt(0,1) )
			pBot->reload();
	}
	else if ( pBot->distanceFrom(m_vStand) > 100.0f )
		pBot->setMoveTo(m_vStand);
	else
	{
		pBot->setLookAtTask(LOOK_VECTOR);
		pBot->setLookVector(m_vAim);
		pBot->stopMoving();

		if ( pBot->DotProductFromOrigin(m_vAim) > 0.99f )
		{
			const float fTime = engine->Time();

			if ( m_fHeldAttackTime == 0.0f )
				m_fHeldAttackTime = fTime + m_fHoldAttackTime + randomFloat(0.0f,0.15f);

			if ( m_fHeldAttackTime > fTime)
				pBot->primaryAttack(true);
			else
			{
				if ( m_fHeldAttackTime < fTime - 0.1f )
					m_fHeldAttackTime = 0.0f;

				pBot->letGoOfButton(IN_ATTACK);
			}

			static_cast<CBotTF2*>(pBot)->setStickyTrapType(m_vEnemy,TF_TRAP_TYPE_ENEMY);
		}
	}
}


//////////////////////////////////////////
CBotTF2DemomanPipeTrap :: CBotTF2DemomanPipeTrap ( eDemoTrapType type, const Vector& vStand, const Vector& vLoc, const Vector& vSpread, bool bAutoDetonate, int wptarea )
{
	m_vPoint = vLoc;
	m_vLocation = vLoc;
	m_vSpread = vSpread;
	m_iState = 0;
	m_iStickies = 6;
	m_iTrapType = type;
	m_vStand = vStand;
	m_fTime = 0.0f;
	m_bAutoDetonate = bAutoDetonate;
	m_iWptArea = wptarea;
}
	
void CBotTF2DemomanPipeTrap :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	bool bFail = false;
	CBotTF2 *pTF2Bot = static_cast<CBotTF2*>(pBot);
	if (pBot->getWeapons()->getWeapon(CWeapons::getWeapon(TF2_WEAPON_PIPEBOMBS)) == nullptr)
	{
		// don't have the weapon
		fail();
		return;
	}
	pBot->wantToChangeWeapon(false);

	if ( pBot->getEnemy() && pBot->hasSomeConditions(CONDITION_SEE_CUR_ENEMY) )
	{
		if ( CTeamFortress2Mod::hasRoundStarted() )
		{
			pBot->secondaryAttack();
			fail();
		}
	}
	
	if ( pTF2Bot->deployStickies(m_iTrapType,m_vStand,m_vLocation,m_vSpread,&m_vPoint,&m_iState,&m_iStickies,&bFail,&m_fTime,m_iWptArea) )
	{
		complete();

		if ( m_bAutoDetonate )
			pBot->secondaryAttack();
	}

	if ( bFail )
		fail();
}
/////////

CMessAround::CMessAround ( edict_t *pFriendly, int iMaxVoiceCmd )
{
	m_fTime = 0.0f;
	m_pFriendly = pFriendly;
	m_iType = randomInt(0,3);
	m_iMaxVoiceCmd = iMaxVoiceCmd;
}

void CMessAround::execute ( CBot *pBot, CBotSchedule *pSchedule )
{
	if ( !m_pFriendly || !CBotGlobals::entityIsValid(m_pFriendly) )
	{
		fail();
		return;
	}

	// smack the friendly player with my melee attack
	switch ( m_iType )
	{
	case 0:
		{
			const Vector origin = CBotGlobals::entityOrigin(m_pFriendly);

		pBot->setLookVector(origin);
		pBot->setLookAtTask(LOOK_VECTOR);


		if ( pBot->distanceFrom(m_pFriendly) > 100 )
		{
			pBot->setMoveTo(origin);
		}
		else if ( pBot->FInViewCone(m_pFriendly) )
		{
			CBotWeapon *pWeapon = pBot->getBestWeapon(nullptr,true,true);

			if ( pWeapon )
			{
				pBot->selectBotWeapon(pWeapon);

				if ( randomInt(0,1) )
					pBot->primaryAttack();
			}
		}

		if ( !m_fTime )
			m_fTime = engine->Time() + randomFloat(3.5f,8.0f);
	}
	break;// taunt at my friendly player
	case 1:
	{
		const Vector origin = CBotGlobals::entityOrigin(m_pFriendly);
		bool ok = true;

		pBot->setLookVector(origin);
		pBot->setLookAtTask(LOOK_VECTOR);

		if ( !pBot->FInViewCone(m_pFriendly) )
		{
			ok = false;
		}

		if ( pBot->distanceFrom(m_pFriendly) > 100 )
		{
			pBot->setMoveTo(origin);
			ok = false;
		}

		if ( ok )
		{
			if ( pBot->isTF2() )
				static_cast<CBotTF2*>(pBot)->taunt(true);
			//else if ( pBot->isDOD() )
			//	((CDODBot*)pBot)->taunt(); pBot->impulse(100);
		}

		if ( !m_fTime )
			m_fTime = engine->Time() + randomFloat(3.5f,6.5f);

	}
	// say some random voice commands
	break;
	case 2:
	{
		if ( !m_fTime )
			pBot->addVoiceCommand(randomInt(0,m_iMaxVoiceCmd-1));

		if ( !m_fTime )
			m_fTime = engine->Time() + randomFloat(1.5f,3.0f);
	}
	// press some random buttons, such as attack2, jump
	break;
	case 3:
	{
		if ( randomInt(0,1) )
			pBot->jump();
		else
		{
			if ( pBot->isTF2() )
			{
				if ( static_cast<CBotTF2*>(pBot)->getClass() == TF_CLASS_HWGUY )
					pBot->secondaryAttack(true);
			}
		}

		if ( !m_fTime )
			m_fTime = engine->Time() + randomFloat(1.5f,3.0f);
	}
	default:
		break;
	}

	if ( m_fTime < engine->Time() )
		complete();

	if ( pBot->isTF2() )
	{
		if ( CTeamFortress2Mod::hasRoundStarted() )
			complete();
	}
	else if ( pBot->isDOD() )
	{
		if ( CDODMod::m_Flags.getNumFlags() > 0 )
			complete();
	}

}

///////////
//defensive technique

CBotTF2Spam :: CBotTF2Spam ( CBot *pBot, const Vector& vStart, int iYaw, CBotWeapon *pWeapon )
{
	Vector forward;
	const QAngle angle = QAngle(0,iYaw,0);

	AngleVectors(angle,&forward);
	m_vTarget = vStart + forward*2000.0f;
	CBotGlobals::quickTraceline(pBot->getEdict(),vStart,m_vTarget);
	m_vTarget = CBotGlobals::getTraceResult()->endpos-forward;
	m_pWeapon = pWeapon;
	m_vStart = vStart;
	m_fNextAttack = 0.0f;
	m_fTime = 0.0f;
}

CBotTF2Spam :: CBotTF2Spam (const Vector& vStart, const Vector& vTarget, CBotWeapon *pWeapon )
{
	m_vTarget = vTarget;
	m_pWeapon = pWeapon;
	m_vStart = vStart;
	m_fNextAttack = 0.0f;
	m_fTime = 0.0f;
}

float CBotTF2Spam :: getDistance () const
{
	return (m_vStart - m_vTarget).Length();
}

void CBotTF2Spam :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	pBot->wantToShoot(false);
	pBot->wantToListen(false);

	if( CTeamFortress2Mod::TF2_IsPlayerInvuln(pBot->getEdict()) )
	{
		fail();
		return;
	}

	if ( pBot->recentlyHurt(1.0f) )
	{
		// taking damage
		fail();
		pBot->updateUtilTime(pBot->getCurrentUtil());
		pBot->wantToShoot(true);
		return;
	}

	if ( m_fTime == 0.0f )
	{
		if ( m_pWeapon->getID() == TF2_WEAPON_GRENADELAUNCHER ) // Triggers crash [APG]RoboCop[CL]
		{
			//Stability fix? [APG]RoboCop[CL]
			if (pSchedule == nullptr) {
				// Handle the error
				return;
			}
			const Vector vVisibleWaypoint = pSchedule->passedVector();

			if ( vVisibleWaypoint.z > m_vTarget.z + 32.0f ) // need to lob grenade
				m_vTarget.z += getGrenadeZ(pBot->getEdict(), nullptr,pBot->getOrigin(),m_vTarget,m_pWeapon->getProjectileSpeed());
			else // mid point between waypoint and target as I won't see target
				m_vTarget = (m_vTarget + vVisibleWaypoint) / 2;
		}

		m_fNextAttack = engine->Time() + randomFloat(0.2f,1.0f);
		m_fTime = engine->Time() + randomFloat(12.0f,24.0f);
	}
	else if ( m_fTime < engine->Time() )
	{
		complete();

		pBot->setLastEnemy(nullptr);

		// prevent bot from keeping doing this
		pBot->updateUtilTime(BOT_UTIL_SPAM_NEAREST_SENTRY);
		pBot->updateUtilTime(BOT_UTIL_SPAM_LAST_ENEMY);
		pBot->updateUtilTime(BOT_UTIL_SPAM_LAST_ENEMY_SENTRY);
	}

	if ( pBot->distanceFrom(m_vStart) > 100 )
		pBot->setMoveTo(m_vStart);
	else
		pBot->stopMoving();

	if ( pBot->getCurrentWeapon() != m_pWeapon )
	{
		if ( !pBot->selectBotWeapon(m_pWeapon) )
			fail();
	}
		
	pBot->setLookVector(m_vTarget);
	pBot->setLookAtTask(LOOK_VECTOR);

	if ( m_pWeapon->outOfAmmo(pBot) )
	{
		pBot->setLastEnemy(nullptr);
		complete();
	}
	else if ( m_pWeapon->needToReload(pBot) )
	{
		if ( randomInt(0,1) )
			pBot->reload();
	}
	else if ( m_fNextAttack < engine->Time() )
	{
		pBot->primaryAttack();
		m_fNextAttack = engine->Time() + randomFloat(0.2f,1.0f); 
	}
}

///////////

CBotTF2AttackSentryGunTask::CBotTF2AttackSentryGunTask ( edict_t *pSentryGun, CBotWeapon *pWeapon )
{
	m_iStartingWaypoint = 0;
	m_iSentryWaypoint = 0;
	m_fDist = 0.0f;
	m_fTime = 0.0f;
	m_pSentryGun = pSentryGun;
	m_pWeapon = pWeapon;
}

void CBotTF2AttackSentryGunTask::execute (CBot *pBot,CBotSchedule *pSchedule)
{
	pBot->wantToListen(false);
	pBot->wantToInvestigateSound(false);

	if ( CTeamFortress2Mod::TF2_IsPlayerInvuln(pBot->getEdict()) )
		fail();

	if ( m_pSentryGun.get() == nullptr)
	{
		fail();
		return;
	}

	if ( m_fTime == 0.0f )
	{
		const CWaypointVisibilityTable *table = CWaypoints::getVisiblity();

		m_fTime = engine->Time() + randomFloat(8.0f,14.0f);

		m_iSentryWaypoint = CWaypointLocations::NearestWaypoint(CBotGlobals::entityOrigin(m_pSentryGun),200.0f,-1);

		m_iStartingWaypoint = CWaypointLocations::NearestWaypoint(pBot->getOrigin(),200.0f,-1,true,true,true, nullptr);

		m_vStart = pBot->getOrigin();
		m_vHide = m_vStart;

		const CWaypoint *pWpt = CWaypoints::getWaypoint(m_iStartingWaypoint);

		if ( pWpt != nullptr)
		{
			float fMinDist = 9999;
			float fDist;
			for ( int i = 0; i < pWpt->numPaths(); i ++ )
			{
				if ( table->GetVisibilityFromTo(pWpt->getPath(i),m_iSentryWaypoint) == false )
				{
					CWaypoint *pPath = CWaypoints::getWaypoint(pWpt->getPath(i));

					if ( (fDist = pPath->distanceFrom(m_vStart)) < fMinDist )
					{
						fMinDist = fDist;
						m_vHide = pPath->getOrigin();
					}
				}

				pWpt->getPath(i);
			}

		}

		// hide waypoint = 
	}
	else if ( m_fTime < engine->Time() )
	{
		complete();
	}

	if ( pBot->getCurrentWeapon() != m_pWeapon )
	{
		pBot->selectBotWeapon(m_pWeapon);
		return;
	}

	pBot->wantToChangeWeapon(false);

	if ( m_pWeapon->outOfAmmo(pBot) )
		complete();

	pBot->lookAtEdict(m_pSentryGun);
	pBot->setLookAtTask(LOOK_EDICT);

	if ( m_pWeapon->needToReload(pBot) || CClassInterface::getSentryEnemy(m_pSentryGun) == pBot->getEdict() )
	{
		if ( pBot->distanceFrom(m_vHide) > 80.0f ) 
			pBot->setMoveTo(m_vHide);
		else
		{
			if ( m_pWeapon->needToReload(pBot) )
			{
				if ( randomInt(0,1) ) 
					pBot->reload();
			}
			pBot->stopMoving();
		}
	}
	else if ( pBot->distanceFrom(m_vStart) > 80.0f )
		pBot->setMoveTo(m_vStart);
	else
		pBot->stopMoving();

	if ( pBot->isVisible(m_pSentryGun) )
	{
		// use this shooting method below
		pBot->wantToShoot(false);

		CBotTF2 *pTF2Bot = static_cast<CBotTF2*>(pBot);
		pTF2Bot->resetAttackingEnemy();
		// attack
		pBot->handleAttack(m_pWeapon,m_pSentryGun);
	}
}

void CBotTF2AttackSentryGunTask::debugString ( char *string )
{
	std::sprintf(string,"CBotTF2AttackSentryGunTask");
}

/////////////

void CBotNest :: execute (CBot *pBot, CBotSchedule *pSchedule)
{
	CBotTF2 *pBotTF2 = static_cast<CBotTF2*>(pBot);

	if ( pBotTF2->getClass() == TF_CLASS_MEDIC && pBotTF2->someoneCalledMedic() )
		fail(); // Follow player

	if ( !pBotTF2->wantToNest() )
	{
		complete();
		pBotTF2->addVoiceCommand(TF_VC_GOGOGO);
		return;
	}
	if ( pBot->hasSomeConditions(CONDITION_PUSH) )
	{
		complete();
		pBot->removeCondition(CONDITION_PUSH);
		pBotTF2->addVoiceCommand(TF_VC_GOGOGO);
		return;
	}

	if ( m_fTime == 0.0f )
	{
		m_fTime = engine->Time() + randomFloat(6.0f,12.0f);

		if ( randomInt(0,1) )
			pBotTF2->addVoiceCommand(TF_VC_HELP);
	}
	else if ( m_fTime < engine->Time() )
	{
		complete();
		pBotTF2->addVoiceCommand(TF_VC_GOGOGO);
	}

	// wait around
	// wait for more friendlies
	// heal up
	// 

	pBot->setLookAtTask(LOOK_AROUND);

	pBot->stopMoving();
}

CBotNest::CBotNest()
{
	m_fTime = 0.0f;
	m_pEnemy = nullptr;
}
////////////////////////////////////////////////



/////////////////////////////////////////////

void CBotJoinSquad:: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	if ( !pBot->inSquad()  )
	{
		pBot->setSquad(CBotSquads::AddSquadMember(m_pPlayer,pBot->getEdict()));
		complete();
	}
		//CBotSquads::SquadJoin(pBot->getEdict(),m_pPlayer);
}

void CBotFollowSquadLeader :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	if ( !pBot->inSquad(m_pSquad) )
	{
		fail();
		return;
	}

	edict_t* pSquadLeader = m_pSquad->GetLeader();

	if ( pSquadLeader == pBot->getEdict() )
	{
		fail();
		return;
	}

	if ( CClassInterface::getMoveType(pSquadLeader) == MOVETYPE_LADDER )
	{
		pBot->stopMoving();
		return;
	}

	if ( m_fVisibleTime > 0.0f && !pBot->hasEnemy() && !pBot->isVisible(pSquadLeader) )
	{
		// haven't seen leader for five seconds
		if ( engine->Time() - m_fVisibleTime > 0.2f )
		{
			complete();
			return;
		}
	}
	else
		m_fVisibleTime = engine->Time();
	
	if ( pBot->hasSomeConditions(CONDITION_SQUAD_IDLE) )
	{
		// squad leader idle for 3 seconds. see what else i can do
		complete();
		return;
	}

	if ( m_fUpdateMovePosTime < engine->Time() )
	{
		Vector vVelocity;

		const float fRand = randomFloat(1.0f, 2.0f);

		CClassInterface::getVelocity(pSquadLeader,&vVelocity);

		m_fLeaderSpeed = MIN(m_fLeaderSpeed,100.0f)*0.5f + vVelocity.Length()*0.5f;

		m_fUpdateMovePosTime = engine->Time() + fRand * (1.0f-vVelocity.Length()/320);
		m_vPos = m_pSquad->GetFormationVector(pBot->getEdict());
		
		m_vForward = m_vPos+vVelocity;
	}

	const float fDist = pBot->distanceFrom(m_vPos); // More than reachable range

	if ( fDist > 400.0f )
	{
		fail(); // find a path instead
		return;
	}
	if ( fDist > m_pSquad->GetSpread() )
	{
		pBot->setMoveTo(m_vPos);
		pBot->setSquadIdleTime(engine->Time());

		if ( pBot->isVisible(pSquadLeader) )
			pBot->setMoveSpeed(m_fLeaderSpeed);
	}
	else
	{
		// idle
		pBot->stopMoving();
		pBot->SquadInPosition();
	}

	pBot->setLookVector(m_vForward);
	pBot->setLookAtTask(LOOK_VECTOR);
}
////////////////////////////////////////////////////

CBotDODSnipe :: CBotDODSnipe ( CBotWeapon *pWeaponToUse, const Vector& vOrigin, float fYaw, bool bUseZ, float z, int iWaypointType )
{
	m_fEnemyTime = 0.0f;
	m_fTime = 0.0f;
	const QAngle angle = QAngle(0, fYaw, 0);
	AngleVectors(angle,&m_vAim);
	m_vAim = vOrigin + m_vAim*1024;
	m_vOrigin = vOrigin;
	m_pWeaponToUse = pWeaponToUse;
	m_fScopeTime = 0.0f;
	m_bUseZ = bUseZ;
	m_z = z; // z = ground level
	m_iWaypointType = iWaypointType;
	m_fTimeout = 0.0f;
}

void CBotDODSnipe :: debugString ( char *string )
{
	std::sprintf(string, "CBotDODSnipe\nm_fTime = %0.2f\npWeaponToUse = %s\nm_bUseZ = %s\nm_z = %0.2f", m_fTime,
			m_pWeaponToUse->getWeaponInfo()->getWeaponName(), m_bUseZ ? "true" : "false", m_z);
}
	
void CBotDODSnipe :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	static CBotWeapon *pCurrentWeapon;
	static CWeapon *pWeapon;

	static bool bDeployedOrZoomed;
	static float fDist;

	bDeployedOrZoomed = false;

	pBot->wantToShoot(false);
	pBot->wantToListen(false);

	if ( m_fTime == 0.0f )
	{
		m_fEnemyTime = 0.0f;
		m_fTime = engine->Time() + randomFloat(20.0f,40.0f);
		pBot->resetLookAroundTime();
	}

	pCurrentWeapon = pBot->getCurrentWeapon();

	if ( !pCurrentWeapon )
	{
		fail();
		return;
	}

	pWeapon = pCurrentWeapon->getWeaponInfo();

	if (pWeapon == nullptr)
	{
		fail();
		return;
	}

	if ( pCurrentWeapon != m_pWeaponToUse )
	{
		if ( !pBot->select_CWeapon(CWeapons::getWeapon(m_pWeaponToUse->getID())) )//TODO: Needs stability fixes? [APG]RoboCop[CL]
		{
			fail();
		}

		return;
	}
	if ( pCurrentWeapon->isZoomable() )
		bDeployedOrZoomed = CClassInterface::isSniperWeaponZoomed(pCurrentWeapon->getWeaponEntity());
	else if ( pCurrentWeapon->isDeployable() )
		bDeployedOrZoomed = CClassInterface::isMachineGunDeployed(pCurrentWeapon->getWeaponEntity());

	if ( m_fScopeTime < engine->Time() )
	{
		if ( !bDeployedOrZoomed )
		{
			pBot->secondaryAttack();
				
			if ( m_fTimeout == 0.0f )
				m_fTimeout = engine->Time();
			else if ( m_fTimeout + 3.0f < engine->Time() )
				fail();
		}
		else
			m_fTimeout = 0.0f;

		m_fScopeTime = engine->Time() + randomFloat(0.5f,1.0f);
	}

	if ( pCurrentWeapon->getAmmo(pBot) < 1 )
	{
		if ( bDeployedOrZoomed )
			pBot->secondaryAttack();

		complete();
	}
	else if ( pBot->distanceFrom(m_vOrigin) > 200 ) // too far from sniper point
	{
		if ( bDeployedOrZoomed )
			pBot->secondaryAttack();
		// too far away
		fail();
	}

	if ( m_fEnemyTime + 5.0f > engine->Time() )
	{
		pBot->setLookAtTask(LOOK_VECTOR);
		pBot->setLookVector(m_vLastEnemy);
	}
	else if ( m_bUseZ )
	{
		const Vector vAim = Vector(m_vAim.x,m_vAim.y,m_z);
		pBot->setLookAtTask(LOOK_VECTOR);
		pBot->setLookVector(pBot->snipe(vAim));
	}
	else
	{
		pBot->setLookAtTask(LOOK_VECTOR);
		pBot->setLookVector(pBot->snipe(m_vAim));
	}

	fDist = (m_vOrigin - pBot->getOrigin()).Length2D();

	if ( fDist > 16 || !bDeployedOrZoomed )
	{
		pBot->setMoveTo(m_vOrigin);
		pBot->setMoveSpeed(CClassInterface::getMaxSpeed(pBot->getEdict())/8);

		if ( fDist < 48 && static_cast<CDODBot*>(pBot)->withinTeammate() )
			fail();
	}
	else
	{

		bool unprone = false;

		pBot->stopMoving();

		if ( m_iWaypointType & CWaypointTypes::W_FL_PRONE )
		{			
			//pBot->updateDanger(MAX_BELIEF);
			pBot->removeCondition(CONDITION_RUN);
			pBot->updateCondition(CONDITION_PRONE);

		}
		else
		{
			if ( m_iWaypointType & CWaypointTypes::W_FL_CROUCH )
				pBot->duck();
			// refrain from proning
			pBot->updateCondition(CONDITION_RUN);
			pBot->removeCondition(CONDITION_PRONE);
			unprone = true;
			
		}

		if ( unprone )
		{
			CClassInterface::getPlayerInfoDOD(pBot->getEdict(),&unprone, nullptr);
			if ( unprone )
			{
				CDODBot *pDODBot = static_cast<CDODBot*>(pBot);
				pDODBot->unProne();
			}
		}

		// no enemy for a while
		if ( m_fEnemyTime + m_fTime < engine->Time() )
		{
			if ( bDeployedOrZoomed )
				pBot->secondaryAttack();

			complete();
		}
	}

	if ( pBot->hasEnemy() )
	{
		pBot->setMoveLookPriority(MOVELOOK_ATTACK);

		pBot->setLookAtTask(LOOK_ENEMY);

		pBot->handleAttack(pCurrentWeapon,pBot->getEnemy());

		pBot->setMoveLookPriority(MOVELOOK_TASK);

		// havin' fun
		m_fEnemyTime = engine->Time();

		m_vLastEnemy = CBotGlobals::entityOrigin(pBot->getEnemy());
	}
}

//////////////////////////

CBotHL2DMSnipe :: CBotHL2DMSnipe ( CBotWeapon *pWeaponToUse, const Vector& vOrigin, float fYaw, bool bUseZ, float z, int iWaypointType )
{
	m_fEnemyTime = 0.0f;
	m_fTime = 0.0f;
	const QAngle angle = QAngle(0, fYaw, 0);
	AngleVectors(angle,&m_vAim);
	m_vAim = vOrigin + m_vAim*1024;
	m_vOrigin = vOrigin;
	m_pWeaponToUse = pWeaponToUse;
	m_fScopeTime = 0;
	m_bUseZ = bUseZ;
	m_z = z; // z = ground level
	m_iWaypointType = iWaypointType;
}

void CBotHL2DMSnipe::debugString(char* string) //Unstable? [APG]RoboCop[CL]
{
	std::sprintf(string, "CBotHL2DMSnipe\nm_fTime = %0.2f\npWeaponToUse = %s\nm_bUseZ = %s\nm_z = %0.2f", m_fTime,
			m_pWeaponToUse->getWeaponInfo()->getWeaponName(), m_bUseZ ? "true" : "false", m_z);
}

void CBotHL2DMSnipe :: execute (CBot *pBot,CBotSchedule *pSchedule)
{
	static CBotWeapon *pCurrentWeapon;
	static CWeapon *pWeapon;

	static bool bDeployedOrZoomed; //Unused? [APG]RoboCop[CL]
	static float fDist;

	bDeployedOrZoomed = false;

	pBot->wantToShoot(false);
	pBot->wantToListen(false);

	if ( m_fTime == 0.0f )
	{
		m_fEnemyTime = engine->Time();
		m_fTime = m_fEnemyTime + randomFloat(20.0f,40.0f);
		pBot->resetLookAroundTime();
	}

	pCurrentWeapon = pBot->getCurrentWeapon();

	if ( !pCurrentWeapon )
	{
		fail();
		return;
	}

	pWeapon = pCurrentWeapon->getWeaponInfo();

	if (pWeapon == nullptr)
	{
		fail();
		return;
	}

	// refrain from proning
	pBot->updateCondition(CONDITION_RUN);

	/*if (pCurrentWeapon != m_pWeaponToUse)
	{
		if ( !pBot->select_CWeapon(CWeapons::getWeapon(m_pWeaponToUse->getID())) ) //Unstable? [APG]RoboCop[CL]
		{
			fail();
		}

		return;
	}*/

	if ( pCurrentWeapon->getAmmo(pBot) < 1 )
	{
		complete();
	}
	else if ( pBot->distanceFrom(m_vOrigin) > 200 ) // too far from sniper point
	{
		// too far away
		fail();
	}

	if ( m_bUseZ )
	{
		const Vector vAim = Vector(m_vAim.x,m_vAim.y,m_z);
		pBot->setLookAtTask(LOOK_VECTOR);
		pBot->setLookVector(pBot->snipe(vAim));
	}
	else
	{
		pBot->setLookAtTask(LOOK_SNIPE);
		pBot->setLookVector(pBot->snipe(m_vAim));
	}

	fDist = (m_vOrigin - pBot->getOrigin()).Length2D();

	if ( fDist > 16 )
	{
		pBot->setMoveTo(m_vOrigin);
		pBot->setMoveSpeed(CClassInterface::getMaxSpeed(pBot->getEdict())/8);

		//if ( ( fDist < 48 ) && ((CDODBot*)pBot)->withinTeammate() )
		//	fail();
	}
	else
	{
		pBot->stopMoving();

		if ( m_iWaypointType & CWaypointTypes::W_FL_CROUCH )
			pBot->duck();

		// no enemy for a while
		if ( m_fEnemyTime + m_fTime < engine->Time() )
		{
			//if ( bDeployedOrZoomed )
			//	pBot->secondaryAttack();

			complete();
		}
	}

	if ( pBot->hasEnemy() )
	{
		pBot->setMoveLookPriority(MOVELOOK_ATTACK);
		pBot->setLookAtTask(LOOK_ENEMY);
		pBot->handleAttack(pCurrentWeapon,pBot->getEnemy());
		pBot->setMoveLookPriority(MOVELOOK_TASK);

		// havin' fun
		m_fEnemyTime = engine->Time();
	}
}

void CBotSynDisarmMineTask::execute(CBot *pBot,CBotSchedule *pSchedule)
{
	if(m_pMine.get() == nullptr)
	{
		fail();
		return;
	}

	//if(CSynergyMod::IsCombineMinePlayerPlaced(m_pMine.get()))
	//{
	//	fail();
	//}

	m_vMinePos = CBotGlobals::entityOrigin(m_pMine.get());
	m_fDist = pBot->distanceFrom(m_pMine.get());

	pBot->wantToChangeWeapon(false);
	pBot->wantToShoot(false);
	pBot->wantToListen(false);
	pBot->setMoveLookPriority(MOVELOOK_OVERRIDE);
	pBot->setLookAtTask(LOOK_VECTOR);
	pBot->setLookVector(m_vMinePos);
	pBot->setMoveLookPriority(MOVELOOK_TASK);

	if(pBot->getCurrentWeapon() != pBot->getWeapons()->getWeapon(CWeapons::getWeapon(SYN_WEAPON_PHYSCANNON)))
	{
		pBot->selectBotWeapon(pBot->getWeapons()->getWeapon(CWeapons::getWeapon(SYN_WEAPON_PHYSCANNON)));
	}

	if(m_fDist <= 400.0f)
	{
		pBot->duck(true);
	}
	
	if(m_fDist <= 250.0f)
	{
		pBot->stopMoving();
		
		if(!m_bTimeSet && CSynergyMod::IsCombineMineHeldByPhysgun(m_pMine.get()))
		{
			m_ftime = engine->Time() + 2.0f;
			m_bTimeSet = true;
		}

		if(m_bTimeSet && m_ftime < engine->Time())
		{
			pBot->letGoOfButton(IN_ATTACK2);
			pBot->tapButton(IN_ATTACK2); // Press M2 again to release
			pBot->letGoOfButton(IN_DUCK);

			if(CClassInterface::gravityGunObject(CClassInterface::getCurrentWeapon(pBot->getEdict())) == nullptr)
			{
				pBot->jump(); // Sometimes the mine blocks the bot's path
				complete();
			}
		}
		else
		{
			pBot->secondaryAttack(true);
		}
	}
	else
	{
		pBot->setMoveTo(m_vMinePos);
	}
}

void CBotSynDisarmMineTask::debugString(char *string)
{
	std::sprintf(string, "CBotSynDisarmMineTask\nPos: (%0.4f,%0.4f,%0.4f)\nDistance: %.2f\nTime Set: %s", m_vMinePos.x,
			m_vMinePos.y, m_vMinePos.z, m_fDist, m_bTimeSet ? "true" : "false");
}

void CBotSynBreakICrateTask::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	if(m_pCrate.get() == nullptr)
	{
		complete();
		return;
	}

	m_vPos = CBotGlobals::entityOrigin(m_pCrate.get());

	if(pBot->distanceFrom(m_vPos) >= 256.0f)
	{
		fail();
		return;
	}

	if(m_pWeapon)
	{
		if(pBot->getCurrentWeapon() != m_pWeapon)
		{
			pBot->selectBotWeapon(m_pWeapon);
		}
	}

	pBot->wantToChangeWeapon(false);
	pBot->wantToListen(false);
	pBot->wantToShoot(false);
	pBot->setMoveLookPriority(MOVELOOK_OVERRIDE);
	pBot->setLookAtTask(LOOK_VECTOR);
	pBot->setLookVector(m_vPos);
	pBot->setMoveLookPriority(MOVELOOK_TASK);

	// Attack
	const CBotWeapon* currentWeapon = pBot->getCurrentWeapon();

	if (currentWeapon && currentWeapon->isGravGun())
	{
		if (CClassInterface::gravityGunObject(CClassInterface::getCurrentWeapon(pBot->getEdict())) != nullptr)
		{
			pBot->primaryAttack();
		}
		else
		{
			pBot->secondaryAttack(true);
		}
	}
	else
	{
		pBot->setMoveTo(m_vPos);
		pBot->primaryAttack();

		if (!currentWeapon->isMelee())
		{
			if (currentWeapon->getClip1(pBot) < 1)
			{
				if (currentWeapon->getAmmo(pBot) < 1)
				{
					m_pWeapon = pBot->getBestWeapon(m_pCrate.get());
				}
				else
				{
					pBot->tapButton(IN_RELOAD);
				}
			}
		}
	}
}

void CBotSynUseCharger::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	if(m_pCharger == NULL)
	{
		fail();
		return;
	}

	m_vPos = CBotGlobals::entityOrigin(m_pCharger);

	if(m_flTime <= engine->Time())
		complete();

	if(CClassInterface::getAnimCycle(m_pCharger) == 1.0f)
		complete();

	if(m_iType == CHARGER_HEALTH && pBot->getHealthPercent() >= 0.99f)
		complete();
	else if(m_iType == CHARGER_ARMOR && static_cast<CBotSynergy*>(pBot)->getArmorPercent() >= 0.99f)
		complete();

	pBot->setMoveLookPriority(MOVELOOK_OVERRIDE);
	pBot->setLookVector(m_vPos);
	pBot->setLookAtTask(LOOK_VECTOR);
	pBot->setMoveLookPriority(MOVELOOK_TASK);

	if(pBot->distanceFrom(m_pCharger) > 96.0f)
	{
		pBot->setMoveTo(m_vPos);
	}
	else
	{
		pBot->stopMoving();
	}

	if(pBot->isFacing(m_vPos))
	{
		pBot->use();
	}
}

///////////////////////////////////////////
// interrupts

CBotTF2EngineerInterrupt :: CBotTF2EngineerInterrupt( CBot *pBot )
{
	pWrench = nullptr;
	m_pSentryGun = CTeamFortress2Mod::getMySentryGun(pBot->getEdict());

	if ( m_pSentryGun.get() != nullptr)
	{
		m_fPrevSentryHealth = CClassInterface::getSentryHealth(m_pSentryGun);
	}
	else
		m_fPrevSentryHealth = 0.0f;
}

bool CBotTF2CoverInterrupt::isInterrupted ( CBot *pBot, bool *bFailed, bool *bCompleted )
{
	if ( CTeamFortress2Mod::TF2_IsPlayerInvuln(pBot->getEdict()) )
	{
		*bFailed = true;
		return true;
	}

	return false;
}

CBotTF2HurtInterrupt :: CBotTF2HurtInterrupt ( CBot *pBot )
{
	m_iHealth = pBot->getHealthPercent();
}

bool CBotTF2HurtInterrupt::isInterrupted ( CBot *pBot, bool *bFailed, bool *bCompleted )
{
	if ( m_iHealth > pBot->getHealthPercent() )
	{
		*bFailed = true;
		pBot->secondaryAttack();
		return true;
	}

	return false;
}

bool CBotTF2EngineerInterrupt :: isInterrupted ( CBot *pBot, bool *bFailed, bool *bCompleted )
{
	if ( m_pSentryGun.get() != nullptr)
	{
		if ( !CClassInterface::getTF2BuildingIsMini(m_pSentryGun) )
		{
			const float m_fCurrentHealth = CClassInterface::getSentryHealth(m_pSentryGun);

			if ( static_cast<CBotFortress*>(pBot)->getMetal()>75 && m_fCurrentHealth < m_fPrevSentryHealth )
			{
				*bFailed = true;
				return true;
			}

			m_fPrevSentryHealth = m_fCurrentHealth;
		}
	}

	return false;
}

/**
 * CSS: Task interruption for when the bot is walking around
 **/
bool CBotCSSRoamInterrupt::isInterrupted(CBot *pBot, bool *bFailed, bool *bCompleted)
{
	if(pBot->hasEnemy())
		return true;

	if(CCounterStrikeSourceMod::isMapType(CS_MAP_BOMBDEFUSAL))
	{
		if(pBot->getTeam() == CS_TEAM_TERRORIST && CCounterStrikeSourceMod::isBombDropped())
		{
			return true;
		}
		if(pBot->getTeam() == CS_TEAM_COUNTERTERRORIST && CCounterStrikeSourceMod::isBombPlanted())
		{
			return true;
		}
	}

	return false;
}

// Task interruption for synergy's roaming tasks
bool CBotSYNRoamInterrupt::isInterrupted(CBot *pBot, bool *bFailed, bool *bCompleted)
{
	CBotSynergy *pBotSyn = static_cast<CBotSynergy*>(pBot);

	// Can be interrupted?
	if(pBotSyn->getInterruptionTimer() <= engine->Time())
	{
		if(pBotSyn->needHealth())
			return true;

		if(pBotSyn->needAmmo())
			return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////
// Base Task
CBotTask :: CBotTask ()
{
	_init();
}

bool CBotTask :: timedOut () const
{
	return this->m_fTimeOut != 0.0f && engine->Time() < this->m_fTimeOut;
}

eTaskState CBotTask :: isInterrupted (CBot *pBot)
{
	if ( m_pInterruptFunc != nullptr)
	{
		bool bFailed = false;
		bool bCompleted = false;

		if ( m_pInterruptFunc->isInterrupted(pBot,&bFailed,&bCompleted) )
		{
			if ( bFailed )
				return STATE_FAIL;
			
			return STATE_COMPLETE;
		}
	}

	if ( m_iCompleteInterruptConditionsHave )
	{
		if ( pBot->hasAllConditions(m_iCompleteInterruptConditionsHave) )
			return STATE_COMPLETE;
	}

	if ( m_iCompleteInterruptConditionsDontHave )
	{
		if ( !pBot->hasAllConditions(m_iCompleteInterruptConditionsDontHave) )
			return STATE_COMPLETE;
	}

	if ( m_iFailInterruptConditionsHave )
	{
		if ( pBot->hasAllConditions(m_iFailInterruptConditionsHave) )
			return STATE_FAIL;
	}

	if ( m_iFailInterruptConditionsDontHave )
	{
		if ( !pBot->hasAllConditions(m_iFailInterruptConditionsDontHave) )
			return STATE_FAIL;
	}

	return STATE_RUNNING;
}

void CBotTask :: _init()
{
	m_pInterruptFunc = nullptr;
	m_iFlags = 0;	
	m_iState = STATE_IDLE;
	m_fTimeOut = 0;
//	m_pEdict = NULL;
//	m_fFloat = 0;
//	m_iInt = 0;
//	m_vVector = Vector(0,0,0);
	m_iFailInterruptConditionsHave = 0;
	m_iFailInterruptConditionsDontHave = 0;
	m_iCompleteInterruptConditionsHave = 0;
	m_iCompleteInterruptConditionsDontHave = 0;
	init();
}

void CBotTask :: init ()
{
}

void CBotTask :: execute ( CBot *pBot, CBotSchedule *pSchedule )
{
}

bool CBotTask :: hasFailed () const
{
	return m_iState == STATE_FAIL;
}

bool CBotTask :: isComplete () const
{
	return m_iState == STATE_COMPLETE;
}
/*
void CBotTask :: setVector ( Vector vOrigin )
{
	m_vVector = vOrigin;
}

void CBotTask :: setFloat ( float fFloat )
{
	m_fFloat = fFloat;
}

void CBotTask :: setEdict ( edict_t *pEdict )
{
	m_pEdict = pEdict;
}
*/
// if this condition is true it will complete, if bUnset is true, the condition must be false to be complete
void CBotTask :: setCompleteInterrupt ( int iInterruptHave, int iInterruptDontHave )
{
	m_iCompleteInterruptConditionsHave = iInterruptHave;
	m_iCompleteInterruptConditionsDontHave = iInterruptDontHave;
}

void CBotTask :: setFailInterrupt ( int iInterruptHave, int iInterruptDontHave )
{
	m_iFailInterruptConditionsHave = iInterruptHave;
	m_iFailInterruptConditionsDontHave = iInterruptDontHave;
}

void CBotTask :: fail ()
{
	m_iState = STATE_FAIL;
}

void CBotTask :: complete ()
{
	m_iState = STATE_COMPLETE;
}
