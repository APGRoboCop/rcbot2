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
#ifndef __BOT_COMMANDS_H__
#define __BOT_COMMANDS_H__

#include "edict.h"
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <vector>

class CClient;

typedef enum : std::uint8_t
{
	COMMAND_NOT_FOUND,     // command not found
	COMMAND_ACCESSED,      // okay
	COMMAND_ERROR,         // accessed but error occurred
	COMMAND_REQUIRE_ACCESS // dont have access to command
} eBotCommandResult;

#define NEED_ARG(x) if ( !(x) || !*(x) ) return COMMAND_ERROR;

enum : std::uint8_t
{
	CMD_ACCESS_NONE = 0,
	CMD_ACCESS_WAYPOINT = 1<<0,
	CMD_ACCESS_BOT = 1<<1,
	CMD_ACCESS_UTIL = 1<<2,
	CMD_ACCESS_CONFIG = 1<<3,
	CMD_ACCESS_DEBUG = 1<<4,
	CMD_ACCESS_USERS = 1<<5,
	CMD_ACCESS_DEDICATED = 1<<6 // replaces canbeUsedDedicated
};

#define CMD_ACCESS_ALL (CMD_ACCESS_WAYPOINT|CMD_ACCESS_UTIL|CMD_ACCESS_BOT|CMD_ACCESS_CONFIG|CMD_ACCESS_DEBUG|CMD_ACCESS_USERS)

/**
 * This is just a deque that returns nullptr if we access an out-of-bounds array element.
 */
class BotCommandArgs : public std::deque<const char*> {
public:
	const char* operator[](const std::size_t at) const
	{
		if (at >= this->size()) {
			return nullptr;
		}
		return std::deque<const char*>::operator[](at);
	}
};

using BotCommandCallback = std::function<eBotCommandResult(CClient*, BotCommandArgs)>;

class CBotCommand
{
protected:
	CBotCommand () : m_iAccessLevel{0}, m_szCommand{nullptr}, m_szHelp{nullptr} { }
	
public:
	virtual ~CBotCommand() = default;
	
	// initialise
	//CBotCommand(const char *command, int iAccessLevel = 0) :
	//		m_iAccessLevel{iAccessLevel}, m_szCommand{command} {};

	CBotCommand(const char* command, const int iAccessLevel = 0, const char* help = nullptr) :
			m_iAccessLevel{iAccessLevel}, m_szCommand{command}, m_szHelp{help} {}

	// check command name
	bool isCommand ( const char *szCommand ) const;	

	// execute command
	// we pass byval / copy the argument list so they can be pushed / shifted without
	// affecting the original list
	virtual eBotCommandResult execute(CClient* pClient, const BotCommandArgs& args);

	bool hasAccess (const CClient *pClient ) const;

	virtual void printCommand ( edict_t *pPrintTo, int indent = 0);

	virtual void printHelp ( edict_t *pPrintTo );

	virtual bool isContainer () { return false; }

	bool canbeUsedDedicated () const { return (this->m_iAccessLevel & CMD_ACCESS_DEDICATED) != 0; }
protected:

	int m_iAccessLevel;
	const char *m_szCommand;
	const char *m_szHelp;
};

class CBotCommandInline : public CBotCommand
{
public:
	CBotCommandInline(const char* cmd, const int iAccessLevel, BotCommandCallback callback, const char* help = nullptr) : CBotCommand(cmd, iAccessLevel, help), m_Callback(std::move(
		callback)) {}
	
	eBotCommandResult execute(CClient* pClient, const BotCommandArgs& args) override;
	
	BotCommandCallback m_Callback;
};

class CBotSubcommands : public CBotCommand
{
public:
	CBotSubcommands(const char* cmd, const int iAccessLevel, const std::vector<CBotCommand*>& subcommands) : CBotCommand(cmd, iAccessLevel, nullptr), m_theCommands{subcommands} {}
	CBotSubcommands(const char* cmd, const int iAccessLevel, const std::vector<CBotCommand*>& subcommands, const char* help) : CBotCommand(cmd, iAccessLevel, help), m_theCommands{subcommands} {}
	
	eBotCommandResult execute(CClient* pClient, const BotCommandArgs& args) override;
	
	void printCommand(edict_t *pPrintTo, int indent = 0) override;
	void printHelp(edict_t *pPrintTo) override;
	
	bool isContainer() override { return true; }
private:
	std::vector<CBotCommand*> m_theCommands;
};

#endif
