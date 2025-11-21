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

#ifndef __BOT_GAMEMODE_CONFIG_H__
#define __BOT_GAMEMODE_CONFIG_H__

#include <vector>
#include <string>
#include <map>

/**
 * Gamemode detection method priority
 */
enum class GamemodeDetectionMethod : std::uint8_t
{
	ENTITY = 0,   // Detect by map entities (highest priority)
	MAP = 1,      // Detect by map name/prefix (medium priority)
	FILE = 2      // Detect by file existence (lowest priority, future)
};

/**
 * Special gamemode properties for edge cases
 */
struct GamemodeSpecialProperties
{
	bool payload_only = false;
	bool custom_flag_handling = false;
	int min_entity_index = 0;
	int max_entity_index = 0;

	bool hasSpecialProperties() const {
		return payload_only || custom_flag_handling ||
		       min_entity_index > 0 || max_entity_index > 0;
	}
};

/**
 * Gamemode definition from config
 */
struct GamemodeDefinition
{
	std::string name;              // Gamemode name (e.g., "CTF")
	std::uint8_t type;            // Gamemode type constant (e.g., TF_MAP_CTF)
	int priority;                  // Detection priority (higher = checked first)

	std::vector<std::string> entities;  // Entity classnames to detect
	std::vector<std::string> maps;      // Map name prefixes to detect
	std::vector<std::string> files;     // Files to detect (future)

	GamemodeSpecialProperties special;  // Special properties

	GamemodeDefinition() : type(0), priority(0) {}
};

/**
 * Gamemode configuration parser and manager
 */
class CGamemodeConfig
{
public:
	CGamemodeConfig();
	~CGamemodeConfig();

	/**
	 * Load gamemode configuration from file
	 * @param filename Path to config file
	 * @return True if loaded successfully
	 */
	bool load(const char* filename);

	/**
	 * Detect gamemode by map name
	 * @param mapname Name of the current map
	 * @return Gamemode type, or 0 if not detected
	 */
	std::uint8_t detectByMapName(const char* mapname);

	/**
	 * Detect gamemode by entity existence
	 * @param entityClassname Entity classname to check
	 * @return Gamemode type, or 0 if not detected
	 */
	std::uint8_t detectByEntity(const char* entityClassname);

	/**
	 * Get gamemode definition by type
	 * @param type Gamemode type constant
	 * @return Pointer to definition, or nullptr if not found
	 */
	const GamemodeDefinition* getDefinition(std::uint8_t type) const;

	/**
	 * Get all gamemode definitions sorted by priority
	 * @return Vector of gamemode definitions
	 */
	const std::vector<GamemodeDefinition>& getDefinitions() const {
		return m_gamemodes;
	}

	/**
	 * Check if config is loaded
	 * @return True if config has been loaded
	 */
	bool isLoaded() const {
		return m_bLoaded;
	}

	/**
	 * Clear all gamemode definitions
	 */
	void clear();

	/**
	 * Enable or disable fallback to hardcoded detection
	 * @param enable True to enable fallback
	 */
	void setHardcodedFallback(bool enable) {
		m_bHardcodedFallback = enable;
	}

	/**
	 * Check if hardcoded fallback is enabled
	 * @return True if fallback enabled
	 */
	bool isHardcodedFallbackEnabled() const {
		return m_bHardcodedFallback;
	}

private:
	std::vector<GamemodeDefinition> m_gamemodes;  // All gamemode definitions
	std::map<std::uint8_t, size_t> m_typeToIndex; // Quick lookup by type
	bool m_bLoaded;                                // Config loaded flag
	bool m_bHardcodedFallback;                    // Use hardcoded detection as fallback

	/**
	 * Parse a single gamemode section from config
	 * @param lines Config file lines
	 * @param startLine Starting line index
	 * @param outDef Output gamemode definition
	 * @return Number of lines parsed
	 */
	int parseGamemodeSection(const std::vector<std::string>& lines,
	                         size_t startLine,
	                         GamemodeDefinition& outDef);

	/**
	 * Parse gamemode type string to constant
	 * @param typeStr Type string (e.g., "TF_MAP_CTF")
	 * @return Type constant, or 0 if invalid
	 */
	std::uint8_t parseGamemodeType(const char* typeStr);

	/**
	 * Trim whitespace from string
	 * @param str String to trim
	 * @return Trimmed string
	 */
	static std::string trim(const std::string& str);

	/**
	 * Split string by delimiter
	 * @param str String to split
	 * @param delimiter Delimiter character
	 * @return Vector of split strings
	 */
	static std::vector<std::string> split(const std::string& str, char delimiter);
};

/**
 * Global gamemode config instance
 */
extern CGamemodeConfig g_GamemodeConfig;

#endif // __BOT_GAMEMODE_CONFIG_H__
