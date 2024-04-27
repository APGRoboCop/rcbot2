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
 *///=================================================================================//

#include "bot.h"
#include "bot_globals.h"
#include "bot_configfile.h"

#include <cstring>

#include "rcbot/logging.h"

std::vector <char *> CBotConfigFile::m_Commands;
unsigned int CBotConfigFile::m_iCmd = 0; // current command (time delayed)
float CBotConfigFile::m_fNextCommandTime = 0.0f;

// 
bot_util_t CRCBotTF2UtilFile::m_fUtils[UTIL_TYPE_MAX][BOT_UTIL_MAX][9];

void CBotConfigFile :: load ()
{
	char filename[512];
	char line[256];
	//int len;
	m_Commands.clear();

	CBotGlobals::buildFileName(filename,"config",BOT_CONFIG_FOLDER,BOT_CONFIG_EXTENSION);

	std::fstream fp = CBotGlobals::openFile(filename, std::fstream::in);

	if ( !fp )
	{
		logger->Log(LogLevel::WARN, "config file not found");
		return;
	}

	while (fp.getline(line, 255))
	{
		if ( line[0] == '#' )
			continue;

		size_t len = std::strlen(line);

		if (len && line[len-1] == '\n') {
			line[--len] = '\0';
		}

		if (len && line[len-1] == '\r') {
			line[--len] = '\0';
		}

		if(!len)
			continue;

		logger->Log(LogLevel::TRACE, "Config entry '%s' read", line);
		m_Commands.emplace_back(CStrings::getString(line));
	}
}

void CBotConfigFile :: doNextCommand ()
{
	char cmd[64] = {};

	if ( m_fNextCommandTime < engine->Time() && m_iCmd < m_Commands.size() )
	{
		snprintf(cmd, sizeof cmd, "%s\n", m_Commands[m_iCmd]);
		engine->ServerCommand(cmd);

		logger->Log(LogLevel::TRACE, "Bot Command '%s' executed", m_Commands[m_iCmd]);
		m_iCmd ++;
		m_fNextCommandTime = engine->Time() + 0.1f;
	}
}

void CBotConfigFile :: executeCommands ()
{
	char cmd[64] = {};

	while ( m_iCmd < m_Commands.size() )
	{
		snprintf(cmd, sizeof cmd, "%s\n", m_Commands[m_iCmd]);
		engine->ServerCommand(cmd);

		logger->Log(LogLevel::TRACE, "Bot Command '%s' executed", m_Commands[m_iCmd]);
		m_iCmd ++;
	}

	engine->ServerExecute();
}

void CRCBotTF2UtilFile :: init()
{
	for (auto& m_fUtil : m_fUtils)
	{
		for (auto& j : m_fUtil)
		{
			for (auto& k : j)
			{
				k.min = 0;
				k.max = 0;
			}
		}
	}
}

void CRCBotTF2UtilFile :: addUtilPerturbation (eBotAction iAction, eTF2UtilType iUtil, float fUtility[9][2])
{
	for ( short unsigned int i = 0; i < 9; i ++ )
	{
		m_fUtils[iUtil][iAction][i].min = fUtility[i][0];
		m_fUtils[iUtil][iAction][i].max = fUtility[i][1];
	}
}

void CRCBotTF2UtilFile :: loadConfig()
{
	init();

	for ( eTF2UtilType iFile = BOT_ATT_UTIL; iFile < UTIL_TYPE_MAX; iFile = static_cast<eTF2UtilType>(static_cast<int>(iFile) + 1) )
	{
		char szFilename[64];
		char szFullFilename[512];
		if ( iFile == BOT_ATT_UTIL )
		{
			std::sprintf(szFilename,"attack_util.csv");
		}
		else
		{
			std::sprintf(szFilename,"normal_util.csv");
		}

		CBotGlobals::buildFileName(szFullFilename,szFilename,BOT_CONFIG_FOLDER);
		std::fstream fp = CBotGlobals::openFile(szFullFilename, std::fstream::in);

		if ( fp )
		{
			char line[256];
			eBotAction iUtil = static_cast<eBotAction>(0);

			while (fp.getline(line, 255))
			{
				if ( line[0] == 'B' && line[1] == 'O' && 
					 line[2] == 'T' && line[3] == '_') // OK
				{
					char utiltype[64];
					float iClassList[TF_CLASS_MAX][2];

					// Format:    U, 1, 2, 3, 4, 5, 6, 7, 8, 9
					//                
					//               s  s  s  d  m  h  p  s  e
					//               c  n  o  e  e  w  y  p  n
					//               o  i  l  m  d  g  r  y  g
					// 
					
					if ( std::sscanf(line,"%[^,],%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f\r\n",utiltype,
						&iClassList[0][0],&iClassList[0][1],
						&iClassList[1][0],&iClassList[1][1],
						&iClassList[2][0],&iClassList[2][1],
						&iClassList[3][0],&iClassList[3][1],
						&iClassList[4][0],&iClassList[4][1],
						&iClassList[5][0],&iClassList[5][1],
						&iClassList[6][0],&iClassList[6][1],
						&iClassList[7][0],&iClassList[7][1],
						&iClassList[8][0],&iClassList[8][1]) )
					{
						//TODO: should be `iAction, iUtil, fUtility`? [APG]RoboCop[CL]
						addUtilPerturbation(iUtil,iFile,iClassList);

						iUtil = static_cast<eBotAction>(static_cast<int>(iUtil) + 1);

						if ( iUtil >= BOT_UTIL_MAX )
							break;

					}
				}
			}
		}
	}
}