// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * ======================================================
 * Metamod:Source Sample Plugin
 * Written by AlliedModders LLC.
 * ======================================================
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from 
 * the use of this software.
 *
 * This sample plugin is public domain.
 */

#include <cstdio>

#pragma push_macro("clamp") //Fix for C++17 [APG]RoboCop[CL]
#undef clamp
#include <algorithm>
#pragma pop_macro("clamp")

#include "bot_plugin_meta.h"

#include "filesystem.h"
#include "interface.h"
#include "IStaticPropMgr.h"

#ifndef SM_EXT
#include "IEffects.h"
#endif

#ifdef __linux__
#include "shake.h"    //bir3yk
#endif

#include "const.h"  // For FL_FAKECLIENT

// Always include ndebugoverlay.h for debug overlay support (includes null stub)
#include "ndebugoverlay.h"
#include "irecipientfilter.h"

#include "bot_cvars.h"

#if defined(_WIN64) || defined(_WIN32)
	#include <ctime>
#endif

// for IServerTools
#include "bot.h"
#include "bot_configfile.h"
#include "bot_globals.h"
#include "bot_profile.h"
#include "bot_menu.h"
#include "bot_getprop.h"
#include "bot_event.h"
#include "bot_profiling.h"
#include "bot_wpt_dist.h"
#include "bot_squads.h"
#include "bot_accessclient.h"
#include "bot_weapons.h"
#include "bot_waypoint_visibility.h"
#include "bot_kv.h"
#include "bot_sigscan.h"
//#include "bot_mods.h"
#include "bot_door.h"
#include "bot_navtest.h"
#include "bot_tactical.h"
#include "bot_gravity.h"

#include "tier0/icommandline.h"

#include "rcbot/logging.h"

#include <cstring>

#include <build_info.h>
#include <sourcehook.h>

#ifdef SM_EXT
#include "rcbot/entprops.h"
#endif
SH_DECL_HOOK6(IServerGameDLL, LevelInit, SH_NOATTRIB, 0, bool, char const *, char const *, char const *, char const *, bool, bool);
SH_DECL_HOOK3_void(IServerGameDLL, ServerActivate, SH_NOATTRIB, 0, edict_t *, int, int);
SH_DECL_HOOK1_void(IServerGameDLL, GameFrame, SH_NOATTRIB, 0, bool);
SH_DECL_HOOK0_void(IServerGameDLL, LevelShutdown, SH_NOATTRIB, 0);
SH_DECL_HOOK2_void(IServerGameClients, ClientActive, SH_NOATTRIB, 0, edict_t *, bool);
SH_DECL_HOOK1_void(IServerGameClients, ClientDisconnect, SH_NOATTRIB, 0, edict_t *);
SH_DECL_HOOK2_void(IServerGameClients, ClientPutInServer, SH_NOATTRIB, 0, edict_t *, char const *);
SH_DECL_HOOK1_void(IServerGameClients, SetCommandClient, SH_NOATTRIB, 0, int);
SH_DECL_HOOK5(IServerGameClients, ClientConnect, SH_NOATTRIB, 0, bool, edict_t *, const char*, const char *, char *, int);
SH_DECL_HOOK2(IGameEventManager2, FireEvent, SH_NOATTRIB, 0, bool, IGameEvent *, bool);

#if SOURCE_ENGINE >= SE_ORANGEBOX
SH_DECL_HOOK2_void(IServerGameClients, NetworkIDValidated, SH_NOATTRIB, 0, const char *, const char *);
SH_DECL_HOOK2_void(IServerGameClients, ClientCommand, SH_NOATTRIB, 0, edict_t *, const CCommand &);
#else
SH_DECL_HOOK1_void(IServerGameClients, ClientCommand, SH_NOATTRIB, 0, edict_t *);
#endif

SH_DECL_MANUALHOOK2_void(MHook_PlayerRunCmd, 0, 0, 0, CUserCmd*, IMoveHelper*); 

IServerGameDLL *server = nullptr;
IGameEventManager2 *gameevents = nullptr;
IServerPluginCallbacks *vsp_callbacks = nullptr;
ICvar *icvar = nullptr;
IVEngineServer *engine = nullptr;  // helper functions (messaging clients, loading content, making entities, running commands, etc)
IFileSystem *filesystem = nullptr;  // file I/O 
IGameEventManager2 *gameeventmanager = nullptr;
IGameEventManager *gameeventmanager1 = nullptr;  // game events interface
IPlayerInfoManager *playerinfomanager = nullptr;  // game dll interface to interact with players
IServerPluginHelpers *helpers = nullptr;  // special 3rd party plugin helpers from the engine
IServerGameClients* gameclients = nullptr;
IEngineTrace *enginetrace = nullptr;
#ifndef SM_EXT
IEffects *g_pEffects = nullptr;
#endif
IBotManager *g_pBotManager = nullptr;
CGlobalVars *gpGlobals = nullptr;
IVDebugOverlay *debugoverlay = nullptr;
CNullDebugOverlay g_NullDebugOverlay;
IServerGameEnts *servergameents = nullptr; // for accessing the server game entities
IServerGameDLL *servergamedll = nullptr;
IServerTools *servertools = nullptr;
IStaticPropMgrServer* staticpropmgr = nullptr;

RCBotPluginMeta g_RCBotPluginMeta;

PLUGIN_EXPOSE(RCBotPluginMeta, g_RCBotPluginMeta);

static ConVar rcbot2_ver_cvar("rcbot_ver", build_info::long_version, FCVAR_REPLICATED, "RCBot version");

CON_COMMAND(rcbotd, "access the bot commands on a server")
{
	if (!engine->IsDedicatedServer() || !CBotGlobals::IsMapRunning())
	{
		logger->Log(LogLevel::ERROR, "Error, no map running or not dedicated server");
		return;
	}

	// shift args and call subcommand
	BotCommandArgs argList;
	for (unsigned i = 1; i <= static_cast<unsigned>(args.ArgC()); i++) {
		argList.emplace_back(args.Arg(static_cast<int>(i)));
	}
	const eBotCommandResult iResult = CBotGlobals::m_pCommands->execute(nullptr, argList);

	if (iResult == COMMAND_ACCESSED)
	{
		// ok
	}
	else if (iResult == COMMAND_REQUIRE_ACCESS)
	{
		logger->Log(LogLevel::ERROR, "You do not have access to this command");
	}
	else if (iResult == COMMAND_NOT_FOUND)
	{
		logger->Log(LogLevel::ERROR, "bot command not found");
	}
	else if (iResult == COMMAND_ERROR)
	{
		logger->Log(LogLevel::ERROR, "bot command returned an error");
	}
}

// Recipient filter classes for HUD messages (used by both SM_EXT and non-SM_EXT builds)
class CBotRecipientFilter : public IRecipientFilter
{
public:
	CBotRecipientFilter(const edict_t *pPlayer)
	{
		m_iPlayerSlot = ENTINDEX(pPlayer);
	}

	bool IsReliable() const override { return false; }
	bool IsInitMessage() const override { return false; }

	int	GetRecipientCount() const override { return 1; }
	int	GetRecipientIndex(int slot) const override { return m_iPlayerSlot; }

private:
	int m_iPlayerSlot;
};

class CClientBroadcastRecipientFilter : public IRecipientFilter
{
public:

	CClientBroadcastRecipientFilter() : m_iMaxCount(0) {

		// Initialize m_iPlayerSlot with a default value, e.g., -1
		std::fill(std::begin(m_iPlayerSlot), std::end(m_iPlayerSlot), -1);

		for (int i = 0; i < RCBOT_MAXPLAYERS; ++i) {
			CClient* client = CClients::get(i);

			if (client->isUsed()) {
				IPlayerInfo* p = playerinfomanager->GetPlayerInfo(client->getPlayer());

				if (p->IsConnected() && !p->IsFakeClient()) {
					m_iPlayerSlot[m_iMaxCount] = i;
					m_iMaxCount++;
				}
			}
		}
	}

	bool IsReliable() const override { return false; }
	bool IsInitMessage() const override { return false; }

	int	GetRecipientCount() const override { return m_iMaxCount; }
	int	GetRecipientIndex(const int slot) const override { return m_iPlayerSlot[slot] + 1; }

private:

	int m_iMaxCount;
	int m_iPlayerSlot[RCBOT_MAXPLAYERS];
};

///////////////
// hud message
///////////////
void RCBotPluginMeta::HudTextMessage(const edict_t* pEntity, const char* szMessage)
{
	int msgid = 0;
	int imsgsize = 0;
	char msgbuf[64];
	bool bOK; //Unused? [APG]RoboCop[CL]

	int hint = -1;
	int say = -1;

	while ((bOK = servergamedll->GetUserMessageInfo(msgid, msgbuf, 63, imsgsize)) == true)
	{
		if (std::strcmp(msgbuf, "HintText") == 0)
			hint = msgid;
		else if (std::strcmp(msgbuf, "SayText") == 0)
			say = msgid;

		msgid++;
	}

	if (msgid == 0)
		return;

	// if (!bOK)
	// return;

	CBotRecipientFilter filter(pEntity);

	bf_write* buf;

	if (hint > 0) {
		buf = engine->UserMessageBegin(&filter, hint);
		buf->WriteString(szMessage);
		engine->MessageEnd();
	}

	if (say > 0) {
		char chatline[128];
		snprintf(chatline, sizeof(chatline), "\x01\x04[RCBot2]\x01 %s\n", szMessage);

		buf = engine->UserMessageBegin(&filter, say);
		buf->WriteString(chatline);
		engine->MessageEnd();
	}
}

//////////////////////////
// chat broadcast message
//////////////////////////
void RCBotPluginMeta::BroadcastTextMessage(const char* szMessage)
{
	int msgid = 0;
	int imsgsize = 0;
	char msgbuf[64];
	bool bOK; //Unused? [APG]RoboCop[CL]

	int hint = -1; //Unused? [APG]RoboCop[CL]
	int say = -1;

	while ((bOK = servergamedll->GetUserMessageInfo(msgid, msgbuf, 63, imsgsize)) == true)
	{
		if (std::strcmp(msgbuf, "HintText") == 0)
			hint = msgid;
		else if (std::strcmp(msgbuf, "SayText") == 0)
			say = msgid;

		msgid++;
	}

	if (msgid == 0)
		return;

	CClientBroadcastRecipientFilter filter; // Allocate on stack
	if (say > 0) {
		char chatline[128];
		snprintf(chatline, sizeof(chatline), "\x01\x04[RCBot2]\x01 %s\n", szMessage);

		bf_write* buf = engine->UserMessageBegin(&filter, say);
		buf->WriteString(chatline);
		engine->MessageEnd();
	}
}

void RCBotPluginMeta::Hook_PlayerRunCmd(CUserCmd *ucmd, IMoveHelper *moveHelper)
{
	static CBot *pBot;

	CBaseEntity *pPlayer = META_IFACEPTR(CBaseEntity);

	const edict_t *pEdict = servergameents->BaseEntityToEdict(pPlayer);

	pBot = CBots::getBotPointer(pEdict);
	
	if ( pBot )
	{
		static CBotCmd *cmd;
		
		cmd = pBot->getUserCMD();

		// put the bot's commands into this move frame
		ucmd->buttons = cmd->buttons;
		ucmd->forwardmove = cmd->forwardmove;
		ucmd->impulse = cmd->impulse;
		ucmd->sidemove = cmd->sidemove;
		ucmd->upmove = cmd->upmove;
		ucmd->viewangles = cmd->viewangles;
		ucmd->weaponselect = cmd->weaponselect;
		ucmd->weaponsubtype = cmd->weaponsubtype;
		ucmd->tick_count = cmd->tick_count;
		ucmd->command_number = cmd->command_number;
	}
	RETURN_META(MRES_IGNORED);
}

/** 
 * Something like this is needed to register cvars/CON_COMMANDs.
 */
class BaseAccessor : public IConCommandBaseAccessor
{
public:
	virtual ~BaseAccessor() = default;

	bool RegisterConCommandBase(ConCommandBase *pCommandBase) override
	{
		/* Always call META_REGCVAR instead of going through the engine. */
		return META_REGCVAR(pCommandBase);
	}
} s_BaseAccessor;

bool RCBotPluginMeta::Load(PluginId id, ISmmAPI *ismm, char *error, std::size_t maxlen, bool late)
{
	extern MTRand_int32 irand;

	PLUGIN_SAVEVARS()

	GET_V_IFACE_CURRENT(GetEngineFactory, enginetrace, IEngineTrace, INTERFACEVERSION_ENGINETRACE_SERVER)
	GET_V_IFACE_CURRENT(GetEngineFactory, engine, IVEngineServer, INTERFACEVERSION_VENGINESERVER)
	GET_V_IFACE_CURRENT(GetEngineFactory, gameevents, IGameEventManager2, INTERFACEVERSION_GAMEEVENTSMANAGER2)
	GET_V_IFACE_CURRENT(GetEngineFactory, helpers, IServerPluginHelpers, INTERFACEVERSION_ISERVERPLUGINHELPERS)
	GET_V_IFACE_CURRENT(GetEngineFactory, icvar, ICvar, CVAR_INTERFACE_VERSION)
	GET_V_IFACE_CURRENT(GetEngineFactory, staticpropmgr, IStaticPropMgrServer, INTERFACEVERSION_STATICPROPMGR_SERVER)

	GET_V_IFACE_ANY(GetEngineFactory, filesystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION)

	GET_V_IFACE_ANY(GetServerFactory, servergameents, IServerGameEnts, INTERFACEVERSION_SERVERGAMEENTS)
	GET_V_IFACE_ANY(GetServerFactory, server, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL)
	GET_V_IFACE_ANY(GetServerFactory, gameclients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS)
	GET_V_IFACE_ANY(GetServerFactory, playerinfomanager, IPlayerInfoManager, INTERFACEVERSION_PLAYERINFOMANAGER)

#ifndef SM_EXT
	GET_V_IFACE_ANY(GetServerFactory, g_pEffects, IEffects, IEFFECTS_INTERFACE_VERSION)
#endif
	GET_V_IFACE_ANY(GetServerFactory, g_pBotManager, IBotManager, INTERFACEVERSION_PLAYERBOTMANAGER)
	GET_V_IFACE_ANY(GetServerFactory, servertools, IServerTools, VSERVERTOOLS_INTERFACE_VERSION)

	// Debug overlay is optional - older engines may not have it
	debugoverlay = static_cast<IVDebugOverlay*>(ismm->VInterfaceMatch(ismm->GetEngineFactory(), VDEBUG_OVERLAY_INTERFACE_VERSION, 0));
	if (debugoverlay == nullptr)
	{
		META_LOG(g_PLAPI, "Warning: IVDebugOverlay interface not available, using null stub");
		debugoverlay = &g_NullDebugOverlay;
	}

	GET_V_IFACE_ANY(GetServerFactory, servergamedll, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL)
	GET_V_IFACE_ANY(GetServerFactory, gameclients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS)

	gpGlobals = ismm->GetCGlobals();

	META_LOG(g_PLAPI, "Starting plugin.");
	fprintf(stderr, "[RCBOT2-DIAG] Step 1: Starting plugin init\n");
	fflush(stderr);

	/* Load the VSP listener.  This is usually needed for IServerPluginHelpers. */
	fprintf(stderr, "[RCBOT2-DIAG] Step 2: Adding listener...\n");
	fflush(stderr);
	ismm->AddListener(this, this);
	fprintf(stderr, "[RCBOT2-DIAG] Step 2: Listener added\n");
	fflush(stderr);
	if ((vsp_callbacks = ismm->GetVSPInfo(nullptr)) == nullptr)
	{
		fprintf(stderr, "[RCBOT2-DIAG] Step 2: Enabling VSP listener...\n");
		fflush(stderr);
		ismm->EnableVSPListener();
	}
	fprintf(stderr, "[RCBOT2-DIAG] Step 2: VSP setup complete\n");
	fflush(stderr);

	fprintf(stderr, "[RCBOT2-DIAG] Step 3: Adding SourceHook hooks...\n");
	fflush(stderr);
	fflush(stdout);
	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, LevelInit, server, this, &RCBotPluginMeta::Hook_LevelInit, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, ServerActivate, server, this, &RCBotPluginMeta::Hook_ServerActivate, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, GameFrame, server, this, &RCBotPluginMeta::Hook_GameFrame, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, LevelShutdown, server, this, &RCBotPluginMeta::Hook_LevelShutdown, false);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientActive, gameclients, this, &RCBotPluginMeta::Hook_ClientActive_Pre, false);  // Pre-hook to fix FL_FAKECLIENT
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientActive, gameclients, this, &RCBotPluginMeta::Hook_ClientActive, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientDisconnect, gameclients, this, &RCBotPluginMeta::Hook_ClientDisconnect, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientPutInServer, gameclients, this, &RCBotPluginMeta::Hook_ClientPutInServer, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientConnect, gameclients, this, &RCBotPluginMeta::Hook_ClientConnect, false);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientCommand, gameclients, this, &RCBotPluginMeta::Hook_ClientCommand, false);
	//Hook FireEvent to our function - unstable for TF2? [APG]RoboCop[CL]
	SH_ADD_HOOK_MEMFUNC(IGameEventManager2, FireEvent, gameevents, this, &RCBotPluginMeta::FireGameEvent, false);
	fprintf(stderr, "[RCBOT2-DIAG] Step 3: SourceHook hooks added\n");
	fflush(stderr);

#if SOURCE_ENGINE >= SE_ORANGEBOX
	fprintf(stderr, "[RCBOT2-DIAG] Step 4: Registering ConVars...\n");
	fflush(stderr);
	g_pCVar = icvar;
	ConVar_Register(0, &s_BaseAccessor);
	fprintf(stderr, "[RCBOT2-DIAG] Step 4: ConVars registered\n");
	fflush(stderr);
#else
	ConCommandBaseMgr::OneTimeInit(&s_BaseAccessor);
#endif

#if SOURCE_ENGINE!=SE_DARKMESSIAH
	// read loglevel from startup param for early logging
	fprintf(stderr, "[RCBOT2-DIAG] Step 5: Reading rcbot_loglevel...\n");
	fflush(stderr);
	ConVarRef rcbot_loglevel("rcbot_loglevel");
	fprintf(stderr, "[RCBOT2-DIAG] Step 5: Calling CommandLine()...\n");
	fflush(stderr);
	void* cmdLine = CommandLine();
	fprintf(stderr, "[RCBOT2-DIAG] Step 5: CommandLine() returned %p\n", cmdLine);
	fflush(stderr);
	if (cmdLine != nullptr) {
		rcbot_loglevel.SetValue(CommandLine()->ParmValue("+rcbot_loglevel", rcbot_loglevel.GetInt()));
	}
	fprintf(stderr, "[RCBOT2-DIAG] Step 5: Loglevel setup complete\n");
	fflush(stderr);
#endif

	// Read Signatures and Offsets
	fprintf(stderr, "[RCBOT2-DIAG] Step 6: Initializing mod folder...\n");
	fflush(stderr);
	CBotGlobals::initModFolder();
	fprintf(stderr, "[RCBOT2-DIAG] Step 6: Mod folder initialized\n");
	fflush(stderr);

	fprintf(stderr, "[RCBOT2-DIAG] Step 7: Reading RCBot folder...\n");
	fflush(stderr);
	CBotGlobals::readRCBotFolder();
	fprintf(stderr, "[RCBOT2-DIAG] Step 7: RCBot folder read\n");
	fflush(stderr);

	char filename[512];
	// Load RCBOT2 hook data
	fprintf(stderr, "[RCBOT2-DIAG] Step 8: Building hookinfo filename...\n");
	fflush(stderr);
	CBotGlobals::buildFileName(filename, "hookinfo", BOT_CONFIG_FOLDER, "ini");
	fprintf(stderr, "[RCBOT2-DIAG] Step 8: Hook file: %s\n", filename);
	fflush(stderr);

	std::fstream fp(filename, std::fstream::in);

	CRCBotKeyValueList kvl;

	if (fp)
		kvl.parseFile(fp);

	void *gameServerFactory = reinterpret_cast<void*>(ismm->GetServerFactory(false));

	int val;

#if defined(_WIN64) || defined(_WIN32)
	if (kvl.getInt("runplayermove_dods_win", &val))
		rcbot_runplayercmd_dods.SetValue(val);
	if (kvl.getInt("gamerules_win", &val))
		rcbot_gamerules_offset.SetValue(val);
	if (kvl.getInt("runplayermove_synergy_win", &val))
		rcbot_runplayercmd_syn.SetValue(val);
	if (kvl.getInt("getdatadescmap_win", &val))
		rcbot_datamap_offset.SetValue(val);
#else
	if (kvl.getInt("runplayermove_dods_linux", &val))
		rcbot_runplayercmd_dods.SetValue(val);
	if (kvl.getInt("runplayermove_synergy_linux", &val))
		rcbot_runplayercmd_syn.SetValue(val);
	if (kvl.getInt("getdatadescmap_linux", &val))
		rcbot_datamap_offset.SetValue(val);
#endif

	fprintf(stderr, "[RCBOT2-DIAG] Step 9: Creating GameRules objects...\n");
	fflush(stderr);
	g_pGameRules_Obj = new CGameRulesObject(kvl, gameServerFactory);
	g_pGameRules_Create_Obj = new CCreateGameRulesObject(kvl, gameServerFactory);
	fprintf(stderr, "[RCBOT2-DIAG] Step 9: GameRules objects created\n");
	fflush(stderr);

	if (fp)
		fp.close();

	fprintf(stderr, "[RCBOT2-DIAG] Step 10: Calling CBotGlobals::gameStart()...\n");
	fflush(stderr);
	if (!CBotGlobals::gameStart())
	{
		fprintf(stderr, "[RCBOT2-DIAG] Step 10: gameStart() FAILED!\n");
		fflush(stderr);
		return false;
	}
	fprintf(stderr, "[RCBOT2-DIAG] Step 10: gameStart() succeeded\n");
	fflush(stderr);

	CBotMod *pMod = CBotGlobals::getCurrentMod(); // `*pMod` Unused? [APG]RoboCop[CL]
	fprintf(stderr, "[RCBOT2-DIAG] Step 11: Current mod pointer: %p\n", static_cast<void*>(pMod));
	fflush(stderr);

#ifdef OVERRIDE_RUNCMD
	// TODO figure out a more robust gamedata fix instead of vtable
	#if SOURCE_ENGINE == SE_DODS
	SH_MANUALHOOK_RECONFIGURE(MHook_PlayerRunCmd, rcbot_runplayercmd_dods.GetInt(), 0, 0);
	#elif SOURCE_ENGINE == SE_SDK2013
	if(pMod->getModId() == MOD_SYNERGY)
	{
		SH_MANUALHOOK_RECONFIGURE(MHook_PlayerRunCmd, rcbot_runplayercmd_syn.GetInt(), 0, 0);
	}
#endif

#endif

	fprintf(stderr, "[RCBOT2-DIAG] Step 12: Calling ENGINE_CALL(LogPrint)...\n");
	fflush(stderr);
	ENGINE_CALL(LogPrint)("All hooks started!\n");
	fprintf(stderr, "[RCBOT2-DIAG] Step 12: ENGINE_CALL(LogPrint) complete\n");
	fflush(stderr);

	//MathLib_Init( 2.2f, 2.2f, 0.0f, 2.0f );
	//ConVar_Register( 0 );
	//InitCVars( interfaceFactory ); // register any cvars we have defined

	fprintf(stderr, "[RCBOT2-DIAG] Step 13: Seeding RNG...\n");
	fflush(stderr);
	std::srand( static_cast<unsigned>(time(nullptr)) );  // initialize the random seed
	MTRand_int32::seed( static_cast<unsigned>(time(nullptr)) );
	fprintf(stderr, "[RCBOT2-DIAG] Step 13: RNG seeded\n");
	fflush(stderr);

	// Find the RCBOT2 Path from metamod VDF
	extern IFileSystem* filesystem;
	fprintf(stderr, "[RCBOT2-DIAG] Step 14: Loading rcbot2.vdf (filesystem=%p)...\n", static_cast<void*>(filesystem));
	fflush(stderr);
	KeyValues* mainkv = new KeyValues("metamodplugin");
	fprintf(stderr, "[RCBOT2-DIAG] Step 14: KeyValues created\n");
	fflush(stderr);

	const char* rcbot2path; //Unused? [APG]RoboCop[CL]
	logger->Log(LogLevel::INFO, "Reading rcbot2 path from VDF...");

	fprintf(stderr, "[RCBOT2-DIAG] Step 14: Calling LoadFromFile...\n");
	fflush(stderr);
	mainkv->LoadFromFile(filesystem, "addons/metamod/rcbot2.vdf", "MOD");
	fprintf(stderr, "[RCBOT2-DIAG] Step 14: LoadFromFile done, calling FindKey...\n");
	fflush(stderr);
	KeyValues* temp = mainkv->FindKey("Metamod Plugin");
	fprintf(stderr, "[RCBOT2-DIAG] Step 14: KeyValues loaded, temp=%p\n", static_cast<void*>(temp));
	fflush(stderr);

	if (temp)
		rcbot2path = temp->GetString("rcbot2path", "\0");

	mainkv->deleteThis(); //mainkv possible redundant? [APG]RoboCop[CL]
	mainkv = temp; // Memory leak fix [APG]RoboCop[CL]

	//eventListener2 = new CRCBotEventListener();

	// Initialize bot variables
	fprintf(stderr, "[RCBOT2-DIAG] Step 15: Setting up bot profiles...\n");
	fflush(stderr);
	CBotProfiles::setupProfiles();
	fprintf(stderr, "[RCBOT2-DIAG] Step 15: Bot profiles setup complete\n");
	fflush(stderr);


	//CBotEvents::setupEvents();
	fprintf(stderr, "[RCBOT2-DIAG] Step 16: Setting up waypoint types...\n");
	fflush(stderr);
	CWaypointTypes::setup();
	fprintf(stderr, "[RCBOT2-DIAG] Step 16: Waypoint types setup complete\n");
	fflush(stderr);

	fprintf(stderr, "[RCBOT2-DIAG] Step 17: Setting up waypoint visibility...\n");
	fflush(stderr);
	CWaypoints::setupVisibility();
	fprintf(stderr, "[RCBOT2-DIAG] Step 17: Waypoint visibility setup complete\n");
	fflush(stderr);

	fprintf(stderr, "[RCBOT2-DIAG] Step 18: Loading bot config file...\n");
	fflush(stderr);
	CBotConfigFile::reset();
	CBotConfigFile::load();
	fprintf(stderr, "[RCBOT2-DIAG] Step 18: Bot config file loaded\n");
	fflush(stderr);

	fprintf(stderr, "[RCBOT2-DIAG] Step 19: Setting up menus...\n");
	fflush(stderr);
	CBotMenuList::setupMenus();
	fprintf(stderr, "[RCBOT2-DIAG] Step 19: Menus setup complete\n");
	fflush(stderr);

	//CRCBotPlugin::ShowLicense();

	//RandomSeed((unsigned)time(NULL));

	fprintf(stderr, "[RCBOT2-DIAG] Step 20: Initializing class interface...\n");
	fflush(stderr);
	CClassInterface::init();
	fprintf(stderr, "[RCBOT2-DIAG] Step 20: Class interface initialized\n");
	fflush(stderr);

	fprintf(stderr, "[RCBOT2-DIAG] Step 21: Setting up RCBOT2 CVars...\n");
	fflush(stderr);
	RCBOT2_Cvar_setup(g_pCVar);
	fprintf(stderr, "[RCBOT2-DIAG] Step 21: CVars setup complete\n");
	fflush(stderr);

	// Bot Quota Settings
	fprintf(stderr, "[RCBOT2-DIAG] Step 22: Loading bot quota settings...\n");
	fflush(stderr);
	char bq_line[128];

	int bot_count = 0;
	int human_count = 0;

	for (int& m_iTargetBot : m_iTargetBots)
	{
		m_iTargetBot = 0;
	}

	CBotGlobals::buildFileName(filename, "bot_quota", BOT_CONFIG_FOLDER, "ini");
	fp = std::fstream(filename, std::fstream::in);

	std::memset(bq_line, 0, sizeof(bq_line));

	if (fp) {
		while (fp.getline(bq_line, sizeof(bq_line))) {
			if (bq_line[0] == '#')
				continue;

			for (char& i : bq_line)
			{
				if (i == '\0')
					break;

				if (!isdigit(i))
					i = ' ';
			}

			if (std::sscanf(bq_line, "%d %d", &human_count, &bot_count) == 2) {
				if (human_count < 0 || human_count > RCBOT_MAXPLAYERS) {
					logger->Log(LogLevel::WARN, "Bot Quota - Invalid Human Count %d", human_count);
					continue;
				}

				if (bot_count < 0 || bot_count > RCBOT_MAXPLAYERS) {
					logger->Log(LogLevel::WARN, "Bot Quota - Invalid Bot Count %d", bot_count);
					continue;
				}

				if (human_count < RCBOT_MAXPLAYERS) { // Ensure human_count is within bounds
					m_iTargetBots[human_count] = bot_count;
					logger->Log(LogLevel::INFO, "Bot Quota - Humans: %d, Bots: %d", human_count, bot_count);
				}
			}
		}
	}
	fprintf(stderr, "[RCBOT2-DIAG] Step 22: Bot quota loaded\n");
	fflush(stderr);

	fprintf(stderr, "[RCBOT2-DIAG] ========================================\n");
	fprintf(stderr, "[RCBOT2-DIAG] Plugin initialization COMPLETE!\n");
	fprintf(stderr, "[RCBOT2-DIAG] ========================================\n");
	fflush(stderr);

	return true;
}

bool RCBotPluginMeta::FireGameEvent(IGameEvent * pevent, bool bDontBroadcast)
{
	CBotEvents::executeEvent(pevent,TYPE_IGAMEEVENT);

	RETURN_META_VALUE(MRES_IGNORED, true);
}

bool RCBotPluginMeta::Unload(char *error, std::size_t maxlen)
{
#ifdef SM_EXT
	SM_UnloadExtension();
#endif
	
	//CBots::kickRandomBot(RCBOT_MAXPLAYERS); //breaks the bot quota system? [APG]RoboCop[CL]
	
	SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, LevelInit, server, this, &RCBotPluginMeta::Hook_LevelInit, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, ServerActivate, server, this, &RCBotPluginMeta::Hook_ServerActivate, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, GameFrame, server, this, &RCBotPluginMeta::Hook_GameFrame, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, LevelShutdown, server, this, &RCBotPluginMeta::Hook_LevelShutdown, false);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientActive, gameclients, this, &RCBotPluginMeta::Hook_ClientActive_Pre, false);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientActive, gameclients, this, &RCBotPluginMeta::Hook_ClientActive, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientDisconnect, gameclients, this, &RCBotPluginMeta::Hook_ClientDisconnect, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientPutInServer, gameclients, this, &RCBotPluginMeta::Hook_ClientPutInServer, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientConnect, gameclients, this, &RCBotPluginMeta::Hook_ClientConnect, false);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientCommand, gameclients, this, &RCBotPluginMeta::Hook_ClientCommand, false);
	
	//SH_REMOVE_MANUALHOOK(MHook_PlayerRunCmd, player_vtable, SH_STATIC(Hook_Function2), false);

	// if another instance is running dont run through this
	//if ( !bInitialised )
	//	return;
	
	CBots::freeAllMemory();
	CStrings::freeAllMemory();
	CBotMods::freeMemory();
	CAccessClients::freeMemory();
	CBotEvents::freeMemory();
	CWaypoints::freeMemory();
	CWaypointTypes::freeMemory();
	CBotProfiles::deleteProfiles();
	CWeapons::freeMemory();
	CBotMenuList::freeMemory();
	//unloadSignatures();

	//UnhookPlayerRunCommand();
	//UnhookGiveNamedItem();

	//ConVar_Unregister();

	//if ( gameevents )
	//	gameevents->RemoveListener(this);

	ConVar_Unregister( );

	return true;
}

void RCBotPluginMeta::OnVSPListening(IServerPluginCallbacks *iface)
{
	vsp_callbacks = iface;
}

void RCBotPluginMeta::Hook_ServerActivate(edict_t *pEdictList, const int edictCount, const int clientMax)
{
	META_LOG(g_PLAPI, "ServerActivate() called: edictCount = %d, clientMax = %d", edictCount, clientMax);

	CAccessClients::load();

	CBotGlobals::setClientMax(clientMax);
}

void RCBotPluginMeta::AllPluginsLoaded()
{
	/* This is where we'd do stuff that relies on the mod or other plugins 
	 * being initialized (for example, cvars added and events registered).
	 */
#ifdef SM_EXT
	BindToSourcemod();
#endif
}

#ifdef SM_EXT
void* RCBotPluginMeta::OnMetamodQuery(const char* iface, int *ret) {
	if (std::strcmp(iface, SOURCEMOD_NOTICE_EXTENSIONS) == 0) {
		BindToSourcemod();
	}
	
	if (ret != nullptr) {
		*ret = IFACE_OK;
	}

	return nullptr;
}
#endif

// Pre-hook for ClientActive - runs BEFORE the game processes the client
// This is where we fix the FL_FAKECLIENT flag if it's missing
void RCBotPluginMeta::Hook_ClientActive_Pre(edict_t *pEntity, const bool bLoadGame)
{
	if (!pEntity || pEntity->IsFree())
		return;

	// Check if FL_FAKECLIENT flag is missing and fix it
	int currentFlags = CClassInterface::getFlags(pEntity);

	// If this looks like it should be a bot (no FL_CLIENT set or name suggests bot)
	// but FL_FAKECLIENT is not set, fix it
	if (!(currentFlags & FL_FAKECLIENT))
	{
		IPlayerInfo *pInfo = playerinfomanager->GetPlayerInfo(pEntity);
		if (pInfo)
		{
			const char* name = pInfo->GetName();
			// Check if this appears to be a bot we're creating
			// Bots typically have names like "RCBot" or are created without proper fake client flag
			// We can also check if IBotController is available
			IBotController* pController = g_pBotManager->GetBotController(pEntity);

			fprintf(stderr, "[RCBOT2] Hook_ClientActive_Pre: name='%s', flags=0x%x, FL_FAKECLIENT=%s, controller=%s\n",
				name, currentFlags,
				(currentFlags & FL_FAKECLIENT) ? "YES" : "NO",
				pController ? "VALID" : "NULL");

			// If the controller is NULL but this seems to be a bot (check by name pattern)
			// or if we're in the middle of creating a bot, set the flag
			if (name && (strstr(name, "Bot") || strstr(name, "bot") || strstr(name, "RCBot")))
			{
				fprintf(stderr, "[RCBOT2] Hook_ClientActive_Pre: Fixing FL_FAKECLIENT flag!\n");
				CClassInterface::addFlags(pEntity, FL_FAKECLIENT | FL_CLIENT);

				// Verify the fix worked
				int newFlags = CClassInterface::getFlags(pEntity);
				fprintf(stderr, "[RCBOT2] Hook_ClientActive_Pre: After fix, flags=0x%x, FL_FAKECLIENT=%s\n",
					newFlags, (newFlags & FL_FAKECLIENT) ? "YES" : "NO");
			}
		}
	}
}

void RCBotPluginMeta::Hook_ClientActive(edict_t *pEntity, const bool bLoadGame)
{
	META_LOG(g_PLAPI, "Hook_ClientActive(%d, %d)", IndexOfEdict(pEntity), bLoadGame);

	// Diagnostic logging
	int slot = IndexOfEdict(pEntity);
	IPlayerInfo *pInfo = playerinfomanager->GetPlayerInfo(pEntity);
	if (pInfo)
	{
		fprintf(stderr, "[RCBOT2] Hook_ClientActive(%d): name='%s', team=%d, isFakeClient=%d\n",
			slot, pInfo->GetName(), pInfo->GetTeamIndex(), pInfo->IsFakeClient() ? 1 : 0);
	}
	else
	{
		fprintf(stderr, "[RCBOT2] Hook_ClientActive(%d): pInfo is NULL\n", slot);
	}

	CClients::clientActive(pEntity);
}

#if SOURCE_ENGINE >= SE_ORANGEBOX
void RCBotPluginMeta::Hook_ClientCommand(edict_t *pEntity, const CCommand &args)
#else
void RCBotPluginMeta::Hook_ClientCommand(edict_t *pEntity)
#endif
{
	static CBotMod *pMod = nullptr;

#if SOURCE_ENGINE <= SE_DARKMESSIAH
	CCommand args;
#endif

	const char *pcmd = args.Arg(0);

	if (!pEntity || pEntity->IsFree())
	{
		return;
	}

	CClient *pClient = CClients::get(pEntity);

	// is bot command?
	if ( CBotGlobals::m_pCommands->isCommand(pcmd) )
	{		
		// create shifted command list
		BotCommandArgs argList;
		for (unsigned i = 1; i <= static_cast<unsigned>(args.ArgC()); i++) {
			argList.emplace_back(args.Arg(static_cast<int>(i)));
		}
		const eBotCommandResult iResult = CBotGlobals::m_pCommands->execute(pClient, argList);

		if ( iResult == COMMAND_ACCESSED )
		{
			// ok
		}
		else if ( iResult == COMMAND_REQUIRE_ACCESS )
		{
			CBotGlobals::botMessage(pEntity,0,"You do not have access to this command");
		}
		else if ( iResult == COMMAND_NOT_FOUND )
		{
			CBotGlobals::botMessage(pEntity,0,"bot command not found");	
		}
		else if ( iResult == COMMAND_ERROR )
		{
			CBotGlobals::botMessage(pEntity,0,"bot command returned an error");	
		}

		RETURN_META(MRES_SUPERCEDE);
	}
	if ( std::strncmp(pcmd,"menuselect",10) == 0 ) // menu command
	{
		if ( pClient->isUsingMenu() )
		{
			int iCommand = std::atoi(args.Arg(1));

			// format is 1.2.3.4.5.6.7.8.9.0
			if ( iCommand == 0 )
				iCommand = 9;
			else
				iCommand --;

			pClient->getCurrentMenu()->selectedMenu(pClient,iCommand);
		}
	}

	// command capturing
	pMod = CBotGlobals::getCurrentMod();

	// capture some client commands e.g. voice commands
	pMod->clientCommand(pEntity,args.ArgC(),pcmd,args.Arg(1),args.Arg(2));

	RETURN_META(MRES_IGNORED); 
}

bool RCBotPluginMeta::Hook_ClientConnect(edict_t *pEntity,
									const char *pszName,
									const char *pszAddress,
									char *reject,
									int maxrejectlen)
{
	META_LOG(g_PLAPI, R"(Hook_ClientConnect(%d, "%s", "%s"))", IndexOfEdict(pEntity), pszName, pszAddress);

	CClients::init(pEntity);

	return true;
}

void RCBotPluginMeta::Hook_ClientPutInServer(edict_t *pEntity, char const* playername)
{
	CBaseEntity *pEnt = servergameents->EdictToBaseEntity(pEntity); //`*pEnt` Unused? [APG]RoboCop[CL]
	constexpr bool is_Rcbot = false;

	if ( CClient *pClient = CClients::clientConnected(pEntity) )
	{
		if ( !engine->IsDedicatedServer() )
		{
			if ( CClients::noListenServerClient() )
			{
				// give listenserver client all access to bot commands
				CClients::setListenServerClient(pClient);
				pClient->setAccessLevel(CMD_ACCESS_ALL);
				pClient->resetMenuCommands();
			}
		}
	}

	CBotMod *pMod = CBotGlobals::getCurrentMod();

	pMod->playerSpawned(pEntity);

#ifdef OVERRIDE_RUNCMD
	if ( pEnt )
	{
		SH_ADD_MANUALHOOK_MEMFUNC(MHook_PlayerRunCmd, pEnt, this, &RCBotPluginMeta::Hook_PlayerRunCmd, false);
	}
#endif
}

void RCBotPluginMeta::Hook_ClientDisconnect(edict_t *pEntity)
{
	// Diagnostic logging for bot kick investigation
	int slot = IndexOfEdict(pEntity);
	CBot *pBot = CBots::getBotPointer(pEntity);

	if (pBot && pBot->inUse())
	{
		IPlayerInfo *pInfo = playerinfomanager->GetPlayerInfo(pEntity);
		int team = pInfo ? pInfo->GetTeamIndex() : -1;
		fprintf(stderr, "[RCBOT2] Hook_ClientDisconnect(%d): Bot '%s' disconnecting, team=%d, inUse=true\n",
			slot, pBot->getName(), team);
	}
	else
	{
		fprintf(stderr, "[RCBOT2] Hook_ClientDisconnect(%d): Not a bot or not in use\n", slot);
	}

	CBaseEntity *pEnt = servergameents->EdictToBaseEntity(pEntity); //`*pEnt` Unused? [APG]RoboCop[CL]

#ifdef OVERRIDE_RUNCMD
	if ( pEnt )
	{
		SH_REMOVE_MANUALHOOK_MEMFUNC(MHook_PlayerRunCmd, pEnt, this, &RCBotPluginMeta::Hook_PlayerRunCmd, false);
	}
#endif

	CClients::clientDisconnected(pEntity);

	META_LOG(g_PLAPI, "Hook_ClientDisconnect(%d)", IndexOfEdict(pEntity));
}

void RCBotPluginMeta::Hook_GameFrame(const bool simulating)
{
	/**
	 * simulating:
	 * ***********
	 * true  | game is ticking
	 * false | game is not ticking
	 */

	if ( simulating && CBotGlobals::IsMapRunning() )
	{
		static CBotMod *currentmod;

		CBots::botThink();
		CClients::clientThink();

		if ( CWaypoints::getVisiblity()->needToWorkVisibility() )
		{
			CWaypoints::getVisiblity()->workVisibility();
		}

		// Profiling
#ifdef _DEBUG
		if ( CClients::clientsDebugging(BOT_DEBUG_PROFILE) )
		{
			CProfileTimers::updateAndDisplay();
		}
#endif

		// Config Commands
		CBotConfigFile::doNextCommand();
		currentmod = CBotGlobals::getCurrentMod();

		currentmod->modFrame();

		// Phase 1 systems per-frame updates
		CNavTestManager::instance().update();
		CDoorManager::instance().update();
		CGravityManager::instance().update();

		// Bot Quota
		if (rcbot_bot_quota_interval.GetFloat() > 0.0) {
			BotQuotaCheck();
		}
	}
}

void RCBotPluginMeta::BotQuotaCheck() {
	// this is configured with config/bot_quota.ini
	if (rcbot_bot_quota_interval.GetFloat() <= 0.0) {
		return;
	}

	if (m_fBotQuotaTimer < 1.0f) {
		m_fBotQuotaTimer = engine->Time() + 10.0f; // Sleep 10 seconds
	}

	if (m_fBotQuotaTimer < engine->Time() - rcbot_bot_quota_interval.GetFloat()) {
		m_fBotQuotaTimer = engine->Time();

		// Change Notification
		bool notify = false;

		// Current Bot Count
		int bot_count = 0;
		int human_count = 0;

		// Count Players
		for (int i = 0; i < RCBOT_MAXPLAYERS; ++i) {
			const CClient* client = CClients::get(i);
			const CBot* bot = CBots::get(i);

			if (bot != nullptr && bot->getEdict() != nullptr && bot->inUse()) {
				IPlayerInfo *p = playerinfomanager->GetPlayerInfo(bot->getEdict());

				if (p->IsConnected() && p->IsFakeClient() && !p->IsHLTV()) {
					bot_count++;
				}
			}

			if (client->getPlayer() != nullptr && client->isUsed()) {
				IPlayerInfo* p = playerinfomanager->GetPlayerInfo(client->getPlayer());

				if (p->IsConnected() && !p->IsFakeClient() && !p->IsHLTV()) {
					if (rcbot_ignore_spectators.GetBool()) {
						if ( CClassInterface::getTeam(client->getPlayer()) >= 2 )
							human_count++;
					} else
						human_count++;
				}
			}
		}

		if (human_count >= RCBOT_MAXPLAYERS) {
			human_count = 0;
		}

		// Get Bot Quota
		int bot_target = m_iTargetBots[human_count];
		const int max_bots = CBots::getMaxBots();
		const int min_bots = CBots::getMinBots();

		// Use min and max as ceiling and floor
		if ((max_bots > -1) && (bot_target > max_bots)) {
			bot_target = max_bots;
		} else if ((min_bots > -1) && (bot_target < min_bots)) {
			bot_target = min_bots;
		}

		// Change Bot Quota
		if (bot_count > bot_target) {
			if (rcbot_nonrandom_kicking.GetBool()) {
				CBots::kickChosenBot(static_cast<unsigned>(bot_count - bot_target));
			} else {
				CBots::kickRandomBot(static_cast<unsigned>(bot_count - bot_target));
			}
			notify = true;
		}
		else if ((bot_target > bot_count) && ( CBots::getAddKickBotTime() < engine->Time() )) {
			const int bot_diff = bot_target - bot_count;

			for (int i = 0; i < bot_diff; ++i) {
				CBots::createBot("", "", "");
				if (rcbot_addbottime.GetFloat() > 0.0)
					break;
			}

			notify = true;
		}

		if (notify) {
			char chatmsg[128];
			snprintf(chatmsg, sizeof(chatmsg), "[Bot Quota] Humans: %d, Bots: %d", human_count, bot_target);
			logger->Log(LogLevel::INFO, chatmsg);
			// RCBotPluginMeta::BroadcastTextMessage(chatmsg);
		}
	}
}

bool RCBotPluginMeta::Hook_LevelInit(const char *pMapName,
								char const *pMapEntities,
								char const *pOldLevel,
								char const *pLandmarkName,
								bool loadGame,
								bool background)
{
	META_LOG(g_PLAPI, "Hook_LevelInit(%s)", pMapName);

		//CClients::initall();
	// Must set this
	CBotGlobals::setMapName(pMapName);

	logger->Log(LogLevel::INFO, "Level \"%s\" has been loaded", pMapName);

	CWaypoints::precacheWaypointTexture();

	CWaypointDistances::reset();

	CProfileTimers::reset();

	CWaypoints::init();
	CWaypoints::load();

	CBotGlobals::setMapRunning(true);
	CBotConfigFile::reset();
	
	if ( mp_teamplay.IsValid() )
		CBotGlobals::setTeamplay(mp_teamplay.GetBool());
	else
		CBotGlobals::setTeamplay(false);

	CBotEvents::setupEvents();

	CBots::mapInit();

	if ( CBotMod *pMod = CBotGlobals::getCurrentMod() )
		pMod->mapInit();

	CBotSquads::FreeMemory();

	CClients::setListenServerClient(nullptr);

	// Initialize Phase 1 systems
	// Door manager - scan map for door entities
	CDoorManager::instance().scanMap();

	// Tactical data manager - analyze waypoints for tactical metadata
	CTacticalDataManager::instance().analyzeAllWaypoints();

	// Gravity manager - analyze waypoint connections for fall damage
	CGravityManager::instance().update();
	CGravityManager::instance().getPathAnalyzer().analyzeAllConnections();

	// Setup game rules
	extern void **g_pGameRules; //Unused? [APG]RoboCop[CL]

	if (g_pGameRules_Obj && g_pGameRules_Obj->found())
	{
		g_pGameRules = g_pGameRules_Obj->getGameRules();
	}
	else if (g_pGameRules_Create_Obj && g_pGameRules_Create_Obj->found())
	{
		g_pGameRules = g_pGameRules_Create_Obj->getGameRules();
	}

	return true;
}

void RCBotPluginMeta::Hook_LevelShutdown()
{
	META_LOG(g_PLAPI, "Hook_LevelShutdown()");

	CClients::initall();
	CWaypointDistances::save();

	CBots::freeMapMemory();	
	CWaypoints::init();

	CBotGlobals::setMapRunning(false);
	CBotEvents::freeMemory();
}

bool RCBotPluginMeta::Pause(char *error, std::size_t maxlen)
{
	return true;
}

bool RCBotPluginMeta::Unpause(char *error, std::size_t maxlen)
{
	return true;
}

const char *RCBotPluginMeta::GetLicense()
{
	return "GPL General Public License";
}

const char *RCBotPluginMeta::GetVersion()
{
	return build_info::short_version;
}

const char *RCBotPluginMeta::GetDate()
{
	return build_info::date;
}

const char *RCBotPluginMeta::GetLogTag()
{
	return "RCBOT2";
}

const char *RCBotPluginMeta::GetAuthor()
{
	return build_info::authors;
}

const char *RCBotPluginMeta::GetDescription()
{
	return "Bot for HL2DM, TF2 and DOD:S";
}

const char *RCBotPluginMeta::GetName()
{
	return "RCBot2";
}

const char *RCBotPluginMeta::GetURL()
{
	return build_info::url;
}

#ifdef SM_EXT
void RCBotPluginMeta::BindToSourcemod()
{
	char error[256];
	if (!SM_LoadExtension(error, sizeof(error))) {
		char message[512];
		snprintf(message, sizeof(message), "Could not load as a SourceMod extension: %s\n", error);
		engine->LogPrint(message);
	}
}
#endif
