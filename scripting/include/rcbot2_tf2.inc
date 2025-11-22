#if defined __rcbot_tf2_included
	#endinput
#endif

#define __rcbot_tf2_included

#include <rcbot2>

/**
 * TF2 Player Classes
 */
enum TFClassType {
	TFClass_Unknown = 0,
	TFClass_Scout,
	TFClass_Sniper,
	TFClass_Soldier,
	TFClass_Demoman,
	TFClass_Medic,
	TFClass_Heavy,
	TFClass_Pyro,
	TFClass_Spy,
	TFClass_Engineer
};

/**
 * TF2 Engineer Building Types
 */
enum TFObjectType {
	TFObject_Dispenser = 0,
	TFObject_Teleporter,
	TFObject_Sentry,
	TFObject_Sapper,
	TFObject_TeleporterExit,
	TFObject_TeleporterEntrance
};

/**
 * TF2 Engineer Commands
 */
enum TFEngiCmd {
	TFEngiCmd_Build = 0,
	TFEngiCmd_Destroy
};

//=============================================================================
// TF2 Class Management
//=============================================================================

/**
 * Sets a bot's TF2 class. Bot must be dead or will change on next spawn.
 *
 * @param client        Bot client index
 * @param class         TF2 class to set
 * @return              true on success, false on failure
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_TF2_SetClass(int client, TFClassType class);

/**
 * Gets a bot's current TF2 class.
 *
 * @param client        Bot client index
 * @return              Current TF2 class, or TFClass_Unknown on failure
 * @error               Invalid client index or client is not an RCBot
 */
native TFClassType RCBot2_TF2_GetClass(int client);

//=============================================================================
// TF2 Engineer Functions
//=============================================================================

/**
 * Commands an Engineer bot to build or destroy a building.
 *
 * @param client        Bot client index (must be Engineer)
 * @param object        Building type to build/destroy
 * @param command       Build or destroy command
 * @return              true on success, false on failure
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_TF2_EngineerBuild(int client, TFObjectType object, TFEngiCmd command);

/**
 * Checks if an Engineer bot has built a specific building type.
 *
 * @param client        Bot client index (must be Engineer)
 * @param object        Building type to check
 * @return              true if building exists, false otherwise
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_TF2_HasBuilding(int client, TFObjectType object);

/**
 * Gets the entity index of an Engineer's building.
 *
 * @param client        Bot client index (must be Engineer)
 * @param object        Building type to find
 * @return              Entity index of building, or -1 if not found
 * @error               Invalid client index or client is not an RCBot
 */
native int RCBot2_TF2_GetBuilding(int client, TFObjectType object);

//=============================================================================
// TF2 Medic Functions
//=============================================================================

/**
 * Sets a Medic bot's healing target.
 *
 * @param client        Bot client index (must be Medic)
 * @param target        Client index to heal, or -1 to clear
 * @return              true on success, false on failure
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_TF2_SetHealTarget(int client, int target);

/**
 * Gets a Medic bot's current healing target.
 *
 * @param client        Bot client index (must be Medic)
 * @return              Client index being healed, or -1 if none
 * @error               Invalid client index or client is not an RCBot
 */
native int RCBot2_TF2_GetHealTarget(int client);

/**
 * Gets a Medic bot's current Ubercharge percentage.
 *
 * @param client        Bot client index (must be Medic)
 * @return              Ubercharge percentage (0.0 to 100.0), or -1.0 on failure
 * @error               Invalid client index or client is not an RCBot
 */
native float RCBot2_TF2_GetUberCharge(int client);

//=============================================================================
// TF2 Spy Functions
//=============================================================================

/**
 * Commands a Spy bot to disguise as a specific class and team.
 *
 * @param client        Bot client index (must be Spy)
 * @param team          Team to disguise as (2 = RED, 3 = BLU)
 * @param class         Class to disguise as
 * @return              true on success, false on failure
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_TF2_Disguise(int client, int team, TFClassType class);

/**
 * Checks if a Spy bot is currently cloaked.
 *
 * @param client        Bot client index (must be Spy)
 * @return              true if cloaked, false otherwise
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_TF2_IsCloaked(int client);

/**
 * Checks if a Spy bot is currently disguised.
 *
 * @param client        Bot client index (must be Spy)
 * @return              true if disguised, false otherwise
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_TF2_IsDisguised(int client);

/**
 * Resets a Spy bot's cloak timer (for when they uncloak).
 *
 * @param client        Bot client index (must be Spy)
 * @return              true on success, false on failure
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_TF2_ResetCloakTime(int client);

//=============================================================================
// TF2 General Functions
//=============================================================================

/**
 * Commands a bot to call for Medic.
 *
 * @param client        Bot client index
 * @return              true on success, false on failure
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_TF2_CallMedic(int client);

/**
 * Commands a bot to perform a taunt.
 *
 * @param client        Bot client index
 * @param force         Force taunt even if conditions aren't met
 * @return              true on success, false on failure
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_TF2_Taunt(int client, bool force = false);

//=============================================================================
// TF2 Event Forwards
//=============================================================================

/**
 * Called when a TF2 bot places a building.
 *
 * @param client        Bot client index
 * @param buildingType  Type of building (see TFObjectType enum)
 * @param entity        Building entity index
 */
forward void RCBot2_TF2_OnBotBuildingPlaced(int client, int buildingType, int entity);

/**
 * Called when a TF2 bot's building is destroyed.
 *
 * @param client        Bot client index
 * @param buildingType  Type of building (see TFObjectType enum)
 * @param attacker      Attacker client index
 */
forward void RCBot2_TF2_OnBotBuildingDestroyed(int client, int buildingType, int attacker);

/**
 * Called when a TF2 medic bot deploys ubercharge.
 *
 * @param client        Medic bot client index
 * @param target        Heal target client index
 */
forward void RCBot2_TF2_OnBotUberDeployed(int client, int target);

/**
 * Called when a TF2 bot changes class.
 *
 * @param client        Bot client index
 * @param oldClass      Old TF2 class (see TFClass enum)
 * @param newClass      New TF2 class (see TFClass enum)
 */
forward void RCBot2_TF2_OnBotClassChanged(int client, int oldClass, int newClass);
