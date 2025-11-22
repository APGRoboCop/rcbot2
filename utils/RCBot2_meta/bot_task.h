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
#ifndef __RCBOT_TASK_H__
#define __RCBOT_TASK_H__

class CBot;
class CBotSquad;
class CBotSchedule;
struct edict_t;

#include "bot.h"
#include "bot_const.h"
#include "bot_fortress.h"
#include "bot_waypoint.h"

#include <memory>

class CWaypointVisibilityTable;

class IBotTaskInterrupt
{
public:
	virtual ~IBotTaskInterrupt() = default;
	virtual bool isInterrupted(CBot* pBot, bool* bFailed, bool* bCompleted) = 0;
};

class CBotTF2EngineerInterrupt : public IBotTaskInterrupt
{
public:
	CBotTF2EngineerInterrupt(const CBot* pBot);

	bool isInterrupted(CBot* pBot, bool* bFailed, bool* bCompleted) override;
private:
	float m_fPrevSentryHealth;
	MyEHandle m_pSentryGun;
	CBotWeapon* pWrench;
};

class CBotTF2CoverInterrupt : public IBotTaskInterrupt
{
public:
	bool isInterrupted(CBot* pBot, bool* bFailed, bool* bCompleted) override;
};

class CBotTF2HurtInterrupt : public IBotTaskInterrupt
{
public:
	CBotTF2HurtInterrupt(const CBot* pBot);

	bool isInterrupted(CBot* pBot, bool* bFailed, bool* bCompleted) override;
private:
	float m_iHealth;
};

class CBotCSSRoamInterrupt : public IBotTaskInterrupt
{
public:
	bool isInterrupted(CBot* pBot, bool* bFailed, bool* bCompleted) override;
};

class CBotSYNRoamInterrupt : public IBotTaskInterrupt
{
public:
	bool isInterrupted(CBot* pBot, bool* bFailed, bool* bCompleted) override;
};

class CBotTask
{
public:
	CBotTask();

	virtual ~CBotTask() = default;
	void _init();
	virtual void init();
	virtual void execute(CBot* pBot, CBotSchedule* pSchedule);

	//void setTimeout ();

	bool hasFailed() const;
	bool isComplete() const;
	//void setVector ( Vector vOrigin );
	//void setFloat ( float fFloat );
	bool timedOut() const;
	//void setEdict ( edict_t *pEdict );
	void setFailInterrupt(int iInterruptHave, int iInterruptDontHave = 0);
	void setCompleteInterrupt(int iInterruptHave, int iInterruptDontHave = 0);
	void setInterruptFunction(std::unique_ptr<IBotTaskInterrupt> func) { m_pInterruptFunc = std::move(func); }
	virtual eTaskState isInterrupted(CBot* pBot);
	void fail();
	void complete();
	bool hasFlag(const int iFlag) const { return (m_iFlags & iFlag) == iFlag; }
	void setFlag(const int iFlag) { m_iFlags |= iFlag; }
	void clearFailInterrupts() { m_iFailInterruptConditionsHave = m_iFailInterruptConditionsDontHave = 0; }
	virtual void debugString(char* string, unsigned bufferSize) { string[0] = 0; }

	//bool isID ( eTaskID eTaskId ) { };

protected:

	std::unique_ptr<IBotTaskInterrupt> m_pInterruptFunc;
	//void setID();
	// flags
	int m_iFlags;
	// conditions that may happen to fail task
	int m_iFailInterruptConditionsHave;
	int m_iCompleteInterruptConditionsHave;
	int m_iFailInterruptConditionsDontHave;
	int m_iCompleteInterruptConditionsDontHave;
	// current state
	int m_iState; //Mistake? [APG]RoboCop[CL]
	// time out
	float m_fTimeOut;
	// vars
	//edict_t *m_pEdict;
	//float m_fFloat;
	//int m_iInt;
	//Vector m_vVector;
};
///////////////////////////////////

class CFindPathTask : public CBotTask
{
public:
	CFindPathTask()
	{
		m_pEdict = nullptr;
		m_LookTask = LOOK_WAYPOINT;
		m_iWaypointId = -1;
		m_flags.m_data = 0;
		m_fRange = 0.0f;
		m_iInt = 0;
		m_iDangerPoint = -1;
		m_bGetPassedIntAsWaypointId = false;
	}

	CFindPathTask(const Vector& vOrigin, const eLookTask looktask = LOOK_WAYPOINT)
		: m_vVector(vOrigin), m_LookTask(looktask)
	{
		m_pEdict = nullptr;
		m_iWaypointId = -1;
		m_flags.m_data = 0;
		m_fRange = 0.0f;
		m_iInt = 0;
		m_iDangerPoint = -1;
		m_bGetPassedIntAsWaypointId = false;
	}

	// having this constructor saves us trying to find the goal waypoint again if we
	// already know it
	CFindPathTask(int iWaypointId, eLookTask looktask = LOOK_WAYPOINT);

	CFindPathTask(edict_t* pEdict);

	void setRange(const float fRange) { m_fRange = fRange; }

	void setEdict(edict_t* pEdict) { m_pEdict = pEdict; }

	void setDangerPoint(const int iWpt) { m_iDangerPoint = iWpt; }

	void getPassedVector() { m_flags.bits.m_bGetPassedVector = true; }

	void getPassedIntAsWaypointId() { m_bGetPassedIntAsWaypointId = true; }

	void dontGoToEdict() { m_flags.bits.m_bDontGoToEdict = true; }

	void setNoInterruptions() { clearFailInterrupts(); m_flags.bits.m_bNoInterruptions = true; }

	void execute(CBot* pBot, CBotSchedule* pSchedule) override;

	void completeOutOfRangeFromEdict() { m_flags.bits.m_bCompleteOutOfRangeEdict = true; }

	void completeInRangeFromEdict() { m_flags.bits.m_bCompleteInRangeOfEdict = true; }

	void completeIfSeeTaskEdict() { m_flags.bits.m_bCompleteSeeTaskEdict = true; }

	void failIfTaskEdictDead() { m_flags.bits.m_bFailTaskEdictDied = true; }

	void init() override;

	void setLookTask(const eLookTask task) { m_LookTask = task; }

	void debugString(char* string, unsigned bufferSize) override;
private:


	Vector m_vVector;
	MyEHandle m_pEdict;
	eLookTask m_LookTask;
	int m_iInt;
	int m_iWaypointId;
	int m_iDangerPoint;
	float m_fRange;

	union
	{
		byte m_data;

		struct
		{
			bool m_bNoInterruptions : 1;	// quick path finding - non concurrent
			bool m_bGetPassedVector : 1;  // receive vector from previous task
			bool m_bDontLookAtWaypoints : 1;
			bool m_bCompleteSeeTaskEdict : 1; // complete when see the edict
			bool m_bFailTaskEdictDied : 1; // fail if the edict no longer exists or dead
			bool m_bDontGoToEdict : 1; // don't complete if nearby edict
			bool m_bCompleteOutOfRangeEdict : 1; // complete if outside of m_fRange from edict (grenades)
			bool m_bCompleteInRangeOfEdict : 1;
		}bits;
	}m_flags;

	bool m_bGetPassedIntAsWaypointId;
	//bool m_bWaitUntilReached;
};

class CBotTF2AttackSentryGunTask : public CBotTask
{
public:
	CBotTF2AttackSentryGunTask(edict_t* pSentryGun, CBotWeapon* pWeapon);

	void execute(CBot* pBot, CBotSchedule* pSchedule) override;

	void init() override
	{
		m_fTime = 0.0f;
	}

	void debugString(char* string, unsigned bufferSize) override;
private:

	MyEHandle m_pSentryGun;
	CBotWeapon* m_pWeapon;
	int m_iStartingWaypoint;
	int m_iSentryWaypoint;
	Vector m_vStart;
	Vector m_vHide;
	float m_fDist;
	float m_fTime;
};

//defensive technique
class CBotTF2Spam : public CBotTask
{
public:
	CBotTF2Spam(const Vector& vStart, const Vector& vTarget, CBotWeapon* pWeapon);

	CBotTF2Spam(const CBot* pBot, const Vector& vStart, float fYaw, CBotWeapon* pWeapon);

	void execute(CBot* pBot, CBotSchedule* pSchedule) override;

	Vector getTarget() { return m_vTarget; }

	void debugString(char* string, const unsigned bufferSize) override
	{
		snprintf(string, bufferSize, "CBotTF2Spam");
	}

	float getDistance() const;
private:

	Vector m_vTarget;
	Vector m_vStart;
	CBotWeapon* m_pWeapon;
	float m_fTime;

	float m_fNextAttack;
};

enum : std::uint8_t
{
	TASK_TF2_DEMO_STATE_LAY_BOMB = 0,
	TASK_TF2_DEMO_STATE_RUN_UP = 1
};

class CBotTF2DemomanPipeJump : public CBotTask
{
public:
	CBotTF2DemomanPipeJump(CBot* pBot, const Vector& vWaypointGround, const Vector& vWaypointNext, CBotWeapon* pWeapon);

	void execute(CBot* pBot, CBotSchedule* pSchedule) override;

	void debugString(char* string, const unsigned bufferSize) override
	{
		snprintf(string, bufferSize, "CBotTF2DemomanPipeJump");
	}
private:

	Vector m_vStart;
	Vector m_vPipe; //TODO: required for proper pipe jumping? [APG]RoboCop[CL]
	Vector m_vEnd;
	MyEHandle m_pPipeBomb;
	bool m_bFired;
	float m_fTime;
	//int m_iState;
	int m_iStartingAmmo;
	CBotWeapon* m_pWeapon;
};

// automatically detonate pipes from a standing location, make sure
// the bot is not standing in a location visible to the enemy
// in vStand
class CBotTF2DemomanPipeEnemy : public CBotTask
{
public:
	//CBotTF2DemomanPipeEnemy ( Vector vStand, Vector vBlastPoint, CBotWeapon *pPipeLauncher, Vector vEnemy, edict_t *pEnemy );
	CBotTF2DemomanPipeEnemy(CBotWeapon* pPipeLauncher, const Vector& vEnemy, edict_t* pEnemy);
	void execute(CBot* pBot, CBotSchedule* pSchedule) override;

	void debugString(char* string, const unsigned bufferSize) override
	{
		snprintf(string, bufferSize, "CBotTF2DemomanPipeEnemy");
	}
private:
	Vector m_vStand;
	Vector m_vEnemy;
	MyEHandle m_pEnemy;
	Vector m_vAim;
	float m_fTime;
	float m_fHoldAttackTime;
	float m_fHeldAttackTime;
	CBotWeapon* m_pPipeLauncher;
};

class CBotTF2DemomanPipeTrap : public CBotTask
{
public:
	// Set up a pipe trap or fire pipe bombs -- 
	// if autodetonate, detonate them when I've shot them rather than wait for an enemy
	// such as when attacking a sentry
	CBotTF2DemomanPipeTrap(eDemoTrapType type, const Vector& vStand, const Vector& vLoc, const Vector& vSpread, bool bAutoDetonate = false, int m_iWptArea = -1);

	void execute(CBot* pBot, CBotSchedule* pSchedule) override;

	void debugString(char* string, const unsigned bufferSize) override
	{
		snprintf(string, bufferSize, "CBotTF2DemomanPipeTrap");
	}
private:
	Vector m_vPoint;
	Vector m_vStand;
	Vector m_vLocation;
	Vector m_vSpread;
	float m_fTime;
	eDemoTrapType m_iTrapType;
	//int m_iState;
	int m_iStickies;
	bool m_bAutoDetonate;
	int m_iWptArea;

};

class CBotTF2FindPipeWaypoint : public CBotTask
{
public:
	CBotTF2FindPipeWaypoint(const Vector& vOrigin, const Vector& vTarget);

	void execute(CBot* pBot, CBotSchedule* pSchedule) override;

	void debugString(char* string, const unsigned bufferSize) override
	{
		snprintf(string, bufferSize, "Finding Pipe Waypoint");
	}
private:
	int m_iters;
	unsigned short int m_i;
	unsigned short int m_j;
	Vector m_vOrigin;
	Vector m_vTarget;
	short int m_iTargetWaypoint;
	float m_fNearesti;
	float m_fNearestj;
	short int m_iNearesti;
	short int m_iNearestj;

	CWaypointVisibilityTable* m_pTable;
	CWaypoint* m_pTarget;
	WaypointList m_WaypointsI;
	WaypointList m_WaypointsJ;
};


class CBotGravGunPickup : public CBotTask
{
public:
	CBotGravGunPickup(edict_t* pWeapon, edict_t* pProp)
		: m_Weapon(pWeapon), m_Prop(pProp)
	{
		m_fTime = 0.0f;
		m_fSecAttTime = 0.0f;
	}

	void execute(CBot* pBot, CBotSchedule* pSchedule) override;

	void debugString(char* string, const unsigned bufferSize) override
	{
		snprintf(string, bufferSize, "Grav Gun Pickup");
	}
private:
	MyEHandle m_Weapon;
	MyEHandle m_Prop;
	float m_fTime;
	float m_fSecAttTime;

};

enum : std::uint8_t
{
	CHARGER_HEALTH = 0,
	CHARGER_ARMOR = 1
};

class CBotHL2DMUseCharger : public CBotTask
{
public:
	CBotHL2DMUseCharger(edict_t* pCharger, const int type) : m_pCharger(pCharger)
	{
		m_fTime = 0;
		m_iType = type;
	}

	void execute(CBot* pBot, CBotSchedule* pSchedule) override;

	void debugString(char* string, const unsigned bufferSize) override
	{
		snprintf(string, bufferSize, "Use Charger");
	}
private:
	MyEHandle m_pCharger;
	float m_fTime;
	int m_iType;

};

class CBotHL2DMUseButton : public CBotTask
{
public:
	CBotHL2DMUseButton(edict_t* pButton) : m_pButton(pButton)
	{
		m_fTime = 0.0f;
		m_bOverrideLook = false;
	}

	CBotHL2DMUseButton(edict_t* pButton, const bool bOverrideLook) : m_pButton(pButton)
	{
		m_fTime = 0.0f;
		m_bOverrideLook = bOverrideLook;
	}

	void execute(CBot* pBot, CBotSchedule* pSchedule) override;

	void debugString(char* string, const unsigned bufferSize) override
	{
		snprintf(string, bufferSize, "Use Button");
	}
private:
	MyEHandle m_pButton;
	float m_fTime;
	bool m_bOverrideLook;
};

class CBotTF2MedicHeal : public CBotTask
{
public:
	CBotTF2MedicHeal();

	void execute(CBot* pBot, CBotSchedule* pSchedule) override;

	void debugString(char* string, const unsigned bufferSize) override
	{
		snprintf(string, bufferSize, "CBotTF2MedicHeal");
	}
private:
	MyEHandle m_pHeal;
	Vector m_vJump;
	bool m_bHealerJumped;

};

class CBotRemoveSapper : public CBotTask
{
public:
	CBotRemoveSapper(edict_t* pBuilding, eEngiBuild id);

	void execute(CBot* pBot, CBotSchedule* pSchedule) override;

	void debugString(char* string, const unsigned bufferSize) override
	{
		snprintf(string, bufferSize, "CBotRemoveSapper");
	}
private:
	float m_fTime;
	float m_fHealTime;
	MyEHandle m_pBuilding;
	eEngiBuild m_id;
};

class CBotUseLunchBoxDrink : public CBotTask
{
public:
	CBotUseLunchBoxDrink();

	void execute(CBot* pBot, CBotSchedule* pSchedule) override;

	void debugString(char* string, const unsigned bufferSize) override
	{
		snprintf(string, bufferSize, "CBotUseLunchBoxDrink");
	}
private:
	float m_fTime;
};

class CBotUseBuffItem : public CBotTask
{
public:
	CBotUseBuffItem();

	void execute(CBot* pBot, CBotSchedule* pSchedule) override;

	void debugString(char* string, const unsigned bufferSize) override
	{
		snprintf(string, bufferSize, "CBotUseBuffITem");
	}
private:
	float m_fTime;
};

class CBotBackstab : public CBotTask
{
public:
	CBotBackstab(edict_t* _pEnemy);

	void execute(CBot* pBot, CBotSchedule* pSchedule) override;

	void debugString(char* string, const unsigned bufferSize) override
	{
		snprintf(string, bufferSize, "CBotBackstab");
	}
private:
	float m_fTime;
	MyEHandle pEnemy;
};

class CBotJoinSquad : public CBotTask
{
public:

	CBotJoinSquad(edict_t* pPlayerToJoin)
	{
		m_pPlayer = pPlayerToJoin;
	}

	void execute(CBot* pBot, CBotSchedule* pSchedule) override;

	void debugString(char* string, const unsigned bufferSize) override
	{
		snprintf(string, bufferSize, "CBotFollowSquadLeader");
	}
private:
	edict_t* m_pPlayer;
};

class CBotFollowSquadLeader : public CBotTask
{
public:

	CBotFollowSquadLeader(CBotSquad* pSquad)
	{
		m_fLeaderSpeed = 0.0f;
		m_pSquad = pSquad;
		m_fVisibleTime = 0.0f;
		m_fUpdateMovePosTime = 0.0f;
		m_vPos = Vector(0, 0, 0);
		m_vForward = Vector(0, 0, 0);
	}

	void execute(CBot* pBot, CBotSchedule* pSchedule) override;

	void debugString(char* string, const unsigned bufferSize) override
	{
		snprintf(string, bufferSize, "CBotFollowSquadLeader");
	}
private:
	CBotSquad* m_pSquad;
	float m_fVisibleTime;
	float m_fUpdateMovePosTime;
	float m_fLeaderSpeed;
	Vector m_vPos;
	Vector m_vForward;
};

class CBotNest : public CBotTask
{
public:
	CBotNest();

	void execute(CBot* pBot, CBotSchedule* pSchedule) override;

	void debugString(char* string, const unsigned bufferSize) override
	{
		snprintf(string, bufferSize, "CBotNest");
	}
private:
	float m_fTime;
	MyEHandle m_pEnemy;
};

class CBotDefendTask : public CBotTask
{
public:
	CBotDefendTask(const Vector& vOrigin, const float fMaxTime = 0.0f, const int iInterrupt = CONDITION_SEE_CUR_ENEMY,
		const bool bDefendOrigin = false, const Vector& vDefendOrigin = Vector(0, 0, 0),
		const eLookTask looktask = LOOK_SNIPE, const int iWaypointType = 0) : m_vOrigin(vOrigin), m_vDefendOrigin(vDefendOrigin)
	{
		m_fMaxTime = fMaxTime;
		m_fTime = 0.0f;
		setCompleteInterrupt(iInterrupt);
		m_bDefendOrigin = bDefendOrigin;
		m_LookTask = looktask;
		m_iWaypointType = iWaypointType;
	}

	void execute(CBot* pBot, CBotSchedule* pSchedule) override;

	void debugString(char* string, const unsigned bufferSize) override
	{
		snprintf(string, bufferSize, "CBotDefendTask");
	}
private:
	float m_fTime;
	float m_fMaxTime;
	Vector m_vOrigin;
	bool m_bDefendOrigin;
	Vector m_vDefendOrigin;
	eLookTask m_LookTask;
	int m_iWaypointType;
};

class CBotInvestigateTask : public CBotTask
{
public:
	CBotInvestigateTask(const Vector& vOrigin, const float fRadius, const Vector& vPOV, const bool bHasPOV, const float fMaxTime = 0,
		const int iInterrupt = CONDITION_SEE_CUR_ENEMY) : m_vOrigin(vOrigin), m_vPOV(vPOV)
	{
		m_fMaxTime = fMaxTime;
		m_fRadius = fRadius;
		m_fTime = 0.0f;
		setCompleteInterrupt(iInterrupt);
		m_iCurPath = 0;
		m_iState = 0;
		m_bHasPOV = bHasPOV;
	}

	void execute(CBot* pBot, CBotSchedule* pSchedule) override;

	void debugString(char* string, const unsigned bufferSize) override
	{
		snprintf(string, bufferSize, "CBotInvestigateTask");
	}
private:
	//int m_iState;
	float m_fTime;
	float m_fMaxTime;
	Vector m_vOrigin;
	float m_fRadius;
	int m_iCurPath;
	bool m_bHasPOV;
	Vector m_vPOV;
	std::vector<Vector> m_InvPoints; // investigation points (waypoint paths)
};

class CBotTF2EngiLookAfter : public CBotTask
{
public:
	CBotTF2EngiLookAfter(edict_t* pSentry)
		: m_fTime(0.0f), m_fHitSentry(0.0f), m_pSentry(pSentry) {
	}

	void execute(CBot* pBot, CBotSchedule* pSchedule) override;

	void debugString(char* string, const unsigned bufferSize) override
	{
		snprintf(string, bufferSize, "CBotTF2EngiLookAfter");
	}
private:
	float m_fTime;
	float m_fHitSentry;
	MyEHandle m_pSentry;
};

class CBotTF2SnipeCrossBow : public CBotTask
{
public:
	CBotTF2SnipeCrossBow(const Vector& vOrigin, int iWpt);

	void execute(CBot* pBot, CBotSchedule* pSchedule) override;

	void debugString(char* string, const unsigned bufferSize) override
	{
		snprintf(string, bufferSize, "CBotSnipe");
	}
private:
	float m_fTime; // time of task
	Vector m_vAim; // base origin to aim at
	Vector m_vOrigin; // origin of snipe waypoint
	Vector m_vEnemy; // origin of last enemy
	int m_iArea; // area of snipe waypoint
	int m_iHideWaypoint; // waypoint id of place to hide
	int m_iSnipeWaypoint; // waypoint id of place to snipe
	Vector m_vHideOrigin; // origin of hiding place
	float m_fHideTime; // if above engine time, hide
	int m_iPrevClip; // used to check i actually fired a bullet or not
	float m_fEnemyTime; // last time i saw an enemy
	float m_fAimTime; // last time i got ready to aim - gives bots time to aim before shooting
	float m_fCheckTime; // time to check if there is a wall in front of me while sniping
	float m_fOriginDistance;
};


class CBotTF2Snipe : public CBotTask
{
public:
	CBotTF2Snipe(const Vector& vOrigin, int iWpt);

	void execute(CBot* pBot, CBotSchedule* pSchedule) override;

	void debugString(char* string, const unsigned bufferSize) override
	{
		snprintf(string, bufferSize, "CBotSnipe");
	}
private:
	float m_fTime; // time of task
	Vector m_vAim; // base origin to aim at
	Vector m_vOrigin; // origin of snipe waypoint
	Vector m_vEnemy; // origin of last enemy
	int m_iArea; // area of snipe waypoint
	int m_iHideWaypoint; // waypoint id of place to hide
	int m_iSnipeWaypoint; // waypoint id of place to snipe
	Vector m_vHideOrigin; // origin of hiding place
	float m_fHideTime; // if above engine time, hide
	int m_iPrevClip; // used to check i actually fired a bullet or not
	float m_fEnemyTime; // last time i saw an enemy
	float m_fAimTime; // last time i got ready to aim - gives bots time to aim before shooting
	float m_fCheckTime; // time to check if there is a wall in front of me while sniping
	float m_fOriginDistance;
};



class CBotDODSnipe : public CBotTask
{
public:
	CBotDODSnipe(CBotWeapon* pWeaponToUse, const Vector& vOrigin, float fYaw, bool bUseZ = false, float z = 0, int iWaypointType = 0);

	void execute(CBot* pBot, CBotSchedule* pSchedule) override;

	void debugString(char* string, unsigned bufferSize) override;

private:
	float m_fTime;
	float m_fEnemyTime;
	float m_fScopeTime;
	Vector m_vAim;
	Vector m_vOrigin;
	CBotWeapon* m_pWeaponToUse;
	Vector m_vLastEnemy;
	bool m_bUseZ;
	float m_z; // z = ground level
	int m_iWaypointType;
	float m_fTimeout;
};

class CBotHL2DMSnipe : public CBotTask
{
public:
	CBotHL2DMSnipe(CBotWeapon* pWeaponToUse, const Vector& vOrigin, float fYaw, bool bUseZ = false, float z = 0, int iWaypointType = 0);

	void execute(CBot* pBot, CBotSchedule* pSchedule) override;

	void debugString(char* string, unsigned bufferSize) override;

private:
	float m_fTime;
	float m_fEnemyTime;
	float m_fScopeTime;
	Vector m_vAim;
	Vector m_vOrigin;
	CBotWeapon* m_pWeaponToUse;
	bool m_bUseZ;
	float m_z; // z = ground level
	int m_iWaypointType;
};

class CBotTF2SpyDisguise : public CBotTask
{
public:
	CBotTF2SpyDisguise();

	void execute(CBot* pBot, CBotSchedule* pSchedule) override;

	void debugString(char* string, const unsigned bufferSize) override
	{
		snprintf(string, bufferSize, "CBotTF2SpyDisguise");
	}
};

class CBotTFEngiBuildTask : public CBotTask
{
public:
	CBotTFEngiBuildTask(eEngiBuild iObject, CWaypoint* pWaypoint);

	void execute(CBot* pBot, CBotSchedule* pSchedule) override;

	void debugString(char* string, unsigned bufferSize) override;

	void oneTryOnly() { m_iTries = 2; }

private:
	Vector m_vOrigin;
	eEngiBuild m_iObject;
	//int m_iState;
	float m_fTime;
	int m_iTries;
	float m_fNextUpdateAngle;
	Vector m_vAimingVector;
	int m_iArea;
	Vector m_vBaseOrigin;
	float m_fRadius;
};


class CDODDropAmmoTask : public CBotTask
{
public:
	CDODDropAmmoTask(edict_t* pPlayer) : m_pPlayer(pPlayer)
	{
		m_fTime = 0.0f;
	}

	void debugString(char* string, unsigned bufferSize) override;
	void execute(CBot* pBot, CBotSchedule* pSchedule) override;

private:
	MyEHandle m_pPlayer;
	float m_fTime;
};

class CDODWaitForGrenadeTask : public CBotTask
{
public:
	CDODWaitForGrenadeTask(edict_t* pGrenade) : m_pGrenade(pGrenade)
	{
		m_fTime = 0.0f;
	}

	void debugString(char* string, unsigned bufferSize) override;
	void execute(CBot* pBot, CBotSchedule* pSchedule) override;

private:
	MyEHandle m_pGrenade;
	float m_fTime;
};

class CDODWaitForBombTask : public CBotTask
{
public:
	CDODWaitForBombTask(edict_t* pBombTarget, CWaypoint* pBlocking) : m_pBombTarget(pBombTarget)
	{
		m_fTime = 0.0f;
		m_pRunTo = nullptr;
		m_pBlocking = pBlocking;
	}

	void debugString(char* string, unsigned bufferSize) override;
	void execute(CBot* pBot, CBotSchedule* pSchedule) override;

private:
	MyEHandle m_pBombTarget;
	float m_fTime;
	CWaypoint* m_pRunTo;
	CWaypoint* m_pBlocking;
};

class CBotDODBomb : public CBotTask
{
public:
	CBotDODBomb(int iBombType, int iBombID, edict_t* m_pBomb, const Vector& vPosition, int iPrevOwner);
	void execute(CBot* pBot, CBotSchedule* pSchedule) override;

	void debugString(char* string, unsigned bufferSize) override;
private:
	Vector m_vOrigin;
	float m_fTime;
	int m_iBombID;
	int m_iPrevTeam; // prev owner
	edict_t* m_pBombTarget;
	int m_iType;
};

class CBotDODAttackPoint : public CBotTask
{
public:
	CBotDODAttackPoint(int iFlagID, const Vector& vOrigin, float fRadius);
	void execute(CBot* pBot, CBotSchedule* pSchedule) override;

	void debugString(char* string, unsigned bufferSize) override;
private:
	Vector m_vOrigin;
	Vector m_vMoveTo;
	float m_fAttackTime;
	float m_fTime;
	int m_iFlagID;
	float m_fRadius;
	bool m_bProne;
};

class CBotTF2AttackPoint : public CBotTask
{
public:
	CBotTF2AttackPoint(int iArea, const Vector& vOrigin, float fRadius);
	void execute(CBot* pBot, CBotSchedule* pSchedule) override;

	void debugString(char* string, unsigned bufferSize) override;
private:
	Vector m_vOrigin;
	Vector m_vMoveTo;
	float m_fAttackTime;
	float m_fTime;
	int m_iArea;
	float m_fRadius;
};

class CBotTF2ShootLastEnemyPosition : public CBotTask
{
public:
	CBotTF2ShootLastEnemyPosition(const Vector& vPosition, edict_t* pEnemy, const Vector& m_vVelocity);
	void execute(CBot* pBot, CBotSchedule* pSchedule) override;

	void debugString(char* string, unsigned bufferSize) override;
private:
	MyEHandle m_pEnemy;
	Vector m_vPosition;
	float m_fTime;
};

class CBotTF2DefendPoint : public CBotTask
{
public:
	CBotTF2DefendPoint(int iArea, const Vector& vOrigin, float fRadius);
	void execute(CBot* pBot, CBotSchedule* pSchedule) override;

	void debugString(char* string, unsigned bufferSize) override;
private:
	Vector m_vOrigin;
	Vector m_vMoveTo;
	float m_fDefendTime;
	float m_fTime;
	int m_iArea;
	float m_fRadius;
};

class CBotInvestigateHidePoint : public CBotTask
{
	// investigate a  possible enemy hiding point

public:
	CBotInvestigateHidePoint(int iWaypointIndexToInvestigate, int iOriginalWaypointIndex);

	void execute(CBot* pBot, CBotSchedule* pSchedule) override;

	void debugString(char* string, unsigned bufferSize) override;
private:
	Vector m_vOrigin;
	Vector m_vMoveTo;
	std::vector<Vector> m_CheckPoints;
	unsigned m_iCurrentCheckPoint;
	float m_fInvestigateTime;
	float m_fTime;
	//int m_iState;
};

class CBotTF2PushPayloadBombTask : public CBotTask
{
public:
	CBotTF2PushPayloadBombTask(edict_t* pPayloadBomb);

	void execute(CBot* pBot, CBotSchedule* pSchedule) override;
	void debugString(char* string, unsigned bufferSize) override;
private:
	MyEHandle m_pPayloadBomb;
	Vector m_vMoveTo;
	Vector m_vRandomOffset;
	float m_fPushTime;
	float m_fTime;
	Vector m_vOrigin;
};

class CBotTF2DefendPayloadBombTask : public CBotTask
{
public:
	CBotTF2DefendPayloadBombTask(edict_t* pPayloadBomb);

	void execute(CBot* pBot, CBotSchedule* pSchedule) override;
	void debugString(char* string, unsigned bufferSize) override;
private:
	MyEHandle m_pPayloadBomb;
	Vector m_vMoveTo;
	Vector m_vRandomOffset;
	float m_fDefendTime;
	float m_fTime;
	Vector m_vOrigin;
};

class CBotTF2UpgradeBuilding : public CBotTask
{
public:
	CBotTF2UpgradeBuilding(edict_t* pBuilding);
	void execute(CBot* pBot, CBotSchedule* pSchedule) override;

	void debugString(char* string, unsigned bufferSize) override;
private:
	MyEHandle m_pBuilding;
	float m_fTime;
};

class CBotTF2WaitAmmoTask : public CBotTask
{
public:
	CBotTF2WaitAmmoTask(const Vector& vOrigin);

	void execute(CBot* pBot, CBotSchedule* pSchedule) override;
	void debugString(char* string, unsigned bufferSize) override;
private:
	Vector m_vOrigin;
	float m_fWaitTime;
};

class CBotTF2WaitHealthTask : public CBotTask
{
public:
	CBotTF2WaitHealthTask(const Vector& vOrigin);

	void execute(CBot* pBot, CBotSchedule* pSchedule) override;
	void debugString(char* string, unsigned bufferSize) override;
private:
	Vector m_vOrigin;
	float m_fWaitTime;
};

class CBotTFDoubleJump : public CBotTask
{
public:
	CBotTFDoubleJump(); // going here

	void execute(CBot* pBot, CBotSchedule* pSchedule) override;
	void debugString(char* string, unsigned bufferSize) override;
private:
	float m_fTime;
};


class CBotTFRocketJump : public CBotTask
{
public:
	CBotTFRocketJump(); // going here

	void execute(CBot* pBot, CBotSchedule* pSchedule) override;
	void debugString(char* string, unsigned bufferSize) override;
private:
	float m_fTime;
	float m_fJumpTime;
	//int m_iState;
};

class CBotTF2SpySap : public CBotTask
{
public:
	CBotTF2SpySap(edict_t* pBuilding, eEngiBuild id); // going to use this 

	void execute(CBot* pBot, CBotSchedule* pSchedule) override;

	void debugString(char* string, unsigned bufferSize) override;
private:
	MyEHandle m_pBuilding;
	float m_fTime;
	eEngiBuild m_id;
};

class CBotTFUseTeleporter : public CBotTask
{
public:
	CBotTFUseTeleporter(edict_t* pTele); // going to use this 

	void execute(CBot* pBot, CBotSchedule* pSchedule) override;
	void debugString(char* string, unsigned bufferSize) override;
private:
	MyEHandle m_pTele;
	float m_fTime;
	Vector m_vLastOrigin;
};

class CBotTaskEngiPickupBuilding : public CBotTask
{
public:
	CBotTaskEngiPickupBuilding(edict_t* pBuilding); // going to use this 
	void execute(CBot* pBot, CBotSchedule* pSchedule) override;
	void debugString(char* string, unsigned bufferSize) override;
private:
	MyEHandle m_pBuilding;
	float m_fTime;
};

class CBotTaskEngiPlaceBuilding : public CBotTask
{
public:
	CBotTaskEngiPlaceBuilding(eEngiBuild iObject, const Vector& vOrigin); // going to use this 
	void execute(CBot* pBot, CBotSchedule* pSchedule) override;
	void debugString(char* string, unsigned bufferSize) override;
private:
	Vector m_vOrigin;
	float m_fTime;
	eEngiBuild m_iObject;
	//int m_iState;
	int m_iTries;
};

class CBotTF2WaitFlagTask : public CBotTask
{
public:
	CBotTF2WaitFlagTask(const Vector& vOrigin, bool bFind = false);

	void execute(CBot* pBot, CBotSchedule* pSchedule) override;
	void debugString(char* string, unsigned bufferSize) override;
private:
	Vector m_vOrigin;
	float m_fWaitTime;
	bool m_bFind;
};

class CThrowGrenadeTask : public CBotTask
{
public:
	CThrowGrenadeTask(CBotWeapon* pWeapon, int ammo, const Vector& vLoc);
	void init() override;
	void execute(CBot* pBot, CBotSchedule* pSchedule) override;
	void debugString(char* string, unsigned bufferSize) override;
private:
	Vector m_vLoc;
	CBotWeapon* m_pWeapon;
	float m_fTime;
	float m_fHoldAttackTime;
	int m_iAmmo;
};

class CAttackEntityTask : public CBotTask
{
public:
	CAttackEntityTask(edict_t* pEdict);
	void init() override;
	void execute(CBot* pBot, CBotSchedule* pSchedule) override;
	void debugString(char* string, unsigned bufferSize) override;
private:
	MyEHandle m_pEdict;
};

////////////////////
class CAutoBuy : public CBotTask
{
public:
	void init() override;

	void execute(CBot* pBot, CBotSchedule* pSchedule) override;

private:
	float m_fTime = 0.0f;
	bool m_bTimeset = false;
};

class CCSSPerformBuyTask : public CBotTask
{
public:
	void init() override;
	void execute(CBot* pBot, CBotSchedule* pSchedule) override;
	void debugString(char* string, const unsigned bufferSize) override
	{
		snprintf(string, bufferSize, "CSS Perform Buy");
	}
private:
	float m_fDelay = 0.0f;
};

class CCSSPlantTheBombTask : public CBotTask
{
public:
	void init() override
	{
		setFailInterrupt(CONDITION_SEE_CUR_ENEMY);
	}
	void execute(CBot* pBot, CBotSchedule* pSchedule) override;
	void debugString(char* string, const unsigned bufferSize) override
	{
		snprintf(string, bufferSize, "CSS Plant C4");
	}
};

class CCSSEngageEnemyTask : public CBotTask
{
public:
	CCSSEngageEnemyTask(const edict_t* pEnemy)
	{
		isBrush = false;
		m_hEnemy.Init(engine->IndexOfEdict(pEnemy), pEnemy->m_NetworkSerialNumber);
	}
	void init() override
	{
		setFailInterrupt(CONDITION_ENEMY_OBSCURED);
		setCompleteInterrupt(CONDITION_ENEMY_DEAD);
	}
	void execute(CBot* pBot, CBotSchedule* pSchedule) override;
	void debugString(char* string, unsigned bufferSize) override;
private:
	CBaseHandle m_hEnemy;
	bool isBrush; // Is my enemy a brush entity?
};

class CCSSDefuseTheBombTask : public CBotTask
{
public:
	CCSSDefuseTheBombTask(const Vector& vBomb) : m_vBomb(vBomb)
	{
	}
	void init() override
	{
		setFailInterrupt(CONDITION_SEE_CUR_ENEMY);
	}
	void execute(CBot* pBot, CBotSchedule* pSchedule) override;
	void debugString(char* string, const unsigned bufferSize) override
	{
		snprintf(string, bufferSize, "CSS Defuse C4\nBomb Vector (%0.4f,%0.4f,%0.4f)", m_vBomb.x, m_vBomb.y, m_vBomb.z);
	}
private:
	Vector m_vBomb;
};

class CCSSGuardTask : public CBotTask
{
public:
	CCSSGuardTask(CBotWeapon* pWeaponToUse, const Vector& vOrigin, const float fYaw, const bool bUseZ, const float z, const int iWaypointType) : m_vOrigin(vOrigin)
	{
		m_fEnemyTime = 0.0f;
		m_fTime = 0.0f;
		const QAngle angle = QAngle(0, fYaw, 0);
		AngleVectors(angle, &m_vAim);
		m_vAim = vOrigin + m_vAim * 1024;
		m_pWeaponToUse = pWeaponToUse;
		m_fScopeTime = 0.0f;
		m_bUseZ = bUseZ;
		m_z = z; // z = ground level
		m_iWaypointType = iWaypointType;
	}
	void execute(CBot* pBot, CBotSchedule* pSchedule) override;
	void debugString(char* string, const unsigned bufferSize) override
	{
		snprintf(string, bufferSize, "CSS Guard Task\nm_fEnemyTime = %.1f\nm_fTime = %.1f", m_fEnemyTime, m_fTime);
	}
private:
	float m_fTime;
	float m_fEnemyTime;
	float m_fScopeTime;
	Vector m_vAim;
	Vector m_vOrigin;
	CBotWeapon* m_pWeaponToUse;
	bool m_bUseZ;
	float m_z; // z = ground level
	int m_iWaypointType;
};

//////////////////////
class CTF2_TauntTask : public CBotTask
{
public:
	CTF2_TauntTask(edict_t* pPlayer, const Vector& vOrigin, const float fDist)
		: m_pPlayer(pPlayer), m_vOrigin(vOrigin), m_fDist(fDist)
	{
		m_fTime = 0.0f;
		m_fTauntUntil = 0.0f;
		m_fActionTime = 0.0f;
	}

	void init() override;
	void execute(CBot* pBot, CBotSchedule* pSchedule) override;
	void debugString(char* string, unsigned bufferSize) override;

private:
	MyEHandle m_pPlayer;
	Vector m_vOrigin;
	float m_fDist;
	float m_fTime;
	float m_fTauntUntil;
	float m_fActionTime;
};

/////////////////////
class CMoveToTask : public CBotTask
{
public:
	CMoveToTask(const Vector& vOrigin) : m_vVector(vOrigin)
	{
		m_pEdict = nullptr;
		fPrevDist = 0.0f;

		setFailInterrupt(CONDITION_SEE_CUR_ENEMY);
	}

	CMoveToTask(edict_t* pEdict);

	void init() override;
	void execute(CBot* pBot, CBotSchedule* pSchedule) override;
	void debugString(char* string, unsigned bufferSize) override;

private:
	float fPrevDist;
	Vector m_vVector;
	MyEHandle m_pEdict;
};

class CMessAround : public CBotTask
{
public:
	CMessAround(edict_t* pFriendly, int iMaxVoiceCmd);

	void execute(CBot* pBot, CBotSchedule* pSchedule) override;

	void debugString(char* string, const unsigned bufferSize) override
	{
		snprintf(string, bufferSize, "CMessAround");
	}
private:
	float m_fTime;
	MyEHandle m_pFriendly;
	int m_iMaxVoiceCmd;
	int m_iType; // 0 = attack friendly , 1 = taunt, 2 = random voice command
};

class CFindLastEnemy : public CBotTask
{
public:
	CFindLastEnemy(const Vector& vLast, const Vector& vVelocity);

	void execute(CBot* pBot, CBotSchedule* pSchedule) override;

	void debugString(char* string, const unsigned bufferSize) override
	{
		snprintf(string, bufferSize, "CFindLastEnemy");
	}
private:
	Vector m_vLast;
	float m_fTime;
};

class CFindGoodHideSpot : public CBotTask
{
public:
	CFindGoodHideSpot(edict_t* pEntity);

	CFindGoodHideSpot(const Vector& vec);

	void init() override;

	void execute(CBot* pBot, CBotSchedule* pSchedule) override;

	void debugString(char* string, const unsigned bufferSize) override
	{
		snprintf(string, bufferSize, "CFindGoodHideSpot");
	}
private:
	Vector m_vHideFrom;
};

class CHideTask : public CBotTask
{
public:
	CHideTask(const Vector& vHideFrom);

	void init() override;

	void execute(CBot* pBot, CBotSchedule* pSchedule) override;

	void debugString(char* string, unsigned bufferSize) override;
private:
	Vector m_vHideFrom;
	float m_fHideTime;
};

class CFollowTask : public CBotTask
{
public:
	CFollowTask(edict_t* pFollow);

	void init() override;

	void execute(CBot* pBot, CBotSchedule* pSchedule) override;

	void debugString(char* string, unsigned bufferSize) override;
private:
	MyEHandle m_pFollow;
	float m_fFollowTime;
	Vector m_vLastSeeVector;
	Vector m_vLastSeeVelocity;
};

class CCrouchHideTask : public CBotTask
{
public:
	CCrouchHideTask(edict_t* pHideFrom);

	void init() override;

	void execute(CBot* pBot, CBotSchedule* pSchedule) override;

	void debugString(char* string, unsigned bufferSize) override;
private:
	MyEHandle m_pHideFrom;
	float m_fHideTime;
	float m_fChangeTime;
	bool m_bCrouching;
	Vector m_vLastSeeVector;
};

class CSpyCheckAir : public CBotTask
{
public:
	CSpyCheckAir()
	{
		seenlist = 0;
		m_fNextCheckUnseen = 0.0f;
		m_fTime = 0.0f;
		m_pUnseenBefore = nullptr;
		m_bHitPlayer = false;
	}

	void execute(CBot* pBot, CBotSchedule* pSchedule) override;

	void debugString(char* string, unsigned bufferSize) override;
private:
	edict_t* m_pUnseenBefore;
	int seenlist;
	float m_fNextCheckUnseen;
	float m_fTime;
	bool m_bHitPlayer;
};

class CTaskVoiceCommand : public CBotTask
{
public:
	CTaskVoiceCommand(const byte iVoiceCmd)
	{
		m_iVoiceCmd = iVoiceCmd;
	}

	void execute(CBot* pBot, CBotSchedule* pSchedule) override
	{
		pBot->addVoiceCommand(m_iVoiceCmd);

		complete();
	}

private:
	byte m_iVoiceCmd;
};

class CPrimaryAttack : public CBotTask
{
public:

	void execute(CBot* pBot, CBotSchedule* pSchedule) override;
};

class CBotWaitTask : public CBotTask
{
public:
	CBotWaitTask(const float waittime)
	{
		m_fTime = engine->Time() + waittime;
		m_bAimSet = false;
	}
	CBotWaitTask(const float waittime, const Vector& vAim) : m_vAim(vAim)
	{
		m_fTime = engine->Time() + waittime;
		m_bAimSet = true;
	}
	void execute(CBot* pBot, CBotSchedule* pSchedule) override;
	void debugString(char* string, unsigned bufferSize) override;
private:
	float m_fTime;
	bool m_bAimSet;
	Vector m_vAim;
};

class CBotSynDisarmMineTask : public CBotTask
{
public:
	CBotSynDisarmMineTask(edict_t* pMine) : m_pMine(pMine)
	{
		m_fDist = 0.0f;
		m_fTime = 0.0f;
		m_bTimeSet = false;
	}
	void execute(CBot* pBot, CBotSchedule* pSchedule) override;
	void debugString(char* string, unsigned bufferSize) override;
private:
	Vector m_vMinePos;
	float m_fDist;
	float m_fTime;
	bool m_bTimeSet;
	MyEHandle m_pMine;
};

class CBotSynBreakICrateTask : public CBotTask
{
public:
	CBotSynBreakICrateTask(edict_t* pCrate, CBotWeapon* pWeapon) : m_pCrate(pCrate)
	{
		m_pWeapon = pWeapon;
		m_vPos = Vector(0, 0, 0);
	}
	void debugString(char* string, const unsigned bufferSize) override
	{
		snprintf(string, bufferSize, "Break Item Crate");
	}
	void execute(CBot* pBot, CBotSchedule* pSchedule) override;
private:
	CBotWeapon* m_pWeapon;
	MyEHandle m_pCrate;
	Vector m_vPos;
};

class CBotSynUseCharger : public CBotTask
{
public:
	CBotSynUseCharger(edict_t* pCharger, const int type) : m_pCharger(pCharger)
	{
		m_vPos = Vector(0, 0, 0);
		m_iType = type;
		m_flTime = engine->Time() + randomFloat(8.0f, 10.0f);
	}
	void debugString(char* string, const unsigned bufferSize) override
	{
		snprintf(string, bufferSize, "Use Charger");
	}
	void execute(CBot* pBot, CBotSchedule* pSchedule) override;
private:
	MyEHandle m_pCharger;
	Vector m_vPos;
	int m_iType;
	float m_flTime;
};

/*
class CAttackTask : public CBotTask
{
public:
	CAttackTask ( Vector vToAttack )
	{
		setVector(vToAttack);
	}

	CAttackTask ( edict_t *pToAttack )
	{
		setEdict(pToAttack);
	}

	void init ()
	{
		setInterrupt(CONDITION_OUT_OF_AMMO|CONDITION_NO_WEAPON);
	}

	virtual void execute ( CBot *pBot )
	{

	}
};

class CFindRunPath : public CBotTask
{
public:
	CFindRunPath ( Vector pGoto )
	{
		setVector(pGoto);
	}

	CFindRunPath ( edict_t *pGoto )
	{
		setEdict(pGoto);
		//setVector(pGoto->v.origin); ??
	}

	void init ()
	{
		setInterrupt(0);
	}

	virtual void execute ( CBot *pBot );
};*/
#endif