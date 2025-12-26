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

#ifndef __BOT_DOOR_H__
#define __BOT_DOOR_H__

#include <vector>
#include <cstdint>
#include "bot_mods.h"

struct edict_t;
class CBot;
class CWaypoint;

//=============================================================================
// Door Activation Types
// How the door can be opened
//=============================================================================
enum class EDoorActivationType : uint8_t
{
	UNKNOWN = 0,
	TOUCH_OPEN,         // Walk through to open (touch-activated)
	USE_OPEN,           // Requires +use interaction
	BUTTON_LINKED,      // Must press an associated button
	TRIGGER_PLATFORM,   // Step on a trigger platform
	BREAKABLE,          // Must be destroyed (shoot/melee)
	LOCKED,             // Currently locked, cannot open
	AUTO_OPEN,          // Opens automatically (proximity sensor)
	KEY_REQUIRED,       // Requires a key item
	MAX_ACTIVATION_TYPES
};

//=============================================================================
// Door Entity Types
// Different door entity classes in Source engine
//=============================================================================
enum class EDoorEntityType : uint8_t
{
	UNKNOWN = 0,
	FUNC_DOOR,              // func_door - basic sliding door
	FUNC_DOOR_ROTATING,     // func_door_rotating - hinged door
	PROP_DOOR_ROTATING,     // prop_door_rotating - model-based door
	FUNC_MOVELINEAR,        // func_movelinear - linear moving platform/door
	FUNC_TRACKTRAIN,        // func_tracktrain - train/elevator
	FUNC_BREAKABLE,         // func_breakable - destructible barrier
	PROP_PHYSICS,           // prop_physics - physics object blocking path
	MAX_DOOR_TYPES
};

//=============================================================================
// Door States
// Current state of a door
//=============================================================================
enum class EDoorState : uint8_t
{
	UNKNOWN = 0,
	CLOSED,
	OPENING,
	OPEN,
	CLOSING,
	LOCKED,
	BROKEN           // Destroyed/removed
};

//=============================================================================
// Source Engine Door Spawnflags
// From func_door and func_door_rotating entities
//=============================================================================
namespace DoorSpawnFlags
{
	// func_door spawnflags
	constexpr int SF_DOOR_STARTS_OPEN      = 1;
	constexpr int SF_DOOR_STARTS_LOCKED    = 2048;
	constexpr int SF_DOOR_SILENT           = 4;
	constexpr int SF_DOOR_USE_ONLY         = 256;    // Must +use to open
	constexpr int SF_DOOR_TOUCH_PLAYER     = 512;    // Opens on player touch
	constexpr int SF_DOOR_IGNORE_USE       = 32;     // Cannot be +used
	constexpr int SF_DOOR_NONSOLID         = 4096;

	// func_door_rotating spawnflags
	constexpr int SF_DOOR_ROTATE_BACKWARDS = 2;
	constexpr int SF_DOOR_ROTATE_ONEWAY    = 16;
	constexpr int SF_DOOR_ROTATE_X         = 64;
	constexpr int SF_DOOR_ROTATE_Y         = 128;

	// prop_door_rotating spawnflags
	constexpr int SF_PROP_DOOR_BREAKABLE   = 32768;
}

//=============================================================================
// CDoorInfo
// Information about a door entity in the map
//=============================================================================
class CDoorInfo
{
public:
	CDoorInfo();
	CDoorInfo(edict_t* pDoor);

	// Initialize from door edict
	bool init(edict_t* pDoor);

	// Update door state from game
	void updateState();

	// Getters
	edict_t* getEdict() const { return m_pEdict; }
	EDoorEntityType getEntityType() const { return m_entityType; }
	EDoorActivationType getActivationType() const { return m_activationType; }
	EDoorState getState() const { return m_state; }
	Vector getOrigin() const { return m_origin; }
	Vector getMins() const { return m_mins; }
	Vector getMaxs() const { return m_maxs; }

	// Linked button (if any)
	edict_t* getLinkedButton() const { return m_pLinkedButton; }
	void setLinkedButton(edict_t* pButton) { m_pLinkedButton = pButton; }

	// Trigger platform (if any)
	edict_t* getTriggerPlatform() const { return m_pTriggerPlatform; }
	void setTriggerPlatform(edict_t* pTrigger) { m_pTriggerPlatform = pTrigger; }

	// State checks
	bool isOpen() const { return m_state == EDoorState::OPEN; }
	bool isClosed() const { return m_state == EDoorState::CLOSED; }
	bool isLocked() const { return m_state == EDoorState::LOCKED || m_activationType == EDoorActivationType::LOCKED; }
	bool isMoving() const { return m_state == EDoorState::OPENING || m_state == EDoorState::CLOSING; }
	bool isUsable() const { return m_activationType == EDoorActivationType::USE_OPEN; }
	bool isTouchActivated() const { return m_activationType == EDoorActivationType::TOUCH_OPEN; }
	bool isBreakable() const { return m_activationType == EDoorActivationType::BREAKABLE; }

	// Timing
	float getAutoCloseTime() const { return m_autoCloseTime; }
	float getLastStateChangeTime() const { return m_lastStateChangeTime; }

	// For pathfinding cost calculations
	float getTraversalCost() const;

	// Check if a position is blocked by this door
	bool isBlockingPosition(const Vector& pos) const;

	// Entity validity
	bool isValid() const;

private:
	// Determine activation type from entity properties
	EDoorActivationType detectActivationType();

	// Determine entity type from classname
	EDoorEntityType detectEntityType();

	// Read state from entity netprops
	EDoorState readStateFromEntity();

	edict_t* m_pEdict;
	edict_t* m_pLinkedButton;
	edict_t* m_pTriggerPlatform;

	EDoorEntityType m_entityType;
	EDoorActivationType m_activationType;
	EDoorState m_state;
	EDoorState m_previousState;

	Vector m_origin;
	Vector m_mins;
	Vector m_maxs;

	int m_spawnFlags;
	float m_autoCloseTime;        // Time until door auto-closes (0 = stays open)
	float m_lastStateChangeTime;  // When state last changed
	float m_lastUpdateTime;       // When we last polled the entity

	bool m_isValid;
};

//=============================================================================
// CDoorManager
// Manages all doors in the current map
//=============================================================================
class CDoorManager
{
public:
	static CDoorManager& instance();

	// Scan map for all door entities
	void scanMap();

	// Reset all door data
	void reset();

	// Update door states (call periodically)
	void update();

	// Find door at or near a position
	CDoorInfo* findDoorAt(const Vector& pos, float radius = 64.0f);

	// Find door by edict
	CDoorInfo* findDoor(edict_t* pEdict);

	// Find door between two waypoints (blocking the path)
	CDoorInfo* findDoorBetween(CWaypoint* pFrom, CWaypoint* pTo);

	// Get all doors
	const std::vector<CDoorInfo>& getDoors() const { return m_doors; }

	// Get door count
	int getDoorCount() const { return static_cast<int>(m_doors.size()); }

	// Check if any door blocks a path between two points
	bool isPathBlockedByDoor(const Vector& from, const Vector& to, CDoorInfo** ppBlockingDoor = nullptr);

	// Find the button linked to a door (by entity I/O)
	static edict_t* findLinkedButton(edict_t* pDoor);

	// Find trigger that opens a door
	static edict_t* findTriggerForDoor(edict_t* pDoor);

	// Get path cost modifier for a door
	float getPathCostForDoor(const CDoorInfo* pDoor) const;

private:
	CDoorManager();
	~CDoorManager();

	// Prevent copying
	CDoorManager(const CDoorManager&) = delete;
	CDoorManager& operator=(const CDoorManager&) = delete;

	// Scan for specific entity type
	void scanForEntityType(const char* classname, EDoorEntityType type);

	// Link buttons and triggers to doors
	void linkButtonsAndTriggers();

	std::vector<CDoorInfo> m_doors;
	float m_lastUpdateTime;

	static CDoorManager* s_instance;
};

//=============================================================================
// Bot Door Tasks
// Tasks for bots to handle doors
//=============================================================================

#include "bot_task.h"

// Task to open a door (handles all activation types)
class CBotTaskOpenDoor : public CBotTask
{
public:
	CBotTaskOpenDoor(CDoorInfo* pDoorInfo);
	CBotTaskOpenDoor(edict_t* pDoor);

	void execute(CBot* pBot, CBotSchedule* pSchedule) override;
	void debugString(char* string, unsigned bufferSize) override;

private:
	enum class EOpenDoorState : uint8_t
	{
		APPROACH,           // Moving toward door/button
		WAIT_FOR_OPEN,      // Door is opening, wait
		USE_DOOR,           // Press +use on door
		USE_BUTTON,         // Press +use on linked button
		STAND_ON_TRIGGER,   // Stand on trigger platform
		BREAK_DOOR,         // Attack breakable door
		PASS_THROUGH,       // Walk through open door
		DONE
	};

	CDoorInfo* m_pDoorInfo;
	MyEHandle m_pDoorEdict;
	EOpenDoorState m_openState;
	float m_fTime;
	float m_fNextAttempt;
	int m_attempts;
};

// Task to wait for a door to open (used when door is already opening)
class CBotTaskWaitForDoor : public CBotTask
{
public:
	CBotTaskWaitForDoor(CDoorInfo* pDoorInfo, float maxWaitTime = 5.0f);

	void execute(CBot* pBot, CBotSchedule* pSchedule) override;
	void debugString(char* string, unsigned bufferSize) override;

private:
	CDoorInfo* m_pDoorInfo;
	float m_fMaxWaitTime;
	float m_fStartTime;
};

#endif // __BOT_DOOR_H__
