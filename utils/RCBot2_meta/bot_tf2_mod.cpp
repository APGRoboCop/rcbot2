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

 // #include "server_class.h"

#include "bot.h"
#include "bot_cvars.h"

#include "in_buttons.h"

#include "bot_configfile.h"
#include "bot_fortress.h"
#include "bot_getprop.h"
#include "bot_globals.h"
#include "bot_mods.h"
#include "bot_navigator.h"
#include "bot_perceptron.h"
#include "bot_sigscan.h"
#include "bot_tf2_points.h"
#include "bot_waypoint.h"
#include "bot_waypoint_locations.h"
#include "bot_weapons.h"

#include <algorithm>
#include <cstring>

#include "rcbot/logging.h"

#if SOURCE_ENGINE == SE_SDK2013 || SOURCE_ENGINE == SE_BMS
#include "valve_minmax_off.h"
#endif

eTFMapType CTeamFortress2Mod :: m_MapType = TF_MAP_CTF;
tf_tele_t CTeamFortress2Mod :: m_Teleporters[RCBOT_MAXPLAYERS];

int CTeamFortress2Mod :: m_iArea = 0;

float CTeamFortress2Mod::m_fSetupTime = 0.0f;
float CTeamFortress2Mod::m_fRoundTime = 0.0f;

MyEHandle CTeamFortress2Mod::m_pFlagCarrierRed = MyEHandle(nullptr);
MyEHandle CTeamFortress2Mod::m_pFlagCarrierBlue = MyEHandle(nullptr);

float CTeamFortress2Mod::m_fArenaPointOpenTime = 0.0f;
float CTeamFortress2Mod::m_fPointTime = 0.0f;

tf_sentry_t CTeamFortress2Mod::m_SentryGuns[RCBOT_MAXPLAYERS];	// used to let bots know if sentries have been sapped or not
tf_disp_t  CTeamFortress2Mod::m_Dispensers[RCBOT_MAXPLAYERS];	// used to let bots know where friendly/enemy dispensers are

MyEHandle CTeamFortress2Mod::m_pResourceEntity = MyEHandle(nullptr);
MyEHandle CTeamFortress2Mod::m_pGameRules = MyEHandle(nullptr);

bool CTeamFortress2Mod::m_bAttackDefendMap = false;
int CTeamFortress2Mod::m_Cappers[MAX_CONTROL_POINTS];
int CTeamFortress2Mod::m_iCapDefenders[MAX_CONTROL_POINTS];
bool CTeamFortress2Mod::m_bHasRoundStarted = true;
bool CTeamFortress2Mod::m_bDontClearPoints = false;
int CTeamFortress2Mod::m_iFlagCarrierTeam = 0;
MyEHandle CTeamFortress2Mod::m_pBoss = MyEHandle(nullptr);
bool CTeamFortress2Mod::m_bBossSummoned = false;
MyEHandle CTeamFortress2Mod::pMediGuns[RCBOT_MAXPLAYERS];
CTFObjectiveResource CTeamFortress2Mod::m_ObjectiveResource = CTFObjectiveResource();
CTeamControlPointMaster *CTeamFortress2Mod::m_PointMaster = nullptr;
CTeamRoundTimer CTeamFortress2Mod::m_Timer;
MyEHandle CTeamFortress2Mod::m_PointMasterResource = MyEHandle(nullptr);
CTeamControlPointRound *CTeamFortress2Mod::m_pCurrentRound = nullptr;

bool CTeamFortress2Mod::bFlagStateDefault = true;

MyEHandle CTeamFortress2Mod::m_pPayLoadBombBlue = MyEHandle(nullptr);
MyEHandle CTeamFortress2Mod::m_pPayLoadBombRed = MyEHandle(nullptr);

bool CTeamFortress2Mod::m_bRoundOver = false;
int CTeamFortress2Mod::m_iWinningTeam = 0;
int CTeamFortress2Mod::m_iLastWinningTeam = 0;

Vector CTeamFortress2Mod::m_vFlagLocationBlue = Vector(0,0,0);
Vector CTeamFortress2Mod::m_vFlagLocationRed = Vector(0,0,0);

bool CTeamFortress2Mod::m_bFlagLocationValidBlue = false;
bool CTeamFortress2Mod::m_bFlagLocationValidRed = false;

bool CTeamFortress2Mod::m_bMVMFlagStartValid = false;
Vector CTeamFortress2Mod::m_vMVMFlagStart = Vector(0,0,0);
bool CTeamFortress2Mod::m_bMVMCapturePointValid = false;
Vector CTeamFortress2Mod::m_vMVMCapturePoint = Vector(0,0,0);
bool CTeamFortress2Mod::m_bMVMAlarmSounded = false;
float CTeamFortress2Mod::m_fMVMCapturePointRadius = 0.0f;

int CTeamFortress2Mod::m_iCapturePointWptID = -1;
int CTeamFortress2Mod::m_iFlagPointWptID = -1;

MyEHandle CTeamFortress2Mod::m_pNearestTankBoss = nullptr;

float CTeamFortress2Mod::m_fNearestTankDistance = 0.0f;
Vector CTeamFortress2Mod::m_vNearestTankLocation = Vector(0, 0, 0);

bool CTeamFortress2Mod::isSuddenDeath()
{
	// Bot weapon Randomizer -- leonardo
	if (!mp_stalemate_enable.IsValid() || !mp_stalemate_enable.GetBool() || isMapType(TF_MAP_ARENA))
		return false;

	void *pGameRules = GetGameRules();

	if (pGameRules)
	{
		const int iRoundState = CClassInterface::TF2_getRoundState(pGameRules);
		 
		return iRoundState == RoundState_Stalemate;
	}

	return false;
}

bool CTeamFortress2Mod::isMedievalMode()
{
	return CClassInterface::TF2_IsMedievalMode(GetGameRules());
}

bool CTeamFortress2Mod::checkWaypointForTeam(CWaypoint* pWpt, const int iTeam)
{
	// Returns true if team can go to waypoint
	return m_bRoundOver || ((!pWpt->hasFlag(CWaypointTypes::W_FL_NOBLU) || iTeam != TF2_TEAM_BLUE) && (!pWpt->hasFlag(CWaypointTypes::W_FL_NORED) || iTeam != TF2_TEAM_RED));
}


bool CTeamFortress2Mod :: isWaypointAreaValid (const int iWptArea, const int iWptFlags)
{
	return CTeamFortress2Mod::m_ObjectiveResource.isWaypointAreaValid(iWptArea,iWptFlags);
}
///////////////////////////
bool CTeamFortress2Mod :: withinEndOfRound (const float fTime)	
{
	if ( m_Timer.m_flTimerEndTime )
		return gpGlobals->curtime > *m_Timer.m_flTimerEndTime - fTime;

	return false;
}


void CTeamFortress2Mod :: getTeamOnlyWaypointFlags (const int iTeam, int *iOn, int *iOff)
{
	if ( iTeam == TF2_TEAM_BLUE )
	{
		*iOn = CWaypointTypes::W_FL_NORED;
		*iOff = CWaypointTypes::W_FL_NOBLU;
	}
	else if ( iTeam == TF2_TEAM_RED )
	{
		*iOn = CWaypointTypes::W_FL_NOBLU;
		*iOff = CWaypointTypes::W_FL_NORED;
	}
}

void CTeamFortress2Mod ::modFrame ()
{
	if( m_bPlayerHasSpawned )
	{
		if ( m_ObjectiveResource.m_ObjectiveResource == NULL )
		{
			m_ObjectiveResource.m_ObjectiveResource = CClassInterface::FindEntityByNetClass(gpGlobals->maxClients+1, "CTFObjectiveResource");
		
			if ( m_ObjectiveResource.m_ObjectiveResource.get() != nullptr)			
				m_ObjectiveResource.setup();			
		}
		else
			CTeamFortress2Mod::m_ObjectiveResource.think();
		/*
		if (m_pGameRules.get() == NULL)
		{
			//m_pGameRules = CClassInterface::FindEntityByClassnameNearest(Vector(0, 0, 0), "tf_gamerules_data", 99999.0f, NULL);
			m_pGameRules = CClassInterface::FindEntityByNetClass(gpGlobals->maxClients + 1, "CTFGameRulesProxy");

			if (m_pGameRules.get())
			{
				const char *classname = m_pGameRules.get()->GetClassName();

				logger->Log(LogLevel::DEBUG, "Found gamerules %s", classname);
			}
		}*/
	}
}

void CTeamFortress2Mod :: initMod ()
{
//	unsigned i;
	// Setup Weapons

    CWeapons::loadWeapons(m_szWeaponListName == nullptr ? "TF2" : m_szWeaponListName, TF2Weaps.data());
	//CWeapons::loadWeapons("TF2", TF2Weaps);
	/*
	i = 0;
	while ( TF2Weaps[i].szWeaponName[0] != '\0' )  //for ( i = 0; i < TF2_WEAPON_MAX; i ++ )	
		CWeapons::addWeapon(new CWeapon(TF2Weaps[i++]));//.iSlot,TF2Weaps[i].szWeaponName,TF2Weaps[i].iId,TF2Weaps[i].m_iFlags,TF2Weaps[i].m_iAmmoIndex,TF2Weaps[i].minPrimDist,TF2Weaps[i].maxPrimDist,TF2Weaps[i].m_iPreference,TF2Weaps[i].m_fProjSpeed));
		*/
	CRCBotTF2UtilFile::loadConfig();

	//memset(g_fBotUtilityPerturb,0,sizeof(float)*TF_CLASS_MAX*BOT_UTIL_MAX);
}


void CTeamFortress2Mod :: mapInit ()
{
	CBotMod::mapInit();

	m_vNearestTankLocation = Vector(0, 0, 0);

	const string_t mapname = gpGlobals->mapname;

	const char *szmapname = mapname.ToCStr();

	m_iLastWinningTeam = 0;
	m_iWinningTeam = 0;
	m_pResourceEntity = nullptr;
	
	m_ObjectiveResource.m_ObjectiveResource = nullptr;
	m_ObjectiveResource.reset();
	
	m_PointMaster = nullptr;
	m_PointMasterResource = nullptr;
	m_pCurrentRound = nullptr;
	m_Timer = CTeamRoundTimer();
	
	bFlagStateDefault = true;
	m_bFlagLocationValidBlue = false;
	m_bFlagLocationValidRed = false;
	m_bMVMAlarmSounded = false;
	m_bMVMFlagStartValid = false;
	m_bMVMCapturePointValid = false;
	
	m_fMVMCapturePointRadius = 0.0f;
	m_iCapturePointWptID = -1;
	m_iFlagPointWptID = -1;

	if (std::strncmp(szmapname, "ctf_", 4) == 0 || std::strncmp(szmapname, "quake_turbine", 13) == 0 || std::strncmp(szmapname, "cp_dustcity", 11) == 0 || std::strncmp(szmapname, "pass_", 5) == 0 || std::strncmp(szmapname, "pd_", 3) == 0 || std::strncmp(szmapname, "vip_", 4) == 0 || std::strncmp(szmapname, "od_", 3) == 0) // Quake Turbine is CTF. - RussiaTails
		m_MapType = TF_MAP_CTF; // capture the flag
	else if (std::strncmp(szmapname, "cp_", 3) == 0 || std::strncmp(szmapname, "cqt_", 4) == 0 || std::strncmp(szmapname, "conquest_", 9) == 0 || std::strncmp(szmapname, "dom_", 4) == 0 || std::strncmp(szmapname, "2koth_", 6) == 0 || std::strncmp(szmapname, "ctf_chouhen", 11) == 0 || std::strncmp(szmapname, "ctf_haarp", 9) == 0 || std::strncmp(szmapname, "falling_cp", 10) == 0) // Conquest, 2koth and DOM works fine as CP_. - RussiaTails
		m_MapType = TF_MAP_CP; // control point
	else if (std::strncmp(szmapname, "tc_", 3) == 0)
		m_MapType = TF_MAP_TC; // territory control
	else if (std::strncmp(szmapname, "pl_", 3) == 0 || std::strncmp(szmapname, "tow_", 4) == 0 || std::strncmp(szmapname, "cp_helmsdeep_v2", 15) == 0) // Tug of War works fine as Payload - RussiaTails
		m_MapType = TF_MAP_CART; // pipeline
	else if (std::strncmp(szmapname, "plr_", 4) == 0 || std::strncmp(szmapname, "arena_tinyrock", 14) == 0 || std::strncmp(szmapname, "arena_hailstone", 15) == 0) // to make bots push payloads on these maps. - RussiaTails
		m_MapType = TF_MAP_CARTRACE; // pipeline racing
	else if (std::strncmp(szmapname, "arena_", 6) == 0 || std::strncmp(szmapname, "vsh_", 4) == 0)  // pongo1321
		m_MapType = TF_MAP_ARENA; // arena mode (also fallback for VS Saxton Hale gamemode)
	//else if ( std::strncmp(szmapname,"arena_",6) == 0 )
	//	m_MapType = TF_MAP_ARENA; // arena mode
	else if (std::strncmp(szmapname, "koth_", 5) == 0 || std::strncmp(szmapname, "ctk_", 4) == 0)  // Control the Keep works almost the same as KOTH. - RussiaTails
		m_MapType = TF_MAP_KOTH; // king of the hill
	else if (std::strncmp(szmapname, "sd_", 3) == 0 || std::strncmp(szmapname, "sdr_", 4) == 0)  // Object Destruction and Special Delivery Race (I dunno why it's named like this when it works as a usual sd_) works the same as SD_. - RussiaTails
		m_MapType = TF_MAP_SD; // special delivery
	else if (std::strncmp(szmapname, "tr_", 3) == 0)
		m_MapType = TF_MAP_TR; // training mode
	else if (std::strncmp(szmapname, "mvm_", 4) == 0 || std::strncmp(szmapname, "gd_", 3) == 0 || std::strncmp(szmapname, "stt_", 4) == 0) // gd is MvM Guardian, for now there are only two maps exist with this prefix. Moved stt to mvm to make bots attack a tank - RussiaTails
		m_MapType = TF_MAP_MVM; // mann vs machine
	else if (std::strncmp(szmapname, "rd_", 3) == 0)
		m_MapType = TF_MAP_RD; // robot destruction
	else if (std::strncmp(szmapname, "zi_", 3) == 0)
		m_MapType = TF_MAP_ZI; // Zombie Infection //TODO: add support for those gamemodes [APG]RoboCop[CL]
	else
		m_MapType = TF_MAP_DM; // deathmatch //TODO: to prevent bots from idling in their spawns by giving them basic tasks [APG]RoboCop[CL]

	m_iArea = 0;

	m_fSetupTime = 5.0f; // 5 seconds normal

	m_fRoundTime = 0.0f;

	m_pFlagCarrierRed = nullptr;
	m_pFlagCarrierBlue = nullptr;
	m_iFlagCarrierTeam = 0;
	m_bDontClearPoints = false;

	for ( unsigned i = 0; i < RCBOT_MAXPLAYERS; i ++ )
	{
		m_Teleporters[i].m_iWaypoint = -1;
		m_Teleporters[i].m_fLastTeleported = 0.0f;
		m_Teleporters[i].entrance = MyEHandle(nullptr);
		m_Teleporters[i].exit = MyEHandle(nullptr);
		m_Teleporters[i].sapper = MyEHandle(nullptr);
		m_SentryGuns[i].sapper = MyEHandle(nullptr);
		m_SentryGuns[i].sentry = MyEHandle(nullptr);
		m_Dispensers[i].sapper = MyEHandle(nullptr);
		m_Dispensers[i].disp = MyEHandle(nullptr);
		pMediGuns[i] = nullptr;
	}

	m_bAttackDefendMap = false;
	m_pBoss = nullptr;
	m_bBossSummoned = false;

	resetCappers();
	resetDefenders();
	//CPoints::loadMapScript();
}

int CTeamFortress2Mod :: getTeleporterWaypoint (const edict_t *pTele)
{
	for (tf_tele_t& m_Teleporter : m_Teleporters)
	{
		if (m_Teleporter.exit.get() == pTele )
			return m_Teleporter.m_iWaypoint; 
	}

	return -1;
}

// Naris @ AlliedModders .net

bool CTeamFortress2Mod :: TF2_IsPlayerZoomed(edict_t *pPlayer)
{
	const int pcond = CClassInterface :: getTF2Conditions(pPlayer);
	return (pcond & TF2_PLAYER_ZOOMED) == TF2_PLAYER_ZOOMED;
}

bool CTeamFortress2Mod :: TF2_IsPlayerSlowed(edict_t *pPlayer)
{
	const int pcond = CClassInterface :: getTF2Conditions(pPlayer);
	return (pcond & TF2_PLAYER_SLOWED) == TF2_PLAYER_SLOWED;
}

bool CTeamFortress2Mod :: TF2_IsPlayerDisguised(edict_t *pPlayer)
{
	const int pcond = CClassInterface :: getTF2Conditions(pPlayer);
	return (pcond & TF2_PLAYER_DISGUISED) == TF2_PLAYER_DISGUISED;
}

bool CTeamFortress2Mod :: TF2_IsPlayerTaunting ( edict_t *pPlayer )
{
	const int pcond = CClassInterface :: getTF2Conditions(pPlayer);
	return (pcond & TF2_PLAYER_TAUNTING) == TF2_PLAYER_TAUNTING;
}

bool CTeamFortress2Mod :: TF2_IsPlayerCloaked(edict_t *pPlayer)
{
	const int pcond = CClassInterface :: getTF2Conditions(pPlayer);
	return (pcond & TF2_PLAYER_CLOAKED) == TF2_PLAYER_CLOAKED;
}

bool CTeamFortress2Mod :: TF2_IsPlayerKrits(edict_t *pPlayer)
{
	const int pcond = CClassInterface :: getTF2Conditions(pPlayer);
	return (pcond & TF2_PLAYER_KRITS) == TF2_PLAYER_KRITS;

	//return false; //Unreachable? [APG]RoboCop[CL]
}

bool CTeamFortress2Mod :: TF2_IsPlayerInvuln(edict_t *pPlayer)
{
	if ( CBotGlobals::isPlayer(pPlayer) )
	{
		const int pcond = CClassInterface :: getTF2Conditions(pPlayer);
		return (pcond & TF2_PLAYER_INVULN) == TF2_PLAYER_INVULN;
	}

	return false;
}

bool CTeamFortress2Mod :: TF2_IsPlayerOnFire(edict_t *pPlayer)
{
	const int pcond = CClassInterface :: getTF2Conditions(pPlayer);
	return (pcond & TF2_PLAYER_ONFIRE) == TF2_PLAYER_ONFIRE;
}

//TODO: Experimental [APG]RoboCop[CL]
int CTeamFortress2Mod::numPlayersOnTeam(int iTeam, bool bAliveOnly)
{
	return 0;
}

int CTeamFortress2Mod ::numClassOnTeam(const int iTeam, const int iClass)
{
	int num = 0;

	for ( int i = 1; i <= CBotGlobals::numClients(); i ++ )
	{
		edict_t* pEdict = INDEXENT(i);

		if ( CBotGlobals::entityIsValid(pEdict) )
		{
			if ( getTeam(pEdict) == iTeam )
			{
				if ( CClassInterface::getTF2Class(pEdict) == iClass )
					num++;
			}
		}
	}

	return num;
}


edict_t *CTeamFortress2Mod :: findResourceEntity()
{
	if ( !m_pResourceEntity ) // crash fix
		m_pResourceEntity = CClassInterface::FindEntityByNetClass(gpGlobals->maxClients+1, "CTFPlayerResource");

	return m_pResourceEntity;
}

TF_Class CTeamFortress2Mod :: getSpyDisguise ( edict_t *pPlayer )
{
	static int iClass;
	static int iTeam;
	static int iIndex;
	static int iHealth;

	CClassInterface::getTF2SpyDisguised(pPlayer,&iClass,&iTeam,&iIndex,&iHealth);

	return static_cast<TF_Class>(iClass);
}

float CTeamFortress2Mod :: TF2_GetClassSpeed(const int iClass) 
{ 
switch (iClass) 
{ 
	case TF_CLASS_SCOUT: return 133.0f; 
	case TF_CLASS_SOLDIER: return 80.0f; 
	case TF_CLASS_DEMOMAN: return 93.0f; 
	case TF_CLASS_MEDIC: return 109.0f; 
	case TF_CLASS_PYRO: return 100.0f; 
	case TF_CLASS_SPY: return 109.0f; 
	case TF_CLASS_ENGINEER: return 100.0f;
	case TF_CLASS_HWGUY: return 77.0f;
	case TF_CLASS_SNIPER: return 100.0f; 
} 
return 0.0f; 
} 
 
float CTeamFortress2Mod :: TF2_GetPlayerSpeed(edict_t *pPlayer, const TF_Class iClass) 
{ 
	static float fSpeed;

	fSpeed = CClassInterface::getMaxSpeed(pPlayer);// * CClassInterface::getSpeedFactor(pPlayer);

	if ( fSpeed == 0.0f )
	{
		if (TF2_IsPlayerSlowed(pPlayer)) 
			return 30.0f;
		return TF2_GetClassSpeed(iClass) * 1.58f;
	}

	return fSpeed;
} 

int CTeamFortress2Mod :: getTeam ( edict_t *pEntity )
{
	return CClassInterface::getTeam (pEntity);
	//return *((int*)(pEntity->GetIServerEntity()->GetBaseEntity())+110);
}

int CTeamFortress2Mod :: getSentryLevel ( edict_t *pSentry )
{
	const string_t model = pSentry->GetIServerEntity()->GetModelName();
	const char *szmodel = model.ToCStr();

	return szmodel[24] - '1'+1;
	//if ( pSentry && pSentry->
}

int CTeamFortress2Mod :: getDispenserLevel ( edict_t *pDispenser )
{
	const string_t model = pDispenser->GetIServerEntity()->GetModelName();
	const char *szmodel = model.ToCStr();

	if ( std::strcmp(szmodel,"models/buildables/dispenser_light.mdl") == 0 )
		return 1;

	return szmodel[31] - '1'+1;
	//if ( pSentry && pSentry->
}

int CTeamFortress2Mod :: getEnemyTeam (const int iTeam)
{
	return iTeam == TF2_TEAM_BLUE?TF2_TEAM_RED:TF2_TEAM_BLUE;
}

/*
------------------int : m_nDisguiseTeam
------------------int : m_nDisguiseClass
------------------int : m_iDisguiseTargetIndex
------------------int : m_iDisguiseHealth

*/

bool CTeamFortress2Mod :: isDispenser (edict_t *pEntity, const int iTeam, const bool checkcarrying)
{
	return (!iTeam || iTeam == getTeam(pEntity)) && std::strcmp(pEntity->GetClassName(),"obj_dispenser")==0 && (checkcarrying||!CClassInterface::isSentryGunBeingPlaced(pEntity));
}

bool CTeamFortress2Mod :: isFlag (edict_t *pEntity, const int iTeam)
{
	return (!iTeam || getEnemyTeam(iTeam) == getTeam(pEntity)) && std::strcmp(pEntity->GetClassName(),"item_teamflag")==0;
}

bool CTeamFortress2Mod ::isBoss (edict_t *pEntity, float *fFactor)
{
	if ( m_bBossSummoned )
	{
		if ( m_pBoss.get() && CBotGlobals::entityIsAlive(m_pBoss.get()) )
			return m_pBoss.get() == pEntity;
		if ( std::strcmp(pEntity->GetClassName(),"merasmus")==0||
			std::strcmp(pEntity->GetClassName(),"headless_hatman")==0||
			std::strcmp(pEntity->GetClassName(),"eyeball_boss")==0||
			std::strcmp(pEntity->GetClassName(),"tf_zombie")==0||
			std::strcmp(pEntity->GetClassName(),"base_boss")==0) // For Krampus and any other NPS which uses base_boss entity - RussiaTails
		{
			m_pBoss = pEntity;
			return true;
		}
	}
	else if (CTeamFortress2Mod::isMapType(TF_MAP_CARTRACE) || isMapType(TF_MAP_CART) || isMapType(TF_MAP_KOTH) || isMapType(TF_MAP_PASS)
		|| isMapType(TF_MAP_CP) || isMapType(TF_MAP_PD) || isMapType(TF_MAP_ARENA) || isMapType(TF_MAP_SD) || isMapType(TF_MAP_DM))
	{
		if ( m_pBoss.get() == pEntity )
			return true;
		// TODO: to allow RCBot to target Mafia Skeleton in pl_spineyard [APG]RoboCop[CL]
		// for bots to target skeletons [APG]RoboCop[CL]
		if (std::strcmp(pEntity->GetClassName(),"tf_zombie")==0||
			std::strcmp(pEntity->GetClassName(),"base_boss")==0) // As you wish - RussiaTails
		{
			m_pBoss = pEntity;
			return true;
		}
		// TODO: to prevent shooting at ghosts? [APG]RoboCop[CL]
		if (std::strcmp(pEntity->GetClassName(),"ghost")==0)
		{
			m_pBoss = pEntity;
			return false;
		}
		
	}
	else if ( CTeamFortress2Mod::isMapType(TF_MAP_MVM) )
	{
		if ( m_pBoss.get() == pEntity )
			return true;
		if (isTankBoss(pEntity))
		{
			if (fFactor != nullptr)
				*fFactor = 200.0f;

			m_pBoss = pEntity;
			return true;
		}
	}

	return false;
}


void CTeamFortress2Mod :: updateTeleportTime (const edict_t* pOwner)
{
	m_Teleporters[ENTINDEX(pOwner)-1].m_fLastTeleported = engine->Time();
}

float CTeamFortress2Mod :: getTeleportTime (const edict_t* pOwner)
{
	return m_Teleporters[ENTINDEX(pOwner)-1].m_fLastTeleported;
}

bool CTeamFortress2Mod :: isSentry (edict_t *pEntity, const int iTeam, const bool checkcarrying)
{
	return (!iTeam || iTeam == getTeam(pEntity)) && std::strcmp(pEntity->GetClassName(),"obj_sentrygun")==0 && (checkcarrying||!CClassInterface::isSentryGunBeingPlaced(pEntity));
}

bool CTeamFortress2Mod::isTankBoss(const edict_t* pEntity)
{
	return std::strcmp(pEntity->GetClassName(), "tank_boss") == 0;
}

bool CTeamFortress2Mod :: isTeleporter (edict_t *pEntity, const int iTeam, const bool checkcarrying)
{
	return (!iTeam || iTeam == getTeam(pEntity)) && std::strcmp(pEntity->GetClassName(),"obj_teleporter")==0 && (checkcarrying||!CClassInterface::isSentryGunBeingPlaced(pEntity));
}

bool CTeamFortress2Mod :: isTeleporterEntrance (edict_t *pEntity, const int iTeam, const bool checkcarrying)
{
	return isTeleporter(pEntity,iTeam) && CClassInterface::isTeleporterMode(pEntity,TELE_ENTRANCE) && (checkcarrying||!CClassInterface::isSentryGunBeingPlaced(pEntity));
}

bool CTeamFortress2Mod :: isTeleporterExit (edict_t *pEntity, const int iTeam, const bool checkcarrying)
{
	return isTeleporter(pEntity,iTeam) && CClassInterface::isTeleporterMode(pEntity,TELE_EXIT) && (checkcarrying||!CClassInterface::isSentryGunBeingPlaced(pEntity));
}

bool CTeamFortress2Mod :: isPipeBomb (edict_t *pEntity, const int iTeam)
{
	return (!iTeam || iTeam == getTeam(pEntity)) && std::strcmp(pEntity->GetClassName(),"tf_projectile_pipe_remote")==0;
}

bool CTeamFortress2Mod :: isHurtfulPipeGrenade (edict_t *pEntity, edict_t *pPlayer, const bool bCheckOwner)
{
	if ( std::strcmp(pEntity->GetClassName(),"tf_projectile_pipe")==0 )
	{
		if ( bCheckOwner && CClassInterface::getPipeBombOwner(pEntity) == pPlayer )
			return true;

		const int iPlayerTeam = getTeam(pPlayer);
		const int iGrenTeam = getTeam(pEntity);

		return iPlayerTeam != iGrenTeam;
	}

	return false;
}

bool CTeamFortress2Mod :: isRocket (edict_t *pEntity, const int iTeam)
{
	return (!iTeam || iTeam == getTeam(pEntity)) && std::strcmp(pEntity->GetClassName(),"tf_projectile_rocket")==0;
}

edict_t *CTeamFortress2Mod:: getMediGun ( edict_t *pPlayer )
{
	if ( CClassInterface::getTF2Class(pPlayer) == TF_CLASS_MEDIC )
		return pMediGuns[ENTINDEX(pPlayer)-1];
	return nullptr;
}

void CTeamFortress2Mod :: findMediGun ( edict_t *pPlayer )
{
	static int i;
	static edict_t *pEnt;
	static Vector vOrigin;

	vOrigin = CBotGlobals::entityOrigin(pPlayer);

	for ( i = gpGlobals->maxClients+1; i < gpGlobals->maxEntities; i ++ )
	{
		pEnt = INDEXENT(i);

		if ( pEnt && CBotGlobals::entityIsValid(pEnt) )
		{
			if ( std::strcmp(pEnt->GetClassName(),"tf_weapon_medigun") == 0 )
			{
				if ( CBotGlobals::entityOrigin(pEnt) == vOrigin )
				{
					pMediGuns[ENTINDEX(pPlayer)-1] = pEnt;
					break;
				}
			}
		}
	}
}

// get the teleporter exit of an entrance
edict_t *CTeamFortress2Mod :: getTeleporterExit (const edict_t *pTele)
{
	edict_t *pExit;

	for (tf_tele_t& m_Teleporter : m_Teleporters)
	{
		if (m_Teleporter.entrance.get() == pTele )
		{
			if ( (pExit = m_Teleporter.exit.get()) != nullptr)
			{
				return pExit;
			}

			return nullptr;
		}
	}

	return nullptr;
}

// check if the entity is a health kit
bool CTeamFortress2Mod :: isHealthKit (const edict_t* pEntity)
{
	return std::strncmp(pEntity->GetClassName(),"item_healthkit",14) == 0;
}

bool CTeamFortress2Mod :: isAreaOwnedByTeam (const int iArea, const int iTeam)
{
	if ( CTeamFortress2Mod::m_ObjectiveResource.isInitialised() )
	{
		return iArea==0 || CTeamFortress2Mod::m_ObjectiveResource.GetOwningTeam(CTeamFortress2Mod::m_ObjectiveResource.m_WaypointAreaToIndexTranslation[iArea])==iTeam;
	}

	return false;
}

// cehc kif the team can pick up a flag in SD mode (special delivery)
bool CTeamFortress2Mod::canTeamPickupFlag_SD(const int iTeam, const bool bGetUnknown)
{
	if ( isArenaPointOpen() )
	{
		if ( bGetUnknown )
			return m_iFlagCarrierTeam==iTeam;
		return m_iFlagCarrierTeam==0;
	}

	return false;
}
// For MVM only
bool CTeamFortress2Mod::getFlagLocation (const int iTeam, Vector *vec)
{
	if ( getDroppedFlagLocation(iTeam,vec) )
	{
		return true;
	}
	if ( hasRoundStarted() && isFlagCarried(iTeam) )
	{
		*vec = CBotGlobals::entityOrigin(getFlagCarrier(iTeam));
		return true;
	}

	return false;
}

// for special delivery mod and MVM
void CTeamFortress2Mod::flagReturned(const int iTeam)
{
	m_iFlagCarrierTeam = 0;
	bFlagStateDefault = true;

	if ( iTeam == TF2_TEAM_BLUE )
	{
		m_bFlagLocationValidBlue = false;
	}
	else if ( iTeam == TF2_TEAM_RED )
	{
		m_bFlagLocationValidRed = false;
	}
}

void CTeamFortress2Mod:: flagPickedUp (const int iTeam, edict_t *pPlayer)
{
	bFlagStateDefault = false;
	if ( iTeam == TF2_TEAM_BLUE )
	{
		m_pFlagCarrierBlue = pPlayer;
		m_bFlagLocationValidBlue = false;
	}
	else if ( iTeam == TF2_TEAM_RED )
	{
		m_pFlagCarrierRed = pPlayer;
		m_bFlagLocationValidRed = false;
	}

	m_iFlagCarrierTeam = iTeam;

	CBotTF2FunctionEnemyAtIntel func(iTeam, CBotGlobals::entityOrigin(pPlayer), EVENT_FLAG_PICKUP);
	CBots::botFunction(&func);
}

bool CTeamFortress2Mod :: isArenaPointOpen ()
{
	return m_fArenaPointOpenTime < engine->Time();
}

void CTeamFortress2Mod :: resetSetupTime ()
{
	m_fRoundTime = engine->Time() + m_Timer.getSetupTime();
	m_fArenaPointOpenTime = engine->Time() + m_fPointTime;
}

bool CTeamFortress2Mod::hasRoundStarted()
{
	return m_bHasRoundStarted || (!isMapType(TF_MAP_MVM) && engine->Time() > m_fRoundTime);

	//return (engine->Time() > m_fRoundTime);
}

void CTeamFortress2Mod :: setPointOpenTime (const int time)
{
	m_fArenaPointOpenTime = 0.0f;
	m_fPointTime = static_cast<float>(time);
}

void CTeamFortress2Mod :: setSetupTime (const int time)
{
  m_fRoundTime = 0.0f;
  m_fSetupTime = static_cast<float>(time);
}

bool CTeamFortress2Mod :: isAmmo (const edict_t* pEntity)
{
	static const char *szClassname;

	szClassname = pEntity->GetClassName();

	return std::strcmp(szClassname,"tf_ammo_pack") == 0 || std::strncmp(szClassname,"item_ammopack",13) == 0;
}

//TODO: Experimental [APG]RoboCop[CL]
int CTeamFortress2Mod::getArea()
{
	return 0;
}

bool CTeamFortress2Mod :: isPayloadBomb ( edict_t *pEntity, const int iTeam )
{
	return std::strncmp(pEntity->GetClassName(),"mapobj_cart_dispenser",21) == 0 && CClassInterface::getTeam(pEntity)==iTeam;
}


void CTeamFortress2Mod::checkMVMTankBoss(edict_t *pEntity)
{
	const float fTankDistance = CBotGlobals::entityOrigin(pEntity).DistTo(m_vMVMCapturePoint);

	if (m_pNearestTankBoss.get() != nullptr)
	{
		if (!CBotGlobals::entityIsAlive(m_pNearestTankBoss))
		{
			m_pNearestTankBoss = nullptr;
			m_fNearestTankDistance = 0.0f;
		}
	}

	if (CBotGlobals::entityIsAlive(pEntity) && (m_pNearestTankBoss.get() == nullptr || (m_fNearestTankDistance == 0.0f || fTankDistance < m_fNearestTankDistance)))
	{
		m_fNearestTankDistance = fTankDistance;
		m_pNearestTankBoss = pEntity;
		m_vNearestTankLocation = CBotGlobals::entityOrigin(pEntity);
	}
}

CWaypoint *CTeamFortress2Mod :: getBestWaypointMVM (const CBot *pBot, const int iFlags)
{
	Vector vFlagLocation;

	const bool bFlagLocationValid = CTeamFortress2Mod::getFlagLocation(TF2_TEAM_BLUE,&vFlagLocation);		

	float fTankDistance = 0.0f; //never used? [APG]RoboCop[CL]

	edict_t *pTank;
	// check tank boss is valid
	if ((pTank = m_pNearestTankBoss.get()) != nullptr)
	{
		if (CBotGlobals::entityIsAlive(pTank) == false)
			m_pNearestTankBoss = nullptr;
	}
	
	if ( hasRoundStarted() && m_bMVMFlagStartValid && m_bMVMCapturePointValid && bFlagLocationValid )
	{
		if ( m_bMVMAlarmSounded )
			return CWaypoints::randomWaypointGoalNearestArea(iFlags,TF2_TEAM_RED,0,false,pBot,true,&m_vMVMCapturePoint,-1,true,m_iCapturePointWptID);
		if (m_pNearestTankBoss.get() != nullptr && m_fNearestTankDistance < vFlagLocation.DistTo(m_vMVMCapturePoint))
			return CWaypoints::randomWaypointGoalBetweenArea(iFlags, TF2_TEAM_RED, 0, false, pBot, true, &m_vNearestTankLocation, &m_vMVMCapturePoint, true, -1, m_iCapturePointWptID);
		if ( (m_vMVMFlagStart-vFlagLocation).Length()<1024.0f )
			return CWaypoints::randomWaypointGoalNearestArea(iFlags,TF2_TEAM_RED,0,false,pBot,true,&vFlagLocation,-1,true);
		return CWaypoints::randomWaypointGoalBetweenArea(iFlags,TF2_TEAM_RED,0,false,pBot,true,&vFlagLocation,&m_vMVMCapturePoint,true,-1,m_iCapturePointWptID);
	}
	if ( bFlagLocationValid )
	{
		return CWaypoints::randomWaypointGoalNearestArea(iFlags,TF2_TEAM_RED,0,false,pBot,true,&vFlagLocation,-1,true,m_iFlagPointWptID);
	}

	return nullptr;
}

// check voice commands
void CTeamFortress2Mod:: clientCommand ( edict_t *pEntity, const int argc, const char *pcmd, const char *arg1, const char *arg2 )
{
	if ( argc > 2 )
	{
		if ( std::strcmp(pcmd,"voicemenu") == 0 )
		{
			// somebody said a voice command
			u_VOICECMD vcmd;

			vcmd.voicecmd = 0;
			vcmd.b1.v1 = std::atoi(arg1);
			vcmd.b1.v2 = std::atoi(arg2);

			CBroadcastVoiceCommand voicecmd = CBroadcastVoiceCommand(pEntity,vcmd.voicecmd); 

			CBots::botFunction(&voicecmd);
		}
	}
	else
	{
		/*if ( std::strcmp(pcmd,"+use_action_slot_item") == 0 )
		{
			CClient *pClient = CClients::get(pEntity);

			if ( pClient != NULL )
			{
				pClient->monitorHighFive();
			}
		}*/
	}
}

// to fixed
void CTeamFortress2Mod :: teleporterBuilt ( edict_t *pOwner, const eEngiBuild type, edict_t *pBuilding )
{
	if ( type != ENGI_TELE ) //(type != ENGI_ENTRANCE) && (type != ENGI_EXIT) )
		return;

	const int iIndex = ENTINDEX(pOwner) - 1;

	if ( iIndex < 0 || iIndex > gpGlobals->maxClients )
		return;

	const int team = getTeam(pOwner);

	if ( CTeamFortress2Mod::isTeleporterEntrance(pBuilding,team) )
		m_Teleporters[iIndex].entrance = MyEHandle(pBuilding);
	else if ( CTeamFortress2Mod::isTeleporterExit(pBuilding,team) )
		m_Teleporters[iIndex].exit = MyEHandle(pBuilding);

	m_Teleporters[iIndex].sapper = MyEHandle();
	m_Teleporters[iIndex].m_fLastTeleported = 0.0f;
	m_Teleporters[iIndex].m_iWaypoint = CWaypointLocations::NearestWaypoint(CBotGlobals::entityOrigin(pBuilding),400.0f,-1,true);
}

// used for changing class if I'm doing badly in my team
int CTeamFortress2Mod ::getHighestScore ()
{
	short int highest = 0;

	for ( int i = 1; i <= gpGlobals->maxClients; i ++ )
	{
		edict_t* edict = INDEXENT(i);

		if ( edict && CBotGlobals::entityIsValid(edict) )
		{
			const short score = static_cast<short>(CClassInterface::getTF2Score(edict));

			highest = std::max(score, highest);
		}
	}

	return highest;
}

// check if there is another building near where I want to build
// check quickly by using the storage of sentryguns etc in the mod class
bool CTeamFortress2Mod::buildingNearby (const int iTeam, const Vector& vOrigin)
{
	for ( int i = 1; i <= gpGlobals->maxClients; i ++ )
	{
		edict_t* pPlayer = INDEXENT(i);

		// crash bug fix 
		if ( !pPlayer || pPlayer->IsFree() )
			continue;

		const short sentryIndex = static_cast<short>(i - 1);

		if ( CClassInterface::getTF2Class(pPlayer) != TF_CLASS_ENGINEER )
			continue;

		if ( CClassInterface::getTeam(pPlayer) != iTeam )
			continue;

		if (m_SentryGuns[sentryIndex].sentry.get())
		{
			if ((vOrigin - CBotGlobals::entityOrigin(m_SentryGuns[sentryIndex].sentry.get())).Length() < 100)
				return true;
		}

		if (m_Dispensers[sentryIndex].disp.get())
		{
			if ((vOrigin - CBotGlobals::entityOrigin(m_Dispensers[sentryIndex].disp.get())).Length() < 100)
				return true;
		}

		if (m_Teleporters[sentryIndex].entrance.get())
		{
			if ( (vOrigin-CBotGlobals::entityOrigin(m_Teleporters[sentryIndex].entrance.get())).Length() < 100 )
				return true;
		}

		if (m_Teleporters[sentryIndex].exit.get())
		{
			if ( (vOrigin-CBotGlobals::entityOrigin(m_Teleporters[sentryIndex].exit.get())).Length() < 100 )
				return true;
		}

	}

	return false;
}

//get the building
edict_t *CTeamFortress2Mod::getBuilding (const eEngiBuild object, const edict_t* pOwner)
{
	static int i;
	static tf_tele_t *tele; //tele not used [APG]RoboCop[CL]

	//index = ENTINDEX(pOwner)-1;
	// i is 1 to 32 ( 0 == worldspawn)
	i = ENTINDEX(pOwner) - 1;

	if (i < 0)
		return nullptr;

	//i = ENTINDEX(pOwner)+1;

	switch ( object )
	{
	case ENGI_DISP:
		return m_Dispensers[i].disp.get();
	case ENGI_SENTRY:
		return m_SentryGuns[i].sentry.get();
	case ENGI_TELE:
		if ( m_Teleporters[i].entrance.get() != nullptr)
			return m_Teleporters[i].entrance.get();
		return m_Teleporters[i].exit.get();
		//TODO: Added support for Sapper and Teleporter? [APG]RoboCop[CL]
	/*case ENGI_SAPPER:
		break;
	case ENGI_EXIT:
		break;
	case ENGI_ENTRANCE:
		break;*/
	default: ;
	}

	return nullptr;
}

// get the owner of 
edict_t *CTeamFortress2Mod ::getBuildingOwner (const eEngiBuild object, const short index)
{
	switch ( object )
	{
		static short int i;
		static tf_tele_t *tele;
	case ENGI_DISP:
		for ( i = 0; i < RCBOT_MAXPLAYERS; i ++ )
		{
			if ( m_Dispensers[i].disp.get() && ENTINDEX(m_Dispensers[i].disp.get())==index )
				return INDEXENT(i+1);
		}
		//m_SentryGuns[i].
		break;
	case ENGI_SENTRY:
		for ( i = 0; i < RCBOT_MAXPLAYERS; i ++ )
		{
			if ( m_SentryGuns[i].sentry.get() && ENTINDEX(m_SentryGuns[i].sentry.get())==index )
				return INDEXENT(i+1);
		}
		break;
	case ENGI_TELE:
		tele = m_Teleporters;

		for ( i = 0; i < RCBOT_MAXPLAYERS; i ++ )
		{
			if ( tele->entrance.get() && ENTINDEX(tele->entrance.get())==index )
				return INDEXENT(i+1);
			if ( tele->exit.get() && ENTINDEX(tele->exit.get())==index )
				return INDEXENT(i+1);

			tele++;
		}
		break;
		//TODO: Added support for Sapper and Teleporter? [APG]RoboCop[CL]
	/*case ENGI_SAPPER:
		break;
	case ENGI_EXIT:
		break;
	case ENGI_ENTRANCE:
		break;*/
	default: ;
	}

	return nullptr;
}

edict_t *CTeamFortress2Mod :: nearestDispenser (const Vector& vOrigin, const int team)
{
	edict_t *pNearest = nullptr;
	float fNearest = bot_use_disp_dist.GetFloat();

	for (tf_disp_t& m_Dispenser : m_Dispensers)
	{
		//m_Dispensers[i]
		edict_t* pDisp = m_Dispenser.disp.get();

		if ( pDisp )
		{
			if ( CTeamFortress2Mod::getTeam(pDisp) == team )
			{
				const float fDist = (CBotGlobals::entityOrigin(pDisp) - vOrigin).Length();

				if ( fDist < fNearest )
				{
					pNearest = pDisp;
					fNearest = fDist;
				}
			}
		}
	}

	return pNearest;
}

void CTeamFortress2Mod::sapperPlaced(const edict_t* pOwner, const eEngiBuild type, edict_t* pSapper)
{
	static int index;
	
	index = ENTINDEX(pOwner) - 1;

	if ( index >= 0 && index < RCBOT_MAXPLAYERS )
	{
		if ( type == ENGI_TELE )
			m_Teleporters[index].sapper = MyEHandle(pSapper);
		else if ( type == ENGI_DISP )
			m_Dispensers[index].sapper = MyEHandle(pSapper);
		else if ( type == ENGI_SENTRY )
			m_SentryGuns[index].sapper = MyEHandle(pSapper);
	}
}

void CTeamFortress2Mod:: addWaypointFlags (edict_t *pPlayer, edict_t *pEdict, int *iFlags, int *iArea, float *fMaxDistance)
{
	const string_t model = pEdict->GetIServerEntity()->GetModelName();

	if ( std::strncmp(pEdict->GetClassName(),"item_health",11) == 0 )
		*iFlags |= CWaypointTypes::W_FL_HEALTH;
	else if ( std::strncmp(pEdict->GetClassName(),"item_ammo",9) == 0 )
		*iFlags |= CWaypointTypes::W_FL_AMMO;
	else if ( std::strcmp(pEdict->GetClassName(),"prop_dynamic") == 0 )
	{
		if ( std::strcmp(model.ToCStr(),"models/props_gameplay/resupply_locker.mdl") == 0 )
		{
			*iFlags |= CWaypointTypes::W_FL_RESUPPLY;

			if ( CTeamFortress2Mod::getTeam(pPlayer) == TF2_TEAM_BLUE )
				*iFlags |= CWaypointTypes::W_FL_NORED;
			else if ( CTeamFortress2Mod::getTeam(pPlayer) == TF2_TEAM_RED )
				*iFlags |= CWaypointTypes::W_FL_NOBLU;
		}
	}
	// do this in the event code
	/*else if ( std::strcmp(pEdict->GetClassName(),"item_teamflag") == 0 )
	{
		if ( !CTeamFortress2Mod::isFlagCarrier(pPlayer) )
			*iFlags |= CWaypointTypes::W_FL_FLAG;
	}*/
	else if ( std::strcmp(pEdict->GetClassName(),"team_control_point") == 0 )
	{
		*iFlags |= CWaypointTypes::W_FL_CAPPOINT;
		*iArea = m_ObjectiveResource.getControlPointArea(pEdict);
		*fMaxDistance = 100;
	}
}

void CTeamFortress2Mod::sapperDestroyed(edict_t *pOwner, const eEngiBuild type, const edict_t *pSapper)
{
	static short int index; 

	for ( index = 0; index < RCBOT_MAXPLAYERS; index ++ )
	{
		if ( type == ENGI_TELE )
		{
			if ( m_Teleporters[index].sapper.get_old() == pSapper )
				m_Teleporters[index].sapper = MyEHandle();
			
		}
		else if ( type == ENGI_DISP )
		{
			if ( m_Dispensers[index].sapper.get_old() == pSapper )
				m_Dispensers[index].sapper = MyEHandle();
		}
		else if ( type == ENGI_SENTRY )
		{
			if ( m_SentryGuns[index].sapper.get_old() == pSapper )
				m_SentryGuns[index].sapper = MyEHandle();
		}
	}
}

void CTeamFortress2Mod::updatePointMaster()
{
	if ( m_PointMasterResource.get() == nullptr)
	{
		edict_t *pMaster = CClassInterface::FindEntityByClassnameNearest(Vector(0,0,0),"team_control_point_master",65535);

		if ( pMaster )
		{
			extern IServerGameEnts *servergameents;
			extern IServerTools *servertools;
			
			// HACK: we use one of the known CBaseEntity-sized entities to compute the offset to the first subclass member for CTeamControlPointMaster / CTeamControlPointRound
			const size_t baseEntityOffset = servertools->GetEntityFactoryDictionary()->FindFactory("simple_physics_brush")->GetEntitySize();

			const uintptr_t pMasterMembers = reinterpret_cast<uintptr_t>(servergameents->EdictToBaseEntity(pMaster)) + baseEntityOffset;
			m_PointMaster = reinterpret_cast<CTeamControlPointMaster*>(pMasterMembers);
			m_PointMasterResource = pMaster;
			
			logger->Log(LogLevel::INFO, "Computed point master offset %d", baseEntityOffset);

			const int idx = m_PointMaster->m_iCurrentRoundIndex;
			const int size = m_PointMaster->m_ControlPointRounds.Count();

			if (idx >= 0 && size >= 0 && idx < 100 && size < 100) 
			{
				int infoCount = 0;

				for (int r = 0; r < m_PointMaster->m_ControlPointRounds.Count(); ++r) 
				{
					try
					{
						CBaseEntity *pent = m_PointMaster->m_ControlPointRounds[r];
						CTeamControlPointRound* pointRound = reinterpret_cast<CTeamControlPointRound*>(reinterpret_cast<uintptr_t>(pent) + baseEntityOffset);

						logger->Log(LogLevel::DEBUG, "Control Points for Round %d", r);

						for (int i = 0; i < pointRound->m_ControlPoints.Count(); ++i)
						{
							const CBaseHandle* handle = &pointRound->m_ControlPoints.Element(i);

							if (handle->IsValid())
							{
								const edict_t* edict = INDEXENT(handle->GetEntryIndex());

								if (!edict->IsFree())
								{
									infoCount++;
									logger->Log(LogLevel::DEBUG, "%d, %d, %d, %s", r, i, handle->GetSerialNumber(), edict->GetClassName());
								}
							}
						}
					}
					catch (...)
					{
						//error
						break;
					}
				}

				//if (infoCount == 0) 
				//{
				//	logger->Log(LogLevel::WARN, "If you are playing cp_* maps, and you get this message, something might be wrong with your mstr_offset!");
				//}
			}
			//else 
			//{
			//	logger->Log(LogLevel::WARN, "If you are playing cp_* maps, and you get this message, something might be wrong with your mstr_offset!");
			//}
		}
	}

	if ( m_PointMaster != nullptr)
	{
		m_pCurrentRound =  m_PointMaster->getCurrentRound();
	}
}

edict_t *CTeamFortress2Mod :: getPayloadBomb (const int team)
{
	if ( team == TF2_TEAM_BLUE )
		return m_pPayLoadBombBlue;
	if ( team == TF2_TEAM_RED )
		return m_pPayLoadBombRed;

	return nullptr;
}

void CTeamFortress2Mod :: roundReset ()
{
	if (CTeamFortress2Mod::isMapType(TF_MAP_MVM) && g_pCVar->FindVar("tf_mvm_endless_force_on")->GetBool())
		return;

	if ( m_ObjectiveResource.m_ObjectiveResource.get() == nullptr)
	{
		m_ObjectiveResource.m_ObjectiveResource = CClassInterface::FindEntityByNetClass(gpGlobals->maxClients+1, "CTFObjectiveResource");
		
		//if ( m_ObjectiveResource.m_ObjectiveResource.get() != NULL )
		//{
		//	m_ObjectiveResource.setup();
		//}
	}

	//always reset on new round
	if ( m_ObjectiveResource.m_ObjectiveResource.get() != nullptr) //!m_ObjectiveResource.isInitialised() )
		m_ObjectiveResource.setup();

	m_Timer.reset();

	updatePointMaster();

	if ( m_ObjectiveResource.isInitialised() )
	{
		const int numpoints = m_ObjectiveResource.GetNumControlPoints();
		int i;

		for ( i = 0; i < numpoints; i ++ )
		{
			if ( m_ObjectiveResource.GetOwningTeam(i) != TF2_TEAM_RED )
				break;
		}

		// if all points are owned by RED at start up then its an attack defend map
		setAttackDefendMap(i==numpoints);

		m_ObjectiveResource.m_fUpdatePointTime = engine->Time() + rcbot_tf2_autoupdate_point_time.GetFloat();
		m_ObjectiveResource.m_fNextCheckMonitoredPoint = engine->Time() + 0.2f;

		m_ObjectiveResource.updatePoints();

	}

	m_pNearestTankBoss = nullptr;
	m_fNearestTankDistance = 0.0f;
	m_iWinningTeam = 0;
	m_bRoundOver = false;
	m_bHasRoundStarted = false;
	m_iFlagCarrierTeam = 0;
	m_pPayLoadBombBlue = nullptr;
	m_pPayLoadBombRed = nullptr;


	if ( isMapType(TF_MAP_MVM) )	
	{
		if ( !m_bMVMFlagStartValid )
		{
			CWaypoint *pGoal = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_FLAG);

			if ( pGoal )
			{
				m_vNearestTankLocation = m_vMVMFlagStart = m_vFlagLocationBlue = pGoal->getOrigin();
				m_bMVMFlagStartValid = m_bFlagLocationValidBlue = true;
				m_iFlagPointWptID = CWaypoints::getWaypointIndex(pGoal);
			}			
		}
		else 
		{
			// Reset Flag Location
			m_vFlagLocationBlue = m_vMVMFlagStart;
			m_bFlagLocationValidBlue = true;
		}

		if ( !m_bMVMCapturePointValid )
		{
			CWaypoint *pGoal = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_CAPPOINT);

			if ( pGoal )
			{
				m_vMVMCapturePoint = pGoal->getOrigin();
				m_bMVMCapturePointValid = true;
				m_fMVMCapturePointRadius = pGoal->getRadius();
				m_iCapturePointWptID = CWaypoints::getWaypointIndex(pGoal);
			}
		}
	}
}

void CTeamFortress2Mod::sentryBuilt(const edict_t* pOwner, const eEngiBuild type, edict_t* pBuilding)
{
	static int index;

	index = ENTINDEX(pOwner) - 1;

	if ( index>=0 && index<RCBOT_MAXPLAYERS )
	{
		if ( type == ENGI_SENTRY )
		{
			static tf_sentry_t *temp;
			temp = &m_SentryGuns[index];
			temp->sentry = MyEHandle(pBuilding);
			temp->sapper = MyEHandle();
			//m_SentryGuns[index].builder
		}
	}
}

bool CTeamFortress2Mod::isSentryGun (edict_t *pEdict)
{
	static short int i;
	static tf_sentry_t *temp;

	temp = m_SentryGuns;

	for ( i = 0; i < RCBOT_MAXPLAYERS; i ++ )
	{
		if ( temp->sentry == pEdict )
			return true;

		temp++;
	}

	return false;
}

void CTeamFortress2Mod::dispenserBuilt(const edict_t* pOwner, const eEngiBuild type, edict_t* pBuilding)
{
	static int index;

	index = ENTINDEX(pOwner)-1;

	if ( index>=0 && index<RCBOT_MAXPLAYERS )
	{
		if ( type == ENGI_DISP )
		{
			static tf_disp_t *temp;
			temp = &m_Dispensers[index];
			temp->disp = MyEHandle(pBuilding);
			temp->sapper = MyEHandle();
			//m_Dispensers[index].builder = userid;
		}
	}
}

void CTeamFortress2Mod::updateRedPayloadBomb ( edict_t *pent )
{
	const edict_t *cur = m_pPayLoadBombRed.get();

	if ( cur != pent )
		m_pPayLoadBombRed = pent;
}

void CTeamFortress2Mod::updateBluePayloadBomb ( edict_t *pent )
{
	const edict_t *cur = m_pPayLoadBombBlue.get();

	if ( cur != pent )
		m_pPayLoadBombBlue = pent;
}

bool CTeamFortress2Mod::isDefending ( edict_t *pPlayer )//, int iCapIndex = -1 )
{
	const int iIndex = (1 << (ENTINDEX(pPlayer) - 1));

	if ( m_ObjectiveResource.GetNumControlPoints() > 0 )
	{
		const int iTeam = CClassInterface::getTeam(pPlayer);

	//if ( iCapIndex == -1 )
	//{
		for ( short int i = 0; i < MAX_CONTROL_POINTS; i ++ )
		{
			if ( m_ObjectiveResource.isCPValid(i,iTeam,TF2_POINT_DEFEND) )
			{
				if ( (m_iCapDefenders[i] & iIndex) == iIndex )
					return true;
			}
		}
	//}
	//else
	//	return ((m_iCapDefenders[iCapIndex] & iIndex) == iIndex );
	}

	return false;
}

bool CTeamFortress2Mod::isCapping ( edict_t *pPlayer )//, int iCapIndex = -1 )
{
	const int index = (1 << (ENTINDEX(pPlayer) - 1));

	if ( m_ObjectiveResource.GetNumControlPoints() > 0 )
	{
		const int iTeam = CClassInterface::getTeam(pPlayer);

		for ( int i = 0; i < MAX_CAP_POINTS; i ++ )
		{				
			if ( m_ObjectiveResource.isCPValid(i,iTeam,TF2_POINT_ATTACK) )
			{
				if ( (m_Cappers[i] & index) == index )
					return true;
			}
		}
	}

	return false;
}
