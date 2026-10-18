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

#ifndef __BOT_PLUGIN_META_H__
#define __BOT_PLUGIN_META_H__

#include <ISmmPlugin.h>
#include <igameevents.h>
#include <iplayerinfo.h>
#include <sh_vector.h>
#include "engine_wrappers.h"
#include <dt_common.h>
#include <shareddefs.h>

#ifdef SM_EXT
#include <bot_sm_ext.h>
#endif

class CUserCmd;
class IMoveHelper;
class CBaseEntity;
class bf_write;
class IRecipientFilter;

#if defined(_WIN64) || defined(_WIN32) && !defined snprintf
#define snprintf _snprintf
#endif

class RCBotPluginMeta : public ISmmPlugin, public IMetamodListener
{
public:
	bool Load(PluginId id, ISmmAPI *ismm, char *error, std::size_t maxlen, bool late) override;
	bool Unload(char *error, std::size_t maxlen) override;
	bool Pause(char *error, std::size_t maxlen) override;
	bool Unpause(char *error, std::size_t maxlen) override;
	void AllPluginsLoaded() override;
//public: //IMetamodListener stuff
	void OnVSPListening(IServerPluginCallbacks *iface) override;
//public: //hooks

	void Hook_ServerActivate(edict_t *pEdictList, int edictCount, int clientMax);
	bool Hook_LevelInit(const char *pMapName,
		char const *pMapEntities,
		char const *pOldLevel,
		char const *pLandmarkName,
		bool loadGame,
		bool background);
	void Hook_GameFrame(bool simulating);
	void Hook_LevelShutdown();
	void Hook_ClientActive(edict_t *pEntity, bool bLoadGame);
	void Hook_ClientDisconnect(edict_t *pEntity);
	void Hook_ClientPutInServer(edict_t *pEntity, char const *playername);
	
	//Called for a game event.  Same definition as server plugins???
	bool FireGameEvent( IGameEvent *pevent, bool bDontBroadcast );
	void Hook_PlayerRunCmd(CUserCmd *ucmd, IMoveHelper *moveHelper);
	
	bool Hook_ClientConnect(edict_t *pEntity, 
		const char *pszName,
		const char *pszAddress,
		char *reject,
		int maxrejectlen);

	static void HudTextMessage(const edict_t *pEntity, const char *szMessage);
	static void BroadcastTextMessage(const char *szMessage);

	bf_write *Hook_UserMessageBegin(IRecipientFilter *pFilter, int iMsgType);
	void Hook_MessageEnd();

	int m_iSetPlayerLocationMsg = -1;     // cached "SetPlayerLocation" usermessage index (-1 = unknown)
	int m_iPendingLocationRecipient = -1; // entindex of an in-flight SetPlayerLocation message
	bf_write *m_pPendingLocationBuf = nullptr; // its buffer, read at MessageEnd

#if SOURCE_ENGINE >= SE_ORANGEBOX
	void Hook_ClientCommand(edict_t *pEntity, const CCommand &args);
#else
	void Hook_ClientCommand(edict_t *pEntity);
#endif

//public: // SourceMod
#ifdef SM_EXT
	void *OnMetamodQuery(const char* iface, int *ret) override;
#endif

//public:

	const char *GetAuthor() override;
	const char *GetName() override;
	const char *GetDescription() override;
	const char *GetURL() override;
	const char *GetLicense() override;
	const char *GetVersion() override;
	const char *GetDate() override;
	const char *GetLogTag() override;

private:
#ifdef SM_EXT
	void BindToSourcemod();
#endif

	int m_iClientCommandIndex = 0;

	// Bot Quota
	float m_fBotQuotaTimer = 0.0f;
	int m_iTargetBots[RCBOT_MAXPLAYERS] = {};

	void BotQuotaCheck();

#ifdef OVERRIDE_RUNCMD
	// The player entities that currently carry our PlayerRunCmd hook, by client index.
	// SourceHook keeps one entry per hooked object and walks that list on every hooked call, so every entry that is
	// not removed makes every PlayerRunCommand of every player slower for as long as the server stays up.
	// The engine removes bots at a map change without calling ClientDisconnect, so the hooks are tracked here and
	// removed explicitly on disconnect, at level shutdown and on unload.
	CBaseEntity *m_pRunCmdHooked[RCBOT_MAXPLAYERS + 1] = {};
	void AddRunCmdHook(int clientIndex, CBaseEntity *pEntity);
	void RemoveRunCmdHook(int clientIndex);
	int RemoveAllRunCmdHooks();   // returns how many hooks were removed
#endif
};

extern RCBotPluginMeta g_RCBotPluginMeta;

PLUGIN_GLOBALVARS();

#endif //_INCLUDE_METAMOD_SOURCE_STUB_PLUGIN_H_