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
 */

#include "bot_waypoint.h"

CBotCommandInline WaypointOnCommand("on", CMD_ACCESS_WAYPOINT, [](CClient *pClient, const BotCommandArgs& args)
{
	if ( pClient )
	{
		pClient->setWaypointOn(true);
		pClient->giveMessage("Waypoints On");
	}

	return COMMAND_ACCESSED;
});

CBotCommandInline WaypointOffCommand("off", CMD_ACCESS_WAYPOINT, [](CClient *pClient, const BotCommandArgs& args)
{
	pClient->setWaypointOn(false);
	pClient->giveMessage("Waypoints Off");
	CBotGlobals::botMessage(pClient->getPlayer(),0,"waypoints off");

	return COMMAND_ACCESSED;
});

CBotCommandInline WaypointAddCommand("add", CMD_ACCESS_WAYPOINT, [](CClient *pClient, const BotCommandArgs& args)
{
	CWaypoints::addWaypoint(pClient,args[0],args[1],args[2],args[3]);

	return COMMAND_ACCESSED;
});

CBotCommandInline WaypointDeleteCommand("delete", CMD_ACCESS_WAYPOINT, [](CClient* pClient, const BotCommandArgs& args)
{
	if (!pClient)
		return COMMAND_ERROR;

	auto deleteWaypointsInRange = [&](const float radius)
	{
		int numDeleted = 0;
		const Vector vOrigin = pClient->getOrigin();
		WaypointList pWpt;
		CWaypointLocations::GetAllInArea(vOrigin, &pWpt, -1);

		for (const int i : pWpt)
		{
			const CWaypoint* pWaypoint = CWaypoints::getWaypoint(i);
			if (pWaypoint && pWaypoint->distanceFrom(vOrigin) < radius)
			{
				CWaypoints::deleteWaypoint(i);
				numDeleted++;
			}
		}

		if (numDeleted > 0)
		{
			CBotGlobals::botMessage(pClient->getPlayer(), 0, "%d waypoints within range of %.0f deleted", numDeleted, radius);
			pClient->updateCurrentWaypoint();
			pClient->playSound("buttons/combine_button_locked");
			pClient->giveMessage("Waypoints deleted");
		}
		else
		{
			CBotGlobals::botMessage(pClient->getPlayer(), 0, "No waypoints within range of %.0f", radius);
			pClient->playSound("weapons/wpn_denyselect");
			pClient->giveMessage("No waypoints deleted");
		}
	};

	if (args[0] && *args[0])
	{
		const float radius = static_cast<float>(std::atof(args[0]));
		if (radius > 0)
		{
			deleteWaypointsInRange(radius);
		}
		else
		{
			CBotGlobals::botMessage(pClient->getPlayer(), 0, "Invalid radius value");
			return COMMAND_ERROR;
		}
	}
	else
	{
		pClient->updateCurrentWaypoint();
		if (CWaypoints::validWaypointIndex(pClient->currentWaypoint()))
		{
			CWaypoints::deleteWaypoint(pClient->currentWaypoint());
			CBotGlobals::botMessage(pClient->getPlayer(), 0, "Waypoint %d deleted", pClient->currentWaypoint());
			pClient->updateCurrentWaypoint();
			pClient->playSound("buttons/combine_button_locked");
			pClient->giveMessage("Waypoint deleted");
		}
		else
		{
			CBotGlobals::botMessage(pClient->getPlayer(), 0, "No waypoint nearby to delete");
			pClient->playSound("weapons/wpn_denyselect");
			pClient->giveMessage("No Waypoint");
		}
	}

	return COMMAND_ACCESSED;
});

CBotCommandInline WaypointInfoCommand("info", CMD_ACCESS_WAYPOINT, [](CClient *pClient, const BotCommandArgs& args)
{
	pClient->updateCurrentWaypoint();

	if ( CWaypoint *pWpt = CWaypoints::getWaypoint(pClient->currentWaypoint()) )
		pWpt->info(pClient->getPlayer());

	return COMMAND_ACCESSED;
});

CBotCommandInline WaypointSaveCommand("save", CMD_ACCESS_WAYPOINT, [](CClient *pClient, const BotCommandArgs& args)
{
	if ( CWaypoints::save(false,(pClient!= nullptr)?pClient->getPlayer(): nullptr,((args[0]!= nullptr) && (*args[0]!=0))?args[0]: nullptr,((args[1]!= nullptr) && (*args[1]!=0))?args[1]: nullptr) )
	{
		CBotGlobals::botMessage(nullptr,0,"waypoints saved");
		if ( pClient )
			pClient->giveMessage("Waypoints Saved");
	}
	else
		CBotGlobals::botMessage(nullptr,0,"error: could not save waypoints");

	return COMMAND_ACCESSED;
});

CBotCommandInline WaypointLoadCommand("load", CMD_ACCESS_WAYPOINT, [](CClient* pClient, const BotCommandArgs& args)
{
	const char* mapNameToLoad = (args[0] && *args[0]) ? args[0] : CBotGlobals::getMapName();
	const bool bLoadOK = CWaypoints::load(mapNameToLoad);

	if (bLoadOK)
		CBotGlobals::botMessage(nullptr, 0, "waypoints %s loaded", mapNameToLoad);
	else
		CBotGlobals::botMessage(nullptr, 0, "error: could not load %s waypoints", mapNameToLoad);

	return COMMAND_ACCESSED;
});


CBotCommandInline WaypointClearCommand("clear", CMD_ACCESS_WAYPOINT, [](const CClient *pClient, const BotCommandArgs& args)
{
	CWaypoints::init();
	CBotGlobals::botMessage(pClient->getPlayer(),0,"waypoints cleared");

	return COMMAND_ACCESSED;
});

CBotCommandInline WaypointGiveTypeCommand("givetype", CMD_ACCESS_WAYPOINT, [](CClient *pClient, const BotCommandArgs& args)
{
	edict_t *pEntity = pClient->getPlayer();

	if ( args[0] && *args[0] )
	{
		if ( pClient->currentWaypoint() == -1 )
			CBotGlobals::botMessage(pEntity,0,"No waypoint nearby to give types (move closer to the waypoint you want to give types)");
		else
		{
			char *type = nullptr;
			constexpr int NUM_ARGS = 4;

			for (int i = 0; i < NUM_ARGS; i++)
			{
				if ( i == 0 )
					type = const_cast<char*>(args[0]);
				else if ( i == 1 )
					type = const_cast<char*>(args[1]);
				else if ( i == 2 )
					type = const_cast<char*>(args[2]);
				else if ( i == 3 )
					type = const_cast<char*>(args[3]);

				if ( !type || !*type )
					break;

				if ( const CWaypointType *pType = CWaypointTypes::getType(type) )
				{
					if ( CWaypoint *pWaypoint = CWaypoints::getWaypoint(pClient->currentWaypoint()) )
					{
						if ( pWaypoint->hasFlag(pType->getBits()) )
						{
							pWaypoint->removeFlag(pType->getBits());
							CBotGlobals::botMessage(pEntity,0,"type %s removed from waypoint %d",type,CWaypoints::getWaypointIndex(pWaypoint));
							pClient->playSound("UI/buttonrollover");
						}
						else
						{
							pWaypoint->addFlag(pType->getBits());
							
							if ( pType->getBits() & CWaypointTypes::W_FL_UNREACHABLE )
							{
								CWaypoints::deletePathsTo(CWaypoints::getWaypointIndex(pWaypoint));
								CWaypoints::deletePathsFrom(CWaypoints::getWaypointIndex(pWaypoint));
							}

							CBotGlobals::botMessage(pEntity,0,"type %s added to waypoint %d",type,CWaypoints::getWaypointIndex(pWaypoint));

							pClient->playSound("UI/buttonclickrelease");
						}
						
					}
				}
				else
				{
					CBotGlobals::botMessage(pEntity,0,"type '%s' not found",type);
					CWaypointTypes::showTypesOnConsole(pEntity);
				}

			}
		}
	}
	else
	{
		CWaypointTypes::showTypesOnConsole(pEntity);
	}

	return COMMAND_ACCESSED;
});

CBotCommandInline WaypointDrawTypeCommand("drawtype", CMD_ACCESS_WAYPOINT, [](CClient* pClient, const BotCommandArgs& args)
{
	if (pClient)
	{
		if (args[0] && *args[0])
		{
			int drawType = std::atoi(args[0]);
			if (drawType >= 0 && drawType <= std::numeric_limits<unsigned short>::max())
			{
				pClient->setDrawType(static_cast<unsigned short>(drawType));
				return COMMAND_ACCESSED;
			}
			// Handle out-of-range value
			return COMMAND_ERROR;
		}
	}
	return COMMAND_ERROR;
}, "0: for effects engine (maximum limit of beams)\n1: for debug overlay (no limit of beams) [LISTEN SERVER CLIENT ONLY]");

CBotCommandInline WaypointAngleCommand("angle", CMD_ACCESS_WAYPOINT, [](CClient *pClient, const BotCommandArgs& args)
{
	if ( pClient && pClient->getPlayer() )
	{
		pClient->updateCurrentWaypoint();

		if ( CWaypoint *pWpt = CWaypoints::getWaypoint(pClient->currentWaypoint()) )
		{
			const QAngle eye = CBotGlobals::playerAngles(pClient->getPlayer());
			CBotGlobals::botMessage(pClient->getPlayer(),0,"Waypoint Angle == %0.3f deg, (Eye == %0.3f)",CBotGlobals::yawAngleFromEdict(pClient->getPlayer(),pWpt->getOrigin()),eye.y);
			pClient->playSound("buttons/combine_button1");
		}
	}

	return COMMAND_ACCESSED;
});

CBotCommandInline WaypointSetAngleCommand("updateyaw", CMD_ACCESS_WAYPOINT, [](CClient *pClient, const BotCommandArgs& args)
{
	pClient->updateCurrentWaypoint();

	if ( CWaypoints::validWaypointIndex(pClient->currentWaypoint()) )
	{
		CWaypoint *pWpt = CWaypoints::getWaypoint(pClient->currentWaypoint());

		(pWpt->setAim(CBotGlobals::playerAngles(pClient->getPlayer()).y));
	}

	return COMMAND_ACCESSED;
});

CBotCommandInline WaypointSetAreaCommand("setarea", CMD_ACCESS_WAYPOINT, [](CClient *pClient, const BotCommandArgs& args)
{
	pClient->updateCurrentWaypoint();

	if ( args[0] && *args[0] && ( CWaypoints::validWaypointIndex(pClient->currentWaypoint()) ) )
	{
		CWaypoint *pWpt = CWaypoints::getWaypoint(pClient->currentWaypoint());

		pWpt->setArea(std::atoi(args[0]));
	}
	else
		return COMMAND_ERROR;

	return COMMAND_ACCESSED;
}, "Go to a waypoint, use setarea <areaid>");

CBotCommandInline WaypointSetRadiusCommand("setradius", CMD_ACCESS_WAYPOINT, [](CClient *pClient, const BotCommandArgs& args)
{
	pClient->updateCurrentWaypoint();

	if ( args[0] && *args[0] && ( CWaypoints::validWaypointIndex(pClient->currentWaypoint()) ) )
	{
		CWaypoint *pWpt = CWaypoints::getWaypoint(pClient->currentWaypoint());

		pWpt->setRadius(static_cast<float>(std::atof(args[0])));
	}
	else
		return COMMAND_ERROR;

	return COMMAND_ACCESSED;
}, "Go to a waypoint, use setradius <radius>");

CBotCommandInline WaypointMenuCommand("menu", CMD_ACCESS_WAYPOINT, [](CClient *pClient, const BotCommandArgs& args)
{
	pClient->setCurrentMenu(CBotMenuList::getMenu(BOT_MENU_WPT));

	return COMMAND_ACCESSED;
	/*
	if ( pClient )
	{
		// number of waypoint types to show per menu
		unsigned const int iNumToShow = 7;

		unsigned i = 0;
		// menu level .. ?
		unsigned iLevel = 0;
		// number of possible waypoint types to show
		unsigned iNumTypes = CWaypointTypes::getNumTypes();

		char num[64], msg[64], cmd[128];
		CWaypointType *p;
		CWaypoint *pWpt;

		// get current waypoint types on current waypoint
		pClient->updateCurrentWaypoint();
		pWpt = CWaypoints::getWaypoint(pClient->currentWaypoint());

		if ( !pWpt )
		{
			CBotGlobals::botMessage(pClient->getPlayer(),0,"Not next to any waypoints to show a menu for");
			return COMMAND_ERROR;
		}

		// Menu level to start, 0 = beginning
		if ( args[0] && *args[0] )
		{
			iLevel = atoi(args[0]);
		}
		else
		{
			iLevel = 0;
		}

		//pClient->update

		KeyValues *kv = new KeyValues( "menu" );
		kv->SetString( "title", "Waypoint Menu" );
		kv->SetInt( "level", 1 );
		kv->SetColor( "color", Color( 255, 0, 0, 255 ));
		kv->SetInt( "time", 15 );
		kv->SetString( "msg", "Select Waypoint Type" );
		
		// start at this waypoint type index
		unsigned iIndex = iLevel*iNumToShow;

		// run through a small number
		while ( (i < iNumToShow) && (iIndex < iNumTypes) )
		{
			p = CWaypointTypes::getTypeByIndex(iIndex);
			
			Q_snprintf( num, sizeof(num), "%i", i );

			// if waypoint has this type show it by putting a "[x]" next to it
			if ( pWpt->getFlags() & p->getBits() )
				Q_snprintf( msg, sizeof(msg), "%s [x]", p->getName() );
			else
				Q_snprintf( msg, sizeof(msg), "%s [ ]", p->getName() );

			Q_snprintf( cmd, sizeof(cmd), "rcbot waypoint givetype %s", p->getName() );

			KeyValues *item1 = kv->FindKey( num, true );
			item1->SetString( "msg", msg );
			item1->SetString( "command", cmd );

			iIndex ++;
			i++;
		}

		// finally show "More" option if available
		if ( iIndex < iNumTypes )
		{
			Q_snprintf( num, sizeof(num), "%i", i );
			Q_snprintf( msg, sizeof(msg), "More..." );
			Q_snprintf( cmd, sizeof(cmd), "rcbot waypoint menu %d", iLevel+1 );

			KeyValues *item1 = kv->FindKey( num, true );
			item1->SetString( "msg", msg );
			item1->SetString( "command", cmd );
		}

		helpers->CreateMessage( pClient->getPlayer(), DIALOG_MENU, kv, &g_RCBOTServerPlugin );
		kv->deleteThis();

		//pClient->showMenu();

		return COMMAND_ACCESSED;
	}

	return COMMAND_ERROR;*/
});

CBotCommandInline WaypointCut("cut", CMD_ACCESS_WAYPOINT, [](CClient *pClient, const BotCommandArgs& args)
{
	if ( pClient )
	{
		pClient->updateCurrentWaypoint();

		if ( const CWaypoint *pWpt = CWaypoints::getWaypoint(pClient->currentWaypoint()) )
		{
			pClient->setWaypointCut(pWpt);
			CWaypoints::deleteWaypoint(pClient->currentWaypoint());
		}

		return COMMAND_ACCESSED;
	}

	return COMMAND_ERROR;
}, "allows you to move a waypoint by cutting it and pasting it");

CBotCommandInline WaypointCopy("copy", CMD_ACCESS_WAYPOINT, [](CClient *pClient, const BotCommandArgs& args)
{
	if ( pClient )
	{
		pClient->updateCurrentWaypoint();

		if ( const CWaypoint *pWpt = CWaypoints::getWaypoint(pClient->currentWaypoint()) )
		{
			pClient->setWaypointCopy(pWpt);
			return COMMAND_ACCESSED;
		}
	}

	return COMMAND_ERROR;
}, "Go to a waypoint, and copy to hold its properties, then use paste to make a new waypoint with the same properties");

CBotCommandInline WaypointPaste("paste", CMD_ACCESS_WAYPOINT, [](CClient *pClient, const BotCommandArgs& args)
{
	if ( pClient )
	{
		CWaypoints::addWaypoint(pClient, nullptr, nullptr, nullptr, nullptr,true);
		return COMMAND_ACCESSED;
	}

	return COMMAND_ERROR;
}, "first copy a waypoint using the copy command and then use paste to make a new waypoint with the same properties");

CBotCommandInline WaypointShiftAreas("shiftareas", CMD_ACCESS_WAYPOINT, [](const CClient *pClient, const BotCommandArgs& args)
{
	if ( pClient )
	{
		int val = 1;

		if ( args[0] && *args[0] )
			val = std::atoi(args[0]);

		if ( args[1] && *args[1] )
		{
			const int newarea = std::atoi(args[1]);

			if ( pClient )
			{
				// change area val to newarea
				CWaypoints::shiftVisibleAreas(pClient->getPlayer(),val,newarea);
			}
		}
		else
			CWaypoints::shiftAreas(val);

		return COMMAND_ACCESSED;
	}

	return COMMAND_ERROR;
}, "shift the areas of flagged waypoints to a different area/only use this once");

CBotCommandInline WaypointTeleportCommand("teleport", CMD_ACCESS_WAYPOINT | CMD_ACCESS_DEDICATED, [](CClient *pClient, const BotCommandArgs& args)
{
	if ( pClient && args[0] && *args[0] )
	{
		CWaypoint *pWpt;
		bool bTele = false;

		const int id = std::atoi(args[0]);

		if ( (pWpt=CWaypoints::getWaypoint(id)) != nullptr)
		{
			if ( pWpt->isUsed() )
			{
				pClient->teleportTo(pWpt->getOrigin()+Vector(0,0,8));
				bTele = true;
			}
		}

		if ( bTele )
		{
			CBotGlobals::botMessage(pClient->getPlayer(),0,"Teleported to waypoint %d",id);
		}
		else
		{
			CBotGlobals::botMessage(pClient->getPlayer(),0,"Invalid waypoint id %d",id);
		}

		return COMMAND_ACCESSED;
	}

	return COMMAND_ERROR;
});

CBotCommandInline WaypointAreaSetToNearest("setareatonearest", CMD_ACCESS_WAYPOINT, [](const CClient *pClient, const BotCommandArgs& args)
{
	if ( pClient )
	{
		CWaypoint *pWpt;
		bool bOk = false;
		int setarea = 0;

		const int id = pClient->currentWaypoint();

		if ( (pWpt=CWaypoints::getWaypoint(id)) != nullptr)
		{
			if ( pWpt->isUsed() )
			{

				#if SOURCE_ENGINE == SE_TF2
					setarea = CTeamFortress2Mod::m_ObjectiveResource.NearestArea(pWpt->getOrigin());
				#elif SOURCE_ENGINE == SE_DODS
					setarea = CDODMod::m_Flags.findNearestObjective(pWpt->getOrigin());
				#endif

				if ( setarea > 0 )
					pWpt->setArea(setarea);

				bOk = true;
			}
		}

		if ( bOk )
		{
			if ( setarea > 0 )
				CBotGlobals::botMessage(pClient->getPlayer(),0,"Changed waypoint %d area to %d",id,setarea);
			else
				CBotGlobals::botMessage(pClient->getPlayer(),0,"No nearest area to wpt id %d",id);
		}
		else
		{
			CBotGlobals::botMessage(pClient->getPlayer(),0,"Invalid waypoint id %d",id);
		}
		return COMMAND_ACCESSED;
	}

	return COMMAND_ERROR;
});

CBotCommandInline WaypointShowCommand("show", CMD_ACCESS_WAYPOINT, [](const CClient *pClient, const BotCommandArgs& args)
{
	if ( pClient )
	{
		const int wpt = std::atoi(args[0]);

		if ( CWaypoint *pWpt = CWaypoints::getWaypoint(wpt) )
		{
#ifndef SM_EXT
			g_pEffects->Beam( CBotGlobals::entityOrigin(pClient->getPlayer()), pWpt->getOrigin(), CWaypoints::waypointTexture(),
				0, 0, 1,
				5, 12, 12, 255,
				10, 255, 255, 255, 200, 10);
#endif

			//pClient->setShowWpt(wpt);

			return COMMAND_ACCESSED;
		}
		//else
			//pClient->setShowWpt(-1);
	}

	return COMMAND_ERROR;
}, "show <waypoint ID>, shows you to this waypoint");

CBotCommandInline WaypointCheckCommand("check", CMD_ACCESS_WAYPOINT, [](const CClient *pClient, const BotCommandArgs& args)
{
	// loop through every waypoint and check the areas are not outside the number of control points

	CWaypoints::checkAreas((pClient== nullptr)? nullptr :pClient->getPlayer());

	return COMMAND_ACCESSED;
});

CBotCommandInline WaypointShowVisCommand("showvis", CMD_ACCESS_WAYPOINT, [](CClient *pClient, const BotCommandArgs& args)
{
#ifndef __linux__
	pClient->updateCurrentWaypoint();

	if ( CWaypoints::validWaypointIndex(pClient->currentWaypoint()) )
	{
		CWaypoint *pWpt = CWaypoints::getWaypoint(pClient->currentWaypoint());

		const float ftime = (args[0] && *args[0]) ? static_cast<float>(std::atof(args[0])) : 5.0f;

		if ( pWpt )
		{
			const int index = CWaypoints::getWaypointIndex(pWpt);
			const CWaypointVisibilityTable *pTable = CWaypoints::getVisiblity();
	
			for ( int i = 0; i < CWaypoints::numWaypoints(); i ++ )
			{
				CWaypoint *pOther = CWaypoints::getWaypoint(i);

				if ( !pOther->isUsed() )
					continue;

				if ( pOther->distanceFrom(pWpt) > 1024.0f )
					continue;

				const bool bVis = pTable->GetVisibilityFromTo(index, i);
				
				debugoverlay->AddTextOverlayRGB(pOther->getOrigin(),0,ftime,bVis ? 0 : 255,bVis ? 255 : 0,0,200,bVis ? "VIS" : "INV" );
			}
		}
		
	}
	else
		return COMMAND_ERROR;
#endif
	return COMMAND_ACCESSED;
}, "Go to a waypoint, use showvis to see visibility");

CBotCommandInline WaypointAutoWaypointCommand("autowaypoint", CMD_ACCESS_WAYPOINT, [](CClient *pClient,
											  const BotCommandArgs& args)
{
	if ( pClient )
	{
		const int temp = std::atoi(args[0]);
		pClient->setAutoWaypointMode(temp > 0, temp == 2);
		CBotGlobals::botMessage(pClient->getPlayer(), 0, "Autowaypointing Mode %s, Debug %s", (temp > 0) ? "ON" : "OFF", (temp == 2) ? "ON" : "OFF");
	}
	return COMMAND_ACCESSED;
});

CBotCommandInline WaypointAutoFix("autofix", CMD_ACCESS_WAYPOINT, [](CClient *pClient, const BotCommandArgs& args)
{
	bool bFixSentry_Sniper_Defend_TeleExtWpts = false;

	if ( args[0] && *args[0] )
	{
		bFixSentry_Sniper_Defend_TeleExtWpts = ( std::atoi(args[0]) == 1 );
	}

	CWaypoints::autoFix(bFixSentry_Sniper_Defend_TeleExtWpts);
	
	return COMMAND_ACCESSED;
});

CBotSubcommands WaypointSubcommands("waypoint", "wpt", CMD_ACCESS_DEDICATED, {
	&WaypointOnCommand,
	&WaypointOffCommand,
	&WaypointAddCommand,
	&WaypointDeleteCommand,
	&WaypointInfoCommand,
	&WaypointSaveCommand,
	&WaypointLoadCommand,
	&WaypointClearCommand,
	&WaypointGiveTypeCommand,
	&WaypointDrawTypeCommand,
	&WaypointAngleCommand,
	&WaypointSetAngleCommand,
	&WaypointSetAreaCommand,
	&WaypointSetRadiusCommand,
	&WaypointMenuCommand,
	&WaypointCut,
	&WaypointCopy,
	&WaypointPaste,
	&WaypointShiftAreas,
	&WaypointTeleportCommand,
	&WaypointAreaSetToNearest,
	&WaypointShowCommand,
	&WaypointCheckCommand,
	&WaypointShowVisCommand,
	&WaypointAutoWaypointCommand,
	&WaypointAutoFix
});
