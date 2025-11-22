#if defined __rcbot_hldm_included
	#endinput
#endif

#define __rcbot_hldm_included

/**
 * RCBot2 HL2DM-Specific Natives
 *
 * These natives are only available when running Half-Life 2: Deathmatch.
 * They provide control over HL2DM-specific bot features like sprinting,
 * suit chargers, and armor management.
 */

//=============================================================================
// HL2DM Sprint Functions
//=============================================================================

/**
 * Commands a bot to start or stop sprinting.
 *
 * @param client        Bot client index
 * @param enable        true to start sprinting, false to stop
 * @return              true on success, false on failure
 * @error               Invalid client index or client is not an HL2DM RCBot
 */
native bool RCBot2_HLDM_UseSprint(int client, bool enable);

/**
 * Checks if a bot is currently sprinting.
 *
 * @param client        Bot client index
 * @return              true if sprinting, false otherwise
 * @error               Invalid client index or client is not an HL2DM RCBot
 */
native bool RCBot2_HLDM_IsSprinting(int client);

/**
 * Gets the bot's remaining sprint time/energy.
 *
 * @param client        Bot client index
 * @return              Remaining sprint time in seconds, or 0.0 on failure
 * @error               Invalid client index or client is not an HL2DM RCBot
 */
native float RCBot2_HLDM_GetSprintTime(int client);

//=============================================================================
// HL2DM Equipment Functions
//=============================================================================

/**
 * Commands a bot to use a health or suit charger.
 *
 * @param client        Bot client index
 * @param charger       Charger entity index
 * @return              true on success, false on failure
 * @error               Invalid client or charger entity index
 */
native bool RCBot2_HLDM_UseCharger(int client, int charger);

/**
 * Gets the bot's current armor as a percentage.
 *
 * @param client        Bot client index
 * @return              Armor percentage (0.0 - 1.0), or 0.0 on failure
 * @error               Invalid client index or client is not an HL2DM RCBot
 */
native float RCBot2_HLDM_GetArmorPercent(int client);
