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

#include "bot_tactical.h"
#include "bot_navtest.h"
#include "bot.h"
#include "bot_globals.h"
#include "bot_waypoint.h"
#include "bot_waypoint_locations.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>

// Static instance
CTacticalDataManager* CTacticalDataManager::s_instance = nullptr;

//=============================================================================
// CTacticalInfo Implementation
//=============================================================================

CTacticalInfo::CTacticalInfo()
	: m_waypointId(-1)
	, m_tacticalFlags(0)
	, m_dangerRating(0.0f)
	, m_coverQuality(0.5f)
	, m_heightAdvantage(0.0f)
	, m_deathCount(0)
	, m_killCount(0)
	, m_visibleEnemySpots(0)
	, m_lastCombatTime(0.0f)
	, m_lastVisitTime(0.0f)
{
}

CTacticalInfo::CTacticalInfo(int waypointId)
	: CTacticalInfo()
{
	m_waypointId = waypointId;
}

void CTacticalInfo::reset()
{
	m_tacticalFlags = 0;
	m_dangerRating = 0.0f;
	m_coverQuality = 0.5f;
	m_heightAdvantage = 0.0f;
	m_deathCount = 0;
	m_killCount = 0;
	m_visibleEnemySpots = 0;
	m_lastCombatTime = 0.0f;
	m_lastVisitTime = 0.0f;
	m_weights = CTacticalWeight();
}

void CTacticalInfo::adjustDanger(float delta)
{
	m_dangerRating = std::clamp(m_dangerRating + delta, 0.0f, 1.0f);
}

float CTacticalInfo::getWeightForPlaystyle(EBotPlaystyle style) const
{
	return m_weights.getWeight(style);
}

float CTacticalInfo::calculateTacticalValue(CBot* pBot) const
{
	float value = 1.0f;

	// Base value from playstyle weights
	EBotPlaystyle style = GetBotPlaystyle(pBot);
	value *= m_weights.getWeight(style);

	// Adjust based on danger rating (lower danger = higher value for defensive bots)
	if (style == EBotPlaystyle::DEFENSIVE || style == EBotPlaystyle::CAMPER)
	{
		value *= (1.0f - m_dangerRating * 0.5f);
	}
	else if (style == EBotPlaystyle::AGGRESSIVE || style == EBotPlaystyle::RUSHER)
	{
		// Aggressive bots might actually prefer dangerous areas for action
		value *= (1.0f + m_dangerRating * 0.2f);
	}

	// Cover quality bonus for defensive playstyles
	if (style == EBotPlaystyle::DEFENSIVE || style == EBotPlaystyle::SNIPER)
	{
		value *= (1.0f + m_coverQuality * 0.3f);
	}

	// Height advantage for snipers
	if (style == EBotPlaystyle::SNIPER && m_heightAdvantage > 0.0f)
	{
		value *= (1.0f + m_heightAdvantage / 200.0f);
	}

	return value;
}

//=============================================================================
// CTacticalAnalyzer Implementation
//=============================================================================

CTacticalAnalyzer::CTacticalAnalyzer()
{
}

void CTacticalAnalyzer::traceMultiDirection(const Vector& origin, std::vector<float>& distances)
{
	distances.clear();

	// Trace in 8 horizontal directions
	static const float angles[] = { 0.0f, 45.0f, 90.0f, 135.0f, 180.0f, 225.0f, 270.0f, 315.0f };

	for (float angle : angles)
	{
		float rad = angle * M_PI / 180.0f;
		Vector dir(cosf(rad), sinf(rad), 0.0f);
		Vector end = origin + dir * 500.0f;

		float fraction = CBotGlobals::quickTraceline(nullptr, origin, end);
		distances.push_back(fraction * 500.0f);
	}
}

void CTacticalAnalyzer::analyzeWaypoint(CWaypoint* pWaypoint, CTacticalInfo* pInfo)
{
	if (pWaypoint == nullptr || pInfo == nullptr)
		return;

	Vector origin = pWaypoint->getOrigin();
	pInfo->setWaypointId(CWaypoints::getWaypointIndex(pWaypoint));

	// Analyze cover quality
	float coverQuality = analyzeCoverQuality(origin);
	pInfo->setCoverQuality(coverQuality);

	// Set cover flags based on quality
	if (coverQuality > 0.7f)
		pInfo->addTacticalFlag(TacticalFlags::COVER_FULL);
	else if (coverQuality > 0.4f)
		pInfo->addTacticalFlag(TacticalFlags::COVER_PARTIAL);

	// Analyze height advantage
	float heightAdv = analyzeHeightAdvantage(origin);
	pInfo->setHeightAdvantage(heightAdv);

	if (heightAdv > 100.0f)
		pInfo->addTacticalFlag(TacticalFlags::COVER_HIGH);
	else if (heightAdv < -50.0f)
		pInfo->addTacticalFlag(TacticalFlags::COVER_LOW);

	// Check for good sightlines (sniper spots)
	int visibleCount = 0;
	if (hasGoodSightlines(origin, &visibleCount))
	{
		pInfo->addTacticalFlag(TacticalFlags::SNIPER_SPOT);
		pInfo->setVisibleEnemySpots(visibleCount);
	}

	// Check for chokepoints
	if (isChokepoint(origin))
	{
		pInfo->addTacticalFlag(TacticalFlags::CHOKE_POINT);
		pInfo->addTacticalFlag(TacticalFlags::AMBUSH_POINT);
	}

	// Check path width (narrow = chokepoint, wide = open)
	float pathWidth = calculatePathWidth(origin);
	if (pathWidth > 300.0f)
		pInfo->addTacticalFlag(TacticalFlags::OPEN_AREA);

	// Adjust weights based on analysis
	CTacticalWeight& weights = pInfo->getWeights();

	// Sniper spots favor sniper playstyle
	if (pInfo->hasTacticalFlag(TacticalFlags::SNIPER_SPOT))
	{
		weights.sniper = 1.5f;
		weights.defensive = 1.2f;
	}

	// Cover positions favor defensive play
	if (coverQuality > 0.5f)
	{
		weights.defensive = 1.0f + coverQuality * 0.5f;
		weights.aggressive = 0.8f;
	}

	// Open areas favor aggressive rushers
	if (pInfo->hasTacticalFlag(TacticalFlags::OPEN_AREA))
	{
		weights.aggressive = 1.2f;
		weights.defensive = 0.7f;
		weights.sniper = 0.6f;
	}

	// Chokepoints good for ambushes
	if (pInfo->hasTacticalFlag(TacticalFlags::CHOKE_POINT))
	{
		weights.defensive = 1.3f;
		weights.flanker = 0.5f; // Flankers avoid chokepoints
	}

	// Check existing waypoint flags for context
	int wptFlags = pWaypoint->getFlags();

	if (wptFlags & CWaypointTypes::W_FL_HEALTH)
		pInfo->addTacticalFlag(TacticalFlags::HEALTH_NEARBY);

	if (wptFlags & CWaypointTypes::W_FL_AMMO)
		pInfo->addTacticalFlag(TacticalFlags::AMMO_NEARBY);

	if (wptFlags & CWaypointTypes::W_FL_FALL)
		pInfo->addTacticalFlag(TacticalFlags::FALL_HAZARD);
}

float CTacticalAnalyzer::analyzeCoverQuality(const Vector& origin)
{
	std::vector<float> distances;
	traceMultiDirection(origin, distances);

	if (distances.empty())
		return 0.5f;

	// Count directions with nearby obstacles (provides cover)
	int coverCount = 0;
	for (float dist : distances)
	{
		if (dist < 150.0f)
			coverCount++;
	}

	// Cover quality based on how many directions have cover
	return static_cast<float>(coverCount) / static_cast<float>(distances.size());
}

float CTacticalAnalyzer::analyzeHeightAdvantage(const Vector& origin)
{
	// Trace down to find ground, compare with nearby positions
	trace_t tr;
	CBotGlobals::quickTraceline(nullptr, origin, origin - Vector(0, 0, 500));
	tr = *CBotGlobals::getTraceResult();

	float groundHeight = origin.z - (tr.fraction * 500.0f);

	// Sample surrounding ground heights
	float avgSurroundingHeight = 0.0f;
	int samples = 0;

	static const float sampleDist = 200.0f;
	static const float angles[] = { 0.0f, 90.0f, 180.0f, 270.0f };

	for (float angle : angles)
	{
		float rad = angle * M_PI / 180.0f;
		Vector samplePos = origin + Vector(cosf(rad) * sampleDist, sinf(rad) * sampleDist, 0.0f);

		CBotGlobals::quickTraceline(nullptr, samplePos, samplePos - Vector(0, 0, 500));
		tr = *CBotGlobals::getTraceResult();

		avgSurroundingHeight += samplePos.z - (tr.fraction * 500.0f);
		samples++;
	}

	if (samples > 0)
		avgSurroundingHeight /= static_cast<float>(samples);

	return groundHeight - avgSurroundingHeight;
}

bool CTacticalAnalyzer::hasGoodSightlines(const Vector& origin, int* pVisibleCount)
{
	int visibleWaypoints = 0;
	int longRangeVisible = 0;

	// Check visibility to other waypoints
	for (int i = 0; i < CWaypoints::numWaypoints(); i++)
	{
		CWaypoint* pWpt = CWaypoints::getWaypoint(i);
		if (pWpt == nullptr || !pWpt->isUsed())
			continue;

		Vector wptOrigin = pWpt->getOrigin();
		float dist = (wptOrigin - origin).Length();

		// Only check waypoints at reasonable sniping distance
		if (dist < 300.0f || dist > 2000.0f)
			continue;

		// Check visibility
		if (CBotGlobals::isVisible(origin, wptOrigin))
		{
			visibleWaypoints++;
			if (dist > 500.0f)
				longRangeVisible++;
		}
	}

	if (pVisibleCount != nullptr)
		*pVisibleCount = visibleWaypoints;

	// Good sightlines = many long-range visible positions
	return longRangeVisible >= 5;
}

bool CTacticalAnalyzer::isChokepoint(const Vector& origin)
{
	float width = calculatePathWidth(origin);
	return width < 150.0f;
}

float CTacticalAnalyzer::calculatePathWidth(const Vector& origin)
{
	std::vector<float> distances;
	traceMultiDirection(origin, distances);

	if (distances.size() < 2)
		return 500.0f;

	// Find minimum opposing direction distance
	float minWidth = FLT_MAX;

	for (size_t i = 0; i < distances.size() / 2; i++)
	{
		float width = distances[i] + distances[i + distances.size() / 2];
		if (width < minWidth)
			minWidth = width;
	}

	return minWidth;
}

//=============================================================================
// CTacticalDataManager Implementation
//=============================================================================

CTacticalDataManager& CTacticalDataManager::instance()
{
	if (s_instance == nullptr)
		s_instance = new CTacticalDataManager();
	return *s_instance;
}

CTacticalDataManager::CTacticalDataManager()
	: m_lastUpdateTime(0.0f)
{
}

CTacticalDataManager::~CTacticalDataManager()
{
}

void CTacticalDataManager::init(int numWaypoints)
{
	reset();
	m_tacticalData.resize(numWaypoints);

	for (int i = 0; i < numWaypoints; i++)
	{
		m_tacticalData[i].setWaypointId(i);
	}
}

void CTacticalDataManager::reset()
{
	m_tacticalData.clear();
	m_lastUpdateTime = 0.0f;
}

CTacticalInfo* CTacticalDataManager::getTacticalInfo(int waypointId)
{
	if (waypointId < 0 || waypointId >= static_cast<int>(m_tacticalData.size()))
		return nullptr;
	return &m_tacticalData[waypointId];
}

const CTacticalInfo* CTacticalDataManager::getTacticalInfo(int waypointId) const
{
	if (waypointId < 0 || waypointId >= static_cast<int>(m_tacticalData.size()))
		return nullptr;
	return &m_tacticalData[waypointId];
}

void CTacticalDataManager::analyzeAllWaypoints()
{
	int numWaypoints = CWaypoints::numWaypoints();
	init(numWaypoints);

	for (int i = 0; i < numWaypoints; i++)
	{
		analyzeWaypoint(i);
	}

	// Scan for health/armor chargers and update HEALTH_NEARBY, ARMOR_NEARBY flags
	scanForChargers();

	CBotGlobals::botMessage(nullptr, 0, "Tactical analysis complete: %d waypoints analyzed", numWaypoints);
}

void CTacticalDataManager::analyzeWaypoint(int waypointId)
{
	CWaypoint* pWpt = CWaypoints::getWaypoint(waypointId);
	if (pWpt == nullptr || !pWpt->isUsed())
		return;

	CTacticalInfo* pInfo = getTacticalInfo(waypointId);
	if (pInfo == nullptr)
		return;

	m_analyzer.analyzeWaypoint(pWpt, pInfo);
}

void CTacticalDataManager::onCombatEvent(const Vector& location, bool wasKill, bool wasDeath)
{
	// Find nearest waypoint
	int nearestWpt = CWaypointLocations::NearestWaypoint(location, 200.0f, -1, true, false, false, nullptr);

	if (nearestWpt < 0)
		return;

	CTacticalInfo* pInfo = getTacticalInfo(nearestWpt);
	if (pInfo == nullptr)
		return;

	if (wasKill)
		pInfo->recordKill();

	if (wasDeath)
		pInfo->recordDeath();

	pInfo->setLastCombatTime(gpGlobals->curtime);
}

int CTacticalDataManager::findBestTacticalWaypoint(CBot* pBot, EBotPlaystyle style, const Vector& nearOrigin, float maxDist)
{
	int bestWpt = -1;
	float bestValue = -1.0f;
	float maxDistSq = maxDist * maxDist;

	for (size_t i = 0; i < m_tacticalData.size(); i++)
	{
		CWaypoint* pWpt = CWaypoints::getWaypoint(static_cast<int>(i));
		if (pWpt == nullptr || !pWpt->isUsed())
			continue;

		// Check distance
		float distSq = (pWpt->getOrigin() - nearOrigin).LengthSqr();
		if (distSq > maxDistSq)
			continue;

		// Calculate tactical value
		float value = m_tacticalData[i].calculateTacticalValue(pBot);

		// Distance penalty
		value *= (1.0f - (distSq / maxDistSq) * 0.3f);

		if (value > bestValue)
		{
			bestValue = value;
			bestWpt = static_cast<int>(i);
		}
	}

	return bestWpt;
}

std::vector<int> CTacticalDataManager::findWaypointsWithFlags(uint32_t requiredFlags, uint32_t excludeFlags)
{
	std::vector<int> result;

	for (size_t i = 0; i < m_tacticalData.size(); i++)
	{
		const CTacticalInfo& info = m_tacticalData[i];
		uint32_t flags = info.getTacticalFlags();

		if ((flags & requiredFlags) == requiredFlags && (flags & excludeFlags) == 0)
		{
			result.push_back(static_cast<int>(i));
		}
	}

	return result;
}

int CTacticalDataManager::findCoverWaypoint(const Vector& origin, const Vector& threatDirection, float maxDist)
{
	int bestWpt = -1;
	float bestScore = -1.0f;
	float maxDistSq = maxDist * maxDist;

	Vector threatDir = threatDirection;
	threatDir.NormalizeInPlace();

	for (size_t i = 0; i < m_tacticalData.size(); i++)
	{
		const CTacticalInfo& info = m_tacticalData[i];

		// Must have some cover
		if (info.getCoverQuality() < 0.3f)
			continue;

		CWaypoint* pWpt = CWaypoints::getWaypoint(static_cast<int>(i));
		if (pWpt == nullptr || !pWpt->isUsed())
			continue;

		Vector wptPos = pWpt->getOrigin();
		float distSq = (wptPos - origin).LengthSqr();
		if (distSq > maxDistSq)
			continue;

		// Prefer cover that's perpendicular or behind relative to threat
		Vector toWpt = wptPos - origin;
		toWpt.NormalizeInPlace();
		float dot = toWpt.Dot(threatDir);

		// Score: cover quality * distance factor * threat angle factor
		float score = info.getCoverQuality();
		score *= (1.0f - sqrtf(distSq) / maxDist * 0.5f);
		score *= (1.0f - dot); // Prefer positions perpendicular or opposite to threat

		if (score > bestScore)
		{
			bestScore = score;
			bestWpt = static_cast<int>(i);
		}
	}

	return bestWpt;
}

std::vector<int> CTacticalDataManager::findFlankingWaypoints(const Vector& origin, const Vector& targetPos, float maxDist)
{
	std::vector<int> result;
	float maxDistSq = maxDist * maxDist;

	Vector toTarget = targetPos - origin;
	toTarget.z = 0;
	toTarget.NormalizeInPlace();

	for (size_t i = 0; i < m_tacticalData.size(); i++)
	{
		CWaypoint* pWpt = CWaypoints::getWaypoint(static_cast<int>(i));
		if (pWpt == nullptr || !pWpt->isUsed())
			continue;

		Vector wptPos = pWpt->getOrigin();

		// Check distance from both origin and target
		float distFromOriginSq = (wptPos - origin).LengthSqr();
		float distFromTargetSq = (wptPos - targetPos).LengthSqr();

		if (distFromOriginSq > maxDistSq || distFromTargetSq > maxDistSq * 1.5f)
			continue;

		// Check if it's a flanking position (perpendicular to main attack axis)
		Vector toWpt = wptPos - origin;
		toWpt.z = 0;
		toWpt.NormalizeInPlace();

		float dot = fabsf(toWpt.Dot(toTarget));

		// Good flanking position if angle is 45-135 degrees from main axis
		if (dot < 0.7f && dot > 0.1f)
		{
			result.push_back(static_cast<int>(i));
		}
	}

	return result;
}

void CTacticalDataManager::updateDangerDecay(float deltaTime)
{
	// Decay danger ratings over time
	const float decayRate = 0.01f; // Decay 1% per second

	for (CTacticalInfo& info : m_tacticalData)
	{
		float danger = info.getDangerRating();
		if (danger > 0.0f)
		{
			danger -= decayRate * deltaTime;
			if (danger < 0.0f)
				danger = 0.0f;
			info.setDangerRating(danger);
		}
	}
}

void CTacticalDataManager::updateTrafficFromNavTest()
{
	CNavTestManager& navTest = CNavTestManager::instance();
	if (!navTest.isSessionActive())
		return;

	CMapCoverageTracker& coverage = navTest.getCoverageTracker();

	// Calculate average visit count for threshold determination
	int totalVisits = 0;
	int visitedWaypoints = 0;

	for (size_t i = 0; i < m_tacticalData.size(); i++)
	{
		int visits = coverage.getVisitCount(static_cast<int>(i));
		if (visits > 0)
		{
			totalVisits += visits;
			visitedWaypoints++;
		}
	}

	// Need at least some data to work with
	if (visitedWaypoints < 5)
		return;

	float avgVisits = static_cast<float>(totalVisits) / static_cast<float>(visitedWaypoints);

	// Waypoints with visits > 2x average are considered high traffic
	float highTrafficThreshold = avgVisits * 2.0f;

	int flagsSet = 0;

	for (size_t i = 0; i < m_tacticalData.size(); i++)
	{
		int visits = coverage.getVisitCount(static_cast<int>(i));

		if (static_cast<float>(visits) >= highTrafficThreshold)
		{
			m_tacticalData[i].addTacticalFlag(TacticalFlags::HIGH_TRAFFIC);
			flagsSet++;
		}
		else
		{
			// Clear flag if visits dropped below threshold
			m_tacticalData[i].removeTacticalFlag(TacticalFlags::HIGH_TRAFFIC);
		}
	}

	if (flagsSet > 0)
	{
		CBotGlobals::botMessage(nullptr, 0, "Nav-test: Updated HIGH_TRAFFIC flag on %d waypoints (threshold: %.1f visits)",
			flagsSet, highTrafficThreshold);
	}
}

void CTacticalDataManager::updateDangerFromNavTest()
{
	CNavTestManager& navTest = CNavTestManager::instance();
	CNavTestIssueTracker& issues = navTest.getIssueTracker();

	if (issues.getIssueCount() == 0)
		return;

	int dangerUpdates = 0;

	for (size_t i = 0; i < m_tacticalData.size(); i++)
	{
		int wptId = static_cast<int>(i);
		int issueFreq = issues.getIssueFrequency(wptId);

		if (issueFreq == 0)
			continue;

		CTacticalInfo& info = m_tacticalData[i];

		// Increase danger rating based on issue frequency
		// More issues = more dangerous for navigation
		float dangerIncrease = 0.0f;

		// Get specific issues for this waypoint for detailed analysis
		std::vector<CNavTestIssue> wptIssues = issues.getIssuesForWaypoint(wptId);

		for (const CNavTestIssue& issue : wptIssues)
		{
			switch (issue.severity)
			{
			case ENavTestIssueSeverity::CRITICAL:
				dangerIncrease += 0.4f;
				break;
			case ENavTestIssueSeverity::HIGH:
				dangerIncrease += 0.25f;
				break;
			case ENavTestIssueSeverity::MEDIUM:
				dangerIncrease += 0.1f;
				break;
			case ENavTestIssueSeverity::LOW:
				dangerIncrease += 0.05f;
				break;
			}

			// Set specific flags based on issue type
			switch (issue.type)
			{
			case ENavTestIssueType::FALL_DAMAGE:
				info.addTacticalFlag(TacticalFlags::FALL_HAZARD);
				break;
			case ENavTestIssueType::STUCK:
			case ENavTestIssueType::PATH_FAILURE:
				info.addTacticalFlag(TacticalFlags::DANGER_ZONE);
				break;
			default:
				break;
			}

			// Adjust playstyle weights based on issues
			// Cautious playstyles should avoid problematic waypoints
			CTacticalWeight& weights = info.getWeights();

			if (issue.severity >= ENavTestIssueSeverity::HIGH)
			{
				// Reduce weights for cautious playstyles
				weights.defensive *= 0.8f;
				weights.sniper *= 0.9f;

				// Aggressive bots might still use risky paths
				weights.aggressive *= 0.95f;
			}
		}

		// Cap danger increase at 1.0
		float currentDanger = info.getDangerRating();
		float newDanger = currentDanger + dangerIncrease;
		if (newDanger > 1.0f)
			newDanger = 1.0f;

		if (newDanger > currentDanger)
		{
			info.setDangerRating(newDanger);
			dangerUpdates++;
		}
	}

	if (dangerUpdates > 0)
	{
		CBotGlobals::botMessage(nullptr, 0, "Nav-test: Updated danger ratings on %d waypoints based on %d issues",
			dangerUpdates, issues.getIssueCount());
	}
}

void CTacticalDataManager::scanForChargers()
{
	// Scan for health and suit charger entities in the map
	// Set HEALTH_NEARBY flag on waypoints near these entities

	if (m_tacticalData.empty())
		return;

	const float CHARGER_RADIUS = 256.0f;  // Distance to flag waypoints as near health
	int flagsSet = 0;

	// Iterate through all entities looking for chargers
	for (int i = gpGlobals->maxClients + 1; i < MAX_ENTITIES; i++)
	{
		edict_t* pEdict = engine->PEntityOfEntIndex(i);
		if (pEdict == nullptr || pEdict->IsFree())
			continue;

		if (!CBotGlobals::entityIsValid(pEdict))
			continue;

		const char* szClassName = pEdict->GetClassName();
		if (szClassName == nullptr)
			continue;

		bool isHealthCharger = false;
		bool isArmorCharger = false;

		// Check for health chargers (HL2/HL2DM)
		if (std::strcmp(szClassName, "item_healthcharger") == 0 ||
			std::strcmp(szClassName, "func_healthcharger") == 0 ||
			std::strcmp(szClassName, "item_healthkit") == 0 ||
			std::strcmp(szClassName, "item_healthvial") == 0)
		{
			isHealthCharger = true;
		}
		// Check for suit/armor chargers (HL2/HL2DM)
		else if (std::strcmp(szClassName, "item_suitcharger") == 0 ||
				 std::strcmp(szClassName, "item_battery") == 0 ||
				 std::strcmp(szClassName, "item_suit") == 0)
		{
			isArmorCharger = true;
		}
		// Check for TF2 health packs
		else if (std::strncmp(szClassName, "item_healthkit", 14) == 0 ||
				 std::strcmp(szClassName, "func_regenerate") == 0)
		{
			isHealthCharger = true;
		}
		// Check for TF2 ammo packs
		else if (std::strncmp(szClassName, "item_ammopack", 13) == 0)
		{
			// Flag for ammo instead
		}

		if (!isHealthCharger && !isArmorCharger)
			continue;

		Vector chargerOrigin = CBotGlobals::entityOrigin(pEdict);

		// Find waypoints within radius and set appropriate flags
		for (size_t w = 0; w < m_tacticalData.size(); w++)
		{
			CWaypoint* pWpt = CWaypoints::getWaypoint(static_cast<int>(w));
			if (pWpt == nullptr || !pWpt->isUsed())
				continue;

			float dist = (pWpt->getOrigin() - chargerOrigin).Length();
			if (dist <= CHARGER_RADIUS)
			{
				if (isHealthCharger)
				{
					m_tacticalData[w].addTacticalFlag(TacticalFlags::HEALTH_NEARBY);
					flagsSet++;
				}
				else if (isArmorCharger)
				{
					m_tacticalData[w].addTacticalFlag(TacticalFlags::ARMOR_NEARBY);
					flagsSet++;
				}
			}
		}
	}

	if (flagsSet > 0)
	{
		CBotGlobals::botMessage(nullptr, 0, "Tactical: Set HEALTH/ARMOR_NEARBY flags on %d waypoints from charger scan", flagsSet);
	}
}

bool CTacticalDataManager::saveData(const char* mapName)
{
	if (mapName == nullptr || m_tacticalData.empty())
		return false;

	char filename[512];
	CBotGlobals::buildFileName(filename, "waypoints", mapName, "tac", true);

	std::ofstream file(filename, std::ios::binary);
	if (!file.is_open())
	{
		CBotGlobals::botMessage(nullptr, 0, "Failed to save tactical data: %s", filename);
		return false;
	}

	// Write header
	const uint32_t TACTICAL_FILE_VERSION = 1;
	file.write(reinterpret_cast<const char*>(&TACTICAL_FILE_VERSION), sizeof(uint32_t));

	uint32_t numWaypoints = static_cast<uint32_t>(m_tacticalData.size());
	file.write(reinterpret_cast<const char*>(&numWaypoints), sizeof(uint32_t));

	// Write tactical data for each waypoint
	for (const CTacticalInfo& info : m_tacticalData)
	{
		int32_t wptId = info.getWaypointId();
		uint32_t flags = info.getTacticalFlags();
		float danger = info.getDangerRating();
		float cover = info.getCoverQuality();
		float height = info.getHeightAdvantage();
		int32_t deaths = info.getDeathCount();
		int32_t kills = info.getKillCount();

		file.write(reinterpret_cast<const char*>(&wptId), sizeof(int32_t));
		file.write(reinterpret_cast<const char*>(&flags), sizeof(uint32_t));
		file.write(reinterpret_cast<const char*>(&danger), sizeof(float));
		file.write(reinterpret_cast<const char*>(&cover), sizeof(float));
		file.write(reinterpret_cast<const char*>(&height), sizeof(float));
		file.write(reinterpret_cast<const char*>(&deaths), sizeof(int32_t));
		file.write(reinterpret_cast<const char*>(&kills), sizeof(int32_t));

		// Write weights
		const CTacticalWeight& weights = info.getWeights();
		file.write(reinterpret_cast<const char*>(&weights.balanced), sizeof(float));
		file.write(reinterpret_cast<const char*>(&weights.aggressive), sizeof(float));
		file.write(reinterpret_cast<const char*>(&weights.defensive), sizeof(float));
		file.write(reinterpret_cast<const char*>(&weights.sniper), sizeof(float));
		file.write(reinterpret_cast<const char*>(&weights.flanker), sizeof(float));
	}

	file.close();
	CBotGlobals::botMessage(nullptr, 0, "Saved tactical data for %d waypoints: %s", numWaypoints, filename);
	return true;
}

bool CTacticalDataManager::loadData(const char* mapName)
{
	if (mapName == nullptr)
		return false;

	char filename[512];
	CBotGlobals::buildFileName(filename, "waypoints", mapName, "tac", true);

	std::ifstream file(filename, std::ios::binary);
	if (!file.is_open())
	{
		// No tactical data file - use defaults (backward compatibility)
		return false;
	}

	// Read header
	uint32_t version;
	file.read(reinterpret_cast<char*>(&version), sizeof(uint32_t));

	if (version != 1)
	{
		CBotGlobals::botMessage(nullptr, 0, "Unsupported tactical data version: %d", version);
		file.close();
		return false;
	}

	uint32_t numWaypoints;
	file.read(reinterpret_cast<char*>(&numWaypoints), sizeof(uint32_t));

	// Initialize if needed
	if (m_tacticalData.size() != numWaypoints)
	{
		init(numWaypoints);
	}

	// Read tactical data
	for (uint32_t i = 0; i < numWaypoints && i < m_tacticalData.size(); i++)
	{
		int32_t wptId;
		uint32_t flags;
		float danger, cover, height;
		int32_t deaths, kills;

		file.read(reinterpret_cast<char*>(&wptId), sizeof(int32_t));
		file.read(reinterpret_cast<char*>(&flags), sizeof(uint32_t));
		file.read(reinterpret_cast<char*>(&danger), sizeof(float));
		file.read(reinterpret_cast<char*>(&cover), sizeof(float));
		file.read(reinterpret_cast<char*>(&height), sizeof(float));
		file.read(reinterpret_cast<char*>(&deaths), sizeof(int32_t));
		file.read(reinterpret_cast<char*>(&kills), sizeof(int32_t));

		CTacticalInfo& info = m_tacticalData[i];
		info.setWaypointId(wptId);
		info.setTacticalFlags(flags);
		info.setDangerRating(danger);
		info.setCoverQuality(cover);
		info.setHeightAdvantage(height);

		// Read weights
		CTacticalWeight& weights = info.getWeights();
		file.read(reinterpret_cast<char*>(&weights.balanced), sizeof(float));
		file.read(reinterpret_cast<char*>(&weights.aggressive), sizeof(float));
		file.read(reinterpret_cast<char*>(&weights.defensive), sizeof(float));
		file.read(reinterpret_cast<char*>(&weights.sniper), sizeof(float));
		file.read(reinterpret_cast<char*>(&weights.flanker), sizeof(float));
	}

	file.close();
	CBotGlobals::botMessage(nullptr, 0, "Loaded tactical data for %d waypoints", numWaypoints);
	return true;
}

//=============================================================================
// Helper Functions
//=============================================================================

EBotPlaystyle GetBotPlaystyle(CBot* pBot)
{
	// TODO: Read from bot profile or determine from behavior
	// For now, return balanced
	return EBotPlaystyle::BALANCED;
}

const char* GetPlaystyleName(EBotPlaystyle style)
{
	switch (style)
	{
	case EBotPlaystyle::BALANCED:   return "Balanced";
	case EBotPlaystyle::AGGRESSIVE: return "Aggressive";
	case EBotPlaystyle::DEFENSIVE:  return "Defensive";
	case EBotPlaystyle::SUPPORT:    return "Support";
	case EBotPlaystyle::SNIPER:     return "Sniper";
	case EBotPlaystyle::FLANKER:    return "Flanker";
	case EBotPlaystyle::CAMPER:     return "Camper";
	case EBotPlaystyle::RUSHER:     return "Rusher";
	default:                        return "Unknown";
	}
}

const char* GetTacticalFlagName(uint32_t flag)
{
	if (flag & TacticalFlags::COVER_FULL)       return "Cover_Full";
	if (flag & TacticalFlags::COVER_PARTIAL)    return "Cover_Partial";
	if (flag & TacticalFlags::COVER_HIGH)       return "Cover_High";
	if (flag & TacticalFlags::COVER_LOW)        return "Cover_Low";
	if (flag & TacticalFlags::SNIPER_SPOT)      return "Sniper_Spot";
	if (flag & TacticalFlags::AMBUSH_POINT)     return "Ambush_Point";
	if (flag & TacticalFlags::CHOKE_POINT)      return "Choke_Point";
	if (flag & TacticalFlags::OPEN_AREA)        return "Open_Area";
	if (flag & TacticalFlags::HIGH_TRAFFIC)     return "High_Traffic";
	if (flag & TacticalFlags::DANGER_ZONE)      return "Danger_Zone";
	if (flag & TacticalFlags::CAMPING_SPOT)     return "Camping_Spot";
	if (flag & TacticalFlags::HEALTH_NEARBY)    return "Health_Nearby";
	if (flag & TacticalFlags::AMMO_NEARBY)      return "Ammo_Nearby";
	if (flag & TacticalFlags::ARMOR_NEARBY)     return "Armor_Nearby";
	if (flag & TacticalFlags::WEAPON_NEARBY)    return "Weapon_Nearby";
	if (flag & TacticalFlags::FALL_HAZARD)      return "Fall_Hazard";
	if (flag & TacticalFlags::FLANK_ROUTE)      return "Flank_Route";
	return "Unknown";
}

uint32_t ParseTacticalFlag(const char* flagName)
{
	if (strcasecmp(flagName, "cover_full") == 0)       return TacticalFlags::COVER_FULL;
	if (strcasecmp(flagName, "cover_partial") == 0)    return TacticalFlags::COVER_PARTIAL;
	if (strcasecmp(flagName, "cover_high") == 0)       return TacticalFlags::COVER_HIGH;
	if (strcasecmp(flagName, "cover_low") == 0)        return TacticalFlags::COVER_LOW;
	if (strcasecmp(flagName, "sniper_spot") == 0)      return TacticalFlags::SNIPER_SPOT;
	if (strcasecmp(flagName, "ambush_point") == 0)     return TacticalFlags::AMBUSH_POINT;
	if (strcasecmp(flagName, "choke_point") == 0)      return TacticalFlags::CHOKE_POINT;
	if (strcasecmp(flagName, "open_area") == 0)        return TacticalFlags::OPEN_AREA;
	if (strcasecmp(flagName, "high_traffic") == 0)     return TacticalFlags::HIGH_TRAFFIC;
	if (strcasecmp(flagName, "danger_zone") == 0)      return TacticalFlags::DANGER_ZONE;
	if (strcasecmp(flagName, "camping_spot") == 0)     return TacticalFlags::CAMPING_SPOT;
	if (strcasecmp(flagName, "health_nearby") == 0)    return TacticalFlags::HEALTH_NEARBY;
	if (strcasecmp(flagName, "ammo_nearby") == 0)      return TacticalFlags::AMMO_NEARBY;
	if (strcasecmp(flagName, "armor_nearby") == 0)     return TacticalFlags::ARMOR_NEARBY;
	if (strcasecmp(flagName, "weapon_nearby") == 0)    return TacticalFlags::WEAPON_NEARBY;
	if (strcasecmp(flagName, "fall_hazard") == 0)      return TacticalFlags::FALL_HAZARD;
	if (strcasecmp(flagName, "flank_route") == 0)      return TacticalFlags::FLANK_ROUTE;
	return 0;
}

EBotPlaystyle ParsePlaystyle(const char* styleName)
{
	if (strcasecmp(styleName, "balanced") == 0)    return EBotPlaystyle::BALANCED;
	if (strcasecmp(styleName, "aggressive") == 0)  return EBotPlaystyle::AGGRESSIVE;
	if (strcasecmp(styleName, "defensive") == 0)   return EBotPlaystyle::DEFENSIVE;
	if (strcasecmp(styleName, "support") == 0)     return EBotPlaystyle::SUPPORT;
	if (strcasecmp(styleName, "sniper") == 0)      return EBotPlaystyle::SNIPER;
	if (strcasecmp(styleName, "flanker") == 0)     return EBotPlaystyle::FLANKER;
	if (strcasecmp(styleName, "camper") == 0)      return EBotPlaystyle::CAMPER;
	if (strcasecmp(styleName, "rusher") == 0)      return EBotPlaystyle::RUSHER;
	return EBotPlaystyle::BALANCED;
}

//=============================================================================
// CTacticalAdvisor Implementation
//=============================================================================

CTacticalAdvisor::CTacticalAdvisor()
	: m_pBot(nullptr)
	, m_currentStyle(EBotPlaystyle::BALANCED)
	, m_bEnabled(true)
	, m_bNeedsHealth(false)
	, m_bNeedsAmmo(false)
	, m_bUnderThreat(false)
	, m_bHasAdvantage(false)
	, m_healthPercent(1.0f)
	, m_ammoPercent(1.0f)
	, m_nearbyEnemies(0)
	, m_nearbyAllies(0)
	, m_lastSituationUpdate(0.0f)
	, m_lastStyleSwitch(0.0f)
	, m_deathStreak(0)
	, m_killStreak(0)
{
}

void CTacticalAdvisor::updateSituation()
{
	if (m_pBot == nullptr)
		return;

	float curTime = gpGlobals->curtime;
	if (curTime - m_lastSituationUpdate < 0.5f)
		return;

	m_lastSituationUpdate = curTime;

	// Update health status
	float maxHealth = m_pBot->getMaxHealth();
	float curHealth = m_pBot->getHealthPercent() * maxHealth;
	m_healthPercent = maxHealth > 0 ? curHealth / maxHealth : 1.0f;
	m_bNeedsHealth = m_healthPercent < 0.5f;

	// Ammo check would need weapon-specific logic
	m_bNeedsAmmo = false;  // Simplified

	// Check for nearby enemies (simplified)
	m_nearbyEnemies = 0;
	m_nearbyAllies = 0;

	edict_t* pEnemy = m_pBot->getEnemy();
	if (pEnemy != nullptr)
	{
		m_nearbyEnemies = 1;
		m_bUnderThreat = true;
	}
	else
	{
		m_bUnderThreat = false;
	}

	// Advantage is having cover and/or height
	m_bHasAdvantage = false;
	Vector origin = m_pBot->getOrigin();
	int nearWpt = CWaypointLocations::NearestWaypoint(origin, 200.0f, -1, true, false, false, nullptr);
	if (nearWpt >= 0)
	{
		const CTacticalInfo* pInfo = CTacticalDataManager::instance().getTacticalInfo(nearWpt);
		if (pInfo != nullptr)
		{
			m_bHasAdvantage = pInfo->getCoverQuality() > 0.5f || pInfo->getHeightAdvantage() > 50.0f;
		}
	}
}

float CTacticalAdvisor::scoreWaypoint(int waypointId) const
{
	const CTacticalInfo* pInfo = CTacticalDataManager::instance().getTacticalInfo(waypointId);
	if (pInfo == nullptr)
		return 0.5f;

	float score = 1.0f;

	// Apply playstyle modifier
	switch (m_currentStyle)
	{
	case EBotPlaystyle::AGGRESSIVE:
	case EBotPlaystyle::RUSHER:
		score *= getAggressiveModifier(*pInfo);
		break;
	case EBotPlaystyle::DEFENSIVE:
	case EBotPlaystyle::CAMPER:
		score *= getDefensiveModifier(*pInfo);
		break;
	case EBotPlaystyle::SNIPER:
		if (pInfo->hasTacticalFlag(TacticalFlags::SNIPER_SPOT))
			score *= 1.5f;
		if (pInfo->hasTacticalFlag(TacticalFlags::COVER_HIGH))
			score *= 1.3f;
		break;
	case EBotPlaystyle::FLANKER:
		if (pInfo->hasTacticalFlag(TacticalFlags::FLANK_ROUTE))
			score *= 1.4f;
		if (pInfo->hasTacticalFlag(TacticalFlags::CHOKE_POINT))
			score *= 0.6f;  // Avoid chokepoints
		break;
	default:
		score *= pInfo->getWeights().balanced;
		break;
	}

	// Apply needs-based modifiers
	score *= scoreWaypointForNeeds(waypointId);

	return score;
}

float CTacticalAdvisor::scoreWaypointForNeeds(int waypointId) const
{
	const CTacticalInfo* pInfo = CTacticalDataManager::instance().getTacticalInfo(waypointId);
	if (pInfo == nullptr)
		return 1.0f;

	float modifier = 1.0f;

	// Health need
	if (m_bNeedsHealth && pInfo->hasTacticalFlag(TacticalFlags::HEALTH_NEARBY))
	{
		modifier *= 1.5f + (1.0f - m_healthPercent);  // More urgent = higher modifier
	}

	// Ammo need
	if (m_bNeedsAmmo && pInfo->hasTacticalFlag(TacticalFlags::AMMO_NEARBY))
	{
		modifier *= 1.3f;
	}

	// Under threat - prefer cover
	if (m_bUnderThreat)
	{
		if (pInfo->getCoverQuality() > 0.5f)
			modifier *= 1.4f;
		if (pInfo->hasTacticalFlag(TacticalFlags::OPEN_AREA))
			modifier *= 0.6f;
	}

	return modifier;
}

int CTacticalAdvisor::recommendDestination(const Vector& nearOrigin, float maxDist)
{
	updateSituation();

	int bestWpt = -1;
	float bestScore = -1.0f;
	float maxDistSq = maxDist * maxDist;

	int numWaypoints = CWaypoints::numWaypoints();
	for (int i = 0; i < numWaypoints; i++)
	{
		CWaypoint* pWpt = CWaypoints::getWaypoint(i);
		if (pWpt == nullptr || !pWpt->isUsed())
			continue;

		float distSq = (pWpt->getOrigin() - nearOrigin).LengthSqr();
		if (distSq > maxDistSq)
			continue;

		float score = scoreWaypoint(i);

		// Distance penalty
		float distFactor = 1.0f - (sqrtf(distSq) / maxDist) * 0.3f;
		score *= distFactor;

		if (score > bestScore)
		{
			bestScore = score;
			bestWpt = i;
		}
	}

	return bestWpt;
}

int CTacticalAdvisor::recommendCoverPosition(const Vector& threatDir)
{
	if (m_pBot == nullptr)
		return -1;

	return CTacticalDataManager::instance().findCoverWaypoint(
		m_pBot->getOrigin(), threatDir, 500.0f);
}

int CTacticalAdvisor::recommendAmbushPosition(const Vector& targetArea, float radius)
{
	std::vector<int> ambushWpts = CTacticalDataManager::instance().findWaypointsWithFlags(
		TacticalFlags::AMBUSH_POINT, 0);

	if (ambushWpts.empty())
		return -1;

	// Find closest to target area
	int bestWpt = -1;
	float bestDist = FLT_MAX;

	for (int wptId : ambushWpts)
	{
		CWaypoint* pWpt = CWaypoints::getWaypoint(wptId);
		if (pWpt == nullptr)
			continue;

		float dist = (pWpt->getOrigin() - targetArea).Length();
		if (dist < radius && dist < bestDist)
		{
			bestDist = dist;
			bestWpt = wptId;
		}
	}

	return bestWpt;
}

int CTacticalAdvisor::recommendFlankingRoute(const Vector& targetPos)
{
	if (m_pBot == nullptr)
		return -1;

	std::vector<int> flankWpts = CTacticalDataManager::instance().findFlankingWaypoints(
		m_pBot->getOrigin(), targetPos, 800.0f);

	if (flankWpts.empty())
		return -1;

	// Return first viable flanking waypoint
	return flankWpts[0];
}

std::vector<int> CTacticalAdvisor::findWaypointsByFlag(uint32_t flags, float maxDist) const
{
	std::vector<int> result;

	if (m_pBot == nullptr)
		return result;

	Vector origin = m_pBot->getOrigin();
	float maxDistSq = maxDist * maxDist;

	std::vector<int> flagWpts = CTacticalDataManager::instance().findWaypointsWithFlags(flags, 0);

	for (int wptId : flagWpts)
	{
		CWaypoint* pWpt = CWaypoints::getWaypoint(wptId);
		if (pWpt == nullptr)
			continue;

		float distSq = (pWpt->getOrigin() - origin).LengthSqr();
		if (distSq <= maxDistSq)
		{
			result.push_back(wptId);
		}
	}

	return result;
}

std::vector<int> CTacticalAdvisor::findHealthWaypoints(float maxDist) const
{
	return findWaypointsByFlag(TacticalFlags::HEALTH_NEARBY, maxDist);
}

std::vector<int> CTacticalAdvisor::findAmmoWaypoints(float maxDist) const
{
	return findWaypointsByFlag(TacticalFlags::AMMO_NEARBY, maxDist);
}

std::vector<int> CTacticalAdvisor::findCoverWaypoints(float maxDist) const
{
	return findWaypointsByFlag(TacticalFlags::COVER_FULL | TacticalFlags::COVER_PARTIAL, maxDist);
}

float CTacticalAdvisor::getAggressiveModifier(const CTacticalInfo& info) const
{
	float mod = info.getWeights().aggressive;

	// Aggressive bots prefer high traffic, open areas, near weapons
	if (info.hasTacticalFlag(TacticalFlags::HIGH_TRAFFIC))
		mod *= 1.3f;
	if (info.hasTacticalFlag(TacticalFlags::OPEN_AREA))
		mod *= 1.1f;
	if (info.hasTacticalFlag(TacticalFlags::WEAPON_NEARBY))
		mod *= 1.2f;

	// Avoid defensive positions
	if (info.hasTacticalFlag(TacticalFlags::CAMPING_SPOT))
		mod *= 0.7f;

	return mod;
}

float CTacticalAdvisor::getDefensiveModifier(const CTacticalInfo& info) const
{
	float mod = info.getWeights().defensive;

	// Defensive bots prefer cover, camping spots, health nearby
	if (info.hasTacticalFlag(TacticalFlags::CAMPING_SPOT))
		mod *= 1.4f;
	if (info.getCoverQuality() > 0.5f)
		mod *= 1.0f + info.getCoverQuality() * 0.5f;
	if (info.hasTacticalFlag(TacticalFlags::HEALTH_NEARBY))
		mod *= 1.2f;

	// Avoid open/dangerous areas
	if (info.hasTacticalFlag(TacticalFlags::OPEN_AREA))
		mod *= 0.6f;
	if (info.hasTacticalFlag(TacticalFlags::DANGER_ZONE))
		mod *= 0.5f;

	return mod;
}

float CTacticalAdvisor::getObjectiveModifier(const CTacticalInfo& info) const
{
	float mod = 1.0f;

	// Objective-focused bots prioritize paths toward objectives
	if (info.hasTacticalFlag(TacticalFlags::OBJECTIVE_NEARBY))
		mod *= 1.5f;

	// Ignore combat opportunities
	if (info.hasTacticalFlag(TacticalFlags::AMBUSH_POINT))
		mod *= 0.9f;

	return mod;
}

void CTacticalAdvisor::evaluatePlaystyleSwitch()
{
	float curTime = gpGlobals->curtime;

	// Don't switch too frequently
	if (curTime - m_lastStyleSwitch < 30.0f)
		return;

	EBotPlaystyle suggested = suggestPlaystyle();
	if (suggested != m_currentStyle)
	{
		m_currentStyle = suggested;
		m_lastStyleSwitch = curTime;

		if (CTacticalModeManager::instance().isDebugMode())
		{
			CBotGlobals::botMessage(nullptr, 0, "Bot switched to %s playstyle", GetPlaystyleName(m_currentStyle));
		}
	}
}

bool CTacticalAdvisor::shouldSwitchPlaystyle() const
{
	// Switch based on health, score, or situation
	if (m_healthPercent < 0.3f && m_currentStyle == EBotPlaystyle::AGGRESSIVE)
		return true;

	if (m_deathStreak >= 3 && m_currentStyle != EBotPlaystyle::DEFENSIVE)
		return true;

	if (m_killStreak >= 5 && m_currentStyle == EBotPlaystyle::DEFENSIVE)
		return true;

	return false;
}

EBotPlaystyle CTacticalAdvisor::suggestPlaystyle() const
{
	// Low health -> defensive
	if (m_healthPercent < 0.3f)
		return EBotPlaystyle::DEFENSIVE;

	// Death streak -> be more careful
	if (m_deathStreak >= 3)
		return EBotPlaystyle::DEFENSIVE;

	// Kill streak -> push advantage
	if (m_killStreak >= 3)
		return EBotPlaystyle::AGGRESSIVE;

	// Under threat with low health -> find cover
	if (m_bUnderThreat && m_healthPercent < 0.5f)
		return EBotPlaystyle::DEFENSIVE;

	// Has advantage -> push
	if (m_bHasAdvantage && m_healthPercent > 0.7f)
		return EBotPlaystyle::AGGRESSIVE;

	return m_currentStyle;  // Keep current
}

void CTacticalAdvisor::printDebugInfo() const
{
	if (m_pBot == nullptr)
		return;

	CBotGlobals::botMessage(nullptr, 0, "=== Tactical Advisor Debug ===");
	CBotGlobals::botMessage(nullptr, 0, "Playstyle: %s", GetPlaystyleName(m_currentStyle));
	CBotGlobals::botMessage(nullptr, 0, "Enabled: %s", m_bEnabled ? "Yes" : "No");
	CBotGlobals::botMessage(nullptr, 0, "Health: %.0f%%", m_healthPercent * 100.0f);
	CBotGlobals::botMessage(nullptr, 0, "Needs Health: %s", m_bNeedsHealth ? "Yes" : "No");
	CBotGlobals::botMessage(nullptr, 0, "Under Threat: %s", m_bUnderThreat ? "Yes" : "No");
	CBotGlobals::botMessage(nullptr, 0, "Has Advantage: %s", m_bHasAdvantage ? "Yes" : "No");
	CBotGlobals::botMessage(nullptr, 0, "Kill Streak: %d, Death Streak: %d", m_killStreak, m_deathStreak);
}

//=============================================================================
// CTacticalModeManager Implementation
//=============================================================================

CTacticalModeManager* CTacticalModeManager::s_instance = nullptr;

CTacticalModeManager& CTacticalModeManager::instance()
{
	if (s_instance == nullptr)
		s_instance = new CTacticalModeManager();
	return *s_instance;
}

CTacticalModeManager::CTacticalModeManager()
	: m_bGlobalEnabled(false)
	, m_bDebugMode(false)
	, m_defaultStyle(EBotPlaystyle::BALANCED)
{
}

CTacticalModeManager::~CTacticalModeManager()
{
	for (CTacticalAdvisor* advisor : m_advisors)
	{
		delete advisor;
	}
	m_advisors.clear();
}

CTacticalAdvisor* CTacticalModeManager::getAdvisor(CBot* pBot)
{
	if (pBot == nullptr)
		return nullptr;

	// Find existing advisor
	for (CTacticalAdvisor* advisor : m_advisors)
	{
		if (advisor->getBot() == pBot)
			return advisor;
	}

	// Create new advisor
	CTacticalAdvisor* newAdvisor = new CTacticalAdvisor();
	newAdvisor->setBot(pBot);
	newAdvisor->setPlaystyle(m_defaultStyle);
	newAdvisor->setEnabled(m_bGlobalEnabled);
	m_advisors.push_back(newAdvisor);

	return newAdvisor;
}

void CTacticalModeManager::update()
{
	if (!m_bGlobalEnabled)
		return;

	for (CTacticalAdvisor* advisor : m_advisors)
	{
		if (advisor->isEnabled() && advisor->getBot() != nullptr)
		{
			advisor->updateSituation();
			advisor->evaluatePlaystyleSwitch();
		}
	}
}

//=============================================================================
// Console Command Handlers
//=============================================================================

void Tactical_Flag_Command(const CCommand& args)
{
	// Usage: rcbot tactical flag <add|remove|list> [flag_name]
	if (args.ArgC() < 2)
	{
		CBotGlobals::botMessage(nullptr, 0, "Usage: rcbot tactical flag <add|remove|list> [flag_name]");
		return;
	}

	// Need to get nearest waypoint from player
	// Simplified - would need client context
	CBotGlobals::botMessage(nullptr, 0, "Tactical flag command - use waypoint editor for now");
}

void Tactical_Weight_Command(const CCommand& args)
{
	CBotGlobals::botMessage(nullptr, 0, "Usage: rcbot tactical weight <playstyle> <value>");
	CBotGlobals::botMessage(nullptr, 0, "Playstyles: balanced, aggressive, defensive, sniper, flanker");
}

void Tactical_Scan_Command(const CCommand& args)
{
	CBotGlobals::botMessage(nullptr, 0, "Running tactical scan on all waypoints...");
	CTacticalDataManager::instance().analyzeAllWaypoints();
}

void Tactical_Show_Command(const CCommand& args)
{
	// Show tactical info for nearest waypoint
	// Would need client context for proper implementation
	CBotGlobals::botMessage(nullptr, 0, "Use rcbot tactical debug to see tactical reasoning");
}

void Tactical_Enable_Command(const CCommand& args)
{
	bool enable = true;
	if (args.ArgC() >= 2)
	{
		enable = atoi(args.Arg(1)) != 0;
	}

	CTacticalModeManager::instance().setGlobalEnabled(enable);
	CBotGlobals::botMessage(nullptr, 0, "Tactical mode %s", enable ? "enabled" : "disabled");
}

void Tactical_Playstyle_Command(const CCommand& args)
{
	if (args.ArgC() < 2)
	{
		CBotGlobals::botMessage(nullptr, 0, "Usage: rcbot tactical playstyle <style>");
		CBotGlobals::botMessage(nullptr, 0, "Styles: balanced, aggressive, defensive, support, sniper, flanker, camper, rusher");
		CBotGlobals::botMessage(nullptr, 0, "Current default: %s", GetPlaystyleName(CTacticalModeManager::instance().getDefaultPlaystyle()));
		return;
	}

	EBotPlaystyle style = ParsePlaystyle(args.Arg(1));
	CTacticalModeManager::instance().setDefaultPlaystyle(style);
	CBotGlobals::botMessage(nullptr, 0, "Default playstyle set to: %s", GetPlaystyleName(style));
}

void Tactical_Debug_Command(const CCommand& args)
{
	bool enable = !CTacticalModeManager::instance().isDebugMode();
	if (args.ArgC() >= 2)
	{
		enable = atoi(args.Arg(1)) != 0;
	}

	CTacticalModeManager::instance().setDebugMode(enable);
	CBotGlobals::botMessage(nullptr, 0, "Tactical debug mode %s", enable ? "enabled" : "disabled");
}
