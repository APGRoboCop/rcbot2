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
 *///=================================================================================//

#include "bot.h"
#include "bot_globals.h"
#include "bot_configfile.h"

#include <cstring>
#include <sstream>
#include <vector>
#include <array>

#include "rcbot/logging.h"

std::vector <char *> CBotConfigFile::m_Commands;
std::size_t CBotConfigFile::m_iCmd = 0; // current command (time delayed)
float CBotConfigFile::m_fNextCommandTime = 0.0f;

// 
bot_util_t CRCBotTF2UtilFile::m_fUtils[UTIL_TYPE_MAX][BOT_UTIL_MAX][9];

void CBotConfigFile::load()
{
	char filename[512];
	char line[256];
	//int len;
    m_Commands.clear();

    CBotGlobals::buildFileName(filename, "config", BOT_CONFIG_FOLDER, BOT_CONFIG_EXTENSION);

    std::ifstream fp(filename);

    if (!fp)
    {
        logger->Log(LogLevel::WARN, "Config file not found");
        return;
    }

	while (fp.getline(line, 255))
    {
		if ( line[0] == '#' )
            continue;

		std::size_t len = std::strlen(line);

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

void CBotConfigFile::doNextCommand()
{
    if (m_fNextCommandTime < engine->Time() &&
        m_iCmd < m_Commands.size())
    {
        char cmd[64] = {};
        snprintf(cmd, sizeof(cmd), "%s\n", m_Commands[m_iCmd]);
        engine->ServerCommand(cmd);

        logger->Log(LogLevel::TRACE, "Bot Command '%s' executed", m_Commands[m_iCmd]);
        m_iCmd++;
        m_fNextCommandTime = engine->Time() + 0.1f;
    }
}

void CBotConfigFile::executeCommands()
{
    while (m_iCmd < m_Commands.size())
    {
        char cmd[64] = {};
        snprintf(cmd, sizeof(cmd), "%s\n", m_Commands[m_iCmd]); // Directly use m_Commands[m_iCmd] as it is a char*
        engine->ServerCommand(cmd);

        logger->Log(LogLevel::TRACE, "Bot Command '%s' executed", m_Commands[m_iCmd]); // Directly use m_Commands[m_iCmd]
        m_iCmd++;
    }

    engine->ServerExecute();
}

void CRCBotTF2UtilFile::init()
{
	for (bot_util_t(&m_fUtil)[BOT_UTIL_MAX][9] : m_fUtils)//Added [BOT_UTIL_MAX] to match the declaration of m_fUtils - [APG]RoboCop[CL]
    {
        for (bot_util_t (&j)[9] : m_fUtil)
        {
            for (bot_util_t& k : j)
            {
                k.min = 0;
                k.max = 0;
            }
        }
    }
}

void CRCBotTF2UtilFile::addUtilPerturbation(const eBotAction iAction, const eTF2UtilType iUtil, float fUtility[9][2])
{
    for (short unsigned i = 0; i < 9; i++)
    {
        m_fUtils[iUtil][iAction][i].min = fUtility[i][0];
        m_fUtils[iUtil][iAction][i].max = fUtility[i][1];
    }
}

void CRCBotTF2UtilFile::loadConfig()
{
    init();

    for (eTF2UtilType iFile = BOT_ATT_UTIL; iFile < UTIL_TYPE_MAX; iFile = static_cast<eTF2UtilType>(static_cast<int>(iFile) + 1))
    {
		char szFilename[64];
		char szFullFilename[512];
		if ( iFile == BOT_ATT_UTIL )
        {
			snprintf(szFilename, sizeof(szFilename), "attack_util.csv");
        }
        else
        {
			snprintf(szFilename, sizeof(szFilename), "normal_util.csv");
        }

        CBotGlobals::buildFileName(szFullFilename, szFilename, BOT_CONFIG_FOLDER);

		if (std::ifstream fp(szFullFilename); fp)
        {
            std::string line;
            eBotAction iUtil = static_cast<eBotAction>(0);

            while (std::getline(fp, line))
            {
                if (line.substr(0, 4) == "BOT_") // OK
                {
                    std::array<float, 18> iClassList;
                    std::string utiltype;

                    std::istringstream ss(line);
                    std::getline(ss, utiltype, ',');

                    for (float& val : iClassList)
                    {
                        std::string temp;
                        std::getline(ss, temp, ',');
                        val = std::stof(temp);
                    }

                    //TODO: should be `iAction, iUtil, fUtility`? [APG]RoboCop[CL]
                    addUtilPerturbation(iUtil, iFile, reinterpret_cast<float(*)[2]>(iClassList.data()));

                    iUtil = static_cast<eBotAction>(static_cast<int>(iUtil) + 1);

                    if (iUtil >= BOT_UTIL_MAX)
                        break;
                }
            }
        }
    }
}