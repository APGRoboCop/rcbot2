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

bool CTacticalDataManager::saveData(const char* mapName)
{
	// TODO: Implement file saving
	return false;
}

bool CTacticalDataManager::loadData(const char* mapName)
{
	// TODO: Implement file loading
	return false;
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
	return "Unknown";
}
