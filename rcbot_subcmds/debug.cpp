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

#include <cmath>

CBotCommandInline DebugGameEventCommand("gameevent", CMD_ACCESS_DEBUG, [](CClient *pClient, BotCommandArgs args)
{
	if (!args[0] || !*args[0]) {
		return COMMAND_ERROR;
	}
	pClient->setDebug(BOT_DEBUG_GAME_EVENT,std::atoi(args[0])>0);
	return COMMAND_ACCESSED;
}, "usage \"gameevent 1 or 0, 1 on, 0 off\" : shows event output");

CBotCommandInline DebugBotCommand("bot", CMD_ACCESS_DEBUG, [](CClient *pClient, BotCommandArgs args)
{
	if (!args[0] || !*args[0]) {
		extern IServerGameEnts *servergameents;
		// do a traceline in front of player

		const Vector vOrigin = pClient->getOrigin();
		const QAngle angles = CBotGlobals::entityEyeAngles(pClient->getPlayer());
		Vector forward;

		AngleVectors(angles,&forward);

		CBotGlobals::quickTraceline(pClient->getPlayer(),vOrigin,vOrigin+forward*1024.0f);
		CBaseEntity *pEntity;

		if ( (pEntity = CBotGlobals::getTraceResult()->m_pEnt) != nullptr)
		{
			edict_t *pEdict = servergameents->BaseEntityToEdict(pEntity);
			if ( CBots::getBotPointer(pEdict) != nullptr)
			{
				pClient->setDebugBot(pEdict);
				CBotGlobals::botMessage(pClient->getPlayer(),0,"debug bot set to bot you are looking at");
				return COMMAND_ACCESSED;
			}
			pClient->setDebugBot(nullptr);
			CBotGlobals::botMessage(pClient->getPlayer(),0,"debug bot cleared");
		}
		else
		{
		pClient->setDebugBot(nullptr);
		CBotGlobals::botMessage(pClient->getPlayer(),0,"debug bot cleared");
		}
		return COMMAND_ERROR;
	}
	
	edict_t *pEnt = CBotGlobals::findPlayerByTruncName(args[0]);

	if ( !pEnt )
	{
		CBotGlobals::botMessage(pClient->getPlayer(),0,"can't find a player with that name");
		return COMMAND_ERROR;
	}

	const CBot *pBot = CBots::getBotPointer(pEnt);

	if ( !pBot )
	{
		CBotGlobals::botMessage(pClient->getPlayer(),0,"can't find a bot with that name");
		return COMMAND_ERROR;
	}

	pClient->setDebugBot(pBot->getEdict());	

	return COMMAND_ACCESSED;
}, "usage \"bot <partial bot name>, or just bot to switch off : shows bot debug output on listen server");

CBotCommandInline DebugNavCommand("nav", CMD_ACCESS_DEBUG, [](CClient *pClient, BotCommandArgs args)
{
	if ( !args[0] || !*args[0] )
		return COMMAND_ERROR;

	pClient->setDebug(BOT_DEBUG_NAV,std::atoi(args[0])>0);

	return COMMAND_ACCESSED;
}, "usage \"nav 1 or 0, 1 on, 0 off\" : shows navigation output");

CBotCommandInline DebugVisCommand("vis", CMD_ACCESS_DEBUG, [](CClient *pClient, BotCommandArgs args)
{
	if ( !args[0] || !*args[0] )
		return COMMAND_ERROR;

	pClient->setDebug(BOT_DEBUG_VIS,std::atoi(args[0])>0);

	return COMMAND_ACCESSED;
}, "usage \"vis 1 or 0, 1 on, 0 off\" : shows bot visibility output");

CBotCommandInline DebugThinkCommand("think", CMD_ACCESS_DEBUG, [](CClient *pClient, BotCommandArgs args)
{
	if ( !args[0] || !*args[0] )
		return COMMAND_ERROR;

	pClient->setDebug(BOT_DEBUG_THINK,std::atoi(args[0])>0);

	return COMMAND_ACCESSED;
}, "usage \"think 1 or 0, 1 on, 0 off\" : shows bot thinking output");

CBotCommandInline DebugLookCommand("look", CMD_ACCESS_DEBUG, [](CClient *pClient, BotCommandArgs args)
{
	if ( !args[0] || !*args[0] )
		return COMMAND_ERROR;

	pClient->setDebug(BOT_DEBUG_LOOK,std::atoi(args[0])>0);

	return COMMAND_ACCESSED;
}, "usage \"look 1 or 0, 1 on, 0 off\" : shows bot look output");

CBotCommandInline DebugHudCommand("hud", CMD_ACCESS_DEBUG, [](CClient *pClient, BotCommandArgs args)
{
	if ( !args[0] || !*args[0] )
		return COMMAND_ERROR;

	pClient->setDebug(BOT_DEBUG_HUD,std::atoi(args[0])>0);

	return COMMAND_ACCESSED;
}, "usage \"hud 1 or 0, 1 on, 0 off\" : displays most important info about bot on the hud");

CBotCommandInline DebugAimCommand("aim", CMD_ACCESS_DEBUG, [](CClient *pClient, BotCommandArgs args)
{
	if ( !args[0] || !*args[0] )
		return COMMAND_ERROR;

	pClient->setDebug(BOT_DEBUG_AIM,std::atoi(args[0])>0);

	return COMMAND_ACCESSED;
}, "usage \"aim 1 or 0, 1 on, 0 off\" : displays aiming accuracy info on the hud");

CBotCommandInline DebugChatCommand("chat", CMD_ACCESS_DEBUG, [](CClient *pClient, BotCommandArgs args)
{
	if ( !args[0] || !*args[0] )
		return COMMAND_ERROR;

	pClient->setDebug(BOT_DEBUG_CHAT,std::atoi(args[0])>0);

	return COMMAND_ACCESSED;
}, "usage \"chat 1 or 0, 1 on, 0 off\" : displays logs in chat");

CBotCommandInline BotGoto("bot_goto", CMD_ACCESS_DEBUG, [](CClient *pClient, BotCommandArgs args)
{
	if ( pClient && pClient->getDebugBot()!= nullptr)
	{
		edict_t *pEdict = pClient->getDebugBot();
		const CBot *pBot = CBots::getBotPointer(pEdict);

		if ( pBot->inUse() )
		{
			int iWpt;

			if ( args[0] && *args[0] )
			{
				iWpt = std::atoi(args[0]);

				if ( (iWpt < 0) || (iWpt >= CWaypoints::numWaypoints()) )
					iWpt = -1;
			}
			else
				iWpt = pClient->currentWaypoint();

			if ( iWpt != -1 )
				pBot->forceGotoWaypoint(iWpt);
		}
	}

	return COMMAND_ACCESSED;
}, "set a debug bot first and then stand near a waypoint to force your bot to go there");

CBotCommandInline BotFlush("bot_flush", CMD_ACCESS_DEBUG, [](CClient *pClient, BotCommandArgs args)
{
	if ( pClient && pClient->getDebugBot()!= nullptr)
	{
		edict_t *pEdict = pClient->getDebugBot();
		const CBot *pBot = CBots::getBotPointer(pEdict);

		if ( pBot->inUse() )
		{
			CBotSchedules *pSched = pBot->getSchedule();
			pSched->freeMemory();
		}
	}

	return COMMAND_ACCESSED;
}, "flush bot tasks");

CBotCommandInline DebugTaskCommand("task", CMD_ACCESS_DEBUG, [](CClient *pClient, BotCommandArgs args)
{
	if ( !args[0] || !*args[0] )
		return COMMAND_ERROR;

	pClient->setDebug(BOT_DEBUG_TASK,std::atoi(args[0])>0);

	return COMMAND_ACCESSED;
}, "usage \"nav 1 or 0, 1 on, 0 off\" : shows navigation output");

CBotCommandInline BotTaskCommand("givetask", CMD_ACCESS_DEBUG, [](CClient *pClient, BotCommandArgs args)
{
#ifndef __linux__

	if ( pClient && pClient->getDebugBot()!= nullptr)
	{
		edict_t *pEdict = pClient->getDebugBot();
		CBot *pBot = CBots::getBotPointer(pEdict);

		if ( pBot->inUse() )
		{
			CBotSchedules *pSched = pBot->getSchedule();

			if ( args[0] && *args[0] )
			{
				//int task = atoi(args[0]);

				pSched->freeMemory();

				// 83
				if ( !strcmp(args[0],"pipe") )
				{
					CBotUtility util = CBotUtility(pBot,BOT_UTIL_PIPE_LAST_ENEMY,true,1.0f);
					pBot->setLastEnemy(pClient->getPlayer());
					pBot->getSchedule()->freeMemory();
					static_cast<CBotTF2*>(pBot)->executeAction(&util);
				}
				// 71
				else if ( !strcmp(args[0],"gren") )
				{
					CBotWeapons* pWeapons = pBot->getWeapons();
					CBotWeapon* gren = pWeapons->getGrenade();

					if ( gren )
					{
						CBotSchedule *sched = new CBotSchedule(new CThrowGrenadeTask(gren,pBot->getAmmo(gren->getWeaponInfo()->getAmmoIndex1()),pClient->getOrigin()));
						pSched->add(sched);
					}
				}
				else if ( !strcmp(args[0],"snipe") )
				{
					if ( pClient )
					{
						
						CWaypoint *pWaypoint = CWaypoints::getWaypoint(CWaypoints::nearestWaypointGoal(CWaypointTypes::W_FL_SNIPER,pClient->getOrigin(),200.0f,pBot->getTeam()));
					
						if ( pWaypoint )
						{
							#if SOURCE_ENGINE == SE_TF2
								//if ( CClassInterface::getTF2Class() )
							#else
							CBotSchedule *snipe = new CBotSchedule();
							CBotTask *findpath = new CFindPathTask(CWaypoints::getWaypointIndex(pWaypoint));

							CBotWeapons* m_pWeapons = pBot->getWeapons();
							CBotWeapon* pWeapon = m_pWeapons->hasWeapon(DOD_WEAPON_K98_SCOPED)
								                      ? m_pWeapons->getWeapon(CWeapons::getWeapon(DOD_WEAPON_K98_SCOPED))
								                      : m_pWeapons->getWeapon(CWeapons::getWeapon(DOD_WEAPON_SPRING));

							if ( pWeapon )
							{
								// linux fix - copy origin onto vector here
								const Vector vOrigin = pWaypoint->getOrigin();
								CBotTask* snipetask = new CBotDODSnipe(pWeapon, vOrigin, pWaypoint->getAimYaw(), false, 0, pWaypoint->getFlags());

								findpath->setCompleteInterrupt(CONDITION_PUSH);
								snipetask->setCompleteInterrupt(CONDITION_PUSH);

								snipe->setID(SCHED_DEFENDPOINT);
								snipe->addTask(findpath);
								snipe->addTask(snipetask);
								
								pSched->add(snipe);
							}
							else
								CBotGlobals::botMessage(nullptr,0,"Bot is not a sniper");
							#endif
						}
						else
							CBotGlobals::botMessage(nullptr,0,"Sniper waypoint not found");

					}
				}
			}
			
		}
	}

#endif
	return COMMAND_ACCESSED;
}, "gives a bot a task : usage <id> <entity name - for reference>");

CBotCommandInline DebugButtonsCommand("buttons", CMD_ACCESS_DEBUG, [](CClient *pClient, BotCommandArgs args)
{
	if ( !args[0] || !*args[0] )
		return COMMAND_ERROR;

	pClient->setDebug(BOT_DEBUG_BUTTONS,std::atoi(args[0])>0);

	return COMMAND_ACCESSED;
}, "usage \"buttons 1 or 0, 1 on, 0 off\" : shows buttons bitmask");

CBotCommandInline DebugSpeedCommand("speed", CMD_ACCESS_DEBUG, [](CClient *pClient, BotCommandArgs args)
{
	if ( !args[0] || !*args[0] )
		return COMMAND_ERROR;

	pClient->setDebug(BOT_DEBUG_SPEED,std::atoi(args[0])>0);

	return COMMAND_ACCESSED;
}, "usage \"speed 1 or 0, 1 on, 0 off\" : shows speed");

CBotCommandInline DebugUsercmdCommand("usercmd", CMD_ACCESS_DEBUG, [](CClient *pClient, BotCommandArgs args)
{
	if ( !args[0] || !*args[0] )
		return COMMAND_ERROR;

	pClient->setDebug(BOT_DEBUG_USERCMD,std::atoi(args[0])>0);

	return COMMAND_ACCESSED;
}, "usage \"usercmd 1 or 0, 1 on, 0 off\" : shows last user command output");

CBotCommandInline DebugUtilCommand("util", CMD_ACCESS_DEBUG, [](CClient *pClient, BotCommandArgs args)
{
	if ( !args[0] || !*args[0] )
		return COMMAND_ERROR;

	pClient->setDebug(BOT_DEBUG_UTIL,std::atoi(args[0])>0);

	return COMMAND_ACCESSED;
}, "usage \"util 1 or 0, 1 on, 0 off\" : shows utility/action output");

CBotCommandInline DebugProfilingCommand("profiling", CMD_ACCESS_DEBUG, [](CClient *pClient, BotCommandArgs args)
{
	if ( !args[0] || !*args[0] )
		return COMMAND_ERROR;

	pClient->setDebug(BOT_DEBUG_PROFILE,std::atoi(args[0])>0);

	return COMMAND_ACCESSED;
}, "usage \"profiling 1 or 0, 1 on, 0 off\" : shows performance profiling");

CBotCommandInline DebugEdictsCommand("edicts", CMD_ACCESS_DEBUG, [](CClient *pClient, BotCommandArgs args)
{
	if ( !args[0] || !*args[0] )
		return COMMAND_ERROR;

	pClient->setDebug(BOT_DEBUG_EDICTS,std::atoi(args[0])>0);

	return COMMAND_ACCESSED;
}, "usage \"edicts 1 or 0, 1 on, 0 off\" : shows allocated/freed edicts");

CBotCommandInline PrintProps("printprops", CMD_ACCESS_DEBUG, [](CClient *pClient, BotCommandArgs args)
{
	if ( pClient )
	{

		if ( args[0] && *args[0] )
		{
			extern bool g_PrintProps;
			unsigned int m_offset;
			g_PrintProps = true;
			
			ServerClass *sc = UTIL_FindServerClass(args[0]);

			if ( sc )
				UTIL_FindSendPropInfo(sc,"",&m_offset);

			g_PrintProps = false;
		}


		return COMMAND_ACCESSED;
	}

	return COMMAND_ERROR;
});

CBotCommandInline SetProp("setprop", CMD_ACCESS_DEBUG, [](CClient *pClient, BotCommandArgs args)
{
	
	
	if ( pClient )
	{
		// classname           // key             // type          //value
		if ( (args[0] && *args[0]) &&(args[1] && *args[1]) && (args[2] && *args[2]) && (args[3] && *args[3]))
		{
			//int i = 0;

			edict_t *pPlayer = pClient->getPlayer();
//			edict_t *pEdict;
			edict_t *pNearest = nullptr;
//			float fDistance;
//			float fNearest = 400.0f;

			pNearest = CClassInterface::FindEntityByNetClassNearest(pClient->getOrigin(),args[0]);

			if ( pNearest )
			{
				void *data = nullptr;

				extern bool g_PrintProps;
				unsigned int m_offset = 0;

				ServerClass *sc = UTIL_FindServerClass(args[0]);

				if ( sc )
				{
					UTIL_FindSendPropInfo(sc,args[1],&m_offset);

					if ( m_offset )
					{
						static IServerUnknown *pUnknown;
						static CBaseEntity *pEntity;
						Vector vdata;

						pUnknown = pNearest->GetUnknown();
					 
						pEntity = pUnknown->GetBaseEntity();

						data = static_cast<void*>(reinterpret_cast<char*>(pEntity) + m_offset);

						if ( data )
						{
							bool *booldata = static_cast<bool*>(data);
							int *intdata = static_cast<int*>(data);
							float *floatdata = static_cast<float*>(data);

							if ( strcmp(args[2],"int")==0)
								*intdata = std::atoi(args[3]);
							else if ( strcmp(args[2],"bool")==0 )
								*booldata = (std::atoi(args[3])==1);
							else if ( strcmp(args[2],"float")==0 )
								*floatdata = std::atof(args[3]);
						}
						else
							CBotGlobals::botMessage(pPlayer,0,"NULL");
					}
					else
						CBotGlobals::botMessage(nullptr,0,"OFFSET NOT FOUND");
				}
				else
					CBotGlobals::botMessage(nullptr,0,"CLASS NOT FOUND");

			}
			else
				CBotGlobals::botMessage(nullptr,0,"EDICT NOT FOUND");
		}
		else
			CBotGlobals::botMessage(nullptr,0,"Usage: getprop CLASSNAME KEY TYPE(int,bool,float) VALUE");

		return COMMAND_ACCESSED;
	}

	return COMMAND_ERROR;
});

CBotCommandInline GetProp("getprop", CMD_ACCESS_DEBUG, [](CClient *pClient, BotCommandArgs args)
{
	if ( pClient )
	{
		if ( (args[0] && *args[0]) &&(args[1] && *args[1]) )
		{
			//int i = 0;

			edict_t *pPlayer = pClient->getPlayer();
//			edict_t *pEdict;
			edict_t *pNearest = nullptr;
//			float fDistance;
//			float fNearest = 400.0f;

			pNearest = CClassInterface::FindEntityByNetClassNearest(pClient->getOrigin(),args[0]);

			if ( pNearest )
			{
				void *data = nullptr;

				extern bool g_PrintProps;
				unsigned int m_offset = 0;

				ServerClass *sc = UTIL_FindServerClass(args[0]);

				if ( sc )
				{
					UTIL_FindSendPropInfo(sc,args[1],&m_offset);

					if ( m_offset )
					{
						static IServerUnknown *pUnknown;
						static CBaseEntity *pEntity;

						pUnknown = pNearest->GetUnknown();
					 
						pEntity = pUnknown->GetBaseEntity();

						int preoffs = 0;

						if ( (args[2] && *args[2]) )
						{
							preoffs = std::atoi(args[2]);	
						}

						data = static_cast<void*>(reinterpret_cast<char*>(pEntity) + m_offset);

						if ( data )
						{
							const Vector vdata = *(static_cast<Vector*>(data) + preoffs);
	
							CBotGlobals::botMessage(pPlayer,0,"int = %d, float = %f, bool = %s, Vector = (%0.4f,%0.4f,%0.4f)",*(static_cast<int*>(data) + preoffs),*(static_cast<float*>(data)+preoffs),*(static_cast<bool*>(data)+preoffs) ? ("true"):("false"),vdata.x,vdata.y,vdata.z );
						}
						else
							CBotGlobals::botMessage(pPlayer,0,"NULL");
					}
					else
						CBotGlobals::botMessage(nullptr,0,"OFFSET NOT FOUND");
				}
				else
					CBotGlobals::botMessage(nullptr,0,"CLASS NOT FOUND");

			}
			else
				CBotGlobals::botMessage(nullptr,0,"EDICT NOT FOUND");
		}
		else
			CBotGlobals::botMessage(nullptr,0,"Usage: getprop CLASS CLASSNAME KEY");

		return COMMAND_ACCESSED;
	}

	return COMMAND_ERROR;
});

CBotCommandInline FindClass("findclass", CMD_ACCESS_DEBUG, [](CClient *pClient, BotCommandArgs args)
{
	if ( pClient )
	{

		if ( args[0] && *args[0] )
		{
			UTIL_FindServerClassPrint(args[0]);
		}


		return COMMAND_ACCESSED;
	}

	return COMMAND_ERROR;
});

CBotCommandInline FindClassname("findclassname", CMD_ACCESS_DEBUG, [](CClient *pClient, BotCommandArgs args)
{
	if ( pClient )
	{

		if ( args[0] && *args[0] )
		{
			const char *pclass = CClassInterface::FindEntityNetClass(0,args[0]);

			if ( pclass )
				CBotGlobals::botMessage(pClient->getPlayer(),0,"%s network name = %s",args[0],pclass);
			else
				CBotGlobals::botMessage(pClient->getPlayer(),0,"%s network name not found",args[0],pclass);
		}


		return COMMAND_ACCESSED;
	}

	return COMMAND_ERROR;
});

CBotCommandInline FindProp("findprop", CMD_ACCESS_DEBUG, [](CClient *pClient, BotCommandArgs args)
{
	if ( args[0] && *args[0] )
	{
		UTIL_FindPropPrint(args[0]);
		return COMMAND_ACCESSED;
	}

	return COMMAND_ERROR;
}, "Usage: findprop <propname>");

#define MEMSEARCH_BYTE 1
#define MEMSEARCH_INT 2
#define MEMSEARCH_FLOAT 3
#define MEMSEARCH_STRING 4

#define MAX_MEM_SEARCH 8192

typedef union
{
	 struct
	 {
		  unsigned searched:1; // already searched
		  unsigned found:1; // offset found
		  unsigned unused:6;
	 }b1;

	 byte data;
}u_MEMSEARCH;

static u_MEMSEARCH stored_offsets[MAX_MEM_SEARCH];
static unsigned int m_size;

CBotCommandInline DebugMemoryScanCommand("memoryscan", CMD_ACCESS_DEBUG, [](CClient *pClient, BotCommandArgs args)
{
	//args[0] = classname
	// args[1] = value
	// args[2] = size in bytes
	// args[3] = want to remember offsets or not

	NEED_ARG(args[0])
	NEED_ARG(args[1])
	NEED_ARG(args[2])

	const unsigned int m_prev_size = m_size;

	if ( ( strcmp(args[2],"bool") == 0 ) || ( strcmp(args[2],"byte") == 0 ))
		m_size = MEMSEARCH_BYTE;
	else if ( strcmp(args[2],"int") == 0 )
		m_size = MEMSEARCH_INT;
	else if ( strcmp(args[2],"float") == 0 )
		m_size = MEMSEARCH_FLOAT;
	else if ( strcmp(args[2],"string") == 0 )
		m_size = MEMSEARCH_STRING;
	else 
		m_size = 0;

	if ( (m_prev_size != m_size) || ((m_size==0) || !args[3] || !*args[3]) || ( std::atoi(args[3]) == 0 ) )
	{
		memset(stored_offsets,0,sizeof(u_MEMSEARCH)*MAX_MEM_SEARCH);
	}


	// find edict
	edict_t *pEdict = CClassInterface::FindEntityByClassnameNearest(pClient->getOrigin(),args[0]);

	if ( pEdict  == nullptr)
	{
		CBotGlobals::botMessage(pClient->getPlayer(),0,"Edict not found");
		return COMMAND_ERROR;
	}

	// begin memory scan
	CBaseEntity *pent = pEdict->GetUnknown()->GetBaseEntity();

	byte *mempoint = reinterpret_cast<byte*>(pent);
	const byte value = static_cast<byte>(std::atoi(args[1]));
	const int ivalue = (std::atoi(args[1]));
	const float fvalue = (atof(args[1]));

	for ( int i = 0; i < MAX_MEM_SEARCH; i ++ ) // 2KB search
	{
		bool bfound = false;

		if ( m_size == MEMSEARCH_BYTE )
			bfound = (value == *mempoint);
		else if ( m_size == MEMSEARCH_INT )
			bfound = (ivalue == *reinterpret_cast<int*>(mempoint));
		else if ( m_size == MEMSEARCH_FLOAT )
			bfound = (fvalue == *reinterpret_cast<float*>(mempoint));
		else if ( m_size == MEMSEARCH_STRING )
		{
			try
			{
				const string_t *str = reinterpret_cast<string_t*>(mempoint);

				if ( str != nullptr)
				{			
					const char *pszstr = STRING(*str);

					if ( pszstr )
						bfound = ( strcmp(pszstr,args[1]) == 0 );
				}
			}
			catch(...)
			{
				CBotGlobals::botMessage(pClient->getPlayer(),0,"Invalid string");
			}
		}

		if ( bfound )
		{
			if ( !stored_offsets[i].b1.searched )					
				stored_offsets[i].b1.found = 1;				
		}
		else if ( stored_offsets[i].b1.searched )
			stored_offsets[i].b1.found = 0;

		stored_offsets[i].b1.searched = 1;

		mempoint++;
	}

	// Current valid offsets print
	for ( int i = 0; i < MAX_MEM_SEARCH; i ++ )
	{
		if ( stored_offsets[i].data != 0 )
		{
			if ( stored_offsets[i].b1.found )
				CBotGlobals::botMessage(pClient->getPlayer(),0,"%d",i);
		}
	}
	// 

	return COMMAND_ACCESSED;
}, "usage \"memoryscan <classname> <value> <type = 'bool/int/byte/float'> [store last = 1]\"");

CBotCommandInline DebugMemoryCheckCommand("memorycheck", CMD_ACCESS_DEBUG, [](CClient *pClient, BotCommandArgs args)
{
	// args[0] = classname
	// args[1] = offset
	// args[2] = type
	NEED_ARG(args[0])
	NEED_ARG(args[1])
	NEED_ARG(args[2])
	// find edict
	edict_t *pEdict = CClassInterface::FindEntityByClassnameNearest(pClient->getOrigin(),args[0]);
	
	if ( pEdict  == nullptr)
	{
		CBotGlobals::botMessage(pClient->getPlayer(),0,"Edict not found");
		return COMMAND_ERROR;
	}

	CBaseEntity *pent = pEdict->GetUnknown()->GetBaseEntity();

	const unsigned int offset = std::atoi(args[1]);

	if ( ( strcmp(args[2],"bool") == 0 ) || ( strcmp(args[2],"byte") == 0 ))
	{
		CBotGlobals::botMessage(pClient->getPlayer(),0,"%s - offset %d - Value(byte) = %d",args[0],offset,*reinterpret_cast<byte*>(reinterpret_cast<unsigned long>(pent) + offset));
	}
	else if ( strcmp(args[2],"int") == 0 )
	{
		CBotGlobals::botMessage(pClient->getPlayer(),0,"%s - offset %d - Value(int) = %d",args[0],offset,*reinterpret_cast<int*>(reinterpret_cast<unsigned long>(pent) + offset));
		/*
		if ( strcmp(args[0],"team_control_point_master") == 0 )
		{
			CTeamControlPointMaster *p;
			CTeamControlPointMaster check;

			unsigned int knownoffset = (unsigned int)&check.m_iCurrentRoundIndex - (unsigned int)&check;

			p = (CTeamControlPointMaster*)((((unsigned long)pent) + offset) - knownoffset); //MAP_CLASS(CTeamControlPoint,(((unsigned long)pent) + offset),knownoffset);
		}
		else if ( strcmp(args[0],"team_control_point") == 0 )
		{
			CTeamControlPoint *p = (CTeamControlPoint*)((((unsigned long)pent) + rcbot_const_point_offset.GetInt())); //MAP_CLASS(CTeamControlPoint,(((unsigned long)pent) + offset),knownoffset);
//			CTeamControlPointData *d = (CTeamControlPointData*)((((unsigned long)pent) + rcbot_const_point_data_offset.GetInt()));

			CBotGlobals::botMessage(NULL,0,"NULL MSG");
		}*/

	}
	else if ( strcmp(args[2],"float") == 0 )
		CBotGlobals::botMessage(pClient->getPlayer(),0,"%s - offset %d - Value(float) = %0.6f",args[0],offset,*reinterpret_cast<float*>(reinterpret_cast<unsigned long>(pent) + offset));
	else if ( strcmp(args[2],"string") == 0 )
	{
		const string_t *str = reinterpret_cast<string_t*>(reinterpret_cast<unsigned long>(pent) + offset);
		if ( str )
			CBotGlobals::botMessage(pClient->getPlayer(),0,"%s - offset %d - Value(string) = %s",args[0],offset,STRING(*str));
		else
			CBotGlobals::botMessage(pClient->getPlayer(),0,"%s - offset %d - INVALID string",args[0],offset);
	}
	else
		return COMMAND_ERROR;

	return COMMAND_ACCESSED;
}, "usage \"memorycheck <classname> <offset> <type>\"");

CBotCommandInline DebugMstrOffsetSearch("mstr_offset_search", CMD_ACCESS_DEBUG, [](CClient *pClient, BotCommandArgs args)
{
	if (strcmp("cp_dustbowl", STRING(gpGlobals->mapname)) != 0)
	{
		CBotGlobals::botMessage(pClient->getPlayer(), 0, "Command can only be used on cp_dustbowl -- change the map first");
		return COMMAND_ERROR;
	}

	edict_t *pMaster = CClassInterface::FindEntityByClassnameNearest(Vector(0, 0, 0), "team_control_point_master", 65535);

	if (pMaster == nullptr)
	{
		CBotGlobals::botMessage(pClient->getPlayer(), 0, "pMaster not found -- have you started the game yet?");
		return COMMAND_ERROR;
	}

	extern IServerGameEnts *servergameents;

	CBaseEntity *pMasterEntity = servergameents->EdictToBaseEntity(pMaster);

	//local variable is initialized but not referenced - [APG]RoboCop[CL]
	unsigned long full_size = sizeof(pMasterEntity);
	unsigned long offset = 800;

	while (offset < 1000)
	{
		const unsigned long mempoint = reinterpret_cast<unsigned long>(pMasterEntity) + offset;

		const CTeamControlPointMaster *PointMaster = reinterpret_cast<CTeamControlPointMaster*>(mempoint);

		try
		{
			if (PointMaster->m_iTeamBaseIcons[0] == 0 && PointMaster->m_iTeamBaseIcons[2] == 5 && PointMaster->m_iTeamBaseIcons[3] == 6)
			{
				if (strcmp(PointMaster->m_iszTeamBaseIcons[3].ToCStr(), "sprites/obj_icons/icon_base_blu") == 0)
				{
					CBotGlobals::botMessage(pClient->getPlayer(), 0, "pMaster offset is %d", offset);
					return COMMAND_ACCESSED;
				}
			}
		}
		catch (...)
		{
			// SEH handling 
		}

		offset++;
	}
	return COMMAND_ACCESSED;
}, "usage \"mstr_offset_search\" must be run on cp_dustbowl only");

CBotSubcommands DebugSubcommands("debug", CMD_ACCESS_DEBUG | CMD_ACCESS_DEDICATED, {
	&DebugGameEventCommand,
	&DebugBotCommand,
	&DebugNavCommand,
	&DebugVisCommand,
	&DebugThinkCommand,
	&DebugLookCommand,
	&DebugHudCommand,
	&DebugAimCommand,
	&DebugChatCommand,
	&BotGoto,
	&BotFlush,
	&DebugTaskCommand,
	&BotTaskCommand,
	&DebugButtonsCommand,
	&DebugSpeedCommand,
	&DebugUsercmdCommand,
	&DebugUtilCommand,
	&DebugProfilingCommand,
	&DebugEdictsCommand,
	&PrintProps,
	&GetProp,
	&SetProp,
	&FindClass,
	&FindClassname,
	&FindProp,
	&DebugMemoryScanCommand,
	&DebugMemoryCheckCommand,
	&DebugMstrOffsetSearch,
});
