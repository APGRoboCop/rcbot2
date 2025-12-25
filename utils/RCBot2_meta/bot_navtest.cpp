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

#include "bot_navtest.h"
#include "bot.h"
#include "bot_globals.h"
#include "bot_waypoint.h"
#include "bot_cvars.h"

#include <algorithm>
#include <cstdio>

// Static instance
CNavTestManager* CNavTestManager::s_instance = nullptr;

//=============================================================================
// CMapCoverageTracker Implementation
//=============================================================================

CMapCoverageTracker::CMapCoverageTracker()
	: m_numWaypoints(0)
	, m_visitedCount(0)
{
}

CMapCoverageTracker::~CMapCoverageTracker()
{
}

void CMapCoverageTracker::init(int numWaypoints)
{
	reset();
	m_numWaypoints = numWaypoints;
	m_waypointInfo.resize(numWaypoints);
}

void CMapCoverageTracker::reset()
{
	m_waypointInfo.clear();
	m_visits.clear();
	m_numWaypoints = 0;
	m_visitedCount = 0;
}

void CMapCoverageTracker::markVisited(int waypointId, int botIndex, float timestamp)
{
	if (waypointId < 0 || waypointId >= m_numWaypoints)
		return;

	WaypointVisitInfo& info = m_waypointInfo[waypointId];

	// Track first visit for coverage stats
	if (info.visitCount == 0)
	{
		info.firstVisitTime = timestamp;
		m_visitedCount++;
	}

	info.visitCount++;
	info.lastVisitTime = timestamp;

	// Record the visit
	CNavTestWaypointVisit visit;
	visit.waypointId = waypointId;
	visit.botIndex = botIndex;
	visit.timestamp = timestamp;
	visit.reachedSuccessfully = true;
	m_visits.push_back(visit);
}

bool CMapCoverageTracker::isVisited(int waypointId) const
{
	if (waypointId < 0 || waypointId >= m_numWaypoints)
		return false;
	return m_waypointInfo[waypointId].visitCount > 0;
}

int CMapCoverageTracker::getVisitCount(int waypointId) const
{
	if (waypointId < 0 || waypointId >= m_numWaypoints)
		return 0;
	return m_waypointInfo[waypointId].visitCount;
}

float CMapCoverageTracker::getLastVisitTime(int waypointId) const
{
	if (waypointId < 0 || waypointId >= m_numWaypoints)
		return 0.0f;
	return m_waypointInfo[waypointId].lastVisitTime;
}

float CMapCoverageTracker::getCoveragePercent() const
{
	if (m_numWaypoints == 0)
		return 0.0f;
	return static_cast<float>(m_visitedCount) / static_cast<float>(m_numWaypoints);
}

int CMapCoverageTracker::getVisitedCount() const
{
	return m_visitedCount;
}

int CMapCoverageTracker::getLeastRecentlyVisited() const
{
	if (m_numWaypoints == 0)
		return -1;

	int lruWaypoint = -1;
	float oldestTime = FLT_MAX;

	for (int i = 0; i < m_numWaypoints; i++)
	{
		const WaypointVisitInfo& info = m_waypointInfo[i];

		// Check if waypoint is valid and used
		CWaypoint* pWpt = CWaypoints::getWaypoint(i);
		if (pWpt == nullptr || !pWpt->isUsed())
			continue;

		if (info.visitCount > 0 && info.lastVisitTime < oldestTime)
		{
			oldestTime = info.lastVisitTime;
			lruWaypoint = i;
		}
	}

	return lruWaypoint;
}

int CMapCoverageTracker::getUnvisitedWaypoint() const
{
	if (m_numWaypoints == 0)
		return -1;

	for (int i = 0; i < m_numWaypoints; i++)
	{
		// Check if waypoint is valid and used
		CWaypoint* pWpt = CWaypoints::getWaypoint(i);
		if (pWpt == nullptr || !pWpt->isUsed())
			continue;

		if (m_waypointInfo[i].visitCount == 0)
			return i;
	}

	return -1;
}

//=============================================================================
// CNavTestIssueTracker Implementation
//=============================================================================

CNavTestIssueTracker::CNavTestIssueTracker()
{
}

CNavTestIssueTracker::~CNavTestIssueTracker()
{
}

void CNavTestIssueTracker::reset()
{
	m_issues.clear();
}

bool CNavTestIssueTracker::tryMergeWithExisting(const CNavTestIssue& issue)
{
	// Look for similar existing issues to merge
	for (CNavTestIssue& existing : m_issues)
	{
		if (existing.type == issue.type &&
			existing.sourceWaypointId == issue.sourceWaypointId &&
			existing.destWaypointId == issue.destWaypointId)
		{
			// Same issue occurred again - increment count
			existing.occurrenceCount++;
			existing.timestamp = issue.timestamp; // Update to latest occurrence

			// Upgrade severity if new occurrence is more severe
			if (static_cast<int>(issue.severity) > static_cast<int>(existing.severity))
				existing.severity = issue.severity;

			return true;
		}
	}
	return false;
}

void CNavTestIssueTracker::reportIssue(const CNavTestIssue& issue)
{
	// Try to merge with existing issue first
	if (!tryMergeWithExisting(issue))
	{
		m_issues.push_back(issue);
	}
}

void CNavTestIssueTracker::reportStuck(int botIndex, const Vector& location, int nearWaypointId, float duration)
{
	CNavTestIssue issue;
	issue.type = ENavTestIssueType::STUCK;
	issue.botIndex = botIndex;
	issue.location = location;
	issue.sourceWaypointId = nearWaypointId;
	issue.timestamp = gpGlobals->curtime;
	issue.serverGravity = CNavTestManager::instance().getServerGravity();

	// Determine severity based on stuck duration
	if (duration >= 10.0f)
		issue.severity = ENavTestIssueSeverity::CRITICAL;
	else if (duration >= 5.0f)
		issue.severity = ENavTestIssueSeverity::HIGH;
	else if (duration >= 2.0f)
		issue.severity = ENavTestIssueSeverity::MEDIUM;
	else
		issue.severity = ENavTestIssueSeverity::LOW;

	char info[128];
	snprintf(info, sizeof(info), "Stuck for %.1fs", duration);
	issue.additionalInfo = info;

	reportIssue(issue);
}

void CNavTestIssueTracker::reportUnreachable(int botIndex, int sourceWpt, int destWpt)
{
	CNavTestIssue issue;
	issue.type = ENavTestIssueType::UNREACHABLE;
	issue.severity = ENavTestIssueSeverity::HIGH;
	issue.botIndex = botIndex;
	issue.sourceWaypointId = sourceWpt;
	issue.destWaypointId = destWpt;
	issue.timestamp = gpGlobals->curtime;
	issue.serverGravity = CNavTestManager::instance().getServerGravity();

	CWaypoint* pWpt = CWaypoints::getWaypoint(destWpt);
	if (pWpt != nullptr)
		issue.location = pWpt->getOrigin();

	reportIssue(issue);
}

void CNavTestIssueTracker::reportPathFailure(int botIndex, int sourceWpt, int destWpt, const std::string& reason)
{
	CNavTestIssue issue;
	issue.type = ENavTestIssueType::PATH_FAILURE;
	issue.severity = ENavTestIssueSeverity::MEDIUM;
	issue.botIndex = botIndex;
	issue.sourceWaypointId = sourceWpt;
	issue.destWaypointId = destWpt;
	issue.timestamp = gpGlobals->curtime;
	issue.serverGravity = CNavTestManager::instance().getServerGravity();
	issue.additionalInfo = reason;

	CWaypoint* pWpt = CWaypoints::getWaypoint(destWpt);
	if (pWpt != nullptr)
		issue.location = pWpt->getOrigin();

	reportIssue(issue);
}

void CNavTestIssueTracker::reportFallDamage(int botIndex, const Vector& location, int fromWpt, int toWpt, float damage, float gravity)
{
	CNavTestIssue issue;
	issue.type = ENavTestIssueType::FALL_DAMAGE;
	issue.botIndex = botIndex;
	issue.location = location;
	issue.sourceWaypointId = fromWpt;
	issue.destWaypointId = toWpt;
	issue.timestamp = gpGlobals->curtime;
	issue.serverGravity = gravity;

	// Severity based on damage amount
	if (damage >= 50.0f)
		issue.severity = ENavTestIssueSeverity::CRITICAL;
	else if (damage >= 25.0f)
		issue.severity = ENavTestIssueSeverity::HIGH;
	else if (damage >= 10.0f)
		issue.severity = ENavTestIssueSeverity::MEDIUM;
	else
		issue.severity = ENavTestIssueSeverity::LOW;

	char info[128];
	snprintf(info, sizeof(info), "Fall damage: %.1f (gravity: %.0f)", damage, gravity);
	issue.additionalInfo = info;

	reportIssue(issue);
}

void CNavTestIssueTracker::reportConnectionBroken(int botIndex, int fromWpt, int toWpt)
{
	CNavTestIssue issue;
	issue.type = ENavTestIssueType::CONNECTION_BROKEN;
	issue.severity = ENavTestIssueSeverity::HIGH;
	issue.botIndex = botIndex;
	issue.sourceWaypointId = fromWpt;
	issue.destWaypointId = toWpt;
	issue.timestamp = gpGlobals->curtime;
	issue.serverGravity = CNavTestManager::instance().getServerGravity();

	CWaypoint* pFromWpt = CWaypoints::getWaypoint(fromWpt);
	if (pFromWpt != nullptr)
		issue.location = pFromWpt->getOrigin();

	reportIssue(issue);
}

int CNavTestIssueTracker::getIssueCountByType(ENavTestIssueType type) const
{
	int count = 0;
	for (const CNavTestIssue& issue : m_issues)
	{
		if (issue.type == type)
			count++;
	}
	return count;
}

int CNavTestIssueTracker::getIssueCountBySeverity(ENavTestIssueSeverity severity) const
{
	int count = 0;
	for (const CNavTestIssue& issue : m_issues)
	{
		if (issue.severity == severity)
			count++;
	}
	return count;
}

std::vector<CNavTestIssue> CNavTestIssueTracker::getIssuesForWaypoint(int waypointId) const
{
	std::vector<CNavTestIssue> result;
	for (const CNavTestIssue& issue : m_issues)
	{
		if (issue.sourceWaypointId == waypointId || issue.destWaypointId == waypointId)
			result.push_back(issue);
	}
	return result;
}

bool CNavTestIssueTracker::waypointHasIssues(int waypointId) const
{
	for (const CNavTestIssue& issue : m_issues)
	{
		if (issue.sourceWaypointId == waypointId || issue.destWaypointId == waypointId)
			return true;
	}
	return false;
}

int CNavTestIssueTracker::getIssueFrequency(int waypointId) const
{
	int frequency = 0;
	for (const CNavTestIssue& issue : m_issues)
	{
		if (issue.sourceWaypointId == waypointId || issue.destWaypointId == waypointId)
			frequency += issue.occurrenceCount;
	}
	return frequency;
}

//=============================================================================
// CNavTestManager Implementation
//=============================================================================

CNavTestManager& CNavTestManager::instance()
{
	if (s_instance == nullptr)
		s_instance = new CNavTestManager();
	return *s_instance;
}

CNavTestManager::CNavTestManager()
	: m_sessionEndTime(0.0f)
	, m_lastUpdateTime(0.0f)
	, m_cachedGravity(600.0f)
{
}

CNavTestManager::~CNavTestManager()
{
}

bool CNavTestManager::startSession(float duration)
{
	if (m_currentSession.isActive)
	{
		CBotGlobals::botMessage(nullptr, 0, "Nav-test session already active. Stop it first.");
		return false;
	}

	// Initialize session
	m_currentSession = CNavTestSession();
	m_currentSession.sessionId = static_cast<int>(std::time(nullptr));
	m_currentSession.mapName = CBotGlobals::getMapName();
	m_currentSession.startTime = std::time(nullptr);
	m_currentSession.isActive = true;
	m_currentSession.totalWaypoints = CWaypoints::numWaypoints();

	// Set end time if duration specified
	if (duration > 0.0f)
		m_sessionEndTime = gpGlobals->curtime + duration;
	else
		m_sessionEndTime = 0.0f;

	// Initialize trackers
	m_coverageTracker.init(CWaypoints::numWaypoints());
	m_issueTracker.reset();

	// Cache gravity
	updateGravity();

	CBotGlobals::botMessage(nullptr, 0, "Nav-test session started on %s (%d waypoints)",
		m_currentSession.mapName.c_str(), m_currentSession.totalWaypoints);

	if (duration > 0.0f)
		CBotGlobals::botMessage(nullptr, 0, "Session will run for %.0f seconds", duration);

	return true;
}

void CNavTestManager::stopSession()
{
	if (!m_currentSession.isActive)
	{
		CBotGlobals::botMessage(nullptr, 0, "No active nav-test session.");
		return;
	}

	// Finalize session
	m_currentSession.endTime = std::time(nullptr);
	m_currentSession.duration = static_cast<float>(m_currentSession.endTime - m_currentSession.startTime);
	m_currentSession.visitedWaypoints = m_coverageTracker.getVisitedCount();
	m_currentSession.totalIssues = m_issueTracker.getIssueCount();
	m_currentSession.criticalIssues = m_issueTracker.getIssueCountBySeverity(ENavTestIssueSeverity::CRITICAL);
	m_currentSession.isActive = false;

	// Clear nav-test mode from all bots
	m_botsInNavTestMode.clear();

	// Print summary
	CBotGlobals::botMessage(nullptr, 0, "Nav-test session stopped.");
	CBotGlobals::botMessage(nullptr, 0, "Duration: %.1f seconds", m_currentSession.duration);
	CBotGlobals::botMessage(nullptr, 0, "Coverage: %d/%d waypoints (%.1f%%)",
		m_currentSession.visitedWaypoints,
		m_currentSession.totalWaypoints,
		m_coverageTracker.getCoveragePercent() * 100.0f);
	CBotGlobals::botMessage(nullptr, 0, "Issues found: %d (%d critical)",
		m_currentSession.totalIssues,
		m_currentSession.criticalIssues);
}

void CNavTestManager::update()
{
	if (!m_currentSession.isActive)
		return;

	float curTime = gpGlobals->curtime;

	// Check if session should end
	if (m_sessionEndTime > 0.0f && curTime >= m_sessionEndTime)
	{
		stopSession();
		return;
	}

	// Periodic updates (every 1 second)
	if (curTime - m_lastUpdateTime >= 1.0f)
	{
		m_lastUpdateTime = curTime;

		// Update cached gravity
		updateGravity();

		// Update session stats
		m_currentSession.visitedWaypoints = m_coverageTracker.getVisitedCount();
		m_currentSession.totalIssues = m_issueTracker.getIssueCount();
	}
}

void CNavTestManager::updateGravity()
{
	if (sv_gravity.IsValid())
		m_cachedGravity = sv_gravity.GetFloat();
	else
		m_cachedGravity = 600.0f;
}

void CNavTestManager::onBotStuck(CBot* pBot, float duration)
{
	if (!m_currentSession.isActive || pBot == nullptr)
		return;

	int botIndex = CBots::slotOfEdict(pBot->getEdict());
	if (!isBotInNavTestMode(pBot))
		return;

	// Find nearest waypoint to stuck location
	Vector botOrigin = pBot->getOrigin();
	int nearWpt = -1;
	float nearestDist = FLT_MAX;

	for (int i = 0; i < CWaypoints::numWaypoints(); i++)
	{
		CWaypoint* pWpt = CWaypoints::getWaypoint(i);
		if (pWpt == nullptr || !pWpt->isUsed())
			continue;

		float dist = (pWpt->getOrigin() - botOrigin).Length();
		if (dist < nearestDist)
		{
			nearestDist = dist;
			nearWpt = i;
		}
	}

	m_issueTracker.reportStuck(botIndex, botOrigin, nearWpt, duration);
}

void CNavTestManager::onBotPathFailure(CBot* pBot, int sourceWpt, int destWpt)
{
	if (!m_currentSession.isActive || pBot == nullptr)
		return;

	int botIndex = CBots::slotOfEdict(pBot->getEdict());
	if (!isBotInNavTestMode(pBot))
		return;

	m_issueTracker.reportPathFailure(botIndex, sourceWpt, destWpt, "Path navigation failed");
}

void CNavTestManager::onBotWaypointReached(CBot* pBot, int waypointId)
{
	if (!m_currentSession.isActive || pBot == nullptr)
		return;

	int botIndex = CBots::slotOfEdict(pBot->getEdict());
	if (!isBotInNavTestMode(pBot))
		return;

	m_coverageTracker.markVisited(waypointId, botIndex, gpGlobals->curtime);
}

void CNavTestManager::onBotFallDamage(CBot* pBot, float damage)
{
	if (!m_currentSession.isActive || pBot == nullptr)
		return;

	int botIndex = CBots::slotOfEdict(pBot->getEdict());
	if (!isBotInNavTestMode(pBot))
		return;

	Vector botOrigin = pBot->getOrigin();

	// Try to determine which waypoints were involved
	// This is approximate - we'd need to track the bot's path more closely for accuracy
	int nearWpt = -1;
	float nearestDist = FLT_MAX;

	for (int i = 0; i < CWaypoints::numWaypoints(); i++)
	{
		CWaypoint* pWpt = CWaypoints::getWaypoint(i);
		if (pWpt == nullptr || !pWpt->isUsed())
			continue;

		float dist = (pWpt->getOrigin() - botOrigin).Length();
		if (dist < nearestDist)
		{
			nearestDist = dist;
			nearWpt = i;
		}
	}

	m_issueTracker.reportFallDamage(botIndex, botOrigin, -1, nearWpt, damage, m_cachedGravity);
}

void CNavTestManager::onBotUnreachable(CBot* pBot, int sourceWpt, int destWpt)
{
	if (!m_currentSession.isActive || pBot == nullptr)
		return;

	int botIndex = CBots::slotOfEdict(pBot->getEdict());
	if (!isBotInNavTestMode(pBot))
		return;

	m_issueTracker.reportUnreachable(botIndex, sourceWpt, destWpt);
}

int CNavTestManager::getNextExplorationWaypoint(CBot* pBot)
{
	if (!m_currentSession.isActive || pBot == nullptr)
		return -1;

	// Priority 1: Unvisited waypoints
	int unvisited = m_coverageTracker.getUnvisitedWaypoint();
	if (unvisited >= 0)
		return unvisited;

	// Priority 2: Least recently visited waypoint
	return m_coverageTracker.getLeastRecentlyVisited();
}

bool CNavTestManager::isBotInNavTestMode(CBot* pBot) const
{
	if (pBot == nullptr)
		return false;

	int botIndex = CBots::slotOfEdict(pBot->getEdict());

	return std::find(m_botsInNavTestMode.begin(), m_botsInNavTestMode.end(), botIndex)
		!= m_botsInNavTestMode.end();
}

void CNavTestManager::setBotNavTestMode(CBot* pBot, bool enabled)
{
	if (pBot == nullptr)
		return;

	int botIndex = CBots::slotOfEdict(pBot->getEdict());

	auto it = std::find(m_botsInNavTestMode.begin(), m_botsInNavTestMode.end(), botIndex);

	if (enabled && it == m_botsInNavTestMode.end())
	{
		m_botsInNavTestMode.push_back(botIndex);
	}
	else if (!enabled && it != m_botsInNavTestMode.end())
	{
		m_botsInNavTestMode.erase(it);
	}
}

void CNavTestManager::generateReport(bool toConsole, const char* filename)
{
	if (!m_currentSession.isActive && m_currentSession.sessionId == 0)
	{
		if (toConsole)
			CBotGlobals::botMessage(nullptr, 0, "No nav-test data available.");
		return;
	}

	// Generate report content
	const CNavTestSession& session = m_currentSession;

	if (toConsole)
	{
		CBotGlobals::botMessage(nullptr, 0, "===== Nav-Test Report =====");
		CBotGlobals::botMessage(nullptr, 0, "Map: %s", session.mapName.c_str());
		CBotGlobals::botMessage(nullptr, 0, "Session ID: %d", session.sessionId);
		CBotGlobals::botMessage(nullptr, 0, "Status: %s", session.isActive ? "Active" : "Completed");
		CBotGlobals::botMessage(nullptr, 0, "Duration: %.1f seconds", session.duration);
		CBotGlobals::botMessage(nullptr, 0, "");
		CBotGlobals::botMessage(nullptr, 0, "--- Coverage ---");
		CBotGlobals::botMessage(nullptr, 0, "Waypoints visited: %d/%d (%.1f%%)",
			m_coverageTracker.getVisitedCount(),
			session.totalWaypoints,
			m_coverageTracker.getCoveragePercent() * 100.0f);
		CBotGlobals::botMessage(nullptr, 0, "");
		CBotGlobals::botMessage(nullptr, 0, "--- Issues Summary ---");
		CBotGlobals::botMessage(nullptr, 0, "Total issues: %d", m_issueTracker.getIssueCount());
		CBotGlobals::botMessage(nullptr, 0, "  Critical: %d", m_issueTracker.getIssueCountBySeverity(ENavTestIssueSeverity::CRITICAL));
		CBotGlobals::botMessage(nullptr, 0, "  High: %d", m_issueTracker.getIssueCountBySeverity(ENavTestIssueSeverity::HIGH));
		CBotGlobals::botMessage(nullptr, 0, "  Medium: %d", m_issueTracker.getIssueCountBySeverity(ENavTestIssueSeverity::MEDIUM));
		CBotGlobals::botMessage(nullptr, 0, "  Low: %d", m_issueTracker.getIssueCountBySeverity(ENavTestIssueSeverity::LOW));
		CBotGlobals::botMessage(nullptr, 0, "");
		CBotGlobals::botMessage(nullptr, 0, "--- Issues by Type ---");
		CBotGlobals::botMessage(nullptr, 0, "  Stuck: %d", m_issueTracker.getIssueCountByType(ENavTestIssueType::STUCK));
		CBotGlobals::botMessage(nullptr, 0, "  Unreachable: %d", m_issueTracker.getIssueCountByType(ENavTestIssueType::UNREACHABLE));
		CBotGlobals::botMessage(nullptr, 0, "  Path Failures: %d", m_issueTracker.getIssueCountByType(ENavTestIssueType::PATH_FAILURE));
		CBotGlobals::botMessage(nullptr, 0, "  Fall Damage: %d", m_issueTracker.getIssueCountByType(ENavTestIssueType::FALL_DAMAGE));
		CBotGlobals::botMessage(nullptr, 0, "  Connection Broken: %d", m_issueTracker.getIssueCountByType(ENavTestIssueType::CONNECTION_BROKEN));
		CBotGlobals::botMessage(nullptr, 0, "===========================");
	}

	// TODO: File output support for SQLite integration
	if (filename != nullptr)
	{
		CBotGlobals::botMessage(nullptr, 0, "File output not yet implemented.");
	}
}

//=============================================================================
// Console Command Implementations
//=============================================================================

void NavTest_StartCommand(const CCommand& args)
{
	float duration = 0.0f;

	if (args.ArgC() >= 2)
	{
		duration = static_cast<float>(std::atof(args.Arg(1)));
	}

	CNavTestManager::instance().startSession(duration);
}

void NavTest_StopCommand(const CCommand& args)
{
	CNavTestManager::instance().stopSession();
}

void NavTest_StatusCommand(const CCommand& args)
{
	CNavTestManager& manager = CNavTestManager::instance();
	const CNavTestSession& session = manager.getCurrentSession();

	if (!session.isActive)
	{
		CBotGlobals::botMessage(nullptr, 0, "No active nav-test session.");
		return;
	}

	float elapsed = static_cast<float>(std::time(nullptr) - session.startTime);

	CBotGlobals::botMessage(nullptr, 0, "Nav-test Status:");
	CBotGlobals::botMessage(nullptr, 0, "  Map: %s", session.mapName.c_str());
	CBotGlobals::botMessage(nullptr, 0, "  Elapsed: %.1f seconds", elapsed);
	CBotGlobals::botMessage(nullptr, 0, "  Coverage: %d/%d (%.1f%%)",
		manager.getCoverageTracker().getVisitedCount(),
		session.totalWaypoints,
		manager.getCoverageTracker().getCoveragePercent() * 100.0f);
	CBotGlobals::botMessage(nullptr, 0, "  Issues: %d", manager.getIssueTracker().getIssueCount());
}

void NavTest_ReportCommand(const CCommand& args)
{
	const char* filename = nullptr;

	if (args.ArgC() >= 2)
	{
		filename = args.Arg(1);
	}

	CNavTestManager::instance().generateReport(true, filename);
}
