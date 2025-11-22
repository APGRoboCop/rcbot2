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

#ifndef __BOT_WAYPOINT_UNDO_H__
#define __BOT_WAYPOINT_UNDO_H__

#include "bot_waypoint.h"
#include <deque>
#include <vector>
#include <memory>

// Forward declarations
class CClient;

// Waypoint operation types for undo/redo
enum class EWaypointOperation
{
	ADD,
	DELETE,
	MODIFY,
	ADD_PATH,
	DELETE_PATH,
	BULK_MODIFY
};

// Base class for waypoint operations
class IWaypointOperation
{
public:
	virtual ~IWaypointOperation() = default;

	virtual void undo() = 0;
	virtual void redo() = 0;
	virtual EWaypointOperation getType() const = 0;
	virtual const char* getDescription() const = 0;
};

// Add waypoint operation
class CWaypointOpAdd : public IWaypointOperation
{
public:
	CWaypointOpAdd(int iWaypointIndex, const Vector& vOrigin, int iFlags, int iYaw, float fRadius);

	void undo() override;
	void redo() override;
	EWaypointOperation getType() const override { return EWaypointOperation::ADD; }
	const char* getDescription() const override { return "Add Waypoint"; }

private:
	int m_iWaypointIndex;
	Vector m_vOrigin;
	int m_iFlags;
	int m_iYaw;
	float m_fRadius;
	std::vector<int> m_Paths; // Paths that were auto-created
};

// Delete waypoint operation
class CWaypointOpDelete : public IWaypointOperation
{
public:
	CWaypointOpDelete(int iWaypointIndex);

	void undo() override;
	void redo() override;
	EWaypointOperation getType() const override { return EWaypointOperation::DELETE; }
	const char* getDescription() const override { return "Delete Waypoint"; }

private:
	int m_iWaypointIndex;
	Vector m_vOrigin;
	int m_iFlags;
	int m_iYaw;
	float m_fRadius;
	std::vector<int> m_PathsTo;   // Paths to this waypoint
	std::vector<int> m_PathsFrom; // Paths from this waypoint
};

// Modify waypoint operation
class CWaypointOpModify : public IWaypointOperation
{
public:
	CWaypointOpModify(int iWaypointIndex, int iOldFlags, int iNewFlags);

	void undo() override;
	void redo() override;
	EWaypointOperation getType() const override { return EWaypointOperation::MODIFY; }
	const char* getDescription() const override { return "Modify Waypoint"; }

private:
	int m_iWaypointIndex;
	int m_iOldFlags;
	int m_iNewFlags;
};

// Add path operation
class CWaypointOpAddPath : public IWaypointOperation
{
public:
	CWaypointOpAddPath(int iFrom, int iTo);

	void undo() override;
	void redo() override;
	EWaypointOperation getType() const override { return EWaypointOperation::ADD_PATH; }
	const char* getDescription() const override { return "Add Path"; }

private:
	int m_iFrom;
	int m_iTo;
};

// Delete path operation
class CWaypointOpDeletePath : public IWaypointOperation
{
public:
	CWaypointOpDeletePath(int iFrom, int iTo);

	void undo() override;
	void redo() override;
	EWaypointOperation getType() const override { return EWaypointOperation::DELETE_PATH; }
	const char* getDescription() const override { return "Delete Path"; }

private:
	int m_iFrom;
	int m_iTo;
};

// Bulk modify operation (for multiple waypoints)
class CWaypointOpBulkModify : public IWaypointOperation
{
public:
	struct WaypointModification
	{
		int iIndex;
		int iOldFlags;
		int iNewFlags;
	};

	CWaypointOpBulkModify(const std::vector<WaypointModification>& modifications);

	void undo() override;
	void redo() override;
	EWaypointOperation getType() const override { return EWaypointOperation::BULK_MODIFY; }
	const char* getDescription() const override { return "Bulk Modify Waypoints"; }

private:
	std::vector<WaypointModification> m_Modifications;
};

// Undo/Redo manager for waypoint operations
class CWaypointUndoManager
{
public:
	static constexpr int MAX_UNDO_LEVELS = 50;

	// Singleton access
	static CWaypointUndoManager& getInstance()
	{
		static CWaypointUndoManager instance;
		return instance;
	}

	// Add an operation to the undo stack
	void addOperation(std::unique_ptr<IWaypointOperation> operation);

	// Undo last operation
	bool undo();

	// Redo last undone operation
	bool redo();

	// Check if undo is available
	bool canUndo() const { return !m_UndoStack.empty(); }

	// Check if redo is available
	bool canRedo() const { return !m_RedoStack.empty(); }

	// Get description of next undo operation
	const char* getUndoDescription() const;

	// Get description of next redo operation
	const char* getRedoDescription() const;

	// Clear all undo/redo history
	void clear();

	// Get number of operations in undo stack
	size_t getUndoCount() const { return m_UndoStack.size(); }

	// Get number of operations in redo stack
	size_t getRedoCount() const { return m_RedoStack.size(); }

private:
	CWaypointUndoManager() = default;
	~CWaypointUndoManager() = default;

	// Delete copy constructor and assignment operator
	CWaypointUndoManager(const CWaypointUndoManager&) = delete;
	CWaypointUndoManager& operator=(const CWaypointUndoManager&) = delete;

	std::deque<std::unique_ptr<IWaypointOperation>> m_UndoStack;
	std::deque<std::unique_ptr<IWaypointOperation>> m_RedoStack;
};

// Clipboard for waypoint copy/paste
class CWaypointClipboard
{
public:
	struct ClipboardWaypoint
	{
		Vector vOrigin;
		int iFlags;
		int iYaw;
		float fRadius;
		std::vector<int> paths; // Relative path indices within clipboard
	};

	// Singleton access
	static CWaypointClipboard& getInstance()
	{
		static CWaypointClipboard instance;
		return instance;
	}

	// Copy waypoint(s) to clipboard
	void copy(int iWaypointIndex);
	void copy(const std::vector<int>& waypointIndices);

	// Paste waypoints at location
	bool paste(const Vector& vPasteOrigin, bool bPreserveRelativePositions = true);

	// Check if clipboard has data
	bool hasData() const { return !m_Waypoints.empty(); }

	// Clear clipboard
	void clear() { m_Waypoints.clear(); m_vCenterOffset = Vector(0, 0, 0); }

	// Get number of waypoints in clipboard
	size_t getCount() const { return m_Waypoints.size(); }

private:
	CWaypointClipboard() = default;
	~CWaypointClipboard() = default;

	// Delete copy constructor and assignment operator
	CWaypointClipboard(const CWaypointClipboard&) = delete;
	CWaypointClipboard& operator=(const CWaypointClipboard&) = delete;

	std::vector<ClipboardWaypoint> m_Waypoints;
	Vector m_vCenterOffset; // Center point of copied waypoints
};

#endif // __BOT_WAYPOINT_UNDO_H__
