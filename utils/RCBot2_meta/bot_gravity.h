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

#ifndef __BOT_GRAVITY_H__
#define __BOT_GRAVITY_H__

#include <vector>
#include <cstdint>
#include <cmath>
#include <cfloat>
#include "mathlib/vector.h"
#include "edict.h"

class CBot;
class CWaypoint;
class CGravityZoneManager;

//=============================================================================
// Fall Damage Constants
// Source engine fall damage calculation constants
//=============================================================================
namespace FallDamage
{
	// Default Source engine gravity
	constexpr float DEFAULT_GRAVITY = 600.0f;

	// Fall damage starts at this speed (units/sec)
	constexpr float FALL_DAMAGE_THRESHOLD = 580.0f;

	// Fatal fall speed (instant death)
	constexpr float FATAL_FALL_SPEED = 1024.0f;

	// Height to start considering fall damage (at default gravity)
	// Approximately: threshold^2 / (2 * gravity) = 580^2 / (2 * 600) = 280 units
	constexpr float SAFE_FALL_HEIGHT_DEFAULT = 280.0f;

	// Maximum safe jump height (player can jump ~45-55 units)
	constexpr float MAX_JUMP_HEIGHT = 52.0f;
}

//=============================================================================
// CGravityInfo
// Information about current gravity and fall damage calculations
//=============================================================================
class CGravityInfo
{
public:
	CGravityInfo();

	// Update from sv_gravity cvar
	void update();

	// Get current gravity value
	float getGravity() const { return m_gravity; }

	// Check if gravity is non-standard
	bool isNonStandardGravity() const;

	// Calculate fall damage for a given height
	float calculateFallDamage(float fallHeight) const;

	// Calculate maximum safe fall height
	float getMaxSafeFallHeight() const;

	// Calculate height where fall becomes fatal
	float getFatalFallHeight() const;

	// Check if a fall would cause damage
	bool wouldCauseFallDamage(float fallHeight) const;

	// Check if a fall would be fatal
	bool wouldBeFatal(float fallHeight) const;

	// Get fall speed for a given height (v = sqrt(2*g*h))
	float getFallSpeed(float height) const;

	// Get height from fall speed (h = v^2 / 2g)
	float getHeightFromSpeed(float speed) const;

	// Calculate time to fall a given height
	float getFallTime(float height) const;

	// Calculate path cost modifier based on fall damage
	float getPathCostModifier(float fallHeight) const;

private:
	float m_gravity;
	float m_lastUpdateTime;
};

//=============================================================================
// CWaypointFallInfo
// Fall damage information for waypoint connections
//=============================================================================
struct CWaypointFallInfo
{
	int sourceWaypointId;       // Source waypoint
	int destWaypointId;         // Destination waypoint
	float heightDifference;     // Vertical drop (positive = falling)
	float estimatedDamage;      // Estimated fall damage at current gravity
	bool requiresJump;          // Does this connection require jumping?
	bool isSafe;                // Is this connection safe at current gravity?

	CWaypointFallInfo()
		: sourceWaypointId(-1)
		, destWaypointId(-1)
		, heightDifference(0.0f)
		, estimatedDamage(0.0f)
		, requiresJump(false)
		, isSafe(true)
	{
	}
};

//=============================================================================
// CGravityPathAnalyzer
// Analyzes waypoint paths for gravity-related risks
//=============================================================================
class CGravityPathAnalyzer
{
public:
	CGravityPathAnalyzer();

	// Analyze a path between two waypoints
	CWaypointFallInfo analyzeConnection(CWaypoint* pFrom, CWaypoint* pTo);

	// Check if bot should avoid a path due to fall damage
	bool shouldAvoidPath(CBot* pBot, int fromWpt, int toWpt);

	// Get path cost modifier for a connection
	float getPathCost(int fromWpt, int toWpt) const;

	// Find alternative route avoiding dangerous falls
	std::vector<int> findSafePath(int startWpt, int endWpt, float maxFallDamage = 25.0f);

	// Analyze all waypoint connections
	void analyzeAllConnections();

	// Update analysis for current gravity
	void updateForGravity();

	// Get fall info for a specific connection
	const CWaypointFallInfo* getFallInfo(int fromWpt, int toWpt) const;

	// Get all dangerous connections
	std::vector<CWaypointFallInfo> getDangerousConnections(float minDamage = 10.0f) const;

private:
	std::vector<CWaypointFallInfo> m_connectionInfo;
	CGravityInfo m_gravityInfo;
	float m_lastAnalysisGravity;
};

//=============================================================================
// CGravityManager
// Global manager for gravity-aware navigation
//=============================================================================
class CGravityManager
{
public:
	static CGravityManager& instance();

	// Update gravity info (call periodically)
	void update();

	// Get current gravity info
	const CGravityInfo& getGravityInfo() const { return m_gravityInfo; }

	// Get path analyzer
	CGravityPathAnalyzer& getPathAnalyzer() { return m_pathAnalyzer; }

	// Get zone manager (allocated lazily in cpp file)
	CGravityZoneManager& getZoneManager();
	const CGravityZoneManager& getZoneManager() const;

	// Check if gravity has changed since last analysis
	bool hasGravityChanged() const;

	// Refresh gravity data (scan zones, re-analyze connections)
	void refresh();

	// Convenience methods

	// Get current gravity value
	float getGravity() const { return m_gravityInfo.getGravity(); }

	// Check if a fall height would cause damage
	bool wouldCauseDamage(float height) const { return m_gravityInfo.wouldCauseFallDamage(height); }

	// Get estimated fall damage
	float estimateDamage(float height) const { return m_gravityInfo.calculateFallDamage(height); }

	// Get maximum safe fall height
	float getMaxSafeHeight() const { return m_gravityInfo.getMaxSafeFallHeight(); }

	// Check if bot should be careful on a path
	bool shouldBotBeCareful(CBot* pBot, int fromWpt, int toWpt) const;

	// Get modified path cost for A* pathfinding
	float getModifiedPathCost(int fromWpt, int toWpt, float baseCost) const;

	// Hook to be called when bot takes fall damage
	void onBotFallDamage(CBot* pBot, float damage, const Vector& location);

	// Get statistics
	int getDangerousConnectionCount() const;
	int getFatalConnectionCount() const;

private:
	CGravityManager();
	~CGravityManager();

	// Prevent copying
	CGravityManager(const CGravityManager&) = delete;
	CGravityManager& operator=(const CGravityManager&) = delete;

	CGravityInfo m_gravityInfo;
	CGravityPathAnalyzer m_pathAnalyzer;
	CGravityZoneManager* m_pZoneManager;

	float m_lastGravityValue;
	float m_lastUpdateTime;

	static CGravityManager* s_instance;
};

//=============================================================================
// Helper Functions
//=============================================================================

// Physics calculation: get fall speed from height
// v = sqrt(2 * g * h)
inline float CalculateFallSpeed(float height, float gravity)
{
	if (height <= 0.0f)
		return 0.0f;
	return sqrtf(2.0f * gravity * height);
}

// Physics calculation: get height from fall speed
// h = v^2 / (2 * g)
inline float CalculateFallHeight(float speed, float gravity)
{
	if (gravity <= 0.0f)
		return 0.0f;
	return (speed * speed) / (2.0f * gravity);
}

// Physics calculation: get fall time from height
// t = sqrt(2 * h / g)
inline float CalculateFallTime(float height, float gravity)
{
	if (gravity <= 0.0f || height <= 0.0f)
		return 0.0f;
	return sqrtf(2.0f * height / gravity);
}

// Check if height is safe at given gravity
inline bool IsSafeFallHeight(float height, float gravity)
{
	float speed = CalculateFallSpeed(height, gravity);
	return speed < FallDamage::FALL_DAMAGE_THRESHOLD;
}

// Scale safe height based on gravity ratio
inline float ScaleSafeHeight(float defaultSafeHeight, float currentGravity)
{
	if (currentGravity <= 0.0f)
		return FLT_MAX; // No gravity = no fall damage

	// Safe height scales inversely with gravity
	// If gravity doubles, safe height halves
	float gravityRatio = FallDamage::DEFAULT_GRAVITY / currentGravity;
	return defaultSafeHeight * gravityRatio;
}

//=============================================================================
// CGravityZone
// Represents a trigger_gravity brush entity with non-standard gravity
//=============================================================================
struct CGravityZone
{
	edict_t* pEntity;              // The trigger_gravity entity
	Vector mins;                   // Bounding box min
	Vector maxs;                   // Bounding box max
	float gravity;                 // Gravity value (0-1 multiplier in Source)
	float absoluteGravity;         // Actual gravity value (gravity * sv_gravity)
	std::vector<int> waypointsInZone; // Waypoints inside this zone

	CGravityZone()
		: pEntity(nullptr)
		, mins(0, 0, 0)
		, maxs(0, 0, 0)
		, gravity(1.0f)
		, absoluteGravity(FallDamage::DEFAULT_GRAVITY)
	{
	}

	// Check if a point is inside this zone
	bool containsPoint(const Vector& point) const
	{
		return point.x >= mins.x && point.x <= maxs.x &&
		       point.y >= mins.y && point.y <= maxs.y &&
		       point.z >= mins.z && point.z <= maxs.z;
	}
};

//=============================================================================
// CGravityZoneManager
// Scans and tracks gravity zones in the map
//=============================================================================
class CGravityZoneManager
{
public:
	CGravityZoneManager();

	// Scan map for trigger_gravity entities
	void scanForGravityZones();

	// Update zone data (if entities change)
	void update();

	// Check if a point is in any gravity zone
	bool isInGravityZone(const Vector& point) const;

	// Get effective gravity at a point
	float getGravityAtPoint(const Vector& point) const;

	// Get zone containing a point (nullptr if not in any zone)
	const CGravityZone* getZoneAtPoint(const Vector& point) const;

	// Get all waypoints affected by non-standard gravity
	std::vector<int> getAffectedWaypoints() const;

	// Get all zones
	const std::vector<CGravityZone>& getZones() const { return m_zones; }

	// Get zone count
	int getZoneCount() const { return static_cast<int>(m_zones.size()); }

	// Clear all zones
	void clear() { m_zones.clear(); }

	// Link waypoints to zones
	void linkWaypointsToZones();

private:
	std::vector<CGravityZone> m_zones;
	float m_lastScanTime;
};

//=============================================================================
// Command handler for gravity refresh
//=============================================================================
class CCommand;
void Gravity_Refresh_Command(const CCommand& args);

#endif // __BOT_GRAVITY_H__
