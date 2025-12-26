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
#ifndef __RCBOT_SCHEDULE_H__
#define __RCBOT_SCHEDULE_H__

#include "bot.h"
#include "bot_task.h"
//#include "bot_fortress.h"

#include <algorithm>
#include <deque>

class CBotTask;
class CAttackEntityTask;
class IBotTaskInterrupt;

enum : std::uint8_t
{
	BITS_SCHED_PASS_INT = 1<<0,
	BITS_SCHED_PASS_FLOAT = 1<<1,
	BITS_SCHED_PASS_VECTOR = 1<<2,
	BITS_SCHED_PASS_EDICT = 1<<3
};

typedef enum : std::uint8_t
{
	SCHED_NONE = 0,
	SCHED_ATTACK,
	SCHED_RUN_FOR_COVER,
	SCHED_GOTO_ORIGIN,
	SCHED_GOOD_HIDE_SPOT,
	SCHED_TF2_GET_FLAG,
	SCHED_TF2_GET_HEALTH,
	SCHED_TF_BUILD,
	SCHED_HEAL,
	SCHED_GET_METAL,
	SCHED_SNIPE,
	SCHED_UPGRADE,
	SCHED_USE_TELE,
	SCHED_SPY_SAP_BUILDING,
	SCHED_USE_DISPENSER,
	SCHED_PICKUP,
	SCHED_TF2_GET_AMMO,
	SCHED_TF2_FIND_FLAG,
	SCHED_LOOKAFTERSENTRY,
	SCHED_DEFEND,
	SCHED_ATTACKPOINT,
	SCHED_DEFENDPOINT,
	SCHED_TF2_PUSH_PAYLOADBOMB,
	SCHED_TF2_DEFEND_PAYLOADBOMB,
	SCHED_TF2_DEMO_PIPETRAP,
	SCHED_TF2_DEMO_PIPEENEMY,
	SCHED_BACKSTAB,
	SCHED_REMOVESAPPER,
	SCHED_GOTONEST,
	SCHED_MESSAROUND,
	SCHED_TF2_ENGI_MOVE_BUILDING,
	SCHED_FOLLOW_LAST_ENEMY,
	SCHED_SHOOT_LAST_ENEMY_POS,
	SCHED_GRAVGUN_PICKUP,
	SCHED_HELP_PLAYER,
	SCHED_BOMB,
	SCHED_TF_SPYCHECK,
	SCHED_FOLLOW,
	SCHED_DOD_DROPAMMO,
	SCHED_INVESTIGATE_NOISE,
	SCHED_CROUCH_AND_HIDE,
	SCHED_DEPLOY_MACHINE_GUN,
	SCHED_ATTACK_SENTRY_GUN,
	SCHED_RETURN_TO_INTEL,
	SCHED_INVESTIGATE_HIDE,
	SCHED_TAUNT,
	SCHED_SYN_OPEN_DOOR,
	SCHED_SYN_DISARM_MINE,
	SCHED_SYN_PLANT_MINE,
	SCHED_SYN_BREAK_ICRATE,
	SCHED_BUY,
	SCHED_PLANT_BOMB,
	SCHED_WAIT,
	SCHED_WAIT_FOR_ENEMY,
	SCHED_NAVTEST_EXPLORE,		// Nav-test exploration mode
	SCHED_MAX
	//SCHED_HIDE_FROM_ENEMY
}eBotSchedule;

class CBotSchedule
{
public:
	CBotSchedule(CBotTask *pTask)
	{
		_init();

		addTask(pTask);
	}

	CBotSchedule();

	void _init ();

	virtual void init () {
	} // nothing, used by sub classes

	virtual ~CBotSchedule() = default; //TODO: experimental [APG]RoboCop[CL]

	void addTask( CBotTask *pTask );

	void execute ( CBot *pBot );

	const char *getIDString () const;

	CBotTask *currentTask () const
	{
		return m_Tasks.empty() ? nullptr : m_Tasks.front();
	}

	bool hasFailed () const
	{
		return m_bFailed;
	}

	bool isComplete () const
	{
		return m_Tasks.empty();
	}

	void freeMemory ()
	{
		for (const CBotTask* task : m_Tasks) {
			delete task;
		}
		m_Tasks.clear();
	}

	void removeTop ();

	//////////////////////////

	void clearPass () { m_bitsPass = 0; }

	void passInt(int i);
	void passFloat(float f);
	void passVector(const Vector& v);
	void passEdict(edict_t *p);
	//////////////////////////

	bool hasPassInfo () const { return m_bitsPass!=0; }

	int passedInt () const { return iPass; }
	float passedFloat() const { return fPass; }
	Vector passedVector() { return vPass; }
	edict_t *passedEdict() const { return pPass; }
	bool isID (const eBotSchedule iId) const { return m_iSchedId == iId; }

	bool hasPassInt () const { return (m_bitsPass&BITS_SCHED_PASS_INT)>0; }
	bool hasPassFloat () const { return (m_bitsPass&BITS_SCHED_PASS_FLOAT)>0; }
	bool hasPassVector () const { return (m_bitsPass&BITS_SCHED_PASS_VECTOR)>0; }
	bool hasPassEdict () const { return (m_bitsPass&BITS_SCHED_PASS_EDICT)>0; }

	void setID (const eBotSchedule iId) { m_iSchedId = iId; }
	eBotSchedule getID() const { return m_iSchedId; }

private:
	std::deque<CBotTask*> m_Tasks;
	bool m_bFailed;
	eBotSchedule m_iSchedId;
	
	// passed information to next task(s)
	int iPass;
	float fPass;
	Vector vPass;
	edict_t *pPass;

	int m_bitsPass;
};

class CBotSchedules
{
public:
	bool hasSchedule(eBotSchedule iSchedule) const
	{
		return std::any_of(m_Schedules.begin(), m_Schedules.end(),
			[iSchedule](const CBotSchedule* sched) {
				return sched->isID(iSchedule);
			});
	}

	bool isCurrentSchedule (const eBotSchedule iSchedule) const
	{
		if ( m_Schedules.empty() )
			return false;
		return m_Schedules.front()->isID(iSchedule);
	}

	// remove the first schedule in the queue matching this schedule identifier
	void removeSchedule (const eBotSchedule iSchedule)
	{
		for (std::deque<CBotSchedule*>::iterator it = m_Schedules.begin(); it != m_Schedules.end(); ) {
			if ((*it)->isID(iSchedule)) {
				m_Schedules.erase(it);
				return;
			}
			++it;
		}
	}

	void execute ( CBot *pBot )
	{
		if ( isEmpty() )
			return;

		CBotSchedule *pSched = m_Schedules.front();
		pSched->execute(pBot);

		if ( pSched->isComplete() || pSched->hasFailed() )
			removeTop();
	}

	void removeTop ()
	{
		CBotSchedule *pSched = m_Schedules.front();
		m_Schedules.pop_front();

		// TODO: eradicate freeMemory from the codebase
		pSched->freeMemory();

		delete pSched;
	}

	void freeMemory ()
	{
		for (const CBotSchedule *sched : m_Schedules) {
			delete sched;
		}
		m_Schedules.clear();
	}

	void add ( CBotSchedule *pSchedule )
	{
		// initialize
		pSchedule->init();
		// add
		m_Schedules.emplace_back(pSchedule);
	}

	void addFront ( CBotSchedule *pSchedule )
	{
		pSchedule->init();
		m_Schedules.push_front(pSchedule);
	}

	bool isEmpty () const
	{
		return m_Schedules.empty();
	}

	CBotTask *getCurrentTask () const
	{
		if ( !m_Schedules.empty() )
		{
			const CBotSchedule *sched = m_Schedules.front();

			if ( sched != nullptr)
			{
				return sched->currentTask();
			}
		}

		return nullptr;
	}

	CBotSchedule *getCurrentSchedule () const
	{
		if ( isEmpty() )
			return nullptr;

		return m_Schedules.front();
	}

private:
	std::deque<CBotSchedule*> m_Schedules;
};
///////////////////////////////////////////
class CBotTF2DemoPipeTrapSched : public CBotSchedule
{
public:
	CBotTF2DemoPipeTrapSched ( eDemoTrapType type, const Vector& vStand, const Vector& vLoc, const Vector& vSpread, bool bAutoDetonate = false, int wptarea = -1 );

	void init() override;
};

class CBotTF2DemoPipeEnemySched : public CBotSchedule
{
public:
	CBotTF2DemoPipeEnemySched ( CBotWeapon *pLauncher, Vector vStand, edict_t *pEnemy );

	void init() override;
};

///////////////////////////////////////////////
class CBotTF2ShootLastEnemyPos : public CBotSchedule
{
public:
	CBotTF2ShootLastEnemyPos (const Vector& vLastSeeEnemyPos, const Vector& vVelocity, edict_t *pLastEnemy );

	void init() override;
};

///////////////////////////////////////////////
class CBotTF2MessAroundSched : public CBotSchedule
{
public:
	CBotTF2MessAroundSched ( edict_t *pFriendly, int iMaxVoiceCmd );

	void init() override;
};

///////////////////////////////////////////////////
class CBotTauntSchedule : public CBotSchedule
{
public:
	CBotTauntSchedule ( edict_t *pPlayer, float fYaw );

	void init () override;
private:
	MyEHandle m_pPlayer;
	float m_fYaw;
};
////////////////////////////////////////////
class CBotUseTeleSched : public CBotSchedule
{
public:
	CBotUseTeleSched ( edict_t *pTele );

	void init () override;
};
///////////////////////////////////////////////////
class CBotEngiMoveBuilding : public CBotSchedule
{
public:
	CBotEngiMoveBuilding ( edict_t *pBotEdict, edict_t *pBuilding, eEngiBuild iObject, const Vector& vNewLocation, bool bCarrying );

	void init () override;
};

/////////////////////////////////////////////////
class CBotAttackPointSched : public CBotSchedule
{
public:
	CBotAttackPointSched (const Vector& vPoint, float fRadius, int iArea, bool bHasRoute = false, const Vector& vRoute = Vector(0,0,0), bool bNest = false, edict_t *pLastEnemySentry = nullptr);

	void init () override;
}; 
//////////////////////////////////////////////////
class CBotDefendPointSched : public CBotSchedule
{
public:
	CBotDefendPointSched (const Vector& vPoint, float fRadius, int iArea);

	void init () override;
}; 
///////////////////////////////////////////////
class CBotTF2PushPayloadBombSched : public CBotSchedule
{
public:
	CBotTF2PushPayloadBombSched (edict_t * ePayloadBomb);

	void init () override;
}; 
///////////////////////////////////////////////
class CBotTF2DefendPayloadBombSched : public CBotSchedule
{
public:
	CBotTF2DefendPayloadBombSched (edict_t * ePayloadBomb);

	void init () override;
}; 
///////////////////////////////////////////////////
class CBotSpySapBuildingSched : public CBotSchedule
{
public:
	CBotSpySapBuildingSched ( edict_t *pBuilding, eEngiBuild id );

	void init () override;
};
////////////////////////////////////////////////
class CBotUseDispSched : public CBotSchedule
{
public:
	CBotUseDispSched (const CBot *pBot, edict_t *pDisp );

	void init () override;
}; 
///////////////////////////////////////////////////

class CBotTFEngiUpgrade : public CBotSchedule
{
public:
	CBotTFEngiUpgrade (const CBot *pBot, edict_t *pBuilding );

	void init () override;
};
//////////////////////////////////////////////////////
class CBotTFEngiMoveBuilding : public CBotSchedule
{
public:
	CBotTFEngiMoveBuilding ( edict_t *pBuilding, Vector vNewOrigin );

	void init () override;
};
///////////////////////////////////////////////////
class CBotTFEngiBuild : public CBotSchedule
{
public:
	CBotTFEngiBuild (const CBot *pBot, eEngiBuild iObject, CWaypoint *pWaypoint );

	void init () override;
};

class CBotRemoveSapperSched : public CBotSchedule
{
public:
	CBotRemoveSapperSched ( edict_t *pBuilding, eEngiBuild id );

	void init() override;
};

class CBotTF2HealSched : public CBotSchedule
{
public:
	CBotTF2HealSched(edict_t *pHeal);
	void init () override;
};

class CBotDefendSched : public CBotSchedule
{
public:
	CBotDefendSched (const Vector& vOrigin, float fMaxTime = 0 );

	CBotDefendSched ( int iWaypointID, float fMaxTime = 0 );

	void init () override;
};

class CBotGetMetalSched : public CBotSchedule
{
public:
	CBotGetMetalSched (const Vector& vOrigin );

	void init () override;
};

class CBotBackstabSched : public CBotSchedule
{
public:
	CBotBackstabSched ( edict_t *pEnemy );

	void init() override;
};

class CBotTFEngiLookAfterSentry : public CBotSchedule
{
public:
	CBotTFEngiLookAfterSentry ( edict_t *pSentry );

	void init () override;
};

class CBotFollowLastEnemy : public CBotSchedule
{
public:
	CBotFollowLastEnemy ( CBot *pBot, edict_t *pEnemy, const Vector& vLastSee );

	void init () override;
};

class CBotTF2SnipeSched : public CBotSchedule
{
public:
	CBotTF2SnipeSched (const Vector& vOrigin, int iWpt );

	void init () override;
};

class CBotTF2SnipeCrossBowSched : public CBotSchedule
{
public:
	CBotTF2SnipeCrossBowSched(const Vector& vOrigin, int iWpt);

	void init() override;
};

class CBotTF2AttackSentryGun : public CBotSchedule 
{
public:
	CBotTF2AttackSentryGun( edict_t *pSentry, CBotWeapon *pWeapon );

	void init () override
	{
		setID(SCHED_ATTACK_SENTRY_GUN);
	}
};

class CBotTF2GetAmmoSched : public CBotSchedule
{
public:
	CBotTF2GetAmmoSched (const Vector& vOrigin );

	void init () override;
};

class CBotTF2GetHealthSched : public CBotSchedule
{
public:
	CBotTF2GetHealthSched (const Vector& vOrigin );

	void init () override;
};

class CBotTF2GetFlagSched : public CBotSchedule
{
public:
	CBotTF2GetFlagSched (const Vector& vOrigin, bool bUseRoute = false, const Vector& vRoute = Vector(0,0,0) );

	void init () override;
};

class CBotTF2FindFlagSched : public CBotSchedule
{
public:
	CBotTF2FindFlagSched (const Vector& vOrigin );

	void init () override;
};

class CBotPickupSched : public CBotSchedule
{
public:
	CBotPickupSched ( edict_t *pEdict );

	void init () override;
};

class CBotPickupSchedUse : public CBotSchedule
{
public:
	CBotPickupSchedUse ( edict_t *pEdict );

	void init () override;
};

class CBotGotoOriginSched : public CBotSchedule
{
public:
	CBotGotoOriginSched (const Vector& vOrigin );

	CBotGotoOriginSched ( edict_t *pEdict );

	void init () override;
};

class CBotAttackSched : public CBotSchedule
{
public:
	CBotAttackSched ( edict_t *pEdict );

	void init () override;
};

class CRunForCover : public CBotSchedule
{
public:
	// run for cover to this spot
	CRunForCover ( Vector vOrigin );

	void init () override
	{
		setID(SCHED_RUN_FOR_COVER);
	}
};

class CBotInvestigateNoiseSched : public CBotSchedule
{
public:
	CBotInvestigateNoiseSched (const Vector& vSource, const Vector& vAttackPoint );

	void init () override;
};

class CGotoHideSpotSched : public CBotSchedule
{
public:
	// find a hide spot
	// hide from an enemy (pEdict)
	CGotoHideSpotSched ( CBot *pBot, edict_t *pEdict, bool bIsGrenade = false );
	// hide from a Vector
	CGotoHideSpotSched ( CBot *pBot, const Vector& vOrigin, IBotTaskInterrupt *interrupt = nullptr);

	void init () override;
};

class CGotoNestSched : public CBotSchedule
{
public:
	// iWaypoint = the waypoint number the bot will go to (to nest)
	// if iWaypoint is -1 it will find a random, suitable nest
	CGotoNestSched ( int iWaypoint );

	void init () override;
};

class CCrouchHideSched : public CBotSchedule
{
public:
	// iWaypoint = the waypoint number the bot will go to (to nest)
	// if iWaypoint is -1 it will find a random, suitable nest
	CCrouchHideSched ( edict_t *pCoverFrom );

	void init () override;
};

class CDeployMachineGunSched : public CBotSchedule
{
public:
	// iWaypoint = the waypoint number the bot will go to (to nest)
	// if iWaypoint is -1 it will find a random, suitable nest
	CDeployMachineGunSched ( CBotWeapon *pWeapon, CWaypoint *pWaypoint, const Vector& vEnemy );

	void init () override
	{
		setID(SCHED_DEPLOY_MACHINE_GUN);
	}
};

class CSynOpenDoorSched : public CBotSchedule
{
public:
	// pDoor - The door the bot will try to open
	CSynOpenDoorSched( edict_t *pDoor );

	void init() override
	{
		setID(SCHED_SYN_OPEN_DOOR);
	}
};

class CSynDisarmMineSched : public CBotSchedule
{
public:
	// @param pMine The mine the bot will try to disarm
	CSynDisarmMineSched( edict_t *pMine );

	void init() override
	{
		setID(SCHED_SYN_DISARM_MINE);
	}
};

class CSynBreakICrateSched : public CBotSchedule
{
public:
	/**
	 * Schedule to break an item_item_crate entity
	 * 
	 * @param pCrate   The crate to break
	 * @param pWeapon  The weapon to use
	 **/
	CSynBreakICrateSched(edict_t* pCrate, CBotWeapon* pWeapon);

	void init() override
	{
		setID(SCHED_SYN_BREAK_ICRATE);
	}
};

/********************************
 *    Counter-Strike: Source    *
 ********************************/

class CCSSPlantBombSched : public CBotSchedule
{
public:
	/**
	 * Schedule for the bot to plant the C4 in CSS
	 * 
	 * @param pWaypoint		The waypoint where the c4 will be planted
	 * @param pRoute		Route waypoint to use
	 **/
	CCSSPlantBombSched(CWaypoint *pWaypoint, CWaypoint *pRoute = nullptr);

	void init() override
	{
		setID(SCHED_PLANT_BOMB);
	}
};

/********************************
 *    Navigation Testing        *
 ********************************/

/**
 * Schedule for nav-test exploration mode
 * Bot will navigate to unvisited/least-visited waypoints to test the waypoint network
 */
class CNavTestExploreSched : public CBotSchedule
{
public:
	/**
	 * Create an exploration schedule
	 *
	 * @param iTargetWaypoint   The waypoint to navigate to (-1 for auto-select)
	 **/
	CNavTestExploreSched(int iTargetWaypoint = -1);

	void init() override
	{
		setID(SCHED_NAVTEST_EXPLORE);
	}

private:
	int m_iTargetWaypoint;
};

#endif