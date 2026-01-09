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

#include "bot_waypoint_autorefine.h"
#include "bot_waypoint.h"
#include "bot_waypoint_locations.h"
#include "bot_globals.h"
#include "bot_genclass.h"

#include <algorithm>
#include <cmath>
#include <ctime>
#include <cstdio>

// Static instance
CWaypointAutoRefiner* CWaypointAutoRefiner::s_instance = nullptr;

//=============================================================================
// Helper: Check if waypoint has a connection to another waypoint
//=============================================================================
static bool WaypointHasConnection(int fromWpt, int toWpt)
{
	CWaypoint* pFrom = CWaypoints::getWaypoint(fromWpt);
	if (pFrom == nullptr)
		return false;

	for (int i = 0; i < pFrom->numPaths(); i++)
	{
		if (pFrom->getPath(i) == toWpt)
			return true;
	}
	return false;
}

//=============================================================================
// CIssueCluster Implementation
//=============================================================================

ENavTestIssueType CIssueCluster::getDominantIssueType() const
{
	std::map<ENavTestIssueType, int> typeCounts;

	for (const auto& issue : issues)
	{
		typeCounts[issue.type]++;
	}

	ENavTestIssueType dominant = ENavTestIssueType::NONE;
	int maxCount = 0;

	for (const auto& pair : typeCounts)
	{
		if (pair.second > maxCount)
		{
			maxCount = pair.second;
			dominant = pair.first;
		}
	}

	return dominant;
}

int CIssueCluster::getIssueCount(ENavTestIssueType type) const
{
	int count = 0;
	for (const auto& issue : issues)
	{
		if (issue.type == type)
			count++;
	}
	return count;
}

//=============================================================================
// CWaypointAutoRefiner Implementation
//=============================================================================

CWaypointAutoRefiner& CWaypointAutoRefiner::instance()
{
	if (s_instance == nullptr)
	{
		s_instance = new CWaypointAutoRefiner();
	}
	return *s_instance;
}

CWaypointAutoRefiner::CWaypointAutoRefiner()
	: m_bRefining(false)
	, m_currentIteration(0)
	, m_totalWaypointsAdded(0)
	, m_totalWaypointsRemoved(0)
	, m_totalConnectionsAdded(0)
	, m_totalConnectionsRemoved(0)
{
}

CWaypointAutoRefiner::~CWaypointAutoRefiner()
{
}

void CWaypointAutoRefiner::resetState()
{
	m_bRefining = false;
	m_currentIteration = 0;
	m_lastAnalysis.clear();
	m_iterationHistory.clear();
	m_totalWaypointsAdded = 0;
	m_totalWaypointsRemoved = 0;
	m_totalConnectionsAdded = 0;
	m_totalConnectionsRemoved = 0;
}

//=============================================================================
// Main Operations
//=============================================================================

bool CWaypointAutoRefiner::startRefinement(const CRefineOptions& options)
{
	if (m_bRefining)
	{
		CBotGlobals::botMessage(nullptr, 0, "Auto-refinement already in progress");
		return false;
	}

	m_options = options;
	resetState();
	m_bRefining = true;

	CBotGlobals::botMessage(nullptr, 0, "Starting waypoint auto-refinement (max %d iterations)",
		m_options.maxIterations);

	// Save initial snapshot for rollback
	saveSnapshot();

	// Run iterations
	while (m_bRefining && m_currentIteration < m_options.maxIterations)
	{
		if (!runIteration())
		{
			break;
		}

		if (shouldStop())
		{
			CBotGlobals::botMessage(nullptr, 0, "Stopping: %s",
				hasImproved() ? "improvement threshold reached" : "no further improvement");
			break;
		}
	}

	m_bRefining = false;

	CBotGlobals::botMessage(nullptr, 0, "Auto-refinement complete: %d iterations, %d waypoints added, %d removed",
		m_currentIteration, m_totalWaypointsAdded, m_totalWaypointsRemoved);

	return true;
}

void CWaypointAutoRefiner::stopRefinement()
{
	if (m_bRefining)
	{
		CBotGlobals::botMessage(nullptr, 0, "Stopping auto-refinement...");
		m_bRefining = false;
	}
}

//=============================================================================
// Analysis
//=============================================================================

CAnalysisResult CWaypointAutoRefiner::analyze()
{
	CAnalysisResult result;

	CNavTestManager& navTest = CNavTestManager::instance();
	CNavTestIssueTracker& issueTracker = navTest.getIssueTracker();

	result.totalIssues = issueTracker.getIssueCount();
	result.criticalIssues = issueTracker.getIssueCountBySeverity(ENavTestIssueSeverity::CRITICAL);

	// Cluster issues
	result.clusters = clusterIssues(m_options.clusterRadius);
	result.clusterCount = static_cast<int>(result.clusters.size());

	// Detect bad waypoints
	result.badWaypoints = detectBadWaypoints();

	// Detect bad connections
	result.badConnections = detectBadConnections();

	// Generate suggestions
	result.suggestions = generateSuggestions(result);

	// Calculate overall health score
	int numWaypoints = CWaypoints::numWaypoints();
	if (numWaypoints > 0)
	{
		float issueRatio = static_cast<float>(result.totalIssues) / static_cast<float>(numWaypoints);
		result.overallHealthScore = 1.0f - std::min(1.0f, issueRatio * 0.1f);

		// Penalize for critical issues
		result.overallHealthScore -= static_cast<float>(result.criticalIssues) * 0.05f;
		if (result.overallHealthScore < 0.0f)
			result.overallHealthScore = 0.0f;
	}

	m_lastAnalysis = result;
	return result;
}

void CWaypointAutoRefiner::printAnalysis(const CAnalysisResult& result, bool verbose)
{
	CBotGlobals::botMessage(nullptr, 0, "=== Waypoint Analysis ===");
	CBotGlobals::botMessage(nullptr, 0, "Total Issues: %d (Critical: %d)",
		result.totalIssues, result.criticalIssues);
	CBotGlobals::botMessage(nullptr, 0, "Issue Clusters: %d", result.clusterCount);
	CBotGlobals::botMessage(nullptr, 0, "Bad Waypoints: %d", static_cast<int>(result.badWaypoints.size()));
	CBotGlobals::botMessage(nullptr, 0, "Bad Connections: %d", static_cast<int>(result.badConnections.size()));
	CBotGlobals::botMessage(nullptr, 0, "Suggestions: %d", static_cast<int>(result.suggestions.size()));
	CBotGlobals::botMessage(nullptr, 0, "Health Score: %.1f%%", result.overallHealthScore * 100.0f);

	if (verbose)
	{
		CBotGlobals::botMessage(nullptr, 0, "--- Issue Clusters ---");
		for (size_t i = 0; i < result.clusters.size() && i < 10; i++)
		{
			const CIssueCluster& cluster = result.clusters[i];
			CBotGlobals::botMessage(nullptr, 0, "  Cluster %d: %d issues, severity %.2f at (%.0f, %.0f, %.0f)",
				static_cast<int>(i + 1),
				static_cast<int>(cluster.issues.size()),
				cluster.severityScore,
				cluster.centroid.x, cluster.centroid.y, cluster.centroid.z);
		}

		CBotGlobals::botMessage(nullptr, 0, "--- Bad Waypoints ---");
		for (size_t i = 0; i < result.badWaypoints.size() && i < 10; i++)
		{
			const CBadWaypointInfo& bad = result.badWaypoints[i];
			CBotGlobals::botMessage(nullptr, 0, "  Waypoint %d: score %.2f (stuck: %d, fail: %d)",
				bad.waypointId, bad.issueScore, bad.stuckCount, bad.pathFailureCount);
		}

		CBotGlobals::botMessage(nullptr, 0, "--- Suggestions ---");
		for (size_t i = 0; i < result.suggestions.size() && i < 10; i++)
		{
			const CWaypointSuggestion& sug = result.suggestions[i];
			const char* typeStr = "Unknown";
			switch (sug.type)
			{
			case CWaypointSuggestion::ESuggestionType::ADD_WAYPOINT:
				typeStr = "Add waypoint";
				break;
			case CWaypointSuggestion::ESuggestionType::REMOVE_WAYPOINT:
				typeStr = "Remove waypoint";
				break;
			case CWaypointSuggestion::ESuggestionType::ADD_CONNECTION:
				typeStr = "Add connection";
				break;
			case CWaypointSuggestion::ESuggestionType::REMOVE_CONNECTION:
				typeStr = "Remove connection";
				break;
			default:
				break;
			}
			CBotGlobals::botMessage(nullptr, 0, "  %s (confidence: %.0f%%): %s",
				typeStr, sug.confidence * 100.0f, sug.reason.c_str());
		}
	}
}

//=============================================================================
// Issue Clustering
//=============================================================================

std::vector<CIssueCluster> CWaypointAutoRefiner::clusterIssues(float radius)
{
	std::vector<CIssueCluster> clusters;

	CNavTestManager& navTest = CNavTestManager::instance();
	CNavTestIssueTracker& issueTracker = navTest.getIssueTracker();
	const std::vector<CNavTestIssue>& allIssues = issueTracker.getAllIssues();

	if (allIssues.empty())
		return clusters;

	// Simple clustering: assign each issue to nearest cluster or create new one
	std::vector<bool> assigned(allIssues.size(), false);

	for (size_t i = 0; i < allIssues.size(); i++)
	{
		if (assigned[i])
			continue;

		// Start new cluster with this issue
		CIssueCluster cluster;
		cluster.issues.push_back(allIssues[i]);
		cluster.centroid = allIssues[i].location;
		assigned[i] = true;

		// Find all issues within radius
		for (size_t j = i + 1; j < allIssues.size(); j++)
		{
			if (assigned[j])
				continue;

			float dist = (allIssues[j].location - cluster.centroid).Length();
			if (dist <= radius)
			{
				cluster.issues.push_back(allIssues[j]);
				assigned[j] = true;

				// Update centroid
				Vector sum(0, 0, 0);
				for (const auto& issue : cluster.issues)
				{
					sum = sum + issue.location;
				}
				cluster.centroid = sum * (1.0f / static_cast<float>(cluster.issues.size()));
			}
		}

		// Calculate cluster properties
		cluster.severityScore = calculateClusterSeverity(cluster);

		// Calculate radius
		float maxDist = 0.0f;
		for (const auto& issue : cluster.issues)
		{
			float dist = (issue.location - cluster.centroid).Length();
			if (dist > maxDist)
				maxDist = dist;
		}
		cluster.radius = maxDist;

		// Find primary waypoint
		cluster.primaryWaypointId = CWaypointLocations::NearestWaypoint(
			cluster.centroid, 500.0f, -1, true, false, false, nullptr);

		// Find all affected waypoints
		cluster.affectedWaypoints = findNearbyWaypoints(cluster.centroid, cluster.radius + 100.0f);

		clusters.push_back(cluster);
	}

	// Sort by severity (highest first)
	std::sort(clusters.begin(), clusters.end(),
		[](const CIssueCluster& a, const CIssueCluster& b) {
			return a.severityScore > b.severityScore;
		});

	return clusters;
}

//=============================================================================
// Severity Scoring
//=============================================================================

float CWaypointAutoRefiner::calculateSeverityScore(const CNavTestIssue& issue) const
{
	float score = 0.0f;

	// Base score from severity level
	switch (issue.severity)
	{
	case ENavTestIssueSeverity::CRITICAL:
		score = 10.0f;
		break;
	case ENavTestIssueSeverity::HIGH:
		score = 5.0f;
		break;
	case ENavTestIssueSeverity::MEDIUM:
		score = 2.0f;
		break;
	case ENavTestIssueSeverity::LOW:
		score = 1.0f;
		break;
	}

	// Multiply by occurrence count
	score *= static_cast<float>(issue.occurrenceCount);

	// Type-specific modifiers
	switch (issue.type)
	{
	case ENavTestIssueType::STUCK:
		score *= 1.5f;  // Stuck is very disruptive
		break;
	case ENavTestIssueType::UNREACHABLE:
		score *= 2.0f;  // Unreachable waypoints are critical
		break;
	case ENavTestIssueType::FALL_DAMAGE:
		score *= 1.2f;
		break;
	case ENavTestIssueType::CONNECTION_BROKEN:
		score *= 1.8f;
		break;
	default:
		break;
	}

	return score;
}

float CWaypointAutoRefiner::calculateClusterSeverity(const CIssueCluster& cluster) const
{
	float totalScore = 0.0f;

	for (const auto& issue : cluster.issues)
	{
		totalScore += calculateSeverityScore(issue);
	}

	// Density bonus: more issues in smaller area = worse
	if (cluster.radius > 0.0f && cluster.issues.size() > 1)
	{
		float density = static_cast<float>(cluster.issues.size()) / cluster.radius;
		totalScore *= (1.0f + density * 0.1f);
	}

	return totalScore;
}

//=============================================================================
// Gap Detection
//=============================================================================

std::vector<Vector> CWaypointAutoRefiner::detectGaps(const std::vector<CIssueCluster>& clusters)
{
	std::vector<Vector> gaps;

	for (const auto& cluster : clusters)
	{
		// Check if cluster area has stuck issues but no nearby waypoints
		int stuckCount = cluster.getIssueCount(ENavTestIssueType::STUCK);
		if (stuckCount < 2)
			continue;

		// Find nearest waypoint to cluster center
		int nearestWpt = CWaypointLocations::NearestWaypoint(
			cluster.centroid, 300.0f, -1, true, false, false, nullptr);

		if (nearestWpt < 0)
		{
			// No waypoint nearby - this is a gap
			gaps.push_back(cluster.centroid);
		}
		else
		{
			// Check if the nearest waypoint is too far
			CWaypoint* pWpt = CWaypoints::getWaypoint(nearestWpt);
			if (pWpt != nullptr)
			{
				float dist = (pWpt->getOrigin() - cluster.centroid).Length();
				if (dist > 200.0f)
				{
					// Waypoint is far - this area needs coverage
					gaps.push_back(cluster.centroid);
				}
			}
		}
	}

	return gaps;
}

//=============================================================================
// Waypoint Placement Suggestions
//=============================================================================

std::vector<CWaypointSuggestion> CWaypointAutoRefiner::generateSuggestions(const CAnalysisResult& analysis)
{
	std::vector<CWaypointSuggestion> suggestions;

	// Suggestion 1: Add waypoints for gaps
	std::vector<Vector> gaps = detectGaps(analysis.clusters);
	for (const auto& gap : gaps)
	{
		if (!isLocationValid(gap))
			continue;

		CWaypointSuggestion sug;
		sug.type = CWaypointSuggestion::ESuggestionType::ADD_WAYPOINT;
		sug.position = gap;
		sug.confidence = 0.8f;
		sug.reason = "Gap detected: stuck issues with no nearby waypoint";
		suggestions.push_back(sug);
	}

	// Suggestion 2: Remove bad waypoints
	for (const auto& bad : analysis.badWaypoints)
	{
		if (bad.shouldRemove && m_options.allowWaypointRemoval)
		{
			CWaypointSuggestion sug;
			sug.type = CWaypointSuggestion::ESuggestionType::REMOVE_WAYPOINT;
			sug.waypointId = bad.waypointId;
			sug.confidence = std::min(1.0f, bad.issueScore / 50.0f);
			sug.reason = "Consistently causes navigation issues";
			suggestions.push_back(sug);
		}
		else if (bad.shouldRelocate)
		{
			CWaypointSuggestion sug;
			sug.type = CWaypointSuggestion::ESuggestionType::RELOCATE_WAYPOINT;
			sug.waypointId = bad.waypointId;
			sug.position = bad.suggestedPosition;
			sug.confidence = 0.6f;
			sug.reason = "Would work better at different position";
			suggestions.push_back(sug);
		}
	}

	// Suggestion 3: Remove bad connections
	for (const auto& bad : analysis.badConnections)
	{
		if (bad.shouldRemove && m_options.allowConnectionRemoval)
		{
			CWaypointSuggestion sug;
			sug.type = CWaypointSuggestion::ESuggestionType::REMOVE_CONNECTION;
			sug.waypointId = bad.fromWaypointId;
			sug.targetWaypointId = bad.toWaypointId;
			sug.confidence = std::min(1.0f, bad.issueScore / 30.0f);
			sug.reason = "Connection causes navigation failures";
			suggestions.push_back(sug);
		}
	}

	// Suggestion 4: Add connections for isolated waypoints near clusters
	for (const auto& cluster : analysis.clusters)
	{
		if (cluster.getIssueCount(ENavTestIssueType::UNREACHABLE) > 0)
		{
			// Find waypoints that should connect but don't
			for (int wptId : cluster.affectedWaypoints)
			{
				CWaypoint* pWpt = CWaypoints::getWaypoint(wptId);
				if (pWpt == nullptr)
					continue;

				std::vector<int> nearby = findNearbyWaypoints(pWpt->getOrigin(), 300.0f);
				for (int nearId : nearby)
				{
					if (nearId == wptId)
						continue;

					// Check if connection exists
					if (!WaypointHasConnection(wptId, nearId))
					{
						// Check if we could add one
						CWaypoint* pNear = CWaypoints::getWaypoint(nearId);
						if (pNear && canReachFrom(pWpt->getOrigin(), pNear->getOrigin()))
						{
							CWaypointSuggestion sug;
							sug.type = CWaypointSuggestion::ESuggestionType::ADD_CONNECTION;
							sug.waypointId = wptId;
							sug.targetWaypointId = nearId;
							sug.confidence = 0.7f;
							sug.reason = "Missing connection causing unreachable waypoints";
							suggestions.push_back(sug);
						}
					}
				}
			}
		}
	}

	// Sort by confidence
	std::sort(suggestions.begin(), suggestions.end(),
		[](const CWaypointSuggestion& a, const CWaypointSuggestion& b) {
			return a.confidence > b.confidence;
		});

	// Filter by minimum confidence
	suggestions.erase(
		std::remove_if(suggestions.begin(), suggestions.end(),
			[this](const CWaypointSuggestion& s) {
				return s.confidence < m_options.minConfidence;
			}),
		suggestions.end());

	return suggestions;
}

//=============================================================================
// Waypoint Insertion
//=============================================================================

int CWaypointAutoRefiner::insertWaypoint(const Vector& position, int flags)
{
	if (!isLocationValid(position))
		return -1;

	// Create new waypoint using the edict_t overload
	int newWptId = CWaypoints::addWaypoint(nullptr, position, flags, true, 0, 0, 0.0f);

	if (newWptId >= 0)
	{
		m_totalWaypointsAdded++;

		// Try to connect to nearby waypoints
		repairConnections(newWptId);

		CBotGlobals::botMessage(nullptr, 0, "Added waypoint %d at (%.0f, %.0f, %.0f)",
			newWptId, position.x, position.y, position.z);
	}

	return newWptId;
}

bool CWaypointAutoRefiner::validateNewWaypoint(int waypointId)
{
	CWaypoint* pWpt = CWaypoints::getWaypoint(waypointId);
	if (pWpt == nullptr || !pWpt->isUsed())
		return false;

	// Check if waypoint has at least one connection
	int numPaths = pWpt->numPaths();
	if (numPaths == 0)
	{
		// Try to add connections
		int added = repairConnections(waypointId);
		if (added == 0)
		{
			CBotGlobals::botMessage(nullptr, 0, "Warning: Waypoint %d has no connections", waypointId);
			return false;
		}
	}

	return true;
}

//=============================================================================
// Connection Repair
//=============================================================================

int CWaypointAutoRefiner::repairConnections(int waypointId)
{
	CWaypoint* pWpt = CWaypoints::getWaypoint(waypointId);
	if (pWpt == nullptr)
		return 0;

	int added = 0;
	std::vector<int> nearby = findNearbyWaypoints(pWpt->getOrigin(), 400.0f);

	for (int nearId : nearby)
	{
		if (nearId == waypointId)
			continue;

		if (WaypointHasConnection(waypointId, nearId))
			continue;

		CWaypoint* pNear = CWaypoints::getWaypoint(nearId);
		if (pNear == nullptr)
			continue;

		if (canReachFrom(pWpt->getOrigin(), pNear->getOrigin()))
		{
			if (addConnection(waypointId, nearId))
			{
				added++;
			}
		}
	}

	return added;
}

bool CWaypointAutoRefiner::addConnection(int fromWpt, int toWpt)
{
	CWaypoint* pFrom = CWaypoints::getWaypoint(fromWpt);
	CWaypoint* pTo = CWaypoints::getWaypoint(toWpt);

	if (pFrom == nullptr || pTo == nullptr)
		return false;

	if (WaypointHasConnection(fromWpt, toWpt))
		return false;

	pFrom->addPathTo(toWpt);
	m_totalConnectionsAdded++;

	return true;
}

//=============================================================================
// Bad Waypoint Detection
//=============================================================================

std::vector<CBadWaypointInfo> CWaypointAutoRefiner::detectBadWaypoints()
{
	std::vector<CBadWaypointInfo> badWaypoints;

	CNavTestManager& navTest = CNavTestManager::instance();
	CNavTestIssueTracker& issueTracker = navTest.getIssueTracker();

	int numWaypoints = CWaypoints::numWaypoints();

	for (int i = 0; i < numWaypoints; i++)
	{
		CWaypoint* pWpt = CWaypoints::getWaypoint(i);
		if (pWpt == nullptr || !pWpt->isUsed())
			continue;

		std::vector<CNavTestIssue> wptIssues = issueTracker.getIssuesForWaypoint(i);
		if (wptIssues.empty())
			continue;

		CBadWaypointInfo info;
		info.waypointId = i;

		for (const auto& issue : wptIssues)
		{
			switch (issue.type)
			{
			case ENavTestIssueType::STUCK:
				info.stuckCount += issue.occurrenceCount;
				break;
			case ENavTestIssueType::PATH_FAILURE:
				info.pathFailureCount += issue.occurrenceCount;
				break;
			case ENavTestIssueType::UNREACHABLE:
				info.unreachableCount += issue.occurrenceCount;
				break;
			default:
				break;
			}

			info.issueScore += calculateSeverityScore(issue);
		}

		// Determine recommendation
		if (info.issueScore > 50.0f || info.stuckCount > 5 || info.unreachableCount > 3)
		{
			info.shouldRemove = true;
		}
		else if (info.issueScore > 20.0f)
		{
			info.shouldRelocate = true;

			// Suggest new position (move away from stuck locations)
			Vector offset(0, 0, 0);
			for (const auto& issue : wptIssues)
			{
				if (issue.type == ENavTestIssueType::STUCK)
				{
					Vector dir = pWpt->getOrigin() - issue.location;
					dir.NormalizeInPlace();
					offset = offset + dir * 50.0f;
				}
			}
			info.suggestedPosition = pWpt->getOrigin() + offset;
		}

		// Find bad connections from this waypoint
		for (int p = 0; p < pWpt->numPaths(); p++)
		{
			int pathId = pWpt->getPath(p);
			float connScore = calculateConnectionIssueScore(i, pathId);
			if (connScore > 10.0f)
			{
				info.badConnections.push_back(pathId);
			}
		}

		if (info.issueScore > 10.0f)
		{
			badWaypoints.push_back(info);
		}
	}

	// Sort by issue score
	std::sort(badWaypoints.begin(), badWaypoints.end(),
		[](const CBadWaypointInfo& a, const CBadWaypointInfo& b) {
			return a.issueScore > b.issueScore;
		});

	return badWaypoints;
}

//=============================================================================
// Waypoint Removal Logic
//=============================================================================

bool CWaypointAutoRefiner::removeWaypoint(int waypointId, bool updateConnections)
{
	CWaypoint* pWpt = CWaypoints::getWaypoint(waypointId);
	if (pWpt == nullptr || !pWpt->isUsed())
		return false;

	// Before removing, check if this would disconnect the graph
	// (simplified check: ensure all connected waypoints have other paths)
	if (updateConnections)
	{
		for (int p = 0; p < pWpt->numPaths(); p++)
		{
			int pathId = pWpt->getPath(p);
			CWaypoint* pOther = CWaypoints::getWaypoint(pathId);
			if (pOther != nullptr && pOther->numPaths() <= 1)
			{
				CBotGlobals::botMessage(nullptr, 0,
					"Cannot remove waypoint %d: would isolate waypoint %d", waypointId, pathId);
				return false;
			}
		}
	}

	CWaypoints::deleteWaypoint(waypointId);
	m_totalWaypointsRemoved++;

	CBotGlobals::botMessage(nullptr, 0, "Removed waypoint %d", waypointId);
	return true;
}

bool CWaypointAutoRefiner::relocateWaypoint(int waypointId, const Vector& newPosition)
{
	CWaypoint* pWpt = CWaypoints::getWaypoint(waypointId);
	if (pWpt == nullptr || !pWpt->isUsed())
		return false;

	if (!isLocationValid(newPosition))
		return false;

	// Save old connections
	std::vector<int> oldPaths;
	for (int p = 0; p < pWpt->numPaths(); p++)
	{
		oldPaths.push_back(pWpt->getPath(p));
	}

	// Update position
	pWpt->move(newPosition);

	// Validate connections still work
	for (int pathId : oldPaths)
	{
		CWaypoint* pOther = CWaypoints::getWaypoint(pathId);
		if (pOther != nullptr && !canReachFrom(newPosition, pOther->getOrigin()))
		{
			// Connection no longer valid
			pWpt->removePathTo(pathId);
			pOther->removePathTo(waypointId);
		}
	}

	// Try to add new connections
	repairConnections(waypointId);

	CBotGlobals::botMessage(nullptr, 0, "Relocated waypoint %d to (%.0f, %.0f, %.0f)",
		waypointId, newPosition.x, newPosition.y, newPosition.z);

	return true;
}

//=============================================================================
// Bad Connection Detection
//=============================================================================

std::vector<CBadConnectionInfo> CWaypointAutoRefiner::detectBadConnections()
{
	std::vector<CBadConnectionInfo> badConnections;

	CNavTestManager& navTest = CNavTestManager::instance();
	CNavTestIssueTracker& issueTracker = navTest.getIssueTracker();
	const std::vector<CNavTestIssue>& allIssues = issueTracker.getAllIssues();

	// Track issues per connection
	std::map<std::pair<int, int>, CBadConnectionInfo> connectionIssues;

	for (const auto& issue : allIssues)
	{
		if (issue.sourceWaypointId < 0 || issue.destWaypointId < 0)
			continue;

		auto key = std::make_pair(issue.sourceWaypointId, issue.destWaypointId);

		if (connectionIssues.find(key) == connectionIssues.end())
		{
			CBadConnectionInfo info;
			info.fromWaypointId = issue.sourceWaypointId;
			info.toWaypointId = issue.destWaypointId;
			connectionIssues[key] = info;
		}

		CBadConnectionInfo& info = connectionIssues[key];

		switch (issue.type)
		{
		case ENavTestIssueType::FALL_DAMAGE:
			info.fallDamageCount += issue.occurrenceCount;
			break;
		case ENavTestIssueType::STUCK:
			info.stuckCount += issue.occurrenceCount;
			break;
		default:
			break;
		}

		info.failureCount += issue.occurrenceCount;
		info.issueScore += calculateSeverityScore(issue);
	}

	// Convert to vector and determine recommendations
	for (auto& pair : connectionIssues)
	{
		CBadConnectionInfo& info = pair.second;

		if (info.issueScore > 20.0f || info.failureCount > 5)
		{
			info.shouldRemove = true;
			info.alternativeRoute = findAlternativeRoute(info.fromWaypointId, info.toWaypointId);
		}

		if (info.issueScore > 5.0f)
		{
			badConnections.push_back(info);
		}
	}

	// Sort by issue score
	std::sort(badConnections.begin(), badConnections.end(),
		[](const CBadConnectionInfo& a, const CBadConnectionInfo& b) {
			return a.issueScore > b.issueScore;
		});

	return badConnections;
}

//=============================================================================
// Connection Removal/Rerouting
//=============================================================================

bool CWaypointAutoRefiner::removeConnection(int fromWpt, int toWpt)
{
	CWaypoint* pFrom = CWaypoints::getWaypoint(fromWpt);
	if (pFrom == nullptr)
		return false;

	// Check if removal would cause isolation
	CWaypoint* pTo = CWaypoints::getWaypoint(toWpt);
	if (pTo != nullptr && pTo->numPaths() <= 1)
	{
		// Would isolate destination - check if there's an alternative
		std::vector<int> alt = findAlternativeRoute(fromWpt, toWpt);
		if (alt.empty())
		{
			CBotGlobals::botMessage(nullptr, 0,
				"Cannot remove connection %d->%d: would isolate waypoint", fromWpt, toWpt);
			return false;
		}
	}

	pFrom->removePathTo(toWpt);
	m_totalConnectionsRemoved++;

	CBotGlobals::botMessage(nullptr, 0, "Removed connection %d -> %d", fromWpt, toWpt);
	return true;
}

std::vector<int> CWaypointAutoRefiner::findAlternativeRoute(int fromWpt, int toWpt)
{
	std::vector<int> route;

	CWaypoint* pFrom = CWaypoints::getWaypoint(fromWpt);
	CWaypoint* pTo = CWaypoints::getWaypoint(toWpt);

	if (pFrom == nullptr || pTo == nullptr)
		return route;

	// Simple alternative: find a common neighbor
	for (int p = 0; p < pFrom->numPaths(); p++)
	{
		int midWpt = pFrom->getPath(p);
		if (midWpt == toWpt)
			continue;

		CWaypoint* pMid = CWaypoints::getWaypoint(midWpt);
		if (pMid == nullptr)
			continue;

		// Check if mid connects to destination
		for (int q = 0; q < pMid->numPaths(); q++)
		{
			if (pMid->getPath(q) == toWpt)
			{
				route.push_back(fromWpt);
				route.push_back(midWpt);
				route.push_back(toWpt);
				return route;
			}
		}
	}

	return route;
}

//=============================================================================
// Iteration Cycle
//=============================================================================

bool CWaypointAutoRefiner::runIteration()
{
	m_currentIteration++;

	CIterationState state;
	state.iterationNumber = m_currentIteration;

	// Get current issue count
	CNavTestManager& navTest = CNavTestManager::instance();
	state.totalIssuesBefore = navTest.getIssueTracker().getIssueCount();

	CBotGlobals::botMessage(nullptr, 0, "--- Iteration %d ---", m_currentIteration);

	// Analyze current state
	CAnalysisResult analysis = analyze();

	if (m_options.analyzeOnly)
	{
		printAnalysis(analysis, true);
		m_iterationHistory.push_back(state);
		return false;  // Don't continue
	}

	// Apply suggestions
	int appliedCount = 0;
	int addCount = 0;
	int removeCount = 0;

	for (const auto& suggestion : analysis.suggestions)
	{
		if (m_options.dryRun)
		{
			CBotGlobals::botMessage(nullptr, 0, "[DRY RUN] Would apply: %s", suggestion.reason.c_str());
			continue;
		}

		// Limit per iteration
		if (suggestion.type == CWaypointSuggestion::ESuggestionType::ADD_WAYPOINT)
		{
			if (addCount >= m_options.maxWaypointsPerIteration)
				continue;
		}
		if (suggestion.type == CWaypointSuggestion::ESuggestionType::REMOVE_WAYPOINT ||
			suggestion.type == CWaypointSuggestion::ESuggestionType::REMOVE_CONNECTION)
		{
			if (removeCount >= m_options.maxRemovalsPerIteration)
				continue;
		}

		if (applySuggestion(suggestion))
		{
			appliedCount++;

			if (suggestion.type == CWaypointSuggestion::ESuggestionType::ADD_WAYPOINT)
				addCount++;
			else if (suggestion.type == CWaypointSuggestion::ESuggestionType::REMOVE_WAYPOINT ||
					 suggestion.type == CWaypointSuggestion::ESuggestionType::REMOVE_CONNECTION)
				removeCount++;
		}
	}

	state.waypointsAdded = addCount;
	state.waypointsRemoved = removeCount;

	// Save snapshot if configured
	if (m_options.autoSave && appliedCount > 0)
	{
		saveSnapshot();
	}

	// Record state
	state.totalIssuesAfter = state.totalIssuesBefore;  // Issues won't change until next nav-test
	if (m_iterationHistory.size() > 0)
	{
		float prevIssues = static_cast<float>(m_iterationHistory.back().totalIssuesBefore);
		if (prevIssues > 0)
		{
			state.improvementPercent = (prevIssues - static_cast<float>(state.totalIssuesBefore)) / prevIssues;
		}
	}

	m_iterationHistory.push_back(state);

	CBotGlobals::botMessage(nullptr, 0, "Iteration %d complete: %d changes applied",
		m_currentIteration, appliedCount);

	return appliedCount > 0;
}

bool CWaypointAutoRefiner::applySuggestion(const CWaypointSuggestion& suggestion)
{
	switch (suggestion.type)
	{
	case CWaypointSuggestion::ESuggestionType::ADD_WAYPOINT:
		return insertWaypoint(suggestion.position, suggestion.suggestedFlags) >= 0;

	case CWaypointSuggestion::ESuggestionType::REMOVE_WAYPOINT:
		return removeWaypoint(suggestion.waypointId, true);

	case CWaypointSuggestion::ESuggestionType::RELOCATE_WAYPOINT:
		return relocateWaypoint(suggestion.waypointId, suggestion.position);

	case CWaypointSuggestion::ESuggestionType::ADD_CONNECTION:
		return addConnection(suggestion.waypointId, suggestion.targetWaypointId);

	case CWaypointSuggestion::ESuggestionType::REMOVE_CONNECTION:
		return removeConnection(suggestion.waypointId, suggestion.targetWaypointId);

	default:
		return false;
	}
}

//=============================================================================
// Improvement Tracking
//=============================================================================

float CWaypointAutoRefiner::calculateImprovement() const
{
	if (m_iterationHistory.size() < 2)
		return 0.0f;

	int firstIssues = m_iterationHistory.front().totalIssuesBefore;
	int lastIssues = m_iterationHistory.back().totalIssuesBefore;

	if (firstIssues == 0)
		return 0.0f;

	return static_cast<float>(firstIssues - lastIssues) / static_cast<float>(firstIssues);
}

bool CWaypointAutoRefiner::hasImproved() const
{
	if (m_iterationHistory.size() < 2)
		return true;  // Assume improvement possible

	const CIterationState& prev = m_iterationHistory[m_iterationHistory.size() - 2];
	const CIterationState& curr = m_iterationHistory.back();

	return curr.totalIssuesBefore < prev.totalIssuesBefore;
}

//=============================================================================
// Stopping Criteria
//=============================================================================

bool CWaypointAutoRefiner::shouldStop() const
{
	if (m_currentIteration >= m_options.maxIterations)
		return true;

	if (m_iterationHistory.size() < 2)
		return false;

	// Check if improvement is below threshold
	float recentImprovement = m_iterationHistory.back().improvementPercent;
	if (recentImprovement < m_options.minImprovement && recentImprovement >= 0.0f)
	{
		// Check if last few iterations showed no improvement
		int noImprovementCount = 0;
		for (size_t i = m_iterationHistory.size(); i > 0 && i > m_iterationHistory.size() - 3; i--)
		{
			if (m_iterationHistory[i - 1].improvementPercent < m_options.minImprovement)
				noImprovementCount++;
		}

		if (noImprovementCount >= 2)
			return true;
	}

	return false;
}

//=============================================================================
// Rollback
//=============================================================================

bool CWaypointAutoRefiner::saveSnapshot()
{
	CWaypointSnapshot snapshot;
	snapshot.iterationNumber = m_currentIteration;
	snapshot.waypointCount = CWaypoints::numWaypoints();

	// Generate filename
	char filename[256];
	snprintf(filename, sizeof(filename), "waypoints_autorefine_backup_%d.rcw", m_currentIteration);
	snapshot.filename = filename;

	// Save waypoints
	const char* mapName = CBotGlobals::getMapName();
	if (mapName != nullptr && CWaypoints::save(false, nullptr, mapName))
	{
		m_snapshots.push_back(snapshot);
		CBotGlobals::botMessage(nullptr, 0, "Saved snapshot for iteration %d", m_currentIteration);
		return true;
	}

	return false;
}

bool CWaypointAutoRefiner::rollback(int iterationNumber)
{
	if (m_snapshots.empty())
	{
		CBotGlobals::botMessage(nullptr, 0, "No snapshots available for rollback");
		return false;
	}

	int targetIteration = iterationNumber;
	if (targetIteration < 0)
	{
		// Rollback to previous
		targetIteration = m_snapshots.back().iterationNumber;
	}

	// Find snapshot
	for (auto it = m_snapshots.rbegin(); it != m_snapshots.rend(); ++it)
	{
		if (it->iterationNumber <= targetIteration)
		{
			// Load this snapshot
			const char* mapName = CBotGlobals::getMapName();
			if (mapName != nullptr && CWaypoints::load(mapName))
			{
				CBotGlobals::botMessage(nullptr, 0, "Rolled back to iteration %d", it->iterationNumber);
				return true;
			}
			break;
		}
	}

	CBotGlobals::botMessage(nullptr, 0, "Failed to rollback to iteration %d", targetIteration);
	return false;
}

//=============================================================================
// Statistics
//=============================================================================

int CWaypointAutoRefiner::getTotalWaypointsAdded() const
{
	return m_totalWaypointsAdded;
}

int CWaypointAutoRefiner::getTotalWaypointsRemoved() const
{
	return m_totalWaypointsRemoved;
}

int CWaypointAutoRefiner::getTotalConnectionsFixed() const
{
	return m_totalConnectionsAdded + m_totalConnectionsRemoved;
}

//=============================================================================
// Internal Helpers
//=============================================================================

float CWaypointAutoRefiner::calculateWaypointIssueScore(int waypointId) const
{
	CNavTestManager& navTest = CNavTestManager::instance();
	CNavTestIssueTracker& issueTracker = navTest.getIssueTracker();

	std::vector<CNavTestIssue> issues = issueTracker.getIssuesForWaypoint(waypointId);
	float score = 0.0f;

	for (const auto& issue : issues)
	{
		score += calculateSeverityScore(issue);
	}

	return score;
}

float CWaypointAutoRefiner::calculateConnectionIssueScore(int fromWpt, int toWpt) const
{
	CNavTestManager& navTest = CNavTestManager::instance();
	CNavTestIssueTracker& issueTracker = navTest.getIssueTracker();
	const std::vector<CNavTestIssue>& allIssues = issueTracker.getAllIssues();

	float score = 0.0f;

	for (const auto& issue : allIssues)
	{
		if (issue.sourceWaypointId == fromWpt && issue.destWaypointId == toWpt)
		{
			score += calculateSeverityScore(issue);
		}
	}

	return score;
}

bool CWaypointAutoRefiner::isLocationValid(const Vector& position) const
{
	// Basic validity check - position should be in world bounds
	// and not inside solid geometry

	// Check world bounds (typical Source engine limits)
	if (fabs(position.x) > 16384.0f ||
		fabs(position.y) > 16384.0f ||
		fabs(position.z) > 16384.0f)
	{
		return false;
	}

	// Would need trace to check solid geometry - simplified for now
	return true;
}

bool CWaypointAutoRefiner::canReachFrom(const Vector& from, const Vector& to) const
{
	// Simplified reachability check
	float dist = (to - from).Length();

	// Too far for direct connection
	if (dist > 500.0f)
		return false;

	// Check height difference
	float heightDiff = to.z - from.z;
	if (heightDiff > 60.0f)  // Too high to jump
		return false;

	if (heightDiff < -300.0f)  // Too far to fall safely
		return false;

	// Would need trace for proper visibility check
	return true;
}

std::vector<int> CWaypointAutoRefiner::findNearbyWaypoints(const Vector& position, float radius) const
{
	std::vector<int> nearby;
	float radiusSq = radius * radius;

	int numWaypoints = CWaypoints::numWaypoints();
	for (int i = 0; i < numWaypoints; i++)
	{
		CWaypoint* pWpt = CWaypoints::getWaypoint(i);
		if (pWpt == nullptr || !pWpt->isUsed())
			continue;

		float distSq = (pWpt->getOrigin() - position).LengthSqr();
		if (distSq <= radiusSq)
		{
			nearby.push_back(i);
		}
	}

	return nearby;
}

//=============================================================================
// Console Command Handlers
//=============================================================================

void Waypoint_AutoRefine_Command(const CCommand& args)
{
	CRefineOptions options;

	// Parse arguments
	for (int i = 1; i < args.ArgC(); i++)
	{
		const char* arg = args.Arg(i);

		if (strcmp(arg, "analyze-only") == 0)
		{
			options.analyzeOnly = true;
		}
		else if (strcmp(arg, "dry-run") == 0)
		{
			options.dryRun = true;
		}
		else if (strcmp(arg, "no-save") == 0)
		{
			options.autoSave = false;
		}
		else if (strcmp(arg, "no-remove") == 0)
		{
			options.allowWaypointRemoval = false;
			options.allowConnectionRemoval = false;
		}
		else if (strncmp(arg, "max-iter=", 9) == 0)
		{
			options.maxIterations = atoi(arg + 9);
			if (options.maxIterations < 1)
				options.maxIterations = 1;
			if (options.maxIterations > 100)
				options.maxIterations = 100;
		}
		else if (strcmp(arg, "stop") == 0)
		{
			CWaypointAutoRefiner::instance().stopRefinement();
			return;
		}
		else if (strcmp(arg, "rollback") == 0)
		{
			int iter = -1;
			if (i + 1 < args.ArgC())
			{
				iter = atoi(args.Arg(i + 1));
			}
			CWaypointAutoRefiner::instance().rollback(iter);
			return;
		}
		else if (strcmp(arg, "help") == 0)
		{
			CBotGlobals::botMessage(nullptr, 0, "Usage: rcbot waypoint autorefine [options]");
			CBotGlobals::botMessage(nullptr, 0, "Options:");
			CBotGlobals::botMessage(nullptr, 0, "  analyze-only  - Only analyze, don't make changes");
			CBotGlobals::botMessage(nullptr, 0, "  dry-run       - Show what would be done");
			CBotGlobals::botMessage(nullptr, 0, "  no-save       - Don't auto-save after iterations");
			CBotGlobals::botMessage(nullptr, 0, "  no-remove     - Don't remove waypoints/connections");
			CBotGlobals::botMessage(nullptr, 0, "  max-iter=N    - Maximum iterations (default: 10)");
			CBotGlobals::botMessage(nullptr, 0, "  stop          - Stop current refinement");
			CBotGlobals::botMessage(nullptr, 0, "  rollback [N]  - Rollback to iteration N");
			return;
		}
	}

	CWaypointAutoRefiner::instance().startRefinement(options);
}

void Waypoint_Analyze_Command(const CCommand& args)
{
	bool verbose = false;

	for (int i = 1; i < args.ArgC(); i++)
	{
		if (strcmp(args.Arg(i), "-v") == 0 || strcmp(args.Arg(i), "verbose") == 0)
		{
			verbose = true;
		}
	}

	CAnalysisResult result = CWaypointAutoRefiner::instance().analyze();
	CWaypointAutoRefiner::instance().printAnalysis(result, verbose);
}
