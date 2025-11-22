#if defined __rcbot_dod_included
	#endinput
#endif

#define __rcbot_dod_included

/**
 * RCBot2 DOD:S-Specific Natives
 *
 * These natives are only available when running Day of Defeat: Source.
 * They provide control over DOD:S-specific bot features like bomb handling,
 * prone movement, and voice commands.
 */

// DOD:S Voice Command Enum (from game)
enum DODVoiceCommand {
	DOD_VC_GOGOGO = 0,
	DOD_VC_HOLD_POSITION,
	DOD_VC_FALL_BACK,
	DOD_VC_COVERING_FIRE,
	DOD_VC_MACHINEGUN_ATTACK,
	DOD_VC_BAZOOKA_ATTACK,
	DOD_VC_SNIPER_ATTACK,
	DOD_VC_FIRE_IN_THE_HOLE,
	DOD_VC_YES,
	DOD_VC_NO,
	DOD_VC_THANKS,
	DOD_VC_SORRY,
	DOD_VC_NEED_AMMO,
	DOD_VC_NEED_BACKUP,
	DOD_VC_NEED_MEDIC,
	DOD_VC_MOVE_OUT
};

//=============================================================================
// DOD:S Bomb Functions
//=============================================================================

/**
 * Checks if a bot is currently carrying a bomb.
 *
 * @param client        Bot client index
 * @return              true if bot has bomb, false otherwise
 * @error               Invalid client index or client is not a DOD:S RCBot
 */
native bool RCBot2_DOD_HasBomb(int client);

/**
 * Removes the bomb from a bot's inventory.
 *
 * @param client        Bot client index
 * @return              true on success, false on failure
 * @error               Invalid client index or client is not a DOD:S RCBot
 */
native bool RCBot2_DOD_RemoveBomb(int client);

//=============================================================================
// DOD:S Movement Functions
//=============================================================================

/**
 * Commands a bot to go prone.
 *
 * @param client        Bot client index
 * @return              true on success, false on failure
 * @error               Invalid client index or client is not a DOD:S RCBot
 */
native bool RCBot2_DOD_Prone(int client);

/**
 * Commands a bot to stand up from prone.
 *
 * @param client        Bot client index
 * @return              true on success, false on failure
 * @error               Invalid client index or client is not a DOD:S RCBot
 */
native bool RCBot2_DOD_UnProne(int client);

//=============================================================================
// DOD:S Weapon Functions
//=============================================================================

/**
 * Checks if a bot has a machine gun equipped.
 *
 * @param client        Bot client index
 * @return              true if bot has MG, false otherwise
 * @error               Invalid client index or client is not a DOD:S RCBot
 */
native bool RCBot2_DOD_HasMG(int client);

/**
 * Checks if a bot has a sniper rifle equipped.
 *
 * @param client        Bot client index
 * @return              true if bot has sniper rifle, false otherwise
 * @error               Invalid client index or client is not a DOD:S RCBot
 */
native bool RCBot2_DOD_HasSniperRifle(int client);

//=============================================================================
// DOD:S Communication Functions
//=============================================================================

/**
 * Commands a bot to issue a voice command.
 *
 * @param client        Bot client index
 * @param voiceCmd      Voice command to issue (see DODVoiceCommand enum)
 * @return              true on success, false on failure
 * @error               Invalid client index or client is not a DOD:S RCBot
 */
native bool RCBot2_DOD_VoiceCommand(int client, DODVoiceCommand voiceCmd);

//=============================================================================
// DOD:S Info Functions
//=============================================================================

/**
 * Gets the bot's current armor as a percentage.
 *
 * @param client        Bot client index
 * @return              Armor percentage (0.0 - 1.0), or 0.0 on failure
 * @error               Invalid client index or client is not a DOD:S RCBot
 */
native float RCBot2_DOD_GetArmorPercent(int client);
