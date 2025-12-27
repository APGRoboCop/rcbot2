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

#ifndef __BOT_NAVTEST_H__
#define __BOT_NAVTEST_H__

#include <vector>
#include <string>
#include <cstdint>
#include <ctime>
#include "mathlib/vector.h"

class CBot;
class CWaypoint;
class CCommand;

//=============================================================================
// Nav-Test Issue Types
// Categorizes different types of navigation problems detected during testing
//=============================================================================
enum class ENavTestIssueType : uint8_t
{
	NONE = 0,
	STUCK,              // Bot got stuck at this location
	UNREACHABLE,        // Waypoint could not be reached via pathfinding
	PATH_FAILURE,       // Bot abandoned path or couldn't complete it
	FALL_DAMAGE,        // Bot took fall damage on this connection
	OUT_OF_BOUNDS,      // Bot went outside expected navigation area
	SLOW_TRAVERSE,      // Path took much longer than expected
	CONNECTION_BROKEN,  // Waypoint connection doesn't work in practice
	TELEFRAG_RISK,      // Teleport destination has collision risk
	DOOR_BLOCKED,       // Door was locked or couldn't be opened
	MAX_ISSUE_TYPES
};

//=============================================================================
// Nav-Test Issue Severity
// How serious is the navigation problem?
//=============================================================================
enum class ENavTestIssueSeverity : uint8_t
{
	LOW = 0,      // Minor inconvenience, bot eventually succeeded
	MEDIUM,       // Significant delay or suboptimal behavior
	HIGH,         // Bot failed to complete navigation goal
	CRITICAL      // Bot is completely stuck or navigation impossible
};

//=============================================================================
// CNavTestIssue
// Represents a single navigation issue detected during testing
//=============================================================================
struct CNavTestIssue
{
	ENavTestIssueType type;
	ENavTestIssueSeverity severity;
	Vector location;              // World position where issue occurred
	int sourceWaypointId;         // Starting waypoint (or -1)
	int destWaypointId;           // Destination waypoint (or -1)
	int connectionId;             // If issue is with a specific connection
	float timestamp;              // Engine time when issue occurred
	int botIndex;                 // Which bot encountered this
	int occurrenceCount;          // How many times this issue was seen
	float serverGravity;          // sv_gravity at time of issue (for fall damage)
	std::string additionalInfo;   // Extra context about the issue

	CNavTestIssue()
		: type(ENavTestIssueType::NONE)
		, severity(ENavTestIssueSeverity::LOW)
		, location(0, 0, 0)
		, sourceWaypointId(-1)
		, destWaypointId(-1)
		, connectionId(-1)
		, timestamp(0.0f)
		, botIndex(-1)
		, occurrenceCount(1)
		, serverGravity(600.0f)
	{
	}
};

//=============================================================================
// CNavTestWaypointVisit
// Tracks when a waypoint was visited during nav-testing
//=============================================================================
struct CNavTestWaypointVisit
{
	int waypointId;
	int botIndex;
	float timestamp;
	float timeSpentNearby;   // How long bot stayed near this waypoint
	bool wasDestination;     // Was this the intended destination?
	bool reachedSuccessfully;

	CNavTestWaypointVisit()
		: waypointId(-1)
		, botIndex(-1)
		, timestamp(0.0f)
		, timeSpentNearby(0.0f)
		, wasDestination(false)
		, reachedSuccessfully(false)
	{
	}
};

//=============================================================================
// CNavTestSession
// Represents a single nav-test session with metadata
//=============================================================================
struct CNavTestSession
{
	int sessionId;
	std::string mapName;
	time_t startTime;
	time_t endTime;
	float duration;              // Session duration in seconds
	int totalWaypoints;          // Number of waypoints in map
	int visitedWaypoints;        // Unique waypoints visited
	int totalIssues;             // Total issues detected
	int criticalIssues;          // Critical severity issues
	bool isActive;

	CNavTestSession()
		: sessionId(0)
		, startTime(0)
		, endTime(0)
		, duration(0.0f)
		, totalWaypoints(0)
		, visitedWaypoints(0)
		, totalIssues(0)
		, criticalIssues(0)
		, isActive(false)
	{
	}
};

//=============================================================================
// CMapCoverageTracker
// Tracks which waypoints have been visited and by which bots
//=============================================================================
class CMapCoverageTracker
{
public:
	CMapCoverageTracker();
	~CMapCoverageTracker();

	// Initialize for current map
	void init(int numWaypoints);

	// Reset all tracking data
	void reset();

	// Mark a waypoint as visited by a bot
	void markVisited(int waypointId, int botIndex, float timestamp);

	// Check if waypoint has been visited
	bool isVisited(int waypointId) const;

	// Get visit count for a waypoint
	int getVisitCount(int waypointId) const;

	// Get last visit time for a waypoint
	float getLastVisitTime(int waypointId) const;

	// Get coverage percentage (0.0 to 1.0)
	float getCoveragePercent() const;

	// Get number of visited waypoints
	int getVisitedCount() const;

	// Get least recently visited waypoint (for exploration priority)
	int getLeastRecentlyVisited() const;
	int getLeastRecentlyVisited(const Vector& fromOrigin) const;

	// Get unvisited waypoint (for exploration priority)
	int getUnvisitedWaypoint() const;
	int getUnvisitedWaypoint(const Vector& fromOrigin) const;

	// Get all visit records
	const std::vector<CNavTestWaypointVisit>& getVisits() const { return m_visits; }

private:
	struct WaypointVisitInfo
	{
		int visitCount;
		float lastVisitTime;
		float firstVisitTime;

		WaypointVisitInfo() : visitCount(0), lastVisitTime(0.0f), firstVisitTime(0.0f) {}
	};

	std::vector<WaypointVisitInfo> m_waypointInfo;
	std::vector<CNavTestWaypointVisit> m_visits;
	int m_numWaypoints;
	int m_visitedCount;
};

//=============================================================================
// CNavTestIssueTracker
// Collects and manages navigation issues during testing
//=============================================================================
class CNavTestIssueTracker
{
public:
	CNavTestIssueTracker();
	~CNavTestIssueTracker();

	// Reset all issues
	void reset();

	// Report a new issue
	void reportIssue(const CNavTestIssue& issue);

	// Report issue with helper function
	void reportStuck(int botIndex, const Vector& location, int nearWaypointId, float duration);
	void reportUnreachable(int botIndex, int sourceWpt, int destWpt);
	void reportUnreachable(CBot* pBot, int sourceWpt, int destWpt, const Vector& location);
	void reportPathFailure(int botIndex, int sourceWpt, int destWpt, const std::string& reason);
	void reportFallDamage(int botIndex, const Vector& location, int fromWpt, int toWpt, float damage, float gravity);
	void reportConnectionBroken(int botIndex, int fromWpt, int toWpt);

	// Get issue count
	int getIssueCount() const { return static_cast<int>(m_issues.size()); }
	int getIssueCountByType(ENavTestIssueType type) const;
	int getIssueCountBySeverity(ENavTestIssueSeverity severity) const;

	// Get issues for a specific waypoint
	std::vector<CNavTestIssue> getIssuesForWaypoint(int waypointId) const;

	// Get all issues
	const std::vector<CNavTestIssue>& getAllIssues() const { return m_issues; }

	// Check if waypoint has issues
	bool waypointHasIssues(int waypointId) const;

	// Get issue frequency for a waypoint (for auto-refinement)
	int getIssueFrequency(int waypointId) const;

private:
	std::vector<CNavTestIssue> m_issues;

	// Check for duplicate/similar issues and increment count instead
	bool tryMergeWithExisting(const CNavTestIssue& issue);
};

//=============================================================================
// CNavTestManager
// Main controller for the nav-test system
//=============================================================================
class CNavTestManager
{
public:
	static CNavTestManager& instance();

	// Session management
	bool startSession(float duration = 0.0f);  // 0 = unlimited
	void stopSession();
	bool isSessionActive() const { return m_currentSession.isActive; }
	const CNavTestSession& getCurrentSession() const { return m_currentSession; }

	// Per-frame update
	void update();

	// Bot hooks - called by bots during nav-test mode
	void onBotStuck(CBot* pBot, float duration);
	void onBotPathFailure(CBot* pBot, int sourceWpt, int destWpt);
	void onBotWaypointReached(CBot* pBot, int waypointId);
	void onBotFallDamage(CBot* pBot, float damage);
	void onBotUnreachable(CBot* pBot, int sourceWpt, int destWpt);

	// Get trackers
	CMapCoverageTracker& getCoverageTracker() { return m_coverageTracker; }
	CNavTestIssueTracker& getIssueTracker() { return m_issueTracker; }

	// Generate report
	void generateReport(bool toConsole = true, const char* filename = nullptr);

	// Get next exploration target for a bot
	int getNextExplorationWaypoint(CBot* pBot);

	// Check if a specific bot is in nav-test mode
	bool isBotInNavTestMode(CBot* pBot) const;
	void setBotNavTestMode(CBot* pBot, bool enabled);

	// Get current server gravity (cached)
	float getServerGravity() const { return m_cachedGravity; }
	void updateGravity();

private:
	CNavTestManager();
	~CNavTestManager();

	// Prevent copying
	CNavTestManager(const CNavTestManager&) = delete;
	CNavTestManager& operator=(const CNavTestManager&) = delete;

	CNavTestSession m_currentSession;
	CMapCoverageTracker m_coverageTracker;
	CNavTestIssueTracker m_issueTracker;

	float m_sessionEndTime;       // Engine time when session should end (0 = no limit)
	float m_lastUpdateTime;
	float m_cachedGravity;

	std::vector<int> m_botsInNavTestMode;  // Bot indices in nav-test mode

	static CNavTestManager* s_instance;
};

//=============================================================================
// CNavTestDatabase
// Handles persistent storage of nav-test data
// Uses binary files for storage - can be upgraded to SQLite later
//
// Database Schema (Binary File Format):
// =====================================
//
// SESSION FILES ({mapname}_session_{id}.bin):
//   - sessionId (int)
//   - mapName (uint32 length + string)
//   - startTime (time_t)
//   - endTime (time_t)
//   - duration (float)
//   - totalWaypoints (int)
//   - visitedWaypoints (int)
//   - totalIssues (int)
//   - criticalIssues (int)
//
// ISSUE FILES ({mapname}_issues_{id}.bin):
//   - count (uint32)
//   - For each issue:
//     - type (ENavTestIssueType)
//     - severity (ENavTestIssueSeverity)
//     - location (3x float: x, y, z)
//     - sourceWaypointId (int)
//     - destWaypointId (int)
//     - connectionId (int)
//     - timestamp (float)
//     - botIndex (int)
//     - occurrenceCount (int)
//     - serverGravity (float)
//     - additionalInfo (uint32 length + string)
//
// COVERAGE FILES ({mapname}_coverage_{id}.bin):
//   - count (uint32)
//   - For each visit:
//     - waypointId (int)
//     - botIndex (int)
//     - timestamp (float)
//     - timeSpentNearby (float)
//     - wasDestination (bool)
//     - reachedSuccessfully (bool)
//
// Thread Safety:
//   - File operations are atomic per-file (one session at a time)
//   - Each CNavTestDatabase instance maintains its own cache
//   - For multi-threaded access, use separate instances per thread
//
//=============================================================================
class CNavTestDatabase
{
public:
	CNavTestDatabase();
	~CNavTestDatabase();

	// Database file management
	bool openDatabase(const char* mapName);
	void closeDatabase();
	bool isDatabaseOpen() const { return m_bIsOpen; }

	// Session management
	bool saveSession(const CNavTestSession& session);
	bool loadSession(int sessionId, CNavTestSession& session);
	std::vector<CNavTestSession> getAllSessions();
	int getNextSessionId();

	// Issue storage
	bool saveIssues(const std::vector<CNavTestIssue>& issues, int sessionId);
	bool loadIssues(int sessionId, std::vector<CNavTestIssue>& issues);
	bool appendIssue(const CNavTestIssue& issue, int sessionId);

	// Coverage storage
	bool saveCoverage(const std::vector<CNavTestWaypointVisit>& visits, int sessionId);
	bool loadCoverage(int sessionId, std::vector<CNavTestWaypointVisit>& visits);

	// Query functions
	std::vector<CNavTestIssue> getIssuesByType(ENavTestIssueType type);
	std::vector<CNavTestIssue> getIssuesByWaypoint(int waypointId);
	std::vector<CNavTestIssue> getIssuesBySeverity(ENavTestIssueSeverity severity);
	int getIssueFrequency(int waypointId);

	// Aggregate statistics
	struct AggregateStats
	{
		int totalSessions;
		int totalIssues;
		int uniqueIssueWaypoints;
		float avgCoverage;
		int mostProblematicWaypoint;
		int mostProblematicWaypointIssues;
	};
	AggregateStats getAggregateStats();

	// Report generation
	bool generateReport(const char* filename);
	bool generateHTMLReport(const char* filename);
	std::string generateReportString();

	// Cleanup
	bool deleteSession(int sessionId);
	bool clearAllData();

private:
	// File paths
	std::string m_databasePath;
	std::string m_mapName;
	bool m_bIsOpen;

	// Cached data for queries
	std::vector<CNavTestSession> m_cachedSessions;
	std::vector<CNavTestIssue> m_cachedIssues;
	bool m_bCacheValid;

	// Internal helpers
	std::string getSessionFilePath(int sessionId);
	std::string getIssuesFilePath(int sessionId);
	std::string getCoverageFilePath(int sessionId);
	void invalidateCache();
	void rebuildCache();
};

//=============================================================================
// Console command handlers
//=============================================================================
void NavTest_StartCommand(const CCommand& args);
void NavTest_StopCommand(const CCommand& args);
void NavTest_StatusCommand(const CCommand& args);
void NavTest_ReportCommand(const CCommand& args);
void NavTest_SaveCommand(const CCommand& args);
void NavTest_LoadCommand(const CCommand& args);

#endif // __BOT_NAVTEST_H__
