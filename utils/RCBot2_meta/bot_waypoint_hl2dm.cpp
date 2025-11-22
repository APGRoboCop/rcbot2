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

#include "bot_waypoint_hl2dm.h"
#include "bot_waypoint_locations.h"
#include "bot_globals.h"
#include "rcbot/logging.h"

#include <cstring>
#include <algorithm>
#include <fstream>

///////////////////////////////////////////////////////////////////////////////
// CHL2DMWaypointManager - Metadata management
///////////////////////////////////////////////////////////////////////////////

void CHL2DMWaypointManager::setMetadata(int iWaypointIndex, const HL2DMWaypointMetadata& metadata)
{
	if (iWaypointIndex < 0 || iWaypointIndex >= CWaypoints::MAX_WAYPOINTS)
		return;

	// Expand metadata vector if needed
	if (static_cast<size_t>(iWaypointIndex) >= m_Metadata.size())
	{
		m_Metadata.resize(iWaypointIndex + 1);
	}

	m_Metadata[iWaypointIndex] = metadata;
}

const HL2DMWaypointMetadata* CHL2DMWaypointManager::getMetadata(int iWaypointIndex) const
{
	if (iWaypointIndex < 0 || static_cast<size_t>(iWaypointIndex) >= m_Metadata.size())
		return nullptr;

	if (m_Metadata[iWaypointIndex].subType == EHL2DMWaypointSubType::NONE)
		return nullptr;

	return &m_Metadata[iWaypointIndex];
}

bool CHL2DMWaypointManager::hasMetadata(int iWaypointIndex) const
{
	return getMetadata(iWaypointIndex) != nullptr;
}

void CHL2DMWaypointManager::clearMetadata(int iWaypointIndex)
{
	if (iWaypointIndex >= 0 && static_cast<size_t>(iWaypointIndex) < m_Metadata.size())
	{
		m_Metadata[iWaypointIndex] = HL2DMWaypointMetadata();
	}
}

void CHL2DMWaypointManager::clearAllMetadata()
{
	m_Metadata.clear();
}

int CHL2DMWaypointManager::findNearestWeapon(const Vector& vOrigin, EHL2DMWaypointSubType weaponType, float fMaxDistance)
{
	int iBestWpt = -1;
	float fBestDist = fMaxDistance;

	for (size_t i = 0; i < m_Metadata.size(); i++)
	{
		if (m_Metadata[i].subType == weaponType)
		{
			CWaypoint* pWpt = CWaypoints::getWaypoint(static_cast<int>(i));
			if (!pWpt || !pWpt->isUsed())
				continue;

			const float fDist = pWpt->distanceFrom(vOrigin);
			if (fDist < fBestDist)
			{
				fBestDist = fDist;
				iBestWpt = static_cast<int>(i);
			}
		}
	}

	return iBestWpt;
}

std::vector<int> CHL2DMWaypointManager::findAllWeapons(EHL2DMWaypointSubType weaponType)
{
	std::vector<int> results;

	for (size_t i = 0; i < m_Metadata.size(); i++)
	{
		if (m_Metadata[i].subType == weaponType)
		{
			CWaypoint* pWpt = CWaypoints::getWaypoint(static_cast<int>(i));
			if (pWpt && pWpt->isUsed())
			{
				results.push_back(static_cast<int>(i));
			}
		}
	}

	return results;
}

int CHL2DMWaypointManager::findNearestButton(const Vector& vOrigin, float fMaxDistance)
{
	int iBestWpt = -1;
	float fBestDist = fMaxDistance;

	for (size_t i = 0; i < m_Metadata.size(); i++)
	{
		if (m_Metadata[i].subType == EHL2DMWaypointSubType::BUTTON ||
		    m_Metadata[i].subType == EHL2DMWaypointSubType::DOOR ||
		    m_Metadata[i].subType == EHL2DMWaypointSubType::TRIGGER)
		{
			CWaypoint* pWpt = CWaypoints::getWaypoint(static_cast<int>(i));
			if (!pWpt || !pWpt->isUsed())
				continue;

			const float fDist = pWpt->distanceFrom(vOrigin);
			if (fDist < fBestDist)
			{
				fBestDist = fDist;
				iBestWpt = static_cast<int>(i);
			}
		}
	}

	return iBestWpt;
}

int CHL2DMWaypointManager::findNearestTeleport(const Vector& vOrigin, bool bSource, float fMaxDistance)
{
	const EHL2DMWaypointSubType searchType = bSource ? EHL2DMWaypointSubType::TELEPORT_SOURCE : EHL2DMWaypointSubType::TELEPORT_DEST;

	int iBestWpt = -1;
	float fBestDist = fMaxDistance;

	for (size_t i = 0; i < m_Metadata.size(); i++)
	{
		if (m_Metadata[i].subType == searchType)
		{
			CWaypoint* pWpt = CWaypoints::getWaypoint(static_cast<int>(i));
			if (!pWpt || !pWpt->isUsed())
				continue;

			const float fDist = pWpt->distanceFrom(vOrigin);
			if (fDist < fBestDist)
			{
				fBestDist = fDist;
				iBestWpt = static_cast<int>(i);
			}
		}
	}

	return iBestWpt;
}

int CHL2DMWaypointManager::getWeaponPriority(EHL2DMWaypointSubType weaponType) const
{
	// Weapon priority ranking for HL2DM (0-100, higher = better)
	switch (weaponType)
	{
	case EHL2DMWaypointSubType::WEAPON_RPG:         return 95;
	case EHL2DMWaypointSubType::WEAPON_CROSSBOW:    return 90;
	case EHL2DMWaypointSubType::WEAPON_AR2:         return 85;
	case EHL2DMWaypointSubType::WEAPON_SHOTGUN:     return 80;
	case EHL2DMWaypointSubType::WEAPON_357:         return 75;
	case EHL2DMWaypointSubType::WEAPON_SMG1:        return 70;
	case EHL2DMWaypointSubType::WEAPON_PHYSCANNON:  return 65;
	case EHL2DMWaypointSubType::WEAPON_GRENADE:     return 60;
	case EHL2DMWaypointSubType::WEAPON_SLAM:        return 55;
	case EHL2DMWaypointSubType::WEAPON_PISTOL:      return 40;
	case EHL2DMWaypointSubType::WEAPON_STUNSTICK:   return 30;
	case EHL2DMWaypointSubType::WEAPON_CROWBAR:     return 20;
	default:                                         return 50;
	}
}

EHL2DMWaypointSubType CHL2DMWaypointManager::classnameToSubType(const char* szClassname)
{
	if (!szClassname)
		return EHL2DMWaypointSubType::NONE;

	// Weapons
	if (std::strcmp(szClassname, "weapon_crowbar") == 0)       return EHL2DMWaypointSubType::WEAPON_CROWBAR;
	if (std::strcmp(szClassname, "weapon_stunstick") == 0)     return EHL2DMWaypointSubType::WEAPON_STUNSTICK;
	if (std::strcmp(szClassname, "weapon_pistol") == 0)        return EHL2DMWaypointSubType::WEAPON_PISTOL;
	if (std::strcmp(szClassname, "weapon_357") == 0)           return EHL2DMWaypointSubType::WEAPON_357;
	if (std::strcmp(szClassname, "weapon_smg1") == 0)          return EHL2DMWaypointSubType::WEAPON_SMG1;
	if (std::strcmp(szClassname, "weapon_ar2") == 0)           return EHL2DMWaypointSubType::WEAPON_AR2;
	if (std::strcmp(szClassname, "weapon_shotgun") == 0)       return EHL2DMWaypointSubType::WEAPON_SHOTGUN;
	if (std::strcmp(szClassname, "weapon_crossbow") == 0)      return EHL2DMWaypointSubType::WEAPON_CROSSBOW;
	if (std::strcmp(szClassname, "weapon_rpg") == 0)           return EHL2DMWaypointSubType::WEAPON_RPG;
	if (std::strcmp(szClassname, "weapon_frag") == 0)          return EHL2DMWaypointSubType::WEAPON_GRENADE;
	if (std::strcmp(szClassname, "weapon_slam") == 0)          return EHL2DMWaypointSubType::WEAPON_SLAM;
	if (std::strcmp(szClassname, "weapon_physcannon") == 0)    return EHL2DMWaypointSubType::WEAPON_PHYSCANNON;

	// Items
	if (std::strcmp(szClassname, "item_suit") == 0)            return EHL2DMWaypointSubType::ITEM_SUIT;
	if (std::strcmp(szClassname, "item_battery") == 0)         return EHL2DMWaypointSubType::ITEM_BATTERY;
	if (std::strcmp(szClassname, "item_healthkit") == 0)       return EHL2DMWaypointSubType::ITEM_HEALTHKIT;
	if (std::strcmp(szClassname, "item_healthvial") == 0)      return EHL2DMWaypointSubType::ITEM_HEALTHVIAL;
	if (std::strcmp(szClassname, "item_ammo_pistol") == 0)     return EHL2DMWaypointSubType::ITEM_AMMO_PISTOL;
	if (std::strcmp(szClassname, "item_ammo_smg1") == 0)       return EHL2DMWaypointSubType::ITEM_AMMO_SMG1;
	if (std::strcmp(szClassname, "item_ammo_ar2") == 0)        return EHL2DMWaypointSubType::ITEM_AMMO_AR2;
	if (std::strcmp(szClassname, "item_ammo_357") == 0)        return EHL2DMWaypointSubType::ITEM_AMMO_357;
	if (std::strcmp(szClassname, "item_ammo_crossbow") == 0)   return EHL2DMWaypointSubType::ITEM_AMMO_CROSSBOW;
	if (std::strcmp(szClassname, "item_box_buckshot") == 0)    return EHL2DMWaypointSubType::ITEM_AMMO_SHOTGUN;
	if (std::strcmp(szClassname, "item_rpg_round") == 0)       return EHL2DMWaypointSubType::ITEM_AMMO_RPG;
	if (std::strcmp(szClassname, "item_ammo_smg1_grenade") == 0) return EHL2DMWaypointSubType::ITEM_AMMO_GRENADE;
	if (std::strcmp(szClassname, "item_ammo_crate") == 0)      return EHL2DMWaypointSubType::ITEM_AMMO_CRATE;

	// Interactables
	if (std::strncmp(szClassname, "func_button", 11) == 0)     return EHL2DMWaypointSubType::BUTTON;
	if (std::strncmp(szClassname, "func_rot_button", 15) == 0) return EHL2DMWaypointSubType::BUTTON;
	if (std::strncmp(szClassname, "func_door", 9) == 0)        return EHL2DMWaypointSubType::DOOR;
	if (std::strcmp(szClassname, "func_breakable") == 0)       return EHL2DMWaypointSubType::BREAKABLE;
	if (std::strncmp(szClassname, "trigger_", 8) == 0)         return EHL2DMWaypointSubType::TRIGGER;

	return EHL2DMWaypointSubType::NONE;
}

const char* CHL2DMWaypointManager::subTypeToString(EHL2DMWaypointSubType subType)
{
	switch (subType)
	{
	case EHL2DMWaypointSubType::WEAPON_CROWBAR:      return "Crowbar";
	case EHL2DMWaypointSubType::WEAPON_STUNSTICK:    return "Stunstick";
	case EHL2DMWaypointSubType::WEAPON_PISTOL:       return "Pistol";
	case EHL2DMWaypointSubType::WEAPON_357:          return ".357 Magnum";
	case EHL2DMWaypointSubType::WEAPON_SMG1:         return "SMG";
	case EHL2DMWaypointSubType::WEAPON_AR2:          return "AR2";
	case EHL2DMWaypointSubType::WEAPON_SHOTGUN:      return "Shotgun";
	case EHL2DMWaypointSubType::WEAPON_CROSSBOW:     return "Crossbow";
	case EHL2DMWaypointSubType::WEAPON_RPG:          return "RPG";
	case EHL2DMWaypointSubType::WEAPON_GRENADE:      return "Grenade";
	case EHL2DMWaypointSubType::WEAPON_SLAM:         return "SLAM";
	case EHL2DMWaypointSubType::WEAPON_PHYSCANNON:   return "Gravity Gun";
	case EHL2DMWaypointSubType::ITEM_SUIT:           return "HEV Suit";
	case EHL2DMWaypointSubType::ITEM_BATTERY:        return "Battery";
	case EHL2DMWaypointSubType::ITEM_HEALTHKIT:      return "Health Kit";
	case EHL2DMWaypointSubType::ITEM_HEALTHVIAL:     return "Health Vial";
	case EHL2DMWaypointSubType::BUTTON:              return "Button";
	case EHL2DMWaypointSubType::DOOR:                return "Door";
	case EHL2DMWaypointSubType::BREAKABLE:           return "Breakable";
	case EHL2DMWaypointSubType::TRIGGER:             return "Trigger";
	case EHL2DMWaypointSubType::TELEPORT_SOURCE:     return "Teleport Entrance";
	case EHL2DMWaypointSubType::TELEPORT_DEST:       return "Teleport Exit";
	default:                                          return "Unknown";
	}
}

bool CHL2DMWaypointManager::saveMetadata(const char* szMapName)
{
	char filename[512];
	CBotGlobals::buildFileName(filename, szMapName, "waypoints", "hl2dm", false);

	std::fstream file(filename, std::ios::out | std::ios::binary);
	if (!file)
	{
		logger->Log(LogLevel::ERROR, "Failed to save HL2DM waypoint metadata: %s", filename);
		return false;
	}

	// Write header
	const uint32_t magic = 0x484C3244; // "HL2D"
	const uint16_t version = 1;
	const uint32_t count = static_cast<uint32_t>(m_Metadata.size());

	file.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
	file.write(reinterpret_cast<const char*>(&version), sizeof(version));
	file.write(reinterpret_cast<const char*>(&count), sizeof(count));

	// Write metadata
	for (const HL2DMWaypointMetadata& meta : m_Metadata)
	{
		file.write(reinterpret_cast<const char*>(&meta.subType), sizeof(meta.subType));
		file.write(reinterpret_cast<const char*>(&meta.vEntityOrigin), sizeof(meta.vEntityOrigin));
		file.write(reinterpret_cast<const char*>(&meta.fRespawnTime), sizeof(meta.fRespawnTime));
		file.write(reinterpret_cast<const char*>(&meta.iWeaponPriority), sizeof(meta.iWeaponPriority));
		file.write(reinterpret_cast<const char*>(&meta.bRequiresUse), sizeof(meta.bRequiresUse));
		file.write(reinterpret_cast<const char*>(&meta.bIsMoving), sizeof(meta.bIsMoving));
		file.write(reinterpret_cast<const char*>(&meta.vUsePosition), sizeof(meta.vUsePosition));
	}

	file.close();

	logger->Log(LogLevel::INFO, "Saved HL2DM waypoint metadata: %d entries", count);
	return true;
}

bool CHL2DMWaypointManager::loadMetadata(const char* szMapName)
{
	char filename[512];
	CBotGlobals::buildFileName(filename, szMapName, "waypoints", "hl2dm", false);

	std::fstream file(filename, std::ios::in | std::ios::binary);
	if (!file)
	{
		logger->Log(LogLevel::WARN, "No HL2DM waypoint metadata found for map: %s", szMapName);
		return false;
	}

	// Read header
	uint32_t magic;
	uint16_t version;
	uint32_t count;

	file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
	file.read(reinterpret_cast<char*>(&version), sizeof(version));
	file.read(reinterpret_cast<char*>(&count), sizeof(count));

	if (magic != 0x484C3244)
	{
		logger->Log(LogLevel::ERROR, "Invalid HL2DM waypoint metadata file");
		return false;
	}

	// Read metadata
	m_Metadata.clear();
	m_Metadata.resize(count);

	for (uint32_t i = 0; i < count; i++)
	{
		file.read(reinterpret_cast<char*>(&m_Metadata[i].subType), sizeof(m_Metadata[i].subType));
		file.read(reinterpret_cast<char*>(&m_Metadata[i].vEntityOrigin), sizeof(m_Metadata[i].vEntityOrigin));
		file.read(reinterpret_cast<char*>(&m_Metadata[i].fRespawnTime), sizeof(m_Metadata[i].fRespawnTime));
		file.read(reinterpret_cast<char*>(&m_Metadata[i].iWeaponPriority), sizeof(m_Metadata[i].iWeaponPriority));
		file.read(reinterpret_cast<char*>(&m_Metadata[i].bRequiresUse), sizeof(m_Metadata[i].bRequiresUse));
		file.read(reinterpret_cast<char*>(&m_Metadata[i].bIsMoving), sizeof(m_Metadata[i].bIsMoving));
		file.read(reinterpret_cast<char*>(&m_Metadata[i].vUsePosition), sizeof(m_Metadata[i].vUsePosition));
	}

	file.close();

	logger->Log(LogLevel::INFO, "Loaded HL2DM waypoint metadata: %d entries", count);
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// CHL2DMEntityScanner - Entity scanning and detection
///////////////////////////////////////////////////////////////////////////////

EHL2DMWaypointSubType CHL2DMEntityScanner::scanForWeapon(const Vector& vOrigin, float fRadius)
{
	edict_t* pEntity = nullptr;

	while ((pEntity = CBotGlobals::findEntityByClassnameNearest(vOrigin, "weapon_*", fRadius, pEntity)) != nullptr)
	{
		const char* szClassname = pEntity->GetClassName();
		const EHL2DMWaypointSubType subType = CHL2DMWaypointManager::classnameToSubType(szClassname);

		if (subType >= EHL2DMWaypointSubType::WEAPON_CROWBAR && subType <= EHL2DMWaypointSubType::WEAPON_PHYSCANNON)
		{
			return subType;
		}
	}

	// Check for items
	while ((pEntity = CBotGlobals::findEntityByClassnameNearest(vOrigin, "item_*", fRadius, pEntity)) != nullptr)
	{
		const char* szClassname = pEntity->GetClassName();
		const EHL2DMWaypointSubType subType = CHL2DMWaypointManager::classnameToSubType(szClassname);

		if (subType != EHL2DMWaypointSubType::NONE)
		{
			return subType;
		}
	}

	return EHL2DMWaypointSubType::NONE;
}

EHL2DMWaypointSubType CHL2DMEntityScanner::scanForInteractable(const Vector& vOrigin, float fRadius)
{
	// Check for buttons
	edict_t* pEntity = CBotGlobals::findEntityByClassnameNearest(vOrigin, "func_button", fRadius);
	if (pEntity)
		return EHL2DMWaypointSubType::BUTTON;

	pEntity = CBotGlobals::findEntityByClassnameNearest(vOrigin, "func_rot_button", fRadius);
	if (pEntity)
		return EHL2DMWaypointSubType::BUTTON;

	// Check for doors
	pEntity = CBotGlobals::findEntityByClassnameNearest(vOrigin, "func_door", fRadius);
	if (pEntity)
		return EHL2DMWaypointSubType::DOOR;

	pEntity = CBotGlobals::findEntityByClassnameNearest(vOrigin, "func_door_rotating", fRadius);
	if (pEntity)
		return EHL2DMWaypointSubType::DOOR;

	// Check for breakables
	pEntity = CBotGlobals::findEntityByClassnameNearest(vOrigin, "func_breakable", fRadius);
	if (pEntity)
		return EHL2DMWaypointSubType::BREAKABLE;

	return EHL2DMWaypointSubType::NONE;
}

bool CHL2DMEntityScanner::scanForTeleport(const Vector& vOrigin, Vector* vDestination, float fRadius)
{
	// Look for trigger_teleport entities
	edict_t* pEntity = CBotGlobals::findEntityByClassnameNearest(vOrigin, "trigger_teleport", fRadius);

	if (pEntity && vDestination)
	{
		// Try to find destination (implementation depends on entity properties)
		// This is a placeholder - actual implementation would need to read entity properties
		*vDestination = vOrigin; // TODO: Get actual destination from entity
		return true;
	}

	return false;
}

std::vector<edict_t*> CHL2DMEntityScanner::getAllWeapons()
{
	std::vector<edict_t*> weapons;

	// Scan for all weapon entities
	const char* weaponPrefixes[] = {
		"weapon_crowbar", "weapon_stunstick", "weapon_pistol", "weapon_357",
		"weapon_smg1", "weapon_ar2", "weapon_shotgun", "weapon_crossbow",
		"weapon_rpg", "weapon_frag", "weapon_slam", "weapon_physcannon"
	};

	for (const char* prefix : weaponPrefixes)
	{
		edict_t* pEntity = nullptr;
		while ((pEntity = CBotGlobals::findEntityByClassname(pEntity, prefix)) != nullptr)
		{
			weapons.push_back(pEntity);
		}
	}

	return weapons;
}

std::vector<edict_t*> CHL2DMEntityScanner::getAllInteractables()
{
	std::vector<edict_t*> interactables;

	const char* interactablePrefixes[] = {
		"func_button", "func_rot_button", "func_door",
		"func_door_rotating", "func_breakable"
	};

	for (const char* prefix : interactablePrefixes)
	{
		edict_t* pEntity = nullptr;
		while ((pEntity = CBotGlobals::findEntityByClassname(pEntity, prefix)) != nullptr)
		{
			interactables.push_back(pEntity);
		}
	}

	return interactables;
}

float CHL2DMEntityScanner::getWeaponRespawnTime(const char* szClassname)
{
	// Default respawn time for HL2DM weapons (in seconds)
	// Most weapons respawn after 30 seconds in HL2DM
	return 30.0f;
}

bool CHL2DMEntityScanner::isWeaponEntity(edict_t* pEntity)
{
	if (!pEntity)
		return false;

	const char* szClassname = pEntity->GetClassName();
	if (!szClassname)
		return false;

	return std::strncmp(szClassname, "weapon_", 7) == 0;
}

bool CHL2DMEntityScanner::isInteractableEntity(edict_t* pEntity)
{
	if (!pEntity)
		return false;

	const char* szClassname = pEntity->GetClassName();
	if (!szClassname)
		return false;

	return std::strncmp(szClassname, "func_button", 11) == 0 ||
	       std::strncmp(szClassname, "func_rot_button", 15) == 0 ||
	       std::strncmp(szClassname, "func_door", 9) == 0 ||
	       std::strcmp(szClassname, "func_breakable") == 0;
}

Vector CHL2DMEntityScanner::getOptimalUsePosition(edict_t* pEntity, const Vector& vNearestWaypoint)
{
	// For now, return the nearest waypoint position
	// In a full implementation, this would calculate the best position to press the button
	return vNearestWaypoint;
}

///////////////////////////////////////////////////////////////////////////////
// CHL2DMAutoWaypoint - Automatic waypoint generation for HL2DM
///////////////////////////////////////////////////////////////////////////////

int CHL2DMAutoWaypoint::generateWeaponWaypoints()
{
	int iGenerated = 0;
	CHL2DMWaypointManager& manager = CHL2DMWaypointManager::getInstance();

	const std::vector<edict_t*> weapons = CHL2DMEntityScanner::getAllWeapons();

	for (edict_t* pWeapon : weapons)
	{
		const Vector vOrigin = CBotGlobals::entityOrigin(pWeapon);
		const char* szClassname = pWeapon->GetClassName();

		// Check if waypoint already exists nearby
		const int iExisting = CWaypointLocations::NearestWaypoint(vOrigin, 50.0f, -1, false, false, false, nullptr);

		if (iExisting == -1)
		{
			// Create new waypoint
			const int iNewWpt = CWaypoints::addWaypoint(nullptr, vOrigin, CWaypointTypes::W_FL_NONE, true);

			if (iNewWpt != -1)
			{
				// Add metadata
				HL2DMWaypointMetadata metadata;
				metadata.subType = CHL2DMWaypointManager::classnameToSubType(szClassname);
				metadata.hEntity = MyEHandle(pWeapon);
				metadata.vEntityOrigin = vOrigin;
				metadata.fRespawnTime = CHL2DMEntityScanner::getWeaponRespawnTime(szClassname);
				metadata.iWeaponPriority = manager.getWeaponPriority(metadata.subType);

				manager.setMetadata(iNewWpt, metadata);
				iGenerated++;
			}
		}
		else
		{
			// Update existing waypoint with metadata
			HL2DMWaypointMetadata metadata;
			metadata.subType = CHL2DMWaypointManager::classnameToSubType(szClassname);
			metadata.hEntity = MyEHandle(pWeapon);
			metadata.vEntityOrigin = vOrigin;
			metadata.fRespawnTime = CHL2DMEntityScanner::getWeaponRespawnTime(szClassname);
			metadata.iWeaponPriority = manager.getWeaponPriority(metadata.subType);

			manager.setMetadata(iExisting, metadata);
		}
	}

	logger->Log(LogLevel::INFO, "Generated %d weapon waypoints for HL2DM", iGenerated);
	return iGenerated;
}

int CHL2DMAutoWaypoint::generateInteractableWaypoints()
{
	int iGenerated = 0;
	CHL2DMWaypointManager& manager = CHL2DMWaypointManager::getInstance();

	const std::vector<edict_t*> interactables = CHL2DMEntityScanner::getAllInteractables();

	for (edict_t* pEntity : interactables)
	{
		const Vector vOrigin = CBotGlobals::entityOrigin(pEntity);
		const char* szClassname = pEntity->GetClassName();

		// Check if waypoint already exists nearby
		const int iExisting = CWaypointLocations::NearestWaypoint(vOrigin, 100.0f, -1, false, false, false, nullptr);

		if (iExisting == -1)
		{
			// Create new waypoint with USE flag
			const int iNewWpt = CWaypoints::addWaypoint(nullptr, vOrigin, CWaypointTypes::W_FL_USE, true);

			if (iNewWpt != -1)
			{
				// Add metadata
				HL2DMWaypointMetadata metadata;
				metadata.subType = CHL2DMWaypointManager::classnameToSubType(szClassname);
				metadata.hEntity = MyEHandle(pEntity);
				metadata.vEntityOrigin = vOrigin;
				metadata.bRequiresUse = true;
				metadata.vUsePosition = vOrigin;

				manager.setMetadata(iNewWpt, metadata);
				iGenerated++;
			}
		}
		else
		{
			// Update existing waypoint
			CWaypoint* pWpt = CWaypoints::getWaypoint(iExisting);
			if (pWpt)
			{
				pWpt->addFlag(CWaypointTypes::W_FL_USE);

				HL2DMWaypointMetadata metadata;
				metadata.subType = CHL2DMWaypointManager::classnameToSubType(szClassname);
				metadata.hEntity = MyEHandle(pEntity);
				metadata.vEntityOrigin = vOrigin;
				metadata.bRequiresUse = true;
				metadata.vUsePosition = vOrigin;

				manager.setMetadata(iExisting, metadata);
			}
		}
	}

	logger->Log(LogLevel::INFO, "Generated %d interactable waypoints for HL2DM", iGenerated);
	return iGenerated;
}

int CHL2DMAutoWaypoint::generateTeleportWaypoints()
{
	int iGenerated = 0;

	// TODO: Implement teleport waypoint generation
	// This requires detecting trigger_teleport entities and their destinations

	logger->Log(LogLevel::INFO, "Generated %d teleport waypoints for HL2DM", iGenerated);
	return iGenerated;
}

int CHL2DMAutoWaypoint::generateAllHL2DMWaypoints()
{
	int iTotal = 0;

	iTotal += generateWeaponWaypoints();
	iTotal += generateInteractableWaypoints();
	iTotal += generateTeleportWaypoints();

	logger->Log(LogLevel::INFO, "Generated %d total HL2DM waypoints", iTotal);
	return iTotal;
}

int CHL2DMAutoWaypoint::updateExistingWaypoints()
{
	int iUpdated = 0;
	CHL2DMWaypointManager& manager = CHL2DMWaypointManager::getInstance();

	// Scan all existing waypoints and add metadata where appropriate
	for (int i = 0; i < CWaypoints::numWaypoints(); i++)
	{
		CWaypoint* pWpt = CWaypoints::getWaypoint(i);
		if (!pWpt || !pWpt->isUsed())
			continue;

		const Vector vOrigin = pWpt->getOrigin();

		// Check for nearby weapons
		const EHL2DMWaypointSubType weaponType = CHL2DMEntityScanner::scanForWeapon(vOrigin, 50.0f);
		if (weaponType != EHL2DMWaypointSubType::NONE)
		{
			HL2DMWaypointMetadata metadata;
			metadata.subType = weaponType;
			metadata.vEntityOrigin = vOrigin;
			metadata.iWeaponPriority = manager.getWeaponPriority(weaponType);

			manager.setMetadata(i, metadata);
			iUpdated++;
			continue;
		}

		// Check for nearby interactables
		const EHL2DMWaypointSubType interactType = CHL2DMEntityScanner::scanForInteractable(vOrigin, 100.0f);
		if (interactType != EHL2DMWaypointSubType::NONE)
		{
			pWpt->addFlag(CWaypointTypes::W_FL_USE);

			HL2DMWaypointMetadata metadata;
			metadata.subType = interactType;
			metadata.vEntityOrigin = vOrigin;
			metadata.bRequiresUse = true;

			manager.setMetadata(i, metadata);
			iUpdated++;
		}
	}

	logger->Log(LogLevel::INFO, "Updated %d existing waypoints with HL2DM metadata", iUpdated);
	return iUpdated;
}
