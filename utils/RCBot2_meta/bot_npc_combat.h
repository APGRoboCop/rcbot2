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

#ifndef __BOT_NPC_COMBAT_H__
#define __BOT_NPC_COMBAT_H__

#include "bot.h"
#include <vector>
#include <unordered_map>

// HL2DM Hostile NPC Detection and Combat System
// For cooperative maps (coop_*, js_coop_*) where bots fight NPCs instead of players

// NPC Threat Categories
enum class ENPCThreatLevel : uint8_t
{
	HARMLESS = 0,    // Non-hostile (citizens, vortigaunts on friendly team)
	LOW = 1,         // Headcrabs, manhacks, zombies
	MEDIUM = 2,      // Combine soldiers, antlions, fast zombies
	HIGH = 3,        // Elite soldiers, antlion guards, poison zombies
	CRITICAL = 4,    // Striders, gunships, hunters, attack helicopters
	BOSS = 5         // Custom boss NPCs, special enemies
};

// NPC Combat Behavior Types
enum class ENPCBehavior : uint8_t
{
	MELEE,           // Close combat (headcrabs, fast zombies)
	RANGED,          // Shoots from distance (combine soldiers)
	HYBRID,          // Can do both (poison zombie)
	VEHICLE,         // Vehicle-based (gunship, helicopter)
	SPECIAL          // Special behavior (strider, hunter)
};

// NPC Information Structure
struct NPCInfo
{
	const char* szClassName;          // Entity classname
	ENPCThreatLevel threatLevel;      // How dangerous
	ENPCBehavior behavior;            // Combat style
	int iHealth;                      // Typical health amount
	float fCombatRange;               // Optimal combat distance
	float fDetectionRange;            // How far they can see
	bool bFlying;                     // Airborne enemy
	bool bArmored;                    // Needs special weapons
	bool bFastMoving;                 // Requires leading shots
	int iPriority;                    // Target priority (0-100)
	const char* szDisplayName;        // Human-readable name
};

// NPC Tracking Entry
struct TrackedNPC
{
	MyEHandle hEntity;                // Entity handle
	Vector vLastKnownPos;             // Last seen position
	float fLastSeenTime;              // Time last spotted
	ENPCThreatLevel threatLevel;      // Threat assessment
	int iHealth;                      // Current health
	bool bAlive;                      // Is still alive
	float fEngageDistance;            // Distance to engage from
	bool bPriorityTarget;             // High priority target

	TrackedNPC()
		: hEntity()
		, vLastKnownPos(0, 0, 0)
		, fLastSeenTime(0.0f)
		, threatLevel(ENPCThreatLevel::LOW)
		, iHealth(0)
		, bAlive(true)
		, fEngageDistance(500.0f)
		, bPriorityTarget(false)
	{
	}
};

// Cooperative Game Mode Detection
class CCoopModeDetector
{
public:
	// Singleton access
	static CCoopModeDetector& getInstance()
	{
		static CCoopModeDetector instance;
		return instance;
	}

	// Detect if map is cooperative mode
	bool isCooperativeMode();

	// Check if should fight NPCs instead of players
	bool shouldTargetNPCs();

	// Detect by map name pattern
	bool isCoopMap(const char* szMapName);

	// Manual override
	void setCooperativeMode(bool bEnabled) { m_bForceCoopMode = bEnabled; }

	// Get all players team (for single-team coop)
	int getCoopTeam();

private:
	CCoopModeDetector() : m_bForceCoopMode(false), m_bDetected(false), m_bIsCoopMode(false) {}
	~CCoopModeDetector() = default;

	CCoopModeDetector(const CCoopModeDetector&) = delete;
	CCoopModeDetector& operator=(const CCoopModeDetector&) = delete;

	bool detectCoopMode();

	bool m_bForceCoopMode;
	bool m_bDetected;
	bool m_bIsCoopMode;
};

// NPC Database - Information about all hostile NPCs
class CNPCDatabase
{
public:
	// Singleton access
	static CNPCDatabase& getInstance()
	{
		static CNPCDatabase instance;
		return instance;
	}

	// Initialize NPC database
	void initialize();

	// Get NPC info by classname
	const NPCInfo* getNPCInfo(const char* szClassName) const;

	// Check if classname is hostile NPC
	bool isHostileNPC(const char* szClassName) const;

	// Get all hostile NPC classnames
	std::vector<const char*> getHostileNPCClasses() const;

	// Get NPCs by threat level
	std::vector<const NPCInfo*> getNPCsByThreat(ENPCThreatLevel threat) const;

	// Check if NPC requires special tactics
	bool requiresSpecialTactics(const char* szClassName) const;

private:
	CNPCDatabase() : m_bInitialized(false) {}
	~CNPCDatabase() = default;

	CNPCDatabase(const CNPCDatabase&) = delete;
	CNPCDatabase& operator=(const CNPCDatabase&) = delete;

	void addNPCInfo(const NPCInfo& info);

	std::unordered_map<std::string, NPCInfo> m_NPCDatabase;
	bool m_bInitialized;
};

// NPC Combat Manager - Handles NPC detection and targeting
class CNPCCombatManager
{
public:
	// Singleton access
	static CNPCCombatManager& getInstance()
	{
		static CNPCCombatManager instance;
		return instance;
	}

	// Update tracked NPCs (call periodically)
	void update();

	// Scan for NPCs in area
	void scanForNPCs(const Vector& vOrigin, float fRadius = 3000.0f);

	// Get best target for bot
	edict_t* getBestNPCTarget(CBot* pBot);

	// Get nearest hostile NPC
	edict_t* getNearestHostileNPC(const Vector& vOrigin, float fMaxDistance = 2000.0f);

	// Get highest threat NPC in range
	edict_t* getHighestThreatNPC(const Vector& vOrigin, float fMaxDistance = 2000.0f);

	// Check if entity is hostile NPC
	bool isHostileNPC(edict_t* pEntity);

	// Get threat level for entity
	ENPCThreatLevel getThreatLevel(edict_t* pEntity);

	// Get all tracked NPCs
	const std::vector<TrackedNPC>& getTrackedNPCs() const { return m_TrackedNPCs; }

	// Get combat distance for NPC
	float getCombatDistance(edict_t* pEntity);

	// Should bot retreat from this NPC
	bool shouldRetreat(CBot* pBot, edict_t* pNPC);

	// Get recommended weapon for NPC
	int getRecommendedWeapon(edict_t* pNPC);

	// Clear all tracked NPCs
	void clear();

	// Get number of NPCs in area
	int getNPCCount(const Vector& vOrigin, float fRadius, ENPCThreatLevel minThreat = ENPCThreatLevel::LOW);

	// Mark NPC as priority target
	void markAsPriority(edict_t* pNPC);

	// Get combat statistics
	int getTotalNPCsTracked() const { return static_cast<int>(m_TrackedNPCs.size()); }
	int getActiveThreats() const;

private:
	CNPCCombatManager() : m_fLastUpdateTime(0.0f) {}
	~CNPCCombatManager() = default;

	CNPCCombatManager(const CNPCCombatManager&) = delete;
	CNPCCombatManager& operator=(const CNPCCombatManager&) = delete;

	// Update single NPC tracking
	void updateNPC(TrackedNPC& npc);

	// Calculate target priority score
	float calculatePriorityScore(CBot* pBot, const TrackedNPC& npc);

	// Remove dead/despawned NPCs
	void cleanupDeadNPCs();

	std::vector<TrackedNPC> m_TrackedNPCs;
	float m_fLastUpdateTime;
};

// NPC Waypoint Integration
class CNPCWaypointManager
{
public:
	// Singleton access
	static CNPCWaypointManager& getInstance()
	{
		static CNPCWaypointManager instance;
		return instance;
	}

	// Generate combat positions for NPC encounters
	int generateCombatWaypoints();

	// Find good combat position against NPC
	int findCombatPosition(const Vector& vNPCOrigin, const Vector& vBotOrigin, float fDistance = 500.0f);

	// Find cover from NPC
	int findCoverFromNPC(const Vector& vNPCOrigin, const Vector& vBotOrigin);

	// Mark area as NPC combat zone
	void markCombatZone(const Vector& vOrigin, float fRadius);

	// Check if waypoint is in combat zone
	bool isInCombatZone(int iWaypointIndex);

	// Get retreat waypoint from NPC
	int getRetreatWaypoint(const Vector& vNPCOrigin, const Vector& vBotOrigin);

private:
	CNPCWaypointManager() = default;
	~CNPCWaypointManager() = default;

	CNPCWaypointManager(const CNPCWaypointManager&) = delete;
	CNPCWaypointManager& operator=(const CNPCWaypointManager&) = delete;

	struct CombatZone
	{
		Vector vOrigin;
		float fRadius;
		float fCreationTime;
	};

	std::vector<CombatZone> m_CombatZones;
};

#endif // __BOT_NPC_COMBAT_H__
