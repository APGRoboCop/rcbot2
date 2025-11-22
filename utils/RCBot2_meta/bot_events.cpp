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

#pragma push_macro("clamp") //Fix for C++17 [APG]RoboCop[CL]
#undef clamp
#include <algorithm>
#pragma pop_macro("clamp")

#include "bot_plugin_meta.h"

#include "igameevents.h"
#include "bot.h"
#include "bot_cvars.h"
#include "bot_event.h"
#include "bot_strings.h"
#include "bot_globals.h"
#include "bot_fortress.h"
#include "bot_dod_bot.h"
#include "bot_weapons.h"
#include "bot_getprop.h"
#include "bot_dod_bot.h"
#include "bot_squads.h"
#include "bot_schedule.h"
#include "bot_waypoint_locations.h"

std::vector<std::unique_ptr<CBotEvent>> CBotEvents :: m_theEvents;
///////////////////////////////////////////////////////

class CBotSeeFriendlyKill : public IBotFunction
{
public:
	CBotSeeFriendlyKill ( edict_t *pTeammate, edict_t *pDied, const char *szKillerWeapon )
	{
		m_pTeammate = pTeammate;
		m_pWeapon = CWeapons::getWeaponByShortName(szKillerWeapon);
		m_pDied = pDied;
	}
	void execute ( CBot *pBot ) override
	{
		if ( CClassInterface::getTeam(m_pTeammate) != pBot->getTeam() )
			return;

		if ( pBot->getEdict() != m_pTeammate )
		{
			if ( pBot->isVisible(m_pTeammate) )
				pBot->seeFriendlyKill(m_pTeammate,m_pDied,m_pWeapon);
		}
	}
private:
	edict_t *m_pTeammate;
	edict_t *m_pDied;
	CWeapon *m_pWeapon;
};

class CBotWaveCompleteMVM : public IBotFunction
{
public:

	void execute ( CBot *pBot ) override
	{
		static_cast<CBotTF2*>(pBot)->MannVsMachineWaveComplete();
	}

};

class CBotSeeFriendlyHurtEnemy : public IBotFunction
{
public:
	CBotSeeFriendlyHurtEnemy ( edict_t *pTeammate, edict_t *pEnemy, const int iWeaponID )
	{
		m_pTeammate = pTeammate;
		m_pEnemy = pEnemy;
		m_pWeapon = CWeapons::getWeapon(iWeaponID);
	}

	void execute ( CBot *pBot ) override
	{
		if ( CClassInterface::getTeam(m_pTeammate) != pBot->getTeam() )
			return;

		if ( pBot->getEdict() != m_pTeammate )
		{
			if ( pBot->isVisible(m_pTeammate) && pBot->isVisible(m_pEnemy) )
				pBot->seeFriendlyHurtEnemy(m_pTeammate,m_pEnemy,m_pWeapon);
		}
	}
private:
	edict_t *m_pTeammate;
	edict_t *m_pEnemy;
	CWeapon *m_pWeapon;
};

class CBroadcastMVMAlarm : public IBotFunction
{
public:
	CBroadcastMVMAlarm(const float fRadius)
	{
		m_bValid = CTeamFortress2Mod::getMVMCapturePoint(&m_vLoc);
		m_fRadius = fRadius;
	}

	void execute ( CBot *pBot ) override
	{
		if ( m_bValid )
			static_cast<CBotTF2*>(pBot)->MannVsMachineAlarmTriggered(m_vLoc + Vector(randomFloat(-m_fRadius,m_fRadius),randomFloat(-m_fRadius,m_fRadius),0));
	}
private:
	Vector m_vLoc;
	float m_fRadius;
	bool m_bValid;
};

class CBotSeeEnemyHurtFriendly : public IBotFunction
{
public:
	CBotSeeEnemyHurtFriendly ( edict_t *pEnemy, edict_t *pTeammate, const int iWeaponID )
	{
		m_pTeammate = pTeammate;
		m_pEnemy = pEnemy;
		m_pWeapon = CWeapons::getWeapon(iWeaponID);
	}

	void execute ( CBot *pBot ) override
	{
		if ( CClassInterface::getTeam(m_pTeammate) != pBot->getTeam() )
			return;

		if ( pBot->getEdict() != m_pTeammate )
		{
			if ( pBot->isVisible(m_pTeammate) )
				pBot->seeEnemyHurtFriendly(m_pTeammate,m_pEnemy,m_pWeapon);
		}
	}
private:
	edict_t *m_pTeammate;
	edict_t *m_pEnemy;
	CWeapon *m_pWeapon;
};


class CBotSeeFriendlyDie : public IBotFunction
{
public:
	CBotSeeFriendlyDie ( edict_t *pDied, edict_t *pKiller, const char *szKillerWeapon )
	{
		m_pDied = pDied;
		m_pWeapon = CWeapons::getWeaponByShortName(szKillerWeapon);
		m_pKiller = pKiller;
	}
	void execute ( CBot *pBot ) override
	{
		if ( CClassInterface::getTeam(m_pDied) != pBot->getTeam() )
			return;

		if ( pBot->getEdict() != m_pDied )
		{
			if ( pBot->isVisible(m_pDied) )
				pBot->seeFriendlyDie(m_pDied,m_pKiller,m_pWeapon);
		}
	}
private:
	edict_t *m_pDied;
	edict_t *m_pKiller;
	CWeapon *m_pWeapon;
};

class CBotHearPlayerAttack : public IBotFunction
{
public:
	CBotHearPlayerAttack ( edict_t *pAttacker, const int iWeaponID )
	{
		m_pAttacker = pAttacker;
		m_iWeaponID = iWeaponID;
	}

	void execute ( CBot *pBot ) override
	{
		if ( !pBot->hasEnemy() && (pBot->wantToListen()||pBot->isListeningToPlayer(m_pAttacker)) && pBot->wantToListenToPlayerAttack(m_pAttacker,m_iWeaponID) )
		{
			const float fDistance = pBot->distanceFrom(m_pAttacker);

			// add some fuzz based on distance
			if ( randomFloat(0.0f,rcbot_listen_dist.GetFloat()) > fDistance )
				pBot->hearPlayerAttack(m_pAttacker,m_iWeaponID);
		}
	}
private:
	edict_t *m_pAttacker;
	int m_iWeaponID;
};

class CTF2BroadcastRoundWin : public IBotFunction
{
public:
	CTF2BroadcastRoundWin (const int iTeamWon, const bool bFullRound)
	{
		m_iTeam = iTeamWon;
		m_bFullRound = bFullRound;
	}

	void execute ( CBot *pBot ) override
	{
		static_cast<CBotTF2*>(pBot)->roundWon(m_iTeam,m_bFullRound);
	}
private:
	int m_iTeam;
	bool m_bFullRound;
};
////////////////////////////////////////////////


void CRoundStartEvent :: execute ( IBotEventInterface *pEvent )
{
	CBots::roundStart();
	#if SOURCE_ENGINE == SE_CSS
	CCounterStrikeSourceMod::onRoundStart();
	#endif
}

void CRoundFreezeEndEvent :: execute ( IBotEventInterface *pEvent )
{
	CCounterStrikeSourceMod::onFreezeTimeEnd();
}

void CPlayerHurtEvent :: execute ( IBotEventInterface *pEvent )
{
	CBot *pBot = CBots::getBotPointer(m_pActivator);
	const int iAttacker = pEvent->getInt("attacker",0);
	const int iWeaponId = pEvent->getInt("weaponid",-1);

	if ( iAttacker > 0 )
	{
		edict_t *pAttacker = CBotGlobals::playerByUserId(iAttacker);
	/*
		if ( m_pActivator )
		{
			int *flags;

			if ( (flags=CClassInterface::getPlayerFlagsPointer(m_pActivator)) != nullptr )
			{
				if ( *flags & FL_GODMODE )
				{
					pEvent->setInt("damage",0);
					pEvent->setInt("health",100);
				}
			}
		}
	*/
		if ( m_pActivator != pAttacker )
		{
			if ( pAttacker && (!pAttacker->m_pNetworkable || !pAttacker->m_NetworkSerialNumber) )
				pAttacker = nullptr;

			if ( pBot )
			{
				pBot->hurt(pAttacker,pEvent->getInt("health"));
			}

			pBot = CBots::getBotPointer(pAttacker);

			if ( pBot )
			{
				pBot->shot(m_pActivator);
			}

			if ( CBotGlobals::isPlayer(m_pActivator) && CBotGlobals::isPlayer(pAttacker) )
			{
				CBotSeeFriendlyHurtEnemy func1(pAttacker,m_pActivator,iWeaponId);
				CBotSeeEnemyHurtFriendly func2(pAttacker,m_pActivator,iWeaponId);

				CBots::botFunction(&func1);
				CBots::botFunction(&func2);
			}
		}
	}
	//CBots::botFunction()
}

void CPlayerDeathEvent :: execute ( IBotEventInterface *pEvent )
{
	CBot *pBot = CBots::getBotPointer(m_pActivator);
	const char *weapon = pEvent->getString("weapon", nullptr);
	CBotSquad *pPrevSquadLeadersSquad;
	const int iAttacker = pEvent->getInt("attacker",0);

		edict_t *pAttacker = iAttacker>0?CBotGlobals::playerByUserId(iAttacker): nullptr;
	
		if ( pAttacker && (CBotGlobals::entityOrigin(pAttacker)-CBotGlobals::entityOrigin(m_pActivator)).Length()>512.0f )
		{
			// killer
			CClient *pClient = CClients::get(pAttacker);

			assert(pClient != nullptr);
			if ( pClient && pClient->autoWaypointOn() )
			{
				const CWeapon *pWeapon = CWeapons::getWeaponByShortName(weapon);

				if ( pWeapon != nullptr)
				{
					if ( pWeapon->isScoped() )
					{
						pClient->autoEventWaypoint(CWaypointTypes::W_FL_SNIPER,100.0f);
					}
					else if ( pWeapon->isDeployable() )
					{
						// non OO hack here
						#if SOURCE_ENGINE == SE_DODS
							edict_t *pentWeapon = CWeapons::findWeapon(pAttacker,pWeapon->getWeaponName());

							if ( CClassInterface::isMachineGunDeployed(pentWeapon) )
							{
								bool bIsProne;
								float flStamina;

								CClassInterface::getPlayerInfoDOD(pAttacker,&bIsProne,&flStamina);

								if ( !bIsProne )
								{
									pClient->autoEventWaypoint(CWaypointTypes::W_FL_MACHINEGUN,100.0f);
								}
							}
						#endif
						//CClassInterface::isMachineGunDeployed(pWeapon->get)
						//pWeapon->isDeployed()
					}
				}
			}

			// victim
			pClient = CClients::get(m_pActivator);

			assert(pClient != nullptr);
			if ( CBotGlobals::isPlayer(pAttacker) && pClient && pClient->autoWaypointOn() )
			{
				const CWeapon *pWeapon = CWeapons::getWeaponByShortName(weapon);

				if ( pWeapon != nullptr)
				{
					if ( pWeapon->isScoped() )
					{
						pClient->autoEventWaypoint(CWaypointTypes::W_FL_SNIPER,200.0f,true,CClassInterface::getTeam(pAttacker),CBotGlobals::entityOrigin(pAttacker)+Vector(0,0,32.0f));
					}
				}
			}
		}

	if ( pBot )
		pBot->died(pAttacker,weapon);

	pBot = CBots::getBotPointer(pAttacker);

	if ( pBot )
	{
		pBot->killed(m_pActivator,const_cast<char*>(weapon));

		pBot->enemyDown(m_pActivator);
	}
	
	if ( m_pActivator && pAttacker ) // not worldspawn
	{
		CBotSeeFriendlyDie func1(m_pActivator,pAttacker,weapon);
		CBotSeeFriendlyKill func2(pAttacker,m_pActivator,weapon);

		CBots::botFunction(&func1);
		CBots::botFunction(&func2);
	}

	if ( (pPrevSquadLeadersSquad = CBotSquads::FindSquadByLeader (m_pActivator)) != nullptr)
	{
		CBotSquads::ChangeLeader(pPrevSquadLeadersSquad);
	}
}

void CBombPickupEvent :: execute ( IBotEventInterface *pEvent )
{
}

void CPlayerFootstepEvent :: execute ( IBotEventInterface *pEvent )
{
}

void CBombDroppedEvent :: execute ( IBotEventInterface *pEvent )
{
}

void CCSSBombPlantedEvent :: execute ( IBotEventInterface *pEvent )
{
	CCounterStrikeSourceMod::onBombPlanted();
}

void CWeaponFireEvent :: execute ( IBotEventInterface *pEvent )
{
}

void CPlayerSpawnEvent :: execute ( IBotEventInterface *pEvent )
{
	if ( CBot *pBot = CBots::getBotPointer(m_pActivator) )
		pBot->spawnInit();

	#if SOURCE_ENGINE == SE_TF2
		if ( pEvent->getInt("class") == TF_CLASS_MEDIC )
		{
			// find medigun
			CTeamFortress2Mod::findMediGun(m_pActivator);
		}
	#endif
}

void CBulletImpactEvent :: execute ( IBotEventInterface *pEvent )
{
	if ( CBot *pBot = CBots::getBotPointer(m_pActivator) )
	{
		pBot->shotmiss();
	}
}
/////////////////////////////////////////

/*
[RCBot] [DEBUG game_event] player_sapped_object
[RCBot] [DEBUG game_event] userid = 2
[RCBot] [DEBUG game_event] ownerid = 4
[RCBot] [DEBUG game_event] object = 2
[RCBot] [DEBUG game_event] sapperid = 400
*/
void CTF2ObjectSapped :: execute ( IBotEventInterface *pEvent )
{
	const int owner = pEvent->getInt("ownerid",-1);
	int building = pEvent->getInt("object",-1);
	const int sapperid = pEvent->getInt("sapperid",-1);

	if ( m_pActivator && owner>=0 && building>=0 && sapperid>=0 )
	{
		edict_t *pSpy = m_pActivator;
		const edict_t *pOwner = CBotGlobals::playerByUserId(owner);
		edict_t *pSapper = INDEXENT(sapperid);

		if ( CBotTF2 *pBot = static_cast<CBotTF2*>(CBots::getBotPointer(pOwner)) )
		{
			pBot->buildingSapped(static_cast<eEngiBuild>(building),pSapper,pSpy);
		}

		CTeamFortress2Mod::sapperPlaced(pOwner,static_cast<eEngiBuild>(building),pSapper);

		CBroadcastSpySap spysap = CBroadcastSpySap(pSpy);

		CBots::botFunction(&spysap);

	}
}

void CTF2RoundActive :: execute ( IBotEventInterface *pEvent )
{
	if ( CTeamFortress2Mod::isMapType(TF_MAP_MVM) )
		CTeamFortress2Mod::roundStarted();
	else 
		CTeamFortress2Mod::resetSetupTime();
}

void COverTimeBegin :: execute ( IBotEventInterface *pEvent )
{
	CBroadcastOvertime function;
	
	CBots::botFunction(&function);
}

void CBossSummonedEvent :: execute ( IBotEventInterface *pEvent )
{
	CTeamFortress2Mod::initBoss(true);
}

void CBossKilledEvent :: execute ( IBotEventInterface *pEvent )
{
	CTeamFortress2Mod::initBoss(false);
}

void CPlayerTeleported ::execute(IBotEventInterface *pEvent)
{
	const int builderid = pEvent->getInt("builderid",-1);

	if ( builderid >= 0 )
	{
		const edict_t *pPlayer = CBotGlobals::playerByUserId(builderid);

		if ( CBot *pBot = CBots::getBotPointer(pPlayer) )
		{
			static_cast<CBotTF2*>(pBot)->teleportedPlayer();
		}

		CTeamFortress2Mod::updateTeleportTime(pPlayer);

	}
}

void CPlayerHealed ::execute(IBotEventInterface *pEvent)
{
	const int patient = pEvent->getInt("patient",-1);
	const int healer = pEvent->getInt("healer",-1);
	const float amount = pEvent->getFloat("amount",0);

	if ( healer != -1 && patient != -1 && healer != patient )
	{
		m_pActivator = CBotGlobals::playerByUserId(patient);

		if ( m_pActivator )
		{
			if ( CBot *pBot = CBots::getBotPointer(m_pActivator) )
			{
				CBotTF2 *pBotTF2 = static_cast<CBotTF2*>(pBot);

				assert(pBotTF2 != nullptr);
				if ( pBotTF2 && randomInt(0,1) )
					pBotTF2->addVoiceCommand(TF_VC_THANKS);
			}
		}

		CBot *pBot = CBots::getBotPointer(CBotGlobals::playerByUserId(healer));

		if ( pBot && pBot->isTF2() )
		{
			static_cast<CBotTF2*>(pBot)->healedPlayer(m_pActivator,amount);
		}
	}
}

/*
[RCBot] [DEBUG game_event] object_destroyed
[RCBot] [DEBUG game_event] userid = 2
[RCBot] [DEBUG game_event] attacker = 4
[RCBot] [DEBUG game_event] weapon = wrench
[RCBot] [DEBUG game_event] weapon_logclassname = wrench
[RCBot] [DEBUG game_event] weaponid = 10
[RCBot] [DEBUG game_event] priority = 6
[RCBot] [DEBUG game_event] objecttype = 3
[RCBot] [DEBUG game_event] index = 436
[RCBot] [DEBUG game_event] was_building = 0
*/
void CTF2ObjectDestroyed :: execute ( IBotEventInterface *pEvent )
{
	int type = pEvent->getInt("objecttype",-1);
	const int index = pEvent->getInt("index",-1);
	const int was_building = pEvent->getInt("was_building",-1);
	const int iAttacker = pEvent->getInt("attacker",-1);

	if ( iAttacker != -1 )
	{
		edict_t *pAttacker = CBotGlobals::playerByUserId(iAttacker);

		if ( pAttacker && m_pActivator && type>=0 && index>=0 && was_building>=0 )
		{
			//if ( !was_building )
			//{ // could be a sapper
			if ( static_cast<eEngiBuild>(type) == ENGI_SAPPER )
			{
				edict_t *pOwner = pAttacker;
				edict_t *pSapper = INDEXENT(index);

				if ( const CBotTF2 *pBot = static_cast<CBotTF2*>(CBots::getBotPointer(pOwner)) )
					pBot->sapperDestroyed(pSapper);

				CTeamFortress2Mod::sapperDestroyed(pOwner,static_cast<eEngiBuild>(type),pSapper);
			}
			else
			{
				if ( CBotTF2 *pBot = static_cast<CBotTF2*>(CBots::getBotPointer(m_pActivator)) )
				{
					edict_t *pBuilding = INDEXENT(index);

					pBot->buildingDestroyed(type,pAttacker,pBuilding);
				}
			}
			//}
		}
	}
}

void CPostInventoryApplicationTF2 :: execute ( IBotEventInterface *pEvent )
{
	const int iUserID = pEvent->getInt( "userid" );

	const edict_t *pEdict = CBotGlobals::playerByUserId(iUserID);

	if ( CBot *pBot = CBots::getBotPointer(pEdict) )
	{
		pBot->onInventoryApplication();
	}
}
/*
player_upgradedobject
Name: 	player_upgradedobject
Structure: 	
short 	userid 	
byte 	object 	
short 	index 	
bool 	isbuilder 	
*/
void CTF2UpgradeObjectEvent :: execute ( IBotEventInterface *pEvent )
{
	if ( bot_use_vc_commands.GetBool() && randomInt(0,1) )
	{
		const eEngiBuild object = static_cast<eEngiBuild>(pEvent->getInt("object", 0));
		const bool isbuilder = pEvent->getInt("isbuilder")>0;
		const int index = pEvent->getInt("index");
	
		if ( !isbuilder )
		{
			// see if builder is a bot
			const edict_t *pOwner = CTeamFortress2Mod::getBuildingOwner (object, index);
			CBotTF2 *pBot;

			if ( (pBot = static_cast<CBotTF2*>(CBots::getBotPointer(pOwner))) != nullptr)
			{
				pBot->addVoiceCommand(TF_VC_THANKS);
			}
		}
	}
}

void CTF2RoundWinEvent :: execute (IBotEventInterface *pEvent )
{
	const int iWinningTeam = pEvent->getInt("team");

	CTF2BroadcastRoundWin fn(iWinningTeam, pEvent->getInt("full_round") == 1);
	CBots::botFunction(&fn);
	
	CTeamFortress2Mod::roundWon(iWinningTeam);
}

void CTF2SetupFinished ::execute(IBotEventInterface *pEvent )
{
	CTeamFortress2Mod::roundStarted();
}

void CTF2BuiltObjectEvent :: execute ( IBotEventInterface *pEvent )
{
	const eEngiBuild type = static_cast<eEngiBuild>(pEvent->getInt("object"));
	const int index = pEvent->getInt("index");
	edict_t *pBuilding = INDEXENT(index);
	CBot *pBot = CBots::getBotPointer(m_pActivator);

	CClient *pClient = CClients::get(m_pActivator);

	if ( type == ENGI_TELE )
	{
		CTeamFortress2Mod::teleporterBuilt(m_pActivator,type,pBuilding);

		assert(pClient != nullptr);
		if ( pClient && pClient->autoWaypointOn() )
		{
			if ( CTeamFortress2Mod::isTeleporterEntrance(pBuilding,CTeamFortress2Mod::getTeam(m_pActivator)) )
				pClient->autoEventWaypoint(CWaypointTypes::W_FL_TELE_ENTRANCE,400.0f);
			else
				pClient->autoEventWaypoint(CWaypointTypes::W_FL_TELE_EXIT,400.0f);
		}
	}

	if ( type == ENGI_SENTRY )
	{
		CTeamFortress2Mod::sentryBuilt(m_pActivator,type,pBuilding);

		assert(pClient != nullptr);
		if ( pClient && pClient->autoWaypointOn() )
		{
			pClient->autoEventWaypoint(CWaypointTypes::W_FL_SENTRY,400.0f);
		}
	}

	if ( type == ENGI_DISP )
	{
		CTeamFortress2Mod::dispenserBuilt(m_pActivator,type,pBuilding);
	}

	if ( pBot && pBot->isTF() )
	{
		static_cast<CBotFortress*>(pBot)->engiBuildSuccess(static_cast<eEngiBuild>(pEvent->getInt("object")),pEvent->getInt("index"));
	}
}

void CTF2ChangeClass :: execute ( IBotEventInterface *pEvent )
{
	CBot *pBot = CBots::getBotPointer(m_pActivator);

	if ( pBot && pBot->isTF() )
	{

		int _class = pEvent->getInt("class");

		static_cast<CBotFortress*>(pBot)->setClass(static_cast<TF_Class>(_class));

	}
}

void CTF2MVMWaveCompleteEvent :: execute ( IBotEventInterface *pEvent )
{
	CBotWaveCompleteMVM func;

	  CTeamFortress2Mod::MVMAlarmReset();
	  CTeamFortress2Mod::roundReset();

	  CBots::botFunction(&func);
}

void CTF2MVMWaveFailedEvent :: execute ( IBotEventInterface *pEvent )
{
	  CTeamFortress2Mod::MVMAlarmReset();
	  CTeamFortress2Mod::roundReset();
}

void CTF2RoundStart :: execute ( IBotEventInterface *pEvent )
{
	// 04/07/09 : add full reset

	  CBroadcastRoundStart roundstart = CBroadcastRoundStart(pEvent->getInt("full_reset") == 1);
	  
	  if ( pEvent->getInt("full_reset") == 1 )
	  {
		//CPoints::resetPoints();
	  }

	  // MUST BE BEFORE RESET SETUP TIME
	  CTeamFortress2Mod::setPointOpenTime(30.0f);
	  
	  // MUST BE AFTER RESETPOINTS
	  CTeamFortress2Mod::roundReset();

	  CTeamFortress2Mod::resetSetupTime();

	  CBots::botFunction(&roundstart);

}
/*
teamplay_capture_broken
Name: 	teamplay_capture_broken
Structure: 	
byte 	cp 	
string 	cpname 	
float 	time_remaining 
*/
void CTF2PointStopCapture :: execute ( IBotEventInterface *pEvent )
{
	const int capindex = pEvent->getInt("cp",0);

	CTeamFortress2Mod::removeCappers(capindex);
	
}
/*
teamplay_capture_blocked

Note: When a player blocks the capture of a control point
Name: 	teamplay_capture_blocked
Structure: 	
byte 	cp 	index of the point that was blocked
string 	cpname 	name of the point
byte 	blocker 	index of the player that blocked the cap 
*/
void CTF2PointBlockedCapture :: execute ( IBotEventInterface *pEvent )
{
	const int capindex = pEvent->getInt("cp",0);

	CTeamFortress2Mod::removeCappers(capindex);
}
void CTF2PointUnlocked :: execute ( IBotEventInterface *pEvent )
{
	CTeamFortress2Mod::setPointOpenTime(0);
	//
}

void CTF2PointLocked :: execute ( IBotEventInterface *pEvent )
{
	//
}

void CTF2PointStartTouch :: execute ( IBotEventInterface *pEvent )
{
	const int capindex = pEvent->getInt("area",0);
	const int iplayerIndex = pEvent->getInt("player",-1);
//	const char *cpname = pEvent->getString("cpname");

	edict_t *pPlayer = INDEXENT(iplayerIndex);

	if ( capindex >= 0 && CTeamFortress2Mod::m_ObjectiveResource.GetNumControlPoints() > 0 && 
		CTeamFortress2Mod::m_ObjectiveResource.GetOwningTeam(capindex) == CClassInterface::getTeam(pPlayer) )
	{
		CTeamFortress2Mod::addCapDefender(pPlayer,capindex);
	}	
}

void CTF2PointEndTouch :: execute ( IBotEventInterface *pEvent ) 
{
	const int capindex = pEvent->getInt("area",0);
	const int iplayerIndex = pEvent->getInt("player",-1);
//	const char *cpname = pEvent->getString("cpname");

	edict_t *pPlayer = INDEXENT(iplayerIndex);

	if ( capindex >= 0 && CTeamFortress2Mod::m_ObjectiveResource.GetNumControlPoints() > 0 && 
		CTeamFortress2Mod::m_ObjectiveResource.GetOwningTeam(capindex) == CClassInterface::getTeam(pPlayer) )
	{
		CTeamFortress2Mod::removeCapDefender(pPlayer,capindex);
	}	
}

void CTF2PointStartCapture :: execute ( IBotEventInterface *pEvent )
{/*
 [RCBot] [DEBUG game_event] teamplay_point_startcapture
[RCBot] [DEBUG game_event] cp = 0
[RCBot] [DEBUG game_event] cpname = #Dustbowl_cap_1_A
[RCBot] [DEBUG game_event] team = 2
[RCBot] [DEBUG game_event] capteam = 3
[RCBot] [DEBUG game_event] captime = 64.134995
[RCBot] [DEBUG game_event] cappers = 
[RCBot] [DEBUG game_event] priority = 7
*/
	const int capteam = pEvent->getInt("capteam",0);
	const int capindex = pEvent->getInt("cp",0);
	//	const char *cpname = pEvent->getString("cpname");

	if ( const char *cappers = pEvent->getString("cappers", nullptr) )
	{
		std::size_t i = 0; // Use std::size_t for the index type [APG]RoboCop[CL]

		while ( cappers[i] != 0 )
		{
			CTeamFortress2Mod::addCapper(capindex,cappers[i]);
			i++;
		}
	}

	CTeamFortress2Mod::m_ObjectiveResource.updateCaptureTime(capindex);
	//CPoints::pointBeingCaptured(capteam,cpname,cappers[0]);

	CBotTF2FunctionEnemyAtIntel fn(capteam, CTeamFortress2Mod::m_ObjectiveResource.GetCPPosition(capindex), EVENT_CAPPOINT, nullptr, capindex);
	CBots::botFunction(&fn);
}

void CTF2MannVsMachineAlarm :: execute ( IBotEventInterface *pEvent )
{
	CBroadcastMVMAlarm alarm = CBroadcastMVMAlarm(CTeamFortress2Mod::getMVMCapturePointRadius());

	CTeamFortress2Mod::MVMAlarmSounded();

	// MUST BE AFTER POINTS HAVE BEEN UPDATED!
	CBots::botFunction(&alarm);
}

void CTF2PointCaptured :: execute ( IBotEventInterface *pEvent )
{
	CBroadcastCapturedPoint cap = CBroadcastCapturedPoint(pEvent->getInt("cp"),pEvent->getInt("team"),pEvent->getString("cpname"));
	
	//CTeamFortress2Mod::m_Resource.debugprint();
	CTeamFortress2Mod::updatePointMaster();

	// update points
	CTeamFortress2Mod::m_ObjectiveResource.m_fUpdatePointTime = 0.0f;
	CTeamFortress2Mod::m_ObjectiveResource.m_fNextCheckMonitoredPoint = engine->Time() + 0.2f;

    // MUST BE AFTER POINTS HAVE BEEN UPDATED!
    CBots::botFunction(&cap);

}

/* Flag has been picked up or dropped */
enum : std::uint8_t
{
	FLAG_PICKUP = 1,
	FLAG_CAPTURED = 2,
	FLAG_DEFEND = 3,
	FLAG_DROPPED = 4,
	FLAG_RETURN = 5
};

void CFlagEvent :: execute ( IBotEventInterface *pEvent )
{
	// dropped / picked up ID
	const int type = pEvent->getInt("eventtype");
	// player id
	const int player = pEvent->getInt("player");

	edict_t *pPlayer = nullptr;
	CBot *pBot = nullptr;
	
	// Crash fix
	if ( player )
	{
		pPlayer = INDEXENT(player);
		pBot = CBots::getBotPointer(pPlayer);
	}

	switch ( type )
	{
	case FLAG_PICKUP: // pickup
		if ( pBot && pBot->isTF() )
		{
			static_cast<CBotTF2*>(pBot)->pickedUpFlag();
		}

		if ( pPlayer )
		{
			const int iTeam = CTeamFortress2Mod::getTeam(pPlayer);

			if ( CTeamFortress2Mod::isFlagAtDefaultState() )
			{
				CClient* pClient = CClients::get(pPlayer);

				assert(pClient != nullptr);
				if ( pClient && pClient->autoWaypointOn() )
					pClient->autoEventWaypoint(CWaypointTypes::W_FL_FLAG,200.0f,false);
			}

			CTeamFortress2Mod::flagPickedUp(iTeam,pPlayer);

		}
		

		break;
	case FLAG_CAPTURED: // captured
		{
			if( pPlayer )
			{
				if ( IPlayerInfo* p = playerinfomanager->GetPlayerInfo(pPlayer) )
				{
					CBroadcastFlagCaptured captured = CBroadcastFlagCaptured(p->GetTeamIndex());
					CBots::botFunction(&captured);
				}
			}

			if ( pBot && pBot->isTF() )
			{
				static_cast<CBotTF2*>(pBot)->capturedFlag();	
				static_cast<CBotTF2*>(pBot)->droppedFlag();	
			}
		
			if ( pPlayer )
			{
				const int iTeam = CTeamFortress2Mod::getTeam(pPlayer);
				CTeamFortress2Mod::flagDropped(iTeam,Vector(0,0,0));

				CClient* pClient = CClients::get(pPlayer);

				assert(pClient != nullptr);
				if ( pClient && pClient->autoWaypointOn() )
					pClient->autoEventWaypoint(CWaypointTypes::W_FL_CAPPOINT,200.0f,false);
			}

			CTeamFortress2Mod::resetFlagStateToDefault();
			
		}
		break;
	case FLAG_DROPPED: // drop
		{
			IPlayerInfo *p = playerinfomanager->GetPlayerInfo(pPlayer);
			Vector vLoc;

			if ( p )
			{
				vLoc = CBotGlobals::entityOrigin(pPlayer);
				CBroadcastFlagDropped dropped = CBroadcastFlagDropped(p->GetTeamIndex(),vLoc);
				CBots::botFunction(&dropped);
			}

			if ( pBot && pBot->isTF() )
				static_cast<CBotTF2*>(pBot)->droppedFlag();

			
			if ( pPlayer )
				CTeamFortress2Mod::flagDropped(CTeamFortress2Mod::getTeam(pPlayer),vLoc);
		}
		break;
	case FLAG_RETURN:
		{
			if ( CTeamFortress2Mod::isMapType(TF_MAP_SD) )
			{
				CBroadcastFlagReturned returned = CBroadcastFlagReturned(CTeamFortress2Mod::getFlagCarrierTeam());
				CBots::botFunction(&returned);
			}
			CTeamFortress2Mod::resetFlagStateToDefault();

			CTeamFortress2Mod::flagReturned(0); // for special delivery
			//p->GetTeamIndex(),CBotGlobals::entityOrigin(pPlayer));
		}
		break;
	default:	
		break;
	}
}

void CFlagCaptured :: execute ( IBotEventInterface *pEvent )
{
}
/////////////////////////////////////////////////
void CDODPointCaptured :: execute ( IBotEventInterface *pEvent )
{
	const int cp = pEvent->getInt("cp");
	const char *szCappers = pEvent->getString("cappers", nullptr);

	// get a capper
	const char userid = szCappers[0];

	int team = 0;

	// find the team - should be a player index
	if ( userid >= 0 && userid <= gpGlobals->maxClients )
	{
		edict_t* pPlayer = INDEXENT(userid);
		team = CClassInterface::getTeam(pPlayer);

		CClient *pClient = CClients::get(pPlayer);

		assert(pClient != nullptr);
		if (pClient && pClient->autoWaypointOn())
		{
			pClient->autoEventWaypoint(CWaypointTypes::W_FL_CAPPOINT, 150.0f, false, 0, Vector(0, 0, 0), true);
		}
	}

	if ( team )
	{
		CBroadcastBombEvent func(DOD_POINT_CAPTURED,cp,team);

		CBots::botFunction(&func);
	}
}

void CDODBombExploded :: execute ( IBotEventInterface *pEvent )
{
	const int cp = pEvent->getInt("cp");
	const int team = CClassInterface::getTeam(m_pActivator);

	if ( m_pActivator )
	{
		CBroadcastBombEvent func(DOD_BOMB_EXPLODED,cp,team);

		CBots::botFunction(&func);
	}

	CDODMod::m_Flags.setBombPlanted(cp,false);
}

void CDODBombDefused :: execute ( IBotEventInterface *pEvent )
{
	const int cp = pEvent->getInt("cp");
	const int team = pEvent->getInt("team");

	CDODMod::m_Flags.setBombPlanted(cp,false);

	CBroadcastBombEvent func(DOD_BOMB_DEFUSE,cp,team);

	CBots::botFunction(&func);
}

void CDODBombPlanted :: execute ( IBotEventInterface *pEvent )
{
	const int cp = pEvent->getInt("cp");
	const int team = pEvent->getInt("team");

	CBroadcastBombEvent func(DOD_BOMB_PLANT,cp,team);

/*	if ( m_pActivator )
	{
		CBot *pBot;

		if ( (pBot = CBots::getBotPointer(m_pActivator)) != nullptr )
		{
			// hack
			((CDODBot*)pBot)->removeBomb();
		}
	}*/

	CBots::botFunction(&func);

	CDODMod::m_Flags.setBombPlanted(cp,true);

}

void CDODRoundStart :: execute ( IBotEventInterface *pEvent )
{
	CDODMod::roundStart();
}

void CDODRoundActive :: execute ( IBotEventInterface *pEvent )
{

}

void CDODRoundWin :: execute ( IBotEventInterface *pEvent )
{
	//CDODMod::m_Flags.reset();
}

void CDODRoundOver :: execute ( IBotEventInterface *pEvent )
{
	//CDODMod::m_Flags.reset();
}

void CDODChangeClass :: execute ( IBotEventInterface *pEvent )
{
	if ( m_pActivator )
	{
		if ( CBot *pBot = CBots::getBotPointer(m_pActivator) )
		{
			CDODBot *pDODBot = static_cast<CDODBot*>(pBot);

			pDODBot->selectedClass(pEvent->getInt("class"));
		}
	}
}

/*
[RCBot] [DEBUG GAME_EVENT] [BEGIN "dod_stats_weapon_attack"]
[RCBot] [DEBUG GAME_EVENT] 	attacker = 5
[RCBot] [DEBUG GAME_EVENT] 	weapon = 14
[RCBot] [DEBUG GAME_EVENT] [END "dod_stats_weapon_attack"]*/

void CDODFireWeaponEvent :: execute ( IBotEventInterface *pEvent )
{
	const int iAttacker = pEvent->getInt("attacker",-1);

	if ( iAttacker >= 0 )
	{
		edict_t *pAttacker = CBotGlobals::playerByUserId(iAttacker);
		const int iWeaponID = pEvent->getInt("weapon",-1);

		CBotHearPlayerAttack func(pAttacker,iWeaponID);
		CBots::botFunction(&func);
	}


}

///////////////////////////////////////////////////////

void CBotEvent :: setType (const char* szType)
{
	m_szType = CStrings::getString(szType);
}

bool CBotEvent :: forCurrentMod () const
{
	return m_iModId == MOD_ANY || CBotGlobals::isMod(m_iModId);
}
// should we execute this ??
inline bool CBotEvent :: isType ( const char *szType ) const
{
	return forCurrentMod() && FStrEq(m_szType,szType);
}

///////////////////////////////////////////////////////
void CBotEvents :: setupEvents ()
{
	addEvent(std::make_unique<CTF2MVMWaveCompleteEvent>());
	addEvent(std::make_unique<CTF2MVMWaveFailedEvent>());
	addEvent(std::make_unique<CRoundStartEvent>());
	addEvent(std::make_unique<CRoundFreezeEndEvent>());
	addEvent(std::make_unique<CPlayerHurtEvent>());
	addEvent(std::make_unique<CPlayerDeathEvent>());
	addEvent(std::make_unique<CBombPickupEvent>());
	addEvent(std::make_unique<CCSSBombPlantedEvent>());
	addEvent(std::make_unique<CPlayerFootstepEvent>());
	addEvent(std::make_unique<CBombDroppedEvent>());
	addEvent(std::make_unique<CWeaponFireEvent>());
	addEvent(std::make_unique<CBulletImpactEvent>());
	addEvent(std::make_unique<CFlagEvent>());
	addEvent(std::make_unique<CPlayerSpawnEvent>());
	////////////// tf2
	addEvent(std::make_unique<CTF2BuiltObjectEvent>());
	addEvent(std::make_unique<CTF2ChangeClass>());
	addEvent(std::make_unique<CTF2RoundStart>());
	addEvent(std::make_unique<CTF2PointCaptured>());
	addEvent(std::make_unique<CTF2PointStartCapture>());
	addEvent(std::make_unique<CTF2ObjectSapped>());
	addEvent(std::make_unique<CTF2ObjectDestroyed>());
	addEvent(std::make_unique<CTF2PointStopCapture>());
	addEvent(std::make_unique<CTF2PointBlockedCapture>());
	addEvent(std::make_unique<CTF2UpgradeObjectEvent>());
	addEvent(std::make_unique<CTF2SetupFinished>());
	addEvent(std::make_unique<COverTimeBegin>());
	addEvent(std::make_unique<CPlayerHealed>());
	addEvent(std::make_unique<CPlayerTeleported>());
	addEvent(std::make_unique<CDODChangeClass>());
	addEvent(std::make_unique<CDODBombPlanted>());
	addEvent(std::make_unique<CDODBombExploded>());
	addEvent(std::make_unique<CDODBombDefused>());
	addEvent(std::make_unique<CDODPointCaptured>());
	addEvent(std::make_unique<CDODFireWeaponEvent>());
	addEvent(std::make_unique<CTF2RoundWinEvent>());
	addEvent(std::make_unique<CTF2PointUnlocked>());
	addEvent(std::make_unique<CTF2PointLocked>());
	addEvent(std::make_unique<CTF2MannVsMachineAlarm>());
	addEvent(std::make_unique<CPostInventoryApplicationTF2>());
/*
pumpkin_lord_summoned
merasmus_summoned
eyeball_boss_summoned

pumpkin_lord_killed
merasmus_killed
merasmus_escaped
eyeball_boss_killed
eyeball_boss_escaped */

	addEvent(std::make_unique<CBossSummonedEvent>("pumpkin_lord_summoned"));
	addEvent(std::make_unique<CBossSummonedEvent>("merasmus_summoned"));
	addEvent(std::make_unique<CBossSummonedEvent>("eyeball_boss_summoned"));
	addEvent(std::make_unique<CBossKilledEvent>("pumpkin_lord_killed"));
	addEvent(std::make_unique<CBossKilledEvent>("merasmus_killed"));
	addEvent(std::make_unique<CBossKilledEvent>("merasmus_escaped"));
	addEvent(std::make_unique<CBossKilledEvent>("eyeball_boss_killed"));
	addEvent(std::make_unique<CBossKilledEvent>("eyeball_boss_escaped"));
	addEvent(std::make_unique<CTF2RoundActive>());
	addEvent(std::make_unique<CDODRoundStart>());
	addEvent(std::make_unique<CDODRoundActive>());
	addEvent(std::make_unique<CDODRoundWin>());
	addEvent(std::make_unique<CDODRoundOver>());
	addEvent(std::make_unique<CTF2PointStartTouch>());
	addEvent(std::make_unique<CTF2PointEndTouch>());
}

void CBotEvents :: addEvent ( std::unique_ptr<CBotEvent> pEvent )
{
	extern IGameEventManager2 *gameeventmanager; //redundant? [APG]RoboCop[CL]
	//extern CRCBotMetaPlugin g_RCBOTServerPlugin;
	extern RCBotPluginMeta g_RCBotPluginMeta; //redundant? [APG]RoboCop[CL]

	//if ( gameeventmanager )
	//	gameeventmanager->AddListener( g_RCBotPluginMeta.getEventListener(), pEvent->getName(), true );

	m_theEvents.emplace_back(std::move(pEvent));
}

void CBotEvents :: freeMemory ()
{
	// Smart pointers automatically delete their contents
	m_theEvents.clear();
}

void CBotEvents::executeEvent(void* pEvent, const eBotEventType iType)
{
	int iEventId = -1;

	std::unique_ptr<IBotEventInterface> pInterface;

	if (iType == TYPE_KEYVALUES)
		pInterface = std::make_unique<CGameEventInterface1>(static_cast<KeyValues*>(pEvent));

	else if (iType == TYPE_IGAMEEVENT)
		pInterface = std::make_unique<CGameEventInterface2>(static_cast<IGameEvent*>(pEvent));

	if (pInterface == nullptr)
		return;

	if (iType != TYPE_IGAMEEVENT)
		iEventId = pInterface->getInt("eventid");

	for (const auto& pFound : m_theEvents)
	{
		// if it has an pEvent id stored just check that
		//if ( ( iType != TYPE_IGAMEEVENT ) && pFound->hasEventId() )
		//	bFound = pFound->isEventId(iEventId);
		//else

		if ( const bool bFound = pFound->forCurrentMod() && pFound->isType(pInterface->getName()) )	
		{
			const int userid = pInterface->getInt("userid",-1);
			// set pEvent id for quick checking
			pFound->setEventId(iEventId);

			pFound->setActivator(userid >= 0 ? CBotGlobals::playerByUserId(userid) : nullptr);

			pFound->execute(pInterface.get());

			break;
		}
	}
}