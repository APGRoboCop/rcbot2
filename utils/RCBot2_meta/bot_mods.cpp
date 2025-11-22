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
#include "engine_wrappers.h"

#include "bot.h"

#include "in_buttons.h"

#include "bot_mods.h"
#include "bot_globals.h"
#include "bot_fortress.h"
#include "bot_weapons.h"
#include "bot_configfile.h"
#include "bot_getprop.h"
#include "bot_dod_bot.h"
#include "bot_navigator.h"
#include "bot_waypoint.h"
#include "bot_waypoint_locations.h"
#include "bot_perceptron.h"

#include <cstring>

#include "rcbot/logging.h"

#if defined(_WIN64) || defined(_WIN32)
#define strcmpi _strcmpi
#endif 

std::vector<edict_wpt_pair_t> CHalfLifeDeathmatchMod::m_LiftWaypoints;

void CBotMods::parseFile()
{
	char buffer[1024];
	char key[64];
	char val[256];

	eModId modtype = MOD_CUSTOM;
	eBotType bottype = BOTTYPE_GENERIC;

	char gamefolder[256];
	char weaponlist[64];

	CBotGlobals::buildFileName(buffer, BOT_MOD_FILE, BOT_CONFIG_FOLDER, BOT_CONFIG_EXTENSION);

	std::fstream fp = CBotGlobals::openFile(buffer, std::fstream::in);

	CBotMod* curmod = nullptr;

	if (!fp)
	{
		logger->Log(LogLevel::WARN, "Failed to open file '%s' for reading - using default configurations", buffer);
		logger->Log(LogLevel::INFO, "You can create %s to define custom game mod configurations", buffer);
		return;
	}

	logger->Log(LogLevel::INFO, "Parsing mod configuration file: %s", buffer);

	while (fp.getline(buffer, 1023))
	{
		if (buffer[0] == '#')
			continue;

		std::size_t len = std::strlen(buffer);

		if (len == 0)
			continue;

		if (buffer[len - 1] == '\n')
			buffer[--len] = 0;

		std::size_t i = 0;
		std::size_t j = 0;

		while (i < len && buffer[i] != '=')
		{
			if (buffer[i] != ' ')
				key[j++] = buffer[i];
			i++;
		}

		i++;

		key[j] = 0;

		j = 0;

		while (i < len && buffer[i] != '\n' && buffer[i] != '\r')
		{
			if (j || buffer[i] != ' ')
				val[j++] = buffer[i];
			i++;
		}

		val[j] = 0;

		if (!std::strcmp(key, "mod"))
		{
			if (curmod)
			{
				curmod->setup(gamefolder, modtype, bottype, weaponlist);
				m_Mods.emplace_back(curmod);
				logger->Log(LogLevel::INFO, "Loaded mod configuration: %s (ID: %d, Type: %d)",
					gamefolder, modtype, bottype);
			}

			curmod = nullptr;
			weaponlist[0] = 0;

			bottype = BOTTYPE_GENERIC;

			//modtype = MOD_CUSTOM;

			if (!strcmpi("CUSTOM", val))
			{
				modtype = MOD_CUSTOM;
				curmod = new CBotMod();
				logger->Log(LogLevel::DEBUG, "Creating CUSTOM mod");
			}
			else if (!strcmpi("CSS", val))
			{
				modtype = MOD_CSS;
				curmod = new CCounterStrikeSourceMod();
				logger->Log(LogLevel::DEBUG, "Creating Counter-Strike: Source mod");
			}
			else if (!strcmpi("HL1DM", val))
			{
				modtype = MOD_HL1DMSRC;
				curmod = new CHLDMSourceMod();
				logger->Log(LogLevel::DEBUG, "Creating Half-Life 1 Deathmatch mod");
			}
			else if (!strcmpi("HL2DM", val))
			{
				modtype = MOD_HLDM2;
				curmod = new CHalfLifeDeathmatchMod();
				logger->Log(LogLevel::DEBUG, "Creating Half-Life 2 Deathmatch mod");
			}
			else if (!strcmpi("FF", val))
			{
				modtype = MOD_FF;
				curmod = new CFortressForeverMod();
				logger->Log(LogLevel::DEBUG, "Creating Fortress Forever mod");
			}
			else if (!strcmpi("TF2", val))
			{
				modtype = MOD_TF2;
				curmod = new CTeamFortress2Mod();
				logger->Log(LogLevel::DEBUG, "Creating Team Fortress 2 mod");
			}
			else if (!strcmpi("SVENCOOP2", val))
			{
				modtype = MOD_SVENCOOP2;
				curmod = new CBotMod();
				logger->Log(LogLevel::DEBUG, "Creating Sven Co-op 2 mod");
			}
			else if (!strcmpi("TIMCOOP", val))
			{
				modtype = MOD_TIMCOOP;
				curmod = new CBotMod();
				logger->Log(LogLevel::DEBUG, "Creating TIM Co-op mod");
			}
			else if (!strcmpi("NS2", val))
			{
				modtype = MOD_NS2;
				curmod = new CBotMod();
				logger->Log(LogLevel::DEBUG, "Creating Natural Selection 2 mod");
			}
			else if (!strcmpi("SYNERGY", val))
			{
				modtype = MOD_SYNERGY;
				curmod = new CSynergyMod();
				logger->Log(LogLevel::DEBUG, "Creating Synergy mod");
			}
			else if (!strcmpi("DOD", val))
			{
				modtype = MOD_DOD;
				curmod = new CDODMod();
				logger->Log(LogLevel::DEBUG, "Creating Day of Defeat: Source mod");
			}
			else if (!strcmpi("BMS", val) || !strcmpi("BLACKMESA", val))
			{
				modtype = MOD_CUSTOM; // Use CUSTOM until Black Mesa mod class is implemented
				curmod = new CBotMod();
				logger->Log(LogLevel::INFO, "Creating Black Mesa mod (using generic bot - full support pending)");
			}
			else
			{
				modtype = MOD_CUSTOM;
				curmod = new CBotMod();
				logger->Log(LogLevel::WARN, "Unknown mod type '%s', creating generic mod", val);
			}
		}
		else if (curmod && !std::strcmp(key, "bot"))
		{
			if (!strcmpi("GENERIC", val))
				bottype = BOTTYPE_GENERIC;
			else if (!strcmpi("CSS", val))
				bottype = BOTTYPE_CSS;
			else if (!strcmpi("HL1DM", val))
				bottype = BOTTYPE_HL1DM;
			else if (!strcmpi("HL2DM", val))
				bottype = BOTTYPE_HL2DM;
			else if (!strcmpi("FF", val))
				bottype = BOTTYPE_FF;
			else if (!strcmpi("TF2", val))
				bottype = BOTTYPE_TF2;
			else if (!strcmpi("COOP", val))
				bottype = BOTTYPE_COOP;
			else if (!strcmpi("ZOMBIE", val))
				bottype = BOTTYPE_ZOMBIE;
			else if (!strcmpi("DOD", val))
				bottype = BOTTYPE_DOD;
			else if (!strcmpi("SYNERGY", val))
				bottype = BOTTYPE_SYN;
		}
		else if (curmod && !strcmpi(key, "gamedir"))
		{
			std::strncpy(gamefolder, val, 255);
		}
		else if (curmod && !strcmpi(key, "weaponlist"))
		{
			std::strncpy(weaponlist, val, 63);
		}
	}

	if (curmod)
	{
		curmod->setup(gamefolder, modtype, bottype, weaponlist);
		m_Mods.emplace_back(curmod);
	}
}

// Enhanced game detection with fallback methods
static CBotMod* detectGameByCapabilities(const char* szGameFolder)
{
	logger->Log(LogLevel::INFO, "Attempting capability-based detection for game folder: %s", szGameFolder);

	// Check game folder name patterns
	if (std::strstr(szGameFolder, "tf") || std::strstr(szGameFolder, "team")) {
		logger->Log(LogLevel::INFO, "Detected Team Fortress pattern");
		return new CTeamFortress2Mod();
	}
	else if (std::strstr(szGameFolder, "dod")) {
		logger->Log(LogLevel::INFO, "Detected Day of Defeat pattern");
		return new CDODMod();
	}
	else if (std::strstr(szGameFolder, "cstrike") || std::strstr(szGameFolder, "css")) {
		logger->Log(LogLevel::INFO, "Detected Counter-Strike pattern");
		return new CCounterStrikeSourceMod();
	}
	else if (std::strstr(szGameFolder, "hl2mp") || std::strstr(szGameFolder, "hl2dm")) {
		logger->Log(LogLevel::INFO, "Detected Half-Life 2: Deathmatch pattern");
		return new CHalfLifeDeathmatchMod();
	}
	else if (std::strstr(szGameFolder, "synergy")) {
		logger->Log(LogLevel::INFO, "Detected Synergy pattern");
		return new CSynergyMod();
	}
	else if (std::strstr(szGameFolder, "bms") || std::strstr(szGameFolder, "blackmesa")) {
		logger->Log(LogLevel::INFO, "Detected Black Mesa pattern");
		// TODO: Implement Black Mesa mod class when available
		return new CBotMod(); // Generic fallback
	}

	logger->Log(LogLevel::WARN, "No specific game pattern detected for '%s', using generic bot", szGameFolder);
	return nullptr;
}

void CBotMods::readMods()
{
	// Enhanced game detection system with capability-based detection and fallback methods
	// Addresses roadmap goal: "Improve game detection for non-listed Source mods"

	logger->Log(LogLevel::INFO, "Initializing game detection system");

#if SOURCE_ENGINE == SE_TF2
	logger->Log(LogLevel::INFO, "Compiled for Team Fortress 2 (SE_TF2)");
	m_Mods.emplace_back(new CTeamFortress2Mod());
#elif SOURCE_ENGINE == SE_DODS
	logger->Log(LogLevel::INFO, "Compiled for Day of Defeat: Source (SE_DODS)");
	m_Mods.emplace_back(new CDODMod());
#elif SOURCE_ENGINE == SE_CSS
	logger->Log(LogLevel::INFO, "Compiled for Counter-Strike: Source (SE_CSS)");
	m_Mods.emplace_back(new CCounterStrikeSourceMod());
#elif SOURCE_ENGINE == SE_HL2DM
	logger->Log(LogLevel::INFO, "Compiled for Half-Life 2: Deathmatch (SE_HL2DM)");
	m_Mods.emplace_back(new CHalfLifeDeathmatchMod());
#elif SOURCE_ENGINE == SE_SDK2013
	logger->Log(LogLevel::INFO, "Compiled for SDK 2013 engine");

	// SDK 2013 supports multiple mods - attempt runtime detection
	const char* gameFolder = engine->GetGameDirectory();
	if (gameFolder) {
		logger->Log(LogLevel::INFO, "Game directory: %s", gameFolder);

		CBotMod* detectedMod = detectGameByCapabilities(gameFolder);
		if (detectedMod) {
			m_Mods.emplace_back(detectedMod);
		} else {
			// Default to Synergy for SDK 2013 if no specific detection
			logger->Log(LogLevel::INFO, "Defaulting to Synergy for SDK 2013");
			m_Mods.emplace_back(new CSynergyMod());
		}
	} else {
		logger->Log(LogLevel::WARN, "Could not retrieve game directory, defaulting to Synergy");
		m_Mods.emplace_back(new CSynergyMod());
	}
#else
	logger->Log(LogLevel::INFO, "Generic Source Engine build - attempting runtime detection");

	// Try to detect game at runtime for generic builds
	const char* gameFolder = engine->GetGameDirectory();
	if (gameFolder) {
		logger->Log(LogLevel::INFO, "Game directory: %s", gameFolder);

		CBotMod* detectedMod = detectGameByCapabilities(gameFolder);
		if (detectedMod) {
			m_Mods.emplace_back(detectedMod);
		}
	} else {
		logger->Log(LogLevel::WARN, "Could not retrieve game directory for detection");
	}

	// Add fallback mods for generic builds
	m_Mods.emplace_back(new CFortressForeverMod());
	m_Mods.emplace_back(new CHLDMSourceMod());

	// Look for extra MODs from configuration file
	logger->Log(LogLevel::INFO, "Parsing bot_mods.ini for additional mod configurations");
	parseFile();
#endif

	if (m_Mods.empty()) {
		logger->Log(LogLevel::FATAL, "No game mods were loaded! Bot cannot function.");
	} else {
		logger->Log(LogLevel::INFO, "Game detection complete. %zu mod(s) loaded.", m_Mods.size());
	}
}

//////////////////////////////////////////////////////////////////////////////

void CBotMod::setup(const char* szModFolder, const eModId iModId, const eBotType iBotType, const char* szWeaponListName)
{
	m_szModFolder = CStrings::getString(szModFolder);
	m_iModId = iModId;
	m_iBotType = iBotType;

	if (szWeaponListName && *szWeaponListName)
		m_szWeaponListName = CStrings::getString(szWeaponListName);
}

/*CBot *CBotMod :: makeNewBots ()
{
	return nullptr;
}*/

bool CBotMod::isModFolder(const char* szModFolder) const
{
	return FStrEq(m_szModFolder, szModFolder);
}

char* CBotMod::getModFolder() const
{
	return m_szModFolder;
}

eModId CBotMod::getModId() const
{
	return m_iModId;
}

//
// MOD LIST

std::vector<CBotMod*> CBotMods::m_Mods;

void CBotMods::freeMemory()
{
	for (CBotMod*& m_Mod : m_Mods)
	{
		m_Mod->freeMemory();
		delete m_Mod;
		m_Mod = nullptr;
	}

	m_Mods.clear();
}

CBotMod* CBotMods::getMod(char* szModFolder)
{
	logger->Log(LogLevel::INFO, "Looking for mod with folder: %s", szModFolder);

	// First, check if there's an exact match in loaded mods
	for (CBotMod* const& m_Mod : m_Mods)
	{
		if (m_Mod->isModFolder(szModFolder))
		{
			logger->Log(LogLevel::INFO, "HL2 MOD ID %d (Game Folder = %s) FOUND", m_Mod->getModId(), szModFolder);
			return m_Mod;
		}
	}

	// If no exact match, try partial matches for robustness
	logger->Log(LogLevel::WARN, "No exact match for '%s', attempting fuzzy matching", szModFolder);

	for (CBotMod* const& m_Mod : m_Mods)
	{
		const char* modFolder = m_Mod->getModFolder();
		if (modFolder && (std::strstr(szModFolder, modFolder) || std::strstr(modFolder, szModFolder)))
		{
			logger->Log(LogLevel::INFO, "Found partial match: MOD ID %d (Game Folder = %s) for requested '%s'",
				m_Mod->getModId(), modFolder, szModFolder);
			return m_Mod;
		}
	}

	// Last resort: try runtime detection
	logger->Log(LogLevel::WARN, "No loaded mod matches '%s', attempting runtime detection", szModFolder);

	CBotMod* detectedMod = detectGameByCapabilities(szModFolder);
	if (detectedMod) {
		logger->Log(LogLevel::INFO, "Runtime detection succeeded for '%s'", szModFolder);
		m_Mods.emplace_back(detectedMod);
		return detectedMod;
	}

	// Failed to find or detect mod
	logger->Log(LogLevel::FATAL,
		"HL2 MODIFICATION \"%s\" NOT FOUND OR COULD NOT BE DETECTED.\n"
		"Searched %zu loaded mod(s). Please check:\n"
		"  1. Game folder name is correct\n"
		"  2. bot_mods.ini in bot config folder has correct configuration\n"
		"  3. Bot is compiled for the correct Source Engine version\n"
		"Game folder received: %s\n"
		"Available mods:", szModFolder, m_Mods.size(), szModFolder);

	// List all loaded mods for debugging
	for (const CBotMod* const& m_Mod : m_Mods)
	{
		logger->Log(LogLevel::ERROR, "  - Mod ID %d: %s",
			m_Mod->getModId(), m_Mod->getModFolder() ? m_Mod->getModFolder() : "(null)");
	}

	return nullptr;
}

void CBotMod::initMod()
{
	m_bPlayerHasSpawned = false;

	CWeapons::loadWeapons(m_szWeaponListName, nullptr);
}

void CBotMod::mapInit()
{
	m_bPlayerHasSpawned = false;
}

bool CBotMod::playerSpawned(edict_t* pPlayer)
{
	if (m_bPlayerHasSpawned)
		return false;

	m_bPlayerHasSpawned = true;

	return true;
}

bool CHalfLifeDeathmatchMod::playerSpawned(edict_t* pPlayer)
{
	if (CBotMod::playerSpawned(pPlayer))
	{
		m_LiftWaypoints.clear();

		CWaypoints::updateWaypointPairs(&m_LiftWaypoints, CWaypointTypes::W_FL_LIFT, "func_button");
	}

	return true;
}

void CHalfLifeDeathmatchMod::initMod()
{
	CWeapons::loadWeapons(m_szWeaponListName == nullptr ? "HL2DM" : m_szWeaponListName, HL2DMWeaps.data());

	//	for ( i = 0; i < HL2DM_WEAPON_MAX; i ++ )
	//	CWeapons::addWeapon(new CWeapon(HL2DMWeaps[i]));//.iSlot,HL2DMWeaps[i].szWeaponName,HL2DMWeaps[i].iId,HL2DMWeaps[i].m_iFlags,HL2DMWeaps[i].m_iAmmoIndex,HL2DMWeaps[i].minPrimDist,HL2DMWeaps[i].maxPrimDist,HL2DMWeaps[i].m_iPreference,HL2DMWeaps[i].m_fProjSpeed));
}

void CHalfLifeDeathmatchMod::mapInit()
{
	CBotMod::mapInit();

	m_LiftWaypoints.clear();
}