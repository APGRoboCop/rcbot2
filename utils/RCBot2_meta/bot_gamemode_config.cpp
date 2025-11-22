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

#include "bot_gamemode_config.h"
#include "bot_globals.h"
#include "bot_fortress.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstring>

// Global instance
CGamemodeConfig g_GamemodeConfig;

CGamemodeConfig::CGamemodeConfig()
	: m_bLoaded(false)
	, m_bHardcodedFallback(true) // Enable fallback by default for compatibility
{
}

CGamemodeConfig::~CGamemodeConfig()
{
	clear();
}

void CGamemodeConfig::clear()
{
	m_gamemodes.clear();
	m_typeToIndex.clear();
	m_bLoaded = false;
}

std::string CGamemodeConfig::trim(const std::string& str)
{
	const size_t first = str.find_first_not_of(" \t\r\n");
	if (first == std::string::npos)
		return "";

	const size_t last = str.find_last_not_of(" \t\r\n");
	return str.substr(first, last - first + 1);
}

std::vector<std::string> CGamemodeConfig::split(const std::string& str, char delimiter)
{
	std::vector<std::string> tokens;
	std::stringstream ss(str);
	std::string token;

	while (std::getline(ss, token, delimiter))
	{
		token = trim(token);
		if (!token.empty())
			tokens.push_back(token);
	}

	return tokens;
}

std::uint8_t CGamemodeConfig::parseGamemodeType(const char* typeStr)
{
	// Map type strings to constants
	if (std::strcmp(typeStr, "TF_MAP_DM") == 0) return TF_MAP_DM;
	if (std::strcmp(typeStr, "TF_MAP_CTF") == 0) return TF_MAP_CTF;
	if (std::strcmp(typeStr, "TF_MAP_CP") == 0) return TF_MAP_CP;
	if (std::strcmp(typeStr, "TF_MAP_TC") == 0) return TF_MAP_TC;
	if (std::strcmp(typeStr, "TF_MAP_CART") == 0) return TF_MAP_CART;
	if (std::strcmp(typeStr, "TF_MAP_CARTRACE") == 0) return TF_MAP_CARTRACE;
	if (std::strcmp(typeStr, "TF_MAP_ARENA") == 0) return TF_MAP_ARENA;
	if (std::strcmp(typeStr, "TF_MAP_SAXTON") == 0) return TF_MAP_SAXTON;
	if (std::strcmp(typeStr, "TF_MAP_PIPEBALL") == 0) return TF_MAP_PIPEBALL;
	if (std::strcmp(typeStr, "TF_MAP_KOTH") == 0) return TF_MAP_KOTH;
	if (std::strcmp(typeStr, "TF_MAP_SD") == 0) return TF_MAP_SD;
	if (std::strcmp(typeStr, "TF_MAP_TR") == 0) return TF_MAP_TR;
	if (std::strcmp(typeStr, "TF_MAP_MVM") == 0) return TF_MAP_MVM;
	if (std::strcmp(typeStr, "TF_MAP_RD") == 0) return TF_MAP_RD;
	if (std::strcmp(typeStr, "TF_MAP_BUMPERCARS") == 0) return TF_MAP_BUMPERCARS;
	if (std::strcmp(typeStr, "TF_MAP_PD") == 0) return TF_MAP_PD;
	if (std::strcmp(typeStr, "TF_MAP_ZI") == 0) return TF_MAP_ZI;
	if (std::strcmp(typeStr, "TF_MAP_PASS") == 0) return TF_MAP_PASS;
	if (std::strcmp(typeStr, "TF_MAP_CPPL") == 0) return TF_MAP_CPPL;
	if (std::strcmp(typeStr, "TF_MAP_GG") == 0) return TF_MAP_GG;

	// Unknown type
	return 0;
}

bool CGamemodeConfig::load(const char* filename)
{
	clear();

	std::ifstream file(filename);
	if (!file.is_open())
	{
		CBotGlobals::botMessage(nullptr, 0, "Failed to open gamemode config: %s", filename);
		return false;
	}

	std::vector<std::string> lines;
	std::string line;

	// Read all lines
	while (std::getline(file, line))
	{
		lines.push_back(line);
	}
	file.close();

	// Parse gamemode sections
	for (size_t i = 0; i < lines.size(); ++i)
	{
		const std::string trimmedLine = trim(lines[i]);

		// Skip comments and empty lines
		if (trimmedLine.empty() || trimmedLine[0] == '#')
			continue;

		// Check for section header [GameModeName]
		if (trimmedLine[0] == '[' && trimmedLine.back() == ']')
		{
			GamemodeDefinition def;
			def.name = trimmedLine.substr(1, trimmedLine.length() - 2);

			const int linesConsumed = parseGamemodeSection(lines, i + 1, def);

			if (def.type != 0 && linesConsumed > 0)
			{
				m_gamemodes.push_back(def);
				i += linesConsumed; // Skip parsed lines
			}
		}
	}

	// Sort by priority (highest first)
	std::sort(m_gamemodes.begin(), m_gamemodes.end(),
		[](const GamemodeDefinition& a, const GamemodeDefinition& b) {
			return a.priority > b.priority;
		});

	// Build type lookup map
	for (size_t i = 0; i < m_gamemodes.size(); ++i)
	{
		m_typeToIndex[m_gamemodes[i].type] = i;
	}

	m_bLoaded = true;
	CBotGlobals::botMessage(nullptr, 0, "Loaded %zu gamemode definitions from %s",
	                        m_gamemodes.size(), filename);

	return true;
}

int CGamemodeConfig::parseGamemodeSection(const std::vector<std::string>& lines,
                                          size_t startLine,
                                          GamemodeDefinition& outDef)
{
	int linesConsumed = 0;

	for (size_t i = startLine; i < lines.size(); ++i, ++linesConsumed)
	{
		const std::string trimmedLine = trim(lines[i]);

		// Stop at next section or empty line after properties
		if (trimmedLine.empty())
			continue;

		if (trimmedLine[0] == '#')
			continue;

		// Stop at next section
		if (trimmedLine[0] == '[')
			break;

		// Parse key = value
		const size_t equalsPos = trimmedLine.find('=');
		if (equalsPos == std::string::npos)
			continue;

		const std::string key = trim(trimmedLine.substr(0, equalsPos));
		const std::string value = trim(trimmedLine.substr(equalsPos + 1));

		if (value.empty())
			continue;

		// Parse properties
		if (key == "type")
		{
			outDef.type = parseGamemodeType(value.c_str());
		}
		else if (key == "priority")
		{
			outDef.priority = std::atoi(value.c_str());
		}
		else if (key.find("entity.") == 0)
		{
			outDef.entities.push_back(value);
		}
		else if (key.find("map.") == 0)
		{
			outDef.maps.push_back(value);
		}
		else if (key.find("file.") == 0)
		{
			outDef.files.push_back(value);
		}
		else if (key == "special.payload_only")
		{
			outDef.special.payload_only = (value == "true" || value == "1");
		}
		else if (key == "special.custom_flag_handling")
		{
			outDef.special.custom_flag_handling = (value == "true" || value == "1");
		}
		else if (key == "special.min_entity_index")
		{
			outDef.special.min_entity_index = std::atoi(value.c_str());
		}
		else if (key == "special.max_entity_index")
		{
			outDef.special.max_entity_index = std::atoi(value.c_str());
		}
	}

	return linesConsumed;
}

std::uint8_t CGamemodeConfig::detectByMapName(const char* mapname)
{
	if (!m_bLoaded)
		return 0;

	// Check all gamemode definitions in priority order
	for (const auto& def : m_gamemodes)
	{
		for (const auto& mapPrefix : def.maps)
		{
			const size_t prefixLen = mapPrefix.length();

			// Exact match or prefix match
			if (std::strncmp(mapname, mapPrefix.c_str(), prefixLen) == 0)
			{
				return def.type;
			}
		}
	}

	return 0;
}

std::uint8_t CGamemodeConfig::detectByEntity(const char* entityClassname)
{
	if (!m_bLoaded)
		return 0;

	// Check all gamemode definitions in priority order
	for (const auto& def : m_gamemodes)
	{
		for (const auto& entity : def.entities)
		{
			if (std::strcmp(entityClassname, entity.c_str()) == 0)
			{
				return def.type;
			}
		}
	}

	return 0;
}

const GamemodeDefinition* CGamemodeConfig::getDefinition(std::uint8_t type) const
{
	const auto it = m_typeToIndex.find(type);
	if (it != m_typeToIndex.end())
	{
		return &m_gamemodes[it->second];
	}

	return nullptr;
}
