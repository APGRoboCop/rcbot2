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
#include "bot_commands.h"
#include "bot.h"
#include <ctime>
#include "bot_accessclient.h"
#include "bot_client.h"
#include "bot_cvars.h"
#include "bot_getprop.h"
#include "bot_globals.h"
#include "bot_menu.h"
#include "bot_schedule.h"
#include "bot_strings.h"
#include "bot_waypoint.h"           // for waypoint commands
#include "bot_waypoint_locations.h" // for waypoint commands
#include "bot_waypoint_visibility.h"
#include "bot_weapons.h"
#include "bot_navtest.h"            // for nav-test commands
#include "bot_door.h"               // for door commands
#include "bot_gravity.h"            // for gravity commands
#include "ndebugoverlay.h"

#include "bot_tf2_points.h"

extern IVDebugOverlay* debugoverlay;

// include our subcommands
#include "rcbot_subcmds/config.cpp"
#include "rcbot_subcmds/debug.cpp"
#include "rcbot_subcmds/pathwaypoint.cpp"
#include "rcbot_subcmds/users.cpp"
#include "rcbot_subcmds/util.cpp"
#include "rcbot_subcmds/waypoint.cpp"

// ML/AI commands
#include "bot_ml_commands.cpp"

// temporarily declared at the bottom
// CBotSubcommands *CBotGlobals :: m_pCommands;

eBotCommandResult CBotCommandInline::execute(CClient* pClient, const BotCommandArgs& args) {
	// fire off callback function
	return m_Callback(pClient, args);
}

CBotCommandInline ControlCommand("control", CMD_ACCESS_BOT | CMD_ACCESS_DEDICATED, [](const CClient* pClient,
	const BotCommandArgs& args)
	{
		edict_t* pEntity = nullptr;

		if (pClient)
			pEntity = pClient->getPlayer();
		if (args[0] && *args[0])
		{
			if (CBots::controlBot(args[0], args[0], args[2], args[3]))
				CBotGlobals::botMessage(pEntity, 0, "bot added");
			else
				CBotGlobals::botMessage(pEntity, 0, "error: couldn't control bot '%s'", args[0]);

			return COMMAND_ACCESSED;

		}
		return COMMAND_ERROR;
	});

CBotCommandInline AddBotCommand("addbot", CMD_ACCESS_BOT | CMD_ACCESS_DEDICATED, [](const CClient* pClient,
	const BotCommandArgs& args)
{
//	bool bOkay = false;

	edict_t* pEntity = nullptr;

	if (pClient)
		pEntity = pClient->getPlayer();

	// That breaks the bot quota system? [APG]RoboCop[CL]
	/*if (rcbot_bot_quota_interval.GetFloat() > 0) {
		CBotGlobals::botMessage(pEntity, 0, "error: cannot manually add bot while rcbot_bot_quota_interval is active");
		return COMMAND_ACCESSED;
	}*/
	
	//if ( !bot_sv_cheat_warning.GetBool() || bot_sv_cheats_auto.GetBool() || (!sv_cheats || sv_cheats->GetBool()) )
	//{
		//if ( !args[0] || !*args[0] )
		//	bOkay = CBots::createBot();
		//else
		//bOkay = CBots::createBot();

	if (CBots::createBot(args[0], args[1], args[2])) {
		CBotGlobals::botMessage(pEntity, 0, "bot adding...");
	}
	else {
		CBotGlobals::botMessage(pEntity, 0, "error: couldn't create bot! (Check maxplayers)");
	}

	//}
	//else
	//	CBotGlobals::botMessage(pEntity,0,"error: sv_cheats must be 1 to add bots");

	return COMMAND_ACCESSED;
	});

CBotCommandInline KickBotCommand("kickbot", CMD_ACCESS_BOT | CMD_ACCESS_DEDICATED, [](CClient* pClient,
	const BotCommandArgs& args)
{
	if (!args[0] || !*args[0])
	{
		if (rcbot_nonrandom_kicking.GetBool()) {
			CBots::kickChosenBot();
		} else {
			CBots::kickRandomBot();
		}
	}
	else
	{
		const int team = std::atoi(args[0]);

		if (rcbot_nonrandom_kicking.GetBool()) {
			CBots::kickChosenBotOnTeam(team);
		} else {
			CBots::kickRandomBotOnTeam(team);
		}
	}

	return COMMAND_ACCESSED;
}, R"(usage "kickbot" or "kickbot <team>" : kicks random bot or bot on team: <team>)");

bool CBotCommand::hasAccess(const CClient* pClient) const
{
	// check access level excluding dedicated server flag
	const int iClientAccessLevel = this->m_iAccessLevel & ~CMD_ACCESS_DEDICATED;
	return (iClientAccessLevel & pClient->accessLevel()) == iClientAccessLevel;
}

bool CBotCommand::isCommand(const char* szCommand) const
{
	return FStrEq(szCommand, m_szCommand);
}

eBotCommandResult CBotCommand::execute(CClient* pClient, const BotCommandArgs& args) {
	return COMMAND_NOT_FOUND;
}

eBotCommandResult CBotSubcommands::execute(CClient* pClient, const BotCommandArgs& args) {
	BotCommandArgs mutableArgs = args; // Make a mutable copy
	const char* subcmd = mutableArgs[0];
	mutableArgs.pop_front();

	for (CBotCommand* const cmd : m_theCommands) {
		if (!cmd->isCommand(subcmd)) {
			continue;
		}

		if (pClient && !cmd->hasAccess(pClient)) {
			return COMMAND_REQUIRE_ACCESS;
		}
		if (!pClient && !cmd->canbeUsedDedicated()) {
			CBotGlobals::botMessage(nullptr, 0, "Sorry, this command cannot be used on a dedicated server");
			return COMMAND_ERROR;
		}

		// shift arguments and call
		const eBotCommandResult result = cmd->execute(pClient, mutableArgs);
		if (result == COMMAND_ERROR) {
			cmd->printHelp(pClient ? pClient->getPlayer() : nullptr);
		}
		return COMMAND_ACCESSED;
	}

	printHelp(pClient ? pClient->getPlayer() : nullptr);
	return COMMAND_NOT_FOUND;
}

void CBotSubcommands::printCommand(edict_t* pPrintTo, const int indent)
{
	if (indent)
	{
		constexpr int maxIndent = 64;
		char szIndent[maxIndent] = {};

		for (int i = 0; (i < (indent * 2)) && (i < maxIndent - 1); i++)
			szIndent[i] = ' ';

		CBotGlobals::botMessage(pPrintTo, 0, "%s[%s]", szIndent, m_szCommand);
	}
	else
		CBotGlobals::botMessage(pPrintTo, 0, "[%s]", m_szCommand);

	for (CBotCommand* const& m_theCommand : m_theCommands)
	{
		m_theCommand->printCommand(pPrintTo, indent + 1);
	}
}

void CBotSubcommands::printHelp(edict_t* pPrintTo) {
	this->printCommand(pPrintTo);
}

CBotCommandInline PrintCommands("printcommands", CMD_ACCESS_DEDICATED, [](const CClient* pClient, const BotCommandArgs& args)
{
	if ( pClient != nullptr)
	{
		CBotGlobals::botMessage(pClient->getPlayer(),0,"All bot commands:");
		CBotGlobals::m_pCommands->printCommand(pClient->getPlayer());
	}
	else
	{
		CBotGlobals::botMessage(nullptr,0,"All bot commands:");
		CBotGlobals::m_pCommands->printCommand(nullptr);
	}

	return COMMAND_ACCESSED;
});

///////////////////////////////////////////

void CBotCommand::printCommand(edict_t* pPrintTo, const int indent)
{
	if (indent)
	{
		constexpr int maxIndent = 64;
		char szIndent[maxIndent];
		int i;

		for (i = 0; (i < (indent * 2)) && (i < maxIndent - 1); i++)
			szIndent[i] = ' ';

		szIndent[maxIndent - 1] = 0;
		szIndent[i] = 0;

		if (!pPrintTo && !canbeUsedDedicated())
			CBotGlobals::botMessage(pPrintTo, 0, "%s%s [can't use]", szIndent, m_szCommand);
		else
			CBotGlobals::botMessage(pPrintTo, 0, "%s%s", szIndent, m_szCommand);
	}
	else
	{
		if (!pPrintTo && !canbeUsedDedicated())
			CBotGlobals::botMessage(pPrintTo, 0, "%s [can't use]", m_szCommand);
		else
			CBotGlobals::botMessage(pPrintTo, 0, m_szCommand);
	}
}

void CBotCommand::printHelp(edict_t* pPrintTo)
{
	if (m_szHelp)
		CBotGlobals::botMessage(pPrintTo, 0, m_szHelp);
	else
		CBotGlobals::botMessage(pPrintTo, 0, "Sorry, no help for this command (yet)");

	//return;
}

CBotCommandInline CTestCommand("test", 0, [](CClient* pClient, const BotCommandArgs& args)
{
	// for developers
	// first argument is at args[0]
	return COMMAND_NOT_FOUND;
});

// Nav-test subcommands
CBotCommandInline NavTestStartCommand("start", CMD_ACCESS_WAYPOINT, [](CClient* pClient, const BotCommandArgs& args)
{
	float duration = 0.0f;
	if (args[0] && *args[0])
		duration = static_cast<float>(atof(args[0]));

	if (CNavTestManager::instance().startSession(duration))
	{
		// Enable nav-test mode on all bots and give them exploration schedules
		int botsEnabled = 0;
		for (int i = 0; i < RCBOT_MAXPLAYERS; i++)
		{
			CBot* pBot = CBots::getBot(i);
			if (pBot != nullptr && pBot->inUse())
			{
				pBot->setNavTestMode(true);

				// Clear current schedules and add exploration schedule
				CBotSchedules* pSchedules = pBot->getSchedule();
				if (pSchedules != nullptr)
				{
					pSchedules->freeMemory();
					pSchedules->add(new CNavTestExploreSched(-1));
				}
				botsEnabled++;
			}
		}

		edict_t* pEntity = pClient ? pClient->getPlayer() : nullptr;
		CBotGlobals::botMessage(pEntity, 0, "Nav-test mode enabled on %d bots", botsEnabled);

		return COMMAND_ACCESSED;
	}
	return COMMAND_ERROR;
}, "Start nav-test session. Usage: rcbot navtest start [duration_seconds]");

CBotCommandInline NavTestStopCommand("stop", CMD_ACCESS_WAYPOINT, [](CClient* pClient, const BotCommandArgs& args)
{
	// Disable nav-test mode on all bots
	for (int i = 0; i < RCBOT_MAXPLAYERS; i++)
	{
		CBot* pBot = CBots::getBot(i);
		if (pBot != nullptr && pBot->inUse())
		{
			pBot->setNavTestMode(false);

			// Clear nav-test schedules
			CBotSchedules* pSchedules = pBot->getSchedule();
			if (pSchedules != nullptr && pSchedules->isCurrentSchedule(SCHED_NAVTEST_EXPLORE))
			{
				pSchedules->freeMemory();
			}
		}
	}

	CNavTestManager::instance().stopSession();
	return COMMAND_ACCESSED;
}, "Stop current nav-test session");

CBotCommandInline NavTestStatusCommand("status", CMD_ACCESS_WAYPOINT, [](CClient* pClient, const BotCommandArgs& args)
{
	const CNavTestSession& session = CNavTestManager::instance().getCurrentSession();
	edict_t* pEntity = pClient ? pClient->getPlayer() : nullptr;

	if (!session.isActive)
	{
		CBotGlobals::botMessage(pEntity, 0, "No active nav-test session.");
		return COMMAND_ACCESSED;
	}

	float elapsed = static_cast<float>(std::time(nullptr) - session.startTime);
	CBotGlobals::botMessage(pEntity, 0, "Nav-test Status:");
	CBotGlobals::botMessage(pEntity, 0, "  Map: %s", session.mapName.c_str());
	CBotGlobals::botMessage(pEntity, 0, "  Elapsed: %.1f seconds", elapsed);
	CBotGlobals::botMessage(pEntity, 0, "  Coverage: %d/%d (%.1f%%)",
		CNavTestManager::instance().getCoverageTracker().getVisitedCount(),
		session.totalWaypoints,
		CNavTestManager::instance().getCoverageTracker().getCoveragePercent() * 100.0f);
	CBotGlobals::botMessage(pEntity, 0, "  Issues: %d", CNavTestManager::instance().getIssueTracker().getIssueCount());

	return COMMAND_ACCESSED;
}, "Show nav-test session status");

CBotCommandInline NavTestReportCommand("report", CMD_ACCESS_WAYPOINT, [](CClient* pClient, const BotCommandArgs& args)
{
	CNavTestManager::instance().generateReport(true, nullptr);
	return COMMAND_ACCESSED;
}, "Generate nav-test report");

CBotSubcommands NavTestSubcommands("navtest", CMD_ACCESS_WAYPOINT, {
	&NavTestStartCommand,
	&NavTestStopCommand,
	&NavTestStatusCommand,
	&NavTestReportCommand
}, "Nav-test commands for automated waypoint testing");

// Door manager commands
CBotCommandInline DoorScanCommand("scan", CMD_ACCESS_WAYPOINT, [](CClient* pClient, const BotCommandArgs& args)
{
	CDoorManager::instance().scanMap();
	return COMMAND_ACCESSED;
}, "Rescan map for door entities");

CBotCommandInline DoorInfoCommand("info", CMD_ACCESS_WAYPOINT, [](CClient* pClient, const BotCommandArgs& args)
{
	edict_t* pEntity = pClient ? pClient->getPlayer() : nullptr;
	CBotGlobals::botMessage(pEntity, 0, "Door Manager: %d doors tracked", CDoorManager::instance().getDoorCount());
	return COMMAND_ACCESSED;
}, "Show door manager info");

CBotSubcommands DoorSubcommands("door", CMD_ACCESS_WAYPOINT, {
	&DoorScanCommand,
	&DoorInfoCommand
}, "Door handling commands");

// Gravity info command
CBotCommandInline GravityInfoCommand("info", CMD_ACCESS_WAYPOINT, [](CClient* pClient, const BotCommandArgs& args)
{
	edict_t* pEntity = pClient ? pClient->getPlayer() : nullptr;
	const CGravityInfo& info = CGravityManager::instance().getGravityInfo();

	CBotGlobals::botMessage(pEntity, 0, "Gravity Info:");
	CBotGlobals::botMessage(pEntity, 0, "  Current gravity: %.0f", info.getGravity());
	CBotGlobals::botMessage(pEntity, 0, "  Max safe fall height: %.0f units", info.getMaxSafeFallHeight());
	CBotGlobals::botMessage(pEntity, 0, "  Fatal fall height: %.0f units", info.getFatalFallHeight());
	CBotGlobals::botMessage(pEntity, 0, "  Non-standard gravity: %s", info.isNonStandardGravity() ? "Yes" : "No");
	CBotGlobals::botMessage(pEntity, 0, "  Dangerous connections: %d", CGravityManager::instance().getDangerousConnectionCount());

	return COMMAND_ACCESSED;
}, "Show gravity and fall damage info");

CBotSubcommands GravitySubcommands("gravity", CMD_ACCESS_WAYPOINT, {
	&GravityInfoCommand
}, "Gravity-aware navigation commands");

CBotSubcommands* CBotGlobals::m_pCommands = new CBotSubcommands("rcbot", CMD_ACCESS_DEDICATED, {
	&WaypointSubcommands,
	&AddBotCommand,
	&ControlCommand,
	&PathWaypointSubcommands,
	&DebugSubcommands,
	&PrintCommands,
	&ConfigSubcommands,
	&KickBotCommand,
	&UserSubcommands,
	&UtilSubcommands,
	&NavTestSubcommands,   // Nav-test commands
	&DoorSubcommands,      // Door handling commands
	&GravitySubcommands    // Gravity navigation commands
});
