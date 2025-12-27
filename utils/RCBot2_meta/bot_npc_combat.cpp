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

#include "bot_npc_combat.h"
#include "bot_globals.h"
#include "bot_waypoint.h"
#include "bot_waypoint_locations.h"
#include "rcbot/logging.h"

#include <cstring>
#include <algorithm>

///////////////////////////////////////////////////////////////////////////////
// CCoopModeDetector - Detect cooperative game mode
///////////////////////////////////////////////////////////////////////////////

bool CCoopModeDetector::isCooperativeMode()
{
	if (m_bForceCoopMode)
		return true;

	if (!m_bDetected)
	{
		m_bIsCoopMode = detectCoopMode();
		m_bDetected = true;
	}

	return m_bIsCoopMode;
}

bool CCoopModeDetector::shouldTargetNPCs()
{
	return isCooperativeMode();
}

bool CCoopModeDetector::isCoopMap(const char* szMapName)
{
	if (!szMapName)
		return false;

	// Check common cooperative map prefixes
	return std::strncmp(szMapName, "coop_", 5) == 0 ||
	       std::strncmp(szMapName, "js_coop_", 8) == 0 ||
	       std::strncmp(szMapName, "pve_", 4) == 0 ||
	       std::strncmp(szMapName, "co_op_", 6) == 0 ||
	       std::strstr(szMapName, "_coop") != nullptr ||
	       std::strstr(szMapName, "_pve") != nullptr;
}

bool CCoopModeDetector::detectCoopMode()
{
	const char* szMapName = CBotGlobals::getMapName();

	// Check map name
	if (isCoopMap(szMapName))
	{
		logger->Log(LogLevel::INFO, "Cooperative mode detected from map name: %s", szMapName);
		return true;
	}

	// Check if there are hostile NPCs in the map
	edict_t* pEntity = nullptr;
	int iNPCCount = 0;

	const char* npcClasses[] = {
		"npc_combine_s", "npc_zombie", "npc_headcrab", "npc_antlion"
	};

	for (const char* szClass : npcClasses)
	{
		pEntity = nullptr;
		while ((pEntity = CBotGlobals::findEntityByClassname(pEntity, szClass)) != nullptr)
		{
			iNPCCount++;
			if (iNPCCount >= 3) // If 3+ hostile NPCs, probably coop
			{
				logger->Log(LogLevel::INFO, "Cooperative mode detected: Found %d+ hostile NPCs", iNPCCount);
				return true;
			}
		}
	}

	return false;
}

int CCoopModeDetector::getCoopTeam()
{
	// In most HL2DM coop maps, all players are on team 2 (Rebels) or team 3
	// Check which team has the most players
	return 2; // Default to Rebels team
}

///////////////////////////////////////////////////////////////////////////////
// CNPCDatabase - NPC Information Database
///////////////////////////////////////////////////////////////////////////////

void CNPCDatabase::initialize()
{
	if (m_bInitialized)
		return;

	// Combine Forces
	addNPCInfo({"npc_combine_s", ENPCThreatLevel::MEDIUM, ENPCBehavior::RANGED, 50, 800.0f, 1500.0f, false, false, false, 60, "Combine Soldier"});
	addNPCInfo({"npc_metropolice", ENPCThreatLevel::LOW, ENPCBehavior::RANGED, 40, 600.0f, 1200.0f, false, false, false, 40, "Metrocop"});
	addNPCInfo({"npc_combinedropship", ENPCThreatLevel::MEDIUM, ENPCBehavior::VEHICLE, 500, 1500.0f, 2000.0f, true, true, false, 50, "Dropship"});
	addNPCInfo({"npc_combinegunship", ENPCThreatLevel::CRITICAL, ENPCBehavior::VEHICLE, 2000, 2000.0f, 3000.0f, true, true, true, 95, "Gunship"});
	addNPCInfo({"npc_helicopter", ENPCThreatLevel::CRITICAL, ENPCBehavior::VEHICLE, 2500, 2000.0f, 3000.0f, true, true, true, 95, "Attack Helicopter"});
	addNPCInfo({"npc_strider", ENPCThreatLevel::BOSS, ENPCBehavior::SPECIAL, 350, 1500.0f, 2500.0f, false, true, false, 100, "Strider"});
	addNPCInfo({"npc_hunter", ENPCThreatLevel::HIGH, ENPCBehavior::HYBRID, 210, 1000.0f, 1800.0f, false, true, true, 80, "Hunter"});

	// Zombies and Headcrabs
	addNPCInfo({"npc_zombie", ENPCThreatLevel::LOW, ENPCBehavior::MELEE, 50, 100.0f, 500.0f, false, false, false, 30, "Zombie"});
	addNPCInfo({"npc_zombie_torso", ENPCThreatLevel::LOW, ENPCBehavior::MELEE, 30, 100.0f, 400.0f, false, false, false, 25, "Zombie Torso"});
	addNPCInfo({"npc_fastzombie", ENPCThreatLevel::MEDIUM, ENPCBehavior::MELEE, 50, 150.0f, 600.0f, false, false, true, 55, "Fast Zombie"});
	addNPCInfo({"npc_fastzombie_torso", ENPCThreatLevel::MEDIUM, ENPCBehavior::MELEE, 30, 150.0f, 500.0f, false, false, true, 50, "Fast Zombie Torso"});
	addNPCInfo({"npc_poisonzombie", ENPCThreatLevel::HIGH, ENPCBehavior::HYBRID, 175, 400.0f, 800.0f, false, false, false, 70, "Poison Zombie"});
	addNPCInfo({"npc_zombine", ENPCThreatLevel::MEDIUM, ENPCBehavior::MELEE, 100, 150.0f, 600.0f, false, false, false, 60, "Zombine"});
	addNPCInfo({"npc_headcrab", ENPCThreatLevel::LOW, ENPCBehavior::MELEE, 10, 100.0f, 300.0f, false, false, true, 20, "Headcrab"});
	addNPCInfo({"npc_headcrab_fast", ENPCThreatLevel::LOW, ENPCBehavior::MELEE, 10, 100.0f, 400.0f, false, false, true, 25, "Fast Headcrab"});
	addNPCInfo({"npc_headcrab_black", ENPCThreatLevel::MEDIUM, ENPCBehavior::MELEE, 10, 100.0f, 350.0f, false, false, true, 45, "Poison Headcrab"});

	// Antlions
	addNPCInfo({"npc_antlion", ENPCThreatLevel::MEDIUM, ENPCBehavior::MELEE, 30, 200.0f, 800.0f, false, false, true, 50, "Antlion"});
	addNPCInfo({"npc_antlion_worker", ENPCThreatLevel::MEDIUM, ENPCBehavior::HYBRID, 60, 500.0f, 1000.0f, false, false, false, 55, "Antlion Worker"});
	addNPCInfo({"npc_antlionguard", ENPCThreatLevel::HIGH, ENPCBehavior::MELEE, 500, 300.0f, 1000.0f, false, true, true, 85, "Antlion Guard"});

	// Synths
	addNPCInfo({"npc_manhack", ENPCThreatLevel::LOW, ENPCBehavior::MELEE, 25, 150.0f, 500.0f, true, false, true, 35, "Manhack"});
	addNPCInfo({"npc_rollermine", ENPCThreatLevel::LOW, ENPCBehavior::MELEE, 15, 100.0f, 400.0f, false, false, true, 30, "Rollermine"});
	addNPCInfo({"npc_cscanner", ENPCThreatLevel::HARMLESS, ENPCBehavior::SPECIAL, 30, 500.0f, 800.0f, true, false, false, 10, "City Scanner"});
	addNPCInfo({"npc_clawscanner", ENPCThreatLevel::HARMLESS, ENPCBehavior::SPECIAL, 30, 500.0f, 800.0f, true, false, false, 10, "Claw Scanner"});

	// Vortigaunts (can be hostile in some maps)
	addNPCInfo({"npc_vortigaunt", ENPCThreatLevel::MEDIUM, ENPCBehavior::RANGED, 80, 700.0f, 1200.0f, false, false, false, 45, "Vortigaunt"});

	// Barnacle
	addNPCInfo({"npc_barnacle", ENPCThreatLevel::LOW, ENPCBehavior::SPECIAL, 35, 200.0f, 300.0f, false, false, false, 20, "Barnacle"});

	m_bInitialized = true;
	logger->Log(LogLevel::INFO, "NPC Database initialized with %d entries", static_cast<int>(m_NPCDatabase.size()));
}

void CNPCDatabase::addNPCInfo(const NPCInfo& info)
{
	m_NPCDatabase[info.szClassName] = info;
}

const NPCInfo* CNPCDatabase::getNPCInfo(const char* szClassName) const
{
	if (!szClassName)
		return nullptr;

	const auto it = m_NPCDatabase.find(szClassName);
	if (it != m_NPCDatabase.end())
		return &it->second;

	return nullptr;
}

bool CNPCDatabase::isHostileNPC(const char* szClassName) const
{
	const NPCInfo* pInfo = getNPCInfo(szClassName);
	if (!pInfo)
		return false;

	// Harmless NPCs are not hostile
	return pInfo->threatLevel != ENPCThreatLevel::HARMLESS;
}

std::vector<const char*> CNPCDatabase::getHostileNPCClasses() const
{
	std::vector<const char*> result;

	for (const auto& pair : m_NPCDatabase)
	{
		if (pair.second.threatLevel != ENPCThreatLevel::HARMLESS)
		{
			result.push_back(pair.first.c_str());
		}
	}

	return result;
}

std::vector<const NPCInfo*> CNPCDatabase::getNPCsByThreat(ENPCThreatLevel threat) const
{
	std::vector<const NPCInfo*> result;

	for (const auto& pair : m_NPCDatabase)
	{
		if (pair.second.threatLevel == threat)
		{
			result.push_back(&pair.second);
		}
	}

	return result;
}

bool CNPCDatabase::requiresSpecialTactics(const char* szClassName) const
{
	const NPCInfo* pInfo = getNPCInfo(szClassName);
	if (!pInfo)
		return false;

	// Armored, flying, or boss enemies require special tactics
	return pInfo->bArmored || pInfo->bFlying || pInfo->threatLevel >= ENPCThreatLevel::CRITICAL;
}

///////////////////////////////////////////////////////////////////////////////
// CNPCCombatManager - NPC Detection and Targeting
///////////////////////////////////////////////////////////////////////////////

void CNPCCombatManager::update()
{
	const float fCurrentTime = engine->Time();

	// Don't update too frequently
	if (fCurrentTime - m_fLastUpdateTime < 0.5f)
		return;

	m_fLastUpdateTime = fCurrentTime;

	// Update all tracked NPCs
	for (TrackedNPC& npc : m_TrackedNPCs)
	{
		updateNPC(npc);
	}

	// Remove dead/despawned NPCs
	cleanupDeadNPCs();
}

void CNPCCombatManager::scanForNPCs(const Vector& vOrigin, float fRadius)
{
	CNPCDatabase& db = CNPCDatabase::getInstance();
	const std::vector<const char*> hostileClasses = db.getHostileNPCClasses();

	for (const char* szClass : hostileClasses)
	{
		edict_t* pEntity = nullptr;

		while ((pEntity = CBotGlobals::findEntityByClassnameNearest(vOrigin, szClass, fRadius, pEntity)) != nullptr)
		{
			// Check if already tracking
			bool bAlreadyTracked = false;
			for (const TrackedNPC& tracked : m_TrackedNPCs)
			{
				if (tracked.hEntity.get() == pEntity)
				{
					bAlreadyTracked = true;
					break;
				}
			}

			if (!bAlreadyTracked)
			{
				// Add new NPC
				TrackedNPC newNPC;
				newNPC.hEntity = MyEHandle(pEntity);
				newNPC.vLastKnownPos = CBotGlobals::entityOrigin(pEntity);
				newNPC.fLastSeenTime = engine->Time();

				const NPCInfo* pInfo = db.getNPCInfo(pEntity->GetClassName());
				if (pInfo)
				{
					newNPC.threatLevel = pInfo->threatLevel;
					newNPC.iHealth = pInfo->iHealth;
					newNPC.fEngageDistance = pInfo->fCombatRange;
				}

				m_TrackedNPCs.push_back(newNPC);
			}
		}
	}
}

void CNPCCombatManager::updateNPC(TrackedNPC& npc)
{
	edict_t* pEntity = npc.hEntity.get();

	if (!pEntity || pEntity->IsFree())
	{
		npc.bAlive = false;
		return;
	}

	// Update position
	npc.vLastKnownPos = CBotGlobals::entityOrigin(pEntity);
	npc.fLastSeenTime = engine->Time();

	// Update health (if we can get it)
	// TODO: Get actual health from entity
	// npc.iHealth = getEntityHealth(pEntity);
}

edict_t* CNPCCombatManager::getBestNPCTarget(CBot* pBot)
{
	if (!pBot)
		return nullptr;

	const Vector vBotOrigin = pBot->getOrigin();
	edict_t* pBestTarget = nullptr;
	float fBestScore = -1.0f;

	for (const TrackedNPC& npc : m_TrackedNPCs)
	{
		if (!npc.bAlive)
			continue;

		edict_t* pEntity = npc.hEntity.get();
		if (!pEntity || pEntity->IsFree())
			continue;

		const float fScore = calculatePriorityScore(pBot, npc);

		if (fScore > fBestScore)
		{
			fBestScore = fScore;
			pBestTarget = pEntity;
		}
	}

	return pBestTarget;
}

edict_t* CNPCCombatManager::getNearestHostileNPC(const Vector& vOrigin, float fMaxDistance)
{
	edict_t* pNearest = nullptr;
	float fNearestDist = fMaxDistance;

	for (const TrackedNPC& npc : m_TrackedNPCs)
	{
		if (!npc.bAlive)
			continue;

		if (npc.threatLevel == ENPCThreatLevel::HARMLESS)
			continue;

		edict_t* pEntity = npc.hEntity.get();
		if (!pEntity || pEntity->IsFree())
			continue;

		const float fDist = (npc.vLastKnownPos - vOrigin).Length();

		if (fDist < fNearestDist)
		{
			fNearestDist = fDist;
			pNearest = pEntity;
		}
	}

	return pNearest;
}

edict_t* CNPCCombatManager::getHighestThreatNPC(const Vector& vOrigin, float fMaxDistance)
{
	edict_t* pHighestThreat = nullptr;
	ENPCThreatLevel highestLevel = ENPCThreatLevel::HARMLESS;
	float fClosestOfThreat = fMaxDistance;

	for (const TrackedNPC& npc : m_TrackedNPCs)
	{
		if (!npc.bAlive)
			continue;

		edict_t* pEntity = npc.hEntity.get();
		if (!pEntity || pEntity->IsFree())
			continue;

		const float fDist = (npc.vLastKnownPos - vOrigin).Length();

		if (fDist > fMaxDistance)
			continue;

		// Higher threat level takes priority
		if (npc.threatLevel > highestLevel)
		{
			highestLevel = npc.threatLevel;
			pHighestThreat = pEntity;
			fClosestOfThreat = fDist;
		}
		// Same threat level, prefer closer
		else if (npc.threatLevel == highestLevel && fDist < fClosestOfThreat)
		{
			pHighestThreat = pEntity;
			fClosestOfThreat = fDist;
		}
	}

	return pHighestThreat;
}

bool CNPCCombatManager::isHostileNPC(edict_t* pEntity)
{
	if (!pEntity || pEntity->IsFree())
		return false;

	const char* szClassName = pEntity->GetClassName();
	if (!szClassName)
		return false;

	return CNPCDatabase::getInstance().isHostileNPC(szClassName);
}

ENPCThreatLevel CNPCCombatManager::getThreatLevel(edict_t* pEntity)
{
	if (!pEntity || pEntity->IsFree())
		return ENPCThreatLevel::HARMLESS;

	const NPCInfo* pInfo = CNPCDatabase::getInstance().getNPCInfo(pEntity->GetClassName());
	if (!pInfo)
		return ENPCThreatLevel::HARMLESS;

	return pInfo->threatLevel;
}

float CNPCCombatManager::getCombatDistance(edict_t* pEntity)
{
	if (!pEntity)
		return 500.0f;

	const NPCInfo* pInfo = CNPCDatabase::getInstance().getNPCInfo(pEntity->GetClassName());
	if (!pInfo)
		return 500.0f;

	return pInfo->fCombatRange;
}

bool CNPCCombatManager::shouldRetreat(CBot* pBot, edict_t* pNPC)
{
	if (!pBot || !pNPC)
		return false;

	const ENPCThreatLevel threat = getThreatLevel(pNPC);

	// Always retreat from boss/critical enemies if low health
	if (threat >= ENPCThreatLevel::CRITICAL && pBot->getHealthPercent() < 0.5f)
		return true;

	// Retreat from high threat if very low health
	if (threat >= ENPCThreatLevel::HIGH && pBot->getHealthPercent() < 0.3f)
		return true;

	return false;
}

int CNPCCombatManager::getRecommendedWeapon(edict_t* pNPC)
{
	// TODO: Return weapon ID based on NPC type
	// For now, return 0 (any weapon)
	return 0;
}

void CNPCCombatManager::clear()
{
	m_TrackedNPCs.clear();
}

int CNPCCombatManager::getNPCCount(const Vector& vOrigin, float fRadius, ENPCThreatLevel minThreat)
{
	int iCount = 0;

	for (const TrackedNPC& npc : m_TrackedNPCs)
	{
		if (!npc.bAlive)
			continue;

		if (npc.threatLevel < minThreat)
			continue;

		const float fDist = (npc.vLastKnownPos - vOrigin).Length();

		if (fDist <= fRadius)
			iCount++;
	}

	return iCount;
}

void CNPCCombatManager::markAsPriority(edict_t* pNPC)
{
	if (!pNPC)
		return;

	for (TrackedNPC& npc : m_TrackedNPCs)
	{
		if (npc.hEntity.get() == pNPC)
		{
			npc.bPriorityTarget = true;
			break;
		}
	}
}

int CNPCCombatManager::getActiveThreats() const
{
	int iCount = 0;

	for (const TrackedNPC& npc : m_TrackedNPCs)
	{
		if (npc.bAlive && npc.threatLevel >= ENPCThreatLevel::MEDIUM)
			iCount++;
	}

	return iCount;
}

float CNPCCombatManager::calculatePriorityScore(CBot* pBot, const TrackedNPC& npc)
{
	if (!pBot)
		return 0.0f;

	const Vector vBotOrigin = pBot->getOrigin();
	const float fDistance = (npc.vLastKnownPos - vBotOrigin).Length();

	// Base score from NPC database priority
	const NPCInfo* pInfo = CNPCDatabase::getInstance().getNPCInfo(npc.hEntity.get()->GetClassName());
	float fScore = pInfo ? static_cast<float>(pInfo->iPriority) : 50.0f;

	// Boost score for priority targets
	if (npc.bPriorityTarget)
		fScore *= 1.5f;

	// Reduce score based on distance (closer = higher priority)
	fScore *= (2000.0f / std::max(fDistance, 100.0f));

	// Boost score for higher threat levels
	fScore *= (1.0f + static_cast<float>(npc.threatLevel) * 0.3f);

	return fScore;
}

void CNPCCombatManager::cleanupDeadNPCs()
{
	m_TrackedNPCs.erase(
		std::remove_if(m_TrackedNPCs.begin(), m_TrackedNPCs.end(),
			[](const TrackedNPC& npc) {
				return !npc.bAlive || !npc.hEntity.get() || npc.hEntity.get()->IsFree();
			}),
		m_TrackedNPCs.end()
	);
}

///////////////////////////////////////////////////////////////////////////////
// CNPCWaypointManager - Combat Waypoint Integration
///////////////////////////////////////////////////////////////////////////////

int CNPCWaypointManager::generateCombatWaypoints()
{
	int iGenerated = 0;

	// Scan for combat areas
	// TODO: Implement combat waypoint generation

	logger->Log(LogLevel::INFO, "Generated %d combat waypoints", iGenerated);
	return iGenerated;
}

int CNPCWaypointManager::findCombatPosition(const Vector& vNPCOrigin, const Vector& vBotOrigin, float fDistance)
{
	// Find waypoint at desired distance from NPC
	int iBestWpt = -1;
	float fBestScore = 9999.0f;

	for (int i = 0; i < CWaypoints::numWaypoints(); i++)
	{
		CWaypoint* pWpt = CWaypoints::getWaypoint(i);
		if (!pWpt || !pWpt->isUsed())
			continue;

		const Vector vWptOrigin = pWpt->getOrigin();
		const float fNPCDist = pWpt->distanceFrom(vNPCOrigin);
		const float fBotDist = pWpt->distanceFrom(vBotOrigin);

		// Score based on how close to desired distance
		const float fDistScore = std::abs(fNPCDist - fDistance);
		const float fTotalScore = fDistScore + fBotDist * 0.5f;

		if (fTotalScore < fBestScore)
		{
			fBestScore = fTotalScore;
			iBestWpt = i;
		}
	}

	return iBestWpt;
}

int CNPCWaypointManager::findCoverFromNPC(const Vector& vNPCOrigin, const Vector& vBotOrigin)
{
	// Find waypoint with cover flag near bot
	const int iNearWpt = CWaypointLocations::NearestWaypoint(vBotOrigin, 500.0f, -1, true, false, false, nullptr);

	if (iNearWpt != -1)
	{
		CWaypoint* pWpt = CWaypoints::getWaypoint(iNearWpt);
		if (pWpt && pWpt->hasFlag(CWaypointTypes::W_FL_DEFEND))
		{
			return iNearWpt;
		}
	}

	return -1;
}

void CNPCWaypointManager::markCombatZone(const Vector& vOrigin, float fRadius)
{
	CombatZone zone;
	zone.vOrigin = vOrigin;
	zone.fRadius = fRadius;
	zone.fCreationTime = engine->Time();

	m_CombatZones.push_back(zone);
}

bool CNPCWaypointManager::isInCombatZone(int iWaypointIndex)
{
	CWaypoint* pWpt = CWaypoints::getWaypoint(iWaypointIndex);
	if (!pWpt)
		return false;

	const Vector vWptOrigin = pWpt->getOrigin();

	for (const CombatZone& zone : m_CombatZones)
	{
		const float fDist = (vWptOrigin - zone.vOrigin).Length();
		if (fDist <= zone.fRadius)
			return true;
	}

	return false;
}

int CNPCWaypointManager::getRetreatWaypoint(const Vector& vNPCOrigin, const Vector& vBotOrigin)
{
	// Find waypoint away from NPC
	int iBestWpt = -1;
	float fBestDist = 0.0f;

	for (int i = 0; i < CWaypoints::numWaypoints(); i++)
	{
		CWaypoint* pWpt = CWaypoints::getWaypoint(i);
		if (!pWpt || !pWpt->isUsed())
			continue;

		const Vector vWptOrigin = pWpt->getOrigin();
		const float fNPCDist = pWpt->distanceFrom(vNPCOrigin);
		const float fBotDist = pWpt->distanceFrom(vBotOrigin);

		// Want to be far from NPC, but not too far from current position
		if (fNPCDist > fBestDist && fBotDist < 1000.0f)
		{
			fBestDist = fNPCDist;
			iBestWpt = i;
		}
	}

	return iBestWpt;
}
