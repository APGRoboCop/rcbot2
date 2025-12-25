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

#ifndef __BOT_TACTICAL_H__
#define __BOT_TACTICAL_H__

#include <vector>
#include <cstdint>

class CWaypoint;
class CBot;

//=============================================================================
// Tactical Flags
// Additional flags that describe tactical properties of waypoints
// These extend the existing W_FL_* waypoint flags
//=============================================================================
namespace TacticalFlags
{
	// Cover types
	constexpr uint32_t COVER_FULL       = (1 << 0);   // Full cover from all directions
	constexpr uint32_t COVER_PARTIAL    = (1 << 1);   // Partial cover (crouch/lean)
	constexpr uint32_t COVER_HIGH       = (1 << 2);   // High ground advantage
	constexpr uint32_t COVER_LOW        = (1 << 3);   // Low position (less visible)

	// Combat positions
	constexpr uint32_t SNIPER_SPOT      = (1 << 4);   // Good for sniping
	constexpr uint32_t AMBUSH_POINT     = (1 << 5);   // Good for ambushes
	constexpr uint32_t CHOKE_POINT      = (1 << 6);   // Narrow passage (chokepoint)
	constexpr uint32_t OPEN_AREA        = (1 << 7);   // Open area (dangerous)

	// Strategic value
	constexpr uint32_t HIGH_TRAFFIC     = (1 << 8);   // Frequently traveled
	constexpr uint32_t OBJECTIVE_NEARBY = (1 << 9);   // Near objective
	constexpr uint32_t SPAWN_AREA       = (1 << 10);  // Near spawn point
	constexpr uint32_t FLANK_ROUTE      = (1 << 11);  // Good for flanking

	// Danger indicators
	constexpr uint32_t DANGER_ZONE      = (1 << 12);  // High combat frequency
	constexpr uint32_t CAMPING_SPOT     = (1 << 13);  // Known camping location
	constexpr uint32_t FALL_HAZARD      = (1 << 14);  // Risk of falling
	constexpr uint32_t EXPLOSION_RISK   = (1 << 15);  // Explosive hazard nearby

	// Movement modifiers
	constexpr uint32_t PREFER_SPRINT    = (1 << 16);  // Sprint through quickly
	constexpr uint32_t PREFER_CROUCH    = (1 << 17);  // Crouch movement preferred
	constexpr uint32_t PREFER_JUMP      = (1 << 18);  // Jumping helps here
	constexpr uint32_t SLOW_APPROACH    = (1 << 19);  // Approach carefully

	// Supply points
	constexpr uint32_t HEALTH_NEARBY    = (1 << 20);  // Health pickups nearby
	constexpr uint32_t AMMO_NEARBY      = (1 << 21);  // Ammo pickups nearby
	constexpr uint32_t ARMOR_NEARBY     = (1 << 22);  // Armor pickups nearby
	constexpr uint32_t WEAPON_NEARBY    = (1 << 23);  // Weapons nearby
}

//=============================================================================
// Bot Playstyle Types
// Different bot personality/playstyle preferences
//=============================================================================
enum class EBotPlaystyle : uint8_t
{
	BALANCED = 0,       // Default balanced play
	AGGRESSIVE,         // Rushes objectives, takes risks
	DEFENSIVE,          // Holds positions, plays safe
	SUPPORT,            // Supports teammates, heals
	SNIPER,             // Prefers long-range combat
	FLANKER,            // Uses alternate routes
	CAMPER,             // Holds strong positions
	RUSHER,             // Fast movement, close combat
	MAX_PLAYSTYLES
};

//=============================================================================
// CTacticalWeight
// Weight modifiers for different playstyles at a waypoint
//=============================================================================
struct CTacticalWeight
{
	float balanced;     // Default weight
	float aggressive;   // Weight for aggressive bots
	float defensive;    // Weight for defensive bots
	float sniper;       // Weight for sniper bots
	float flanker;      // Weight for flanking bots

	CTacticalWeight()
		: balanced(1.0f)
		, aggressive(1.0f)
		, defensive(1.0f)
		, sniper(1.0f)
		, flanker(1.0f)
	{
	}

	// Get weight for a specific playstyle
	float getWeight(EBotPlaystyle style) const
	{
		switch (style)
		{
		case EBotPlaystyle::AGGRESSIVE:
		case EBotPlaystyle::RUSHER:
			return aggressive;
		case EBotPlaystyle::DEFENSIVE:
		case EBotPlaystyle::CAMPER:
			return defensive;
		case EBotPlaystyle::SNIPER:
			return sniper;
		case EBotPlaystyle::FLANKER:
			return flanker;
		default:
			return balanced;
		}
	}
};

//=============================================================================
// CTacticalInfo
// Tactical metadata for a single waypoint
//=============================================================================
class CTacticalInfo
{
public:
	CTacticalInfo();
	CTacticalInfo(int waypointId);

	// Initialize/reset
	void reset();

	// Waypoint ID
	int getWaypointId() const { return m_waypointId; }
	void setWaypointId(int id) { m_waypointId = id; }

	// Tactical flags
	uint32_t getTacticalFlags() const { return m_tacticalFlags; }
	void setTacticalFlags(uint32_t flags) { m_tacticalFlags = flags; }
	void addTacticalFlag(uint32_t flag) { m_tacticalFlags |= flag; }
	void removeTacticalFlag(uint32_t flag) { m_tacticalFlags &= ~flag; }
	bool hasTacticalFlag(uint32_t flag) const { return (m_tacticalFlags & flag) == flag; }

	// Danger rating (0.0 = safe, 1.0 = extremely dangerous)
	float getDangerRating() const { return m_dangerRating; }
	void setDangerRating(float rating) { m_dangerRating = rating; }
	void adjustDanger(float delta);

	// Combat statistics (updated during gameplay)
	int getDeathCount() const { return m_deathCount; }
	int getKillCount() const { return m_killCount; }
	void recordDeath() { m_deathCount++; adjustDanger(0.1f); }
	void recordKill() { m_killCount++; }

	// Visibility data
	int getVisibleEnemySpots() const { return m_visibleEnemySpots; }
	void setVisibleEnemySpots(int count) { m_visibleEnemySpots = count; }

	// Cover quality (0.0 = no cover, 1.0 = excellent cover)
	float getCoverQuality() const { return m_coverQuality; }
	void setCoverQuality(float quality) { m_coverQuality = quality; }

	// Height advantage over surroundings
	float getHeightAdvantage() const { return m_heightAdvantage; }
	void setHeightAdvantage(float height) { m_heightAdvantage = height; }

	// Tactical weights
	const CTacticalWeight& getWeights() const { return m_weights; }
	CTacticalWeight& getWeights() { return m_weights; }

	// Get weight for bot's playstyle
	float getWeightForPlaystyle(EBotPlaystyle style) const;

	// Calculate overall tactical value for a bot
	float calculateTacticalValue(CBot* pBot) const;

	// Timestamps
	float getLastCombatTime() const { return m_lastCombatTime; }
	void setLastCombatTime(float time) { m_lastCombatTime = time; }

	float getLastVisitTime() const { return m_lastVisitTime; }
	void setLastVisitTime(float time) { m_lastVisitTime = time; }

private:
	int m_waypointId;
	uint32_t m_tacticalFlags;

	float m_dangerRating;        // 0.0 - 1.0 danger level
	float m_coverQuality;        // 0.0 - 1.0 cover rating
	float m_heightAdvantage;     // Height difference from average

	int m_deathCount;            // Deaths recorded at this waypoint
	int m_killCount;             // Kills recorded at this waypoint
	int m_visibleEnemySpots;     // Number of enemy positions visible from here

	float m_lastCombatTime;      // Last time combat occurred here
	float m_lastVisitTime;       // Last time visited

	CTacticalWeight m_weights;   // Playstyle-based weights
};

//=============================================================================
// CTacticalAnalyzer
// Analyzes waypoints to generate tactical metadata
//=============================================================================
class CTacticalAnalyzer
{
public:
	CTacticalAnalyzer();

	// Analyze a single waypoint's tactical properties
	void analyzeWaypoint(CWaypoint* pWaypoint, CTacticalInfo* pInfo);

	// Analyze cover quality at a position
	float analyzeCoverQuality(const Vector& origin);

	// Analyze height advantage
	float analyzeHeightAdvantage(const Vector& origin);

	// Check if position has good sightlines
	bool hasGoodSightlines(const Vector& origin, int* pVisibleCount = nullptr);

	// Check if position is a chokepoint
	bool isChokepoint(const Vector& origin);

	// Calculate path width at a position
	float calculatePathWidth(const Vector& origin);

private:
	// Trace in multiple directions to gather data
	void traceMultiDirection(const Vector& origin, std::vector<float>& distances);
};

//=============================================================================
// CTacticalDataManager
// Manages tactical data for all waypoints
//=============================================================================
class CTacticalDataManager
{
public:
	static CTacticalDataManager& instance();

	// Initialize for current map
	void init(int numWaypoints);

	// Reset all data
	void reset();

	// Get tactical info for a waypoint
	CTacticalInfo* getTacticalInfo(int waypointId);
	const CTacticalInfo* getTacticalInfo(int waypointId) const;

	// Analyze all waypoints (call during map load or manually)
	void analyzeAllWaypoints();

	// Analyze a single waypoint
	void analyzeWaypoint(int waypointId);

	// Update tactical data based on combat event
	void onCombatEvent(const Vector& location, bool wasKill, bool wasDeath);

	// Get best waypoint for a playstyle and situation
	int findBestTacticalWaypoint(CBot* pBot, EBotPlaystyle style, const Vector& nearOrigin, float maxDist = 1000.0f);

	// Get waypoints matching tactical criteria
	std::vector<int> findWaypointsWithFlags(uint32_t requiredFlags, uint32_t excludeFlags = 0);

	// Find cover waypoint near a position
	int findCoverWaypoint(const Vector& origin, const Vector& threatDirection, float maxDist = 500.0f);

	// Find flanking route waypoints
	std::vector<int> findFlankingWaypoints(const Vector& origin, const Vector& targetPos, float maxDist = 800.0f);

	// Save/load tactical data
	bool saveData(const char* mapName);
	bool loadData(const char* mapName);

	// Decay danger ratings over time
	void updateDangerDecay(float deltaTime);

	// Nav-test integration
	// Updates HIGH_TRAFFIC flags based on nav-test visit frequency data
	void updateTrafficFromNavTest();

	// Updates danger ratings and weights based on nav-test issue data
	void updateDangerFromNavTest();

private:
	CTacticalDataManager();
	~CTacticalDataManager();

	// Prevent copying
	CTacticalDataManager(const CTacticalDataManager&) = delete;
	CTacticalDataManager& operator=(const CTacticalDataManager&) = delete;

	std::vector<CTacticalInfo> m_tacticalData;
	CTacticalAnalyzer m_analyzer;
	float m_lastUpdateTime;

	static CTacticalDataManager* s_instance;
};

//=============================================================================
// Helper functions
//=============================================================================

// Get playstyle from bot profile or behavior
EBotPlaystyle GetBotPlaystyle(CBot* pBot);

// Get string name for playstyle
const char* GetPlaystyleName(EBotPlaystyle style);

// Get tactical flag name
const char* GetTacticalFlagName(uint32_t flag);

#endif // __BOT_TACTICAL_H__
