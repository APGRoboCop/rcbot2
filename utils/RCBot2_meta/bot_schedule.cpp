// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
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

#include "bot_schedule.h"
#include "bot.h"
#include "bot_client.h"
#include "bot_getprop.h"
#include "bot_globals.h"
#include "bot_task.h"
#include "bot_waypoint_locations.h"
#include "bot_weapons.h"
////////////////////////////////////
// these must match the SCHED IDs
const char *szSchedules[SCHED_MAX+1] = 
{
	"SCHED_NONE",
	"SCHED_ATTACK",
	"SCHED_RUN_FOR_COVER",
	"SCHED_GOTO_ORIGIN",
	"SCHED_GOOD_HIDE_SPOT",
	"SCHED_TF2_GET_FLAG",
	"SCHED_TF2_GET_HEALTH",
	"SCHED_TF_BUILD",
	"SCHED_HEAL",
	"SCHED_GET_METAL",
	"SCHED_SNIPE",
	"SCHED_UPGRADE",
	"SCHED_USE_TELE",
	"SCHED_SPY_SAP_BUILDING",
	"SCHED_USE_DISPENSER",
	"SCHED_PICKUP",
	"SCHED_TF2_GET_AMMO",
	"SCHED_TF2_FIND_FLAG",
	"SCHED_LOOKAFTERSENTRY",
	"SCHED_DEFEND",
	"SCHED_ATTACKPOINT",
	"SCHED_DEFENDPOINT",
	"SCHED_TF2_PUSH_PAYLOADBOMB",
	"SCHED_TF2_DEFEND_PAYLOADBOMB",
	"SCHED_TF2_DEMO_PIPETRAP",
	"SCHED_TF2_DEMO_PIPESENTRY",
	"SCHED_BACKSTAB",
	"SCHED_REMOVESAPPER",
	"SCHED_GOTONEST",
	"SCHED_MESSAROUND",
	"SCHED_TF2_ENGI_MOVE_BUILDING",
	"SCHED_FOLLOW_LAST_ENEMY",
	"SCHED_SHOOT_LAST_ENEMY_POS",
	"SCHED_GRAVGUN_PICKUP",
	"SCHED_HELP_PLAYER",
	"SCHED_BOMB",
	"SCHED_TF_SPYCHECK",
	"SCHED_FOLLOW",
	"SCHED_DOD_DROPAMMO",
	"SCHED_INVESTIGATE_NOISE",
	"SCHED_CROUCH_AND_HIDE",	
	"SCHED_DEPLOY_MACHINE_GUN",
	"SCHED_ATTACK_SENTRY_GUN",
	"SCHED_RETURN_TO_INTEL",
	"SCHED_INVESTIGATE_HIDE",
	"SCHED_TAUNT",
	"SCHED_SYN_OPEN_DOOR",
	"SCHED_SYN_DISARM_MINE",
	"SCHED_SYN_PLANT_MINE",
	"SCHED_SYN_BREAK_ICRATE",
	"SCHED_BUY",
	"SCHED_PLANT_BOMB",
	"SCHED_WAIT",
	"SCHED_WAIT_FOR_ENEMY",
	"SCHED_MAX"
};
////////////////////// unused
CBotTF2DemoPipeEnemySched :: CBotTF2DemoPipeEnemySched ( CBotWeapon *pLauncher, Vector vStand, edict_t *pEnemy )
{
	//addTask(new CFindPathTask(vStand));
	//addTask(new CBotTF2DemomanPipeEnemy(pLauncher,pEnemy));
}

void CBotTF2DemoPipeEnemySched :: init()
{
	setID(SCHED_TF2_DEMO_PIPEENEMY);
}
///////////////////

CBotTF2DemoPipeTrapSched :: CBotTF2DemoPipeTrapSched (const eDemoTrapType type, const Vector& vStand, const Vector& vLoc, const Vector& vSpread, const bool bAutoDetonate, const int wptarea)
{
	addTask(new CFindPathTask(vStand));
	addTask(new CBotTF2DemomanPipeTrap(type,vStand,vLoc,vSpread,bAutoDetonate,wptarea));
}

void CBotTF2DemoPipeTrapSched :: init()
{
	setID(SCHED_TF2_DEMO_PIPETRAP);
}

//////////////////////////////////////

CBotTF2HealSched::CBotTF2HealSched(edict_t *pHeal)
{
	CFindPathTask *findpath = new CFindPathTask(pHeal);
	findpath->setCompleteInterrupt(CONDITION_SEE_HEAL);
	addTask(findpath);
	addTask(new CBotTF2MedicHeal());
}

void CBotTF2HealSched::init()
{
	setID(SCHED_HEAL);
}

/////////////////////////////////////////////

CBotTFEngiBuild :: CBotTFEngiBuild (const CBot *pBot, const eEngiBuild iObject, CWaypoint *pWaypoint)
{
	CFindPathTask *pathtask = new CFindPathTask(CWaypoints::getWaypointIndex(pWaypoint));
	addTask(pathtask); // first

	pathtask->setInterruptFunction(std::make_unique<CBotTF2EngineerInterrupt>(pBot));

	addTask(new CBotTFEngiBuildTask(iObject,pWaypoint)); // second
}

void CBotTFEngiBuild :: init ()
{
	setID(SCHED_TF_BUILD);
}

//////////////////////////////////////////////

CBotGetMetalSched :: CBotGetMetalSched (const Vector& vOrigin)
{

	CFindPathTask *task1 = new CFindPathTask(vOrigin);
	CBotTF2WaitAmmoTask *task2 = new CBotTF2WaitAmmoTask(vOrigin);

	task1->setCompleteInterrupt(0,CONDITION_NEED_AMMO);
	task2->setCompleteInterrupt(0,CONDITION_NEED_AMMO);

	addTask(task1); // first
	addTask(task2);
}

void CBotGetMetalSched :: init ()
{
	setID(SCHED_GET_METAL);
}

//////////////////////////////////////////////

CBotEngiMoveBuilding::CBotEngiMoveBuilding(edict_t *pBotEdict, edict_t *pBuilding, const eEngiBuild iObject, const Vector& vNewLocation, const bool bCarrying)
{
	// not carrying
	if ( !bCarrying )
	{
		addTask(new CFindPathTask(pBuilding));
		addTask(new CBotTaskEngiPickupBuilding(pBuilding));
	}
	// otherwise already carrying

	addTask(new CFindPathTask(vNewLocation));
	addTask(new CBotTaskEngiPlaceBuilding(iObject,vNewLocation));
}

void CBotEngiMoveBuilding :: init ()
{
	setID(SCHED_TF2_ENGI_MOVE_BUILDING);
}

///////////////////////////////////////////

CBotTF2PushPayloadBombSched :: CBotTF2PushPayloadBombSched (edict_t * ePayloadBomb)
{
	addTask(new CFindPathTask(ePayloadBomb)); // first
	addTask(new CBotTF2PushPayloadBombTask(ePayloadBomb)); // second
}

void CBotTF2PushPayloadBombSched :: init ()
{
	setID(SCHED_TF2_PUSH_PAYLOADBOMB);
}

///////////////////////////////////

CBotTF2DefendPayloadBombSched :: CBotTF2DefendPayloadBombSched (edict_t * ePayloadBomb)
{
	addTask(new CFindPathTask(CBotGlobals::entityOrigin(ePayloadBomb))); // first
	addTask(new CBotTF2DefendPayloadBombTask(ePayloadBomb)); // second
}

void CBotTF2DefendPayloadBombSched :: init ()
{
	setID(SCHED_TF2_DEFEND_PAYLOADBOMB);
}

//////////////////////////////////////////////

CBotTFEngiUpgrade :: CBotTFEngiUpgrade (const CBot *pBot, edict_t *pBuilding)
{
	CFindPathTask *pathtask = new CFindPathTask(pBuilding);

	addTask(pathtask);

	pathtask->completeInRangeFromEdict();
	pathtask->failIfTaskEdictDead();
	pathtask->setRange(150.0f);

	if ( !CTeamFortress2Mod::isSentryGun(pBuilding) )
	{
		pathtask->setInterruptFunction(std::make_unique<CBotTF2EngineerInterrupt>(pBot));

		CBotTF2UpgradeBuilding *upgbuilding = new CBotTF2UpgradeBuilding(pBuilding);
		addTask(upgbuilding);
		upgbuilding->setInterruptFunction(std::make_unique<CBotTF2EngineerInterrupt>(pBot));

	}
	else
	{
		addTask(new CBotTF2UpgradeBuilding(pBuilding));
	}
}

void CBotTFEngiUpgrade :: init ()
{
	setID(SCHED_UPGRADE);
}

//////////////////////////////////////////////////
CBotBackstabSched :: CBotBackstabSched ( edict_t *pEnemy )
{
	Vector vangles;

	AngleVectors(CBotGlobals::entityEyeAngles(pEnemy),&vangles);
	const Vector vrear = CBotGlobals::entityOrigin(pEnemy) - vangles * 45 + Vector(0, 0, 32);

	addTask(new CFindPathTask(vrear));
	addTask(new CBotBackstab(pEnemy));
}

void CBotBackstabSched :: init ()
{
	setID(SCHED_BACKSTAB);
}

///////////


CBotTF2SnipeCrossBowSched::CBotTF2SnipeCrossBowSched(const Vector& vOrigin, const int iWpt)
{
	CBotTask *pFindPath = new CFindPathTask(iWpt);
	CBotTask *pSnipeTask = new CBotTF2SnipeCrossBow(vOrigin, iWpt);

	addTask(pFindPath); // first
	addTask(pSnipeTask); // second

	pFindPath->setFailInterrupt(CONDITION_PARANOID);
	pSnipeTask->setFailInterrupt(CONDITION_PARANOID);
}

void CBotTF2SnipeCrossBowSched::init()
{
	setID(SCHED_SNIPE);
}

CBotTF2SnipeSched :: CBotTF2SnipeSched (const Vector& vOrigin, const int iWpt)
{
	CBotTask *pFindPath = new CFindPathTask(iWpt);
	CBotTask *pSnipeTask = new CBotTF2Snipe(vOrigin,iWpt);

	addTask(pFindPath); // first
	addTask(pSnipeTask); // second

	pFindPath->setFailInterrupt(CONDITION_PARANOID);
	pSnipeTask->setFailInterrupt(CONDITION_PARANOID);
}

void CBotTF2SnipeSched :: init ()
{
	setID(SCHED_SNIPE);
}
/////////

CBotTFEngiLookAfterSentry :: CBotTFEngiLookAfterSentry ( edict_t *pSentry )
{
	addTask(new CFindPathTask(pSentry)); // first
	addTask(new CBotTF2EngiLookAfter(pSentry)); // second
}

void CBotTFEngiLookAfterSentry :: init ()
{
	setID(SCHED_LOOKAFTERSENTRY);
}

////////////
CBotTF2GetHealthSched :: CBotTF2GetHealthSched (const Vector& vOrigin)
{
	CFindPathTask *task1 = new CFindPathTask(vOrigin);
	CBotTF2WaitHealthTask *task2 = new CBotTF2WaitHealthTask(vOrigin);

	// if bot doesn't have need ammo flag anymore ....
	// fail so that the bot doesn't move onto the next task
	task1->setCompleteInterrupt(0,CONDITION_NEED_HEALTH);
	task2->setCompleteInterrupt(0,CONDITION_NEED_HEALTH);

	addTask(task1); // first
	addTask(task2); // second
}

void CBotTF2GetHealthSched :: init ()
{
	setID(SCHED_TF2_GET_HEALTH);
}

///////////////////////////////////////

CBotTF2GetAmmoSched :: CBotTF2GetAmmoSched (const Vector& vOrigin)
{
	CFindPathTask *task1 = new CFindPathTask(vOrigin);
	CBotTF2WaitAmmoTask *task2 = new CBotTF2WaitAmmoTask(vOrigin);

	// if bot doesn't have need ammo flag anymore ....
	// fail so that the bot doesn't move onto the next task
	task1->setCompleteInterrupt(0,CONDITION_NEED_AMMO);
	task2->setCompleteInterrupt(0,CONDITION_NEED_AMMO);

	addTask(task1); // first
	addTask(task2); // second
}

void CBotTF2GetAmmoSched ::  init ()
{
	setID(SCHED_TF2_GET_AMMO);
}

//////////////////////////////////////////////
CBotTF2GetFlagSched :: CBotTF2GetFlagSched (const Vector& vOrigin, const bool bUseRoute, const Vector& vRoute)
{
	if ( bUseRoute )
		addTask(new CFindPathTask(vRoute));

	addTask(new CFindPathTask(vOrigin)); // first
	addTask(new CBotTF2WaitFlagTask(vOrigin)); // second
}

void CBotTF2GetFlagSched :: init ()
{
	setID(SCHED_TF2_GET_FLAG);
}

///////////////////////////////////////////
CBotUseTeleSched :: CBotUseTeleSched ( edict_t *pTele )
{
	// find path
	addTask(new CFindPathTask(pTele)); // first
	addTask(new CBotTFUseTeleporter(pTele)); // second
}
void CBotUseTeleSched :: init ()
{
	setID(SCHED_USE_TELE);
}

//////////////////////////////////////////

CBotUseDispSched :: CBotUseDispSched (const CBot *pBot, edict_t *pDisp)//, bool bNest )
{
	CFindPathTask *pathtask = new CFindPathTask(pDisp);
	CBotTF2WaitHealthTask *gethealth = new CBotTF2WaitHealthTask(CBotGlobals::entityOrigin(pDisp));
	addTask(pathtask);
	pathtask->setInterruptFunction(std::make_unique<CBotTF2EngineerInterrupt>(pBot));

	addTask(gethealth); // second
	gethealth->setInterruptFunction(std::make_unique<CBotTF2EngineerInterrupt>(pBot));

	//if ( bNest )
	//	addTask(new CBotNest()); // third
}

void CBotUseDispSched :: init ()
{
	setID(SCHED_USE_DISPENSER);
}

////////////////////////////////////////////
void CBotSpySapBuildingSched :: init ()
{
	setID(SCHED_SPY_SAP_BUILDING);
}

CBotSpySapBuildingSched :: CBotSpySapBuildingSched ( edict_t *pBuilding, const eEngiBuild id )
{
	CFindPathTask *findpath = new CFindPathTask(pBuilding);

	addTask(findpath); // first
	addTask(new CBotTF2SpySap(pBuilding,id)); // second

	findpath->setDangerPoint(CWaypointLocations::NearestWaypoint(CBotGlobals::entityOrigin(pBuilding),200.0f,-1));
}
//////////////////////////////////////
CBotTauntSchedule :: CBotTauntSchedule ( edict_t *pPlayer, const float fYaw ) : m_pPlayer(pPlayer)
{
	const QAngle angles = QAngle(0,fYaw,0);
	Vector forward;
	constexpr float fTauntDist = 40.0f;

	m_fYaw = 180 - fYaw;

	AngleVectors(angles,&forward);

	//forward = forward/forward.Length();
	const Vector vOrigin = CBotGlobals::entityOrigin(pPlayer);

	const Vector vGoto = vOrigin + forward * fTauntDist;

	CBotGlobals::fixFloatAngle(&m_fYaw);

	addTask(new CFindPathTask(vGoto));
	addTask(new CMoveToTask(vGoto));
	addTask(new CTF2_TauntTask(pPlayer, vGoto, 5.0f));
}

void CBotTauntSchedule :: init ()
{
	setID(SCHED_TAUNT);
}


///////////////////////////////////////////
CBotTF2FindFlagSched :: CBotTF2FindFlagSched (const Vector& vOrigin)
{
	addTask(new CFindPathTask(vOrigin)); // first
	addTask(new CBotTF2WaitFlagTask(vOrigin,true)); // second
}

void CBotTF2FindFlagSched :: init ()
{
	setID(SCHED_TF2_FIND_FLAG);
}
/////////////////////////////////////////////
CBotPickupSched::CBotPickupSched( edict_t *pEdict )
{
	addTask(new CFindPathTask(pEdict));	
	addTask(new CMoveToTask(pEdict));
}

void CBotPickupSched :: init ()
{
	setID(SCHED_PICKUP);
}

CBotPickupSchedUse::CBotPickupSchedUse( edict_t *pEdict )
{
	addTask(new CFindPathTask(pEdict));	
	addTask(new CMoveToTask(pEdict));
	addTask(new CBotHL2DMUseButton(pEdict));
}

void CBotPickupSchedUse :: init ()
{
	setID(SCHED_PICKUP);

}
/////////////////////////////////////////////
CBotInvestigateNoiseSched::CBotInvestigateNoiseSched (const Vector& vSource, const Vector& vAttackPoint)
{
	addTask(new CFindPathTask(vSource,LOOK_NOISE));	
	addTask(new CBotInvestigateTask(vSource,200.0f,vAttackPoint,true,3.0f));
}

void CBotInvestigateNoiseSched::init ()
{
	setID(SCHED_INVESTIGATE_NOISE);
}

////////////////////////////////////////////////
CBotGotoOriginSched :: CBotGotoOriginSched (const Vector& vOrigin)
{
	addTask(new CFindPathTask(vOrigin)); // first
	addTask(new CMoveToTask(vOrigin)); // second
}

CBotGotoOriginSched :: CBotGotoOriginSched ( edict_t *pEdict )
{
	addTask(new CFindPathTask(pEdict));	
	addTask(new CMoveToTask(pEdict));
}

void CBotGotoOriginSched :: init ()
{
	setID(SCHED_GOTO_ORIGIN);
}

///////////////////////////////////////
CBotDefendSched ::CBotDefendSched (const Vector& vOrigin, const float fMaxTime)
{
	addTask(new CFindPathTask(vOrigin));
	addTask(new CBotDefendTask(vOrigin,fMaxTime));
}

CBotDefendSched::CBotDefendSched (const int iWaypointID, const float fMaxTime)
{
	CWaypoint* pWaypoint = CWaypoints::getWaypoint(iWaypointID);

	addTask(new CFindPathTask(iWaypointID));
	addTask(new CBotDefendTask(pWaypoint->getOrigin(),fMaxTime,8,false,Vector(0,0,0),LOOK_SNIPE,pWaypoint->getFlags()));
}

void CBotDefendSched :: init ()
{
	setID(SCHED_DEFEND);
}

//////

CBotRemoveSapperSched :: CBotRemoveSapperSched ( edict_t *pBuilding, const eEngiBuild id )
{
	CFindPathTask *pathtask = new CFindPathTask(pBuilding);
	addTask(pathtask);
	pathtask->completeInRangeFromEdict();
	pathtask->setRange(150.0f);
	addTask(new CBotRemoveSapper(pBuilding,id));
}

void CBotRemoveSapperSched :: init ()
{
	setID(SCHED_REMOVESAPPER);
}
///////////
CGotoHideSpotSched :: CGotoHideSpotSched ( CBot *pBot, edict_t *pEdict, const bool bIsGrenade )
{
	// run at flank while shooting	
	CFindPathTask *pHideGoalPoint = new CFindPathTask(pEdict);

	pBot->setCoverFrom(pEdict);
	addTask(new CFindGoodHideSpot(pEdict));
	addTask(pHideGoalPoint);
	if ( bIsGrenade )
		addTask(new CDODWaitForGrenadeTask(pEdict));

	// don't need to hide if the player we're hiding from died while we're running away
	pHideGoalPoint->failIfTaskEdictDead();
	pHideGoalPoint->setLookTask(LOOK_WAYPOINT);
	// no interrupts, should be a quick waypoint path anyway
	pHideGoalPoint->setNoInterruptions();
	// get vector from good hide spot task
	pHideGoalPoint->getPassedVector();
	pHideGoalPoint->dontGoToEdict();
	if ( bIsGrenade )
	{
		pHideGoalPoint->setRange(BLAST_RADIUS+100.0f);
		pHideGoalPoint->completeOutOfRangeFromEdict();
	}
}

CGotoHideSpotSched :: CGotoHideSpotSched (CBot *pBot, const Vector& vOrigin, IBotTaskInterrupt *interrupt)
{
	// run at flank while shooting	
	CFindPathTask *pHideGoalPoint = new CFindPathTask();
	
	pBot->setCoverFrom(nullptr);
	addTask(new CFindGoodHideSpot(vOrigin));
	addTask(pHideGoalPoint);

	// no interrupts, should be a quick waypoint path anyway
	pHideGoalPoint->setNoInterruptions();
	pHideGoalPoint->setInterruptFunction(std::unique_ptr<IBotTaskInterrupt>(interrupt));
	// get vector from good hide spot task
	pHideGoalPoint->getPassedVector();
}

void CGotoHideSpotSched :: init ()
{
	setID(SCHED_GOOD_HIDE_SPOT);
}
///////////////
CGotoNestSched :: CGotoNestSched (int iWaypoint)
{
	//addTask(new CFindGoodHideSpot(1));
	//addTask(new CNestTask());
}
void CGotoNestSched :: init ()
{
	setID(SCHED_GOTONEST);
}

//////////////
CCrouchHideSched :: CCrouchHideSched ( edict_t *pCoverFrom )
{
	addTask(new CCrouchHideTask(pCoverFrom));
}

void CCrouchHideSched :: init ()
{
	setID(SCHED_CROUCH_AND_HIDE);
}

/////////////
CBotTF2AttackSentryGun::CBotTF2AttackSentryGun( edict_t *pSentry, CBotWeapon *pWeapon )
{
	CFindPathTask *path = new CFindPathTask(pSentry);

	addTask(path);
	addTask(new CBotTF2AttackSentryGunTask(pSentry,pWeapon));

	path->completeInRangeFromEdict();
	path->completeIfSeeTaskEdict();

	path->setRange(pWeapon->primaryMaxRange() - 100);
}

/////////////
CBotAttackSched :: CBotAttackSched ( edict_t *pEdict )
{
	addTask(new CAttackEntityTask(pEdict));
	//addTask(new CFindGoodHideSpot(pEdict));
}

void CBotAttackSched :: init ()
{
	setID(SCHED_ATTACK);
}
///////////////////////////////////////////
CBotAttackPointSched :: CBotAttackPointSched (const Vector& vPoint, const float fRadius, const int iArea, const bool bHasRoute, const Vector& vRoute, const bool bNest, edict_t *pLastEnemySentry)
{
	int iDangerWpt = -1;

	if ( pLastEnemySentry != nullptr)
		iDangerWpt = CWaypointLocations::NearestWaypoint(CBotGlobals::entityOrigin(pLastEnemySentry),200.0f,-1,true,true);

	// First find random route 
	if ( bHasRoute )
	{
		CFindPathTask *toRoute = new CFindPathTask(vRoute);
		addTask(toRoute); // first
		toRoute->setDangerPoint(iDangerWpt);

		if ( bNest )
			addTask(new CBotNest());
	}

	CFindPathTask *toPoint = new CFindPathTask(vPoint);
	addTask(toPoint); // second / first
	toPoint->setDangerPoint(iDangerWpt);
	addTask(new CBotTF2AttackPoint(iArea,vPoint,fRadius)); // third / second 
}

void CBotAttackPointSched ::init ()
{	
	setID(SCHED_ATTACKPOINT);
}
///////////////
CBotTF2MessAroundSched :: CBotTF2MessAroundSched ( edict_t *pFriendly, const int iMaxVoiceCmd )
{
	addTask(new CMessAround(pFriendly,iMaxVoiceCmd));
}

void CBotTF2MessAroundSched :: init()
{
	setID(SCHED_MESSAROUND);
}
////////////////////////////////////////////////

CBotFollowLastEnemy::CBotFollowLastEnemy(CBot* pBot, edict_t* pEnemy, const Vector& vLastSee)
{
	Vector vVelocity = Vector(0, 0, 0);
	CClient* pClient = CClients::get(pEnemy);

	CFindPathTask* pFindPath = new CFindPathTask(vLastSee, LOOK_LAST_ENEMY);

	if (CClassInterface::getVelocity(pEnemy, &vVelocity))
	{
		if (pClient && vVelocity == Vector(0, 0, 0))
			vVelocity = pClient->getVelocity();
	}
	else
	{
		if (pClient)
			vVelocity = pClient->getVelocity();
	}
	pFindPath->setCompleteInterrupt(CONDITION_SEE_CUR_ENEMY);

	addTask(pFindPath);

	/*if ( pBot->isTF2() )
	{
		int playerclass = ((CBotTF2*)pBot)->getClass();

		if ( ( playerclass == TF_CLASS_SOLDIER ) || (playerclass == TF_CLASS_DEMOMAN) )
			addTask(new CBotTF2ShootLastEnemyPosition(vLastSee,pEnemy,vVelocity));
	}*/

	addTask(new CFindLastEnemy(vLastSee, vVelocity));

	//////////////
	pFindPath->setNoInterruptions();
}
///////////////////////////////////////////////////
void CBotFollowLastEnemy :: init ()
{
	setID(SCHED_FOLLOW_LAST_ENEMY);
}
 

CBotTF2ShootLastEnemyPos::CBotTF2ShootLastEnemyPos (const Vector& vLastSeeEnemyPos, const Vector& vVelocity, edict_t *pLastEnemy)
{
	addTask(new CBotTF2ShootLastEnemyPosition(vLastSeeEnemyPos,pLastEnemy,vVelocity));
}

void CBotTF2ShootLastEnemyPos::init()
{
	setID(SCHED_SHOOT_LAST_ENEMY_POS);
}

///////////////////////////////
CDeployMachineGunSched :: CDeployMachineGunSched (CBotWeapon *pWeapon, CWaypoint *pWaypoint, const Vector& vEnemy)
{
	addTask(new CFindPathTask(CWaypoints::getWaypointIndex(pWaypoint),LOOK_LAST_ENEMY));
	addTask(new CBotDODSnipe(pWeapon,pWaypoint->getOrigin(),pWaypoint->getAimYaw(),true,vEnemy.z,pWaypoint->getFlags()));
}
//////////////////////////////////////////////////
CBotDefendPointSched ::	CBotDefendPointSched (const Vector& vPoint, const float fRadius, const int iArea)
{
	addTask(new CFindPathTask(vPoint)); // first
	addTask(new CBotTF2DefendPoint(iArea,vPoint,fRadius)); // second
}

void CBotDefendPointSched ::init ()
{
	setID(SCHED_DEFENDPOINT);
}

CSynOpenDoorSched::CSynOpenDoorSched(edict_t *pDoor)
{
	addTask(new CBotHL2DMUseButton(pDoor)); // The bot will already move to range with this task
	addTask(new CBotWaitTask(3.0f));
}

CSynDisarmMineSched::CSynDisarmMineSched(edict_t *pMine)
{
	addTask(new CBotSynDisarmMineTask(pMine));
}

CSynBreakICrateSched::CSynBreakICrateSched(edict_t* pCrate, CBotWeapon* pWeapon)
{
	CFindPathTask *pPathTask = new CFindPathTask(pCrate);
	pPathTask->setRange(96.0f);
	addTask(pPathTask);
	addTask(new CMoveToTask(pCrate));
	addTask(new CBotSynBreakICrateTask(pCrate, pWeapon));
}

/********************************
 *    Counter-Strike: Source    *
 ********************************/
CCSSPlantBombSched::CCSSPlantBombSched(CWaypoint *pWaypoint, CWaypoint *pRoute)
{
	if(pRoute)
	{
		CBotTask *pRouteTask = new CFindPathTask(CWaypoints::getWaypointIndex(pRoute));
		pRouteTask->setFailInterrupt(CONDITION_SEE_CUR_ENEMY);
		addTask(pRouteTask);
	}

	CBotTask *pTask = new CFindPathTask(CWaypoints::getWaypointIndex(pWaypoint));
	pTask->setFailInterrupt(CONDITION_SEE_CUR_ENEMY);
	addTask(pTask);
	addTask(new CMoveToTask(pWaypoint->getOrigin()));
	addTask(new CCSSPlantTheBombTask());
}

/////////////////////////////////////////////
void CBotSchedule :: execute ( CBot *pBot )
{
	// current task
	static CBotTask *pTask;
	static eTaskState iState;

	if ( m_Tasks.empty() )
	{
		m_bFailed = true;
		return;
	}

	// why would task ever be null??
	pTask = m_Tasks.front();

	if ( pTask == nullptr)
	{
		m_bFailed = true;
		return;
	}

	iState = pTask->isInterrupted(pBot);

	if ( iState == STATE_FAIL )
		pTask->fail();
	else if ( iState == STATE_COMPLETE )
		pTask->complete();
	else // still running
	{
		// timed out ??
		if ( pTask->timedOut() )
			pTask->fail(); // fail
		else
		{			
			if ( CClients::clientsDebugging(BOT_DEBUG_TASK) )
			{
				char dbg[512];

				pTask->debugString(dbg, {});

				CClients::clientDebugMsg(BOT_DEBUG_TASK,dbg,pBot);
			}

			pTask->execute(pBot,this); // run
		}
	}

	if ( pTask->hasFailed() )
	{
		m_bFailed = true;
	}
	else if ( pTask->isComplete() )
	{
		removeTop();
	}
}

void CBotSchedule :: addTask ( CBotTask *pTask )
{
	// initialize
	pTask->init();
	// add
	m_Tasks.emplace_back(pTask);
}

void CBotSchedule :: removeTop ()
{
	const CBotTask *pTask = m_Tasks.front();
	m_Tasks.pop_front();
	delete pTask;
}

const char *CBotSchedule :: getIDString () const
{
	return szSchedules[m_iSchedId];
}

/////////////////////

CBotSchedule :: CBotSchedule ()
{
	_init();
}

void CBotSchedule :: _init ()
{
	m_bFailed = false;
	m_bitsPass = 0;		
	m_iSchedId = SCHED_NONE;

	// pass information
	iPass = 0;
	fPass = 0;
	vPass = Vector(0,0,0);
	pPass = nullptr;	

	init();
}

void CBotSchedule :: passInt(const int i)
{
	iPass = i;
	m_bitsPass |= BITS_SCHED_PASS_INT;
}
void CBotSchedule :: passFloat(const float f)
{
	fPass = f;
	m_bitsPass |= BITS_SCHED_PASS_FLOAT;
}
void CBotSchedule :: passVector(const Vector& v)
{
	vPass = v;
	m_bitsPass |= BITS_SCHED_PASS_VECTOR;
}
void CBotSchedule :: passEdict(edict_t *p)
{
	pPass = p;
	m_bitsPass |= BITS_SCHED_PASS_EDICT;
}
////////////////////