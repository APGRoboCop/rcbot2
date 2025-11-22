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
//============================================================================//
//
// HPB_bot2.cpp - bot source code file (Copyright 2004, Jeffrey "botman" Broome)
//
//============================================================================//

#include <cstdio>
#include <cstring>
#include <cmath>

#include "mathlib.h"
#include "vector.h"
#include "eiface.h"

#ifdef __linux__
#include "shareddefs.h" //bir3yk
#endif

#include "in_buttons.h"
#include "ndebugoverlay.h"

#ifdef __linux__
#include "shake.h"    //bir3yk
#endif

#include "bot.h"
#include "bot_cvars.h"
#include "bot_schedule.h"
#include "bot_buttons.h"
#include "bot_navigator.h"
//#include "bot_black_mesa.h"
#include "bot_css_bot.h"
#include "bot_coop.h"
#include "bot_zombie.h"
#include "bot_dod_bot.h"
#include "bot_hldm_bot.h"
#include "bot_hl1dmsrc_bot.h"
#include "bot_fortress.h"
#include "bot_synergy.h"
#include "bot_visibles.h"
#include "bot_weapons.h"
#include "bot_profile.h"
#include "bot_waypoint_locations.h"
#include "bot_waypoint.h"
#include "bot_squads.h"

#include "bot_mtrand.h"

#include "bot_getprop.h"
#include "bot_profiling.h"

#include "rcbot/logging.h"

#include <algorithm>
#include <random>
#include <vector>

//caxanga334: SDK 2013 contains macros for std::min and std::max which causes errors when compiling
//#if SOURCE_ENGINE == SE_SDK2013 || SOURCE_ENGINE == SE_BMS
#include "valve_minmax_off.h"
//#endif

// SourceMod event integration
#include "../../sm_ext/bot_sm_events.h"

constexpr float DEG_TO_RAD(const float x) { return x * 0.0174533f; }
constexpr float RAD_TO_DEG(const float x) { return x * 57.29578f; }

//extern void HookPlayerRunCommand ( edict_t *edict );

// instantiate bots -- make different for different mods
CBot **CBots::m_Bots = nullptr;

const float CBot :: m_fAttackLowestHoldTime = 0.1f;
const float CBot :: m_fAttackHighestHoldTime = 0.6f;
const float CBot :: m_fAttackLowestLetGoTime = 0.1f;
const float CBot :: m_fAttackHighestLetGoTime = 0.5f;

int CBots :: m_iMaxBots = -1;
int CBots :: m_iMinBots = -1;
// add or kick bot time
float  CBots :: m_flAddKickBotTime = 0;

#define TICK_INTERVAL			(gpGlobals->interval_per_tick)
#define TIME_TO_TICKS( dt )		( (int)( 0.5f + (float)(dt) / TICK_INTERVAL ) )

extern IVDebugOverlay *debugoverlay;

const char *g_szLookTaskToString[LOOK_MAX] = 
{
	"LOOK_NONE",
	"LOOK_VECTOR",
	"LOOK_WAYPOINT",
	"LOOK_WAYPOINT_NEXT_ONLY",
	"LOOK_AROUND",
	"LOOK_ENEMY",
	"LOOK_LAST_ENEMY",
	"LOOK_HURT_ORIGIN",
	"LOOK_EDICT",
	"LOOK_GROUND",
	"LOOK_SNIPE",
	"LOOK_WAYPOINT_AIM",
	"LOOK_BUILD",
	"LOOK_NOISE",
};

// Borrowed from RCBot1
bool BotFunc_BreakableIsEnemy ( edict_t *pBreakable, edict_t *pEdict )
{
	const int flags = CClassInterface::getPlayerFlags(pBreakable);

	// i. explosives required to blow breakable
	// ii. OR is not a world brush (non breakable) and can be broken by shooting
	if ( !(flags & FL_WORLDBRUSH) )
	{
		const Vector vSize = pBreakable->GetCollideable()->OBBMaxs() - pBreakable->GetCollideable()->OBBMins();
		const Vector vMySize = pEdict->GetCollideable()->OBBMaxs() - pEdict->GetCollideable()->OBBMins();
		
		if ( vSize.x >= vMySize.x ||
			vSize.y >= vMySize.y ||
			vSize.z >= vMySize.z/2 ) // this breakable could block my path
		{
			// 00000000001111111111222222222233333333334
			// 01234567890123456789012345678901234567890
			// models/props_c17/oildrum001_explosive.mdl
			const char *model = pBreakable->GetIServerEntity()->GetModelName().ToCStr();

			if ( model[13] == 'c' && model[17] == 'o' && model[20] == 'd' && model[28]== 'e' ) // explosive
				return false;
			// Only shoot breakables that are bigger than me (crouch size)
			// or that target something...
			return CClassInterface::getPlayerHealth(pBreakable)<1000; // breakable still visible (not broken yet)
		}
	}

	return false;
}

///////////////////////////////////////
// voice commands
////////////////////////////////////////////////
void CBroadcastVoiceCommand :: execute ( CBot *pBot )
{
	if ( !m_pPlayer )
		return;

	if ( m_pPlayer == pBot->getEdict() )
		return;

	if ( pBot->isEnemy(m_pPlayer,false) )
	{
		// listen to enemy voice commands if they are nearby
		if ( pBot->wantToListen() && pBot->wantToListenToPlayerAttack(m_pPlayer) && pBot->distanceFrom(m_pPlayer) < CWaypointLocations::REACHABLE_RANGE )
			pBot->listenToPlayer(m_pPlayer,true,false);
	}
	else
		pBot->hearVoiceCommand(m_pPlayer,m_VoiceCmd);
}
///////////////////////////////////////
void CBot::runPlayerMove()
{
	const int cmdnumbr = cmd.command_number + 1;

	//////////////////////////////////
	Q_memset(&cmd, 0, sizeof cmd);
	//////////////////////////////////

	if (rcbot_dont_move.GetBool())
	{
		cmd.forwardmove = 0;
		cmd.sidemove = 0;
	}
	else
	{
		cmd.forwardmove = m_fForwardSpeed;
		cmd.sidemove = m_fSideSpeed;
		cmd.upmove = m_fUpSpeed;
	}

	cmd.buttons = m_iButtons;
	cmd.impulse = m_iImpulse;
	cmd.viewangles = m_vViewAngles;
	cmd.weaponselect = m_iSelectWeapon;
	cmd.tick_count = gpGlobals->tickcount;
	cmd.command_number = cmdnumbr;

	if (bot_attack.GetInt() == 1) {
		cmd.buttons = IN_ATTACK;
	}

	m_iSelectWeapon = 0;
	m_iImpulse = 0;

	if (CClients::clientsDebugging(BOT_DEBUG_BUTTONS))
	{
		char dbg[512];

		snprintf(dbg, sizeof(dbg), "m_pButtons = %d/%x, Weapon Select = %d, impulse = %d", cmd.buttons, cmd.buttons, cmd.weaponselect, cmd.impulse);

		CClients::clientDebugMsg(BOT_DEBUG_BUTTONS, dbg, this);
	}

#ifndef OVERRIDE_RUNCMD
	// Controlling will be done in the RCBotPluginMeta::Hook_PlayerRunCmd hook if controlling puppet bots
	// see bot_plugin_meta.cpp
	m_pController->RunPlayerMove(&cmd);
#endif
}

bool CBot :: startGame ()
{
	return true;
}

// returns true if offset has been applied when not before
bool CBot :: walkingTowardsWaypoint ( CWaypoint *pWaypoint, bool *bOffsetApplied, Vector &vOffset )
{
	if ( pWaypoint->hasFlag(CWaypointTypes::W_FL_CROUCH) )
	{
		duck(true);
	}
	
	if ( pWaypoint->hasFlag(CWaypointTypes::W_FL_LIFT) )
	{
		updateCondition(CONDITION_LIFT);
	}
	else
	{
		removeCondition(CONDITION_LIFT);
	}

	/* //TODO: Add bot condition when on ladders? [APG]RoboCop[CL]
	if (pWaypoint->hasFlag(CWaypointTypes::W_FL_LADDER))
	{
		if (onLadder())
		{
			updateCondition(CONDITION_LADDER);
		}
		else
		{
			removeCondition(CONDITION_LADDER);
		}
	}*/

	if ( !*bOffsetApplied )
	{
		const float fRadius = pWaypoint->getRadius();

		if ( fRadius > 0 )
			vOffset = Vector(randomFloat(-fRadius,fRadius),randomFloat(-fRadius,fRadius),0);
		else
			vOffset = Vector(0,0,0);

		*bOffsetApplied = true;

		return true;

		/*if ( CClients::clientsDebugging(BOT_DEBUG_NAV) )
		{
			debugoverlay->AddLineOverlay(m_pBot->getOrigin(),pWaypoint->getOrigin() + m_vOffset,255,255,0,true,5.0f);
		}*/
	}

	return false;
}

void CBot :: setEdict (edict_t *pEdict)
{
	m_pEdict = pEdict;
	m_bUsed = true;
	m_szBotName[0] = 0;

	if ( m_pEdict )
	{
		m_pPlayerInfo = playerinfomanager->GetPlayerInfo(m_pEdict);
		m_pController = g_pBotManager->GetBotController(m_pEdict);		
		std::strncpy(m_szBotName,m_pPlayerInfo->GetName(),63);
		m_szBotName[63]=0;
	}
	else
	{
		return;
	}

	spawnInit();
}

bool CBot :: isUnderWater () const //Needs to be const to prevent losing mobility? [APG]RoboCop[CL]
{
	return CClassInterface::getWaterLevel(m_pEdict) > 1; //m_pController->IsEFlagSet(EFL_TOUCHING_FLUID);
}

// return false if there is a problem
bool CBot :: createBotFromEdict(edict_t *pEdict, CBotProfile *pProfile)
{
	char szModel[128];

	init();
	setEdict(pEdict);
	setup();
	m_fTimeCreated = engine->Time();

	/////////////////////////////

	m_pProfile = pProfile;

	if (!pEdict || !pProfile) {
		logger->Log(LogLevel::ERROR, "Invalid edict or profile");
		return false;
	}

	logger->Log(LogLevel::TRACE, "===================================");
	logger->Log(LogLevel::TRACE, "Creating Bot: %s", m_pProfile->m_szName);
	logger->Log(LogLevel::TRACE, "AimSkill: %f", m_pProfile->m_fAimSkill);
	logger->Log(LogLevel::TRACE, "Braveness: %f", m_pProfile->m_fBraveness);
	logger->Log(LogLevel::TRACE, "PathTicks: %d", m_pProfile->m_iPathTicks);
	logger->Log(LogLevel::TRACE, "Sensitivity: %d", m_pProfile->m_iSensitivity);
	logger->Log(LogLevel::TRACE, "VisionTicks: %d", m_pProfile->m_iVisionTicks);
	logger->Log(LogLevel::TRACE, "VisionTicksClients: %d", m_pProfile->m_iVisionTicksClients);
	logger->Log(LogLevel::TRACE, "===================================");

	engine->SetFakeClientConVarValue(pEdict,"cl_team","default");
	engine->SetFakeClientConVarValue(pEdict,"cl_defaultweapon","pistol");
	engine->SetFakeClientConVarValue(pEdict,"cl_autowepswitch","1");	
	engine->SetFakeClientConVarValue(pEdict,"tf_medigun_autoheal","0");	

	// joining name not the same as the profile name, change name
	if (std::strcmp(m_szBotName,pProfile->m_szName) != 0)
	{
		engine->SetFakeClientConVarValue(pEdict,"name",pProfile->m_szName);
		std::strcpy(m_szBotName,pProfile->m_szName);
	}

	if ( m_pPlayerInfo && pProfile->m_iTeam != -1 )
		m_pPlayerInfo->ChangeTeam(pProfile->m_iTeam);

	/////////////////////////////
	// safe copy
	std::strncpy(szModel,pProfile->m_szModel,127);
	szModel[127] = 0;

	if ( FStrEq(szModel,"default") )	
	{
		const int iModel = randomInt(1,7);	

		if ( randomInt(0,1) )
			snprintf(szModel, sizeof(szModel), "models/humans/Group03/Male_0%d.mdl", iModel);
		else
			snprintf(szModel, sizeof(szModel), "models/humans/Group03/female_0%d.mdl", iModel);
	}

	m_iDesiredTeam = pProfile->m_iTeam;
	m_iDesiredClass = pProfile->m_iClass;

	engine->SetFakeClientConVarValue(pEdict,"cl_playermodel",szModel);
	engine->SetFakeClientConVarValue(pEdict,"hud_fastswitch","1");

	#if SOURCE_ENGINE == SE_TF2
	helpers->ClientCommand(pEdict, "jointeam auto");

	//"heavy" should be "heavyweapons" in TF2?
	//
	//the class names should be changed to the slot names [APG]RoboCop[CL]
	
	//char classNames[32][10] = {
	//"auto", "scout", "soldier", "pyro", "demoman", "heavy", "medic",
	//"engineer", "sniper", "spy",
	//};
	
	char command[32];

	//TODO: To reduce any instability issues that could cause crashes [APG]RoboCop[CL]
	if (m_iDesiredClass >= 0 && static_cast<unsigned>(m_iDesiredClass) < 10) {
		char classNames[10][10] = {
			"auto", "scout", "sniper", "soldier", "demoman", "medic", "heavy",
			"pyro", "spy", "engineer"
		};
		snprintf(command, sizeof(command), "joinclass %s", classNames[m_iDesiredClass]);
	}
	else {
		snprintf(command, sizeof(command), "joinclass auto");
	}
	
	helpers->ClientCommand(pEdict, command);
	#endif
	/////////////////////////////

	return true;
}

bool CBot :: FVisible (const Vector &vOrigin, edict_t *pDest) const
{
	//return CBotGlobals::isVisible(m_pEdict,getEyePosition(),vOrigin);
	// fix bots seeing through gates/doors
	return CBotGlobals::isVisibleHitAllExceptPlayer(m_pEdict,getEyePosition(),vOrigin,pDest);

}

bool CBot :: FVisible ( edict_t *pEdict, const bool bCheckHead )
{
	static Vector eye;

	// use special hit traceline for players so bots dont shoot through things 
	// For players -- do two tracelines -- one at the origin and one at the head (for headshots)
	if ( bCheckHead || pEdict == m_pEnemy || CBotGlobals::isPlayer(pEdict) )
	{
		// use this method to get origin -- quicker 
		const Vector vOrigin = pEdict->GetCollideable()->GetCollisionOrigin();
		const Vector vHead = vOrigin + Vector(0, 0, pEdict->GetCollideable()->OBBMaxs().z);

		if ( FVisible(vHead,pEdict) )
		{
			if ( m_pEnemy == pEdict )
			{
				updateCondition(CONDITION_SEE_ENEMY_HEAD);

				if ( FVisible(vOrigin,pEdict) )
					updateCondition(CONDITION_SEE_ENEMY_GROUND);
				else
					removeCondition(CONDITION_SEE_ENEMY_GROUND);
			}

			return true;
		}
		/*
#if defined(_DEBUG) && !defined(__linux__)
		else if ( CClients::clientsDebugging(BOT_DEBUG_VIS) && (CBotGlobals::getTraceResult()->m_pEnt != nullptr) )
		{
			extern IServerGameEnts *servergameents;

			edict_t *edict = servergameents->BaseEntityToEdict(CBotGlobals::getTraceResult()->m_pEnt);

			if ( edict )
				debugoverlay->AddTextOverlay(CBotGlobals::getTraceResult()->endpos,2.0f,"Traceline hit %s",edict->GetClassName());
		}
#endif */
		if ( m_pEnemy == pEdict )
		{
			if ( FVisible(vOrigin,pEdict) )
			{
				updateCondition(CONDITION_SEE_ENEMY_GROUND);
				return true;
			}
			removeCondition(CONDITION_SEE_ENEMY_GROUND);
			return false;
		}

		return FVisible(vOrigin,pEdict);
	}

	eye = getEyePosition();

	// use typical traceline for non players
	return CBotGlobals::isVisible(m_pEdict,eye,pEdict);//CBotGlobals::entityOrigin(pEdict)+Vector(0,0,50.0f));
}

inline QAngle CBot :: eyeAngles () const
{
	return CBotGlobals::playerAngles(m_pEdict);
}

Vector CBot :: getEyePosition () const
{
	Vector vOrigin;//'/ = getOrigin();
	//vOrigin.z = m_pPlayerInfo->GetPlayerMaxs().z;

	gameclients->ClientEarPosition(m_pEdict,&vOrigin);

	return vOrigin;
}

bool CBot :: checkStuck ()
{
	static float fTime;

	if ( !moveToIsValid() )
		return false;
	if ( rcbot_dont_move.GetBool() ) // bots not moving
		return false;
	if ( hasEnemy() )
		return false;

	fTime = engine->Time();

	if ( m_fLastWaypointVisible == 0.0f )
	{
		m_bFailNextMove = false;

		if ( !hasSomeConditions(CONDITION_SEE_WAYPOINT) )
			m_fLastWaypointVisible = fTime;
	}
	else
	{
		if ( hasSomeConditions(CONDITION_SEE_WAYPOINT) )
			m_fLastWaypointVisible = 0.0f;
		else
		{
			if ( m_fLastWaypointVisible + 2.0f < fTime )
			{
				m_fLastWaypointVisible = 0.0f;
				m_bFailNextMove = true;

				return true;
			}
		}
	}

	if (m_fWaypointStuckTime > 0.0f && m_fWaypointStuckTime < engine->Time())
	{
		m_bFailNextMove = true;
		m_fWaypointStuckTime = engine->Time() + randomFloat(15.0f, 20.0f);
	}

	if ( m_fCheckStuckTime > fTime )
		return m_bThinkStuck;

	if ( hasSomeConditions(CONDITION_LIFT) || onLadder())//fabs(m_vMoveTo.z - getOrigin().z) > 48 )
	{
		if ( m_vVelocity.z != 0.0f )
			return false;
	}

	const float fSpeed = m_vVelocity.Length();
	float fIdealSpeed = m_fIdealMoveSpeed;

	if ( m_pButtons->holdingButton(IN_DUCK) )
		fIdealSpeed /= 2;

	if ( fIdealSpeed == 0.0f )
	{
		m_bThinkStuck = false; // not stuck
		m_fPercentMoved = 1.0f;
	}
	else
	{
		// alpha percentage check
		m_fPercentMoved = m_fPercentMoved/2 + fSpeed/fIdealSpeed/2;

		if ( m_fPercentMoved < 0.1f )
		{
			m_bThinkStuck = true;
			m_fPercentMoved = 0.1f;

			m_pButtons->jump();
			m_pButtons->duck(0.25f,randomFloat(0.2f,0.4f));

			if ( m_fStrafeTime < engine->Time() )
			{
				reduceTouchDistance();

				if ( CBotGlobals::yawAngleFromEdict(m_pEdict,m_vMoveTo) > 0 )
					m_fSideSpeed = m_fIdealMoveSpeed/2;
				else
					m_fSideSpeed = -(m_fIdealMoveSpeed/2);

				/*
				CTraceFilterWorldAndPropsOnly filter;
				Vector vOrigin = getOrigin();
				QAngle v_eyeangles;
				Vector vForward;
				float left = 0;
				float right = 0;

				Vector vTr;

				v_eyeangles = eyeAngles();

				v_eyeangles.y -= 45;

				CBotGlobals::fixFloatAngle(&(v_eyeangles.y));

				VectorAngles(vForward,v_eyeangles);

				vTr = (vForward / vForward.Length()) * 128;

				CBotGlobals::traceLine(vOrigin,vOrigin + vTr,MASK_SOLID_BRUSHONLY,&filter);

				left = CBotGlobals::getTraceResult()->fraction;

				v_eyeangles.y += 90;

				CBotGlobals::fixFloatAngle(&(v_eyeangles.y));

				VectorAngles(vForward,v_eyeangles);

				vTr = (vForward / vForward.Length()) * 128;

				CBotGlobals::traceLine(vOrigin,vOrigin + vTr,MASK_SOLID_BRUSHONLY,&filter);

				right = CBotGlobals::getTraceResult()->fraction;



				if ( left > right )
					m_fSideSpeed = -(m_fIdealMoveSpeed/2);
				else
					m_fSideSpeed = m_fIdealMoveSpeed/2;

				m_fStrafeTime = engine->Time() + 2.0f;
				*/
				
			}


			m_fCheckStuckTime = engine->Time() + 2.04f;
		}
		else
			m_bThinkStuck = false;
	}

	return m_bThinkStuck;
}

bool CBot :: isVisible (const edict_t *pEdict) const
{
	return m_pVisibles->isVisible(pEdict);
}

bool CBot :: canAvoid ( edict_t *pEntity )
{
	if ( !CBotGlobals::entityIsValid(pEntity) )
		return false;
	if ( m_pEdict == pEntity ) // can't avoid self!!!
		return false;
	if ( m_pLookEdict == pEntity )
		return false;
	if ( m_pLastEnemy == pEntity )
		return false;

	const Vector vAvoidOrigin = CBotGlobals::entityOrigin(pEntity);

	const float distance = distanceFrom(vAvoidOrigin);

	if ( distance > 1 && distance < bot_avoid_radius.GetFloat() && std::fabs(getOrigin().z - vAvoidOrigin.z) < 32 )
	{
		const SolidType_t solid = pEntity->GetCollideable()->GetSolid() ;

		if ( solid == SOLID_BBOX || solid == SOLID_VPHYSICS )
		{			
			return isEnemy(pEntity,false);
		}
	}

	return false;
}

void CBot :: reachedCoverSpot (int flags)
{
	
}

// something now visiable or not visible anymore
bool CBot :: setVisible ( edict_t *pEntity, const bool bVisible )
{
	const bool bValid = CBotGlobals::entityIsValid(pEntity);

	if ( bValid && bVisible )
	{
		if ( canAvoid(pEntity) )
		{
			if ( m_pAvoidEntity.get()== nullptr || distanceFrom(pEntity) < distanceFrom(m_pAvoidEntity) )
					m_pAvoidEntity = pEntity;
		}
	}
	else
	{
		if ( m_pAvoidEntity == pEntity )
			m_pAvoidEntity = nullptr;
		if ( m_pEnemy == pEntity )
		{
			m_pLastEnemy = m_pEnemy;
		}
	}

	// return if entity is valid or not
	return bValid;
}

bool CBot :: isUsingProfile (const CBotProfile *pProfile) const
{
	return m_pProfile == pProfile;
}

void CBot :: currentlyDead ()
{
	/*if ( m_bNeedToInit )
	{
		spawnInit();
		m_bNeedToInit = false;
	}*/

	//attack();

	// keep updating until alive
	m_fSpawnTime = engine->Time();
}

CBotWeapon *CBot::getCurrentWeapon()
{
	return m_pWeapons->getActiveWeapon(m_pPlayerInfo->GetWeaponName());
}

void CBot :: selectWeaponName ( const char *szWeaponName ) const
{
	m_pController->SetActiveWeapon(szWeaponName);
}

CBotWeapon *CBot :: getBestWeapon (edict_t *pEnemy, const bool bAllowMelee, const bool bAllowMeleeFallback, const bool bMeleeOnly, const bool bExplosivesOnly ) const
{
	return m_pWeapons->getBestWeapon(pEnemy,bAllowMelee,bAllowMeleeFallback,bMeleeOnly,bExplosivesOnly);
}

bool CBot::isHoldingPrimaryAttack() const
{
	return m_pButtons->holdingButton(IN_ATTACK);
}

void CBot :: debugMsg (const int iLev, const char *szMsg) const
{
	if ( CClients::clientsDebugging () )
	{
		char szMsg2[512];

		snprintf(szMsg2, sizeof(szMsg2), "(%s):%s", m_pPlayerInfo->GetName(), szMsg);

		CClients::clientDebugMsg (iLev,szMsg2,this);
	}
}

void CBot::SquadInPosition ()
{
	if ( m_uSquadDetail.b1.said_in_position == false )
	{
		// say something here
		sayInPosition();
		m_uSquadDetail.b1.said_in_position = true;
	}
}

void CBot :: kill () const
{
	helpers->ClientCommand(m_pEdict,"kill\n");
}

void CBot :: think ()
{
	static float fTime;
	//static bool debug;
	//static bool battack;

	//debug = CClients::clientsDebugging(BOT_DEBUG_THINK);
	
	fTime = engine->Time();

//	Vector *pvVelocity;

	// important!!!
	//
	m_iLookPriority = 0;
	m_iMovePriority = 0;
	m_iMoveSpeedPriority = 0;
	
	// re-added
	if ( !CBotGlobals::entityIsValid(m_pEdict) || m_pPlayerInfo == nullptr)
	{
		m_pPlayerInfo = playerinfomanager->GetPlayerInfo(m_pEdict);
		logger->Log(LogLevel::INFO, "%s : m_pPlayerInfo = NULL; Waiting for player info...", m_szBotName);
		return;
	}

#ifdef _DEBUG
	if ( rcbot_debug_iglev.GetInt() != 1 )
	{
#endif
	//
	// if bot is not in game, start it!!!
	if ( !startGame() )
	{
		doButtons();
		return; // don't do anything just now
	}

	doButtons();
#ifdef _DEBUG
	}
#endif
	if ( !isAlive() )
	{/*
	 // Dont need this anymore!! events does it, woohoo!
		if ( m_bNeedToInit )
		{
			spawnInit();
			m_bNeedToInit = false;
		}*/
		currentlyDead();

		return;
	}
#ifdef _DEBUG
	if ( rcbot_debug_iglev.GetInt() != 2 )
	{
#endif
	checkDependantEntities();

	//m_bNeedToInit = true;

	doMove();
	doLook();
#ifdef _DEBUG
	}
#endif
	if ( m_fNextThink > fTime )
		return;

	m_pButtons->letGoAllButtons(false);

	m_fNextThink = fTime + 0.03f;

	if ( m_pWeapons )
	{
		// update carried weapons
		if ( m_pWeapons->update(overrideAmmoTypes()) )
		{
			m_pPrimaryWeapon = m_pWeapons->getPrimaryWeapon();
		}
	}

	/////////////////////////////

	//m_iFlags = CClassInterface::getFlags(m_pEdict);

	//if ( m_pController->IsEFlagSet(EFL_BOT_FROZEN)  || (m_iFlags & FL_FROZEN) )
	//{
	//	stopMoving(12);
	//	return;
	//}

	//////////////////////////////

	//m_pCurrentWeapon = m_pBaseCombatChar->GetActiveWeapon (); 
#ifdef _DEBUG
	if ( rcbot_debug_iglev.GetInt() != 3 )
	{
#endif
	m_pVisibles->updateVisibles();
#ifdef _DEBUG
	}

	if ( rcbot_debug_iglev.GetInt() != 4 )
	{
#endif
		if ( checkStuck() )
		{
			// look in the direction I'm going to see what I'm stuck on
			setLookAtTask(LOOK_WAYPOINT,randomFloat(2.0f,4.0f));
		}
#ifdef _DEBUG
	}
#endif
	// 
	m_bOpenFire = true;
	m_bWantToListen = true;
	m_bWantToChangeWeapon = true;

	if ( !rcbot_debug_notasks.GetBool() )
	{
#ifdef _DEBUG
	if ( rcbot_debug_iglev.GetInt() != 5 )
	{
#endif
	getTasks();	
	}
#ifdef _DEBUG
	}

	if ( rcbot_debug_iglev.GetInt() != 7 )
	{
#endif
	wantToInvestigateSound(true);
	setMoveLookPriority(MOVELOOK_TASK);
	m_pSchedules->execute(this);
	setMoveLookPriority(MOVELOOK_THINK);
#ifdef _DEBUG
	}
// fix -- put listening AFTER task executed, as m_bWantToListen may update
	if ( rcbot_debug_iglev.GetInt() != 6 )
	{
#endif
		if ( m_bWantToListen && !hasEnemy() && !hasSomeConditions(CONDITION_SEE_CUR_ENEMY) && m_fWantToListenTime<engine->Time() )
		{
			setMoveLookPriority(MOVELOOK_LISTEN);
			listenForPlayers();
			setMoveLookPriority(MOVELOOK_THINK);
		}
		else if ( hasEnemy() )
		{
			// got an enemy -- reset 
			m_PlayerListeningTo = MyEHandle(nullptr);
			m_fLookSetTime = 0.0f;
			m_fListenTime = 0.0f;
			m_bListenPositionValid = false;
			m_fWantToListenTime = engine->Time() + 1.0f;

			// is player
			if ( ENTINDEX(m_pEnemy.get()) <= gpGlobals->maxClients )
				m_fLastSeeEnemyPlayer = engine->Time();
		}

#ifdef _DEBUG
	}

	if ( rcbot_debug_iglev.GetInt() != 8 )
	{
#endif
	m_vGoal = m_pNavigator->getGoalOrigin();

	if ( m_pNavigator->hasNextPoint() )
	{
		m_pNavigator->updatePosition();
	}
	else
	{
		m_fWaypointStuckTime = 0.0f;
		stopMoving();		
		setLookAtTask(LOOK_AROUND);
	}
#ifdef _DEBUG
	}

	if ( rcbot_debug_iglev.GetInt() != 9 )
	{
#endif
	// update m_pEnemy with findEnemy()
	m_pOldEnemy = m_pEnemy;
	m_pEnemy = nullptr;

	if ( m_pOldEnemy )
		findEnemy(m_pOldEnemy); // any better enemies than this one?
	else
		findEnemy();
#ifdef _DEBUG
	}
#endif
	updateConditions();

#ifdef _DEBUG
	if ( !CClassInterface::getVelocity(m_pEdict,&m_vVelocity) )
	{
#endif
		if ( m_fUpdateOriginTime < fTime )
		{
			const Vector vOrigin = getOrigin();

			m_vVelocity = m_vLastOrigin-vOrigin;
			m_vLastOrigin = vOrigin;
			m_fUpdateOriginTime = fTime+1.0f;
		}
#ifdef _DEBUG
	}
#endif

	setMoveLookPriority(MOVELOOK_MODTHINK);
#ifdef _DEBUG
	if ( rcbot_debug_iglev.GetInt() != 10 )
	{
#endif
	modThink();
#ifdef _DEBUG
	}
#endif

#ifdef _DEBUG
	if ( rcbot_debug_iglev.GetInt() != 11 )
	{
#endif
		if ( m_fStatsTime < engine->Time() )
		{
			updateStatistics();
			m_fStatsTime = engine->Time() + 0.15f;
		}
#ifdef _DEBUG
	}
#endif
	

#ifdef _DEBUG
	if ( rcbot_debug_iglev.GetInt() != 12 )
	{
#endif
		if ( inSquad() && !isSquadLeader() )
		{
			if ( m_pSquad->IsCrouchMode() )
				duck();
			else if ( m_pSquad->IsProneMode() )
				updateCondition(CONDITION_PRONE);
			else if ( m_pSquad->IsStealthMode() )
				updateCondition(CONDITION_COVERT);
		}
#ifdef _DEBUG
	}
#endif
	setMoveLookPriority(MOVELOOK_ATTACK);

	if ( !rcbot_debug_dont_shoot.GetBool() )
		handleWeapons();

	// deal with voice commands bot wants to say,
	// incase that he wants to use it in between frames (e.g. during an event call)
	// deal with it here
	if ( m_fNextVoiceCommand < engine->Time() && !m_nextVoicecmd.empty() )
	{
		const byte voiceCmd = static_cast<byte>(m_nextVoicecmd.front());

		m_fNextVoiceCommand = engine->Time() + randomFloat(0.4f,1.2f);
		
		if ( m_fLastVoiceCommand[voiceCmd] < engine->Time() )
		{
			voiceCommand(voiceCmd);
			m_fLastVoiceCommand[voiceCmd] = engine->Time() + randomFloat(8.0f,16.0f);
		}

		m_nextVoicecmd.pop();
	}

	m_iPrevHealth = m_pPlayerInfo->GetHealth();

	m_bInitAlive = false;

	if (!m_pEnemy || m_pEnemy != m_pOldEnemy)
		m_fEnemyAimLerp = 0.0f;
	else
	{
		Vector vEnemyAimLerpVelocity;
		CClassInterface::getVelocity(m_pEnemy.get(), &vEnemyAimLerpVelocity);

		const float fLerpTimeDelta = engine->Time() - m_fEnemyAimLerpTime;

		// Reset multiplier if enemy's velocity has changed drastically
		constexpr float fMaxDifference = 600.0f;
		const float fMaxDifferenceAdjusted = fMaxDifference * fLerpTimeDelta;

		m_fEnemyAimLerp = !vEnemyAimLerpVelocity.IsValid() || vEnemyAimLerpVelocity.Length() == 0.0f
			|| (std::fabs(vEnemyAimLerpVelocity.x - m_vEnemyAimLerpVelocity.x)
				+ std::fabs(vEnemyAimLerpVelocity.y - m_vEnemyAimLerpVelocity.y)
				+ std::fabs(vEnemyAimLerpVelocity.z - m_vEnemyAimLerpVelocity.z))
			* fLerpTimeDelta
		> fMaxDifferenceAdjusted
			? 0.0f
			: std::min(std::max(0.0f, m_fEnemyAimLerp + fLerpTimeDelta), 1.0f);

		m_vEnemyAimLerpVelocity = vEnemyAimLerpVelocity;
	}

	m_fEnemyAimLerpTime = engine->Time();
}

void CBot :: addVoiceCommand (const byte voiceCmd)
{
	if ( bot_use_vc_commands.GetBool() && m_fLastVoiceCommand[voiceCmd] < engine->Time() )
	{
		m_nextVoicecmd.push(voiceCmd);
		m_fNextVoiceCommand = engine->Time() + randomFloat(0.2f,1.0f);
	}
}

void CBot :: handleWeapons ()
{
	//
	// Handle attacking at this point
	//
	if ( m_pEnemy && !hasSomeConditions(CONDITION_ENEMY_DEAD) && 
		hasSomeConditions(CONDITION_SEE_CUR_ENEMY) && wantToShoot() && 
		isVisible(m_pEnemy) && isEnemy(m_pEnemy) )
	{
		CBotWeapon* pWeapon = getBestWeapon(m_pEnemy);

		if ( m_bWantToChangeWeapon && pWeapon != nullptr && pWeapon != getCurrentWeapon() && pWeapon->getWeaponIndex() )
		{
			selectWeapon(pWeapon->getWeaponIndex());
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

CBot :: CBot()
{
	CBot::init(true);
	m_iMovePriority = 0;
	m_iLookPriority = 0;
	m_iMoveSpeedPriority = 0;
	m_iMoveLookPriority = 0;
}
/*
* init()
*
* initialize all bot variables 
* (this is called when bot is made for the first time)
*/
void CBot :: init (const bool bVarInit)
{
	//m_bNeedToInit = false; // doing this now
	m_fLastHurtTime = 0.0f;
	m_iAmmo = nullptr;
	m_pButtons = nullptr;
	m_pNavigator = nullptr;
	m_pSchedules = nullptr;
	m_pVisibles = nullptr;
	m_pEdict = nullptr;
//	m_pBaseEdict = NULL;
	m_pFindEnemyFunc = nullptr;
	m_bUsed = false;
	m_pController = nullptr;
	m_pPlayerInfo = nullptr;

	m_pWeapons = nullptr;
	m_fTimeCreated = 0;	
	m_pProfile = nullptr;
	m_szBotName[0] = 0;
	m_fIdealMoveSpeed = 320;
	m_fFov = BOT_DEFAULT_FOV;
	m_bOpenFire = true;
	m_pSquad = nullptr;

	cmd.command_number = 0;

	if ( bVarInit )
		spawnInit();
}

edict_t *CBot :: getEdict () const
{
	return m_pEdict;
}

bool CBot :: isSquadLeader () const
{
	return m_pSquad->GetLeader() == m_pEdict;
}

void CBot :: updateConditions ()
{
	if ( m_pEnemy.get() != nullptr)
	{
		if ( !CBotGlobals::entityIsAlive(m_pEnemy) )
		{
			updateCondition(CONDITION_ENEMY_DEAD);
			m_pEnemy = nullptr;
		}
		else
		{
			removeCondition(CONDITION_ENEMY_DEAD);

			// clear enemy
			if ( m_pVisibles->isVisible(m_pEnemy) )
			{
				updateCondition(CONDITION_SEE_CUR_ENEMY);
				removeCondition(CONDITION_ENEMY_OBSCURED);
			}
			else 
			{
				if ( !m_pLastEnemy || m_pLastEnemy != m_pEnemy)
					enemyLost(m_pEnemy);

				setLastEnemy(m_pEnemy);

				removeCondition(CONDITION_SEE_CUR_ENEMY);
				removeCondition(CONDITION_SEE_ENEMY_HEAD);
				updateCondition(CONDITION_ENEMY_OBSCURED);
			}
		}
	}
	else
	{
		removeCondition(CONDITION_SEE_CUR_ENEMY);
		removeCondition(CONDITION_ENEMY_OBSCURED);
		removeCondition(CONDITION_ENEMY_DEAD);
		removeCondition(CONDITION_SEE_ENEMY_HEAD);
	}

	if ( inSquad() )
	{
		if ( isSquadLeader() )
		{
			if ( m_uSquadDetail.b1.said_move_out == false )
			{
				sayMoveOut();
				m_uSquadDetail.b1.said_move_out = true;
			}
		}
		else
		{
			edict_t *pLeader = m_pSquad->GetLeader();

			if ( CBotGlobals::entityIsValid(pLeader) && CBotGlobals::entityIsAlive(pLeader) )
			{
				removeCondition(CONDITION_SQUAD_LEADER_DEAD);

				if ( distanceFrom(pLeader) <= 400.0f )
					updateCondition(CONDITION_SQUAD_LEADER_INRANGE);
				else
					removeCondition(CONDITION_SQUAD_LEADER_INRANGE);

				if ( isVisible(pLeader) )
					updateCondition(CONDITION_SEE_SQUAD_LEADER);
				else
					removeCondition(CONDITION_SEE_SQUAD_LEADER);

				float fSpeed = 0.0f;

				if ( const CClient *pClient = CClients::get(pLeader) )
					fSpeed = pClient->getSpeed();

				// update squad idle condition. If squad is idle, bot can move around a small radius 
				// around the leader and do what they want, e.g. defend or snipe
				if (hasEnemy() || (fSpeed > 10.0f && CClassInterface::getMoveType(pLeader) != MOVETYPE_LADDER))
				{
					setSquadIdleTime(engine->Time());
					removeCondition(CONDITION_SQUAD_IDLE);
				}
				else if ( engine->Time() - m_fSquadIdleTime > rcbot_squad_idle_time.GetFloat() )
					updateCondition(CONDITION_SQUAD_IDLE);

			}
			else
				updateCondition(CONDITION_SQUAD_LEADER_DEAD);
		}
	}

	if ( m_pLastEnemy )
	{
		if ( m_fLastSeeEnemy > 0.0f )
		{
			if ( m_fLastUpdateLastSeeEnemy < engine->Time() )
			{
				m_fLastUpdateLastSeeEnemy = engine->Time() + 0.5f;

				if ( FVisible(m_vLastSeeEnemyBlastWaypoint) )
					updateCondition(CONDITION_SEE_LAST_ENEMY_POS);
				else
					removeCondition(CONDITION_SEE_LAST_ENEMY_POS);
			}
		}
		else
			removeCondition(CONDITION_SEE_LAST_ENEMY_POS);
	}

	if ( FVisible(m_vLookVector) )
	{
		updateCondition(CONDITION_SEE_LOOK_VECTOR);
	}
	else
	{
		removeCondition(CONDITION_SEE_LOOK_VECTOR);
	}
}

// Called when working out route
bool CBot::canGotoWaypoint(const Vector& vPrevWaypoint, CWaypoint* pWaypoint, CWaypoint* pPrev)
{
	if (pWaypoint->hasFlag(CWaypointTypes::W_FL_UNREACHABLE))
		return false;
	if (!pWaypoint->forTeam(getTeam()))
		return false;
	if (pWaypoint->hasFlag(CWaypointTypes::W_FL_OPENS_LATER))
	{
		if (pPrev != nullptr)
		{
			return pPrev->isPathOpened(pWaypoint->getOrigin());
		}
		if (vPrevWaypoint != pWaypoint->getOrigin() && !CBotGlobals::checkOpensLater(vPrevWaypoint, pWaypoint->getOrigin()))
			return false;
	}
	if (pWaypoint->hasFlag(CWaypointTypes::W_FL_FALL))
	{
		if (getHealthPercent() <= 0.1f)
		{
			if (vPrevWaypoint.z - pWaypoint->getOrigin().z > 200.0f)
				return false;
		}
	}

	return true;
}

void CBot::updatePosition() const
{
	m_pNavigator->rollBackPosition();
}

bool CBot::handleAttack ( CBotWeapon *pWeapon, edict_t *pEnemy )
{
	if ( pWeapon )
	{
		clearFailedWeaponSelect();

		if ( pWeapon->isMelee() )
			setMoveTo(CBotGlobals::entityOrigin(pEnemy));

		if ( pWeapon->mustHoldAttack() )
			primaryAttack(true);
		else
			primaryAttack();
	}
	else
		primaryAttack();

	return true;
}

int CBot :: getHealth () const
{
	return m_pPlayerInfo->GetHealth();
}

float CBot :: getHealthPercent () const
{
	return static_cast<float>(m_pPlayerInfo->GetHealth()) / static_cast<float>(m_pPlayerInfo->GetMaxHealth());
}

bool CBot ::isOnLift() const
{
	return m_vVelocity.z < -8.0f || m_vVelocity.z >= 8.0f;//&&(CClassInterface::getFlags(m_pEdict) & FL_ONGROUND);
}

edict_t *CBot :: getVisibleSpecial ()
{
	return nullptr;
}

bool CBot::wantToInvestigateSound () 
{ 
	return m_fSpawnTime + 10.0f < engine->Time() && !hasEnemy() && m_bWantToInvestigateSound; 
}

bool CBot :: recentlyHurt (const float fTime) const
{
	return m_fLastHurtTime>0 && m_fLastHurtTime>engine->Time()-fTime;
}

void CBot :: spawnInit ()
{
	m_fLastHurtTime = 0.0f;
	m_bWantToInvestigateSound = true;
	m_fSpawnTime = engine->Time();
	m_bIncreaseSensitivity = false;
	m_fLastSeeEnemyPlayer = 0.0f;
	m_PlayerListeningTo = nullptr;
	m_pPrimaryWeapon = nullptr;
	m_uSquadDetail.dat = 0;
	m_bStatsCanUse = false;
	m_StatsCanUse.data = 0;
	m_Stats.data = 0;
	m_iStatsIndex = 0;
	m_fStatsTime = 0.0f;

	m_fWantToListenTime = 0.0f;

	resetTouchDistance(48.0f);
	m_pLastCoverFrom = MyEHandle(nullptr);

	m_vAimOffset = Vector(1.0f,1.0f,1.0f);

	m_fTotalAimFactor = 0.0f;

	m_fAimMoment = 0.0f;

	std::memset(m_fLastVoiceCommand,0,sizeof(float)*MAX_VOICE_CMDS);

	m_fLastUpdateLastSeeEnemy = 0.0f;
	m_fPercentMoved = 1.0f;

	for (float& m_fUtilTime : m_fUtilTimes)
		m_fUtilTime = 0;

	if ( m_pSchedules != nullptr)
		m_pSchedules->freeMemory(); // clear tasks, im dead now!!
	if ( m_pVisibles != nullptr)
		m_pVisibles->reset();	

	if ( m_pEdict && m_iAmmo == nullptr)
		m_iAmmo = CClassInterface::getAmmoList(m_pEdict);

	m_fCurrentDanger = 0.0f;
	m_iSpecialVisibleId = 0;
	m_fUseRouteTime = 0.0f;
	m_bWantToListen = true;
	m_iPrevWeaponSelectFailed = 0;
	m_bOpenFire = true;
	m_fListenTime = 0.0f;
	m_bListenPositionValid = false;
	m_bPrevAimVectorValid = false;
	m_fLastSeeEnemy = 0.0f;
	m_fAvoidTime = 0.0f;
	m_vLookAroundOffset = Vector(0,0,0);
	m_fWaypointStuckTime = 0.0f;
	m_pPickup = nullptr;
	m_pAvoidEntity = nullptr;
	m_bThinkStuck = false;
	m_pLookEdict = nullptr;
	m_fLookAroundTime = 0.0f;
	m_bLookedForEnemyLast = false;
	////////////////////////
	m_iPrevHealth = 0;    // 
	////////////////////////
	m_vStuckPos = Vector(0,0,0);
	//m_iTimesStuck = 0;
	m_fUpdateDamageTime = 0.0f;
	m_iAccumulatedDamage = 0;
	m_fCheckStuckTime = engine->Time() + 8.0f;
	m_fStuckTime = 0.0f;
	m_vLastOrigin = Vector(0,0,0);
	m_vVelocity = Vector(0,0,0);
	m_fUpdateOriginTime = 0.0f;
	m_fNextUpdateAimVector = 0.0f;
	m_vAimVector = Vector(0,0,0);

	m_fLookSetTime = 0.0f;
	m_vHurtOrigin = Vector(0,0,0);

	m_pOldEnemy = nullptr;
	m_pEnemy = nullptr;	

	m_vLastSeeEnemy = Vector(0,0,0);
	m_pLastEnemy = nullptr; // enemy we were fighting before we lost it
	//m_pAvoidEntity = NULL; // avoid this guy
	m_fLastWaypointVisible = 0.0f;
	m_vGoal = Vector(0,0,0);
	m_bHasGoal = false;
	m_fLookAtTimeStart = 0.0f;
	m_fLookAtTimeEnd = 0.0f;
	m_fNextThink = 0.0f;
	m_iImpulse = 0;
	m_iButtons = 0;
	m_fForwardSpeed = 0.0f;
	m_fSideSpeed = 0.0f;
	m_fUpSpeed = 0.0f;
	m_iConditions = 0;
	m_fStrafeTime = 0.0f;

	m_bInitAlive = true;

	m_vMoveTo = Vector(0,0,0);
	m_bMoveToIsValid = false;
	m_vLookAt = Vector(0,0,0);
	m_bLookAtIsValid = false;
	m_iSelectWeapon = 0;

	m_fAvoidSideSwitch = 0.0f;

	m_bAvoidRight = randomInt(0,1)==0;

	m_iLookTask = LOOK_WAYPOINT;

	//
	m_vViewAngles = QAngle(0,0,0);

	if ( m_pVisibles != nullptr)
		m_pVisibles->reset();

	m_fEnemyAimLerp = 0.0f;
	m_fEnemyAimLerpTime = 0.0f;
	m_vEnemyAimLerpVelocity = Vector(0.0f, 0.0f, 0.0f);
}

void CBot::setLastEnemy(edict_t *pEnemy)
{
	if ( pEnemy == nullptr)
	{
		m_fLastSeeEnemy = 0.0f;
		m_pLastEnemy = nullptr;
		return;
	}

	m_fLastSeeEnemy = engine->Time();
	m_pLastEnemy = pEnemy;
	m_fLastUpdateLastSeeEnemy = 0.0f;
	m_vLastSeeEnemy = CBotGlobals::entityOrigin(m_pLastEnemy);
	m_vLastSeeEnemyBlastWaypoint = m_vLastSeeEnemy;

	CWaypoint* pWpt = CWaypoints::getWaypoint(CWaypointLocations::NearestBlastWaypoint(m_vLastSeeEnemy, getOrigin(),
		8192.0f, -1, true, true, false, false, 0, false));
	
	if ( pWpt )
		m_vLastSeeEnemyBlastWaypoint = pWpt->getOrigin();

}

bool CBot :: selectBotWeapon ( CBotWeapon *pBotWeapon )
{
	if ( const int id = pBotWeapon->getWeaponIndex() )
	{
		selectWeapon(id);
		return true;
	}
	return false;
}

float CBot :: getEnemyFactor ( edict_t *pEnemy )
{
	return distanceFrom(pEnemy);
}

void CBot :: touchedWpt ( CWaypoint *pWaypoint, int iNextWaypoint, int iPrevWaypoint )
{
	resetTouchDistance(48.0f);

	m_fWaypointStuckTime = engine->Time() + randomFloat(7.5f,12.5f);

	if ( pWaypoint->getFlags() & CWaypointTypes::W_FL_JUMP )
		jump();
	if ( pWaypoint->getFlags() & CWaypointTypes::W_FL_CROUCH )
		duck();
	if ( pWaypoint->getFlags() & CWaypointTypes::W_FL_SPRINT )
		updateCondition(CONDITION_RUN);

	updateDanger(m_pNavigator->getBelief(CWaypoints::getWaypointIndex(pWaypoint)));
}

void CBot :: updateDanger (const float fBelief) 
{ 
	m_fCurrentDanger = m_fCurrentDanger * m_pProfile->m_fBraveness + fBelief * (1.0f - m_pProfile->m_fBraveness); 
}
// setup buttons and data structures
void CBot :: setup ()
{
	/////////////////////////////////
	m_pButtons = new CBotButtons();
	/////////////////////////////////
	m_pSchedules = new CBotSchedules();
	/////////////////////////////////
	m_pNavigator = new CWaypointNavigator(this);   
	/////////////////////////////////
	m_pVisibles = new CBotVisibles(this);
	/////////////////////////////////
	m_pFindEnemyFunc = new CFindEnemyFunc(this);
	/////////////////////////////////
	m_pWeapons = new CBotWeapons(this);

	//stucknet = new CBotNeuralNet(3,2,2,1,0.5f);
	//stucknet_tset = new CTrainingSet(3,1,10);

//	m_pGAvStuck = new CBotStuckValues();
//	m_pGAStuck = new CGA(10);
	//m_pThinkStuck = new CPerceptron(2);
}

/*
* called when a bot dies
*/
void CBot :: died ( edict_t *pKiller, const char *pszWeapon )
{	
	spawnInit();

	if ( m_pSquad != nullptr)
	{
		// died
		CBotSquads::removeSquadMember(m_pSquad,m_pEdict);
	}

	m_vLastDiedOrigin = getOrigin();
}

/*
* called when a bot kills something
*/
void CBot :: killed ( edict_t *pVictim, char *weapon )
{	
	if ( pVictim == m_pLastEnemy )
		m_pLastEnemy = nullptr;
}

// called when bot shoots a wall or similar object -i.e. not the enemy
void CBot :: shotmiss ()
{
}

// shot an enemy (or teammate?) //TODO: Experimental [APG]RoboCop[CL]
void CBot :: shot ( edict_t *pEnemy )
{
	if ( pEnemy == nullptr )
		return;

	if ( CClassInterface::getTeam(pEnemy) == getTeam() )
		friendlyFire(pEnemy);

}

// got shot by someone
bool CBot :: hurt ( edict_t *pAttacker, const int iHealthNow, const bool bDontHide )
{
	if ( !pAttacker )
		return false;

	if ( CClassInterface::getTeam(pAttacker) == getTeam() )
		friendlyFire(pAttacker);

	m_vHurtOrigin = CBotGlobals::entityOrigin(pAttacker);

	if ( !hasSomeConditions(CONDITION_SEE_CUR_ENEMY) )
	{
		m_fLookSetTime = 0.0f;
		setLookAtTask(LOOK_HURT_ORIGIN);
		m_fLookSetTime = engine->Time() + randomFloat(3.0f,8.0f);
	}

	const float fTime = engine->Time();

	if ( m_fUpdateDamageTime < fTime )
	{
		m_fUpdateDamageTime = fTime + 0.5f;
		m_fCurrentDanger += static_cast<float>(m_iAccumulatedDamage)/m_pPlayerInfo->GetMaxHealth()*MAX_BELIEF;
		m_iAccumulatedDamage = 0;
	}

	m_fLastHurtTime = engine->Time();
	m_iAccumulatedDamage += m_iPrevHealth-iHealthNow;
	m_iPrevHealth = iHealthNow;	

	// TODO: replace with perceptron method
	if ( m_iAccumulatedDamage > m_pPlayerInfo->GetMaxHealth() * m_pProfile->m_fBraveness )
	{
		if ( !bDontHide )
		{
			m_pSchedules->removeSchedule(SCHED_GOOD_HIDE_SPOT);
			m_pSchedules->addFront(new CGotoHideSpotSched(this,m_vHurtOrigin));
		}

		m_iAccumulatedDamage = 0;
		m_fUpdateDamageTime = 0.0f;

		return true;
	}

	return false;
}

void CBot :: checkEntity ( edict_t **pEdict )
{
	if ( pEdict && *pEdict && !CBotGlobals::entityIsValid(*pEdict) )
		*pEdict = nullptr;
}

void CBot :: checkDependantEntities ()
{
	//checkEntity(&m_pOldEnemy);
	//checkEntity(&m_pLookEdict);
	//checkEntity(&m_pAvoidEntity);
	//checkEntity(&m_pEnemy);
	//checkEntity(&m_pLastEnemy);
	//checkEntity(&m_pPickup);
}

void CBot :: findEnemy ( edict_t *pOldEnemy )
{
	m_pFindEnemyFunc->init();

	if ( pOldEnemy && isEnemy(pOldEnemy,true) ) 
		m_pFindEnemyFunc->setOldEnemy(pOldEnemy);
	/*else if ( CBotGlobals::entityIsAlive(pOldEnemy) ) /// lost enemy
	{
		CWaypoint *pWpt;

		m_fLastSeeEnemy = engine->Time();
		m_pLastEnemy = pOldEnemy;
		m_fLastUpdateLastSeeEnemy = engine->Time() + 0.1f;
		m_vLastSeeEnemy = CBotGlobals::entityOrigin(m_pLastEnemy);
		m_vLastSeeEnemyBlastWaypoint = m_vLastSeeEnemy;

		pWpt = CWaypoints::getWaypoint(CWaypointLocations::NearestBlastWaypoint(m_vLastSeeEnemy,getOrigin(),8192.0,-1,true,true,false,false,0,false));
		
		if ( pWpt )
			m_vLastSeeEnemyBlastWaypoint = pWpt->getOrigin();
	}*/

	m_pVisibles->eachVisible(m_pFindEnemyFunc);

	m_pEnemy = m_pFindEnemyFunc->getBestEnemy();

	if ( m_pEnemy && m_pEnemy != pOldEnemy )
	{
		enemyFound(m_pEnemy);
	}
	else if ( pOldEnemy && !m_pEnemy )
	{
		// Notify SourceMod that bot lost its enemy
		RCBotEvents::OnBotEnemyLost(m_pEdict, pOldEnemy);
	}
}

bool CBot :: isAlive ()
{
	return !m_pPlayerInfo->IsDead();
}

int CBot :: getTeam ()
{
	return m_pPlayerInfo->GetTeamIndex();
}

const char *pszConditionsDebugStrings[NUM_CONDITIONS] =
{"CONDITION_ENEMY_OBSCURED",
"CONDITION_NO_WEAPON",
"CONDITION_OUT_OF_AMMO",
"CONDITION_SEE_CUR_ENEMY",
"CONDITION_ENEMY_DEAD",
"CONDITION_SEE_WAYPOINT",
"CONDITION_NEED_AMMO",
"CONDITION_NEED_HEALTH",
"CONDITION_SEE_LOOK_VECTOR",
"CONDITION_POINT_CAPTURED",
"CONDITION_PUSH",
"CONDITION_LIFT",
"CONDITION_SEE_HEAL",
"CONDITION_SEE_LAST_ENEMY_POS",
"CONDITION_CHANGED",
"CONDITION_COVERT",
"CONDITION_RUN",
"CONDITION_GREN",
"CONDITION_NEED_BOMB",
"CONDITION_SEE_ENEMY_HEAD",
"CONDITION_PRONE",
"CONDITION_PARANOID",
"CONDITION_SEE_SQUAD_LEADER",
"CONDITION_SQUAD_LEADER_DEAD",
"CONDITION_SQUAD_LEADER_INRANGE",
"CONDITION_SQUAD_IDLE",
"CONDITION_DEFENSIVE",
"CONDITION_BUILDING_SAPPED",
"CONDITION_SEE_ENEMY_GROUND"};

void CBot :: clearSquad ()
{
	//causes stack overflow
	//if ( m_pSquad != NULL )
	//	CBotSquads::removeSquadMember(m_pSquad,m_pEdict);

	m_pSquad = nullptr;
}

bool CBot :: isFacing (const Vector& vOrigin) const
{
	return DotProductFromOrigin(vOrigin) > 0.97f;
}

void CBot ::debugBot(char *msg)
{
	const bool hastask = m_pSchedules->getCurrentTask()!= nullptr;

	char szConditions[512];
	//int iBit = 0;

	szConditions[0] = 0; // initialise string

	for (std::size_t iCond = 0; iCond < NUM_CONDITIONS; iCond++)
	{
		if ( m_iConditions[iCond] )
		{
			std::strcat(szConditions, pszConditionsDebugStrings[iCond]);
			std::strcat(szConditions, "\n");
		}
	}

	char task_string[256];

	extern const char *g_szUtils[BOT_UTIL_MAX+1]; //Redundant? [APG]RoboCopCL]

	edict_t *pEnemy = m_pEnemy.get();

	IPlayerInfo *p = nullptr;

	const int iEnemyID = ENTINDEX(pEnemy);

	if ( iEnemyID > 0 && iEnemyID <= gpGlobals->maxClients )
		p = playerinfomanager->GetPlayerInfo(pEnemy);
	

	if ( hastask )
		m_pSchedules->getCurrentTask()->debugString(task_string, {});

	const bool hasNextPoint = m_pNavigator->hasNextPoint();
	const int currentWaypointID = hasNextPoint ? m_pNavigator->getCurrentWaypointID() : -1;
	const int currentGoalID = hasNextPoint ? m_pNavigator->getCurrentGoalID() : -1;

	std::sprintf(msg, "Debugging bot: %s\n \
		Current Util: %s \n \
		Current Schedule: %s\n \
		Current Task: {%s}\n \
		Look Task:%s\n \
		Current Waypoint:%d\n \
		Current Goal: %d\n \
		Danger: %0.2f pc\n \
		Enemy: %s (name = '%s')\n \
		---CONDITIONS---\n%s",
		m_szBotName,
		m_CurrentUtil < BOT_UTIL_MAX + 1 ? g_szUtils[m_CurrentUtil] : "none",
		m_pSchedules->isEmpty() ? "none" : m_pSchedules->getCurrentSchedule()->getIDString(),
		hastask ? task_string : "none",
		g_szLookTaskToString[m_iLookTask],
		currentWaypointID,
		currentGoalID,
		m_fCurrentDanger / MAX_BELIEF * 100,
		pEnemy != nullptr ? pEnemy->GetClassName() : "none",
		p != nullptr ? p->GetName() : "none",
		szConditions
	);
}

int CBot :: nearbyFriendlies (const float fDistance)
{
	int num = 0;
	const int maxclients = CBotGlobals::maxClients();

	for ( int i = 0; i <= maxclients; i ++ )
	{
		edict_t* pEdict = INDEXENT(i);

		if ( !CBotGlobals::entityIsValid(pEdict) )
			continue;

		if ( distanceFrom(pEdict) > fDistance )
			continue;

		if ( !isVisible(pEdict) )
			continue;

		if ( isEnemy(pEdict) )
			continue;

		num ++;
	}

	return num;
}

void CBot :: freeMapMemory ()
{
	// we can save things here
	// 
	/////////////////////////////////
	if ( m_pButtons != nullptr)
	{
		m_pButtons->freeMemory();
		delete m_pButtons;
		m_pButtons = nullptr;
	}
	/////////////////////////////////
	if ( m_pSchedules != nullptr)
	{
		m_pSchedules->freeMemory();
		delete m_pSchedules;
		m_pSchedules = nullptr;
	}
	/////////////////////////////////
	if ( m_pNavigator != nullptr)
	{
		m_pNavigator->beliefSave(true);
		m_pNavigator->freeMapMemory();
		delete m_pNavigator;
		m_pNavigator = nullptr;
	}
	/////////////////////////////////
	if ( m_pVisibles != nullptr)
	{
		m_pVisibles->reset();
		delete m_pVisibles;
		m_pVisibles = nullptr;
	}
	/////////////////////////////////
	if ( m_pFindEnemyFunc != nullptr)
	{
		delete m_pFindEnemyFunc;
		m_pFindEnemyFunc = nullptr;
	}
	/////////////////////////////////
	if ( m_pWeapons != nullptr)
	{
		delete m_pWeapons;
		m_pWeapons = nullptr;
	}

	m_iAmmo = nullptr;
	/////////////////////////////////
	init();
}

void CBot :: updateStatistics ()
{
	bool bVisible;
	bool bIsEnemy;

	if ( m_iStatsIndex == 0 || m_iStatsIndex > gpGlobals->maxClients )
	{
		if ( m_iStatsIndex != 0 )
			m_bStatsCanUse = true;

		m_StatsCanUse.data = m_Stats.data;
		m_Stats.data = 0;
		m_iStatsIndex = 0; // reset to be sure in case of m_iStatsIndex > gpGlobals->maxClients

		if (!m_uSquadDetail.b1.said_area_clear && m_StatsCanUse.stats.m_iEnemiesInRange == 0 &&
			m_StatsCanUse.stats.m_iEnemiesVisible == 0 && m_StatsCanUse.stats.m_iTeamMatesInRange > 0)
		{
			if (!inSquad() || (isSquadLeader() &&
				(m_fLastSeeEnemy > 0.0f && m_fLastSeeEnemy + 10.0f < engine->Time())))
			{
				areaClear();
			}

			m_uSquadDetail.b1.said_area_clear = true;
		}
	}

	const CClient *pClient = CClients::get(m_iStatsIndex++);

	if ( !pClient->isUsed() )
		return;

	edict_t *pPlayer = pClient->getPlayer();

	if ( pPlayer == m_pEdict )
		return; // don't listen to self

	IPlayerInfo *p = playerinfomanager->GetPlayerInfo(pPlayer);

	// 05/07/09 fix crash bug
	if ( !p || !p->IsConnected() || p->IsDead() || p->IsObserver() || !p->IsPlayer() )
		return;

	try
	{
		bVisible = isVisible(pPlayer);
		bIsEnemy = isEnemy(pPlayer, false);
	}

	catch(...)
	{
		return;
	}

	if ( bIsEnemy )
	{
		if ( bVisible )
			m_Stats.stats.m_iEnemiesVisible++;

		if ( distanceFrom(pPlayer) < rcbot_stats_inrange_dist.GetFloat() )
			m_Stats.stats.m_iEnemiesInRange++;
	}
	else
	{
		// team-mate
		if ( bVisible )
			m_Stats.stats.m_iTeamMatesVisible++;

		if ( distanceFrom(pPlayer) < rcbot_stats_inrange_dist.GetFloat() )
			m_Stats.stats.m_iTeamMatesInRange++;
	}
}

bool CBot :: wantToListen () const
{
	return m_bWantToListen && m_fWantToListenTime < engine->Time() && m_fLastSeeEnemy+2.5f < engine->Time();
}
// Listen for players who are shooting
void CBot :: listenForPlayers ()
{
	//m_fNextListenTime = engine->Time() + randomFloat(0.5f,2.0f);

	edict_t *pListenNearest = nullptr;
	float fMaxFactor = 0.0f;
	Vector vVelocity;
	bool bIsNearestAttacking = false;

	if ( m_bListenPositionValid && m_fListenTime > engine->Time() ) // already listening to something ?
	{
		setLookAtTask(LOOK_NOISE);
		return;
	}

	m_bListenPositionValid = false;

	for (int i = 1; i <= gpGlobals->maxClients; i ++ )
	{
		edict_t* pPlayer = INDEXENT(i);

		if ( pPlayer == m_pEdict )
			continue; // don't listen to self

		if ( pPlayer->IsFree() )
			continue;

		const CClient* pClient = CClients::get(pPlayer);

		if ( !pClient->isUsed() )
			continue;

		IPlayerInfo* p = playerinfomanager->GetPlayerInfo(pPlayer);

		// 05/07/09 fix crash bug
		if ( !p || !p->IsConnected() || p->IsDead() || p->IsObserver() || !p->IsPlayer() )
			continue;

		const float fDist = distanceFrom(pPlayer);

		if ( fDist > rcbot_listen_dist.GetFloat() )
			continue;
		
		float fFactor = 0.0f;

		const CBotCmd command = p->GetLastUserCommand();

		if ( command.buttons & IN_ATTACK )
		{
			if ( wantToListenToPlayerAttack(pPlayer) )
				fFactor += 1000.0f;
		}
		
		// can't see this player and I'm on my own
		if ( wantToListenToPlayerFootsteps(pPlayer) && !isVisible(pPlayer) && ( m_bStatsCanUse && m_StatsCanUse.stats.m_iTeamMatesVisible==0/* && (m_fSeeTeamMateTime ...) */) )
		{
			CClassInterface::getVelocity(pPlayer,&vVelocity);

			const float fVelocity = vVelocity.Length();

			if ( fVelocity > rcbot_footstep_speed.GetFloat() )
				fFactor += vVelocity.Length();
		}

		if ( fFactor == 0.0f )
			continue;

		// add inverted distance to the factor (i.e. closer = better)
		fFactor += rcbot_listen_dist.GetFloat() - fDist;

		if ( fFactor > fMaxFactor )
		{
			fMaxFactor = fFactor;
			pListenNearest = pPlayer;
			bIsNearestAttacking = command.buttons & IN_ATTACK;
		}
	}

	if ( pListenNearest != nullptr)
	{
		listenToPlayer(pListenNearest,false,bIsNearestAttacking);
	}
}

void CBot :: hearPlayerAttack( edict_t *pAttacker, int iWeaponID )
{
	if ( m_fListenTime < engine->Time() ) // already listening to something ?
		listenToPlayer(pAttacker,false,true);
}

void CBot :: listenToPlayer ( edict_t *pPlayer, bool bIsEnemy, const bool bIsAttacking )
{
	const bool bIsVisible = isVisible(pPlayer);

	if ( CBotGlobals::isPlayer( pPlayer ) )
	{
		IPlayerInfo *p = playerinfomanager->GetPlayerInfo(pPlayer);

		if ( bIsEnemy == false )
			bIsEnemy = isEnemy(pPlayer);
			 
		// look at possible enemy
		if ( !bIsVisible || bIsEnemy )
		{
			m_vListenPosition = p->GetAbsOrigin();
		}
		else if ( bIsAttacking )
		{
			if ( !bIsEnemy && wantToInvestigateSound() ) //TODO: !bIsEnemy always true? [APG]RoboCop[CL]
			{
				const QAngle angle = p->GetAbsAngles();
				Vector forward;

				AngleVectors( angle, &forward );

				// look where team mate is shooting
				m_vListenPosition = p->GetAbsOrigin() + forward*1024.0f;		

				// not investigating any noise right now -- depending on my braveness I will check it out
				if ( !m_pSchedules->isCurrentSchedule(SCHED_INVESTIGATE_NOISE) && randomFloat(0.0f,0.75f) < m_pProfile->m_fBraveness )
				{
					const trace_t *TraceResult = CBotGlobals::getTraceResult();

					const Vector vAttackerOrigin = CBotGlobals::entityOrigin(pPlayer);

					if ( distanceFrom(vAttackerOrigin) > 96.0f )
					{
						// check exactly where teammate is firing
						CBotGlobals::quickTraceline(pPlayer,vAttackerOrigin,m_vListenPosition);

						// update the wall or player teammate is shooting
						m_vListenPosition = TraceResult->endpos; 

						CBotGlobals::quickTraceline(m_pEdict,getOrigin(),m_vListenPosition);

						// can't see what my teammate is shooting -- go there
						if ( TraceResult->fraction < 1.0f && TraceResult->m_pEnt != m_pEdict->GetIServerEntity()->GetBaseEntity() )
						{
							m_pSchedules->removeSchedule(SCHED_INVESTIGATE_NOISE);

							Vector vecLOS = getOrigin() - vAttackerOrigin;
							vecLOS = vecLOS/vecLOS.Length();

							const float flDot = DotProduct(vecLOS, forward);

							if ( flDot > 0.5f )
							{
								// Facing my direction
								m_pSchedules->addFront(new CBotInvestigateNoiseSched(m_vListenPosition,m_vListenPosition));
							}
							else
							{
								// Not facing my direction
								m_pSchedules->addFront(new CBotInvestigateNoiseSched(vAttackerOrigin,m_vListenPosition));
							}						
						}
					}
				}
			}
		}
	}
	else
		m_vListenPosition = CBotGlobals::entityOrigin(pPlayer);

	m_PlayerListeningTo = pPlayer;
	m_bListenPositionValid = true;
	m_fListenTime = engine->Time() + randomFloat(1.0f,2.0f);
	setLookAtTask(LOOK_NOISE);
	m_fLookSetTime = m_fListenTime;

	if ( bIsVisible || !bIsEnemy ) 
	{// certain where noise is coming from -- don't listen elsewhere for another second
		m_fWantToListenTime = engine->Time() + 1.0f;
	}
	else
		m_fWantToListenTime = engine->Time() + 0.25f;

}

bool CBot :: onLadder () const // onLadder needs to be constant? [APG]RoboCop[CL]
{	
	return CClassInterface::isMoveType(m_pEdict,MOVETYPE_LADDER);
}

void CBot :: freeAllMemory ()
{	
	freeMapMemory();
}

void CBot :: forceGotoWaypoint (const int wpt) const
{
	if ( wpt != -1 )
	{
		m_pSchedules->freeMemory();
		m_pSchedules->add(new CBotSchedule(new CFindPathTask(wpt)));
	}
}
// found a new enemy
void CBot :: enemyFound (edict_t *pEnemy)
{
	// Notify SourceMod of new enemy found
	RCBotEvents::OnBotEnemyFound(m_pEdict, pEnemy);

	m_bLookedForEnemyLast = false;
}
// work move velocity
void CBot :: doMove ()
{
	// Temporary measure to make bot follow me when i make listen serevr
	//setMoveTo(CBotGlobals::entityOrigin(INDEXENT(1)));

	// moving somewhere?
	if ( moveToIsValid () )
	{
		Vector2D move;

		if ( m_pAvoidEntity && m_fAvoidTime < engine->Time() )
		{
			if ( canAvoid(m_pAvoidEntity) )
			{
				//m_vMoveTo = getOrigin() + ((m_vMoveTo-getOrigin())-((m_vAvoidOrigin-getOrigin())*bot_avoid_strength.GetFloat()));
				//float fAvoidDist = distanceFrom(m_pAvoidEntity);

				const Vector vMove = m_vMoveTo-getOrigin();

				if ( vMove.Length2D() > bot_avoid_strength.GetFloat() )
				{
					Vector vLeft = vMove.Cross(Vector(0, 0, 1));
					vLeft = vLeft/vLeft.Length();

					if ( m_fAvoidSideSwitch < engine->Time() )
					{
						m_fAvoidSideSwitch = engine->Time() + randomFloat(2.0f,3.0f);
						m_bAvoidRight = !m_bAvoidRight;
					}
#ifndef __linux__
					if ( CClients::clientsDebugging(BOT_DEBUG_THINK) )
					{
						const Vector m_vAvoidOrigin = CBotGlobals::entityOrigin(m_pAvoidEntity);

						debugoverlay->AddLineOverlay (getOrigin(), m_vAvoidOrigin, 0,0,255, false, 0.05f);
						debugoverlay->AddLineOverlay (getOrigin(), m_bAvoidRight ? getOrigin()+vLeft*bot_avoid_strength.GetFloat():getOrigin()-vLeft*bot_avoid_strength.GetFloat(), 0,255,0, false, 0.05f);
						debugoverlay->AddLineOverlay (getOrigin(), getOrigin() + vMove/vMove.Length()*bot_avoid_strength.GetFloat(), 255,0,0, false, 0.05f);
						debugoverlay->AddTextOverlayRGB(getOrigin()+Vector(0,0,100),0,0.05f,255,255,255,255,"Avoiding: %s",m_pAvoidEntity.get()->GetClassName());
					}
#endif

					//debugoverlay->AddLineOverlay (getOrigin(), m_vAvoidOrigin, 0,0,255, false, 0.05f);
					//debugoverlay->AddLineOverlay (getOrigin(), m_bAvoidRight ? (getOrigin()+(vLeft*bot_avoid_strength.GetFloat())):(getOrigin()-(vLeft*bot_avoid_strength.GetFloat())), 0,255,0, false, 0.05f);
					//debugoverlay->AddLineOverlay (getOrigin(), m_vMoveTo, 255,0,0, false, 0.05f);
					

					if ( m_bAvoidRight )
						m_vMoveTo = getOrigin() + vMove/vMove.Length()*bot_avoid_strength.GetFloat() + vLeft*bot_avoid_strength.GetFloat();
					else
						m_vMoveTo = getOrigin() + vMove/vMove.Length()*bot_avoid_strength.GetFloat() - vLeft*bot_avoid_strength.GetFloat();
				}
			}
			else
				m_pAvoidEntity = nullptr;
		}

		const float fAngle = CBotGlobals::yawAngleFromEdict(m_pEdict, m_vMoveTo);
		const float fDist = (getOrigin() - m_vMoveTo).Length2D();

		const float radians = DEG_TO_RAD(fAngle);
		//radians = fAngle * 3.141592f / 180.0f; // degrees to radians
		// fl Move is percentage (0 to 1) of forward speed,
		// flSide is percentage (0 to 1) of side speed.
		
		// quicker
		SinCos(radians,&move.y,&move.x);

		move = move / move.Length();

		const float flMove = move.x;
		const float flSide = move.y;

		m_fForwardSpeed = m_fIdealMoveSpeed * flMove;

		// dont want this to override strafe speed if we're trying 
		// to strafe to avoid a wall for instance.
		if ( m_fStrafeTime < engine->Time() )
		{
			// side speed 
			m_fSideSpeed = m_fIdealMoveSpeed * flSide;
		}

		if ( hasSomeConditions(CONDITION_LIFT) )//fabs(m_vMoveTo.z - getOrigin().z) > 48 )
		{
			if ( std::fabs(m_vVelocity.z) > 16.0f )
			{
				m_fForwardSpeed = 0.0f;
				m_fSideSpeed = 0.0f;
			}
		}

		// moving less than 1.0 units/sec? just stop to 
		// save bot jerking around..
		if ( std::fabs(m_fForwardSpeed) < 1.0f )
			m_fForwardSpeed = 0.0f;
		if ( std::fabs(m_fSideSpeed) < 1.0f )
			m_fSideSpeed = 0.0f;

		if ( m_fForwardSpeed < 1.0f && m_fSideSpeed < 1.0f )
		{
			if ( m_pButtons->holdingButton(IN_SPEED) && m_pButtons->holdingButton(IN_FORWARD) )
			{
				m_pButtons->letGo(IN_SPEED);
				m_pButtons->letGo(IN_FORWARD);
			}
		}

		if ( !onLadder() && !m_pNavigator->nextPointIsOnLadder() && fDist < 8.0f )
		{
			m_fForwardSpeed = 0.0f;
			m_fSideSpeed = 0.0f;
		}

		if ( isUnderWater() || onLadder() )
		{
			if ( m_vMoveTo.z > getOrigin().z + 32.0f )
				m_fUpSpeed = m_fIdealMoveSpeed;
			else if ( m_vMoveTo.z < getOrigin().z - 32.0f )
				m_fUpSpeed = -m_fIdealMoveSpeed;
		}
	}
	else
	{	
		m_fForwardSpeed = 0.0f;
		// bots side move speed
		m_fSideSpeed = 0.0f;
		// bots upward move speed (e.g in water)
		m_fUpSpeed = 0.0f;
	}
}

bool CBot :: recentlySpawned (const float fTime) const
{
	return m_fSpawnTime + fTime > engine->Time();
}

bool CBot :: FInViewCone ( edict_t *pEntity ) const
{	
	static Vector origin;
	
	if ( CBotGlobals::isBrushEntity(pEntity) )
		origin = CBotGlobals::worldCenter(pEntity);
	else
	origin = CBotGlobals::entityOrigin(pEntity);

	return (origin - getEyePosition()).Length()>1 && DotProductFromOrigin(origin) > 0; // 90 degree !! 0.422618f ); // 65 degree field of view   
}

float CBot :: DotProductFromOrigin (const Vector& pOrigin) const
{
	static Vector vecLOS;
	static float flDot;
	
	Vector vForward;

	const QAngle eyes = eyeAngles();

	// in fov? Check angle to edict
	AngleVectors(eyes,&vForward);
	
	vecLOS = pOrigin - getEyePosition();
	vecLOS = vecLOS/vecLOS.Length();
	
	flDot = DotProduct (vecLOS , vForward );
	
	return flDot; 
}

void CBot :: updateUtilTime (const int util)
{
	m_fUtilTimes[util] = engine->Time() + 0.5f;	
}

Vector CBot::getAimVector ( edict_t *pEntity )
{
	static Vector v_desired_offset;
	static Vector v_origin;
	static float fSensitivity;
	static Vector v_size;
	static float fDist;
	static float fDist2D;

	if ( m_fNextUpdateAimVector > engine->Time() )
	{
		return m_vAimVector;
	}

	if ( CBotGlobals::isBrushEntity(pEntity) )
	{
		v_origin = CBotGlobals::worldCenter(pEntity);
		fDist = distanceFrom(v_origin);
		fDist2D = distanceFrom2D(v_origin);
	}
	else
	{
	fDist = distanceFrom(pEntity);
	fDist2D = distanceFrom2D(pEntity);
		v_origin = CBotGlobals::entityOrigin(pEntity);
	}

	v_size = pEntity->GetCollideable()->OBBMaxs() - pEntity->GetCollideable()->OBBMins();
	v_size = v_size * 0.5f;

	fSensitivity = static_cast<float>(m_pProfile->m_iSensitivity)/20;

	modAim(pEntity,v_origin,&v_desired_offset,v_size,fDist,fDist2D);

	// post aim
	// update 
	if ( rcbot_supermode.GetBool() )
	{
		m_vAimOffset = v_desired_offset;
	}
	else
	{
		m_vAimOffset.x = (1.0f-fSensitivity)*m_vAimOffset.x + fSensitivity*v_desired_offset.x; 
		m_vAimOffset.y = (1.0f-fSensitivity)*m_vAimOffset.y + fSensitivity*v_desired_offset.y;
		m_vAimOffset.z = (1.0f-fSensitivity)*m_vAimOffset.z + fSensitivity*v_desired_offset.z;

		// check for QNAN
		if ( m_vAimOffset.x != m_vAimOffset.x || 
			m_vAimOffset.y != m_vAimOffset.y || 
			m_vAimOffset.z != m_vAimOffset.z )
		{
			m_vAimOffset = Vector(1.0f,1.0f,1.0f);
		}
	}

	if ( pEntity == CClassInterface::getGroundEntity(m_pEdict) )
		m_vAimOffset.z -= 32.0f;

	m_vAimVector = v_origin + m_vAimOffset;

	m_fNextUpdateAimVector = engine->Time() + (1.0f-m_pProfile->m_fAimSkill)*0.2f;

#ifndef __linux__
	if ( CClients::clientsDebugging(BOT_DEBUG_AIM) && CClients::isListenServerClient(CClients::get(0)) )
	{
		if ( CClients::get(0)->getDebugBot() == getEdict() )
		{
			int line = 0;
			const float ftime = m_fNextUpdateAimVector-engine->Time();

			debugoverlay->AddTextOverlayRGB(m_vAimVector,line++,ftime,255,200,100,230,"x Aiming Info");
			debugoverlay->AddTextOverlayRGB(m_vAimVector,line++,ftime,255,200,100,230,"fDist = %0.2f",fDist);
			debugoverlay->AddTextOverlayRGB(m_vAimVector,line++,ftime,255,200,100,230,"fSensitivity = %0.2f",fSensitivity);
			debugoverlay->AddTextOverlayRGB(m_vAimVector,line++,ftime,255,200,100,230,"v_size = (%0.2f,%0.2f,%0.2f)",v_size.x,v_size.y,v_size.z);
			debugoverlay->AddTextOverlayRGB(m_vAimVector,line++,ftime,255,200,100,230,"v_desired_offset = (%0.2f,%0.2f,%0.2f)",v_desired_offset.x,v_desired_offset.y,v_desired_offset.z);
			debugoverlay->AddTextOverlayRGB(m_vAimVector,line++,ftime,255,200,100,230,"m_vAimOffset = (%0.2f,%0.2f,%0.2f)",m_vAimOffset.x,m_vAimOffset.y,m_vAimOffset.z);
			debugoverlay->AddLineOverlayAlpha(getEyePosition(), m_vAimVector, 255, 0, 0, 255, true, ftime);
			debugoverlay->AddLineOverlayAlpha(getEyePosition(), v_origin, 0, 0, 255, 255, true, ftime);
		}
	}
#endif
	
	return m_vAimVector;
}

void CBot::modAim ( edict_t *pEntity, Vector &v_origin, Vector *v_desired_offset, Vector &v_size, const float fDist, float fDist2D )
{
	static Vector vel;
	static Vector myvel;
	static Vector enemyvel;
	static float fDistFactor;
	static float fHeadOffset;

	const int iPlayerFlags = CClassInterface::getPlayerFlags(pEntity);

	fHeadOffset = 0.0f;

	if ( rcbot_supermode.GetBool() )
	{
		v_desired_offset->x = 0;
		v_desired_offset->y = 0;
		v_desired_offset->z = v_size.z-1;

		return;
	}

	const CBotWeapon *pWp = getCurrentWeapon();

	float fVelFactor = 0.003125f;

	if ( pWp && pWp->isMelee() )
	{
		fDistFactor = 0.0f;
		fVelFactor = 0.0f;
	}
	else
	{
		if ( fDist < 160.0f )
			fVelFactor = 0.001f;

		fDistFactor = 1.0f - m_pProfile->m_fAimSkill + fDist * 0.000125f * (m_fFov / 90.0f);
	}
	// origin is always the bottom part of the entity
	// add body height
	fHeadOffset += v_size.z-1;

	if ( ENTINDEX(pEntity) <= gpGlobals->maxClients ) // add body height
	{
		// aim for head
		if ( !(iPlayerFlags & FL_DUCKING) && hasSomeConditions(CONDITION_SEE_ENEMY_HEAD) && m_fFov < BOT_DEFAULT_FOV )
			fHeadOffset += v_size.z-1;
	}

	myvel = Vector(0,0,0);
	enemyvel = Vector(0,0,0);

	// change in velocity
	if ( CClassInterface::getVelocity(pEntity,&enemyvel) && CClassInterface::getVelocity(m_pEdict,&myvel) )
	{
		vel = enemyvel - myvel; // relative velocity

		vel = vel * fVelFactor;//0.003125f;

		//fVelocityFactor = exp(-1.0f + ((vel.Length() * 0.003125f)*2)); // divide by max speed
	}
	else
	{
		vel = Vector(0.5f,0.5f,0.5f);
		//fVelocityFactor = 1.0f;
	}

	// velocity
	v_desired_offset->x = randomFloat(-vel.x,vel.x)*fDistFactor*v_size.x;
	v_desired_offset->y = randomFloat(-vel.y,vel.y)*fDistFactor*v_size.y;
	v_desired_offset->z = randomFloat(-vel.z,vel.z)*fDistFactor*v_size.z;

	// target
	v_desired_offset->z += fHeadOffset * m_pProfile->m_fAimSkill + randomFloat(0.0f,1.0f-m_pProfile->m_fAimSkill)*fHeadOffset;

}

void CBot::hearVoiceCommand(edict_t* pPlayer, byte voiceCmd) //Needs properly implemented? [APG]RoboCop[CL]
{
}

void CBot :: grenadeThrown ()
{

}

void CBot::voiceCommand(byte voiceCmd)
{

}

void CBot :: checkCanPickup ( edict_t *pPickup )
{

}

Vector CBot::snipe (const Vector& vAiming)
{
		if ( m_fLookAroundTime < engine->Time() )
		{
			CTraceFilterWorldAndPropsOnly filter; //Unused? [APG]RoboCop[CL]
			float fTime;
			Vector vOrigin = getOrigin(); //Unused? [APG]RoboCop[CL]

			//trace_t *tr = CBotGlobals::getTraceResult();

			m_vLookAroundOffset = Vector(randomFloat(-64.0f,64.0f),randomFloat(-64.0f,64.0f),randomFloat(-64.0f,32.0f));
			// forward
			//CBotGlobals::traceLine(vOrigin,m_vWaypointAim+m_vLookAroundOffset,MASK_SOLID_BRUSHONLY|CONTENTS_OPAQUE,&filter);	

			if ( m_fLookAroundTime == 0.0f )
				fTime = 0.1f;
			else
				fTime = randomFloat(3.0f,7.0f);

			m_fLookAroundTime = engine->Time() + fTime;
#ifndef __linux__
			if ( CClients::clientsDebugging(BOT_DEBUG_NAV) )
			{
				debugoverlay->AddLineOverlay (getOrigin(), m_vWaypointAim, 255,100,100, false, fTime);
				debugoverlay->AddLineOverlay (getOrigin(), m_vWaypointAim+m_vLookAroundOffset, 255,40,40, false, fTime);
			}
#endif

			//m_vWaypointAim = m_vWaypointAim + m_vLookAroundOffset;
		}

		return vAiming+m_vLookAroundOffset;
}

void CBot :: getLookAtVector ()
{
	static bool bDebug = false;

	bDebug = CClients::clientsDebugging(BOT_DEBUG_LOOK);

	switch ( m_iLookTask )
	{
	case LOOK_NONE:
		{
			stopLooking();

			if ( bDebug )
				CClients::clientDebugMsg(BOT_DEBUG_LOOK,"LOOK_NONE",this);
		}
		break;
	case LOOK_VECTOR:
		{
			setLookAt(m_vLookVector);

			if ( bDebug )
				CClients::clientDebugMsg(BOT_DEBUG_LOOK,"LOOK_VECTOR",this);
		}
		break;
	case LOOK_EDICT:
		{
			try
			{

				if (m_pLookEdict.get() != nullptr)
					setLookAt(getAimVector(m_pLookEdict));
				//setLookAt(CBotGlobals::entityOrigin(m_pLookEdict)+Vector(0,0,32));

				if (bDebug)
					CClients::clientDebugMsg(BOT_DEBUG_LOOK, "LOOK_EDICT", this);

			}
			catch (...)
			{
				m_pLookEdict = nullptr;
				setLookAtTask(LOOK_NONE);
			}
		}
		break;
	case LOOK_GROUND:
		{
			setLookAt(m_vMoveTo);
			//setLookAt(getOrigin()-Vector(0,0,64));
			if ( bDebug )
				CClients::clientDebugMsg(BOT_DEBUG_LOOK,"LOOK_GROUND",this);
		}
		break;
	case LOOK_ENEMY:
		{
			try
			{

				if (m_pEnemy.get() != nullptr)
				{
					setLookAt(getAimVector(m_pEnemy));
				}
				else if (m_pLastEnemy)
					setLookAt(m_vLastSeeEnemy);

				if (bDebug)
					CClients::clientDebugMsg(BOT_DEBUG_LOOK, "LOOK_ENEMY", this);
			}
			catch (...)
			{
				m_pEnemy = nullptr;
				setLookAtTask(LOOK_NONE);
			}
		}		
		break;
	case LOOK_LAST_ENEMY:
		{
			if ( m_pLastEnemy )
				setLookAt(m_vLastSeeEnemy);
			//else
			// LOOK_WAYPOINT, below
			if ( bDebug )
				CClients::clientDebugMsg(BOT_DEBUG_LOOK,"LOOK_LAST_ENEMY",this);
		}
		break;
	case LOOK_WAYPOINT_NEXT_ONLY:
		{
			setLookAt(m_vMoveTo);
			break;
		}
	case LOOK_WAYPOINT:
		{
			//static float fWaypointHeight = 0.0f;
			Vector vLook;

			if ( m_pNavigator->nextPointIsOnLadder() )
			{
				Vector vforward;

				vLook = m_pNavigator->getNextPoint();

				const QAngle angle = QAngle(0, m_pNavigator->getNextYaw(), 0);

				AngleVectors(angle,&vforward);

				vforward = vforward/vforward.Length()*64;

				vforward.z = 64.0f;

				setLookAt(vLook + vforward);
			}
			else if ( m_pNavigator->hasNextPoint() && m_pButtons->holdingButton(IN_SPEED) )
			{
				if (m_pNavigator->getNextRoutePoint(&vLook)) {
					setLookAt(vLook);
				} else {
					vLook = m_pNavigator->getPreviousPoint();
					setLookAt(vLook);

					CClients::clientDebugMsg(BOT_DEBUG_AIM,"no valid route point",this);
				}
			}
			else if ( m_pLastEnemy.get()!= nullptr && m_fLastSeeEnemy + 5.0f > engine->Time() )
				setLookAt(m_vLastSeeEnemy);
			else if ( m_fCurrentDanger >= 20.0f && m_pNavigator->getDangerPoint(&vLook) )
				setLookAt(vLook);
			else if ( m_pNavigator->getNextRoutePoint(&vLook) )
				setLookAt(vLook);				
			else
			{
				vLook = m_pNavigator->getPreviousPoint();
				setLookAt(vLook);
			}
				
			if ( bDebug )
				CClients::clientDebugMsg(BOT_DEBUG_LOOK,"LOOK_WAYPOINT",this);
		}
		break;
	case LOOK_WAYPOINT_AIM:
			setLookAt(m_vWaypointAim);
		if ( bDebug )
				CClients::clientDebugMsg(BOT_DEBUG_LOOK,"LOOK_WAYPOINT_AIM",this);
		break;
	case LOOK_BUILD:
		{
			//if ( m_pEnemy && hasSomeConditions(CONDITION_SEE_CUR_ENEMY) )
			//{
			//	setLookAtTask((LOOK_ENEMY));
			//	return;
			//}
			if ( m_fLookAroundTime < engine->Time() )
			{
				const float fTime = randomFloat(2.0f,4.0f);
				m_fLookAroundTime = engine->Time() + fTime;

				m_vLookAroundOffset = Vector(randomFloat(-64.0f,64.0f),randomFloat(-64.0f,64.0f),randomFloat(-64.0f,32.0f));
			}

			setLookAt(m_vWaypointAim+m_vLookAroundOffset);

		if ( bDebug )
				CClients::clientDebugMsg(BOT_DEBUG_LOOK,"LOOK_BUILD",this);
		}
		break;
	case LOOK_SNIPE:
		{
			setLookAt(snipe(m_vWaypointAim));

		if ( bDebug )
				CClients::clientDebugMsg(BOT_DEBUG_LOOK,"LOOK_SNIPE",this);
		}
		break;
	case LOOK_NOISE:
		{
			if ( m_pEnemy && hasSomeConditions(CONDITION_SEE_CUR_ENEMY) )
			{
				setLookAtTask(LOOK_ENEMY);
				return;
			}
			if ( !m_bListenPositionValid || m_fListenTime < engine->Time() ) // already listening to something ?
			{
				setLookAtTask(LOOK_WAYPOINT);
				return;
			}

			setLookAt(m_vListenPosition);

		if ( bDebug )
				CClients::clientDebugMsg(BOT_DEBUG_LOOK,"LOOK_NOISE",this);
		}
		break;


	case LOOK_AROUND:
		{
			if ( m_fLookAroundTime < engine->Time() )
			{
				if ( m_fCurrentDanger < 10.0f || (m_pNavigator->numPaths() == 0 || !m_pNavigator->randomDangerPath(&m_vLookAroundOffset))  )
				{
					// random
					m_vLookAroundOffset = getEyePosition();
				}
					
				m_fLookAroundTime = engine->Time() + randomFloat(2.0f,3.0f);
				m_vLookAroundOffset = m_vLookAroundOffset + Vector(randomFloat(-128,128),randomFloat(-128,128),randomFloat(-16,16));
			}

			setLookAt(m_vLookAroundOffset);

		if ( bDebug )
				CClients::clientDebugMsg(BOT_DEBUG_LOOK,"LOOK_AROUND",this);
		}
		break;
	case LOOK_HURT_ORIGIN:
		{
			setLookAt(m_vHurtOrigin);


		if ( bDebug )
				CClients::clientDebugMsg(BOT_DEBUG_LOOK,"LOOK_HURT_ORIGIN",this);
		}
		break;
	//case LOOK_MAX:
	//	break;
	default:
		break;
	}
}

int CBot :: getPlayerID () const
{
	return m_pPlayerInfo->GetUserID();
}

void CBot :: letGoOfButton (const int button) const
{
	m_pButtons->letGo(button);
}

void CBot :: changeAngles (float fSpeed, const float* fIdeal, float* fCurrent, float* fUpdate)
{
	float current = *fCurrent;
	const float ideal = *fIdeal;

	if (bot_anglespeed.GetFloat() < 0.01f)
		bot_anglespeed.SetValue(0.16f);

	// Really not needed for bigger sensitivities
	// Also this does not take the skill system into account
	if (fSpeed < 15.0f)
		fSpeed = Lerp(m_fEnemyAimLerp, fSpeed, 15.0f);

	const float alphaspeed = fSpeed / 20;

	const float alpha = alphaspeed * bot_anglespeed.GetFloat();
	
	float diff = ideal - current;

	if ( diff < -180.0f )
		diff += 360.0f;
	else if ( diff > 180.0f )
		diff -= 360.0f;

	float delta = diff * alpha + m_fAimMoment * alphaspeed;

	//check for QNAN
	if ( delta != delta ) //Not required? [APG]RoboCop[CL]
		delta = 1.0f;

	m_fAimMoment = m_fAimMoment * alphaspeed + delta * (1.0f-alphaspeed);

	//check for QNAN
	if ( m_fAimMoment != m_fAimMoment ) //Not required? [APG]RoboCop[CL]
		m_fAimMoment = 1.0f;

	current = current + delta;

	if ( current > 180.0f )
		current -= 360.0f;
	else if ( current < -180.0f )
		current += 360.0f;

	*fCurrent = current;

	if ( *fCurrent > 180.0f )
		*fCurrent -= 360.0f;
	else if ( *fCurrent < -180.0f )
		*fCurrent += 360.0f;
}

bool CBot :: select_CWeapon ( CWeapon *pWeapon )
{
	if ( const CBotWeapon *pSelect = m_pWeapons->getWeapon(pWeapon) )
	{
		if ( const int id = pSelect->getWeaponIndex() )
		{
			failWeaponSelect();
			selectWeapon(id);
			return true;
		}

	}

	return false;
}

void CBot :: doLook ()
{
	// what do we want to look at
	getLookAtVector();

	// looking at something?
	if ( lookAtIsValid () )
	{	
		float fSensitivity;
		if ( rcbot_supermode.GetBool() || m_bIncreaseSensitivity || onLadder() )
			fSensitivity = 15.0f;
		else
			fSensitivity = static_cast<float>(m_pProfile->m_iSensitivity);

		QAngle requiredAngles;

		VectorAngles(m_vLookAt-getEyePosition(),requiredAngles);
		CBotGlobals::fixFloatAngle(&requiredAngles.x);
		CBotGlobals::fixFloatAngle(&requiredAngles.y);

		if ( m_iLookTask == LOOK_GROUND )
			requiredAngles.x = 89.0f;

		const CBotCmd command = m_pPlayerInfo->GetLastUserCommand();

		m_vViewAngles = command.viewangles;

		if (m_vViewAngles.x == 0.0f && m_vViewAngles.y == 0.0f) {
			CClients::clientDebugMsg(BOT_DEBUG_AIM, "view angle invalid", this);
		}

		changeAngles(fSensitivity,&requiredAngles.x,&m_vViewAngles.x, nullptr);
		changeAngles(fSensitivity,&requiredAngles.y,&m_vViewAngles.y, nullptr);
		CBotGlobals::fixFloatAngle(&m_vViewAngles.x);
		CBotGlobals::fixFloatAngle(&m_vViewAngles.y);

		// Clamp pitch
		if ( m_vViewAngles.x > 89.0f )
			m_vViewAngles.x = 89.0f;
		else if ( m_vViewAngles.x < -89.0f )
			m_vViewAngles.x = -89.0f;

		// Clamp yaw
		//if ( m_vViewAngles.x > 180.0f )
		//	m_vViewAngles.x = 180.0f;
		//else if ( m_vViewAngles.x < -180.0f )
		//	m_vViewAngles.x = -180.0f;
	}

	m_bIncreaseSensitivity = false;
}

void CBot :: doButtons ()
{
	m_iButtons = m_pButtons->getBitMask();
}

void CBot :: secondaryAttack (const bool bHold, const float fTime) const
{
	float fLetGoTime = 0.15f;
	float fHoldTime = 0.12f;

	if ( bHold )
	{
		fLetGoTime = 0.0f;

		if ( fTime )
			fHoldTime = fTime;
		else
			fHoldTime = 1.0f;
	}

	// not currently in "letting go" stage?
	if ( bHold || m_pButtons->canPressButton(IN_ATTACK2) )
	{
		m_pButtons->holdButton
			(
				IN_ATTACK2,
				0/* reaction time? (time to press)*/,
				fHoldTime/* hold time*/,
				fLetGoTime/*let go time*/
			); 
	}
}

void CBot :: primaryAttack (const bool bHold, const float fTime) const
{
	float fLetGoTime = 0.15f;
	float fHoldTime = 0.12f;

	if ( bHold )
	{
		fLetGoTime = 0.0f;

		if ( fTime )
			fHoldTime = fTime;
		else
			fHoldTime = 2.0f;
	}

	// not currently in "letting go" stage?
	if ( bHold || m_pButtons->canPressButton(IN_ATTACK) )
	{
		m_pButtons->holdButton
			(
				IN_ATTACK,
				0/* reaction time? (time to press)*/,
				fHoldTime/* hold time*/,
				fLetGoTime/*let go time*/
			); 
	}
}

void CBot :: tapButton (const int iButton) const
{
	m_pButtons->tap(iButton);
}
void CBot :: reload () const
{
	if ( m_pButtons->canPressButton(IN_RELOAD) )
		m_pButtons->tap(IN_RELOAD);
}
void CBot :: use () const
{
	if ( m_pButtons->canPressButton(IN_USE) )
		m_pButtons->tap(IN_USE);
}

void CBot :: jump () const
{
	if ( m_pButtons->canPressButton(IN_JUMP) )
	{		
		m_pButtons->holdButton(IN_JUMP,0.0f/* time to press*/,0.5f/* hold time*/,0.5f/*let go time*/); 
		// do the trademark jump & duck
		m_pButtons->holdButton(IN_DUCK,0.2f/* time to press*/,0.3f/* hold time*/,0.5f/*let go time*/); 
	}
}

void CBot :: duck (const bool hold) const
{
	if ( hold || m_pButtons->canPressButton(IN_DUCK) )
		m_pButtons->holdButton(IN_DUCK,0.0f/* time to press*/,1.0f/* hold time*/,0.5f/*let go time*/); 
}

// TODO: perceptron method
bool CBot::wantToFollowEnemy ()
{
	return getHealthPercent() > 1.0f - m_pProfile->m_fBraveness;
}

////////////////////////////
void CBot :: getTasks (unsigned iIgnore)
{
	if ( !m_bLookedForEnemyLast && m_pLastEnemy && CBotGlobals::entityIsAlive(m_pLastEnemy) )
	{
		if ( wantToFollowEnemy() )
		{
			Vector vVelocity = Vector(0,0,0);
			CClient *pClient = CClients::get(m_pLastEnemy);
			CBotSchedule *pSchedule = new CBotSchedule();
			
			CFindPathTask *pFindPath = new CFindPathTask(m_vLastSeeEnemy);	
			
			if ( pClient )
				vVelocity = pClient->getVelocity();

			pSchedule->addTask(pFindPath);
			pSchedule->addTask(new CFindLastEnemy(m_vLastSeeEnemy,vVelocity));

			//////////////
			pFindPath->setNoInterruptions();

			m_pSchedules->add(pSchedule);

			m_bLookedForEnemyLast = true;
		}
	}

	if ( !m_pSchedules->isEmpty() )
		return; // already got some tasks left

	// roam

	if ( CWaypoint *pWaypoint = CWaypoints::getWaypoint(CWaypoints::randomFlaggedWaypoint(getTeam())) )
	{
		m_pSchedules->add(new CBotGotoOriginSched(pWaypoint->getOrigin()));
	}

}

///////////////////////

bool CBots::controlBot(edict_t* pEdict)
{
	CBotProfile* pBotProfile = CBotProfiles::getRandomFreeProfile();

	const int slot = slotOfEdict(pEdict);
	if (slot < 0) {
		logger->Log(LogLevel::ERROR, "Invalid slot value");
	}

	const std::size_t slotIndex = static_cast<std::size_t>(slot);

	if (m_Bots[static_cast<std::size_t>(slotIndex)]->getEdict() == pEdict)
	{
		return false;
	}
	if (pBotProfile == nullptr)
	{
		logger->Log(LogLevel::INFO, "No bot profiles are free, creating a default bot...");

		pBotProfile = CBotProfiles::getDefaultProfile();

		if (pBotProfile == nullptr)
			return false;
	}

	m_Bots[slotIndex]->createBotFromEdict(pEdict, pBotProfile);

	return true;
}

#define SET_PROFILE_DATA_INT(varname,membername) if ( (varname) && *(varname) ) { pBotProfile->membername = std::atoi(varname); }
#define SET_PROFILE_STRING(varname,localname,membername) if ( (varname) && *(varname) ) { (localname) = (char*)(varname); } else { (localname) = pBotProfile->membername; }

bool CBots::controlBot(const char* szOldName, const char* szName, const char* szTeam, const char* szClass)
{
	edict_t* pEdict;

	const char* szOVName = "";

	if ((pEdict = CBotGlobals::findPlayerByTruncName(szOldName)) == nullptr)
	{
		logger->Log(LogLevel::ERROR, "Can't find player");
		return false;
	}

	const int slot = slotOfEdict(pEdict);

	if (slot < 0) {
		logger->Log(LogLevel::ERROR, "Invalid slot value");
	}

	const std::size_t botIndex = static_cast<std::size_t>(slot);

	if (m_Bots[botIndex]->getEdict() == pEdict)
	{
		logger->Log(LogLevel::ERROR, "already controlling player");
		return false;
	}

	if (m_iMaxBots != -1 && CBotGlobals::numPlayersPlaying() >= m_iMaxBots)
	{
		logger->Log(LogLevel::ERROR, "Can't create bot, max_bots reached");
		return false;
	}

	m_flAddKickBotTime = engine->Time() + 2.0f;

	CBotProfile* pBotProfile = CBotProfiles::getRandomFreeProfile();

	if (pBotProfile == nullptr)
	{
		logger->Log(LogLevel::INFO, "No bot profiles are free, creating a default bot...");

		pBotProfile = CBotProfiles::getDefaultProfile();

		if (pBotProfile == nullptr)
			return false;
	}

	SET_PROFILE_DATA_INT(szClass, m_iClass)
	SET_PROFILE_DATA_INT(szTeam, m_iTeam)
	SET_PROFILE_STRING(szName, szOVName, m_szName)

	//IBotController *p = g_pBotManager->GetBotController(pEdict);	

	return m_Bots[botIndex]->createBotFromEdict(pEdict, pBotProfile);
}

bool CBots :: createBot (const char *szClass, const char *szTeam, const char *szName)
{
	CBotMod *pMod = CBotGlobals::getCurrentMod(); // `*pMod` Unused? [APG]RoboCop[CL]
	const char *szOVName = ""; // `szOVName` Unused? [APG]RoboCop[CL]

	if ( m_iMaxBots != -1 && CBotGlobals::numPlayersPlaying() >= m_iMaxBots )
		logger->Log(LogLevel::ERROR, "Can't create bot, max_bots reached");

	m_flAddKickBotTime = engine->Time() + rcbot_addbottime.GetFloat();

	CBotProfile* pBotProfile;
	if (rcbot_nonrandom_profile.GetBool()) {
		pBotProfile = CBotProfiles::getChosenFreeProfile();
	} else {
		pBotProfile = CBotProfiles::getRandomFreeProfile();
	}

	if ( pBotProfile == nullptr)
	{
		logger->Log(LogLevel::WARN, "No bot profiles are free, creating a default bot...");

		pBotProfile = CBotProfiles::getDefaultProfile();

		if ( pBotProfile == nullptr)
			return false;
	}

	SET_PROFILE_DATA_INT(szClass,m_iClass)
	SET_PROFILE_DATA_INT(szTeam,m_iTeam)
	SET_PROFILE_STRING(szName,szOVName,m_szName)

	edict_t* pEdict = g_pBotManager->CreateBot(szOVName);

	if ( pEdict == nullptr)
		return false;

	return m_Bots[static_cast<std::size_t>(slotOfEdict(pEdict))]->createBotFromEdict(pEdict, pBotProfile);
}

int CBots::createDefaultBot(const char* szName) {
	edict_t* pEdict = g_pBotManager->CreateBot(szName);

	if (!pEdict) {
		return -1;
	}

	// hack: there's no way to remove names / profiles here
	CBotProfile* pBotProfile = new CBotProfile(*CBotProfiles::getDefaultProfile());
	pBotProfile->m_szName = CStrings::getString(szName);

	int slotInt = slotOfEdict(pEdict); // Get the slot as an int

	if (slotInt < 0) {
		// Handle invalid slot case
		delete pBotProfile;

		return -1;
	}

	const std::size_t slot = static_cast<std::size_t>(slotInt); // Convert to std::size_t explicitly

	m_Bots[slot]->createBotFromEdict(pEdict, pBotProfile);

	return slotInt; // Return the original int slot value
}

void CBots :: botFunction ( IBotFunction *function )
{
	for ( unsigned i = 0; i < RCBOT_MAXPLAYERS; i ++ )
	{
		if ( m_Bots[i]->inUse() && m_Bots[i]->getEdict() )
			function->execute (m_Bots[i]);
	}
}

int CBots :: slotOfEdict (const edict_t* pEdict)
{
	return engine->IndexOfEdict(pEdict) - 1;
}

void CBots :: init ()
{
	m_Bots = new CBot*[RCBOT_MAXPLAYERS];
	//m_Bots = (CBot**)malloc(sizeof(CBot*) * RCBOT_MAXPLAYERS);

	for ( unsigned i = 0; i < RCBOT_MAXPLAYERS; i ++ )
	{
		switch ( CBotGlobals::getCurrentMod()->getBotType() )
		{
		case BOTTYPE_GENERIC:
			m_Bots[i] = new CBot();
			break;
		case BOTTYPE_DOD:
			m_Bots[i] = new CDODBot();
			break;
		case BOTTYPE_CSS:
			m_Bots[i] = new CCSSBot();
			break;
		case BOTTYPE_HL2DM:
			m_Bots[i] = new CHLDMBot();
			break;
		case BOTTYPE_HL1DM:
			m_Bots[i] = new CHL1DMSrcBot();
			break;
		case BOTTYPE_COOP:
			m_Bots[i] = new CBotCoop();
			break;
		case BOTTYPE_TF2:
			m_Bots[i] = new CBotTF2();//[RCBOT_MAXPLAYERS];
			//CBotGlobals::setEventVersion(2);
			break;
		case BOTTYPE_FF:
			m_Bots[i] = new CBotFF();
			break;
		case BOTTYPE_ZOMBIE:
			m_Bots[i] = new CBotZombie();
			break;
		case BOTTYPE_SYN:
			m_Bots[i] = new CBotSynergy();
			break;
		//case BOTTYPE_BMS: //TODO: Add Black Mesa Support [APG]RoboCop[CL]
		//	m_Bots[i] = new CBotBMS();
		//	break;
		//case BOTTYPE_NS2:
		//	break;
		//case BOTTYPE_MAX:
		//	break;
		default:
			m_Bots[i] = new CBot();
			break;
		}
	}
}
int CBots :: numBots ()
{
	static CBot *pBot;

	int iCount = 0;

	for ( short i = 0; i < RCBOT_MAXPLAYERS; i ++ )
	{
		pBot = m_Bots[i];

		if ( pBot->inUse() )
			iCount++;		
	}

	return iCount;
}

CBot *CBots :: findBotByProfile (const CBotProfile *pProfile)
{
	for ( short i = 0; i < RCBOT_MAXPLAYERS; i ++ )
	{
		CBot* pBot = m_Bots[i];

		if ( pBot->inUse() )
		{
			if ( pBot->isUsingProfile(pProfile) )
				return pBot;
		}
	}
	
	return nullptr;
}

void CBots :: runPlayerMoveAll ()
{
	static CBot *pBot;
	for ( short i = 0; i < RCBOT_MAXPLAYERS; i ++ )
	{
		pBot = m_Bots[i];

		if ( pBot->inUse() )
		{
			pBot->runPlayerMove();
		}
	}
}

//TODO: Experimental [APG]RoboCop[CL]
int CBots::levelInit()
{
	return 0;
}

#define CHECK_STRING(str) (((str)==nullptr)?"NULL":(str))

void CBots :: botThink ()
{
	static CBot *pBot;

	const bool bBotStop = bot_stop.GetInt() > 0;

#ifdef _DEBUG

	CProfileTimer* CBotsBotThink = CProfileTimers::getTimer(BOTS_THINK_TIMER);
	CProfileTimer* CBotThink = CProfileTimers::getTimer(BOT_THINK_TIMER);

	if ( CClients::clientsDebugging(BOT_DEBUG_PROFILE) )
	{
		CBotsBotThink->Start();
	}

#endif

	for ( short i = 0; i < RCBOT_MAXPLAYERS; i ++ )
	{
		pBot = m_Bots[i];

		if ( pBot->inUse() )
		{
			if ( !bBotStop )
			{
				#ifdef _DEBUG

					if ( CClients::clientsDebugging(BOT_DEBUG_PROFILE) )
					{
						CBotThink->Start();
					}

				#endif				

				pBot->setMoveLookPriority(MOVELOOK_THINK);
				pBot->think();
				pBot->setMoveLookPriority(MOVELOOK_EVENT);

				#ifdef _DEBUG

					if ( CClients::clientsDebugging(BOT_DEBUG_PROFILE) )
					{
						CBotThink->Stop();
					}

				#endif
			}

			const char* command = bot_command.GetString();

			if (command && *command)
			{
				helpers->ClientCommand(pBot->getEdict(), command); // Use the cached value [APG]RoboCop[CL]

				bot_command.SetValue("");
			}

			pBot->runPlayerMove();
		}
	}

#ifdef _DEBUG

	if ( CClients::clientsDebugging(BOT_DEBUG_PROFILE) )
	{
		CBotsBotThink->Stop();
	}

#endif

	if ( m_flAddKickBotTime < engine->Time() && needToAddBot() )
	{
		createBot(nullptr, nullptr, nullptr);
	}
	else if ( needToKickBot () )
	{
		if (rcbot_nonrandom_kicking.GetBool()) {
			kickChosenBot();
		} else {
			kickRandomBot();
		}
	}
}

CBot* CBots::getBotPointer(const edict_t* pEdict)
{
	if (!pEdict)
		return nullptr;

	const int slot = slotOfEdict(pEdict);

	if (slot < 0 || slot >= RCBOT_MAXPLAYERS)
		return nullptr;

	CBot* pBot = m_Bots[static_cast<unsigned>(slot)];

	if (pBot->inUse())
		return pBot;

	return nullptr;
}

CBot* CBots::getBot(const int slot)
{
	if (slot < 0 || slot >= RCBOT_MAXPLAYERS)
		return nullptr; //TODO: Experimental - Return nullptr for invalid slot values [APG]RoboCop[CL]
	
	CBot* pBot = m_Bots[static_cast<unsigned>(slot)];
	if (pBot->inUse())
		return pBot;

	return nullptr;
}

void CBots :: freeMapMemory ()
{
	if (m_Bots == nullptr)
		return;

	//bots should have been freed when they disconnected
	// just incase do this 
	for ( short int i = 0; i < RCBOT_MAXPLAYERS; i ++ )
	{
		if ( m_Bots[i] )
			m_Bots[i]->freeMapMemory();
	}
}

void CBots :: freeAllMemory ()
{
	if (m_Bots == nullptr)
		return;

	for ( short i = 0; i < RCBOT_MAXPLAYERS; i ++ )
	{
		if (m_Bots[i] != nullptr)
		{
			m_Bots[i]->freeAllMemory();
			delete m_Bots[i];
			m_Bots[i] = nullptr;
		}
	}

	delete[] m_Bots;
	m_Bots = nullptr;
}

void CBots :: roundStart ()
{
	for ( short i = 0; i < RCBOT_MAXPLAYERS; i ++ )
	{
		if ( m_Bots[i]->inUse() )
			m_Bots[i]->spawnInit();
	}
}

void CBots :: mapInit ()
{
	m_flAddKickBotTime = engine->Time() + 10.0f;
}

bool CBots :: needToAddBot ()
{
	if (rcbot_bot_quota_interval.GetFloat() > 0.0f) {
		return false;
	}

	const int iClients = CBotGlobals::numPlayersPlaying();
	const int iBots = CBots::numBots();

	if ((m_iMinBots!=-1 && iBots < m_iMinBots) || (iClients < m_iMaxBots && m_iMaxBots != -1)) {
		return true;
	}

	return false;
}

bool CBots :: needToKickBot ()
{
	if (rcbot_bot_quota_interval.GetFloat() > 0.0f) {
		return false;
	}

	const int iClients = CBotGlobals::numPlayersPlaying();
	const int iBots = CBots::numBots();

	if ( m_flAddKickBotTime < engine->Time() )
	{
		if ( m_iMinBots != -1 && iBots <= m_iMinBots )
			return false;

		if ( m_iMaxBots > 0 && iClients > m_iMaxBots ) {
			return true;
		}
	}

	return false;
}

void CBots :: kickChosenBot (const unsigned count)
{
        std::vector<CBot*> botList;
        //gather list of bots
        for ( unsigned i = 0; i < RCBOT_MAXPLAYERS; i ++ )
        {
		if ( m_Bots[i]->inUse() )
			botList.emplace_back(m_Bots[i]);
        }

        if ( botList.empty() )
        {
                logger->Log(LogLevel::DEBUG, "kickChosenBot() : No bots to kick");
                return;
        }

	int team;
	int teamA = CBotGlobals::numPlayersOnTeam(2,false);
	int teamB = CBotGlobals::numPlayersOnTeam(3,false);

	unsigned numBotsKicked = 0;
	
	while (numBotsKicked < count && !botList.empty()) {
		// check numBotsOnTeam in case all the remaining bots are on the smaller team
		if ((teamA > teamB) && (CBotGlobals::numBotsOnTeam(2,false) > 0)) {
			team = 2;
		} else if ((teamA < teamB) && (CBotGlobals::numBotsOnTeam(3,false) > 0)) {
			team = 3;
		} else {
			team = 0;
		}

		CBot* pBot = nullptr;
		for (CBot* tBot : botList) {
			if ((team < 2) || (tBot->getTeam() == team)) {
				if ((pBot == nullptr) || ( pBot->getCreateTime() < tBot->getCreateTime() ))
					pBot = tBot;
			}
		}

		if (pBot == nullptr) {
			logger->Log(LogLevel::DEBUG, "kickChosenBot() : No bot to kick");
			return;
		}

		if (pBot->getTeam() == 2)
			teamA--;
		else if (pBot->getTeam() == 3)
			teamB--;

		char szCommand[512];

		snprintf(szCommand, sizeof(szCommand), "kickid %d\n", pBot->getPlayerID());
		engine->ServerCommand(szCommand);

		botList.erase(std::remove(botList.begin(), botList.end(), pBot),
			botList.end());

		numBotsKicked++;
	}

	m_flAddKickBotTime = engine->Time() + 2.0f;
}

void CBots :: kickRandomBot (const unsigned count)
{
	std::vector<int> botList;
	//gather list of bots
	for ( unsigned i = 0; i < RCBOT_MAXPLAYERS; i ++ )
	{
		if ( m_Bots[i]->inUse() )
			botList.emplace_back(m_Bots[i]->getPlayerID());
	}

	if ( botList.empty() )
	{
		logger->Log(LogLevel::DEBUG, "kickRandomBot() : No bots to kick");
		return;
	}

	std::shuffle( botList.begin(), botList.end(), std::mt19937(std::random_device()()));

	// std::random_shuffle was removed in C++17
	// std::random_shuffle ( botList.begin(), botList.end() );

	//std::random_device rd;
	//std::mt19937 g(rd());
	//std::shuffle(botList.begin(), botList.end(), g);

	unsigned numBotsKicked = 0;
	while (numBotsKicked < count && !botList.empty()) {
		char szCommand[512];

		snprintf(szCommand, sizeof(szCommand), "kickid %d\n", botList.back());
		engine->ServerCommand(szCommand);
		numBotsKicked++;
		
		botList.pop_back();
	}

	m_flAddKickBotTime = engine->Time() + 2.0f;
}

void CBots::kickChosenBotOnTeam(const int team)
{
	std::vector<CBot*> botList;
	//gather list of bots
	for ( unsigned i = 0; i < RCBOT_MAXPLAYERS; i ++ )
	{
		if ( m_Bots[i]->inUse() && m_Bots[i]->getTeam() == team)
			botList.emplace_back(m_Bots[i]);
	}

	if ( botList.empty() )
	{
		logger->Log(LogLevel::DEBUG, "kickChosenBotOnTeam() : No bots to kick");
		return;
	}

	CBot* pBot = nullptr;
	for (CBot* tBot : botList) {
		if ((pBot == nullptr) || ( pBot->getCreateTime() < tBot->getCreateTime() ))
			pBot = tBot;
	}

	char szCommand[512];

	snprintf(szCommand, sizeof(szCommand), "kickid %d\n", pBot->getPlayerID());
	engine->ServerCommand(szCommand);

	m_flAddKickBotTime = engine->Time() + 2.0f;
}

void CBots::kickRandomBotOnTeam(const int team)
{
	std::vector<int> botList;
	char szCommand[512];

	//gather list of bots
	for (short int i = 0; i < RCBOT_MAXPLAYERS; i++)
	{
		if (m_Bots[i]->inUse() && m_Bots[i]->getTeam() == team)
		{
			botList.emplace_back(m_Bots[i]->getPlayerID());
		}
	}

	if (botList.empty())
	{
		logger->Log(LogLevel::DEBUG, "kickRandomBotOnTeam() : No bots to kick");
		return;
	}

	const std::size_t botListSize = botList.size(); // Use std::size_t for size

	if (botListSize > 0) {
		const int index = randomInt(0, static_cast<int>(botListSize) - 1);
		snprintf(szCommand, sizeof(szCommand), "kickid %d\n", botList[static_cast<std::size_t>(index)]);
	}

	m_flAddKickBotTime = engine->Time() + 2.0f;

	engine->ServerCommand(szCommand);
}
////////////////////////

CBotLastSee :: CBotLastSee ( edict_t *pEdict ) : m_pLastSee(pEdict)
{
	update();
}

void CBotLastSee :: update ()
{
	if ( m_pLastSee.get() == nullptr || !CBotGlobals::entityIsAlive(m_pLastSee) )
	{
		m_fLastSeeTime = 0.0f;
		m_pLastSee = nullptr;
	}
	else
	{
		m_fLastSeeTime = engine->Time();
		m_vLastSeeLoc = CBotGlobals::entityOrigin(m_pLastSee);
		CClassInterface::getVelocity(m_pLastSee,&m_vLastSeeVel);
	}
}

bool CBotLastSee :: hasSeen (const float fTime)
{
	return m_pLastSee.get() != nullptr && m_fLastSeeTime + fTime > engine->Time();
}

Vector CBotLastSee :: getLocation () const
{
	return m_vLastSeeLoc + m_vLastSeeVel;
}

	//MyEHandle m_pLastSee; // edict
	//float m_fLastSeeTime; // time
	//Vector m_vLastSeeLoc; // location
	//Vector m_vLastSeeVel; // velocity
