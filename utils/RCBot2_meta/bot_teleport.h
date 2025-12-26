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

#ifndef __BOT_TELEPORT_H__
#define __BOT_TELEPORT_H__

#include <vector>
#include <map>
#include <cstdint>

class CBot;
class CWaypoint;
class CBotSchedule;

//=============================================================================
// Teleport Waypoint Flags
// Additional waypoint flags for teleport handling
//=============================================================================
namespace TeleportWaypointFlags
{
	// Note: These flags need to be integrated with existing CWaypointTypes
	// Using high bit values to avoid conflicts with existing flags
	constexpr int W_FL_TELEPORT_ENTRANCE = (1 << 24);   // Teleport entrance
	constexpr int W_FL_TELEPORT_EXIT = (1 << 25);       // Teleport exit
	constexpr int W_FL_TELEPORT_BIDIRECTIONAL = (1 << 26); // Can teleport both ways
}

//=============================================================================
// ETeleportType
// Type of teleport entity
//=============================================================================
enum class ETeleportType : uint8_t
{
	UNKNOWN,
	TRIGGER_TELEPORT,      // Standard trigger_teleport brush
	POINT_TELEPORT,        // Point-based teleport
	TRIGGER_PUSH,          // Trigger push (can simulate teleport)
	PORTAL,                // Portal-style teleport (bidirectional)
	GAME_SPECIFIC          // Game-specific teleport mechanism
};

//=============================================================================
// ETeleportSafety
// Safety assessment of a teleport
//=============================================================================
enum class ETeleportSafety : uint8_t
{
	SAFE,                  // Definitely safe to use
	RISKY,                 // Might be dangerous (fall damage, etc.)
	DANGEROUS,             // Likely to cause harm
	TELEFRAG_RISK,         // Might telefrag or be telefragged
	UNKNOWN                // Haven't assessed yet
};

//=============================================================================
// CTeleportDestination
// Information about a teleport destination
//=============================================================================
struct CTeleportDestination
{
	Vector position;               // Destination position
	QAngle angles;                 // Destination angles
	edict_t* pTargetEntity;        // Target entity (info_teleport_destination, etc.)
	int nearestWaypointId;         // Nearest waypoint to destination
	float distanceToWaypoint;      // Distance from destination to nearest waypoint
	bool isValid;                  // Is this destination valid?
	bool hasFloor;                 // Is there ground below?
	float fallHeight;              // How far would we fall?
	ETeleportSafety safety;        // Safety assessment

	CTeleportDestination()
		: position(0, 0, 0)
		, angles(0, 0, 0)
		, pTargetEntity(nullptr)
		, nearestWaypointId(-1)
		, distanceToWaypoint(0.0f)
		, isValid(false)
		, hasFloor(false)
		, fallHeight(0.0f)
		, safety(ETeleportSafety::UNKNOWN)
	{
	}
};

//=============================================================================
// CTeleportInfo
// Complete information about a teleport entity
//=============================================================================
struct CTeleportInfo
{
	edict_t* pEntity;              // The teleport entity
	ETeleportType type;            // Type of teleport
	Vector mins;                   // Trigger bounding box min
	Vector maxs;                   // Trigger bounding box max
	Vector center;                 // Center of the teleport trigger

	// Destinations (some teleports can have multiple)
	std::vector<CTeleportDestination> destinations;

	// Linked waypoints
	int entranceWaypointId;        // Waypoint at entrance (or -1)
	std::vector<int> exitWaypointIds;  // Waypoints at exits

	// Usage tracking
	float cooldownTime;            // How long to wait between uses
	float lastUseTime;             // When was this last used
	int useCount;                  // How many times used

	// Safety
	ETeleportSafety overallSafety; // Overall safety assessment
	bool hasTelefragRisk;          // Could we telefrag someone?
	bool couldBeTrapped;           // Could we get trapped at exit?

	// Pathfinding
	float pathCost;                // Cost to use this teleport in pathfinding
	bool shouldUse;                // Should bots use this teleport?

	CTeleportInfo()
		: pEntity(nullptr)
		, type(ETeleportType::UNKNOWN)
		, mins(0, 0, 0)
		, maxs(0, 0, 0)
		, center(0, 0, 0)
		, entranceWaypointId(-1)
		, cooldownTime(1.0f)
		, lastUseTime(0.0f)
		, useCount(0)
		, overallSafety(ETeleportSafety::UNKNOWN)
		, hasTelefragRisk(false)
		, couldBeTrapped(false)
		, pathCost(0.0f)
		, shouldUse(true)
	{
	}

	// Check if a point is inside the trigger
	bool containsPoint(const Vector& point) const
	{
		return point.x >= mins.x && point.x <= maxs.x &&
		       point.y >= mins.y && point.y <= maxs.y &&
		       point.z >= mins.z && point.z <= maxs.z;
	}

	// Check if teleport is ready to use
	bool isReady(float currentTime) const
	{
		return (currentTime - lastUseTime) >= cooldownTime;
	}

	// Get the primary destination
	const CTeleportDestination* getPrimaryDestination() const
	{
		if (destinations.empty())
			return nullptr;
		return &destinations[0];
	}

	// Get the best (safest) destination
	const CTeleportDestination* getSafestDestination() const;
};

//=============================================================================
// CTeleportRoute
// A route that includes teleportation
//=============================================================================
struct CTeleportRoute
{
	int startWaypointId;           // Starting waypoint
	int teleportIndex;             // Index into teleport list
	int destinationIndex;          // Which destination to use
	int endWaypointId;             // Ending waypoint
	float totalCost;               // Total pathfinding cost
	float timeSaved;               // Time saved vs walking
	bool isViable;                 // Is this route usable?

	CTeleportRoute()
		: startWaypointId(-1)
		, teleportIndex(-1)
		, destinationIndex(0)
		, endWaypointId(-1)
		, totalCost(0.0f)
		, timeSaved(0.0f)
		, isViable(false)
	{
	}
};

//=============================================================================
// CBotTeleportCooldown
// Per-bot teleport cooldown tracking
//=============================================================================
struct CBotTeleportCooldown
{
	int teleportIndex;             // Which teleport
	float lastUseTime;             // When bot last used it
	int consecutiveUses;           // Times used in quick succession
	bool isBanned;                 // Temporarily banned from using

	CBotTeleportCooldown()
		: teleportIndex(-1)
		, lastUseTime(0.0f)
		, consecutiveUses(0)
		, isBanned(false)
	{
	}
};

//=============================================================================
// CTeleportManager
// Main manager for teleport waypointing
//=============================================================================
class CTeleportManager
{
public:
	static CTeleportManager& instance();

	// Initialization
	void init();
	void clear();

	// Entity scanning
	void scanForTeleports();
	void update();

	// Get teleport info
	int getTeleportCount() const { return static_cast<int>(m_teleports.size()); }
	const CTeleportInfo* getTeleport(int index) const;
	const CTeleportInfo* getTeleportAtPoint(const Vector& point) const;
	const CTeleportInfo* getTeleportForWaypoint(int waypointId) const;

	// Waypoint linking
	void linkWaypointsToTeleports();
	bool createTeleportWaypoints();
	int getEntranceWaypoint(int teleportIndex) const;
	std::vector<int> getExitWaypoints(int teleportIndex) const;

	// Pathfinding integration
	bool isTeleportEdge(int fromWpt, int toWpt) const;
	float getTeleportEdgeCost(int fromWpt, int toWpt) const;
	std::vector<CTeleportRoute> findTeleportRoutes(int startWpt, int endWpt);
	CTeleportRoute findBestTeleportRoute(int startWpt, int endWpt);

	// Route comparison
	float getWalkingDistance(int startWpt, int endWpt) const;
	bool isTeleportFaster(int startWpt, int endWpt, int teleportIndex) const;

	// Safety assessment
	void assessTeleportSafety(CTeleportInfo& teleport);
	ETeleportSafety checkTelefragRisk(const CTeleportInfo& teleport) const;
	bool isExitTrapped(const CTeleportDestination& dest) const;

	// Bot usage
	bool shouldBotUseTeleport(CBot* pBot, int teleportIndex) const;
	void onBotUseTeleport(CBot* pBot, int teleportIndex);
	bool isTeleportOnCooldown(CBot* pBot, int teleportIndex) const;

	// Cooldown management
	void updateCooldowns();
	float getCooldownRemaining(CBot* pBot, int teleportIndex) const;

	// Tactical integration
	bool isTeleportTacticallySound(CBot* pBot, int teleportIndex) const;
	float getTacticalValue(CBot* pBot, int teleportIndex) const;

	// Debug/Visualization
	void debugDrawTeleports() const;
	void printTeleportInfo() const;

private:
	CTeleportManager();
	~CTeleportManager();

	// Prevent copying
	CTeleportManager(const CTeleportManager&) = delete;
	CTeleportManager& operator=(const CTeleportManager&) = delete;

	// Internal helpers
	void scanTriggerTeleports();
	void findTeleportDestinations(CTeleportInfo& teleport);
	int findNearestWaypoint(const Vector& position) const;
	float calculatePathCost(const CTeleportInfo& teleport) const;
	bool traceTeleportPath(const Vector& start, const Vector& end) const;

	// Data
	std::vector<CTeleportInfo> m_teleports;
	std::map<int, std::vector<int>> m_waypointToTeleport;  // waypoint -> teleport indices
	std::map<CBot*, std::vector<CBotTeleportCooldown>> m_botCooldowns;

	float m_lastScanTime;
	float m_lastUpdateTime;

	static CTeleportManager* s_instance;
};

//=============================================================================
// Console command handlers
//=============================================================================
class CCommand;
void Teleport_Scan_Command(const CCommand& args);
void Teleport_Info_Command(const CCommand& args);

#endif // __BOT_TELEPORT_H__
