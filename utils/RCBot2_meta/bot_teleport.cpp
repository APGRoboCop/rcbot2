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

#include "bot_teleport.h"
#include "bot.h"
#include "bot_globals.h"
#include "bot_waypoint.h"
#include "bot_waypoint_locations.h"
#include "bot_gravity.h"
#include "bot_tactical.h"

#include <algorithm>
#include <cstring>
#include <cmath>

// Local trace filter for world/brush-only traces (ignores all entities)
// Named differently to avoid conflict with SDK's CTraceFilterWorldOnly
class CTeleportTraceFilter : public CTraceFilter
{
public:
	virtual ~CTeleportTraceFilter() = default;
	bool ShouldHitEntity(IHandleEntity *pHandleEntity, int contentsMask) override
	{
		return false; // Don't hit any entities, only world geometry
	}
};

// Helper function to replace UTIL_TraceLine (which is server-side only)
static void TeleportTraceLine(const Vector& start, const Vector& end, unsigned int mask, trace_t* tr)
{
	Ray_t ray;
	CTeleportTraceFilter filter;
	std::memset(tr, 0, sizeof(trace_t));
	ray.Init(start, end);
	enginetrace->TraceRay(ray, mask, &filter, tr);
}

// Static instance
CTeleportManager* CTeleportManager::s_instance = nullptr;

//=============================================================================
// CTeleportInfo Implementation
//=============================================================================

const CTeleportDestination* CTeleportInfo::getSafestDestination() const
{
	const CTeleportDestination* safest = nullptr;
	ETeleportSafety bestSafety = ETeleportSafety::DANGEROUS;

	for (const CTeleportDestination& dest : destinations)
	{
		if (!dest.isValid)
			continue;

		if (safest == nullptr ||
		    static_cast<int>(dest.safety) < static_cast<int>(bestSafety))
		{
			safest = &dest;
			bestSafety = dest.safety;
		}
	}

	return safest;
}

const CTeleportDestination* CTeleportInfo::getNextDestination(const Vector& goalPos)
{
	if (destinations.empty())
		return nullptr;

	if (destinations.size() == 1)
		return &destinations[0];

	switch (destMode)
	{
		case EDestinationMode::SINGLE:
			return &destinations[0];

		case EDestinationMode::RANDOM:
		{
			int idx = rand() % static_cast<int>(destinations.size());
			return &destinations[idx];
		}

		case EDestinationMode::SEQUENTIAL:
		{
			lastDestinationUsed = (lastDestinationUsed + 1) % static_cast<int>(destinations.size());
			return &destinations[lastDestinationUsed];
		}

		case EDestinationMode::CLOSEST:
			return getDestinationForGoal(goalPos);

		default:
			return &destinations[0];
	}
}

const CTeleportDestination* CTeleportInfo::getDestinationForGoal(const Vector& goalPos) const
{
	if (destinations.empty())
		return nullptr;

	if (destinations.size() == 1)
		return &destinations[0];

	const CTeleportDestination* closest = nullptr;
	float closestDist = FLT_MAX;

	for (const CTeleportDestination& dest : destinations)
	{
		if (!dest.isValid)
			continue;

		float dist = (dest.position - goalPos).Length();
		if (dist < closestDist)
		{
			closestDist = dist;
			closest = &dest;
		}
	}

	return closest != nullptr ? closest : &destinations[0];
}

//=============================================================================
// CTeleportManager Implementation
//=============================================================================

CTeleportManager& CTeleportManager::instance()
{
	if (s_instance == nullptr)
		s_instance = new CTeleportManager();
	return *s_instance;
}

CTeleportManager::CTeleportManager()
	: m_lastScanTime(0.0f)
	, m_lastUpdateTime(0.0f)
{
}

CTeleportManager::~CTeleportManager()
{
}

void CTeleportManager::init()
{
	clear();
	scanForTeleports();
	linkWaypointsToTeleports();

	// Detect one-way vs two-way teleports
	detectBidirectionalTeleports();

	// Flag teleport exits as tactical ambush points
	flagTeleportExitsAsAmbush();
}

void CTeleportManager::clear()
{
	m_teleports.clear();
	m_waypointToTeleport.clear();
	m_botCooldowns.clear();
	m_lastScanTime = 0.0f;
}

void CTeleportManager::scanForTeleports()
{
	m_teleports.clear();

	scanTriggerTeleports();

	if (!m_teleports.empty())
	{
		// Find destinations for each teleport
		for (CTeleportInfo& teleport : m_teleports)
		{
			findTeleportDestinations(teleport);
			teleport.pathCost = calculatePathCost(teleport);
			assessTeleportSafety(teleport);
		}

		CBotGlobals::botMessage(nullptr, 0, "Found %d teleport entities",
			static_cast<int>(m_teleports.size()));
	}

	m_lastScanTime = gpGlobals->curtime;
}

void CTeleportManager::scanTriggerTeleports()
{
	int maxEnts = gpGlobals->maxEntities;

	for (int i = 0; i < maxEnts; i++)
	{
		edict_t* pEdict = INDEXENT(i);
		if (pEdict == nullptr || pEdict->IsFree())
			continue;

		const char* classname = pEdict->GetClassName();
		if (classname == nullptr)
			continue;

		ETeleportType type = ETeleportType::UNKNOWN;

		if (strcmp(classname, "trigger_teleport") == 0)
			type = ETeleportType::TRIGGER_TELEPORT;
		else if (strcmp(classname, "point_teleport") == 0)
			type = ETeleportType::POINT_TELEPORT;
		else if (strcmp(classname, "trigger_push") == 0)
		{
			// trigger_push can act like a teleport if it has high push speed
			// We'll check for very high push values later
			// For now, skip trigger_push - too many false positives
			continue;
		}

		if (type == ETeleportType::UNKNOWN)
			continue;

		CTeleportInfo teleport;
		teleport.pEntity = pEdict;
		teleport.type = type;

		// Get bounding box
		ICollideable* pCollideable = pEdict->GetCollideable();
		if (pCollideable != nullptr)
		{
			const Vector& origin = pCollideable->GetCollisionOrigin();
			teleport.mins = origin + pCollideable->OBBMins();
			teleport.maxs = origin + pCollideable->OBBMaxs();
			teleport.center = (teleport.mins + teleport.maxs) * 0.5f;
		}
		else
		{
			// Fallback for point teleports - use CBotGlobals utility
			Vector origin = CBotGlobals::entityOrigin(pEdict);
			teleport.center = origin;
			teleport.mins = origin - Vector(32, 32, 32);
			teleport.maxs = origin + Vector(32, 32, 32);
		}

		m_teleports.push_back(teleport);
	}
}

void CTeleportManager::findTeleportDestinations(CTeleportInfo& teleport)
{
	if (teleport.pEntity == nullptr)
		return;

	// In Source engine, trigger_teleport has a "target" key that points to
	// an info_teleport_destination or another entity

	// Try to find the target entity
	// This is tricky because we need to parse keyvalues
	// For now, we'll search for info_teleport_destination entities
	// and try to match them by proximity or naming patterns

	int maxEnts = gpGlobals->maxEntities;

	for (int i = 0; i < maxEnts; i++)
	{
		edict_t* pEdict = INDEXENT(i);
		if (pEdict == nullptr || pEdict->IsFree())
			continue;

		const char* classname = pEdict->GetClassName();
		if (classname == nullptr)
			continue;

		if (strcmp(classname, "info_teleport_destination") == 0 ||
		    strcmp(classname, "info_target") == 0)
		{
			IServerEntity* pServerEntity = pEdict->GetIServerEntity();
			if (pServerEntity == nullptr)
				continue;

			ICollideable* pCollideable = pServerEntity->GetCollideable();
			if (pCollideable == nullptr)
				continue;

			CTeleportDestination dest;
			dest.pTargetEntity = pEdict;
			dest.position = pCollideable->GetCollisionOrigin();
			dest.angles = pCollideable->GetCollisionAngles();
			dest.isValid = true;

			// Find nearest waypoint to destination
			dest.nearestWaypointId = findNearestWaypoint(dest.position);
			if (dest.nearestWaypointId >= 0)
			{
				CWaypoint* pWpt = CWaypoints::getWaypoint(dest.nearestWaypointId);
				if (pWpt != nullptr)
				{
					dest.distanceToWaypoint = (dest.position - pWpt->getOrigin()).Length();
				}
			}

			// Check for ground and fall height
			trace_t tr;
			Vector endPos = dest.position - Vector(0, 0, 1024);
			TeleportTraceLine(dest.position, endPos, MASK_PLAYERSOLID_BRUSHONLY, &tr);

			if (tr.DidHit())
			{
				dest.hasFloor = true;
				dest.fallHeight = dest.position.z - tr.endpos.z;

				// Assess fall damage
				CGravityManager& gravity = CGravityManager::instance();
				if (gravity.estimateDamage(dest.fallHeight) > 0.0f)
				{
					if (gravity.estimateDamage(dest.fallHeight) >= 100.0f)
						dest.safety = ETeleportSafety::DANGEROUS;
					else
						dest.safety = ETeleportSafety::RISKY;
				}
				else
				{
					dest.safety = ETeleportSafety::SAFE;
				}
			}
			else
			{
				dest.hasFloor = false;
				dest.fallHeight = 1024.0f;
				dest.safety = ETeleportSafety::DANGEROUS;
			}

			teleport.destinations.push_back(dest);
		}
	}

	// If we couldn't find any destinations, the teleport might be broken
	// or use a different mechanism
	if (teleport.destinations.empty())
	{
		teleport.shouldUse = false;
	}
}

void CTeleportManager::update()
{
	float curTime = gpGlobals->curtime;

	// Update periodically (every 5 seconds)
	if (curTime - m_lastUpdateTime < 5.0f)
		return;

	m_lastUpdateTime = curTime;

	// Update cooldowns
	updateCooldowns();

	// Re-assess safety for teleports that might have changed
	for (CTeleportInfo& teleport : m_teleports)
	{
		// Check telefrag risk (players near exits)
		teleport.hasTelefragRisk = (checkTelefragRisk(teleport) == ETeleportSafety::TELEFRAG_RISK);
	}
}

const CTeleportInfo* CTeleportManager::getTeleport(int index) const
{
	if (index < 0 || index >= static_cast<int>(m_teleports.size()))
		return nullptr;
	return &m_teleports[index];
}

const CTeleportInfo* CTeleportManager::getTeleportAtPoint(const Vector& point) const
{
	for (const CTeleportInfo& teleport : m_teleports)
	{
		if (teleport.containsPoint(point))
			return &teleport;
	}
	return nullptr;
}

const CTeleportInfo* CTeleportManager::getTeleportForWaypoint(int waypointId) const
{
	auto it = m_waypointToTeleport.find(waypointId);
	if (it == m_waypointToTeleport.end() || it->second.empty())
		return nullptr;

	return getTeleport(it->second[0]);
}

void CTeleportManager::linkWaypointsToTeleports()
{
	m_waypointToTeleport.clear();

	int numWaypoints = CWaypoints::numWaypoints();

	for (int i = 0; i < numWaypoints; i++)
	{
		CWaypoint* pWpt = CWaypoints::getWaypoint(i);
		if (pWpt == nullptr || !pWpt->isUsed())
			continue;

		Vector origin = pWpt->getOrigin();

		// Check if waypoint is inside any teleport
		for (size_t t = 0; t < m_teleports.size(); t++)
		{
			CTeleportInfo& teleport = m_teleports[t];

			if (teleport.containsPoint(origin))
			{
				teleport.entranceWaypointId = i;
				m_waypointToTeleport[i].push_back(static_cast<int>(t));
			}

			// Check if waypoint is near any exit
			for (const CTeleportDestination& dest : teleport.destinations)
			{
				if (dest.nearestWaypointId == i)
				{
					teleport.exitWaypointIds.push_back(i);
				}
			}
		}
	}

	// Log results
	int linkedEntrances = 0;
	int linkedExits = 0;
	for (const CTeleportInfo& teleport : m_teleports)
	{
		if (teleport.entranceWaypointId >= 0)
			linkedEntrances++;
		linkedExits += static_cast<int>(teleport.exitWaypointIds.size());
	}

	if (linkedEntrances > 0 || linkedExits > 0)
	{
		CBotGlobals::botMessage(nullptr, 0, "Linked %d teleport entrances, %d exits to waypoints",
			linkedEntrances, linkedExits);
	}
}

bool CTeleportManager::createTeleportWaypoints()
{
	int created = 0;

	for (CTeleportInfo& teleport : m_teleports)
	{
		// Create entrance waypoint if needed
		if (teleport.entranceWaypointId < 0)
		{
			int newWpt = CWaypoints::addWaypoint(nullptr, teleport.center,
				TeleportWaypointFlags::W_FL_TELEPORT_ENTRANCE, true, 0, 0, 0.0f);

			if (newWpt >= 0)
			{
				teleport.entranceWaypointId = newWpt;
				created++;
			}
		}

		// Create exit waypoints if needed
		for (CTeleportDestination& dest : teleport.destinations)
		{
			if (!dest.isValid)
				continue;

			if (dest.nearestWaypointId < 0 || dest.distanceToWaypoint > 128.0f)
			{
				int newWpt = CWaypoints::addWaypoint(nullptr, dest.position,
					TeleportWaypointFlags::W_FL_TELEPORT_EXIT, true, 0, 0, 0.0f);

				if (newWpt >= 0)
				{
					dest.nearestWaypointId = newWpt;
					dest.distanceToWaypoint = 0.0f;
					teleport.exitWaypointIds.push_back(newWpt);
					created++;
				}
			}
		}
	}

	if (created > 0)
	{
		CBotGlobals::botMessage(nullptr, 0, "Created %d teleport waypoints", created);
	}

	return created > 0;
}

int CTeleportManager::getEntranceWaypoint(int teleportIndex) const
{
	const CTeleportInfo* pTeleport = getTeleport(teleportIndex);
	if (pTeleport == nullptr)
		return -1;
	return pTeleport->entranceWaypointId;
}

std::vector<int> CTeleportManager::getExitWaypoints(int teleportIndex) const
{
	const CTeleportInfo* pTeleport = getTeleport(teleportIndex);
	if (pTeleport == nullptr)
		return std::vector<int>();
	return pTeleport->exitWaypointIds;
}

bool CTeleportManager::isTeleportEdge(int fromWpt, int toWpt) const
{
	// Check if this is a teleport connection
	for (size_t i = 0; i < m_teleports.size(); i++)
	{
		const CTeleportInfo& teleport = m_teleports[i];

		if (teleport.entranceWaypointId == fromWpt)
		{
			for (int exitWpt : teleport.exitWaypointIds)
			{
				if (exitWpt == toWpt)
					return true;
			}
		}
	}

	return false;
}

float CTeleportManager::getTeleportEdgeCost(int fromWpt, int toWpt) const
{
	for (const CTeleportInfo& teleport : m_teleports)
	{
		if (teleport.entranceWaypointId == fromWpt)
		{
			for (int exitWpt : teleport.exitWaypointIds)
			{
				if (exitWpt == toWpt)
					return teleport.pathCost;
			}
		}
	}

	return 0.0f;
}

std::vector<CTeleportRoute> CTeleportManager::findTeleportRoutes(int startWpt, int endWpt)
{
	std::vector<CTeleportRoute> routes;

	CWaypoint* pStart = CWaypoints::getWaypoint(startWpt);
	CWaypoint* pEnd = CWaypoints::getWaypoint(endWpt);

	if (pStart == nullptr || pEnd == nullptr)
		return routes;

	Vector startPos = pStart->getOrigin();
	Vector endPos = pEnd->getOrigin();

	for (size_t i = 0; i < m_teleports.size(); i++)
	{
		const CTeleportInfo& teleport = m_teleports[i];

		if (!teleport.shouldUse)
			continue;

		if (teleport.entranceWaypointId < 0)
			continue;

		// Check each destination
		for (size_t d = 0; d < teleport.destinations.size(); d++)
		{
			const CTeleportDestination& dest = teleport.destinations[d];

			if (!dest.isValid || dest.nearestWaypointId < 0)
				continue;

			CTeleportRoute route;
			route.startWaypointId = startWpt;
			route.teleportIndex = static_cast<int>(i);
			route.destinationIndex = static_cast<int>(d);
			route.endWaypointId = endWpt;

			// Calculate cost: distance to teleport + teleport cost + distance from exit to goal
			float distToTeleport = (startPos - teleport.center).Length();
			float distFromExit = (dest.position - endPos).Length();

			route.totalCost = distToTeleport + teleport.pathCost + distFromExit;

			// Calculate time saved vs walking
			float walkingDist = (startPos - endPos).Length();
			route.timeSaved = walkingDist - route.totalCost;

			route.isViable = (route.timeSaved > 100.0f); // At least 100 units saved

			if (route.isViable)
				routes.push_back(route);
		}
	}

	// Sort by total cost
	std::sort(routes.begin(), routes.end(),
		[](const CTeleportRoute& a, const CTeleportRoute& b) {
			return a.totalCost < b.totalCost;
		});

	return routes;
}

CTeleportRoute CTeleportManager::findBestTeleportRoute(int startWpt, int endWpt)
{
	std::vector<CTeleportRoute> routes = findTeleportRoutes(startWpt, endWpt);

	if (routes.empty())
		return CTeleportRoute();

	return routes[0];
}

float CTeleportManager::getWalkingDistance(int startWpt, int endWpt) const
{
	CWaypoint* pStart = CWaypoints::getWaypoint(startWpt);
	CWaypoint* pEnd = CWaypoints::getWaypoint(endWpt);

	if (pStart == nullptr || pEnd == nullptr)
		return 0.0f;

	// Simple straight-line distance
	// In practice, this should use actual pathfinding
	return (pStart->getOrigin() - pEnd->getOrigin()).Length();
}

bool CTeleportManager::isTeleportFaster(int startWpt, int endWpt, int teleportIndex) const
{
	const CTeleportInfo* pTeleport = getTeleport(teleportIndex);
	if (pTeleport == nullptr)
		return false;

	CWaypoint* pStart = CWaypoints::getWaypoint(startWpt);
	CWaypoint* pEnd = CWaypoints::getWaypoint(endWpt);

	if (pStart == nullptr || pEnd == nullptr)
		return false;

	float walkingDist = getWalkingDistance(startWpt, endWpt);

	// Calculate teleport distance
	float distToTeleport = (pStart->getOrigin() - pTeleport->center).Length();
	float distFromExit = 0.0f;

	const CTeleportDestination* pDest = pTeleport->getSafestDestination();
	if (pDest != nullptr)
	{
		distFromExit = (pDest->position - pEnd->getOrigin()).Length();
	}

	float teleportDist = distToTeleport + pTeleport->pathCost + distFromExit;

	return teleportDist < walkingDist;
}

void CTeleportManager::assessTeleportSafety(CTeleportInfo& teleport)
{
	ETeleportSafety worstSafety = ETeleportSafety::SAFE;

	for (CTeleportDestination& dest : teleport.destinations)
	{
		if (!dest.isValid)
			continue;

		// Already assessed during destination finding
		if (static_cast<int>(dest.safety) > static_cast<int>(worstSafety))
		{
			worstSafety = dest.safety;
		}

		// Check if exit could be trapped
		if (isExitTrapped(dest))
		{
			teleport.couldBeTrapped = true;
			if (worstSafety == ETeleportSafety::SAFE)
				worstSafety = ETeleportSafety::RISKY;
		}
	}

	teleport.overallSafety = worstSafety;

	// Mark dangerous teleports as unusable
	if (worstSafety == ETeleportSafety::DANGEROUS)
	{
		teleport.shouldUse = false;
	}
}

ETeleportSafety CTeleportManager::checkTelefragRisk(const CTeleportInfo& teleport) const
{
	// Check if there are players near any exit
	for (const CTeleportDestination& dest : teleport.destinations)
	{
		if (!dest.isValid)
			continue;

		// Check for players near this exit
		// In a real implementation, we'd iterate through players
		// For now, assume no telefrag risk
	}

	return ETeleportSafety::SAFE;
}

bool CTeleportManager::isExitTrapped(const CTeleportDestination& dest) const
{
	if (!dest.isValid)
		return true;

	// Check if exit has sufficient room
	trace_t tr;

	// Check for ceiling
	TeleportTraceLine(dest.position, dest.position + Vector(0, 0, 72),
		MASK_PLAYERSOLID_BRUSHONLY, &tr);

	if (tr.fraction < 1.0f)
	{
		// Low ceiling - might be trapped
		float headroom = 72.0f * tr.fraction;
		if (headroom < 36.0f)
			return true;
	}

	// Check for walls on all sides
	int blockedSides = 0;
	Vector directions[] = {
		Vector(1, 0, 0), Vector(-1, 0, 0),
		Vector(0, 1, 0), Vector(0, -1, 0)
	};

	for (const Vector& dir : directions)
	{
		TeleportTraceLine(dest.position, dest.position + dir * 32.0f,
			MASK_PLAYERSOLID_BRUSHONLY, &tr);

		if (tr.fraction < 0.5f)
			blockedSides++;
	}

	// If 3+ sides blocked, might be trapped
	return blockedSides >= 3;
}

bool CTeleportManager::shouldBotUseTeleport(CBot* pBot, int teleportIndex) const
{
	const CTeleportInfo* pTeleport = getTeleport(teleportIndex);
	if (pTeleport == nullptr)
		return false;

	if (!pTeleport->shouldUse)
		return false;

	// Check cooldown
	if (isTeleportOnCooldown(pBot, teleportIndex))
		return false;

	// Check safety vs bot's health
	if (pBot != nullptr && pTeleport->overallSafety == ETeleportSafety::RISKY)
	{
		float health = pBot->getHealthPercent() * 100.0f;
		if (health < 50.0f)
			return false; // Don't take risks when low on health
	}

	// Check telefrag risk
	if (pTeleport->hasTelefragRisk)
		return false;

	return true;
}

void CTeleportManager::onBotUseTeleport(CBot* pBot, int teleportIndex)
{
	if (pBot == nullptr)
		return;

	if (teleportIndex < 0 || teleportIndex >= static_cast<int>(m_teleports.size()))
		return;

	m_teleports[teleportIndex].lastUseTime = gpGlobals->curtime;
	m_teleports[teleportIndex].useCount++;

	// Update bot's cooldown
	auto& cooldowns = m_botCooldowns[pBot];

	bool found = false;
	for (CBotTeleportCooldown& cd : cooldowns)
	{
		if (cd.teleportIndex == teleportIndex)
		{
			cd.lastUseTime = gpGlobals->curtime;
			cd.consecutiveUses++;

			// Ban if used too many times in quick succession
			if (cd.consecutiveUses >= 3)
			{
				cd.isBanned = true;
			}

			found = true;
			break;
		}
	}

	if (!found)
	{
		CBotTeleportCooldown cd;
		cd.teleportIndex = teleportIndex;
		cd.lastUseTime = gpGlobals->curtime;
		cd.consecutiveUses = 1;
		cooldowns.push_back(cd);
	}
}

bool CTeleportManager::isTeleportOnCooldown(CBot* pBot, int teleportIndex) const
{
	if (pBot == nullptr)
		return false;

	auto it = m_botCooldowns.find(pBot);
	if (it == m_botCooldowns.end())
		return false;

	for (const CBotTeleportCooldown& cd : it->second)
	{
		if (cd.teleportIndex == teleportIndex)
		{
			if (cd.isBanned)
				return true;

			const CTeleportInfo* pTeleport = getTeleport(teleportIndex);
			if (pTeleport == nullptr)
				return false;

			float timeSince = gpGlobals->curtime - cd.lastUseTime;
			return timeSince < pTeleport->cooldownTime;
		}
	}

	return false;
}

void CTeleportManager::updateCooldowns()
{
	float curTime = gpGlobals->curtime;

	for (auto& pair : m_botCooldowns)
	{
		for (CBotTeleportCooldown& cd : pair.second)
		{
			// Reset consecutive use counter after 10 seconds
			if (curTime - cd.lastUseTime > 10.0f)
			{
				cd.consecutiveUses = 0;
				cd.isBanned = false;
			}
		}
	}
}

float CTeleportManager::getCooldownRemaining(CBot* pBot, int teleportIndex) const
{
	if (pBot == nullptr)
		return 0.0f;

	auto it = m_botCooldowns.find(pBot);
	if (it == m_botCooldowns.end())
		return 0.0f;

	for (const CBotTeleportCooldown& cd : it->second)
	{
		if (cd.teleportIndex == teleportIndex)
		{
			const CTeleportInfo* pTeleport = getTeleport(teleportIndex);
			if (pTeleport == nullptr)
				return 0.0f;

			float timeSince = gpGlobals->curtime - cd.lastUseTime;
			float remaining = pTeleport->cooldownTime - timeSince;
			return remaining > 0.0f ? remaining : 0.0f;
		}
	}

	return 0.0f;
}

bool CTeleportManager::isTeleportTacticallySound(CBot* pBot, int teleportIndex) const
{
	const CTeleportInfo* pTeleport = getTeleport(teleportIndex);
	if (pTeleport == nullptr || pBot == nullptr)
		return false;

	// Check if exit is in a bad tactical position
	// (near enemies, in open area when enemies are near, etc.)

	// For now, basic check - always tactically sound if safe
	return pTeleport->overallSafety == ETeleportSafety::SAFE;
}

float CTeleportManager::getTacticalValue(CBot* pBot, int teleportIndex) const
{
	const CTeleportInfo* pTeleport = getTeleport(teleportIndex);
	if (pTeleport == nullptr)
		return 0.0f;

	float value = 1.0f;

	// Reduce value for risky teleports
	switch (pTeleport->overallSafety)
	{
		case ETeleportSafety::SAFE:
			value = 1.0f;
			break;
		case ETeleportSafety::RISKY:
			value = 0.6f;
			break;
		case ETeleportSafety::DANGEROUS:
			value = 0.2f;
			break;
		case ETeleportSafety::TELEFRAG_RISK:
			value = 0.1f;
			break;
		default:
			value = 0.5f;
			break;
	}

	// Bonus for frequently used teleports (they're probably good)
	if (pTeleport->useCount > 10)
		value *= 1.2f;

	return value;
}

void CTeleportManager::printTeleportInfo() const
{
	CBotGlobals::botMessage(nullptr, 0, "=== Teleport Information ===");
	CBotGlobals::botMessage(nullptr, 0, "Total teleports: %d", static_cast<int>(m_teleports.size()));

	for (size_t i = 0; i < m_teleports.size(); i++)
	{
		const CTeleportInfo& teleport = m_teleports[i];

		const char* typeStr = "Unknown";
		switch (teleport.type)
		{
			case ETeleportType::TRIGGER_TELEPORT: typeStr = "trigger_teleport"; break;
			case ETeleportType::POINT_TELEPORT: typeStr = "point_teleport"; break;
			case ETeleportType::TRIGGER_PUSH: typeStr = "trigger_push"; break;
			case ETeleportType::PORTAL: typeStr = "portal"; break;
			default: break;
		}

		const char* safetyStr = "Unknown";
		switch (teleport.overallSafety)
		{
			case ETeleportSafety::SAFE: safetyStr = "Safe"; break;
			case ETeleportSafety::RISKY: safetyStr = "Risky"; break;
			case ETeleportSafety::DANGEROUS: safetyStr = "Dangerous"; break;
			case ETeleportSafety::TELEFRAG_RISK: safetyStr = "Telefrag Risk"; break;
			default: break;
		}

		CBotGlobals::botMessage(nullptr, 0, "[%d] %s at (%.0f, %.0f, %.0f) - %s - %d destinations - Wpt: %d",
			static_cast<int>(i), typeStr,
			teleport.center.x, teleport.center.y, teleport.center.z,
			safetyStr,
			static_cast<int>(teleport.destinations.size()),
			teleport.entranceWaypointId);
	}
}

int CTeleportManager::findNearestWaypoint(const Vector& position) const
{
	int nearest = -1;
	float nearestDist = FLT_MAX;

	int numWaypoints = CWaypoints::numWaypoints();
	for (int i = 0; i < numWaypoints; i++)
	{
		CWaypoint* pWpt = CWaypoints::getWaypoint(i);
		if (pWpt == nullptr || !pWpt->isUsed())
			continue;

		float dist = (position - pWpt->getOrigin()).Length();
		if (dist < nearestDist)
		{
			nearestDist = dist;
			nearest = i;
		}
	}

	return nearest;
}

float CTeleportManager::calculatePathCost(const CTeleportInfo& teleport) const
{
	// Base cost for using a teleport (equivalent to walking 100 units)
	float cost = 100.0f;

	// Add cost based on safety
	switch (teleport.overallSafety)
	{
		case ETeleportSafety::SAFE:
			break;
		case ETeleportSafety::RISKY:
			cost += 200.0f;
			break;
		case ETeleportSafety::DANGEROUS:
			cost += 1000.0f;
			break;
		case ETeleportSafety::TELEFRAG_RISK:
			cost += 500.0f;
			break;
		default:
			cost += 300.0f;
			break;
	}

	return cost;
}

bool CTeleportManager::traceTeleportPath(const Vector& start, const Vector& end) const
{
	trace_t tr;
	TeleportTraceLine(start, end, MASK_PLAYERSOLID_BRUSHONLY, &tr);
	return tr.fraction >= 1.0f;
}

void CTeleportManager::detectBidirectionalTeleports()
{
	// Check each teleport to see if there's a return teleport at any destination
	for (size_t i = 0; i < m_teleports.size(); i++)
	{
		CTeleportInfo& teleportA = m_teleports[i];
		teleportA.isBidirectional = false;
		teleportA.returnTeleportIndex = -1;

		// Check if any destination has a teleport that returns to our entrance
		for (const CTeleportDestination& destA : teleportA.destinations)
		{
			if (!destA.isValid)
				continue;

			// Look for another teleport at this destination
			for (size_t j = 0; j < m_teleports.size(); j++)
			{
				if (i == j)
					continue;

				CTeleportInfo& teleportB = m_teleports[j];

				// Is teleport B located at our destination?
				if (teleportB.containsPoint(destA.position) ||
				    (destA.position - teleportB.center).Length() < 128.0f)
				{
					// Does teleport B go back to our entrance?
					for (const CTeleportDestination& destB : teleportB.destinations)
					{
						if (!destB.isValid)
							continue;

						if (teleportA.containsPoint(destB.position) ||
						    (destB.position - teleportA.center).Length() < 128.0f)
						{
							// Found bidirectional pair!
							teleportA.isBidirectional = true;
							teleportA.returnTeleportIndex = static_cast<int>(j);
							teleportB.isBidirectional = true;
							teleportB.returnTeleportIndex = static_cast<int>(i);
							break;
						}
					}
				}

				if (teleportA.isBidirectional)
					break;
			}

			if (teleportA.isBidirectional)
				break;
		}
	}

	// Log results
	int bidirectionalCount = 0;
	for (const CTeleportInfo& tp : m_teleports)
	{
		if (tp.isBidirectional)
			bidirectionalCount++;
	}

	if (bidirectionalCount > 0)
	{
		CBotGlobals::botMessage(nullptr, 0, "Detected %d bidirectional teleport pairs", bidirectionalCount / 2);
	}
}

bool CTeleportManager::hasBidirectionalReturn(int teleportIndex) const
{
	const CTeleportInfo* pTeleport = getTeleport(teleportIndex);
	if (pTeleport == nullptr)
		return false;

	return pTeleport->isBidirectional && pTeleport->returnTeleportIndex >= 0;
}

void CTeleportManager::flagTeleportExitsAsAmbush()
{
	// Mark teleport exits as potential ambush points in tactical system
	// Players teleporting in are vulnerable for a moment

	CTacticalDataManager& tacticalMgr = CTacticalDataManager::instance();

	int flaggedCount = 0;

	for (const CTeleportInfo& teleport : m_teleports)
	{
		if (!teleport.shouldUse)
			continue;

		for (int exitWpt : teleport.exitWaypointIds)
		{
			if (exitWpt < 0)
				continue;

			CTacticalInfo* pInfo = tacticalMgr.getTacticalInfo(exitWpt);
			if (pInfo != nullptr)
			{
				// Flag as potential ambush point - enemies coming from teleport
				// are momentarily disoriented and vulnerable
				pInfo->addTacticalFlag(TacticalFlags::AMBUSH_POINT);
				pInfo->addTacticalFlag(TacticalFlags::HIGH_TRAFFIC);
				flaggedCount++;
			}
		}
	}

	if (flaggedCount > 0)
	{
		CBotGlobals::botMessage(nullptr, 0, "Flagged %d teleport exits as tactical ambush points", flaggedCount);
	}
}

void CTeleportManager::debugDrawTeleports() const
{
#ifndef __linux__
	extern IVDebugOverlay* debugoverlay;
	if (debugoverlay == nullptr)
		return;

	for (size_t i = 0; i < m_teleports.size(); i++)
	{
		const CTeleportInfo& teleport = m_teleports[i];

		// Draw teleport trigger box
		int r = 0, g = 255, b = 255;  // Cyan for teleports

		// Change color based on safety
		switch (teleport.overallSafety)
		{
			case ETeleportSafety::SAFE:
				r = 0; g = 255; b = 0;  // Green
				break;
			case ETeleportSafety::RISKY:
				r = 255; g = 255; b = 0;  // Yellow
				break;
			case ETeleportSafety::DANGEROUS:
				r = 255; g = 0; b = 0;  // Red
				break;
			case ETeleportSafety::TELEFRAG_RISK:
				r = 255; g = 0; b = 255;  // Magenta
				break;
			default:
				break;
		}

		// Draw entrance box
		debugoverlay->AddBoxOverlay(teleport.center,
			teleport.mins - teleport.center, teleport.maxs - teleport.center,
			QAngle(0, 0, 0), r, g, b, 50, 1.0f);

		// Draw lines to each destination
		for (const CTeleportDestination& dest : teleport.destinations)
		{
			if (!dest.isValid)
				continue;

			// Draw line from entrance to destination
			debugoverlay->AddLineOverlayAlpha(teleport.center, dest.position,
				r, g, b, 200, false, 1.0f);

			// Draw a small sphere at destination
			debugoverlay->AddBoxOverlay(dest.position,
				Vector(-16, -16, -16), Vector(16, 16, 16),
				QAngle(0, 0, 0), r, g, b, 100, 1.0f);
		}

		// Draw bidirectional indicator
		if (teleport.isBidirectional)
		{
			debugoverlay->AddTextOverlay(teleport.center + Vector(0, 0, 32),
				1.0f, "<->");
		}
	}
#endif
}

//=============================================================================
// Command Handlers
//=============================================================================

void Teleport_Scan_Command(const CCommand& args)
{
	CTeleportManager& mgr = CTeleportManager::instance();
	mgr.scanForTeleports();
	mgr.linkWaypointsToTeleports();
}

void Teleport_Info_Command(const CCommand& args)
{
	CTeleportManager::instance().printTeleportInfo();
}
