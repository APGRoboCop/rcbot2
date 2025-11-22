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

#include "bot_waypoint_auto.h"
#include "bot_globals.h"
#include "bot_waypoint_locations.h"
#include "bot_configfile.h"
#include "bot_cvars.h"

#include <algorithm>
#include <cmath>

// Entity type mappings for different mods
std::vector<CWaypointTypeDetector::EntityTypeMapping> CWaypointTypeDetector::m_TF2Mappings;
std::vector<CWaypointTypeDetector::EntityTypeMapping> CWaypointTypeDetector::m_CSMappings;
std::vector<CWaypointTypeDetector::EntityTypeMapping> CWaypointTypeDetector::m_DODMappings;
std::vector<CWaypointTypeDetector::EntityTypeMapping> CWaypointTypeDetector::m_HL2DMMappings;

///////////////////////////////////////////////////////////////////////////////
// CWaypointAutoGenerator - Improved auto-waypoint generation
///////////////////////////////////////////////////////////////////////////////

float CWaypointAutoGenerator::calculateOptimalSpacing(const Vector& vOrigin, float fDefaultSpacing)
{
	std::vector<float> distances;
	traceMultiDirection(nullptr, vOrigin, distances);

	if (distances.empty())
		return fDefaultSpacing;

	// Calculate variance to determine area complexity
	const float fVariance = calculateAreaVariance(distances);

	// High variance = complex area = tighter spacing
	// Low variance = open area = wider spacing
	float fSpacing = fDefaultSpacing;

	if (fVariance > 500.0f) {
		// Very complex area (lots of obstacles)
		fSpacing = fDefaultSpacing * 0.5f; // 100 units
	} else if (fVariance > 200.0f) {
		// Moderately complex area
		fSpacing = fDefaultSpacing * 0.75f; // 150 units
	} else if (fVariance < 50.0f) {
		// Very open area
		fSpacing = fDefaultSpacing * 1.5f; // 300 units
	}

	// Clamp to reasonable bounds
	return std::clamp(fSpacing, 100.0f, 400.0f);
}

int CWaypointAutoGenerator::detectWaypointType(edict_t* pPlayer, const Vector& vOrigin)
{
	int iFlags = 0;

	// Scan for nearby entities
	const int iEntityFlags = CWaypointTypeDetector::scanNearbyEntities(pPlayer, vOrigin);
	if (iEntityFlags != 0)
		return iEntityFlags;

	// Check terrain features
	if (providesCover(pPlayer, vOrigin))
		iFlags |= CWaypointTypes::W_FL_DEFEND;

	return iFlags;
}

bool CWaypointAutoGenerator::analyzeTerrainFeatures(edict_t* pPlayer, const Vector& vOrigin, int* pFlags)
{
	if (!pFlags)
		return false;

	*pFlags = 0;

	// Check for crouch requirement
	if (requiresCrouch(pPlayer, vOrigin))
		*pFlags |= CWaypointTypes::W_FL_CROUCH;

	// Check for cover
	if (providesCover(pPlayer, vOrigin))
		*pFlags |= CWaypointTypes::W_FL_DEFEND;

	return *pFlags != 0;
}

bool CWaypointAutoGenerator::isSuitableLocation(const Vector& vOrigin, float fMinDistance)
{
	// Check if there's already a waypoint too close
	const int iNearestWpt = CWaypointLocations::NearestWaypoint(vOrigin, fMinDistance, -1, true, false, false, nullptr);

	if (iNearestWpt != -1)
		return false;

	// Check if on solid ground (basic trace down)
	trace_t tr;
	CBotGlobals::quickTraceline(nullptr, vOrigin, vOrigin - Vector(0, 0, 100));
	tr = *CBotGlobals::getTraceResult();

	// Must have ground beneath
	return tr.fraction < 1.0f;
}

bool CWaypointAutoGenerator::detectCorner(const Vector& vCurrent, const Vector& vPrevious, float fAngleThreshold)
{
	// Calculate direction vectors
	Vector vDir1 = vCurrent - vPrevious;
	vDir1.z = 0; // Ignore vertical component
	vDir1 = vDir1.Normalize();

	if (vDir1.Length() < 0.1f)
		return false;

	// Trace forward from current position
	Vector vForward = vCurrent + vDir1 * 200.0f;
	CBotGlobals::quickTraceline(nullptr, vCurrent, vForward);
	trace_t* tr = CBotGlobals::getTraceResult();

	// If we hit something within short distance, it's likely a corner
	if (tr->fraction < 0.5f)
		return true;

	return false;
}

bool CWaypointAutoGenerator::requiresCrouch(edict_t* pPlayer, const Vector& vOrigin)
{
	// Trace upward to check ceiling height
	Vector vUp = vOrigin + Vector(0, 0, 72); // Standing height
	CBotGlobals::quickTraceline(pPlayer, vOrigin, vUp);
	trace_t* tr = CBotGlobals::getTraceResult();

	// If trace hits something before full standing height, requires crouch
	if (tr->fraction < 0.8f)
		return true;

	return false;
}

bool CWaypointAutoGenerator::providesCover(edict_t* pPlayer, const Vector& vOrigin)
{
	// Trace in 8 directions at chest height to check for cover
	const Vector vTestOrigin = vOrigin + Vector(0, 0, 36);
	int iCoverCount = 0;

	for (int i = 0; i < 8; i++)
	{
		const float fAngle = static_cast<float>(i) * 45.0f * (M_PI / 180.0f);
		const Vector vDir(std::cos(fAngle) * 100.0f, std::sin(fAngle) * 100.0f, 0);
		const Vector vEnd = vTestOrigin + vDir;

		CBotGlobals::quickTraceline(pPlayer, vTestOrigin, vEnd);
		trace_t* tr = CBotGlobals::getTraceResult();

		if (tr->fraction < 1.0f)
			iCoverCount++;
	}

	// If blocked in 3+ directions, provides cover
	return iCoverCount >= 3;
}

void CWaypointAutoGenerator::traceMultiDirection(edict_t* pPlayer, const Vector& vOrigin, std::vector<float>& distances)
{
	distances.clear();

	// Trace in 8 horizontal directions
	for (int i = 0; i < 8; i++)
	{
		const float fAngle = static_cast<float>(i) * 45.0f * (M_PI / 180.0f);
		const Vector vDir(std::cos(fAngle) * 500.0f, std::sin(fAngle) * 500.0f, 0);
		const Vector vEnd = vOrigin + vDir;

		CBotGlobals::quickTraceline(pPlayer, vOrigin, vEnd);
		trace_t* tr = CBotGlobals::getTraceResult();

		distances.push_back(tr->fraction * 500.0f);
	}
}

float CWaypointAutoGenerator::calculateAreaVariance(const std::vector<float>& distances)
{
	if (distances.empty())
		return 0.0f;

	// Calculate mean
	float fMean = 0.0f;
	for (const float fDist : distances)
		fMean += fDist;
	fMean /= static_cast<float>(distances.size());

	// Calculate variance
	float fVariance = 0.0f;
	for (const float fDist : distances)
	{
		const float fDiff = fDist - fMean;
		fVariance += fDiff * fDiff;
	}
	fVariance /= static_cast<float>(distances.size());

	return fVariance;
}

///////////////////////////////////////////////////////////////////////////////
// CWaypointTypeDetector - Entity-based waypoint type detection
///////////////////////////////////////////////////////////////////////////////

void CWaypointTypeDetector::initializeMappings()
{
	// Initialize once
	if (!m_TF2Mappings.empty())
		return;

	// TF2 Entity Mappings
	m_TF2Mappings = {
		{"item_healthkit_small", CWaypointTypes::W_FL_HEALTH, 50.0f, false},
		{"item_healthkit_medium", CWaypointTypes::W_FL_HEALTH, 50.0f, false},
		{"item_healthkit_full", CWaypointTypes::W_FL_HEALTH, 50.0f, false},
		{"item_ammopack_small", CWaypointTypes::W_FL_AMMO, 50.0f, false},
		{"item_ammopack_medium", CWaypointTypes::W_FL_AMMO, 50.0f, false},
		{"item_ammopack_full", CWaypointTypes::W_FL_AMMO, 50.0f, false},
		{"func_regenerate", CWaypointTypes::W_FL_RESUPPLY, 80.0f, false},
		{"item_teamflag", CWaypointTypes::W_FL_FLAG, 100.0f, false},
		{"team_control_point", CWaypointTypes::W_FL_CAPPOINT, 150.0f, false},
		{"team_control_point_round", CWaypointTypes::W_FL_CAPPOINT, 150.0f, false},
	};

	// CS:S Entity Mappings
	m_CSMappings = {
		{"item_healthkit", CWaypointTypes::W_FL_HEALTH, 50.0f, false},
		{"hostage_entity", CWaypointTypes::W_FL_RESCUEZONE, 100.0f, false},
		{"func_bomb_target", CWaypointTypes::W_FL_GOAL, 200.0f, false},
		{"func_hostage_rescue", CWaypointTypes::W_FL_RESCUEZONE, 200.0f, false},
	};

	// DOD:S Entity Mappings
	m_DODMappings = {
		{"dod_control_point", CWaypointTypes::W_FL_CAPPOINT, 150.0f, false},
		{"dod_bomb_target", CWaypointTypes::W_FL_BOMBS_HERE, 150.0f, false},
		{"func_door", CWaypointTypes::W_FL_BOMB_TO_OPEN, 100.0f, false},
	};

	// HL2DM Entity Mappings
	m_HL2DMMappings = {
		{"item_healthkit", CWaypointTypes::W_FL_HEALTH, 50.0f, false},
		{"item_healthvial", CWaypointTypes::W_FL_HEALTH, 50.0f, false},
		{"item_battery", CWaypointTypes::W_FL_HEALTH, 50.0f, false},
		{"item_ammo_ar2", CWaypointTypes::W_FL_AMMO, 50.0f, false},
		{"item_ammo_smg1", CWaypointTypes::W_FL_AMMO, 50.0f, false},
		{"item_ammo_357", CWaypointTypes::W_FL_AMMO, 50.0f, false},
		{"item_box_buckshot", CWaypointTypes::W_FL_AMMO, 50.0f, false},
	};
}

int CWaypointTypeDetector::scanNearbyEntities(edict_t* pPlayer, const Vector& vOrigin, float fRadius)
{
	initializeMappings();

	const std::vector<EntityTypeMapping>& mappings = getEntityMappings();

	for (const EntityTypeMapping& mapping : mappings)
	{
		if (hasEntityNearby(vOrigin, mapping.szClassName, mapping.fRadius))
			return mapping.iWaypointFlags;
	}

	return 0;
}

bool CWaypointTypeDetector::hasEntityNearby(const Vector& vOrigin, const char* szClassName, float fRadius)
{
	// Search for entity by classname within radius
	edict_t* pEntity = nullptr;

	while ((pEntity = CBotGlobals::findEntityByClassnameNearest(vOrigin, szClassName, fRadius, pEntity)) != nullptr)
	{
		return true;
	}

	return false;
}

const std::vector<CWaypointTypeDetector::EntityTypeMapping>& CWaypointTypeDetector::getEntityMappings()
{
	switch (CBotGlobals::getGameInfo()->getGameRules())
	{
	case BOTGAMERULES_TEAMPLAY_CSS:
		return m_CSMappings;
	case BOTGAMERULES_DOD:
		return m_DODMappings;
	case BOTGAMERULES_HL2DM:
		return m_HL2DMMappings;
	case BOTGAMERULES_TEAMPLAY:
	default:
		return m_TF2Mappings;
	}
}
