#if defined __rcbot_hldm_included
	#endinput
#endif

#define __rcbot_hldm_included

/**
 * RCBot2 HL2DM-Specific Natives
 *
 * These natives are only available when running Half-Life 2: Deathmatch.
 * They provide comprehensive control over HL2DM-specific bot features including:
 * - Sprint mechanics and suit power management
 * - Gravity Gun physics manipulation
 * - Equipment interaction (chargers, batteries, health kits, ammo crates)
 * - Weapon and ammo management
 * - Grenade throwing and combat utilities
 * - Physics object and breakable interaction
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
 * Gets the bot's sprint cooldown time.
 *
 * @param client        Bot client index
 * @return              Time remaining until bot can sprint again, or 0.0 if ready
 * @error               Invalid client index or client is not an HL2DM RCBot
 */
native float RCBot2_HLDM_GetSprintTime(int client);

//=============================================================================
// HL2DM Equipment Functions
//=============================================================================

/**
 * Commands a bot to use an armor (suit) charger.
 *
 * @param client        Bot client index
 * @param charger       Charger entity index
 * @return              true on success, false on failure
 * @error               Invalid client or charger entity index
 */
native bool RCBot2_HLDM_UseCharger(int client, int charger);

/**
 * Commands a bot to use a health charger.
 *
 * @param client        Bot client index
 * @param charger       Health charger entity index
 * @return              true on success, false on failure
 * @error               Invalid client or charger entity index
 */
native bool RCBot2_HLDM_UseHealthCharger(int client, int charger);

/**
 * Gets the bot's current armor as a percentage.
 *
 * @param client        Bot client index
 * @return              Armor percentage (0.0 - 1.0), or 0.0 on failure
 * @error               Invalid client index or client is not an HL2DM RCBot
 */
native float RCBot2_HLDM_GetArmorPercent(int client);

//=============================================================================
// HL2DM Gravity Gun Functions
//=============================================================================

/**
 * Checks if a bot is currently carrying an object with the Gravity Gun.
 *
 * @param client        Bot client index
 * @return              true if carrying an object, false otherwise
 * @error               Invalid client index or client is not an HL2DM RCBot
 */
native bool RCBot2_HLDM_IsCarryingObject(int client);

/**
 * Gets the entity index of the object the bot is carrying with the Gravity Gun.
 *
 * @param client        Bot client index
 * @return              Entity index of carried object, or -1 if none
 * @error               Invalid client index or client is not an HL2DM RCBot
 */
native int RCBot2_HLDM_GetCarriedObject(int client);

/**
 * Commands a bot to pick up an object with the Gravity Gun.
 *
 * @param client        Bot client index
 * @param object        Physics object entity index
 * @return              true on success, false on failure
 * @error               Invalid client or object entity index
 */
native bool RCBot2_HLDM_PickupObject(int client, int object);

/**
 * Commands a bot to drop the currently carried Gravity Gun object.
 *
 * @param client        Bot client index
 * @return              true on success, false if not carrying an object
 * @error               Invalid client index or client is not an HL2DM RCBot
 */
native bool RCBot2_HLDM_DropObject(int client);

/**
 * Commands a bot to launch the currently carried Gravity Gun object.
 *
 * @param client        Bot client index
 * @return              true on success, false if not carrying an object
 * @error               Invalid client index or client is not an HL2DM RCBot
 */
native bool RCBot2_HLDM_LaunchObject(int client);

//=============================================================================
// HL2DM Physics & Breakables Functions
//=============================================================================

/**
 * Gets the entity index of the nearest physics object to the bot.
 *
 * @param client        Bot client index
 * @return              Entity index of nearest physics object, or -1 if none
 * @error               Invalid client index or client is not an HL2DM RCBot
 */
native int RCBot2_HLDM_GetNearestPhysicsObject(int client);

/**
 * Gets the entity index of the nearest breakable object to the bot.
 *
 * @param client        Bot client index
 * @return              Entity index of nearest breakable, or -1 if none
 * @error               Invalid client index or client is not an HL2DM RCBot
 */
native int RCBot2_HLDM_GetNearestBreakable(int client);

/**
 * Marks an object as failed, preventing the bot from attempting to interact with it.
 *
 * @param client        Bot client index
 * @param object        Physics object entity index to mark as failed
 * @return              true on success, false on failure
 * @error               Invalid client or object entity index
 */
native bool RCBot2_HLDM_SetFailedObject(int client, int object);

/**
 * Gets the entity index of the object marked as failed for this bot.
 *
 * @param client        Bot client index
 * @return              Entity index of failed object, or -1 if none
 * @error               Invalid client index or client is not an HL2DM RCBot
 */
native int RCBot2_HLDM_GetFailedObject(int client);

//=============================================================================
// HL2DM Item/Pickup Functions
//=============================================================================

/**
 * Gets the entity index of the nearest weapon to the bot.
 *
 * @param client        Bot client index
 * @return              Entity index of nearest weapon, or -1 if none
 * @error               Invalid client index or client is not an HL2DM RCBot
 */
native int RCBot2_HLDM_GetNearestWeapon(int client);

/**
 * Gets the entity index of the nearest health kit to the bot.
 *
 * @param client        Bot client index
 * @return              Entity index of nearest health kit, or -1 if none
 * @error               Invalid client index or client is not an HL2DM RCBot
 */
native int RCBot2_HLDM_GetNearestHealthKit(int client);

/**
 * Gets the entity index of the nearest battery (armor pickup) to the bot.
 *
 * @param client        Bot client index
 * @return              Entity index of nearest battery, or -1 if none
 * @error               Invalid client index or client is not an HL2DM RCBot
 */
native int RCBot2_HLDM_GetNearestBattery(int client);

/**
 * Gets the entity index of the nearest ammo crate to the bot.
 *
 * @param client        Bot client index
 * @return              Entity index of nearest ammo crate, or -1 if none
 * @error               Invalid client index or client is not an HL2DM RCBot
 */
native int RCBot2_HLDM_GetNearestAmmoCrate(int client);

/**
 * Commands a bot to use the nearest ammo crate.
 *
 * @param client        Bot client index
 * @return              true on success, false if no crate nearby
 * @error               Invalid client index or client is not an HL2DM RCBot
 */
native bool RCBot2_HLDM_UseAmmoCrate(int client);

//=============================================================================
// HL2DM Weapon Management Functions
//=============================================================================

/**
 * Gets the entity index of the bot's currently equipped weapon.
 *
 * @param client        Bot client index
 * @return              Entity index of current weapon, or -1 if none
 * @error               Invalid client index or client is not an HL2DM RCBot
 */
native int RCBot2_HLDM_GetCurrentWeapon(int client);

/**
 * Gets the primary clip ammo count for the bot's current weapon.
 *
 * @param client        Bot client index
 * @return              Primary clip ammo count, or -1 on failure
 * @error               Invalid client index or client is not an HL2DM RCBot
 */
native int RCBot2_HLDM_GetWeaponClip1(int client);

/**
 * Gets the secondary clip ammo count for the bot's current weapon.
 *
 * @param client        Bot client index
 * @return              Secondary clip ammo count, or -1 on failure
 * @error               Invalid client index or client is not an HL2DM RCBot
 */
native int RCBot2_HLDM_GetWeaponClip2(int client);

/**
 * Checks if a bot has the Gravity Gun weapon.
 *
 * @param client        Bot client index
 * @return              true if bot has Gravity Gun, false otherwise
 * @error               Invalid client index or client is not an HL2DM RCBot
 */
native bool RCBot2_HLDM_HasGravityGun(int client);

//=============================================================================
// HL2DM Combat Functions
//=============================================================================

/**
 * Commands a bot to throw a grenade.
 *
 * @param client        Bot client index
 * @return              true on success, false if bot has no grenades
 * @error               Invalid client index or client is not an HL2DM RCBot
 */
native bool RCBot2_HLDM_ThrowGrenade(int client);

/**
 * Checks if a bot can throw a grenade (has grenades available).
 *
 * @param client        Bot client index
 * @return              true if bot has grenades, false otherwise
 * @error               Invalid client index or client is not an HL2DM RCBot
 */
native bool RCBot2_HLDM_CanThrowGrenade(int client);

//=============================================================================
// HL2DM Event Forwards
//=============================================================================

/**
 * Called when an HL2DM bot picks up a weapon.
 *
 * @param client        Bot client index
 * @param weapon        Weapon entity index
 */
forward void RCBot2_HLDM_OnBotWeaponPickup(int client, int weapon);

/**
 * Called when an HL2DM bot picks up an object with the Gravity Gun.
 *
 * @param client        Bot client index
 * @param object        Physics object entity index
 */
forward void RCBot2_HLDM_OnBotGravityGunPickup(int client, int object);

/**
 * Called when an HL2DM bot launches an object with the Gravity Gun.
 *
 * @param client        Bot client index
 * @param object        Physics object entity index
 */
forward void RCBot2_HLDM_OnBotGravityGunLaunch(int client, int object);

/**
 * Called when an HL2DM bot drops an object from the Gravity Gun.
 *
 * @param client        Bot client index
 * @param object        Physics object entity index
 */
forward void RCBot2_HLDM_OnBotGravityGunDrop(int client, int object);

/**
 * Called when an HL2DM bot uses a suit charger or health charger.
 *
 * @param client        Bot client index
 * @param chargerType   Charger type (0 = health, 1 = suit/armor)
 * @param chargerEntity Charger entity index
 */
forward void RCBot2_HLDM_OnBotSuitChargeUsed(int client, int chargerType, int chargerEntity);
