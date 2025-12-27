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

#include "bot_gravity.h"
#include "bot.h"
#include "bot_globals.h"
#include "bot_waypoint.h"
#include "bot_cvars.h"
#include "bot_navtest.h"

#include <algorithm>
#include <cmath>

// Static instance
CGravityManager* CGravityManager::s_instance = nullptr;

//=============================================================================
// CGravityInfo Implementation
//=============================================================================

CGravityInfo::CGravityInfo()
	: m_gravity(FallDamage::DEFAULT_GRAVITY)
	, m_lastUpdateTime(0.0f)
{
}

void CGravityInfo::update()
{
	if (sv_gravity.IsValid())
	{
		m_gravity = sv_gravity.GetFloat();
	}
	else
	{
		m_gravity = FallDamage::DEFAULT_GRAVITY;
	}

	m_lastUpdateTime = gpGlobals->curtime;
}

bool CGravityInfo::isNonStandardGravity() const
{
	// Consider gravity non-standard if it differs by more than 10%
	float ratio = m_gravity / FallDamage::DEFAULT_GRAVITY;
	return ratio < 0.9f || ratio > 1.1f;
}

float CGravityInfo::calculateFallDamage(float fallHeight) const
{
	if (fallHeight <= 0.0f)
		return 0.0f;

	// Calculate fall speed: v = sqrt(2 * g * h)
	float fallSpeed = getFallSpeed(fallHeight);

	// No damage below threshold
	if (fallSpeed < FallDamage::FALL_DAMAGE_THRESHOLD)
		return 0.0f;

	// Source engine fall damage formula (approximate):
	// damage = (fallSpeed - threshold) * damagePerUnit
	// For default gravity, ~100 damage at fatal speed
	float excessSpeed = fallSpeed - FallDamage::FALL_DAMAGE_THRESHOLD;
	float damageRange = FallDamage::FATAL_FALL_SPEED - FallDamage::FALL_DAMAGE_THRESHOLD;

	// Scale damage from 0 to 100
	float damage = (excessSpeed / damageRange) * 100.0f;

	// Cap at 100 (fatal)
	return std::min(damage, 100.0f);
}

float CGravityInfo::getMaxSafeFallHeight() const
{
	// Height where fall speed equals damage threshold
	// h = v^2 / (2 * g)
	return getHeightFromSpeed(FallDamage::FALL_DAMAGE_THRESHOLD);
}

float CGravityInfo::getFatalFallHeight() const
{
	return getHeightFromSpeed(FallDamage::FATAL_FALL_SPEED);
}

bool CGravityInfo::wouldCauseFallDamage(float fallHeight) const
{
	return getFallSpeed(fallHeight) >= FallDamage::FALL_DAMAGE_THRESHOLD;
}

bool CGravityInfo::wouldBeFatal(float fallHeight) const
{
	return getFallSpeed(fallHeight) >= FallDamage::FATAL_FALL_SPEED;
}

float CGravityInfo::getFallSpeed(float height) const
{
	return CalculateFallSpeed(height, m_gravity);
}

float CGravityInfo::getHeightFromSpeed(float speed) const
{
	return CalculateFallHeight(speed, m_gravity);
}

float CGravityInfo::getFallTime(float height) const
{
	return CalculateFallTime(height, m_gravity);
}

float CGravityInfo::getPathCostModifier(float fallHeight) const
{
	float damage = calculateFallDamage(fallHeight);

	if (damage <= 0.0f)
		return 1.0f;  // No extra cost

	if (damage >= 100.0f)
		return 100.0f; // Very high cost for fatal falls

	// Exponential cost increase with damage
	// 10 damage = 1.5x cost
	// 25 damage = 3x cost
	// 50 damage = 10x cost
	return 1.0f + (damage / 10.0f) * (damage / 10.0f);
}

//=============================================================================
// CGravityPathAnalyzer Implementation
//=============================================================================

CGravityPathAnalyzer::CGravityPathAnalyzer()
	: m_lastAnalysisGravity(0.0f)
{
}

CWaypointFallInfo CGravityPathAnalyzer::analyzeConnection(CWaypoint* pFrom, CWaypoint* pTo)
{
	CWaypointFallInfo info;

	if (pFrom == nullptr || pTo == nullptr)
		return info;

	info.sourceWaypointId = CWaypoints::getWaypointIndex(pFrom);
	info.destWaypointId = CWaypoints::getWaypointIndex(pTo);

	Vector fromOrigin = pFrom->getOrigin();
	Vector toOrigin = pTo->getOrigin();

	// Calculate height difference
	info.heightDifference = fromOrigin.z - toOrigin.z;

	// Only positive height difference = falling
	if (info.heightDifference > 0.0f)
	{
		info.estimatedDamage = m_gravityInfo.calculateFallDamage(info.heightDifference);
		info.isSafe = (info.estimatedDamage <= 0.0f);
	}
	else
	{
		// Going up - check if jump is required
		info.heightDifference = 0.0f;
		info.estimatedDamage = 0.0f;
		info.isSafe = true;

		// Check if the height difference requires a jump
		float heightUp = toOrigin.z - fromOrigin.z;
		if (heightUp > 0.0f && heightUp <= FallDamage::MAX_JUMP_HEIGHT)
		{
			info.requiresJump = true;
		}
		else if (heightUp > FallDamage::MAX_JUMP_HEIGHT)
		{
			// Can't reach - might need ladder, elevator, etc.
			// Check for W_FL_LADDER or W_FL_LIFT flags
			int toFlags = pTo->getFlags();
			if (!(toFlags & CWaypointTypes::W_FL_LADDER) &&
				!(toFlags & CWaypointTypes::W_FL_LIFT))
			{
				info.isSafe = false; // Can't traverse normally
			}
		}
	}

	return info;
}

bool CGravityPathAnalyzer::shouldAvoidPath(CBot* pBot, int fromWpt, int toWpt)
{
	const CWaypointFallInfo* pInfo = getFallInfo(fromWpt, toWpt);
	if (pInfo == nullptr)
	{
		// Analyze on-demand if not cached
		CWaypoint* pFrom = CWaypoints::getWaypoint(fromWpt);
		CWaypoint* pTo = CWaypoints::getWaypoint(toWpt);
		if (pFrom == nullptr || pTo == nullptr)
			return false;

		CWaypointFallInfo info = analyzeConnection(pFrom, pTo);
		return !info.isSafe;
	}

	if (!pInfo->isSafe)
		return true;

	// Check if damage would be too high given bot's current health
	if (pBot != nullptr)
	{
		float health = pBot->getHealthPercent() * 100.0f;
		if (pInfo->estimatedDamage >= health)
			return true; // Would kill the bot
	}

	return false;
}

float CGravityPathAnalyzer::getPathCost(int fromWpt, int toWpt) const
{
	const CWaypointFallInfo* pInfo = getFallInfo(fromWpt, toWpt);
	if (pInfo == nullptr)
	{
		CWaypoint* pFrom = CWaypoints::getWaypoint(fromWpt);
		CWaypoint* pTo = CWaypoints::getWaypoint(toWpt);
		if (pFrom == nullptr || pTo == nullptr)
			return 1.0f;

		// Calculate height difference directly (const-safe)
		float heightDiff = pFrom->getOrigin().z - pTo->getOrigin().z;
		return m_gravityInfo.getPathCostModifier(heightDiff);
	}

	return m_gravityInfo.getPathCostModifier(pInfo->heightDifference);
}

void CGravityPathAnalyzer::analyzeAllConnections()
{
	m_connectionInfo.clear();
	m_gravityInfo.update();
	m_lastAnalysisGravity = m_gravityInfo.getGravity();

	int numWaypoints = CWaypoints::numWaypoints();

	for (int i = 0; i < numWaypoints; i++)
	{
		CWaypoint* pFrom = CWaypoints::getWaypoint(i);
		if (pFrom == nullptr || !pFrom->isUsed())
			continue;

		// Check all paths from this waypoint
		for (int j = 0; j < pFrom->numPaths(); j++)
		{
			int toIdx = pFrom->getPath(j);
			CWaypoint* pTo = CWaypoints::getWaypoint(toIdx);
			if (pTo == nullptr || !pTo->isUsed())
				continue;

			CWaypointFallInfo info = analyzeConnection(pFrom, pTo);
			if (info.heightDifference > 0.0f)
			{
				// Only store connections with falls
				m_connectionInfo.push_back(info);
			}
		}
	}

	CBotGlobals::botMessage(nullptr, 0, "Gravity analysis: %d connections with falls, current gravity: %.0f",
		static_cast<int>(m_connectionInfo.size()), m_lastAnalysisGravity);
}

void CGravityPathAnalyzer::updateForGravity()
{
	m_gravityInfo.update();

	// Re-analyze if gravity changed significantly
	float currentGravity = m_gravityInfo.getGravity();
	float ratio = currentGravity / m_lastAnalysisGravity;

	if (ratio < 0.9f || ratio > 1.1f)
	{
		// Re-calculate damage estimates for all cached connections
		for (CWaypointFallInfo& info : m_connectionInfo)
		{
			info.estimatedDamage = m_gravityInfo.calculateFallDamage(info.heightDifference);
			info.isSafe = (info.estimatedDamage <= 0.0f);
		}

		m_lastAnalysisGravity = currentGravity;
	}
}

const CWaypointFallInfo* CGravityPathAnalyzer::getFallInfo(int fromWpt, int toWpt) const
{
	for (const CWaypointFallInfo& info : m_connectionInfo)
	{
		if (info.sourceWaypointId == fromWpt && info.destWaypointId == toWpt)
			return &info;
	}
	return nullptr;
}

std::vector<CWaypointFallInfo> CGravityPathAnalyzer::getDangerousConnections(float minDamage) const
{
	std::vector<CWaypointFallInfo> result;

	for (const CWaypointFallInfo& info : m_connectionInfo)
	{
		if (info.estimatedDamage >= minDamage)
			result.push_back(info);
	}

	return result;
}

std::vector<int> CGravityPathAnalyzer::findSafePath(int startWpt, int endWpt, float maxFallDamage)
{
	// TODO: Implement modified A* that avoids dangerous falls
	// For now, return empty vector to indicate "use default pathfinding"
	return std::vector<int>();
}

//=============================================================================
// CGravityManager Implementation
//=============================================================================

CGravityManager& CGravityManager::instance()
{
	if (s_instance == nullptr)
		s_instance = new CGravityManager();
	return *s_instance;
}

CGravityManager::CGravityManager()
	: m_pZoneManager(new CGravityZoneManager())
	, m_lastGravityValue(FallDamage::DEFAULT_GRAVITY)
	, m_lastUpdateTime(0.0f)
{
}

CGravityManager::~CGravityManager()
{
	delete m_pZoneManager;
	m_pZoneManager = nullptr;
}

CGravityZoneManager& CGravityManager::getZoneManager()
{
	return *m_pZoneManager;
}

const CGravityZoneManager& CGravityManager::getZoneManager() const
{
	return *m_pZoneManager;
}

void CGravityManager::update()
{
	float curTime = gpGlobals->curtime;

	// Update periodically (every 1 second)
	if (curTime - m_lastUpdateTime < 1.0f)
		return;

	m_lastUpdateTime = curTime;
	m_lastGravityValue = m_gravityInfo.getGravity();

	m_gravityInfo.update();
	m_pathAnalyzer.updateForGravity();
}

bool CGravityManager::hasGravityChanged() const
{
	float currentGravity = m_gravityInfo.getGravity();
	float ratio = currentGravity / m_lastGravityValue;
	return ratio < 0.95f || ratio > 1.05f;
}

bool CGravityManager::shouldBotBeCareful(CBot* pBot, int fromWpt, int toWpt) const
{
	const CWaypointFallInfo* pInfo = m_pathAnalyzer.getFallInfo(fromWpt, toWpt);
	if (pInfo == nullptr)
		return false;

	// Bot should be careful if fall would cause significant damage
	if (pInfo->estimatedDamage > 10.0f)
		return true;

	// Extra careful if low on health
	if (pBot != nullptr && pInfo->estimatedDamage > 0.0f)
	{
		float health = pBot->getHealthPercent() * 100.0f;
		if (pInfo->estimatedDamage >= health * 0.25f)
			return true; // Would lose 25%+ of remaining health
	}

	return false;
}

float CGravityManager::getModifiedPathCost(int fromWpt, int toWpt, float baseCost) const
{
	float modifier = m_pathAnalyzer.getPathCost(fromWpt, toWpt);
	return baseCost * modifier;
}

void CGravityManager::onBotFallDamage(CBot* pBot, float damage, const Vector& location)
{
	// Report to nav-test system if active
	CNavTestManager& navTest = CNavTestManager::instance();
	if (navTest.isSessionActive())
	{
		navTest.onBotFallDamage(pBot, damage);
	}

	// Log the fall damage event for debugging
	if (damage >= 25.0f)
	{
		CBotGlobals::botMessage(nullptr, 0, "Bot took %.0f fall damage at gravity %.0f",
			damage, m_gravityInfo.getGravity());
	}
}

int CGravityManager::getDangerousConnectionCount() const
{
	int count = 0;
	for (const CWaypointFallInfo& info : m_pathAnalyzer.getDangerousConnections(10.0f))
	{
		count++;
	}
	return count;
}

int CGravityManager::getFatalConnectionCount() const
{
	int count = 0;
	for (const CWaypointFallInfo& info : m_pathAnalyzer.getDangerousConnections(100.0f))
	{
		count++;
	}
	return count;
}

void CGravityManager::refresh()
{
	// Update gravity info
	m_gravityInfo.update();
	m_lastGravityValue = m_gravityInfo.getGravity();

	// Scan for gravity zones
	m_pZoneManager->scanForGravityZones();
	m_pZoneManager->linkWaypointsToZones();

	// Re-analyze all connections
	m_pathAnalyzer.analyzeAllConnections();

	CBotGlobals::botMessage(nullptr, 0, "Gravity data refreshed: %d zones, gravity = %.0f",
		m_pZoneManager->getZoneCount(), m_gravityInfo.getGravity());
}

//=============================================================================
// CGravityZoneManager Implementation
//=============================================================================

CGravityZoneManager::CGravityZoneManager()
	: m_lastScanTime(0.0f)
{
}

void CGravityZoneManager::scanForGravityZones()
{
	m_zones.clear();

	// Iterate through all entities looking for trigger_gravity
	int maxEnts = gpGlobals->maxEntities;
	for (int i = 0; i < maxEnts; i++)
	{
		edict_t* pEdict = INDEXENT(i);
		if (pEdict == nullptr || pEdict->IsFree())
			continue;

		const char* classname = pEdict->GetClassName();
		if (classname == nullptr)
			continue;

		if (strcmp(classname, "trigger_gravity") == 0)
		{
			CGravityZone zone;
			zone.pEntity = pEdict;

			// Get the entity's bounding box
			ICollideable* pCollideable = pEdict->GetCollideable();
			if (pCollideable != nullptr)
			{
				const Vector& origin = pCollideable->GetCollisionOrigin();
				zone.mins = origin + pCollideable->OBBMins();
				zone.maxs = origin + pCollideable->OBBMaxs();
			}
			else
			{
				// Fallback: use edict origin with default bounds
				// Can't use CBaseEntity::GetAbsOrigin() as CBaseEntity is incomplete
				zone.mins = Vector(-64, -64, -64);
				zone.maxs = Vector(64, 64, 64);
			}

			// Get the gravity value from the entity
			// trigger_gravity stores gravity as a float multiplier
			float gravityMultiplier = 1.0f;

			// Try to read the gravity keyvalue
			// In Source, trigger_gravity has a "gravity" key (0.0-1.0 or higher)
			// We'll try to get it from the entity datadesc
			IServerEntity* pServerEnt = pEdict->GetIServerEntity();
			if (pServerEnt != nullptr)
			{
				CBaseEntity* pBaseEnt = pServerEnt->GetBaseEntity();
				if (pBaseEnt != nullptr)
				{
					// Try getting gravity via serverclass/datadesc
					// Fallback: use a default of 0.5 (low gravity) if we can't read it
					// In practice, maps often use values like 0.1, 0.5, 2.0, etc.
					gravityMultiplier = 0.5f; // Assume low gravity zone
				}
			}

			zone.gravity = gravityMultiplier;
			zone.absoluteGravity = gravityMultiplier * FallDamage::DEFAULT_GRAVITY;

			m_zones.push_back(zone);
		}
	}

	m_lastScanTime = gpGlobals->curtime;

	if (!m_zones.empty())
	{
		CBotGlobals::botMessage(nullptr, 0, "Found %d trigger_gravity zones",
			static_cast<int>(m_zones.size()));
	}
}

void CGravityZoneManager::update()
{
	// Periodically re-scan (every 30 seconds)
	if (gpGlobals->curtime - m_lastScanTime > 30.0f)
	{
		scanForGravityZones();
		linkWaypointsToZones();
	}
}

bool CGravityZoneManager::isInGravityZone(const Vector& point) const
{
	return getZoneAtPoint(point) != nullptr;
}

float CGravityZoneManager::getGravityAtPoint(const Vector& point) const
{
	const CGravityZone* pZone = getZoneAtPoint(point);
	if (pZone != nullptr)
		return pZone->absoluteGravity;

	// Return server gravity if not in a zone
	if (sv_gravity.IsValid())
		return sv_gravity.GetFloat();

	return FallDamage::DEFAULT_GRAVITY;
}

const CGravityZone* CGravityZoneManager::getZoneAtPoint(const Vector& point) const
{
	for (const CGravityZone& zone : m_zones)
	{
		if (zone.containsPoint(point))
			return &zone;
	}
	return nullptr;
}

std::vector<int> CGravityZoneManager::getAffectedWaypoints() const
{
	std::vector<int> affected;

	for (const CGravityZone& zone : m_zones)
	{
		for (int wptId : zone.waypointsInZone)
		{
			// Avoid duplicates
			bool found = false;
			for (int existing : affected)
			{
				if (existing == wptId)
				{
					found = true;
					break;
				}
			}
			if (!found)
				affected.push_back(wptId);
		}
	}

	return affected;
}

void CGravityZoneManager::linkWaypointsToZones()
{
	// Clear existing links
	for (CGravityZone& zone : m_zones)
	{
		zone.waypointsInZone.clear();
	}

	// Check each waypoint
	int numWaypoints = CWaypoints::numWaypoints();
	for (int i = 0; i < numWaypoints; i++)
	{
		CWaypoint* pWpt = CWaypoints::getWaypoint(i);
		if (pWpt == nullptr || !pWpt->isUsed())
			continue;

		Vector origin = pWpt->getOrigin();

		// Check if this waypoint is in any zone
		for (CGravityZone& zone : m_zones)
		{
			if (zone.containsPoint(origin))
			{
				zone.waypointsInZone.push_back(i);
				break; // Waypoint can only be in one zone
			}
		}
	}

	// Log results
	int totalAffected = 0;
	for (const CGravityZone& zone : m_zones)
	{
		totalAffected += static_cast<int>(zone.waypointsInZone.size());
	}

	if (totalAffected > 0)
	{
		CBotGlobals::botMessage(nullptr, 0, "%d waypoints in gravity zones", totalAffected);
	}
}

//=============================================================================
// Gravity Refresh Command
//=============================================================================
void Gravity_Refresh_Command(const CCommand& args)
{
	CGravityManager::instance().refresh();
}
