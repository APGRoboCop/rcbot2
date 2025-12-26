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

#ifndef __BOT_WAYPOINT_AUTOREFINE_H__
#define __BOT_WAYPOINT_AUTOREFINE_H__

#include <vector>
#include <string>
#include <map>
#include <cstdint>

#include "bot_navtest.h"

class CWaypoint;

//=============================================================================
// Issue Cluster
// Groups nearby navigation issues to identify problem areas
//=============================================================================
struct CIssueCluster
{
	Vector centroid;                       // Center of the cluster
	std::vector<CNavTestIssue> issues;     // Issues in this cluster
	float radius;                          // Cluster radius
	float severityScore;                   // Weighted severity score
	int primaryWaypointId;                 // Main waypoint associated with cluster
	std::vector<int> affectedWaypoints;    // All waypoints in cluster area

	CIssueCluster()
		: centroid(0, 0, 0)
		, radius(0.0f)
		, severityScore(0.0f)
		, primaryWaypointId(-1)
	{
	}

	// Get the most common issue type in this cluster
	ENavTestIssueType getDominantIssueType() const;

	// Get count of issues by type
	int getIssueCount(ENavTestIssueType type) const;
};

//=============================================================================
// Waypoint Suggestion
// Suggested waypoint placement to fix navigation issues
//=============================================================================
struct CWaypointSuggestion
{
	enum class ESuggestionType : uint8_t
	{
		ADD_WAYPOINT,        // Add a new waypoint
		REMOVE_WAYPOINT,     // Remove an existing waypoint
		RELOCATE_WAYPOINT,   // Move waypoint to new position
		ADD_CONNECTION,      // Add connection between waypoints
		REMOVE_CONNECTION,   // Remove a bad connection
		MODIFY_FLAGS         // Change waypoint flags
	};

	ESuggestionType type;
	Vector position;                   // Position for new/relocated waypoint
	int waypointId;                    // Existing waypoint ID (for remove/relocate/modify)
	int targetWaypointId;              // Target waypoint (for connections)
	int suggestedFlags;                // Suggested waypoint flags
	float confidence;                  // Confidence in this suggestion (0.0-1.0)
	std::string reason;                // Human-readable reason for suggestion
	const CIssueCluster* sourceCluster; // Cluster that prompted this suggestion

	CWaypointSuggestion()
		: type(ESuggestionType::ADD_WAYPOINT)
		, position(0, 0, 0)
		, waypointId(-1)
		, targetWaypointId(-1)
		, suggestedFlags(0)
		, confidence(0.0f)
		, sourceCluster(nullptr)
	{
	}
};

//=============================================================================
// Bad Waypoint Info
// Information about a problematic waypoint
//=============================================================================
struct CBadWaypointInfo
{
	int waypointId;
	float issueScore;                  // How problematic this waypoint is
	int stuckCount;                    // Number of stuck events
	int pathFailureCount;              // Number of path failures
	int unreachableCount;              // Number of unreachable reports
	std::vector<int> badConnections;   // Connections that cause problems
	bool shouldRemove;                 // Recommendation to remove
	bool shouldRelocate;               // Recommendation to relocate
	Vector suggestedPosition;          // If relocating, where to move

	CBadWaypointInfo()
		: waypointId(-1)
		, issueScore(0.0f)
		, stuckCount(0)
		, pathFailureCount(0)
		, unreachableCount(0)
		, shouldRemove(false)
		, shouldRelocate(false)
		, suggestedPosition(0, 0, 0)
	{
	}
};

//=============================================================================
// Bad Connection Info
// Information about a problematic waypoint connection
//=============================================================================
struct CBadConnectionInfo
{
	int fromWaypointId;
	int toWaypointId;
	float issueScore;
	int fallDamageCount;               // Fall damage incidents
	int stuckCount;                    // Stuck incidents on this connection
	int failureCount;                  // Total failures
	float averageFallDamage;           // Average damage when falling
	bool shouldRemove;                 // Recommendation to remove
	std::vector<int> alternativeRoute; // Suggested alternative path

	CBadConnectionInfo()
		: fromWaypointId(-1)
		, toWaypointId(-1)
		, issueScore(0.0f)
		, fallDamageCount(0)
		, stuckCount(0)
		, failureCount(0)
		, averageFallDamage(0.0f)
		, shouldRemove(false)
	{
	}
};

//=============================================================================
// Iteration State
// State of the auto-refinement iteration cycle
//=============================================================================
struct CIterationState
{
	int iterationNumber;
	int totalIssuesBefore;
	int totalIssuesAfter;
	int waypointsAdded;
	int waypointsRemoved;
	int connectionsAdded;
	int connectionsRemoved;
	float improvementPercent;          // Percentage improvement in issues
	std::string timestamp;

	CIterationState()
		: iterationNumber(0)
		, totalIssuesBefore(0)
		, totalIssuesAfter(0)
		, waypointsAdded(0)
		, waypointsRemoved(0)
		, connectionsAdded(0)
		, connectionsRemoved(0)
		, improvementPercent(0.0f)
	{
	}
};

//=============================================================================
// Waypoint Snapshot
// Saved state for rollback capability
//=============================================================================
struct CWaypointSnapshot
{
	int iterationNumber;
	std::string filename;              // Saved waypoint file
	int waypointCount;
	int connectionCount;
	int issueCount;

	CWaypointSnapshot()
		: iterationNumber(0)
		, waypointCount(0)
		, connectionCount(0)
		, issueCount(0)
	{
	}
};

//=============================================================================
// Analysis Result
// Complete analysis of navigation issues
//=============================================================================
struct CAnalysisResult
{
	std::vector<CIssueCluster> clusters;
	std::vector<CBadWaypointInfo> badWaypoints;
	std::vector<CBadConnectionInfo> badConnections;
	std::vector<CWaypointSuggestion> suggestions;
	int totalIssues;
	int criticalIssues;
	int clusterCount;
	float overallHealthScore;          // 0.0 = terrible, 1.0 = perfect

	CAnalysisResult()
		: totalIssues(0)
		, criticalIssues(0)
		, clusterCount(0)
		, overallHealthScore(1.0f)
	{
	}

	void clear()
	{
		clusters.clear();
		badWaypoints.clear();
		badConnections.clear();
		suggestions.clear();
		totalIssues = 0;
		criticalIssues = 0;
		clusterCount = 0;
		overallHealthScore = 1.0f;
	}
};

//=============================================================================
// Refine Options
// Configuration options for auto-refinement
//=============================================================================
struct CRefineOptions
{
	bool analyzeOnly;                  // Only analyze, don't make changes
	bool dryRun;                       // Show what would be done without doing it
	int maxIterations;                 // Maximum number of iterations
	bool autoSave;                     // Automatically save after each iteration
	float minImprovement;              // Minimum improvement to continue (0.0-1.0)
	float clusterRadius;               // Radius for issue clustering
	float minConfidence;               // Minimum confidence for suggestions
	bool allowWaypointRemoval;         // Allow removing waypoints
	bool allowConnectionRemoval;       // Allow removing connections
	int maxWaypointsPerIteration;      // Max waypoints to add per iteration
	int maxRemovalsPerIteration;       // Max removals per iteration

	CRefineOptions()
		: analyzeOnly(false)
		, dryRun(false)
		, maxIterations(10)
		, autoSave(true)
		, minImprovement(0.05f)
		, clusterRadius(200.0f)
		, minConfidence(0.6f)
		, allowWaypointRemoval(true)
		, allowConnectionRemoval(true)
		, maxWaypointsPerIteration(10)
		, maxRemovalsPerIteration(5)
	{
	}
};

//=============================================================================
// CWaypointAutoRefiner
// Main coordinator for iterative waypoint auto-generation
//=============================================================================
class CWaypointAutoRefiner
{
public:
	static CWaypointAutoRefiner& instance();

	// Main operations
	bool startRefinement(const CRefineOptions& options = CRefineOptions());
	void stopRefinement();
	bool isRefining() const { return m_bRefining; }

	// Analysis
	CAnalysisResult analyze();
	void printAnalysis(const CAnalysisResult& result, bool verbose = false);

	// Issue clustering
	std::vector<CIssueCluster> clusterIssues(float radius = 200.0f);

	// Severity scoring
	float calculateSeverityScore(const CNavTestIssue& issue) const;
	float calculateClusterSeverity(const CIssueCluster& cluster) const;

	// Gap detection
	std::vector<Vector> detectGaps(const std::vector<CIssueCluster>& clusters);

	// Waypoint placement suggestions
	std::vector<CWaypointSuggestion> generateSuggestions(const CAnalysisResult& analysis);

	// Waypoint insertion
	int insertWaypoint(const Vector& position, int flags = 0);
	bool validateNewWaypoint(int waypointId);

	// Connection repair
	int repairConnections(int waypointId);
	bool addConnection(int fromWpt, int toWpt);

	// Bad waypoint detection
	std::vector<CBadWaypointInfo> detectBadWaypoints();

	// Waypoint removal
	bool removeWaypoint(int waypointId, bool updateConnections = true);
	bool relocateWaypoint(int waypointId, const Vector& newPosition);

	// Bad connection detection
	std::vector<CBadConnectionInfo> detectBadConnections();

	// Connection removal/rerouting
	bool removeConnection(int fromWpt, int toWpt);
	std::vector<int> findAlternativeRoute(int fromWpt, int toWpt);

	// Iteration cycle
	bool runIteration();
	int getCurrentIteration() const { return m_currentIteration; }
	const std::vector<CIterationState>& getIterationHistory() const { return m_iterationHistory; }

	// Improvement tracking
	float calculateImprovement() const;
	bool hasImproved() const;

	// Stopping criteria
	bool shouldStop() const;

	// Rollback
	bool saveSnapshot();
	bool rollback(int iterationNumber = -1);  // -1 = previous iteration
	const std::vector<CWaypointSnapshot>& getSnapshots() const { return m_snapshots; }

	// Statistics
	int getTotalWaypointsAdded() const;
	int getTotalWaypointsRemoved() const;
	int getTotalConnectionsFixed() const;

private:
	CWaypointAutoRefiner();
	~CWaypointAutoRefiner();

	// Prevent copying
	CWaypointAutoRefiner(const CWaypointAutoRefiner&) = delete;
	CWaypointAutoRefiner& operator=(const CWaypointAutoRefiner&) = delete;

	// Internal helpers
	void resetState();
	bool applySuggestion(const CWaypointSuggestion& suggestion);
	float calculateWaypointIssueScore(int waypointId) const;
	float calculateConnectionIssueScore(int fromWpt, int toWpt) const;
	bool isLocationValid(const Vector& position) const;
	bool canReachFrom(const Vector& from, const Vector& to) const;
	std::vector<int> findNearbyWaypoints(const Vector& position, float radius) const;

	// State
	bool m_bRefining;
	CRefineOptions m_options;
	int m_currentIteration;
	CAnalysisResult m_lastAnalysis;
	std::vector<CIterationState> m_iterationHistory;
	std::vector<CWaypointSnapshot> m_snapshots;

	// Statistics
	int m_totalWaypointsAdded;
	int m_totalWaypointsRemoved;
	int m_totalConnectionsAdded;
	int m_totalConnectionsRemoved;

	static CWaypointAutoRefiner* s_instance;
};

//=============================================================================
// Console command handlers
//=============================================================================
void Waypoint_AutoRefine_Command(const CCommand& args);
void Waypoint_Analyze_Command(const CCommand& args);

#endif // __BOT_WAYPOINT_AUTOREFINE_H__
