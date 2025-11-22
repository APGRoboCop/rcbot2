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

#ifndef __BOT_WAYPOINT_HL2DM_H__
#define __BOT_WAYPOINT_HL2DM_H__

#include "bot_waypoint.h"
#include <vector>

// HL2DM-specific waypoint enhancements
// Supports weapon pickups, interactable entities (buttons, triggers), and teleports

// HL2DM Waypoint Sub-Types
// These are stored as metadata, not in the main waypoint flags
// This allows us to have detailed weapon information without using up flag bits
enum class EHL2DMWaypointSubType : uint8_t
{
	NONE = 0,

	// Weapon Types
	WEAPON_CROWBAR,
	WEAPON_STUNSTICK,
	WEAPON_PISTOL,
	WEAPON_357,
	WEAPON_SMG1,
	WEAPON_AR2,
	WEAPON_SHOTGUN,
	WEAPON_CROSSBOW,
	WEAPON_RPG,
	WEAPON_GRENADE,
	WEAPON_SLAM,
	WEAPON_PHYSCANNON,

	// Special Pickups
	ITEM_SUIT,
	ITEM_BATTERY,
	ITEM_HEALTHKIT,
	ITEM_HEALTHVIAL,
	ITEM_AMMO_PISTOL,
	ITEM_AMMO_SMG1,
	ITEM_AMMO_AR2,
	ITEM_AMMO_357,
	ITEM_AMMO_CROSSBOW,
	ITEM_AMMO_SHOTGUN,
	ITEM_AMMO_RPG,
	ITEM_AMMO_GRENADE,
	ITEM_AMMO_CRATE,

	// Interactable Entities
	BUTTON,           // func_button, func_rot_button
	DOOR,             // func_door, func_door_rotating
	BREAKABLE,        // func_breakable
	TRIGGER,          // trigger_multiple, trigger_once
	TELEPORT_DEST,    // Teleport destination
	TELEPORT_SOURCE,  // Teleport entrance

	MAX_SUBTYPES
};

// HL2DM Waypoint metadata (per-waypoint extended data)
struct HL2DMWaypointMetadata
{
	EHL2DMWaypointSubType subType;
	MyEHandle hEntity;        // Entity handle for the associated entity
	Vector vEntityOrigin;     // Original entity position
	float fRespawnTime;       // Weapon respawn time (-1 = doesn't respawn)
	int iWeaponPriority;      // Weapon priority (0-100, higher = more important)
	bool bRequiresUse;        // Requires +use to interact
	bool bIsMoving;           // For doors/platforms that move
	Vector vUsePosition;      // Optimal position to use from

	HL2DMWaypointMetadata()
		: subType(EHL2DMWaypointSubType::NONE)
		, hEntity()
		, vEntityOrigin(0, 0, 0)
		, fRespawnTime(30.0f)
		, iWeaponPriority(50)
		, bRequiresUse(false)
		, bIsMoving(false)
		, vUsePosition(0, 0, 0)
	{
	}
};

// HL2DM Waypoint Manager
class CHL2DMWaypointManager
{
public:
	// Singleton access
	static CHL2DMWaypointManager& getInstance()
	{
		static CHL2DMWaypointManager instance;
		return instance;
	}

	// Set metadata for a waypoint
	void setMetadata(int iWaypointIndex, const HL2DMWaypointMetadata& metadata);

	// Get metadata for a waypoint
	const HL2DMWaypointMetadata* getMetadata(int iWaypointIndex) const;

	// Check if waypoint has metadata
	bool hasMetadata(int iWaypointIndex) const;

	// Clear metadata for a waypoint
	void clearMetadata(int iWaypointIndex);

	// Clear all metadata
	void clearAllMetadata();

	// Find nearest weapon waypoint
	int findNearestWeapon(const Vector& vOrigin, EHL2DMWaypointSubType weaponType, float fMaxDistance = 2000.0f);

	// Find all weapon waypoints of a type
	std::vector<int> findAllWeapons(EHL2DMWaypointSubType weaponType);

	// Find nearest button/use waypoint
	int findNearestButton(const Vector& vOrigin, float fMaxDistance = 500.0f);

	// Find nearest teleport
	int findNearestTeleport(const Vector& vOrigin, bool bSource = true, float fMaxDistance = 1000.0f);

	// Get weapon priority
	int getWeaponPriority(EHL2DMWaypointSubType weaponType) const;

	// Convert entity classname to subtype
	static EHL2DMWaypointSubType classnameToSubType(const char* szClassname);

	// Convert subtype to readable name
	static const char* subTypeToString(EHL2DMWaypointSubType subType);

	// Save metadata to file
	bool saveMetadata(const char* szMapName);

	// Load metadata from file
	bool loadMetadata(const char* szMapName);

private:
	CHL2DMWaypointManager() = default;
	~CHL2DMWaypointManager() = default;

	// Delete copy constructor and assignment operator
	CHL2DMWaypointManager(const CHL2DMWaypointManager&) = delete;
	CHL2DMWaypointManager& operator=(const CHL2DMWaypointManager&) = delete;

	std::vector<HL2DMWaypointMetadata> m_Metadata; // Indexed by waypoint ID
};

// HL2DM Entity Scanner - Enhanced entity detection for HL2DM
class CHL2DMEntityScanner
{
public:
	// Scan for HL2DM weapons near a position
	static EHL2DMWaypointSubType scanForWeapon(const Vector& vOrigin, float fRadius = 100.0f);

	// Scan for interactable entities
	static EHL2DMWaypointSubType scanForInteractable(const Vector& vOrigin, float fRadius = 100.0f);

	// Scan for teleport entities
	static bool scanForTeleport(const Vector& vOrigin, Vector* vDestination, float fRadius = 100.0f);

	// Get all HL2DM weapons in map
	static std::vector<edict_t*> getAllWeapons();

	// Get all buttons/interactables in map
	static std::vector<edict_t*> getAllInteractables();

	// Get respawn time for weapon
	static float getWeaponRespawnTime(const char* szClassname);

	// Check if entity is a weapon
	static bool isWeaponEntity(edict_t* pEntity);

	// Check if entity is interactable
	static bool isInteractableEntity(edict_t* pEntity);

	// Get optimal use position for entity
	static Vector getOptimalUsePosition(edict_t* pEntity, const Vector& vNearestWaypoint);
};

// HL2DM Auto-Waypoint Generator Extension
class CHL2DMAutoWaypoint
{
public:
	// Generate waypoints for all weapons on map
	static int generateWeaponWaypoints();

	// Generate waypoints for all buttons/interactables
	static int generateInteractableWaypoints();

	// Generate waypoints for teleports
	static int generateTeleportWaypoints();

	// Generate all HL2DM-specific waypoints
	static int generateAllHL2DMWaypoints();

	// Update existing waypoints with HL2DM metadata
	static int updateExistingWaypoints();
};

#endif // __BOT_WAYPOINT_HL2DM_H__
