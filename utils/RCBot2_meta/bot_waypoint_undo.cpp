// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
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

#include "bot_waypoint_undo.h"
#include "bot_waypoint.h"
#include "bot_waypoint_locations.h"
#include "bot_globals.h"
#include "rcbot/logging.h"

#include <algorithm>

///////////////////////////////////////////////////////////////////////////////
// CWaypointOpAdd - Add waypoint operation
///////////////////////////////////////////////////////////////////////////////

CWaypointOpAdd::CWaypointOpAdd(int iWaypointIndex, const Vector& vOrigin, int iFlags, int iYaw, float fRadius)
	: m_iWaypointIndex(iWaypointIndex)
	, m_vOrigin(vOrigin)
	, m_iFlags(iFlags)
	, m_iYaw(iYaw)
	, m_fRadius(fRadius)
{
	// Store paths that were auto-created when waypoint was added
	CWaypoint* pWpt = CWaypoints::getWaypoint(iWaypointIndex);
	if (pWpt)
	{
		for (int i = 0; i < pWpt->numPaths(); i++)
		{
			m_Paths.push_back(pWpt->getPath(i));
		}
	}
}

void CWaypointOpAdd::undo()
{
	// Delete the waypoint that was added
	CWaypoints::deleteWaypoint(m_iWaypointIndex);
}

void CWaypointOpAdd::redo()
{
	// Re-add the waypoint
	const int iNewIndex = CWaypoints::addWaypoint(nullptr, m_vOrigin, m_iFlags, false, static_cast<int>(m_iYaw), 0, m_fRadius);

	if (iNewIndex != m_iWaypointIndex)
	{
		logger->Log(LogLevel::WARN, "Redo add waypoint: index mismatch (expected %d, got %d)", m_iWaypointIndex, iNewIndex);
	}

	// Restore auto-created paths
	CWaypoint* pWpt = CWaypoints::getWaypoint(iNewIndex);
	if (pWpt)
	{
		for (const int iPathIndex : m_Paths)
		{
			pWpt->addPathTo(iPathIndex);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// CWaypointOpDelete - Delete waypoint operation
///////////////////////////////////////////////////////////////////////////////

CWaypointOpDelete::CWaypointOpDelete(int iWaypointIndex)
	: m_iWaypointIndex(iWaypointIndex)
{
	// Store waypoint data before deletion
	CWaypoint* pWpt = CWaypoints::getWaypoint(iWaypointIndex);
	if (pWpt)
	{
		m_vOrigin = pWpt->getOrigin();
		m_iFlags = pWpt->getFlags();
		m_iYaw = static_cast<int>(pWpt->getAimYaw());
		m_fRadius = pWpt->getRadius();

		// Store all paths
		for (int i = 0; i < pWpt->numPaths(); i++)
		{
			m_PathsFrom.push_back(pWpt->getPath(i));
		}

		// Store paths to this waypoint
		for (int i = 0; i < pWpt->numPathsToThisWaypoint(); i++)
		{
			m_PathsTo.push_back(pWpt->getPathToThisWaypoint(i));
		}
	}
}

void CWaypointOpDelete::undo()
{
	// Re-add the deleted waypoint
	const int iNewIndex = CWaypoints::addWaypoint(nullptr, m_vOrigin, m_iFlags, false, m_iYaw, 0, m_fRadius);

	if (iNewIndex != m_iWaypointIndex)
	{
		logger->Log(LogLevel::WARN, "Undo delete waypoint: index mismatch (expected %d, got %d)", m_iWaypointIndex, iNewIndex);
	}

	// Restore paths
	CWaypoint* pWpt = CWaypoints::getWaypoint(iNewIndex);
	if (pWpt)
	{
		for (const int iPathIndex : m_PathsFrom)
		{
			pWpt->addPathTo(iPathIndex);
		}

		for (const int iPathIndex : m_PathsTo)
		{
			CWaypoint* pFromWpt = CWaypoints::getWaypoint(iPathIndex);
			if (pFromWpt)
			{
				pFromWpt->addPathTo(iNewIndex);
			}
		}
	}
}

void CWaypointOpDelete::redo()
{
	// Re-delete the waypoint
	CWaypoints::deleteWaypoint(m_iWaypointIndex);
}

///////////////////////////////////////////////////////////////////////////////
// CWaypointOpModify - Modify waypoint operation
///////////////////////////////////////////////////////////////////////////////

CWaypointOpModify::CWaypointOpModify(int iWaypointIndex, int iOldFlags, int iNewFlags)
	: m_iWaypointIndex(iWaypointIndex)
	, m_iOldFlags(iOldFlags)
	, m_iNewFlags(iNewFlags)
{
}

void CWaypointOpModify::undo()
{
	CWaypoint* pWpt = CWaypoints::getWaypoint(m_iWaypointIndex);
	if (pWpt)
	{
		pWpt->removeFlags();
		pWpt->addFlag(m_iOldFlags);
	}
}

void CWaypointOpModify::redo()
{
	CWaypoint* pWpt = CWaypoints::getWaypoint(m_iWaypointIndex);
	if (pWpt)
	{
		pWpt->removeFlags();
		pWpt->addFlag(m_iNewFlags);
	}
}

///////////////////////////////////////////////////////////////////////////////
// CWaypointOpAddPath - Add path operation
///////////////////////////////////////////////////////////////////////////////

CWaypointOpAddPath::CWaypointOpAddPath(int iFrom, int iTo)
	: m_iFrom(iFrom)
	, m_iTo(iTo)
{
}

void CWaypointOpAddPath::undo()
{
	CWaypoint* pWpt = CWaypoints::getWaypoint(m_iFrom);
	if (pWpt)
	{
		pWpt->removePathTo(m_iTo);
	}
}

void CWaypointOpAddPath::redo()
{
	CWaypoint* pWpt = CWaypoints::getWaypoint(m_iFrom);
	if (pWpt)
	{
		pWpt->addPathTo(m_iTo);
	}
}

///////////////////////////////////////////////////////////////////////////////
// CWaypointOpDeletePath - Delete path operation
///////////////////////////////////////////////////////////////////////////////

CWaypointOpDeletePath::CWaypointOpDeletePath(int iFrom, int iTo)
	: m_iFrom(iFrom)
	, m_iTo(iTo)
{
}

void CWaypointOpDeletePath::undo()
{
	CWaypoint* pWpt = CWaypoints::getWaypoint(m_iFrom);
	if (pWpt)
	{
		pWpt->addPathTo(m_iTo);
	}
}

void CWaypointOpDeletePath::redo()
{
	CWaypoint* pWpt = CWaypoints::getWaypoint(m_iFrom);
	if (pWpt)
	{
		pWpt->removePathTo(m_iTo);
	}
}

///////////////////////////////////////////////////////////////////////////////
// CWaypointOpBulkModify - Bulk modify operation
///////////////////////////////////////////////////////////////////////////////

CWaypointOpBulkModify::CWaypointOpBulkModify(const std::vector<WaypointModification>& modifications)
	: m_Modifications(modifications)
{
}

void CWaypointOpBulkModify::undo()
{
	for (const WaypointModification& mod : m_Modifications)
	{
		CWaypoint* pWpt = CWaypoints::getWaypoint(mod.iIndex);
		if (pWpt)
		{
			pWpt->removeFlags();
			pWpt->addFlag(mod.iOldFlags);
		}
	}
}

void CWaypointOpBulkModify::redo()
{
	for (const WaypointModification& mod : m_Modifications)
	{
		CWaypoint* pWpt = CWaypoints::getWaypoint(mod.iIndex);
		if (pWpt)
		{
			pWpt->removeFlags();
			pWpt->addFlag(mod.iNewFlags);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// CWaypointUndoManager - Undo/Redo manager
///////////////////////////////////////////////////////////////////////////////

void CWaypointUndoManager::addOperation(std::unique_ptr<IWaypointOperation> operation)
{
	// Clear redo stack when new operation is added
	m_RedoStack.clear();

	// Add to undo stack
	m_UndoStack.push_back(std::move(operation));

	// Limit undo stack size
	while (m_UndoStack.size() > MAX_UNDO_LEVELS)
	{
		m_UndoStack.pop_front();
	}
}

bool CWaypointUndoManager::undo()
{
	if (m_UndoStack.empty())
		return false;

	// Get last operation
	std::unique_ptr<IWaypointOperation> operation = std::move(m_UndoStack.back());
	m_UndoStack.pop_back();

	// Execute undo
	operation->undo();

	// Move to redo stack
	m_RedoStack.push_back(std::move(operation));

	return true;
}

bool CWaypointUndoManager::redo()
{
	if (m_RedoStack.empty())
		return false;

	// Get last undone operation
	std::unique_ptr<IWaypointOperation> operation = std::move(m_RedoStack.back());
	m_RedoStack.pop_back();

	// Execute redo
	operation->redo();

	// Move back to undo stack
	m_UndoStack.push_back(std::move(operation));

	return true;
}

const char* CWaypointUndoManager::getUndoDescription() const
{
	if (m_UndoStack.empty())
		return nullptr;

	return m_UndoStack.back()->getDescription();
}

const char* CWaypointUndoManager::getRedoDescription() const
{
	if (m_RedoStack.empty())
		return nullptr;

	return m_RedoStack.back()->getDescription();
}

void CWaypointUndoManager::clear()
{
	m_UndoStack.clear();
	m_RedoStack.clear();
}

///////////////////////////////////////////////////////////////////////////////
// CWaypointClipboard - Copy/Paste functionality
///////////////////////////////////////////////////////////////////////////////

void CWaypointClipboard::copy(int iWaypointIndex)
{
	std::vector<int> indices = { iWaypointIndex };
	copy(indices);
}

void CWaypointClipboard::copy(const std::vector<int>& waypointIndices)
{
	m_Waypoints.clear();

	if (waypointIndices.empty())
		return;

	// Calculate center point of selection
	Vector vCenter(0, 0, 0);
	int iValidCount = 0;

	for (const int iIndex : waypointIndices)
	{
		CWaypoint* pWpt = CWaypoints::getWaypoint(iIndex);
		if (pWpt)
		{
			vCenter = vCenter + pWpt->getOrigin();
			iValidCount++;
		}
	}

	if (iValidCount > 0)
	{
		vCenter = vCenter / static_cast<float>(iValidCount);
		m_vCenterOffset = vCenter;
	}

	// Copy waypoint data
	for (size_t i = 0; i < waypointIndices.size(); i++)
	{
		CWaypoint* pWpt = CWaypoints::getWaypoint(waypointIndices[i]);
		if (!pWpt)
			continue;

		ClipboardWaypoint clipWpt;
		clipWpt.vOrigin = pWpt->getOrigin();
		clipWpt.iFlags = pWpt->getFlags();
		clipWpt.iYaw = static_cast<int>(pWpt->getAimYaw());
		clipWpt.fRadius = pWpt->getRadius();

		// Store paths (only to other waypoints in selection)
		for (int j = 0; j < pWpt->numPaths(); j++)
		{
			const int iPathIndex = pWpt->getPath(j);

			// Find if path target is in selection
			for (size_t k = 0; k < waypointIndices.size(); k++)
			{
				if (waypointIndices[k] == iPathIndex)
				{
					clipWpt.paths.push_back(static_cast<int>(k));
					break;
				}
			}
		}

		m_Waypoints.push_back(clipWpt);
	}
}

bool CWaypointClipboard::paste(const Vector& vPasteOrigin, bool bPreserveRelativePositions)
{
	if (m_Waypoints.empty())
		return false;

	std::vector<int> newIndices;

	// Calculate offset from center
	const Vector vOffset = vPasteOrigin - m_vCenterOffset;

	// Create new waypoints
	for (const ClipboardWaypoint& clipWpt : m_Waypoints)
	{
		Vector vNewOrigin = bPreserveRelativePositions ? clipWpt.vOrigin + vOffset : vPasteOrigin;

		const int iNewIndex = CWaypoints::addWaypoint(nullptr, vNewOrigin, clipWpt.iFlags, false, clipWpt.iYaw, 0, clipWpt.fRadius);

		newIndices.push_back(iNewIndex);
	}

	// Restore paths
	for (size_t i = 0; i < m_Waypoints.size(); i++)
	{
		const ClipboardWaypoint& clipWpt = m_Waypoints[i];
		CWaypoint* pWpt = CWaypoints::getWaypoint(newIndices[i]);

		if (!pWpt)
			continue;

		for (const int iRelativePath : clipWpt.paths)
		{
			if (iRelativePath >= 0 && iRelativePath < static_cast<int>(newIndices.size()))
			{
				pWpt->addPathTo(newIndices[iRelativePath]);
			}
		}
	}

	return true;
}
