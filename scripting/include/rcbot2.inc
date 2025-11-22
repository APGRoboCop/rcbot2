#if defined __rcbot_included
	#endinput
#endif

#define __rcbot_included

enum RCBotProfileVar {
	// speed of recognizing non-player entities (NPCs / teleporters)
	RCBotProfile_iVisionTicks,
	
	// speed of finding a path
	RCBotProfile_iPathTicks,
	
	// speed of recognizing other players
	RCBotProfile_iVisionTicksClients,
	
	// sensitivity of bot's "mouse"; integer between 1 and 20
	RCBotProfile_iSensitivity,
	
	// sensitivity to danger (brave = less sensitive); float between 0.0 to 1.0
	RCBotProfile_fBraveness,
	
	// ability to predict player movements; float between 0.0 to 1.0
	RCBotProfile_fAimSkill,
	
	// preferred player class
	RCBotProfile_iClass,
};

/**
 * Returns true if a waypoint is available for the current map.
 */
native bool RCBot2_IsWaypointAvailable();

/**
 * Instantiates an RCBot2 bot for the current mod with a copy of the default profile.
 * 
 * @return RCBot2 client index, or -1 on failure
 */
native int RCBot2_CreateBot(const char[] name);

/**
 * Returns true if the client index is an RCBot2 bot.  This does not currently true until after
 * the bot is in the game.
 * 
 * @error Client not in game or invalid client index.
 */
native int IsRCBot2Client(int client);

/**
 * Sets the profile property to the given integer value.
 * 
 * @noreturn
 * @error Client is not an RCBot2 bot or specified property is not an integer value
 */
native void RCBot2_SetProfileInt(int client, RCBotProfileVar property, int value);

/**
 * Gets the float value for a given bot's profile property.
 * 
 * @error Client is not an RCBot2 bot or specified property is not an integer value
 */
native int RCBot2_GetProfileInt(int client, RCBotProfileVar property);

/**
 * Sets the profile property to the given float value.
 * 
 * @noreturn
 * @error Client is not an RCBot2 bot or specified property is not a float value
 */
native void RCBot2_SetProfileFloat(int client, RCBotProfileVar property, float value);

/**
 * Gets the float value for a given bot's profile property.
 *
 * @error Client is not an RCBot2 bot or specified property is not a float value
 */
native float RCBot2_GetProfileFloat(int client, RCBotProfileVar property);

//=============================================================================
// Phase 1: Enhanced SourceMod Integration
//=============================================================================

/**
 * Bot schedule types (read-only, for task queries)
 */
enum RCBotSchedule {
	RCBotSchedule_None = 0,
	RCBotSchedule_Attack,
	RCBotSchedule_RunForCover,
	RCBotSchedule_GotoOrigin,
	RCBotSchedule_GoodHideSpot,
	RCBotSchedule_GetHealth,
	RCBotSchedule_Build,
	RCBotSchedule_Heal,
	RCBotSchedule_GetMetal,
	RCBotSchedule_Snipe,
	RCBotSchedule_Upgrade,
	RCBotSchedule_UseTele,
	RCBotSchedule_SapBuilding,
	RCBotSchedule_UseDispenser,
	RCBotSchedule_Pickup,
	RCBotSchedule_GetAmmo,
	RCBotSchedule_FindFlag,
	RCBotSchedule_LookAfterSentry,
	RCBotSchedule_Defend,
	RCBotSchedule_AttackPoint,
	RCBotSchedule_DefendPoint,
	RCBotSchedule_Backstab,
	RCBotSchedule_RemoveSapper,
	RCBotSchedule_Follow,
	RCBotSchedule_InvestigateNoise,
	RCBotSchedule_Other = 255
};

/**
 * Bot actions that can be forced
 */
enum RCBotAction {
	RCBotAction_Jump = 0,
	RCBotAction_Crouch,
	RCBotAction_Attack1,
	RCBotAction_Attack2,
	RCBotAction_Reload,
	RCBotAction_Use
};

//=============================================================================
// Phase 1: Bot Command & Control
//=============================================================================

/**
 * Forces a bot to target a specific enemy.
 *
 * @param client        Bot client index
 * @param target        Target client index, or -1 to clear
 * @return              true on success, false on failure
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_SetBotEnemy(int client, int target);

/**
 * Gets the bot's current enemy target.
 *
 * @param client        Bot client index
 * @return              Enemy client index, or -1 if no enemy
 * @error               Invalid client index or client is not an RCBot
 */
native int RCBot2_GetBotEnemy(int client);

/**
 * Forces a bot to perform a specific action.
 *
 * @param client        Bot client index
 * @param action        Action to perform
 * @return              true on success, false on failure
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_ForceBotAction(int client, RCBotAction action);

/**
 * Gets the bot's current team.
 *
 * @param client        Bot client index
 * @return              Team index, or -1 on failure
 * @error               Invalid client index or client is not an RCBot
 */
native int RCBot2_GetBotTeam(int client);

/**
 * Gets the bot's current health.
 *
 * @param client        Bot client index
 * @return              Health value, or -1 on failure
 * @error               Invalid client index or client is not an RCBot
 */
native int RCBot2_GetBotHealth(int client);

/**
 * Gets the bot's current position.
 *
 * @param client        Bot client index
 * @param origin        Array to store position
 * @return              true on success, false on failure
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_GetBotOrigin(int client, float origin[3]);

/**
 * Gets the bot's current eye angles.
 *
 * @param client        Bot client index
 * @param angles        Array to store angles
 * @return              true on success, false on failure
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_GetBotEyeAngles(int client, float angles[3]);

/**
 * Commands the bot to move to a specific location.
 *
 * @param client        Bot client index
 * @param origin        Target position
 * @return              true on success, false on failure
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_GotoOrigin(int client, const float origin[3]);

/**
 * Stops the bot's current movement.
 *
 * @param client        Bot client index
 * @return              true on success, false on failure
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_StopMovement(int client);

//=============================================================================
// Phase 1: Weapon & Equipment Management
//=============================================================================

/**
 * Forces a bot to select a specific weapon by classname.
 *
 * @param client        Bot client index
 * @param weapon        Weapon classname (e.g., "weapon_crowbar")
 * @return              true on success, false on failure
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_SelectWeapon(int client, const char[] weapon);

/**
 * Gets the bot's currently selected weapon classname.
 *
 * @param client        Bot client index
 * @param weapon        Buffer to store weapon classname
 * @param maxlength     Maximum buffer size
 * @return              true on success, false on failure
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_GetCurrentWeapon(int client, char[] weapon, int maxlength);

/**
 * Forces a bot to attack with current weapon.
 *
 * @param client        Bot client index
 * @param primary       true for primary fire, false for secondary
 * @param duration      How long to hold attack (in seconds)
 * @return              true on success, false on failure
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_ForceAttack(int client, bool primary = true, float duration = 0.1);

/**
 * Forces a bot to reload current weapon.
 *
 * @param client        Bot client index
 * @return              true on success, false on failure
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_ForceReload(int client);

//=============================================================================
// Phase 1: Task System Queries
//=============================================================================

/**
 * Gets the bot's current schedule type.
 *
 * @param client        Bot client index
 * @return              Current RCBotSchedule, or RCBotSchedule_None on failure
 * @error               Invalid client index or client is not an RCBot
 */
native RCBotSchedule RCBot2_GetCurrentSchedule(int client);

/**
 * Clears all of a bot's scheduled tasks.
 *
 * @param client        Bot client index
 * @return              true on success, false on failure
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_ClearSchedule(int client);

/**
 * Checks if the bot has a specific schedule active.
 *
 * @param client        Bot client index
 * @param schedule      Schedule type to check for
 * @return              true if bot has this schedule, false otherwise
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_HasSchedule(int client, RCBotSchedule schedule);

//=============================================================================
// Phase 3: Navigation & Pathfinding
//=============================================================================

/**
 * Gets the total number of waypoints loaded for the current map.
 *
 * @return              Number of waypoints, or 0 if none loaded
 */
native int RCBot2_GetWaypointCount();

/**
 * Finds the nearest waypoint to a position.
 *
 * @param origin        Position to search from
 * @param maxdist       Maximum search distance (0 = unlimited)
 * @return              Waypoint index, or -1 if none found
 */
native int RCBot2_GetNearestWaypoint(const float origin[3], float maxdist = 0.0);

/**
 * Gets a waypoint's position.
 *
 * @param waypointid    Waypoint index
 * @param origin        Array to store waypoint position
 * @return              true on success, false if waypoint invalid
 */
native bool RCBot2_GetWaypointOrigin(int waypointid, float origin[3]);

/**
 * Gets a waypoint's flags (properties like jump, crouch, ladder, etc).
 *
 * @param waypointid    Waypoint index
 * @return              Waypoint flags bitfield, or 0 if invalid
 */
native int RCBot2_GetWaypointFlags(int waypointid);

/**
 * Checks if a bot currently has a navigation path.
 *
 * @param client        Bot client index
 * @return              true if bot has a path, false otherwise
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_HasPath(int client);

/**
 * Gets a bot's current navigation goal position.
 *
 * @param client        Bot client index
 * @param origin        Array to store goal position
 * @return              true on success, false if no goal set
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_GetGoalOrigin(int client, float origin[3]);

/**
 * Gets the waypoint ID the bot is currently moving towards.
 *
 * @param client        Bot client index
 * @return              Waypoint index, or -1 if no current waypoint
 * @error               Invalid client index or client is not an RCBot
 */
native int RCBot2_GetCurrentWaypointID(int client);

/**
 * Gets the waypoint ID of the bot's final navigation goal.
 *
 * @param client        Bot client index
 * @return              Goal waypoint index, or -1 if no goal
 * @error               Invalid client index or client is not an RCBot
 */
native int RCBot2_GetGoalWaypointID(int client);

/**
 * Clears the bot's current navigation path.
 *
 * @param client        Bot client index
 * @return              true on success, false on failure
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_ClearPath(int client);

/**
 * Checks if the bot is currently stuck (unable to move).
 *
 * @param client        Bot client index
 * @return              true if stuck, false otherwise
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_IsStuck(int client);

//=============================================================================
// Phase 4: Event System & Callbacks
//=============================================================================

/**
 * Called when a bot spawns in the game.
 *
 * @param client        Bot client index
 */
forward void RCBot2_OnBotSpawn(int client);

/**
 * Called when a bot dies.
 *
 * @param client        Bot client index
 * @param attacker      Attacker client index, or 0 if world/suicide
 */
forward void RCBot2_OnBotDeath(int client, int attacker);

/**
 * Called when a bot kills another player.
 *
 * @param client        Bot client index
 * @param victim        Victim client index
 */
forward void RCBot2_OnBotKill(int client, int victim);

/**
 * Called every time a bot thinks (processes AI logic).
 * WARNING: This fires very frequently! Use sparingly.
 *
 * @param client        Bot client index
 */
forward void RCBot2_OnBotThink(int client);

/**
 * Called when a bot acquires a new enemy target.
 *
 * @param client        Bot client index
 * @param enemy         Enemy client index
 */
forward void RCBot2_OnBotEnemyFound(int client, int enemy);

/**
 * Called when a bot loses track of an enemy.
 *
 * @param client        Bot client index
 * @param enemy         Enemy client index that was lost
 */
forward void RCBot2_OnBotEnemyLost(int client, int enemy);

/**
 * Called when a bot takes damage.
 *
 * @param client        Bot client index
 * @param attacker      Attacker client index, or 0 if world
 * @param damage        Amount of damage taken
 */
forward void RCBot2_OnBotDamaged(int client, int attacker, int damage);

/**
 * Called when a bot's active task/schedule changes.
 *
 * @param client        Bot client index
 * @param oldTask       Previous RCBotSchedule
 * @param newTask       New RCBotSchedule
 */
forward void RCBot2_OnBotTaskChange(int client, int oldTask, int newTask);

public Extension __ext_rcbot2 = {
	name = "RCBot2",
	file = "rcbot2.ext",
#if defined AUTOLOAD_EXTENSIONS
	autoload = 1,
#else
	autoload = 0,
#endif
#if defined REQUIRE_EXTENSIONS
	required = 1,
#else
	required = 0,
#endif
};

#if !defined REQUIRE_EXTENSIONS
public void __ext_rcbot2_SetNTVOptional() {
}
#endif
