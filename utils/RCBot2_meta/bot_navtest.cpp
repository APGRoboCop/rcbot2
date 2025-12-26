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
#include "bot_tactical.h"
#include "bot.h"
#include "bot_globals.h"
#include "bot_waypoint.h"
#include "bot_cvars.h"
#include "bot_schedule.h"

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

int CMapCoverageTracker::getLeastRecentlyVisited(const Vector& fromOrigin) const
{
	if (m_numWaypoints == 0)
		return -1;

	// Find LRU waypoints and prefer ones closer to bot
	int lruWaypoint = -1;
	float oldestTime = FLT_MAX;
	float closestDistForOldest = FLT_MAX;

	for (int i = 0; i < m_numWaypoints; i++)
	{
		const WaypointVisitInfo& info = m_waypointInfo[i];

		CWaypoint* pWpt = CWaypoints::getWaypoint(i);
		if (pWpt == nullptr || !pWpt->isUsed())
			continue;

		if (info.visitCount > 0)
		{
			float dist = (pWpt->getOrigin() - fromOrigin).Length();

			// Prefer older visits, but if similar age, prefer closer
			if (info.lastVisitTime < oldestTime - 1.0f ||
			    (fabs(info.lastVisitTime - oldestTime) < 1.0f && dist < closestDistForOldest))
			{
				oldestTime = info.lastVisitTime;
				closestDistForOldest = dist;
				lruWaypoint = i;
			}
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

int CMapCoverageTracker::getUnvisitedWaypoint(const Vector& fromOrigin) const
{
	if (m_numWaypoints == 0)
		return -1;

	// Find closest unvisited waypoint to the given origin
	int closestUnvisited = -1;
	float closestDist = FLT_MAX;

	for (int i = 0; i < m_numWaypoints; i++)
	{
		CWaypoint* pWpt = CWaypoints::getWaypoint(i);
		if (pWpt == nullptr || !pWpt->isUsed())
			continue;

		if (m_waypointInfo[i].visitCount == 0)
		{
			float dist = (pWpt->getOrigin() - fromOrigin).Length();
			if (dist < closestDist)
			{
				closestDist = dist;
				closestUnvisited = i;
			}
		}
	}

	return closestUnvisited;
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

void CNavTestIssueTracker::reportUnreachable(CBot* pBot, int sourceWpt, int destWpt, const Vector& location)
{
	int botIndex = -1;
	if (pBot != nullptr)
		botIndex = CBots::slotOfEdict(pBot->getEdict());

	CNavTestIssue issue;
	issue.type = ENavTestIssueType::UNREACHABLE;
	issue.severity = ENavTestIssueSeverity::HIGH;
	issue.botIndex = botIndex;
	issue.sourceWaypointId = sourceWpt;
	issue.destWaypointId = destWpt;
	issue.location = location;
	issue.timestamp = gpGlobals->curtime;
	issue.serverGravity = CNavTestManager::instance().getServerGravity();

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

		// Re-schedule bots in nav-test mode that have empty schedules
		for (int i = 0; i < RCBOT_MAXPLAYERS; i++)
		{
			CBot* pBot = CBots::getBot(i);
			if (pBot == nullptr || !pBot->inUse() || !pBot->isNavTestMode())
				continue;

			if (!pBot->isAlive())
				continue;

			CBotSchedules* pSchedules = pBot->getSchedule();
			if (pSchedules == nullptr)
				continue;

			// If bot has no current schedule or nav-test schedule completed, give it a new one
			if (pSchedules->isEmpty() || !pSchedules->isCurrentSchedule(SCHED_NAVTEST_EXPLORE))
			{
				pSchedules->freeMemory();
				pSchedules->add(new CNavTestExploreSched(-1));
			}
		}
	}

	// Update tactical data integration (every 10 seconds)
	static float s_lastTacticalUpdate = 0.0f;
	if (curTime - s_lastTacticalUpdate >= 10.0f)
	{
		s_lastTacticalUpdate = curTime;

		CTacticalDataManager& tactical = CTacticalDataManager::instance();
		tactical.updateTrafficFromNavTest();
		tactical.updateDangerFromNavTest();
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

//=============================================================================
// CNavTestDatabase Implementation
//=============================================================================

#include <fstream>
#include <sstream>
#include <map>

CNavTestDatabase::CNavTestDatabase()
	: m_bIsOpen(false)
	, m_bCacheValid(false)
{
}

CNavTestDatabase::~CNavTestDatabase()
{
	closeDatabase();
}

bool CNavTestDatabase::openDatabase(const char* mapName)
{
	if (mapName == nullptr || mapName[0] == '\0')
		return false;

	m_mapName = mapName;

	// Build database path
	char path[512];
	CBotGlobals::buildFileName(path, "navtest", mapName, "", true);
	m_databasePath = path;

	// Create directory if it doesn't exist
	// The path will be used as a prefix for session files

	m_bIsOpen = true;
	m_bCacheValid = false;

	return true;
}

void CNavTestDatabase::closeDatabase()
{
	m_bIsOpen = false;
	m_bCacheValid = false;
	m_cachedSessions.clear();
	m_cachedIssues.clear();
}

std::string CNavTestDatabase::getSessionFilePath(int sessionId)
{
	char filename[512];
	snprintf(filename, sizeof(filename), "%s_session_%d.bin", m_databasePath.c_str(), sessionId);
	return std::string(filename);
}

std::string CNavTestDatabase::getIssuesFilePath(int sessionId)
{
	char filename[512];
	snprintf(filename, sizeof(filename), "%s_issues_%d.bin", m_databasePath.c_str(), sessionId);
	return std::string(filename);
}

std::string CNavTestDatabase::getCoverageFilePath(int sessionId)
{
	char filename[512];
	snprintf(filename, sizeof(filename), "%s_coverage_%d.bin", m_databasePath.c_str(), sessionId);
	return std::string(filename);
}

bool CNavTestDatabase::saveSession(const CNavTestSession& session)
{
	if (!m_bIsOpen)
		return false;

	std::string filepath = getSessionFilePath(session.sessionId);
	std::ofstream file(filepath, std::ios::binary);
	if (!file.is_open())
		return false;

	// Write session data
	file.write(reinterpret_cast<const char*>(&session.sessionId), sizeof(session.sessionId));

	// Write map name (length + data)
	uint32_t mapLen = static_cast<uint32_t>(session.mapName.length());
	file.write(reinterpret_cast<const char*>(&mapLen), sizeof(mapLen));
	file.write(session.mapName.c_str(), mapLen);

	file.write(reinterpret_cast<const char*>(&session.startTime), sizeof(session.startTime));
	file.write(reinterpret_cast<const char*>(&session.endTime), sizeof(session.endTime));
	file.write(reinterpret_cast<const char*>(&session.duration), sizeof(session.duration));
	file.write(reinterpret_cast<const char*>(&session.totalWaypoints), sizeof(session.totalWaypoints));
	file.write(reinterpret_cast<const char*>(&session.visitedWaypoints), sizeof(session.visitedWaypoints));
	file.write(reinterpret_cast<const char*>(&session.totalIssues), sizeof(session.totalIssues));
	file.write(reinterpret_cast<const char*>(&session.criticalIssues), sizeof(session.criticalIssues));

	file.close();
	invalidateCache();
	return true;
}

bool CNavTestDatabase::loadSession(int sessionId, CNavTestSession& session)
{
	if (!m_bIsOpen)
		return false;

	std::string filepath = getSessionFilePath(sessionId);
	std::ifstream file(filepath, std::ios::binary);
	if (!file.is_open())
		return false;

	file.read(reinterpret_cast<char*>(&session.sessionId), sizeof(session.sessionId));

	uint32_t mapLen = 0;
	file.read(reinterpret_cast<char*>(&mapLen), sizeof(mapLen));
	if (mapLen > 0 && mapLen < 256)
	{
		char mapName[256];
		file.read(mapName, mapLen);
		mapName[mapLen] = '\0';
		session.mapName = mapName;
	}

	file.read(reinterpret_cast<char*>(&session.startTime), sizeof(session.startTime));
	file.read(reinterpret_cast<char*>(&session.endTime), sizeof(session.endTime));
	file.read(reinterpret_cast<char*>(&session.duration), sizeof(session.duration));
	file.read(reinterpret_cast<char*>(&session.totalWaypoints), sizeof(session.totalWaypoints));
	file.read(reinterpret_cast<char*>(&session.visitedWaypoints), sizeof(session.visitedWaypoints));
	file.read(reinterpret_cast<char*>(&session.totalIssues), sizeof(session.totalIssues));
	file.read(reinterpret_cast<char*>(&session.criticalIssues), sizeof(session.criticalIssues));

	session.isActive = false; // Loaded sessions are never active

	file.close();
	return true;
}

int CNavTestDatabase::getNextSessionId()
{
	return static_cast<int>(std::time(nullptr));
}

bool CNavTestDatabase::saveIssues(const std::vector<CNavTestIssue>& issues, int sessionId)
{
	if (!m_bIsOpen)
		return false;

	std::string filepath = getIssuesFilePath(sessionId);
	std::ofstream file(filepath, std::ios::binary);
	if (!file.is_open())
		return false;

	uint32_t count = static_cast<uint32_t>(issues.size());
	file.write(reinterpret_cast<const char*>(&count), sizeof(count));

	for (const CNavTestIssue& issue : issues)
	{
		file.write(reinterpret_cast<const char*>(&issue.type), sizeof(issue.type));
		file.write(reinterpret_cast<const char*>(&issue.severity), sizeof(issue.severity));
		file.write(reinterpret_cast<const char*>(&issue.location.x), sizeof(float));
		file.write(reinterpret_cast<const char*>(&issue.location.y), sizeof(float));
		file.write(reinterpret_cast<const char*>(&issue.location.z), sizeof(float));
		file.write(reinterpret_cast<const char*>(&issue.sourceWaypointId), sizeof(issue.sourceWaypointId));
		file.write(reinterpret_cast<const char*>(&issue.destWaypointId), sizeof(issue.destWaypointId));
		file.write(reinterpret_cast<const char*>(&issue.connectionId), sizeof(issue.connectionId));
		file.write(reinterpret_cast<const char*>(&issue.timestamp), sizeof(issue.timestamp));
		file.write(reinterpret_cast<const char*>(&issue.botIndex), sizeof(issue.botIndex));
		file.write(reinterpret_cast<const char*>(&issue.occurrenceCount), sizeof(issue.occurrenceCount));
		file.write(reinterpret_cast<const char*>(&issue.serverGravity), sizeof(issue.serverGravity));

		uint32_t infoLen = static_cast<uint32_t>(issue.additionalInfo.length());
		file.write(reinterpret_cast<const char*>(&infoLen), sizeof(infoLen));
		if (infoLen > 0)
			file.write(issue.additionalInfo.c_str(), infoLen);
	}

	file.close();
	invalidateCache();
	return true;
}

bool CNavTestDatabase::loadIssues(int sessionId, std::vector<CNavTestIssue>& issues)
{
	if (!m_bIsOpen)
		return false;

	std::string filepath = getIssuesFilePath(sessionId);
	std::ifstream file(filepath, std::ios::binary);
	if (!file.is_open())
		return false;

	issues.clear();

	uint32_t count = 0;
	file.read(reinterpret_cast<char*>(&count), sizeof(count));

	for (uint32_t i = 0; i < count; i++)
	{
		CNavTestIssue issue;

		file.read(reinterpret_cast<char*>(&issue.type), sizeof(issue.type));
		file.read(reinterpret_cast<char*>(&issue.severity), sizeof(issue.severity));
		file.read(reinterpret_cast<char*>(&issue.location.x), sizeof(float));
		file.read(reinterpret_cast<char*>(&issue.location.y), sizeof(float));
		file.read(reinterpret_cast<char*>(&issue.location.z), sizeof(float));
		file.read(reinterpret_cast<char*>(&issue.sourceWaypointId), sizeof(issue.sourceWaypointId));
		file.read(reinterpret_cast<char*>(&issue.destWaypointId), sizeof(issue.destWaypointId));
		file.read(reinterpret_cast<char*>(&issue.connectionId), sizeof(issue.connectionId));
		file.read(reinterpret_cast<char*>(&issue.timestamp), sizeof(issue.timestamp));
		file.read(reinterpret_cast<char*>(&issue.botIndex), sizeof(issue.botIndex));
		file.read(reinterpret_cast<char*>(&issue.occurrenceCount), sizeof(issue.occurrenceCount));
		file.read(reinterpret_cast<char*>(&issue.serverGravity), sizeof(issue.serverGravity));

		uint32_t infoLen = 0;
		file.read(reinterpret_cast<char*>(&infoLen), sizeof(infoLen));
		if (infoLen > 0 && infoLen < 1024)
		{
			char info[1024];
			file.read(info, infoLen);
			info[infoLen] = '\0';
			issue.additionalInfo = info;
		}

		issues.push_back(issue);
	}

	file.close();
	return true;
}

bool CNavTestDatabase::appendIssue(const CNavTestIssue& issue, int sessionId)
{
	std::vector<CNavTestIssue> issues;
	loadIssues(sessionId, issues);
	issues.push_back(issue);
	return saveIssues(issues, sessionId);
}

bool CNavTestDatabase::saveCoverage(const std::vector<CNavTestWaypointVisit>& visits, int sessionId)
{
	if (!m_bIsOpen)
		return false;

	std::string filepath = getCoverageFilePath(sessionId);
	std::ofstream file(filepath, std::ios::binary);
	if (!file.is_open())
		return false;

	uint32_t count = static_cast<uint32_t>(visits.size());
	file.write(reinterpret_cast<const char*>(&count), sizeof(count));

	for (const CNavTestWaypointVisit& visit : visits)
	{
		file.write(reinterpret_cast<const char*>(&visit.waypointId), sizeof(visit.waypointId));
		file.write(reinterpret_cast<const char*>(&visit.botIndex), sizeof(visit.botIndex));
		file.write(reinterpret_cast<const char*>(&visit.timestamp), sizeof(visit.timestamp));
		file.write(reinterpret_cast<const char*>(&visit.timeSpentNearby), sizeof(visit.timeSpentNearby));
		file.write(reinterpret_cast<const char*>(&visit.wasDestination), sizeof(visit.wasDestination));
		file.write(reinterpret_cast<const char*>(&visit.reachedSuccessfully), sizeof(visit.reachedSuccessfully));
	}

	file.close();
	return true;
}

bool CNavTestDatabase::loadCoverage(int sessionId, std::vector<CNavTestWaypointVisit>& visits)
{
	if (!m_bIsOpen)
		return false;

	std::string filepath = getCoverageFilePath(sessionId);
	std::ifstream file(filepath, std::ios::binary);
	if (!file.is_open())
		return false;

	visits.clear();

	uint32_t count = 0;
	file.read(reinterpret_cast<char*>(&count), sizeof(count));

	for (uint32_t i = 0; i < count; i++)
	{
		CNavTestWaypointVisit visit;

		file.read(reinterpret_cast<char*>(&visit.waypointId), sizeof(visit.waypointId));
		file.read(reinterpret_cast<char*>(&visit.botIndex), sizeof(visit.botIndex));
		file.read(reinterpret_cast<char*>(&visit.timestamp), sizeof(visit.timestamp));
		file.read(reinterpret_cast<char*>(&visit.timeSpentNearby), sizeof(visit.timeSpentNearby));
		file.read(reinterpret_cast<char*>(&visit.wasDestination), sizeof(visit.wasDestination));
		file.read(reinterpret_cast<char*>(&visit.reachedSuccessfully), sizeof(visit.reachedSuccessfully));

		visits.push_back(visit);
	}

	file.close();
	return true;
}

void CNavTestDatabase::invalidateCache()
{
	m_bCacheValid = false;
	m_cachedSessions.clear();
	m_cachedIssues.clear();
}

void CNavTestDatabase::rebuildCache()
{
	if (m_bCacheValid)
		return;

	// For now, just mark cache as valid
	// Full cache rebuild would scan for session files
	m_bCacheValid = true;
}

std::vector<CNavTestIssue> CNavTestDatabase::getIssuesByType(ENavTestIssueType type)
{
	std::vector<CNavTestIssue> result;

	rebuildCache();

	for (const CNavTestIssue& issue : m_cachedIssues)
	{
		if (issue.type == type)
			result.push_back(issue);
	}

	return result;
}

std::vector<CNavTestIssue> CNavTestDatabase::getIssuesByWaypoint(int waypointId)
{
	std::vector<CNavTestIssue> result;

	rebuildCache();

	for (const CNavTestIssue& issue : m_cachedIssues)
	{
		if (issue.sourceWaypointId == waypointId || issue.destWaypointId == waypointId)
			result.push_back(issue);
	}

	return result;
}

std::vector<CNavTestIssue> CNavTestDatabase::getIssuesBySeverity(ENavTestIssueSeverity severity)
{
	std::vector<CNavTestIssue> result;

	rebuildCache();

	for (const CNavTestIssue& issue : m_cachedIssues)
	{
		if (issue.severity == severity)
			result.push_back(issue);
	}

	return result;
}

int CNavTestDatabase::getIssueFrequency(int waypointId)
{
	int frequency = 0;

	rebuildCache();

	for (const CNavTestIssue& issue : m_cachedIssues)
	{
		if (issue.sourceWaypointId == waypointId || issue.destWaypointId == waypointId)
			frequency += issue.occurrenceCount;
	}

	return frequency;
}

CNavTestDatabase::AggregateStats CNavTestDatabase::getAggregateStats()
{
	AggregateStats stats = {};

	rebuildCache();

	stats.totalSessions = static_cast<int>(m_cachedSessions.size());
	stats.totalIssues = static_cast<int>(m_cachedIssues.size());

	// Count unique waypoints with issues
	std::map<int, int> waypointIssueCount;
	for (const CNavTestIssue& issue : m_cachedIssues)
	{
		if (issue.sourceWaypointId >= 0)
			waypointIssueCount[issue.sourceWaypointId] += issue.occurrenceCount;
		if (issue.destWaypointId >= 0)
			waypointIssueCount[issue.destWaypointId] += issue.occurrenceCount;
	}

	stats.uniqueIssueWaypoints = static_cast<int>(waypointIssueCount.size());

	// Find most problematic waypoint
	stats.mostProblematicWaypoint = -1;
	stats.mostProblematicWaypointIssues = 0;

	for (const auto& pair : waypointIssueCount)
	{
		if (pair.second > stats.mostProblematicWaypointIssues)
		{
			stats.mostProblematicWaypoint = pair.first;
			stats.mostProblematicWaypointIssues = pair.second;
		}
	}

	// Calculate average coverage
	float totalCoverage = 0.0f;
	for (const CNavTestSession& session : m_cachedSessions)
	{
		if (session.totalWaypoints > 0)
			totalCoverage += static_cast<float>(session.visitedWaypoints) / static_cast<float>(session.totalWaypoints);
	}
	if (stats.totalSessions > 0)
		stats.avgCoverage = totalCoverage / static_cast<float>(stats.totalSessions);

	return stats;
}

std::string CNavTestDatabase::generateReportString()
{
	std::ostringstream report;

	AggregateStats stats = getAggregateStats();

	report << "===== Nav-Test Database Report =====\n";
	report << "Map: " << m_mapName << "\n\n";
	report << "--- Aggregate Statistics ---\n";
	report << "Total sessions: " << stats.totalSessions << "\n";
	report << "Total issues: " << stats.totalIssues << "\n";
	report << "Unique problem waypoints: " << stats.uniqueIssueWaypoints << "\n";
	report << "Average coverage: " << (stats.avgCoverage * 100.0f) << "%\n";

	if (stats.mostProblematicWaypoint >= 0)
	{
		report << "\nMost problematic waypoint: #" << stats.mostProblematicWaypoint;
		report << " (" << stats.mostProblematicWaypointIssues << " issues)\n";
	}

	// Count issues by type for recommendations
	int stuckCount = 0, unreachableCount = 0, pathFailCount = 0;
	int fallDamageCount = 0, connectionBrokenCount = 0;

	report << "\n--- Issues by Type ---\n";
	for (int t = 0; t < static_cast<int>(ENavTestIssueType::MAX_ISSUE_TYPES); t++)
	{
		ENavTestIssueType type = static_cast<ENavTestIssueType>(t);
		std::vector<CNavTestIssue> issues = getIssuesByType(type);
		if (!issues.empty())
		{
			const char* typeName = "Unknown";
			switch (type)
			{
				case ENavTestIssueType::STUCK:
					typeName = "Stuck";
					stuckCount = static_cast<int>(issues.size());
					break;
				case ENavTestIssueType::UNREACHABLE:
					typeName = "Unreachable";
					unreachableCount = static_cast<int>(issues.size());
					break;
				case ENavTestIssueType::PATH_FAILURE:
					typeName = "Path Failure";
					pathFailCount = static_cast<int>(issues.size());
					break;
				case ENavTestIssueType::FALL_DAMAGE:
					typeName = "Fall Damage";
					fallDamageCount = static_cast<int>(issues.size());
					break;
				case ENavTestIssueType::CONNECTION_BROKEN:
					typeName = "Connection Broken";
					connectionBrokenCount = static_cast<int>(issues.size());
					break;
				case ENavTestIssueType::OUT_OF_BOUNDS: typeName = "Out of Bounds"; break;
				case ENavTestIssueType::SLOW_TRAVERSE: typeName = "Slow Traverse"; break;
				default: break;
			}
			report << "  " << typeName << ": " << issues.size() << "\n";
		}
	}

	// Generate actionable recommendations
	report << "\n--- Recommendations ---\n";

	bool hasRecommendations = false;

	if (stats.avgCoverage < 0.8f)
	{
		report << "* LOW COVERAGE: Only " << (stats.avgCoverage * 100.0f) << "% of waypoints visited.\n";
		report << "  -> Add more waypoints to improve map coverage\n";
		report << "  -> Check for isolated waypoint clusters with no connections\n";
		hasRecommendations = true;
	}

	if (stuckCount > 0)
	{
		report << "* STUCK ISSUES (" << stuckCount << "): Bots getting stuck at certain locations.\n";
		report << "  -> Check waypoint placement near obstacles, stairs, or tight spaces\n";
		report << "  -> Consider adding jump or crouch flags to problematic waypoints\n";
		report << "  -> Use 'rcbot refine analyze' to identify specific stuck locations\n";
		hasRecommendations = true;
	}

	if (unreachableCount > 0)
	{
		report << "* UNREACHABLE WAYPOINTS (" << unreachableCount << "): Some waypoints cannot be reached.\n";
		report << "  -> Check waypoint connections - may need additional paths\n";
		report << "  -> Verify waypoints aren't placed in inaccessible areas\n";
		report << "  -> Look for one-way drops that need return paths\n";
		hasRecommendations = true;
	}

	if (pathFailCount > 0)
	{
		report << "* PATH FAILURES (" << pathFailCount << "): Bots abandoning paths before completion.\n";
		report << "  -> Connections may be blocked by dynamic obstacles\n";
		report << "  -> Check for doors that need door waypoint flags\n";
		report << "  -> Verify path distances aren't too long without intermediate waypoints\n";
		hasRecommendations = true;
	}

	if (fallDamageCount > 0)
	{
		report << "* FALL DAMAGE (" << fallDamageCount << "): Bots taking fall damage on routes.\n";
		report << "  -> Use 'rcbot gravity info' to check current gravity settings\n";
		report << "  -> Add intermediate waypoints on tall drops\n";
		report << "  -> Consider adding ladder or jump waypoints for safer routes\n";
		report << "  -> Mark dangerous connections with appropriate flags\n";
		hasRecommendations = true;
	}

	if (connectionBrokenCount > 0)
	{
		report << "* BROKEN CONNECTIONS (" << connectionBrokenCount << "): Waypoint connections not traversable.\n";
		report << "  -> Remove or repair broken connections\n";
		report << "  -> Check for geometry changes since waypoints were created\n";
		report << "  -> Use 'rcbot refine autorefine' to automatically repair issues\n";
		hasRecommendations = true;
	}

	if (stats.mostProblematicWaypoint >= 0 && stats.mostProblematicWaypointIssues >= 3)
	{
		report << "* HOTSPOT WAYPOINT #" << stats.mostProblematicWaypoint << ": ";
		report << stats.mostProblematicWaypointIssues << " issues at this location.\n";
		report << "  -> Priority: Investigate and fix this waypoint first\n";
		report << "  -> Consider relocating or removing this waypoint\n";
		hasRecommendations = true;
	}

	if (!hasRecommendations)
	{
		report << "No significant issues detected. Waypoint network appears healthy.\n";
	}

	report << "\n====================================\n";

	return report.str();
}

bool CNavTestDatabase::generateReport(const char* filename)
{
	if (filename == nullptr)
		return false;

	std::string report = generateReportString();

	std::ofstream file(filename);
	if (!file.is_open())
		return false;

	file << report;
	file.close();

	CBotGlobals::botMessage(nullptr, 0, "Nav-test report saved to %s", filename);
	return true;
}

bool CNavTestDatabase::generateHTMLReport(const char* filename)
{
	if (filename == nullptr)
		return false;

	AggregateStats stats = getAggregateStats();

	std::ofstream file(filename);
	if (!file.is_open())
		return false;

	file << "<!DOCTYPE html>\n<html>\n<head>\n";
	file << "<title>Nav-Test Report - " << m_mapName << "</title>\n";
	file << "<style>\n";
	file << "body { font-family: Arial, sans-serif; margin: 20px; }\n";
	file << "h1 { color: #333; }\n";
	file << "table { border-collapse: collapse; width: 100%; }\n";
	file << "th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }\n";
	file << "th { background-color: #4CAF50; color: white; }\n";
	file << "tr:nth-child(even) { background-color: #f2f2f2; }\n";
	file << ".critical { color: red; font-weight: bold; }\n";
	file << ".high { color: orange; }\n";
	file << ".medium { color: #cc9900; }\n";
	file << ".low { color: green; }\n";
	file << "</style>\n</head>\n<body>\n";

	file << "<h1>Nav-Test Report: " << m_mapName << "</h1>\n";

	file << "<h2>Summary</h2>\n";
	file << "<ul>\n";
	file << "<li>Total Sessions: " << stats.totalSessions << "</li>\n";
	file << "<li>Total Issues: " << stats.totalIssues << "</li>\n";
	file << "<li>Problem Waypoints: " << stats.uniqueIssueWaypoints << "</li>\n";
	file << "<li>Average Coverage: " << (stats.avgCoverage * 100.0f) << "%</li>\n";
	file << "</ul>\n";

	file << "</body>\n</html>\n";
	file.close();

	CBotGlobals::botMessage(nullptr, 0, "Nav-test HTML report saved to %s", filename);
	return true;
}

bool CNavTestDatabase::deleteSession(int sessionId)
{
	if (!m_bIsOpen)
		return false;

	std::remove(getSessionFilePath(sessionId).c_str());
	std::remove(getIssuesFilePath(sessionId).c_str());
	std::remove(getCoverageFilePath(sessionId).c_str());

	invalidateCache();
	return true;
}

bool CNavTestDatabase::clearAllData()
{
	invalidateCache();
	return true;
}

std::vector<CNavTestSession> CNavTestDatabase::getAllSessions()
{
	rebuildCache();
	return m_cachedSessions;
}

//=============================================================================
// Save/Load Command Implementations
//=============================================================================

void NavTest_SaveCommand(const CCommand& args)
{
	CNavTestManager& manager = CNavTestManager::instance();
	const CNavTestSession& session = manager.getCurrentSession();

	if (session.sessionId == 0)
	{
		CBotGlobals::botMessage(nullptr, 0, "No session data to save.");
		return;
	}

	CNavTestDatabase db;
	if (!db.openDatabase(session.mapName.c_str()))
	{
		CBotGlobals::botMessage(nullptr, 0, "Failed to open database.");
		return;
	}

	if (db.saveSession(session) &&
	    db.saveIssues(manager.getIssueTracker().getAllIssues(), session.sessionId) &&
	    db.saveCoverage(manager.getCoverageTracker().getVisits(), session.sessionId))
	{
		CBotGlobals::botMessage(nullptr, 0, "Nav-test data saved (session %d)", session.sessionId);
	}
	else
	{
		CBotGlobals::botMessage(nullptr, 0, "Failed to save nav-test data.");
	}
}

void NavTest_LoadCommand(const CCommand& args)
{
	if (args.ArgC() < 2)
	{
		CBotGlobals::botMessage(nullptr, 0, "Usage: rcbot navtest load <session_id>");
		return;
	}

	int sessionId = atoi(args.Arg(1));

	CNavTestDatabase db;
	if (!db.openDatabase(CBotGlobals::getMapName()))
	{
		CBotGlobals::botMessage(nullptr, 0, "Failed to open database.");
		return;
	}

	CNavTestSession session;
	if (db.loadSession(sessionId, session))
	{
		CBotGlobals::botMessage(nullptr, 0, "Loaded session %d from %s",
			session.sessionId, session.mapName.c_str());
		CBotGlobals::botMessage(nullptr, 0, "  Duration: %.1f seconds", session.duration);
		CBotGlobals::botMessage(nullptr, 0, "  Coverage: %d/%d waypoints",
			session.visitedWaypoints, session.totalWaypoints);
		CBotGlobals::botMessage(nullptr, 0, "  Issues: %d (%d critical)",
			session.totalIssues, session.criticalIssues);
	}
	else
	{
		CBotGlobals::botMessage(nullptr, 0, "Failed to load session %d", sessionId);
	}
}
