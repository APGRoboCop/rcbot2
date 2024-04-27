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

#include "bot_client.h"
#include "bot_waypoint_locations.h"
#include "bot_accessclient.h"
#include "bot_commands.h"
#include "bot_globals.h"
#include "bot_waypoint.h"
#include "ndebugoverlay.h"
#include "bot_menu.h"
// autowaypoint
#include "bot_getprop.h"
#include "bot_waypoint_locations.h"
//#include "bot_hooks.h"
#include "in_buttons.h"
#include "bot_plugin_meta.h"

// setup static client array
CClient CClients::m_Clients[RCBOT_MAXPLAYERS];
CClient *CClients::m_pListenServerClient = nullptr;
bool CClients::m_bClientsDebugging = false;

extern IVDebugOverlay *debugoverlay;


void CToolTip::send(edict_t *pPlayer) const
{
	//CRCBotPlugin::HudTextMessage(pPlayer,m_pszMessage);

	RCBotPluginMeta::HudTextMessage(pPlayer, m_pszMessage);

	if ( m_pszSound )
		engine->ClientCommand(pPlayer,"play %s",m_pszSound);
}

void CClient :: init ()
{
	m_iWaypointShowFlags = 0;
	m_fMonitorHighFiveTime = 0.0f;
	m_fSpeed = 0.0f;
	m_pPlayer = nullptr;
	m_szSteamID = nullptr;
	m_bWaypointOn = false;
	m_iCurrentWaypoint = -1;
	m_iAccessLevel = 0;
	m_bAutoPaths = true;
	m_iPathFrom = -1;
	m_iPathTo = -1;
	m_bPathWaypointOn = false;
	m_iDebugLevels = 0;
	m_pPlayerInfo = nullptr;
	m_iWptArea = 0;
	m_iWaypointDrawType = 3;
	m_bShowMenu = false;
	m_fUpdatePos = 0.0f;
	m_pDebugBot = nullptr;
	m_fCopyWptRadius = 0.0f;
	m_iCopyWptFlags = 0;
	m_iCopyWptArea = 0;
	m_pMenu = nullptr;
	m_iMenuCommand = -1;
	m_fNextUpdateMenuTime = 0.0f;

	while ( !m_NextTooltip.empty() )
		m_NextTooltip.pop();

	m_fNextBotServerMessage = 0.0f;
	m_bSentWelcomeMessage = false;
	m_fSpeed = 0.0f;
	m_fUpdatePos = 0.0f;
}

bool CClient :: needToRenderMenu () const
{ 
	return m_fNextUpdateMenuTime < engine->Time(); 
}

void CClient :: updateRenderMenuTime () 
{ 
	m_fNextUpdateMenuTime = engine->Time() + rcbot_menu_update_time2.GetFloat(); 
}

void CClient :: setEdict ( edict_t *pPlayer )
{
	m_pPlayer = pPlayer;
	m_pPlayerInfo = playerinfomanager->GetPlayerInfo(pPlayer);
}
	
void CClient :: setupMenuCommands () const
{
	/*engine->ClientCommand(m_pPlayer,"alias \"rcbot_setup\" \"bind 0 menuselect0\"");
	engine->ClientCommand(m_pPlayer,"rcbot_setup");bind 2 \"menuselect 2\"");*/
	engine->ClientCommand(m_pPlayer,"bind 1 \"menuselect 1\"");
	engine->ClientCommand(m_pPlayer,"bind 2 \"menuselect 2\"");
	engine->ClientCommand(m_pPlayer,"bind 3 \"menuselect 3\"");
	engine->ClientCommand(m_pPlayer,"bind 4 \"menuselect 4\"");
	engine->ClientCommand(m_pPlayer,"bind 5 \"menuselect 5\"");
	engine->ClientCommand(m_pPlayer,"bind 6 \"menuselect 6\"");
	engine->ClientCommand(m_pPlayer,"bind 7 \"menuselect 7\"");
	engine->ClientCommand(m_pPlayer,"bind 8 \"menuselect 8\"");
	engine->ClientCommand(m_pPlayer,"bind 9 \"menuselect 9\"");
	engine->ClientCommand(m_pPlayer,"bind 0 \"menuselect 0\"");
}
	
void CClient :: resetMenuCommands () const
{
	/*engine->ClientCommand(m_pPlayer,"alias \"rcbot_reset\" \"bind 0 slot10\"");
	engine->ClientCommand(m_pPlayer,"rcbot_reset");bind 2 \"menuselect 2\"");*/
	engine->ClientCommand(m_pPlayer,"bind 1 \"slot1\"");
	engine->ClientCommand(m_pPlayer,"bind 2 \"slot2\"");
	engine->ClientCommand(m_pPlayer,"bind 3 \"slot3\"");
	engine->ClientCommand(m_pPlayer,"bind 4 \"slot4\"");
	engine->ClientCommand(m_pPlayer,"bind 5 \"slot5\"");
	engine->ClientCommand(m_pPlayer,"bind 6 \"slot6\"");
	engine->ClientCommand(m_pPlayer,"bind 7 \"slot7\"");
	engine->ClientCommand(m_pPlayer,"bind 8 \"slot8\"");
	engine->ClientCommand(m_pPlayer,"bind 9 \"slot9\"");
	engine->ClientCommand(m_pPlayer,"bind 0 \"slot10\"");
}

void CClient :: playSound ( const char *pszSound )
{
	if ( isWaypointOn() )
	{
		if ( bot_cmd_enable_wpt_sounds.GetBool() )
			std::sprintf(m_szSoundToPlay,"play \"%s\"",pszSound);
	}
}

void CClient :: autoEventWaypoint ( int iType, float fRadius, bool bAtOtherOrigin, int iTeam, const Vector& vOrigin, bool bIgnoreTeam, bool bAutoType )
{
	m_iAutoEventWaypoint = iType;
	m_fAutoEventWaypointRadius = fRadius;

	CBotMod *pMod = CBotGlobals::getCurrentMod();

	m_bAutoEventWaypointAutoType = bAutoType;

	if ( bAtOtherOrigin )
	{
		m_vAutoEventWaypointOrigin = vOrigin;
	}
	else
	{
		m_vAutoEventWaypointOrigin = getOrigin();
		iTeam = CClassInterface::getTeam(m_pPlayer);
	}

	if ( bIgnoreTeam )
		m_iAutoEventWaypointTeam = 0;
	else
	{
		pMod->getTeamOnlyWaypointFlags(iTeam,&m_iAutoEventWaypointTeamOn,&m_iAutoEventWaypointTeamOff);
		m_iAutoEventWaypointTeam = iTeam;
	}	
}

void CClient :: teleportTo (const Vector& vOrigin)
{
	m_bIsTeleporting = true;
	m_fTeleportTime = engine->Time()+0.1f;

	Vector *v_origin = CClassInterface::getOrigin(m_pPlayer);

	byte *pMoveType = CClassInterface::getMoveTypePointer(m_pPlayer);
	int *pPlayerFlags = CClassInterface::getPlayerFlagsPointer(m_pPlayer);

	*pMoveType &= ~15;
	*pMoveType |= MOVETYPE_FLYGRAVITY;

	*pPlayerFlags &= ~FL_ONGROUND;
	*pPlayerFlags |= FL_FLY;

	*v_origin = vOrigin;
}

class CBotFunc_HighFiveSearch : public IBotFunction
{
public:
	CBotFunc_HighFiveSearch ( edict_t *pPlayer, int iTeam )
	{
		m_pPlayer = pPlayer;
		m_iTeam = iTeam;
		m_pNearestBot = nullptr;
		m_fNearestDist = 0.0f;
	}

	void execute ( CBot *pBot ) override
	{
		if ( pBot->getEdict() != m_pPlayer && pBot->getTeam() == m_iTeam && pBot->isVisible(m_pPlayer) )
		{
			const float fDist = pBot->distanceFrom(m_pPlayer);

			if ( !m_pNearestBot || fDist < m_fNearestDist )
			{
				m_pNearestBot = pBot;
				m_fNearestDist = fDist;
			}
		}
	}

	CBot *getNearestBot () const
	{
		return m_pNearestBot;
	}

private:
	edict_t *m_pPlayer;
	int m_iTeam;
	CBot *m_pNearestBot;
	float m_fNearestDist;
};

// called each frame
void CClient :: think ()
{
	//if ( m_pPlayer  )
	//	HookGiveNamedItem(m_pPlayer);

	/*if ( m_pPlayer.get() == NULL )
	{
		clientDisconnected();
		return;
	}
	*/

	//if ( m_fMonitorHighFiveTime > engine->Time() )
	//{

	if ( m_pPlayer != nullptr && m_pPlayerInfo == nullptr)
	{
		m_pPlayerInfo = playerinfomanager->GetPlayerInfo(m_pPlayer);
	}

	#if SOURCE_ENGINE == SE_TF2
		if ( m_fMonitorHighFiveTime < engine->Time() && m_pPlayer != nullptr && m_pPlayerInfo != nullptr && m_pPlayerInfo->IsConnected() && 
			!m_pPlayerInfo->IsDead() && m_pPlayerInfo->IsPlayer() && !m_pPlayerInfo->IsObserver() && 
			CClassInterface::getTF2HighFiveReady(m_pPlayer) )
		{
			m_fMonitorHighFiveTime = engine->Time() + 0.25f;

			if ( CClassInterface::getHighFivePartner(m_pPlayer) == nullptr)
			{
				// wanting high five partner
				// search for bots nearby who can see this player
				CBotFunc_HighFiveSearch func(m_pPlayer, CClassInterface::getTeam(m_pPlayer));

				CBots::botFunction(&func);

				CBot *pBot = func.getNearestBot();

				if ( pBot != nullptr)
				{
					static_cast<CBotTF2*>(pBot)->highFivePlayer(m_pPlayer,CClassInterface::getTF2TauntYaw(m_pPlayer));
					m_fMonitorHighFiveTime = engine->Time() + 3.0f;
				}
			}
		}
	#endif

	if ( m_szSoundToPlay[0] != 0 )
	{
		if ( bot_cmd_enable_wpt_sounds.GetBool() )
			engine->ClientCommand(m_pPlayer,m_szSoundToPlay);

		m_szSoundToPlay[0] = 0;
	}

	if ( m_bIsTeleporting )
	{
		if ( m_fTeleportTime < engine->Time() )
		{
			m_bIsTeleporting = false;
			m_fTeleportTime = 0.0f;
			//reset movetypes
			byte *pMoveType = CClassInterface::getMoveTypePointer(m_pPlayer);
			int *pPlayerFlags = CClassInterface::getPlayerFlagsPointer(m_pPlayer);

			*pMoveType &= ~15;
			*pMoveType |= MOVETYPE_WALK;

			*pPlayerFlags &= ~FL_FLY;
			*pPlayerFlags |= FL_ONGROUND;
		}
	}

	if ( m_bShowMenu )
	{
		m_bShowMenu = false;
		engine->ClientCommand(m_pPlayer,"cancelselect");
	}

	if ( m_pMenu != nullptr)
	{
		if ( needToRenderMenu() )
			m_pMenu->render(this);
		//CBotMenuList::render(pClient);
	}

	if ( isWaypointOn() )
		CWaypoints::drawWaypoints(this);

	if ( m_fUpdatePos < engine->Time() )
	{
		m_vVelocity = getOrigin()-m_vLastPos;
		m_fSpeed = m_vVelocity.Length();
		m_vLastPos = getOrigin();

		if ( m_fUpdatePos > 0 && m_fSpeed > 0 )
		{
			if ( !m_bSentWelcomeMessage && rcbot_show_welcome_msg.GetBool() )
			{
				m_bSentWelcomeMessage = true;

				giveMessage(CStrings::getString(BOT_WELCOME_MESSAGE));

				giveMessage(CStrings::getString(CWaypoints::getWelcomeMessage()),5.0f);
			}
		}

		m_fUpdatePos = engine->Time() + 1.0f;
	}

	if ( isDebugging() )
	{
		IPlayerInfo *p = playerinfomanager->GetPlayerInfo(m_pPlayer);


		if ( isDebugOn(BOT_DEBUG_SPEED) )
		{
			CBotGlobals::botMessage(m_pPlayer,0,"speed = %0.0f",m_fSpeed);
		}

		if ( isDebugOn(BOT_DEBUG_USERCMD) )
		{

			if ( p )
			{
				CBotCmd cmd = p->GetLastUserCommand();

				CBotGlobals::botMessage(m_pPlayer,0,"Btns = %d, cmd_no = %d, impulse = %d, weapselect = %d, weapsub = %d",cmd.buttons,cmd.command_number,cmd.impulse,cmd.weaponselect,cmd.weaponsubtype);

			}
		}


		if ( m_pDebugBot!= nullptr && isDebugOn(BOT_DEBUG_HUD) )
		{
			if ( m_fNextPrintDebugInfo < engine->Time() )
			{
				char msg[1024];
				CBot *pBot = CBots::getBotPointer(m_pDebugBot);

				QAngle eyes = p->GetLastUserCommand().viewangles;
				Vector vForward;
				// in fov? Check angle to edict
				AngleVectors(eyes,&vForward);

				vForward = vForward / vForward.Length(); // normalize
				Vector vLeft = (vForward-p->GetAbsOrigin()).Cross(Vector(0,0,1));
				vLeft = vLeft/vLeft.Length();
				
				Vector vDisplay = p->GetAbsOrigin() + vForward*300.0f; 
				vDisplay = vDisplay - vLeft*300.0f;

				// get debug message
				pBot->debugBot(msg);

#ifndef __linux__
				int i = 0; 
				int n = 0;
				char line[256];
				int linenum = 0;
				int iIndex = ENTINDEX(m_pDebugBot);

				do
				{
					while ( msg[i]!=0 && msg[i]!='\n' ) 
						line[n++] = msg[i++];

					line[n]=0;
					debugoverlay->AddEntityTextOverlay(iIndex,linenum++,1.0f,255,255,255,255,line);
					n = 0;

					if ( msg[i] == 0 )
						break;
					i++;
				}while ( true ) ;
				//int ent_index, int line_offset, float duration, int r, int g, int b, int a, const char *format, ...
			//	debugoverlay->AddEntityTextOverlay();
#endif
				m_fNextPrintDebugInfo = engine->Time() + 1.0f;
			}
		}
			//this->cm_pDebugBot->getTaskDebug();
		//m_pDebugBot->canAvoid();
	}

	if ( m_fNextBotServerMessage < engine->Time() )
	{
		if ( !m_NextTooltip.empty() )
		{
			m_NextTooltip.front().send(m_pPlayer);
			m_NextTooltip.pop();

			m_fNextBotServerMessage = engine->Time() + 11.0f;
		}
		else
			m_fNextBotServerMessage = engine->Time() + 1.0f;
	}


	/***** Autowaypoint stuff below borrowed and converted from RCBot1 *****/
	
	if ( m_bAutoWaypoint )
	{
		if ( !m_pPlayerInfo )
			m_pPlayerInfo = playerinfomanager->GetPlayerInfo(m_pPlayer);

		if ( !m_bSetUpAutoWaypoint || !m_pPlayerInfo || m_pPlayerInfo->IsDead() )
		{
			int i;
			int start = 0;

			if ( !m_pPlayerInfo->IsDead() )
				start = 1; // grab one location


			m_fLastAutoWaypointCheckTime = engine->Time() + 0.5f;

			if ( !m_pPlayerInfo->IsDead() )
				m_vLastAutoWaypointCheckPos[0].SetVector(getOrigin());

			for ( i = start; i < MAX_STORED_AUTOWAYPOINT; i++ )
			{
				m_vLastAutoWaypointCheckPos[i].UnSetPoint();
			}

			m_vLastAutoWaypointPlacePos = getOrigin();
			m_bSetUpAutoWaypoint = true;
			m_fCanPlaceJump = 0.0f;
			m_iLastButtons = 0;

			m_iLastJumpWaypointIndex = -1;
			m_iLastLadderWaypointIndex = -1;
			m_iLastMoveType = 0;
			m_fCanPlaceLadder = 0.0f;
			m_iJoinLadderWaypointIndex = -1;
		}
		else
		{			
			int iMoveType = CClassInterface::getMoveType(m_pPlayer);
			int iPlayerFlags = CClassInterface::getPlayerFlags(m_pPlayer);
			CBotCmd cmd = m_pPlayerInfo->GetLastUserCommand();

			Vector vPlayerOrigin = getOrigin();

			// ****************************************************
			// Jump waypoint
			//
			// wait until checkJump is not -1 (meaning that bot is in the air)
			// set checkJump to time+0.5 after landing 
			// 
			// ****************************************************

			if ( m_iAutoEventWaypoint != 0 )
			{
				int iWpt = CWaypointLocations::NearestWaypoint(m_vAutoEventWaypointOrigin,m_fAutoEventWaypointRadius,-1,false,false,false, nullptr,false,m_iAutoEventWaypointTeam,false,false,Vector(0,0,0),m_iAutoEventWaypoint);

				CWaypoint *pWpt = CWaypoints::getWaypoint(iWpt);

				if ( !pWpt )
				{
					//updateCurrentWaypoint();

					//pWpt = CWaypoints::getWaypoint(currentWaypoint());
					
					//if ( !pWpt || pWpt->distanceFrom(m_vAutoEventWaypointOrigin) > 32.0f )
					//{

					if ( m_bAutoEventWaypointAutoType )
					{
						CWaypointType *pMainType = CWaypointTypes::getTypeByFlags(m_iAutoEventWaypoint);

						if ( pMainType )
							CWaypoints::addWaypoint(this,pMainType->getName(),"","","");
						else
							CWaypoints::addWaypoint(m_pPlayer, m_vAutoEventWaypointOrigin,
							                        m_iAutoEventWaypointTeam == 0
								                        ? m_iAutoEventWaypoint
								                        : (m_iAutoEventWaypoint | m_iAutoEventWaypointTeamOn) & ~
								                        m_iAutoEventWaypointTeamOff, true, cmd.viewangles.y, 0, 32.0f);
					}
					else
						CWaypoints::addWaypoint(m_pPlayer, m_vAutoEventWaypointOrigin,
						                        m_iAutoEventWaypointTeam == 0
							                        ? m_iAutoEventWaypoint
							                        : (m_iAutoEventWaypoint | m_iAutoEventWaypointTeamOn) & ~
							                        m_iAutoEventWaypointTeamOff, true, cmd.viewangles.y, 0, 32.0f);
					//}
					/*else
					{
						pWpt->addFlag(m_iAutoEventWaypoint);
					}*/
				}

				m_iAutoEventWaypoint = 0;
			}
			//g_pBotManager->GetBotController(m_pPlayer)->IsEFlagSet();

			if ( /*(pev->waterlevel < 3) &&*/ m_fCanPlaceJump < engine->Time() )
			{
				if ( m_fCanPlaceJump != -1 && m_iLastButtons & IN_JUMP && !(iPlayerFlags & FL_ONGROUND) )
				{
					int iNearestWpt = CWaypointLocations::NearestWaypoint(vPlayerOrigin, 80.0f, -1, true, false, false, nullptr);

					m_iLastJumpWaypointIndex = -1;
					
					if ( iNearestWpt == -1 )
					{
						m_iLastJumpWaypointIndex = CWaypoints::addWaypoint(m_pPlayer,vPlayerOrigin,CWaypointTypes::W_FL_JUMP,true);
					}
					else
						m_iLastJumpWaypointIndex = iNearestWpt; // can still update a current waypoint for land position
					
					m_vLastAutoWaypointPlacePos = vPlayerOrigin;

					m_fCanPlaceJump = -1;
				}
				// ****************************************************
				// Join jump waypoint to the landed waypoint
				// ****************************************************
				else if ( m_fCanPlaceJump == -1 && iPlayerFlags & FL_ONGROUND )
				{
					if ( m_iLastJumpWaypointIndex != -1 )
					{
						int iNearestWpt = CWaypointLocations::NearestWaypoint(vPlayerOrigin, 80.0f, -1, true, false, false, nullptr);
						
						if ( iNearestWpt == -1 )
						{
							int iNewWpt = CWaypoints::addWaypoint(m_pPlayer,vPlayerOrigin,0,true);

							if ( iNewWpt != -1 )
							{
								Vector v_floor;
								CWaypoint *pWpt = CWaypoints::getWaypoint(iNewWpt);
								CWaypoint *pJumpWpt = CWaypoints::getWaypoint(m_iLastJumpWaypointIndex);

								pJumpWpt->addPathTo(iNewWpt);
				
								pJumpWpt->addFlag(CWaypointTypes::W_FL_JUMP);
								
								trace_t *tr;
								
								Vector v_src = pWpt->getOrigin();

								CBotGlobals::quickTraceline(m_pPlayer,v_src,v_src-Vector(0,0,144));
								
								tr = CBotGlobals::getTraceResult();

								v_floor = tr->endpos;
								float len = v_src.z-tr->endpos.z;
								
								CBotGlobals::quickTraceline(m_pPlayer,v_src,v_src+Vector(0,0,144));
								
								len += tr->endpos.z-v_src.z;
								
								if ( len > 72 )
								{
									pWpt->removeFlag(CWaypointTypes::W_FL_CROUCH);
									pWpt->move(v_floor+Vector(0,0,36));
								}
								else if ( len > 32 )
								{
									pWpt->addFlag(CWaypointTypes::W_FL_CROUCH);
									pWpt->move(v_floor+Vector(0,0,12));
								}
							}
						}
						else if ( iNearestWpt != m_iLastJumpWaypointIndex )
						{
							CWaypoint *pJumpWpt = CWaypoints::getWaypoint(m_iLastJumpWaypointIndex);

							pJumpWpt->addPathTo(iNearestWpt);
							pJumpWpt->addFlag(CWaypointTypes::W_FL_JUMP);
						}
					}

					m_iLastJumpWaypointIndex = -1;

					// wait a sec after player lands before checking jump again
					m_fCanPlaceJump = engine->Time() + 0.5f;
				}				
			}

			bool bCheckDistance = iMoveType != MOVETYPE_FLY && m_fCanPlaceLadder == 0; // always check distance unless ladder climbing

			// ****************************************************
			// Ladder waypoint
			// make the frist waypoint (e.g. bottom waypoint)
			// ****************************************************
			if ( iMoveType == MOVETYPE_FLY && m_iLastMoveType != MOVETYPE_FLY )
			{
				// went ON to a ladder

				int iNearestWpt = CWaypointLocations::NearestWaypoint(vPlayerOrigin, 80.0f, -1, true, false, false, nullptr);

				m_iLastLadderWaypointIndex = -1;
					
				if ( iNearestWpt == -1 )
					m_iLastLadderWaypointIndex = CWaypoints::addWaypoint(m_pPlayer,vPlayerOrigin,CWaypointTypes::W_FL_LADDER,true);
				else
				{
					m_iLastLadderWaypointIndex = iNearestWpt; // can still update a current waypoint for land position

					CWaypoint *pLadderWpt = CWaypoints::getWaypoint(m_iLastLadderWaypointIndex);

					pLadderWpt->addFlag(CWaypointTypes::W_FL_LADDER); // update flags
				}
					
				m_vLastAutoWaypointPlacePos = vPlayerOrigin;

				bCheckDistance = false;

				m_fCanPlaceLadder = 0.0f;

				// need to unset every check point when going on ladder first time
				for ( int i = 0; i < MAX_STORED_AUTOWAYPOINT; i ++ )
				{
						m_vLastAutoWaypointCheckPos[i].UnSetPoint();					
				}
			}
			else if ( iMoveType != MOVETYPE_FLY && m_iLastMoveType == MOVETYPE_FLY )
			{
				// went OFF a ladder
				m_fCanPlaceLadder = engine->Time() + 0.2f;
			}
			
			// ****************************************************
			// If we have walked off a ladder for a small amount of time
			// Make the top/bottom ladder waypoint
			// ****************************************************
			if ( m_fCanPlaceLadder && m_fCanPlaceLadder < engine->Time() )
			{
				if ( m_iLastLadderWaypointIndex != -1 )
					// place a ladder waypoint before jumping off
				{
					int iNearestWpt = CWaypointLocations::NearestWaypoint(vPlayerOrigin, 80.0f, -1, true, false, false, nullptr);
					
					if ( iNearestWpt == -1 )
					{
						int iNewWpt = CWaypoints::addWaypoint(m_pPlayer,vPlayerOrigin,CWaypointTypes::W_FL_LADDER,true);
						
						if ( iNewWpt != -1 )
						{
							CWaypoint *pLadderWpt = CWaypoints::getWaypoint(m_iLastLadderWaypointIndex);

							m_iJoinLadderWaypointIndex = iNewWpt;

							pLadderWpt->addPathTo(iNewWpt);
						}
					}
					else if ( iNearestWpt != m_iLastLadderWaypointIndex )
					{
						CWaypoint *pLadderWpt = CWaypoints::getWaypoint(m_iJoinLadderWaypointIndex);

						m_iJoinLadderWaypointIndex = iNearestWpt;

						pLadderWpt->addPathTo(iNearestWpt);
					}				
				}
				
				m_iLastLadderWaypointIndex = -1;
				
				bCheckDistance = false;

				m_fCanPlaceLadder = 0.0f;
			}
			
			// ****************************************************
			// Join top ladder waypoint to a ground waypoint
			// ****************************************************
			if ( m_iJoinLadderWaypointIndex != -1 && iPlayerFlags & FL_ONGROUND && iMoveType == MOVETYPE_WALK )
			{
				int iNearestWpt = CWaypointLocations::NearestWaypoint(vPlayerOrigin, 40.0f, m_iJoinLadderWaypointIndex, true, false, false, nullptr);
				
				if ( iNearestWpt == -1 )
				{
					int iNewWpt = CWaypoints::addWaypoint(m_pPlayer,vPlayerOrigin,0,true);
					
					if ( iNewWpt != -1 )
					{
						CWaypoint *pLadderWpt = CWaypoints::getWaypoint(m_iJoinLadderWaypointIndex);

						pLadderWpt->addPathTo(iNewWpt);
					}
				}
				else if ( iNearestWpt != m_iJoinLadderWaypointIndex )
				{
					CWaypoint *pLadderWpt = CWaypoints::getWaypoint(m_iJoinLadderWaypointIndex);

					pLadderWpt->addPathTo(iNearestWpt);
				}

				m_iJoinLadderWaypointIndex = -1;
			}

			m_iLastButtons = cmd.buttons;
			m_iLastMoveType = iMoveType;

			if ( m_fLastAutoWaypointCheckTime < engine->Time() )
			{
				// ****************************************
				// Corner - Check
				// ****************************************
				//
				// place a "Check - point" at player origin
				//

				CAutoWaypointCheck *vCurVector;				
				Vector vCheckOrigin;

				Vector vPlacePosition;
				int iFlags = 0;
				bool bPlace = false;
				
				int i;
				int n;

				trace_t *tr;

				int numset = 0;
				int last = 0;

				for ( n = 0; n < MAX_STORED_AUTOWAYPOINT; n ++ )
				{
					if ( m_vLastAutoWaypointCheckPos[n].IsVectorSet() )
					{
						numset++;
					}
				}

				if ( numset == MAX_STORED_AUTOWAYPOINT )
				{
					// move check points down
					for ( n = 0; n < MAX_STORED_AUTOWAYPOINT-1; n ++ )
					{
						m_vLastAutoWaypointCheckPos[n] = m_vLastAutoWaypointCheckPos[n+1];						
					}
					
					last = MAX_STORED_AUTOWAYPOINT-1;
				}
				else
				{					
					last = numset;
				}

				iFlags = 0;

				// sort out flags for this waypoint depending on player
				if ((iPlayerFlags & FL_DUCKING) == FL_DUCKING)
				{
					iFlags |= CWaypointTypes::W_FL_CROUCH;  // crouching waypoint
				}
				
				if (iMoveType == MOVETYPE_LADDER)
					iFlags |= CWaypointTypes::W_FL_LADDER;  // waypoint on a ladder

				m_vLastAutoWaypointCheckPos[last].SetPoint(vPlayerOrigin,iFlags);
				
				if ( m_iLastJumpWaypointIndex==-1 && bCheckDistance && (vPlayerOrigin - m_vLastAutoWaypointPlacePos).Length() > 200 )
				{
					int iNearestWpt = CWaypointLocations::NearestWaypoint(vPlayerOrigin, rcbot_autowaypoint_dist.GetFloat(), -1, true, false, false, nullptr);
					
					if ( iNearestWpt == -1 )
						CWaypoints::addWaypoint(this,"","","","");
					
					// set regardless
					m_vLastAutoWaypointPlacePos = vPlayerOrigin;
				}

				// search for occluded check points from player
				for ( i = 0; i < MAX_STORED_AUTOWAYPOINT; i++ )
				{
					vCurVector = &m_vLastAutoWaypointCheckPos[i];

					if ( !vCurVector->IsVectorSet() )
						continue;

					vCheckOrigin = vCurVector->GetVector();

					CBotGlobals::quickTraceline(m_pPlayer,vPlayerOrigin,vCheckOrigin);
					tr = CBotGlobals::getTraceResult();

#ifndef __linux__
					if ( m_bDebugAutoWaypoint && !engine->IsDedicatedServer() )
					{
						debugoverlay->AddLineOverlay(vCheckOrigin+Vector(0,0,16),vCheckOrigin-Vector(0,0,16),255,255,255,false,2);
						debugoverlay->AddLineOverlay(vPlayerOrigin,vCheckOrigin,255,255,255,false,2);
					}
#endif					
					if ( tr->fraction < 1.0f )
					{
						if ( tr->m_pEnt )
						{
							Vector vel;
							extern IServerGameEnts *servergameents;
							edict_t *pEdict = servergameents->BaseEntityToEdict(tr->m_pEnt);

							if ( CClassInterface::getVelocity(pEdict,&vel) )
							{
								// on a lift/train moving "fast"
								if ( vel.Length() > 20.0f )
									continue;
							}
						}
						// find next which is visible
						for ( n = i+1; n < MAX_STORED_AUTOWAYPOINT; n++ )
						{
							vCurVector = &m_vLastAutoWaypointCheckPos[n];
							
							if ( !vCurVector->IsVectorSet() )
								continue;
							
							vCheckOrigin = vCurVector->GetVector();

							CBotGlobals::quickTraceline(m_pPlayer,vPlayerOrigin,vCheckOrigin);
			
#ifndef __linux__
							if ( m_bDebugAutoWaypoint )
								debugoverlay->AddLineOverlay(vPlayerOrigin,vCheckOrigin,255,255,255,false,2);
#endif							
							if ( tr->fraction >= 1.0f )
							{
								int iNearestWpt = CWaypointLocations::NearestWaypoint(vCheckOrigin, 100.0f, -1, true, false, false, nullptr);
								
								if ( iNearestWpt == -1 )
								{
									bPlace = true;		
									vPlacePosition = vCheckOrigin;
									iFlags = vCurVector->getFlags();
									
									break;
								}
							}
						}
						
					}
				}

				if ( bPlace )
				{
					int inewwpt = CWaypoints::addWaypoint(m_pPlayer,vPlacePosition,iFlags,true);
					CWaypoint *pWpt = CWaypoints::getWaypoint(inewwpt);
					Vector v_floor; //Unused? [APG]RoboCop[CL]

					m_vLastAutoWaypointPlacePos = vPlacePosition;
					bool bCanStand;

					trace_t *tr;
					
					Vector v_src = vPlacePosition;

					CBotGlobals::quickTraceline(m_pPlayer,v_src,v_src-Vector(0,0,144));
					
					tr = CBotGlobals::getTraceResult();

					v_floor = tr->endpos;
					float len = v_src.z-tr->endpos.z;
					
					CBotGlobals::quickTraceline(m_pPlayer,v_src,v_src+Vector(0,0,144));
					
					len += tr->endpos.z-v_src.z;
					
					bCanStand = len > 72;

					if ( m_iLastJumpWaypointIndex != -1 && bCanStand )
					{
						pWpt->removeFlag(CWaypointTypes::W_FL_CROUCH);
						//waypoints[inewwpt].origin = v_floor+Vector(0,0,36);
					}
					//clear from i

					int pos = n;
					//int n = 0;

					for ( n = 0; pos < MAX_STORED_AUTOWAYPOINT; n ++ )
					{
						m_vLastAutoWaypointCheckPos[n] = m_vLastAutoWaypointCheckPos[pos];

						pos++;
					}

					for ( n = 0; n < MAX_STORED_AUTOWAYPOINT; n ++ )
					{
						m_vLastAutoWaypointCheckPos[n].UnSetPoint();					
					}
				}

				m_fLastAutoWaypointCheckTime = engine->Time() + 0.5f;
			}
		}
	}
}

void CClient::giveMessage(const char *msg,float fTime)
{
	if ( rcbot_tooltips.GetBool() )
	{
		m_NextTooltip.emplace(msg, nullptr);
		m_fNextBotServerMessage = engine->Time() + fTime;
	}
}

void CClients::giveMessage(const char* msg, float fTime, edict_t* pPlayer)
{
	CClient *pClient;

	if ( pPlayer != nullptr)
	{
		pClient = get(pPlayer);
		
		if ( pClient )
			pClient->giveMessage(msg,fTime);
	}
	else
	{
		for ( int i = 0; i < 32; i ++ )
		{
			pClient = get(i);

			if ( pClient )
			{
				pClient->giveMessage(msg,fTime);
			}
		}
	}
}

const char *CClient :: getName () const
{
	IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( m_pPlayer );

	if ( playerinfo )
		return playerinfo->GetName();

	return nullptr;
}

void CClient ::setTeleportVector()
{
	m_vTeleportVector = getOrigin();
	m_bTeleportVectorValid = true;
}

void CClient :: clientActive ()
{
	// get steam id
	IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( m_pPlayer );

	m_szSteamID = nullptr;

	if ( playerinfo )
	{
		// store steam id
		m_szSteamID = const_cast<char*>(playerinfo->GetNetworkIDString());
	
		// check my access levels
		CAccessClients::checkClientAccess(this);
	}
}
// this player joins with pPlayer edict
void CClient :: clientConnected ( edict_t *pPlayer )
{
	init();
	// set player edict
	setEdict(pPlayer);
}

void CClient :: updateCurrentWaypoint ()
{
	setWaypoint(CWaypointLocations::NearestWaypoint(getOrigin(),50.0f,-1,false,true,false, nullptr,false,0,false,false,Vector(0,0,0),m_iWaypointShowFlags));
}
// this player disconnects
void CClient :: clientDisconnected ()
{
	// is bot?
	CBot *pBot = CBots::getBotPointer(m_pPlayer);

	if ( pBot != nullptr)
	{
		if ( pBot->inUse() )
		{
			// free bots memory and other stuff
			pBot->freeAllMemory();
		}
	}

	if ( !engine->IsDedicatedServer() )
	{
		if ( CClients::isListenServerClient(this) )
		{
			CClients::setListenServerClient(nullptr);
		}
	}

	/*extern IServerGameEnts *servergameents;

	DWORD *baseentity = ( DWORD* )*( DWORD* )servergameents->EdictToBaseEntity(m_pPlayer);

	if ( baseentity == GiveNamedItemHookedClass )
		UnhookGiveNamedItem();
		*/
	init();
}

int CClient :: accessLevel () const
{
	return m_iAccessLevel;
}

bool CClient :: isUsed () const
{
	return m_pPlayer != nullptr;
}

Vector CClient :: getOrigin () const
{
	IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( m_pPlayer );

	if ( playerinfo )
	{
		return  playerinfo->GetAbsOrigin() + Vector(0,0,32);
	}

	return CBotGlobals::entityOrigin(m_pPlayer) + Vector(0,0,32);//m_pPlayer->GetCollideable()->GetCollisionOrigin();
}

void CClients :: clientActive ( edict_t *pPlayer )
{
	CClient *pClient = &m_Clients[slotOfEdict(pPlayer)];

	pClient->clientActive();
}

CClient *CClients :: clientConnected ( edict_t *pPlayer )
{
	CClient *pClient = &m_Clients[slotOfEdict(pPlayer)];

	pClient->clientConnected(pPlayer);

	return pClient;
}

void CClients :: init ( edict_t *pPlayer )
{
	m_Clients[slotOfEdict(pPlayer)].init();
}

void CClients :: clientDisconnected ( edict_t *pPlayer )
{
	CClient *pClient = &m_Clients[slotOfEdict(pPlayer)];

	pClient->clientDisconnected();
}

void CClients :: clientThink ()
{
	static CClient *pClient;

	m_bClientsDebugging = false;

	for ( int i = 0; i < RCBOT_MAXPLAYERS; i ++ )
	{
		pClient = &m_Clients[i];

		if ( !pClient->isUsed() )
			continue;
		if ( !m_bClientsDebugging && pClient->isDebugging() )
			m_bClientsDebugging = true;

		edict_t* pPlayer = pClient->getPlayer();
	
		if ( pPlayer && pPlayer->GetIServerEntity() )
			pClient->think();
	}
}

CClient *CClients :: findClientBySteamID (const char* szSteamID)
{
	for ( int i = 0; i < RCBOT_MAXPLAYERS; i ++ )
	{
		CClient* pClient = &m_Clients[i];

		if ( pClient->isUsed() )
		{
			if ( FStrEq(pClient->getSteamID(),szSteamID) )
				return pClient;
		}
	}	

	return nullptr;
}

void CClients::clientDebugMsg(CBot *pBot, int iLev, const char *fmt, ... )
{
	va_list argptr; 
	static char string[1024];

	va_start (argptr, fmt);
	std::vsprintf (string, fmt, argptr); 
	va_end (argptr); 

	clientDebugMsg(iLev,string,pBot);
}

const char *g_szDebugTags[15] =
{
"GAME_EVENT",
"NAV",
"SPEED",
"VIS",
"TASK",
"BUTTONS",
"USERCMD",
"UTIL",
"PROFILE",
"EDICTS",
"THINK",
"LOOK",
"HUD",
"AIM",
"CHAT"
};


void CClients :: clientDebugMsg ( int iLev, const char *szMsg, CBot *pBot )
{
	for ( int i = 0; i < RCBOT_MAXPLAYERS; i ++ )
	{
		CClient* pClient = &m_Clients[i];

		if ( !pClient->isUsed() )
			continue;
		if ( !pClient->isDebugOn(iLev) )
			continue;
		if ( pBot && !pClient->isDebuggingBot(pBot->getEdict()) )
			continue;

		if (pClient->isDebugOn(BOT_DEBUG_CHAT)) {
			char logmsg[128] = {};
			snprintf(logmsg, sizeof logmsg,"[DEBUG %s] %s",g_szDebugTags[iLev],szMsg);
			RCBotPluginMeta::HudTextMessage(pClient->getPlayer(), logmsg);
		}

		CBotGlobals::botMessage(pClient->getPlayer(),0,"[DEBUG %s] %s",g_szDebugTags[iLev],szMsg);
	}
}

	// get index in array
int CClients :: slotOfEdict (const edict_t* pPlayer)
{
	return ENTINDEX(pPlayer)-1;
}

bool CClients :: clientsDebugging (int iLev)
{
	if ( iLev == 0 )
		return m_bClientsDebugging;
	if ( m_bClientsDebugging )
	{
		for ( int i = 0; i < RCBOT_MAXPLAYERS; i ++ )
		{
			const CClient* pClient = CClients::get(i);

			if ( pClient->isUsed() )
			{
				if ( pClient->isDebugOn(iLev) )
					return true;
			}
		}
	}

	return false;
}

void CClient :: setWaypointCut (CWaypoint *pWaypoint)
{
	if ( pWaypoint )
	{
		setWaypointCopy(pWaypoint);

		m_WaypointCutPaths.clear();

		for ( int i = 0; i < pWaypoint->numPaths(); i ++ )
		{
			m_WaypointCutPaths.emplace_back(pWaypoint->getPath(i));
		}

		m_WaypointCopyType = WPT_COPY_CUT;
	}
}

void CClient :: setWaypointCopy (CWaypoint *pWaypoint) 
{
	if (pWaypoint) 
	{ 
		m_fCopyWptRadius = pWaypoint->getRadius();
		m_iCopyWptFlags = pWaypoint->getFlags();
		m_iCopyWptArea = pWaypoint->getArea();
		m_WaypointCopyType = WPT_COPY_COPY;
	} 
}