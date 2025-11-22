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

//=============================================================================
// Phase 5: Squad & Team Coordination
//=============================================================================

/**
 * Creates a new bot squad with a leader.
 *
 * @param leader        Client index of squad leader
 * @return              Squad ID, or -1 on failure
 * @error               Invalid client index or client is not an RCBot
 */
native int RCBot2_CreateSquad(int leader);

/**
 * Destroys a squad and removes all members.
 *
 * @param leader        Leader client index of squad to destroy
 * @return              true on success, false on failure
 */
native bool RCBot2_DestroySquad(int leader);

/**
 * Adds a bot to a squad.
 *
 * @param client        Bot client index to add
 * @param leader        Squad leader client index
 * @return              true on success, false on failure
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_AddBotToSquad(int client, int leader);

/**
 * Removes a bot from its current squad.
 *
 * @param client        Bot client index
 * @return              true on success, false if bot wasn't in a squad
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_RemoveBotFromSquad(int client);

/**
 * Gets the squad leader for a bot.
 *
 * @param client        Bot client index
 * @return              Leader client index, or -1 if not in a squad
 * @error               Invalid client index or client is not an RCBot
 */
native int RCBot2_GetBotSquadLeader(int client);

/**
 * Gets the number of members in a squad.
 *
 * @param leader        Squad leader client index
 * @return              Number of members, or 0 if no squad
 */
native int RCBot2_GetSquadMemberCount(int leader);

/**
 * Gets all members of a squad.
 *
 * @param leader        Squad leader client index
 * @param members       Array to store member client indices
 * @param maxsize       Maximum size of array
 * @return              Number of members written to array
 */
native int RCBot2_GetSquadMembers(int leader, int[] members, int maxsize);

/**
 * Checks if a bot is in a squad.
 *
 * @param client        Bot client index
 * @return              true if in a squad, false otherwise
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_IsInSquad(int client);

//=============================================================================
// Phase 6: Advanced Bot Management
//=============================================================================

/**
 * Kicks a bot from the server.
 *
 * @param client        Bot client index to kick
 * @return              true on success, false on failure
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_KickBot(int client);

/**
 * Counts the number of RCBots on the server.
 *
 * @param team          Team to count (-1 for all teams)
 * @return              Number of bots
 */
native int RCBot2_CountBots(int team = -1);

/**
 * Gets a bot client index by iteration index.
 *
 * @param index         Bot index (0 to RCBot2_CountBots()-1)
 * @return              Client index, or -1 if invalid index
 */
native int RCBot2_GetBotByIndex(int index);

/**
 * Gets the name of a bot.
 *
 * @param client        Bot client index
 * @param name          Buffer to store name
 * @param maxlength     Maximum buffer size
 * @return              true on success, false on failure
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_GetBotName(int client, char[] name, int maxlength);

//=============================================================================
// Phase 7: Perception & AI Configuration
//=============================================================================

/**
 * Bot condition flags (bit flags)
 */
enum RCBotCondition {
	RCBotCond_EnemyObscured = (1 << 0),      // Bot lost sight of enemy
	RCBotCond_NoWeapon = (1 << 1),           // Bot doesn't have a weapon
	RCBotCond_OutOfAmmo = (1 << 2),          // Bot has no ammo
	RCBotCond_SeeCurrentEnemy = (1 << 3),    // Bot can see current enemy
	RCBotCond_EnemyDead = (1 << 4),          // Bot's enemy is dead
	RCBotCond_SeeWaypoint = (1 << 5),        // Bot can see the current waypoint
	RCBotCond_NeedAmmo = (1 << 6),           // Bot needs ammo (low)
	RCBotCond_NeedHealth = (1 << 7),         // Bot needs health
	RCBotCond_SeeLookVector = (1 << 8),      // Bot can see his 'look' vector
	RCBotCond_PointCaptured = (1 << 9),      // A point has been captured recently
	RCBotCond_Push = (1 << 10),              // Bots more likely to attack
	RCBotCond_Lift = (1 << 11),              // Bot is on a lift
	RCBotCond_SeePlayerToHelp = (1 << 12),   // See player to heal
	RCBotCond_SeeLastEnemyPos = (1 << 13),   // Can see last enemy position
	RCBotCond_Changed = (1 << 14),           // Bot wants to change action
	RCBotCond_Covert = (1 << 15),            // Bot is being covert
	RCBotCond_Run = (1 << 16),               // Bot needs to run (danger nearby)
	RCBotCond_Grenade = (1 << 17),           // Bot will throw grenade
	RCBotCond_NeedBomb = (1 << 18),          // Bot needs a bomb
	RCBotCond_SeeEnemyHead = (1 << 19),      // Bot can aim for headshot
	RCBotCond_Prone = (1 << 20),             // Bot needs to go prone
	RCBotCond_Paranoid = (1 << 21),          // Bot is paranoid of spies
	RCBotCond_SeeSquadLeader = (1 << 22),    // Bot can see squad leader
	RCBotCond_SquadLeaderDead = (1 << 23),   // Squad leader is dead
	RCBotCond_SquadLeaderInRange = (1 << 24),// Squad leader in range
	RCBotCond_SquadIdle = (1 << 25),         // Squad isn't doing anything
	RCBotCond_Defensive = (1 << 26),         // Bot told to defend
	RCBotCond_BuildingSapped = (1 << 27),    // Engineer building sapped
	RCBotCond_SeeEnemyGround = (1 << 28)     // Can see enemy ground for explosives
};

/**
 * Sets the bot's field of view angle.
 *
 * @param client        Bot client index
 * @param fov           Field of view in degrees (default 80.0)
 * @return              true on success, false on failure
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_SetBotFOV(int client, float fov);

/**
 * Gets the bot's field of view angle.
 *
 * @param client        Bot client index
 * @return              Field of view in degrees
 * @error               Invalid client index or client is not an RCBot
 */
native float RCBot2_GetBotFOV(int client);

/**
 * Gets all visible enemies for a bot.
 *
 * @param client        Bot client index
 * @param enemies       Array to store enemy client indices
 * @param maxsize       Maximum size of array
 * @return              Number of enemies written to array
 * @error               Invalid client index or client is not an RCBot
 */
native int RCBot2_GetVisibleEnemies(int client, int[] enemies, int maxsize);

/**
 * Gets all nearby allies within a radius.
 *
 * @param client        Bot client index
 * @param allies        Array to store ally client indices
 * @param maxsize       Maximum size of array
 * @param radius        Search radius in units
 * @return              Number of allies written to array
 * @error               Invalid client index or client is not an RCBot
 */
native int RCBot2_GetNearbyAllies(int client, int[] allies, int maxsize, float radius);

/**
 * Sets a condition flag on a bot.
 *
 * @param client        Bot client index
 * @param condition     RCBotCondition flag to set
 * @return              true on success, false on failure
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_SetCondition(int client, int condition);

/**
 * Removes a condition flag from a bot.
 *
 * @param client        Bot client index
 * @param condition     RCBotCondition flag to remove
 * @return              true on success, false on failure
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_RemoveCondition(int client, int condition);

/**
 * Checks if a bot has a specific condition.
 *
 * @param client        Bot client index
 * @param condition     RCBotCondition flag to check
 * @return              true if condition is set, false otherwise
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_HasCondition(int client, int condition);

/**
 * Gets all active conditions for a bot.
 *
 * @param client        Bot client index
 * @param conditions    Array to store active condition flags
 * @param maxsize       Maximum size of array
 * @return              Number of conditions written to array
 * @error               Invalid client index or client is not an RCBot
 */
native int RCBot2_GetConditions(int client, int[] conditions, int maxsize);

//=============================================================================
// Phase 6: Extended Bot Management
//=============================================================================

/**
 * Saves a bot's profile to a file.
 *
 * @param client        Bot client index
 * @param filename      Profile filename to save
 * @return              true on success, false on failure
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_SaveProfile(int client, const char[] filename);

/**
 * Loads a bot's profile from a file.
 *
 * @param client        Bot client index
 * @param filename      Profile filename to load
 * @return              true on success, false on failure
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_LoadProfile(int client, const char[] filename);

/**
 * Resets a bot's profile to default values.
 *
 * @param client        Bot client index
 * @return              true on success, false on failure
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_ResetProfile(int client);

/**
 * Gets the bot's kill count.
 *
 * @param client        Bot client index
 * @return              Number of kills, or 0 on failure
 * @error               Invalid client index or client is not an RCBot
 */
native int RCBot2_GetBotKills(int client);

/**
 * Gets the bot's death count.
 *
 * @param client        Bot client index
 * @return              Number of deaths, or 0 on failure
 * @error               Invalid client index or client is not an RCBot
 */
native int RCBot2_GetBotDeaths(int client);

//=============================================================================
// Phase 7: Extended Perception & AI Configuration
//=============================================================================

/**
 * Sets a weapon preference for a bot.
 *
 * @param client        Bot client index
 * @param weapon        Weapon name
 * @param preference    Preference value (0.0 - 1.0)
 * @return              true on success, false on failure
 * @error               Invalid client index or client is not an RCBot
 */
native bool RCBot2_SetWeaponPreference(int client, const char[] weapon, float preference);

/**
 * Gets a weapon preference for a bot.
 *
 * @param client        Bot client index
 * @param weapon        Weapon name
 * @return              Preference value (0.0 - 1.0), or 1.0 on failure
 * @error               Invalid client index or client is not an RCBot
 */
native float RCBot2_GetWeaponPreference(int client, const char[] weapon);

//=============================================================================
// Event Forwards
//=============================================================================

/**
 * Called when a bot spawns.
 *
 * @param client        Bot client index
 */
forward void RCBot2_OnBotSpawn(int client);

/**
 * Called when a bot dies.
 *
 * @param client        Bot client index
 * @param attacker      Attacker client index
 */
forward void RCBot2_OnBotDeath(int client, int attacker);

/**
 * Called when a bot kills someone.
 *
 * @param client        Bot client index
 * @param victim        Victim client index
 */
forward void RCBot2_OnBotKill(int client, int victim);

/**
 * Called during bot think cycle.
 *
 * @param client        Bot client index
 */
forward void RCBot2_OnBotThink(int client);

/**
 * Called when a bot acquires a new enemy.
 *
 * @param client        Bot client index
 * @param enemy         Enemy client index
 */
forward void RCBot2_OnBotEnemyFound(int client, int enemy);

/**
 * Called when a bot loses its enemy.
 *
 * @param client        Bot client index
 * @param enemy         Enemy client index
 */
forward void RCBot2_OnBotEnemyLost(int client, int enemy);

/**
 * Called when a bot takes damage.
 *
 * @param client        Bot client index
 * @param attacker      Attacker client index
 * @param damage        Damage amount
 */
forward void RCBot2_OnBotDamaged(int client, int attacker, int damage);

/**
 * Called when a bot's task/schedule changes.
 *
 * @param client        Bot client index
 * @param oldTask       Old schedule ID
 * @param newTask       New schedule ID
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
