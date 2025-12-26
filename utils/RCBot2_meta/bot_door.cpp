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

#include "bot_door.h"
#include "bot.h"
#include "bot_globals.h"
#include "bot_waypoint.h"
#include "bot_getprop.h"
#include "bot_schedule.h"

#include <cstring>
#include <algorithm>

// Static instance
CDoorManager* CDoorManager::s_instance = nullptr;

//=============================================================================
// CDoorInfo Implementation
//=============================================================================

CDoorInfo::CDoorInfo()
	: m_pEdict(nullptr)
	, m_pLinkedButton(nullptr)
	, m_pTriggerPlatform(nullptr)
	, m_entityType(EDoorEntityType::UNKNOWN)
	, m_activationType(EDoorActivationType::UNKNOWN)
	, m_state(EDoorState::UNKNOWN)
	, m_previousState(EDoorState::UNKNOWN)
	, m_origin(0, 0, 0)
	, m_mins(0, 0, 0)
	, m_maxs(0, 0, 0)
	, m_spawnFlags(0)
	, m_autoCloseTime(0.0f)
	, m_lastStateChangeTime(0.0f)
	, m_lastUpdateTime(0.0f)
	, m_isValid(false)
{
}

CDoorInfo::CDoorInfo(edict_t* pDoor)
	: CDoorInfo()
{
	init(pDoor);
}

bool CDoorInfo::init(edict_t* pDoor)
{
	if (pDoor == nullptr || !CBotGlobals::entityIsValid(pDoor))
	{
		m_isValid = false;
		return false;
	}

	m_pEdict = pDoor;
	m_entityType = detectEntityType();
	m_activationType = detectActivationType();
	m_state = readStateFromEntity();

	// Get position and bounds
	ICollideable* pCollide = pDoor->GetCollideable();
	if (pCollide != nullptr)
	{
		m_origin = pCollide->GetCollisionOrigin();
		m_mins = pCollide->OBBMins();
		m_maxs = pCollide->OBBMaxs();
	}

	m_lastUpdateTime = gpGlobals->curtime;
	m_isValid = (m_entityType != EDoorEntityType::UNKNOWN);

	return m_isValid;
}

void CDoorInfo::updateState()
{
	if (!isValid())
		return;

	m_previousState = m_state;
	m_state = readStateFromEntity();

	if (m_state != m_previousState)
	{
		m_lastStateChangeTime = gpGlobals->curtime;
	}

	// Update position (for moving doors)
	ICollideable* pCollide = m_pEdict->GetCollideable();
	if (pCollide != nullptr)
	{
		m_origin = pCollide->GetCollisionOrigin();
	}

	m_lastUpdateTime = gpGlobals->curtime;
}

EDoorEntityType CDoorInfo::detectEntityType()
{
	if (m_pEdict == nullptr)
		return EDoorEntityType::UNKNOWN;

	const char* szClassname = m_pEdict->GetClassName();
	if (szClassname == nullptr)
		return EDoorEntityType::UNKNOWN;

	if (std::strcmp(szClassname, "func_door") == 0)
		return EDoorEntityType::FUNC_DOOR;
	else if (std::strcmp(szClassname, "func_door_rotating") == 0)
		return EDoorEntityType::FUNC_DOOR_ROTATING;
	else if (std::strcmp(szClassname, "prop_door_rotating") == 0)
		return EDoorEntityType::PROP_DOOR_ROTATING;
	else if (std::strcmp(szClassname, "func_movelinear") == 0)
		return EDoorEntityType::FUNC_MOVELINEAR;
	else if (std::strcmp(szClassname, "func_tracktrain") == 0)
		return EDoorEntityType::FUNC_TRACKTRAIN;
	else if (std::strcmp(szClassname, "func_breakable") == 0)
		return EDoorEntityType::FUNC_BREAKABLE;
	else if (std::strncmp(szClassname, "prop_physics", 12) == 0)
		return EDoorEntityType::PROP_PHYSICS;

	return EDoorEntityType::UNKNOWN;
}

EDoorActivationType CDoorInfo::detectActivationType()
{
	if (m_pEdict == nullptr)
		return EDoorActivationType::UNKNOWN;

	// For breakables, they need to be destroyed
	if (m_entityType == EDoorEntityType::FUNC_BREAKABLE ||
		m_entityType == EDoorEntityType::PROP_PHYSICS)
	{
		return EDoorActivationType::BREAKABLE;
	}

	// Check spawnflags for door behavior
	// Note: Spawnflags are typically read via network properties or datamaps
	// For now, use reasonable defaults based on door type

	// prop_door_rotating typically requires +use
	if (m_entityType == EDoorEntityType::PROP_DOOR_ROTATING)
	{
		return EDoorActivationType::USE_OPEN;
	}

	// func_door and func_door_rotating check spawnflags
	// Default to touch-activated unless spawnflags indicate otherwise
	if (m_entityType == EDoorEntityType::FUNC_DOOR ||
		m_entityType == EDoorEntityType::FUNC_DOOR_ROTATING)
	{
		// In Source, SF_DOOR_USE_ONLY (256) requires +use
		// SF_DOOR_TOUCH_PLAYER (512) allows touch
		// Without access to spawnflags, assume touch for func_door
		return EDoorActivationType::TOUCH_OPEN;
	}

	// func_movelinear usually requires button or trigger
	if (m_entityType == EDoorEntityType::FUNC_MOVELINEAR)
	{
		return EDoorActivationType::BUTTON_LINKED;
	}

	return EDoorActivationType::UNKNOWN;
}

EDoorState CDoorInfo::readStateFromEntity()
{
	if (m_pEdict == nullptr || !CBotGlobals::entityIsValid(m_pEdict))
		return EDoorState::UNKNOWN;

	// Check if entity still exists
	if (m_entityType == EDoorEntityType::FUNC_BREAKABLE ||
		m_entityType == EDoorEntityType::PROP_PHYSICS)
	{
		// For breakables, check if destroyed
		int effects = CClassInterface::getEffects(m_pEdict);
		if (effects & EF_NODRAW)
			return EDoorState::BROKEN;
	}

	// For doors, we'd ideally read m_eDoorState or m_toggle_state
	// Without direct access, we estimate based on animation cycle
	float fCycle = CClassInterface::getAnimCycle(m_pEdict);

	// Animation cycle 0 = closed, 1 = open, in between = moving
	if (fCycle < 0.1f)
		return EDoorState::CLOSED;
	else if (fCycle > 0.9f)
		return EDoorState::OPEN;
	else
	{
		// Door is animating - determine direction based on previous state
		if (m_previousState == EDoorState::CLOSED || m_previousState == EDoorState::OPENING)
			return EDoorState::OPENING;
		else
			return EDoorState::CLOSING;
	}
}

float CDoorInfo::getTraversalCost() const
{
	// Calculate pathfinding cost penalty for this door
	switch (m_activationType)
	{
	case EDoorActivationType::TOUCH_OPEN:
	case EDoorActivationType::AUTO_OPEN:
		return 10.0f;  // Minimal cost - just walk through

	case EDoorActivationType::USE_OPEN:
		return 50.0f;  // Need to stop and interact

	case EDoorActivationType::BUTTON_LINKED:
		return 100.0f; // Need to find and press button

	case EDoorActivationType::BREAKABLE:
		return 150.0f; // Need to attack

	case EDoorActivationType::LOCKED:
	case EDoorActivationType::KEY_REQUIRED:
		return 1000.0f; // Very high cost - probably can't open

	default:
		return 200.0f;
	}
}

bool CDoorInfo::isBlockingPosition(const Vector& pos) const
{
	if (!isValid() || isOpen())
		return false;

	// Check if position is within door bounds (with some margin)
	const float margin = 32.0f;
	Vector mins = m_origin + m_mins - Vector(margin, margin, margin);
	Vector maxs = m_origin + m_maxs + Vector(margin, margin, margin);

	return pos.x >= mins.x && pos.x <= maxs.x &&
		   pos.y >= mins.y && pos.y <= maxs.y &&
		   pos.z >= mins.z && pos.z <= maxs.z;
}

bool CDoorInfo::isValid() const
{
	return m_isValid && m_pEdict != nullptr && CBotGlobals::entityIsValid(m_pEdict);
}

//=============================================================================
// CDoorManager Implementation
//=============================================================================

CDoorManager& CDoorManager::instance()
{
	if (s_instance == nullptr)
		s_instance = new CDoorManager();
	return *s_instance;
}

CDoorManager::CDoorManager()
	: m_lastUpdateTime(0.0f)
{
}

CDoorManager::~CDoorManager()
{
}

void CDoorManager::scanMap()
{
	reset();

	// Scan for different door entity types
	scanForEntityType("func_door", EDoorEntityType::FUNC_DOOR);
	scanForEntityType("func_door_rotating", EDoorEntityType::FUNC_DOOR_ROTATING);
	scanForEntityType("prop_door_rotating", EDoorEntityType::PROP_DOOR_ROTATING);
	scanForEntityType("func_movelinear", EDoorEntityType::FUNC_MOVELINEAR);

	// Link buttons and triggers after scanning all doors
	linkButtonsAndTriggers();

	CBotGlobals::botMessage(nullptr, 0, "Door scan complete: %d doors found", getDoorCount());
}

void CDoorManager::scanForEntityType(const char* classname, EDoorEntityType type)
{
	// Iterate through all entities looking for this classname
	// Use MAX_ENTITIES from bot_globals.h
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

		if (std::strcmp(szClassName, classname) == 0)
		{
			CDoorInfo door(pEdict);
			if (door.isValid())
			{
				m_doors.push_back(door);
			}
		}
	}
}

void CDoorManager::linkButtonsAndTriggers()
{
	for (CDoorInfo& door : m_doors)
	{
		edict_t* pDoor = door.getEdict();
		if (pDoor == nullptr)
			continue;

		// Try to find linked button
		edict_t* pButton = findLinkedButton(pDoor);
		if (pButton != nullptr)
		{
			door.setLinkedButton(pButton);
			// Update activation type if button found
			if (door.getActivationType() != EDoorActivationType::BUTTON_LINKED)
			{
				// Door has a button, so it's button-activated
			}
		}

		// Try to find trigger
		edict_t* pTrigger = findTriggerForDoor(pDoor);
		if (pTrigger != nullptr)
		{
			door.setTriggerPlatform(pTrigger);
		}
	}
}

void CDoorManager::reset()
{
	m_doors.clear();
	m_lastUpdateTime = 0.0f;
}

void CDoorManager::update()
{
	float curTime = gpGlobals->curtime;

	// Update door states periodically (every 0.5 seconds)
	if (curTime - m_lastUpdateTime < 0.5f)
		return;

	m_lastUpdateTime = curTime;

	// Update each door's state
	for (CDoorInfo& door : m_doors)
	{
		door.updateState();
	}

	// Remove invalid doors
	m_doors.erase(
		std::remove_if(m_doors.begin(), m_doors.end(),
			[](const CDoorInfo& door) { return !door.isValid(); }),
		m_doors.end()
	);
}

CDoorInfo* CDoorManager::findDoorAt(const Vector& pos, float radius)
{
	float radiusSq = radius * radius;
	CDoorInfo* pNearest = nullptr;
	float nearestDistSq = FLT_MAX;

	for (CDoorInfo& door : m_doors)
	{
		if (!door.isValid())
			continue;

		float distSq = (door.getOrigin() - pos).LengthSqr();
		if (distSq < radiusSq && distSq < nearestDistSq)
		{
			nearestDistSq = distSq;
			pNearest = &door;
		}
	}

	return pNearest;
}

CDoorInfo* CDoorManager::findDoor(edict_t* pEdict)
{
	if (pEdict == nullptr)
		return nullptr;

	for (CDoorInfo& door : m_doors)
	{
		if (door.getEdict() == pEdict)
			return &door;
	}

	return nullptr;
}

CDoorInfo* CDoorManager::findDoorBetween(CWaypoint* pFrom, CWaypoint* pTo)
{
	if (pFrom == nullptr || pTo == nullptr)
		return nullptr;

	Vector vFrom = pFrom->getOrigin();
	Vector vTo = pTo->getOrigin();

	// Check each door to see if it's between the waypoints
	for (CDoorInfo& door : m_doors)
	{
		if (!door.isValid())
			continue;

		// Check if door is roughly on the line between waypoints
		Vector vDoor = door.getOrigin();

		// Simple check: is door within a cylinder between the waypoints?
		Vector vDir = vTo - vFrom;
		float fLength = vDir.Length();
		if (fLength < 1.0f)
			continue;

		vDir = vDir / fLength;

		// Project door position onto line
		Vector vRelative = vDoor - vFrom;
		float fProj = vRelative.Dot(vDir);

		// Check if projection is between waypoints
		if (fProj < 0.0f || fProj > fLength)
			continue;

		// Check distance from line
		Vector vClosestOnLine = vFrom + vDir * fProj;
		float fDistFromLine = (vDoor - vClosestOnLine).Length();

		// Door should be within 100 units of the path
		if (fDistFromLine < 100.0f)
		{
			return &door;
		}
	}

	return nullptr;
}

bool CDoorManager::isPathBlockedByDoor(const Vector& from, const Vector& to, CDoorInfo** ppBlockingDoor)
{
	// Similar logic to findDoorBetween but checks if door is closed
	Vector vDir = to - from;
	float fLength = vDir.Length();
	if (fLength < 1.0f)
		return false;

	vDir = vDir / fLength;

	for (CDoorInfo& door : m_doors)
	{
		if (!door.isValid())
			continue;

		// Only closed doors block paths
		if (door.isOpen())
			continue;

		Vector vDoor = door.getOrigin();
		Vector vRelative = vDoor - from;
		float fProj = vRelative.Dot(vDir);

		if (fProj < 0.0f || fProj > fLength)
			continue;

		Vector vClosestOnLine = from + vDir * fProj;
		float fDistFromLine = (vDoor - vClosestOnLine).Length();

		// Check if path intersects door bounds
		if (fDistFromLine < 64.0f)
		{
			if (ppBlockingDoor != nullptr)
				*ppBlockingDoor = &door;
			return true;
		}
	}

	return false;
}

edict_t* CDoorManager::findLinkedButton(edict_t* pDoor)
{
	if (pDoor == nullptr)
		return nullptr;

	Vector vDoor = CBotGlobals::entityOrigin(pDoor);

	// Search for nearby buttons
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

		// Check for button entities
		if (std::strncmp(szClassName, "func_button", 11) == 0 ||
			std::strncmp(szClassName, "func_rot_button", 15) == 0)
		{
			Vector vButton = CBotGlobals::entityOrigin(pEdict);
			float fDist = (vButton - vDoor).Length();

			// Button should be within reasonable distance
			if (fDist < 500.0f)
			{
				// TODO: Check entity I/O connections if possible
				// For now, return nearest button
				return pEdict;
			}
		}
	}

	return nullptr;
}

edict_t* CDoorManager::findTriggerForDoor(edict_t* pDoor)
{
	if (pDoor == nullptr)
		return nullptr;

	Vector vDoor = CBotGlobals::entityOrigin(pDoor);

	// Search for nearby triggers
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

		// Check for trigger entities
		if (std::strncmp(szClassName, "trigger_", 8) == 0)
		{
			Vector vTrigger = CBotGlobals::entityOrigin(pEdict);
			float fDist = (vTrigger - vDoor).Length();

			// Trigger should be near door
			if (fDist < 200.0f)
			{
				return pEdict;
			}
		}
	}

	return nullptr;
}

float CDoorManager::getPathCostForDoor(const CDoorInfo* pDoor) const
{
	if (pDoor == nullptr)
		return 0.0f;

	return pDoor->getTraversalCost();
}

//=============================================================================
// CBotTaskOpenDoor Implementation
//=============================================================================

CBotTaskOpenDoor::CBotTaskOpenDoor(CDoorInfo* pDoorInfo)
	: m_pDoorInfo(pDoorInfo)
	, m_pDoorEdict(pDoorInfo ? pDoorInfo->getEdict() : nullptr)
	, m_openState(EOpenDoorState::APPROACH)
	, m_fTime(0.0f)
	, m_fNextAttempt(0.0f)
	, m_attempts(0)
{
}

CBotTaskOpenDoor::CBotTaskOpenDoor(edict_t* pDoor)
	: m_pDoorInfo(nullptr)
	, m_pDoorEdict(pDoor)
	, m_openState(EOpenDoorState::APPROACH)
	, m_fTime(0.0f)
	, m_fNextAttempt(0.0f)
	, m_attempts(0)
{
	// Try to find door info from manager
	if (pDoor != nullptr)
	{
		m_pDoorInfo = CDoorManager::instance().findDoor(pDoor);
	}
}

void CBotTaskOpenDoor::execute(CBot* pBot, CBotSchedule* pSchedule)
{
	if (pBot == nullptr || !m_pDoorEdict.get())
	{
		fail();
		return;
	}

	// Update door info if available
	if (m_pDoorInfo != nullptr)
	{
		m_pDoorInfo->updateState();

		// Door is already open - we're done
		if (m_pDoorInfo->isOpen())
		{
			complete();
			return;
		}
	}

	float curTime = gpGlobals->curtime;
	Vector vDoor = CBotGlobals::entityOrigin(m_pDoorEdict.get());
	float fDist = pBot->distanceFrom(vDoor);

	// Initialize timer
	if (m_fTime == 0.0f)
		m_fTime = curTime;

	// Timeout check
	if (curTime - m_fTime > 30.0f)
	{
		fail();
		return;
	}

	// State machine for opening door
	switch (m_openState)
	{
	case EOpenDoorState::APPROACH:
		// Move toward door
		if (fDist > 100.0f)
		{
			pBot->setMoveTo(vDoor);
		}
		else
		{
			// Close enough, determine action
			if (m_pDoorInfo != nullptr)
			{
				switch (m_pDoorInfo->getActivationType())
				{
				case EDoorActivationType::TOUCH_OPEN:
				case EDoorActivationType::AUTO_OPEN:
					m_openState = EOpenDoorState::PASS_THROUGH;
					break;

				case EDoorActivationType::USE_OPEN:
					m_openState = EOpenDoorState::USE_DOOR;
					break;

				case EDoorActivationType::BUTTON_LINKED:
					m_openState = EOpenDoorState::USE_BUTTON;
					break;

				case EDoorActivationType::BREAKABLE:
					m_openState = EOpenDoorState::BREAK_DOOR;
					break;

				default:
					// Try using the door
					m_openState = EOpenDoorState::USE_DOOR;
					break;
				}
			}
			else
			{
				// No door info - try walking through then using
				m_openState = EOpenDoorState::PASS_THROUGH;
			}
		}
		break;

	case EOpenDoorState::USE_DOOR:
		if (curTime >= m_fNextAttempt)
		{
			pBot->setLookVector(vDoor);
			pBot->use();
			m_fNextAttempt = curTime + 0.5f;
			m_attempts++;

			if (m_attempts > 10)
			{
				// Can't open by using, try walking through
				m_openState = EOpenDoorState::PASS_THROUGH;
				m_attempts = 0;
			}
		}
		break;

	case EOpenDoorState::USE_BUTTON:
		if (m_pDoorInfo != nullptr && m_pDoorInfo->getLinkedButton() != nullptr)
		{
			Vector vButton = CBotGlobals::entityOrigin(m_pDoorInfo->getLinkedButton());
			float fButtonDist = pBot->distanceFrom(vButton);

			if (fButtonDist > 80.0f)
			{
				pBot->setMoveTo(vButton);
			}
			else if (curTime >= m_fNextAttempt)
			{
				pBot->setLookVector(vButton);
				pBot->use();
				m_fNextAttempt = curTime + 1.0f;
				m_openState = EOpenDoorState::WAIT_FOR_OPEN;
			}
		}
		else
		{
			// No button found, try using door directly
			m_openState = EOpenDoorState::USE_DOOR;
		}
		break;

	case EOpenDoorState::BREAK_DOOR:
		pBot->setLookVector(vDoor);
		pBot->primaryAttack();

		// Check if door is destroyed
		if (m_pDoorInfo != nullptr && m_pDoorInfo->getState() == EDoorState::BROKEN)
		{
			complete();
			return;
		}
		break;

	case EOpenDoorState::WAIT_FOR_OPEN:
		// Wait for door to open after pressing button
		if (m_pDoorInfo != nullptr && m_pDoorInfo->isOpen())
		{
			m_openState = EOpenDoorState::PASS_THROUGH;
		}
		else if (curTime >= m_fNextAttempt + 3.0f)
		{
			// Timeout waiting for door
			m_attempts++;
			if (m_attempts > 3)
			{
				fail();
				return;
			}
			m_openState = EOpenDoorState::USE_BUTTON;
		}
		break;

	case EOpenDoorState::PASS_THROUGH:
		// Walk through the door
		pBot->setMoveTo(vDoor);

		if (fDist < 32.0f)
		{
			complete();
			return;
		}
		break;

	case EOpenDoorState::DONE:
		complete();
		break;
	}
}

void CBotTaskOpenDoor::debugString(char* string, unsigned bufferSize)
{
	const char* stateStr = "Unknown";
	switch (m_openState)
	{
	case EOpenDoorState::APPROACH: stateStr = "Approach"; break;
	case EOpenDoorState::WAIT_FOR_OPEN: stateStr = "WaitOpen"; break;
	case EOpenDoorState::USE_DOOR: stateStr = "UseDoor"; break;
	case EOpenDoorState::USE_BUTTON: stateStr = "UseButton"; break;
	case EOpenDoorState::STAND_ON_TRIGGER: stateStr = "StandTrigger"; break;
	case EOpenDoorState::BREAK_DOOR: stateStr = "BreakDoor"; break;
	case EOpenDoorState::PASS_THROUGH: stateStr = "PassThru"; break;
	case EOpenDoorState::DONE: stateStr = "Done"; break;
	}

	snprintf(string, bufferSize, "OpenDoor (%s)", stateStr);
}

//=============================================================================
// CBotTaskWaitForDoor Implementation
//=============================================================================

CBotTaskWaitForDoor::CBotTaskWaitForDoor(CDoorInfo* pDoorInfo, float maxWaitTime)
	: m_pDoorInfo(pDoorInfo)
	, m_fMaxWaitTime(maxWaitTime)
	, m_fStartTime(0.0f)
{
}

void CBotTaskWaitForDoor::execute(CBot* pBot, CBotSchedule* pSchedule)
{
	if (pBot == nullptr || m_pDoorInfo == nullptr)
	{
		fail();
		return;
	}

	float curTime = gpGlobals->curtime;

	// Initialize start time
	if (m_fStartTime == 0.0f)
		m_fStartTime = curTime;

	// Update door state
	m_pDoorInfo->updateState();

	// Check if door is open
	if (m_pDoorInfo->isOpen())
	{
		complete();
		return;
	}

	// Check timeout
	if (curTime - m_fStartTime > m_fMaxWaitTime)
	{
		fail();
		return;
	}

	// Stop and wait
	pBot->stopMoving();
}

void CBotTaskWaitForDoor::debugString(char* string, unsigned bufferSize)
{
	float elapsed = gpGlobals->curtime - m_fStartTime;
	snprintf(string, bufferSize, "WaitDoor (%.1f/%.1f)", elapsed, m_fMaxWaitTime);
}
