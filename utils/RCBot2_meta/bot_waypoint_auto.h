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

#ifndef __BOT_WAYPOINT_AUTO_H__
#define __BOT_WAYPOINT_AUTO_H__

#include "bot_waypoint.h"
#include <vector>

// Enhanced auto-waypoint generation system
class CWaypointAutoGenerator
{
public:
	// Analyzes area complexity to determine optimal waypoint spacing
	static float calculateOptimalSpacing(const Vector& vOrigin, float fDefaultSpacing = 200.0f);

	// Detects nearby entities and suggests waypoint types
	static int detectWaypointType(edict_t* pPlayer, const Vector& vOrigin);

	// Analyzes terrain features (slopes, obstacles, etc.)
	static bool analyzeTerrainFeatures(edict_t* pPlayer, const Vector& vOrigin, int* pFlags);

	// Checks if location is suitable for waypoint placement
	static bool isSuitableLocation(const Vector& vOrigin, float fMinDistance = 100.0f);

	// Improved corner detection algorithm
	static bool detectCorner(const Vector& vCurrent, const Vector& vPrevious, float fAngleThreshold = 45.0f);

	// Detect narrow passages requiring crouch
	static bool requiresCrouch(edict_t* pPlayer, const Vector& vOrigin);

	// Detect if area provides cover
	static bool providesCover(edict_t* pPlayer, const Vector& vOrigin);

private:
	// Helper: Trace in multiple directions for terrain analysis
	static void traceMultiDirection(edict_t* pPlayer, const Vector& vOrigin, std::vector<float>& distances);

	// Helper: Calculate area variance (for complexity analysis)
	static float calculateAreaVariance(const std::vector<float>& distances);
};

// Automatic entity-based waypoint type detection
class CWaypointTypeDetector
{
public:
	struct EntityTypeMapping
	{
		const char* szClassName;
		int iWaypointFlags;
		float fRadius;
		bool bRequiresLineOfSight;
	};

	// Scan for nearby entities and determine waypoint type
	static int scanNearbyEntities(edict_t* pPlayer, const Vector& vOrigin, float fRadius = 200.0f);

	// Check for specific entity type near location
	static bool hasEntityNearby(const Vector& vOrigin, const char* szClassName, float fRadius);

	// Get entity-based waypoint mappings for current mod
	static const std::vector<EntityTypeMapping>& getEntityMappings();

private:
	static std::vector<EntityTypeMapping> m_TF2Mappings;
	static std::vector<EntityTypeMapping> m_CSMappings;
	static std::vector<EntityTypeMapping> m_DODMappings;
	static std::vector<EntityTypeMapping> m_HL2DMMappings;

	static void initializeMappings();
};

#endif // __BOT_WAYPOINT_AUTO_H__
