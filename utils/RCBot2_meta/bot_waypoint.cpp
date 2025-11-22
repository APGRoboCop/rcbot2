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

#include "engine_wrappers.h"

#include "filesystem.h"

#include "iplayerinfo.h"

#include "ndebugoverlay.h"

#include "bot.h"
#include "bot_cvars.h"

#include "in_buttons.h"

#include "bot_client.h"
#include "bot_fortress.h"
#include "bot_getprop.h"
#include "bot_globals.h"
#include "bot_navigator.h"
#include "bot_profile.h"
#include "bot_schedule.h"
#include "bot_waypoint.h"
#include "bot_waypoint_locations.h"
#include "bot_waypoint_visibility.h"
#include "bot_wpt_color.h"
#include "bot_wpt_dist.h"

#include "rcbot/logging.h"
#include "rcbot/utils.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>    //bir3yk

 //caxanga334: SDK 2013 contains macros for std::min and std::max which causes errors when compiling
//#if SOURCE_ENGINE == SE_SDK2013 || SOURCE_ENGINE == SE_BMS
#include "valve_minmax_off.h"
//#endif

int CWaypoints::m_iNumWaypoints = 0;
CWaypoint CWaypoints::m_theWaypoints[MAX_WAYPOINTS];
float CWaypoints::m_fNextDrawWaypoints = 0.0f;
int CWaypoints::m_iWaypointTexture = 0;
CWaypointVisibilityTable * CWaypoints::m_pVisibilityTable = nullptr;
std::vector<CWaypointType*> CWaypointTypes::m_Types;
char CWaypoints::m_szAuthor[32];
char CWaypoints::m_szModifiedBy[32];
char CWaypoints::m_szWelcomeMessage[128];
const WptColor WptColor::white = WptColor(255,255,255,255) ;

extern IVDebugOverlay *debugoverlay;

///////////////////////////////////////////////////////////////
// initialise
void CWaypointNavigator::init()
{
	m_pBot = nullptr;

	m_vOffset = Vector(0, 0, 0);
	m_bOffsetApplied = false;

	m_iCurrentWaypoint = -1;
	m_iNextWaypoint = -1;
	m_iGoalWaypoint = -1;

	std::stack<int>().swap(m_currentRoute);
	std::queue<int>().swap(m_oldRoute);

	m_iLastFailedWpt = -1;
	m_iPrevWaypoint = -1;
	m_bWorkingRoute = false;

	std::memset(m_fBelief, 0, sizeof(float) * CWaypoints::MAX_WAYPOINTS);

	m_iFailedGoals.clear();
}

bool CWaypointNavigator::beliefLoad()
{
	unsigned short int filebelief[CWaypoints::MAX_WAYPOINTS];

	char filename[1024];

	char mapname[512];

	m_bLoadBelief = false;
	m_iBeliefTeam = static_cast<short>(m_pBot->getTeam());

	snprintf(mapname, sizeof(mapname), "%s%d", CBotGlobals::getMapName(), m_iBeliefTeam);

	CBotGlobals::buildFileName(filename, mapname, BOT_AUXILERY_FOLDER, "rcb", true);

	std::fstream bfp(filename, std::ios::in | std::ios::binary);

	if (!bfp)
	{
		logger->Log(LogLevel::ERROR, "Can't open Waypoint belief array for reading!");
		return false;
	}

	bfp.seekg(0, std::ios::end); // seek at end

	const std::size_t iSize = bfp.tellg(); // get file size
	const std::size_t iDesiredSize = static_cast<std::size_t>(CWaypoints::numWaypoints()) * sizeof(unsigned short int);

	// size not right, return false to re workout table
	if (iSize != iDesiredSize)
	{
		return false;
	}

	bfp.seekg(0, std::ios::beg); // seek at start

	std::memset(filebelief, 0, sizeof(unsigned short int) * CWaypoints::MAX_WAYPOINTS);

	bfp.read(reinterpret_cast<char*>(filebelief), static_cast<std::streamsize>(sizeof(unsigned short)) * CWaypoints::numWaypoints());

	// convert from short int to float

	const unsigned short int num = static_cast<unsigned short>(CWaypoints::numWaypoints());

	// quick loop
	for (unsigned short int i = 0; i < num; i++)
	{
	   m_fBelief[i] = static_cast<float>(filebelief[i])/32767 * MAX_BELIEF;
	}

	return true;
}

// update belief array with averaged belief for this team
bool CWaypointNavigator::beliefSave(const bool bOverride)
{
	unsigned short int filebelief[CWaypoints::MAX_WAYPOINTS];
	char filename[1024];
	char mapname[512];

	if (m_pBot->getTeam() == m_iBeliefTeam && !bOverride)
		return false;

	std::memset(filebelief, 0, sizeof(unsigned short int) * CWaypoints::MAX_WAYPOINTS);

	// m_iBeliefTeam is the team we've been using -- we might have changed team now
	// so would need to change files if a different team
	// stick to the current team we've been using
	snprintf(mapname, sizeof(mapname), "%s%d", CBotGlobals::getMapName(), m_iBeliefTeam);
	CBotGlobals::buildFileName(filename, mapname, BOT_AUXILERY_FOLDER, "rcb", true);

	{
		std::fstream bfp(filename, std::ios::in | std::ios::binary);

		if (bfp)
		{
			bfp.seekg(0, std::ios::end); // seek at end

			const std::size_t iSize = bfp.tellg(); // get file size
			const std::size_t iDesiredSize = static_cast<std::size_t>(CWaypoints::numWaypoints()) * sizeof(unsigned short int);

			// size not right, return false to re workout table
			if (iSize == iDesiredSize)
			{
				bfp.seekg(0, std::ios::beg); // seek at start

				bfp.read(reinterpret_cast<char*>(filebelief), static_cast<std::streamsize>(sizeof(unsigned short)) * CWaypoints::numWaypoints());
			}
		}
	}

	std::fstream bfp(filename, std::ios::out | std::ios::binary);

	if (!bfp)
	{
		m_bLoadBelief = true;
		m_iBeliefTeam = m_pBot->getTeam();
		logger->Log(LogLevel::ERROR, "Can't open Waypoint Belief array for writing!");
		return false;
	}

	// convert from short int to float

	const unsigned short int num = static_cast<unsigned short>(CWaypoints::numWaypoints());

	// quick loop
   for ( unsigned short int i = 0; i < num; i ++ )
   {
	   filebelief[i] = filebelief[i]/2 + static_cast<unsigned short>(m_fBelief[i] / MAX_BELIEF * 16383); 
   }

	bfp.seekg(0, std::ios::beg); // seek at start

	bfp.write(reinterpret_cast<char*>(filebelief), static_cast<std::streamsize>(sizeof(unsigned short)) * num);

	// new team -- load belief 
	m_iBeliefTeam = m_pBot->getTeam();
	m_bLoadBelief = true;
	m_bBeliefChanged = false; // saved

	return true;
}

bool CWaypointNavigator::wantToSaveBelief()
{
	// playing on this map for more than a normal load time
	return m_bBeliefChanged && m_iBeliefTeam != m_pBot->getTeam();
}

int CWaypointNavigator :: numPaths ()
{
	if ( m_iCurrentWaypoint != -1 )
		return CWaypoints::getWaypoint(m_iCurrentWaypoint)->numPaths();

	return 0;
}

bool CWaypointNavigator :: randomDangerPath (Vector *vec)
{
	float fMaxDanger = 0.0f;
	float fBelief;
	int i;
	CWaypoint *pNext;
	const CWaypoint *pOnRouteTo = nullptr;

	if ( m_iCurrentWaypoint == -1 )
		return false;

	if ( !m_currentRoute.empty() )
	{
		const int head = m_currentRoute.top();
		static CWaypoint *pW; //Unused? [APG]RoboCop[CL]

		if (head != -1)
		{
			pOnRouteTo = CWaypoints::getWaypoint(head);
		}
	}

	const CWaypoint* pWpt = CWaypoints::getWaypoint(m_iCurrentWaypoint);

	if ( pWpt == nullptr)
		return false;

	float fTotal = 0.0f;

	for (const int iPathIndex : *pWpt)
	{
		pNext = CWaypoints::getWaypoint(iPathIndex);
		fBelief = getBelief(CWaypoints::getWaypointIndex(pNext));

		if ( pNext == pOnRouteTo )
			fBelief *= static_cast<float>(pWpt->numPaths());

		fMaxDanger = std::max(fBelief, fMaxDanger);

		fTotal += fBelief;
	}

	if ( fMaxDanger < 10 )
		return false; // not useful enough

	const float fRand = randomFloat(0, fTotal);

	for (const int iPathIndex : *pWpt)
	{
		pNext = CWaypoints::getWaypoint(iPathIndex);
		fBelief = getBelief(CWaypoints::getWaypointIndex(pNext));

		if ( pNext == pOnRouteTo )
			fBelief *= static_cast<float>(pWpt->numPaths());

		fTotal += fBelief;

		if ( fRand < fTotal )
		{
			*vec = pNext->getOrigin();
			return true;
		}
	}

	return false;

}

Vector CWaypointNavigator :: getPath (const int pathid)
{
	 return CWaypoints::getWaypoint(CWaypoints::getWaypoint(m_iCurrentWaypoint)->getPath(pathid))->getOrigin();
}


int CWaypointNavigator ::  getPathFlags (const int iPath)
{
	const CWaypoint *pWpt = CWaypoints::getWaypoint(m_iCurrentWaypoint);

	return CWaypoints::getWaypoint(pWpt->getPath(iPath))->getFlags();
}

bool CWaypointNavigator::nextPointIsOnLadder()
{
	if (m_iCurrentWaypoint != -1)
	{			
		CWaypoint *pWaypoint;

		if ( (pWaypoint = CWaypoints::getWaypoint(m_iCurrentWaypoint)) != nullptr)
		{
			return pWaypoint->hasFlag(CWaypointTypes::W_FL_LADDER);
		}
	}

	return false;
}

float CWaypointNavigator :: getNextYaw ()
{
	if ( m_iCurrentWaypoint != -1 )
		return CWaypoints::getWaypoint(m_iCurrentWaypoint)->getAimYaw();

	return 0.0f;
}

// best waypoints are those with lowest danger
CWaypoint *CWaypointNavigator :: chooseBestFromBeliefBetweenAreas ( const std::vector<AStarNode*> &goals, const bool bHighDanger, const bool bIgnoreBelief ) const
{
	CWaypoint *pWpt = nullptr;
	//CWaypoint *pCheck;

	// simple checks
	switch ( goals.size() )
	{
	case 0:return nullptr;
	case 1:return CWaypoints::getWaypoint(goals[0]->getWaypoint());
	default:
		{
			float fBelief = 0.0f;
			AStarNode *node;

			for (AStarNode* const goal : goals)
			{
				node = goal;

				if ( bIgnoreBelief )
				{
					if ( bHighDanger )
						fBelief += node->getHeuristic();
					else
						fBelief += 131072.0f - node->getHeuristic();
				}
				else if ( bHighDanger )
					fBelief += m_fBelief[node->getWaypoint()] + node->getHeuristic();
				else
					fBelief += MAX_BELIEF - m_fBelief[node->getWaypoint()] + (131072.0f - node->getHeuristic());
			}

			const float fSelect = randomFloat(0, fBelief);

			fBelief = 0.0f;
			
			for (AStarNode* const goal : goals)
			{
				node = goal;

				if ( bIgnoreBelief )
				{
					if ( bHighDanger )
						fBelief += node->getHeuristic();
					else
						fBelief += 131072.0f - node->getHeuristic();
				}
				else if ( bHighDanger )
					fBelief += m_fBelief[node->getWaypoint()] + node->getHeuristic();
				else
					fBelief += MAX_BELIEF - m_fBelief[node->getWaypoint()] + (131072.0f - node->getHeuristic());

				if ( fSelect <= fBelief )
				{
					pWpt = CWaypoints::getWaypoint(node->getWaypoint());
					break;
				}
			}

			if (!goals.empty() && pWpt == nullptr)
				pWpt = CWaypoints::getWaypoint(goals[static_cast<std::size_t>(randomInt(0, static_cast<int>(goals.size()) - 1))]->getWaypoint());
		}
	}
		
	return pWpt;
}

// best waypoints are those with lowest danger
CWaypoint *CWaypointNavigator :: chooseBestFromBelief ( const std::vector<CWaypoint*> &goals, const bool bHighDanger, const int iSearchFlags, const int iTeam ) const
{
	CWaypoint *pWpt = nullptr;

	// simple checks
	switch ( goals.size() )
	{
	case 0:return nullptr;
	case 1:return goals[0];
	default:
		{
			float fBelief = 0.0f;
			float bBeliefFactor = 1.0f;
			for (CWaypoint* goal : goals)
			{
				//bBeliefFactor = 1.0f;

				if ( iSearchFlags & WPT_SEARCH_AVOID_SENTRIES )
				{
					for ( int j = 1; j <= gpGlobals->maxClients; j ++ )
					{
						edict_t *pSentry = CTeamFortress2Mod::getSentryGun(j-1);

						if ( pSentry != nullptr)
						{
							if (goal->distanceFrom(CBotGlobals::entityOrigin(pSentry)) < 200.0f )
							{
								bBeliefFactor *= 0.1f;
							}
						}
					}
				}

				if ( iSearchFlags & WPT_SEARCH_AVOID_SNIPERS )
				{
					for ( int j = 1; j <= gpGlobals->maxClients; j ++ )
					{
						/**
						* So this was using 'i' instead of 'j' and was basically broken since forever -caxanga334
						*/
						edict_t* pPlayer = INDEXENT(j);

						if ( pPlayer != nullptr && !pPlayer->IsFree() && CClassInterface::getTF2Class(pPlayer)==TF_CLASS_SNIPER )
						{
							if ( iTeam == 0 || iTeam == CClassInterface::getTeam(pPlayer) )
							{
								if (goal->distanceFrom(CBotGlobals::entityOrigin(pPlayer)) < 200.0f )
								{
									bBeliefFactor *= 0.1f;
								}
							}
						}
					}
				}

				if ( iSearchFlags & WPT_SEARCH_AVOID_TEAMMATE )
				{
					for ( int j = 1; j <= gpGlobals->maxClients; j ++ )
					{
						edict_t* pPlayer = INDEXENT(j);

						if ( pPlayer != nullptr && !pPlayer->IsFree() )
						{
							if ( iTeam == 0 || iTeam == CClassInterface::getTeam(pPlayer) )
							{
								if (goal->distanceFrom(CBotGlobals::entityOrigin(pPlayer)) < 200.0f )
								{
									bBeliefFactor *= 0.1f;
								}
							}
						}
					}
				}

				if ( bHighDanger )
				{
					fBelief += bBeliefFactor * (1.0f + m_fBelief[CWaypoints::getWaypointIndex(goal)]);	
				}
				else
				{
					fBelief += bBeliefFactor * (1.0f + (MAX_BELIEF - m_fBelief[CWaypoints::getWaypointIndex(goal)]));
				}
			}

			const float fSelect = randomFloat(0, fBelief);

			fBelief = 0.0f;
			
			for (CWaypoint* goal : goals)
			{
				CWaypoint* pCheck = goal;

				bBeliefFactor = 1.0f;

				if ( iSearchFlags & WPT_SEARCH_AVOID_SENTRIES )
				{
					for ( int j = 0; j < RCBOT_MAXPLAYERS; j ++ )
					{
						edict_t *pSentry = CTeamFortress2Mod::getSentryGun(j);

						if ( pSentry != nullptr)
						{
							if (goal->distanceFrom(CBotGlobals::entityOrigin(pSentry)) < 200.0f )
							{
								bBeliefFactor *= 0.1f;
							}
						}
					}
				}

				if ( iSearchFlags & WPT_SEARCH_AVOID_SNIPERS )
				{
					for ( int j = 1; j <= gpGlobals->maxClients; j ++ )
					{
						edict_t* pPlayer = INDEXENT(j);

						if ( pPlayer != nullptr && !pPlayer->IsFree() && CClassInterface::getTF2Class(pPlayer)==TF_CLASS_SNIPER )
						{
							if (goal->distanceFrom(CBotGlobals::entityOrigin(pPlayer)) < 200.0f )
							{
								bBeliefFactor *= 0.1f;
							}
						}
					}
				}

				if ( iSearchFlags & WPT_SEARCH_AVOID_TEAMMATE )
				{
					for ( int j = 1; j <= gpGlobals->maxClients; j ++ )
					{
						edict_t* pPlayer = INDEXENT(j);

						if ( pPlayer != nullptr && !pPlayer->IsFree() )
						{
							if (goal->distanceFrom(CBotGlobals::entityOrigin(pPlayer)) < 200.0f )
							{
								bBeliefFactor *= 0.1f;
							}
						}
					}
				}

				if ( bHighDanger )
				{
					fBelief += bBeliefFactor * (1.0f + m_fBelief[CWaypoints::getWaypointIndex(goal)]);
				}
				else
				{
					fBelief += bBeliefFactor * (1.0f + (MAX_BELIEF - m_fBelief[CWaypoints::getWaypointIndex(goal)]));
				}

				if ( fSelect <= fBelief )
				{
					pWpt = pCheck;
					break;
				}
			}

			if (!goals.empty() && pWpt == nullptr)
				pWpt = goals[static_cast<std::size_t>(randomInt(0, static_cast<int>(goals.size()) - 1))];
		}
	}
		
	return pWpt;
}

// get the covering waypoint vector vCover
bool CWaypointNavigator::getCoverPosition(const Vector& vCoverOrigin, Vector* vCover)
{
	const int iWpt = CWaypointLocations::GetCoverWaypoint(m_pBot->getOrigin(), vCoverOrigin, nullptr);

	CWaypoint* pWaypoint = CWaypoints::getWaypoint(iWpt);

	if (pWaypoint == nullptr)
		return false;

	*vCover = pWaypoint->getOrigin();

	return true;
}

void CWaypointNavigator :: beliefOne (const int iWptIndex, const BotBelief iBeliefType, const float fDist)
{
	if ( iBeliefType == BELIEF_SAFETY )
	{
		if ( m_fBelief[iWptIndex] > 0)
			m_fBelief[iWptIndex] *= bot_belief_fade.GetFloat();
		m_fBelief[iWptIndex] = std::max<float>(m_fBelief[iWptIndex], 0);
	}
	else // danger	
	{
		if ( m_fBelief[iWptIndex] < MAX_BELIEF )
			m_fBelief[iWptIndex] += 2048.0f / fDist;
		m_fBelief[iWptIndex] = std::min(m_fBelief[iWptIndex], MAX_BELIEF);
	}

	m_bBeliefChanged = true;
}

// get belief nearest to current origin using waypoints to store belief
void CWaypointNavigator :: belief (const Vector& vOrigin, const Vector& vOther, const float fBelief,
								   const float fStrength, const BotBelief iType)
{
	static float factor; //Unused? [APG]RoboCop[CL]
	static float fEDist;
	static int iWptIndex;
	CWaypoint *pWpt;
	WaypointList m_iVisibles;
	WaypointList m_iInvisibles;
	static int iWptFrom;
	static int iWptTo;

	// get nearest waypoint visible to others
	iWptFrom = CWaypointLocations::NearestWaypoint(vOrigin,2048.0f,-1,true,true,false, nullptr,false,0,false,true,vOther);
	iWptTo = CWaypointLocations::NearestWaypoint(vOther,2048.0f,-1,true,true,false, nullptr,false,0,false,true,vOrigin);

	// no waypoint information
	if ( iWptFrom == -1 || iWptTo == -1 )
		return;

	fEDist = (vOrigin-vOther).Length(); // range

	m_iVisibles.emplace_back(iWptFrom);
	m_iVisibles.emplace_back(iWptTo);

	//TODO: duplicates? [APG]RoboCop[CL]
	CWaypointLocations::GetAllVisible(iWptFrom,iWptTo,vOrigin,vOther,fEDist,&m_iVisibles,&m_iInvisibles);
	//CWaypointLocations::GetAllVisible(iWptFrom,iWptTo,vOther,vOrigin,fEDist,&m_iVisibles,&m_iInvisibles);

	for (const int m_iVisible : m_iVisibles)
	{
		pWpt = CWaypoints::getWaypoint(m_iVisible);
		iWptIndex = CWaypoints::getWaypointIndex(pWpt);

		if ( iType == BELIEF_SAFETY )
		{
			if ( m_fBelief[iWptIndex] > 0)
				m_fBelief[iWptIndex] *= bot_belief_fade.GetFloat();//(fStrength / (vOrigin-pWpt->getOrigin()).Length())*fBelief;
			m_fBelief[iWptIndex] = std::max<float>(m_fBelief[iWptIndex], 0);

			//debugoverlay->AddTextOverlayRGB(pWpt->getOrigin(),0,5.0f,0.0,150,0,200,"Safety");
		}
		else if ( iType == BELIEF_DANGER )
		{
			if ( m_fBelief[iWptIndex] < MAX_BELIEF )
				m_fBelief[iWptIndex] += fStrength / (vOrigin-pWpt->getOrigin()).Length()*fBelief;
			m_fBelief[iWptIndex] = std::min(m_fBelief[iWptIndex], MAX_BELIEF);

			//debugoverlay->AddTextOverlayRGB(pWpt->getOrigin(),0,5.0f,255,0,0,200,"Danger %0.2f",m_fBelief[iWptIndex]);
		}
	}

	for (const int m_iInvisible : m_iInvisibles)
	{
		pWpt = CWaypoints::getWaypoint(m_iInvisible);
		iWptIndex = CWaypoints::getWaypointIndex(pWpt);

		// this waypoint is safer from this danger
		if ( iType == BELIEF_DANGER )
		{
			if ( m_fBelief[iWptIndex] > 0)
				m_fBelief[iWptIndex] *= 0.9f;//(fStrength / (vOrigin-pWpt->getOrigin()).Length())*fBelief;

			//debugoverlay->AddTextOverlayRGB(pWpt->getOrigin(),1,5.0f,0.0,150,0,200,"Safety INV");
		}
		else if ( iType == BELIEF_SAFETY )
		{
			if ( m_fBelief[iWptIndex] < MAX_BELIEF )
				m_fBelief[iWptIndex] += fStrength / (vOrigin-pWpt->getOrigin()).Length()*fBelief*0.5f;
			if ( m_fBelief[iWptIndex] > MAX_BELIEF )
				m_fBelief[iWptIndex] = MAX_BELIEF;

			//debugoverlay->AddTextOverlayRGB(pWpt->getOrigin(),1,5.0f,255,0,0,200,"Danger INV %0.2f",m_fBelief[iWptIndex]);
		}
	}
	
/*
	i = m_oldRoute.size();

	while ( !m_oldRoute.empty() )
	{
		iWptIndex = m_oldRoute.front();

		factor = ((float)i)/m_oldRoute.size();
		i--;

		if ( iWptIndex >= 0 )
		{
			if ( iType == BELIEF_SAFETY )
			{
				if ( m_fBelief[iWptIndex] > 0)
					m_fBelief[iWptIndex] *= bot_belief_fade.GetFloat()*factor;//(fStrength / (vOrigin-pWpt->getOrigin()).Length())*fBelief;
				if ( m_fBelief[iWptIndex] < 0 )
					m_fBelief[iWptIndex] = 0;
			}
			else if ( iType == BELIEF_DANGER )
			{
				if ( m_fBelief[iWptIndex] < MAX_BELIEF )
					m_fBelief[iWptIndex] += factor*fBelief;
				if ( m_fBelief[iWptIndex] > MAX_BELIEF )
					m_fBelief[iWptIndex] = MAX_BELIEF;
			}
		}

		m_oldRoute.pop();
	}*/

	m_iVisibles.clear();
	m_iInvisibles.clear();

	m_bBeliefChanged = true;
}

int CWaypointNavigator :: getCurrentFlags ()
{
	if ( m_iCurrentWaypoint != -1 )
		return CWaypoints::getWaypoint(m_iCurrentWaypoint)->getFlags();

	return 0;
}

float CWaypointNavigator :: getCurrentBelief ( )
{
	if ( m_iCurrentWaypoint >= 0 )
	{
		return m_fBelief[m_iCurrentWaypoint];
	}

	return 0;
}
/*
bool CWaypointNavigator :: getCrouchHideSpot ( Vector vCoverOrigin, Vector *vCover )
{
	
}
*/
// get the hide spot position (vCover) from origin vCoverOrigin
bool CWaypointNavigator :: getHideSpotPosition (const Vector& vCoverOrigin, Vector *vCover)
{
	int iWpt;

	if ( m_pBot->hasGoal() )
		iWpt = CWaypointLocations::GetCoverWaypoint(m_pBot->getOrigin(),vCoverOrigin, nullptr,m_pBot->getGoalOrigin());
	else
		iWpt = CWaypointLocations::GetCoverWaypoint(m_pBot->getOrigin(),vCoverOrigin, nullptr);

	CWaypoint *pWaypoint = CWaypoints::getWaypoint(iWpt);
	
	if ( pWaypoint == nullptr)
		return false;
	
	*vCover = pWaypoint->getOrigin();

	return true;
}
// AStar Algorithm : open a waypoint
void CWaypointNavigator :: open ( AStarNode *pNode )
{ 
	if ( !pNode->isOpen() )
	{
		pNode->open();
		//m_theOpenList.emplace_back(pNode);
		m_theOpenList.add(pNode);
	}
}
// AStar Algorithm : get the waypoint with lowest cost
AStarNode *CWaypointNavigator :: nextNode ()
{
	AStarNode* pNode = m_theOpenList.top();
	m_theOpenList.pop();
		
	return pNode;
}

// clears the AStar open list
void CWaypointNavigator :: clearOpenList ()
{
	m_theOpenList.destroy();
	
	//for ( unsigned i = 0; i < m_theOpenList.size(); i ++ )
	//	m_theOpenList[i]->unOpen();

	//m_theOpenList.clear();
}

void CWaypointNavigator :: failMove ()
{
	m_iLastFailedWpt = m_iCurrentWaypoint;

	m_lastFailedPath.bValid = true;
	m_lastFailedPath.iFrom = m_iPrevWaypoint;
	m_lastFailedPath.iTo = m_iCurrentWaypoint;
	m_lastFailedPath.bSkipped = false;

	if ( std::find(m_iFailedGoals.begin(), m_iFailedGoals.end(), m_iGoalWaypoint) == m_iFailedGoals.end() )
	{
		m_iFailedGoals.emplace_back(m_iGoalWaypoint);
		m_fNextClearFailedGoals = engine->Time() + randomFloat(8.0f,30.0f);
	}
}

float CWaypointNavigator :: distanceTo (const Vector& vOrigin)
{
	if ( m_iCurrentWaypoint == -1 )
		m_iCurrentWaypoint = CWaypointLocations::NearestWaypoint(m_pBot->getOrigin(),CWaypointLocations::REACHABLE_RANGE,-1,true,false,true, nullptr,false,m_pBot->getTeam());
	
	if ( m_iCurrentWaypoint != -1 )
	{
		const int iGoal = CWaypointLocations::NearestWaypoint(vOrigin, CWaypointLocations::REACHABLE_RANGE, -1, true, false, true,
															  nullptr, false, m_pBot->getTeam());

		if ( iGoal != -1 )
			return CWaypointDistances::getDistance(m_iCurrentWaypoint,iGoal);
	}
		
	return m_pBot->distanceFrom(vOrigin);
}

float CWaypointNavigator :: distanceTo ( CWaypoint *pWaypoint )
{
	return distanceTo(pWaypoint->getOrigin());
}

// find route using A* algorithm
bool CWaypointNavigator :: workRoute (const Vector& vFrom,
									  const Vector& vTo, 
									  bool *bFail,
									  const bool bRestart,
									  const bool bNoInterruptions,
									  const int iGoalId,
									  const int iConditions, const int iDangerId)
{
	if ( bRestart )
	{
		if ( wantToSaveBelief() )
			beliefSave();
		if ( wantToLoadBelief() )
			beliefLoad();

		*bFail = false;

		m_bWorkingRoute = true;

		if ( iGoalId == -1 )
			m_iGoalWaypoint = CWaypointLocations::NearestWaypoint(vTo,CWaypointLocations::REACHABLE_RANGE,m_iLastFailedWpt,true,false,true,&m_iFailedGoals,false,m_pBot->getTeam());
		else
			m_iGoalWaypoint = iGoalId;

		const CWaypoint* pGoalWaypoint = CWaypoints::getWaypoint(m_iGoalWaypoint);

		if ( CClients::clientsDebugging(BOT_DEBUG_NAV) )
		{
			char str[64];

			snprintf(str, sizeof(str), "goal waypoint = %d", m_iGoalWaypoint);

			CClients::clientDebugMsg(BOT_DEBUG_NAV,str,m_pBot);

		}

		if ( m_iGoalWaypoint == -1 )
		{
			*bFail = true;
			m_bWorkingRoute = false;
			return true;
		}
			
		m_vPreviousPoint = vFrom;
		// get closest waypoint -- ignore previous failed waypoint
		Vector vIgnore;
		float fIgnoreSize;

		const bool bIgnore = m_pBot->getIgnoreBox(&vIgnore,&fIgnoreSize) && pGoalWaypoint->distanceFrom(vFrom) > fIgnoreSize*2;

		m_iCurrentWaypoint = CWaypointLocations::NearestWaypoint(vFrom,CWaypointLocations::REACHABLE_RANGE,m_iLastFailedWpt,
			true,false,true, nullptr,false,m_pBot->getTeam(),true,false,vIgnore,0, nullptr,bIgnore,fIgnoreSize);

		// no nearest waypoint -- find nearest waypoint
		if ( m_iCurrentWaypoint == -1 )
		{
			// don't ignore this time
			m_iCurrentWaypoint = CWaypointLocations::NearestWaypoint(vFrom,CWaypointLocations::REACHABLE_RANGE,-1,true,false,true, nullptr,false,m_pBot->getTeam(),false,false,Vector(0,0,0),0,m_pBot->getEdict());

			if ( m_iCurrentWaypoint == -1 )
			{
				*bFail = true;
				m_bWorkingRoute = false;
				return true;
			}
		}

		// reset
		m_iLastFailedWpt = -1;

		clearOpenList();
		Q_memset(paths, 0, sizeof(AStarNode) * CWaypoints::MAX_WAYPOINTS);

		AStarNode* currentNode = &paths[m_iCurrentWaypoint];
		currentNode->setWaypoint(m_iCurrentWaypoint);
		currentNode->setHeuristic(m_pBot->distanceFrom(vTo));
		open(currentNode);
	}
/////////////////////////////////
	if ( m_iGoalWaypoint == -1 )
	{
		*bFail = true;
		m_bWorkingRoute = false;
		return true;
	}
	if ( m_iCurrentWaypoint == -1 )
	{
		*bFail = true;
		m_bWorkingRoute = false;
		return true;
	}
///////////////////////////////

	int iLoops = 0;
	//int iMaxLoops = this->m_pBot->getProfile()->getPathTicks(); // bot_pathrevs.GetInt(); //IBotNavigator::MAX_PATH_TICKS; - DNA.styx
	int iMaxLoops = bot_pathrevs.GetInt(); //this->m_pBot->getProfile()->getPathTicks();//IBotNavigator::MAX_PATH_TICKS;

	if ( iMaxLoops <= 0 )
		iMaxLoops = 200;
	
	if ( bNoInterruptions )
		iMaxLoops *= 2; // "less" interruptions, however dont want to hang, or use massive cpu

	int iCurrentNode; // node selected

	bool bFoundGoal = false;

	const CWaypointVisibilityTable *pVisTable = CWaypoints::getVisiblity();

	float fCost;
	float fOldCost;

	int iLastNode = -1;

	float fBeliefSensitivity = 1.5f;

	if ( iConditions & CONDITION_COVERT )
		fBeliefSensitivity = 2.0f;

	while ( !bFoundGoal && !m_theOpenList.empty() && iLoops < iMaxLoops )
	{
		iLoops ++;

		curr = this->nextNode();

		if ( !curr )
			break;

		iCurrentNode = curr->getWaypoint();
		
		bFoundGoal = iCurrentNode == m_iGoalWaypoint;

		if ( bFoundGoal )
			break;

		// can get here now
		m_iFailedGoals.erase(std::remove(m_iFailedGoals.begin(), m_iFailedGoals.end(), iCurrentNode), m_iFailedGoals.end());

		CWaypoint* currWpt = CWaypoints::getWaypoint(iCurrentNode);

		const Vector vOrigin = currWpt->getOrigin();

		succ = nullptr;

		for (const int iSucc : *currWpt)
		{

			if ( iSucc == iLastNode )
				continue;
			if ( iSucc == iCurrentNode ) // argh?
				continue;			
			if ( m_lastFailedPath.bValid )
			{
				if ( m_lastFailedPath.iFrom == iCurrentNode ) 
				{
					// failed this path last time
					if ( m_lastFailedPath.iTo == iSucc )
					{
						m_lastFailedPath.bSkipped = true;
						continue;
					}
				}
			}

			succ = &paths[iSucc];
			CWaypoint* succWpt = CWaypoints::getWaypoint(iSucc);
#ifndef __linux__
			if ( rcbot_debug_show_route.GetBool() )
			{
				edict_t *pListenEdict; //Unused? [APG]RoboCop[CL]

				if ( !engine->IsDedicatedServer() && (pListenEdict = CClients::getListenServerClient())!= nullptr)
				{
					debugoverlay->AddLineOverlayAlpha(succWpt->getOrigin(),currWpt->getOrigin(),255,0,0,255,false,5.0f);
				}
			}
#endif
			if ( iSucc != m_iGoalWaypoint && !m_pBot->canGotoWaypoint(vOrigin,succWpt,currWpt) )
				continue;

			if ( currWpt->hasFlag(CWaypointTypes::W_FL_TELEPORT_CHEAT) )
				fCost = curr->getCost();
			else if ( succWpt->hasFlag(CWaypointTypes::W_FL_TELEPORT_CHEAT) )
				fCost = succWpt->distanceFrom(vOrigin);
			else 
				fCost = curr->getCost()+succWpt->distanceFrom(vOrigin);

			if ( !CWaypointDistances::isSet(m_iCurrentWaypoint,iSucc) || CWaypointDistances::getDistance(m_iCurrentWaypoint,iSucc) > fCost )
				CWaypointDistances::setDistance(m_iCurrentWaypoint,iSucc,fCost);

			if ( succ->isOpen() || succ->isClosed() )
			{
				if ( succ->getParent() != -1 )
				{
					fOldCost = succ->getCost();

					if ( fCost >= fOldCost )
						continue; // ignore route
				}
				else
					continue;
			}

			succ->unClose();

			succ->setParent(iCurrentNode);

			if ( fBeliefSensitivity > 1.6f )
			{
				if ( m_pBot->getEnemy() != nullptr && CBotGlobals::isPlayer(m_pBot->getEnemy()) && m_pBot->isVisible(m_pBot->getEnemy()) )
				{
					if ( CBotGlobals::DotProductFromOrigin(m_pBot->getEnemy(),succWpt->getOrigin()) > 0.96f )
						succ->setCost(fCost+CWaypointLocations::REACHABLE_RANGE);
					else
						succ->setCost(fCost);

					if ( iDangerId != -1 )
					{
						if ( pVisTable->GetVisibilityFromTo(iDangerId,iSucc) )
							succ->setCost(succ->getCost()+m_fBelief[iSucc]*fBeliefSensitivity*2);
					}
				}
				else if ( iDangerId != -1 )
				{
					if ( !pVisTable->GetVisibilityFromTo(iDangerId,iSucc) )
						succ->setCost(fCost);
					else
						succ->setCost(fCost+m_fBelief[iSucc]*fBeliefSensitivity*2);
				}
				else
					succ->setCost(fCost+m_fBelief[iSucc]*fBeliefSensitivity);
				//succ->setCost(fCost-(MAX_BELIEF-m_fBelief[iSucc]));
				//succ->setCost(fCost-((MAX_BELIEF*fBeliefSensitivity)-(m_fBelief[iSucc]*(fBeliefSensitivity-m_pBot->getProfile()->m_fBraveness))));	
			}
			else
				succ->setCost(fCost+m_fBelief[iSucc]*(fBeliefSensitivity-m_pBot->getProfile()->m_fBraveness));	

			succ->setWaypoint(iSucc);

			if ( !succ->heuristicSet() )		
			{
				if ( fBeliefSensitivity > 1.6f )
					succ->setHeuristic(m_pBot->distanceFrom(succWpt->getOrigin())+succWpt->distanceFrom(vTo)+m_fBelief[iSucc]*2);	
				else 
					succ->setHeuristic(m_pBot->distanceFrom(succWpt->getOrigin())+succWpt->distanceFrom(vTo));		
			}

			// Fix: do this AFTER setting heuristic and cost!!!!
			if ( !succ->isOpen() )
			{
				open(succ);
			}

		}

		curr->close(); // close chosen node

		iLastNode = iCurrentNode;		
	}
	/////////
	if ( iLoops == iMaxLoops )
	{
		//*bFail = true;
		
		return false; // not finished yet, wait for next iteration
	}

	m_bWorkingRoute = false;
	
	clearOpenList(); // finished

	if ( !bFoundGoal )
	{
		*bFail = true;

		//no other path
		if ( m_lastFailedPath.bSkipped )
			m_lastFailedPath.bValid = false;

		if (std::find(m_iFailedGoals.begin(), m_iFailedGoals.end(), m_iGoalWaypoint) == m_iFailedGoals.end())
		{
			m_iFailedGoals.emplace_back(m_iGoalWaypoint);
			m_fNextClearFailedGoals = engine->Time() + randomFloat(8.0f,30.0f);
		}

		return true; // waypoint not found but searching is complete
	}

	while ( !m_oldRoute.empty() )
		m_oldRoute.pop();

	iCurrentNode = m_iGoalWaypoint;

	while ( !m_currentRoute.empty() )
		m_currentRoute.pop();

	iLoops = 0;

	const int iNumWaypoints = CWaypoints::numWaypoints();
	float fDistance = 0.0f;

	while ( iCurrentNode != -1 && iCurrentNode != m_iCurrentWaypoint && iLoops <= iNumWaypoints )
	{
		iLoops++;

		m_currentRoute.push(iCurrentNode);
		m_oldRoute.push(iCurrentNode);

		const int iParent = paths[iCurrentNode].getParent();

		// crash bug fix
		if ( iParent != -1 )
			fDistance += (CWaypoints::getWaypoint(iCurrentNode)->getOrigin() - CWaypoints::getWaypoint(iParent)->getOrigin()).Length();
#ifndef __linux__		
		if ( rcbot_debug_show_route.GetBool() )
		{
			edict_t *pListenEdict; //Unused? [APG]RoboCop[CL]

			if ( !engine->IsDedicatedServer() && (pListenEdict = CClients::getListenServerClient())!= nullptr)
			{
				debugoverlay->AddLineOverlayAlpha(CWaypoints::getWaypoint(iCurrentNode)->getOrigin()+Vector(0,0,8.0f),CWaypoints::getWaypoint(iParent)->getOrigin()+Vector(0,0,8.0f),255,255,255,255,false,5.0f);
			}
		}
#endif
		iCurrentNode = iParent;
	}

	CWaypointDistances::setDistance(m_iCurrentWaypoint,m_iGoalWaypoint,fDistance);
	m_fGoalDistance = fDistance;

	// erh??
	if ( iLoops > iNumWaypoints )
	{
		while ( !m_oldRoute.empty () )
			m_oldRoute.pop();

		while ( !m_currentRoute.empty() )
			m_currentRoute.pop();

		*bFail = true;
	}
	else
	{
		m_vGoal = CWaypoints::getWaypoint(m_iGoalWaypoint)->getOrigin();
	}

	return true; 
}
// if bot has a current position to walk to return the boolean
bool CWaypointNavigator :: hasNextPoint ()
{
	return m_iCurrentWaypoint != -1;
}
// return the vector of the next point
Vector CWaypointNavigator :: getNextPoint ()
{
	return CWaypoints::getWaypoint(m_iCurrentWaypoint)->getOrigin();
}

bool CWaypointNavigator :: getNextRoutePoint ( Vector *vPoint )
{
	if ( !m_currentRoute.empty() )
	{
		if (const int head = m_currentRoute.top())
		{
			static CWaypoint *pW;
			pW = CWaypoints::getWaypoint(head);
			*vPoint = pW->getOrigin();// + pW->applyRadius();

			return true;
		}
	}

	return false;
}

bool CWaypointNavigator :: canGetTo (const Vector& vOrigin)
{
	const int iwpt = CWaypointLocations::NearestWaypoint(vOrigin,100,-1,true,false,true, nullptr,false,m_pBot->getTeam());

	if ( iwpt >= 0 )
	{
		if (std::find(m_iFailedGoals.begin(), m_iFailedGoals.end(), iwpt) != m_iFailedGoals.end())
			return false;
	}
	else
		return false;

	return true;
}

void CWaypointNavigator :: rollBackPosition ()
{
	m_vPreviousPoint = m_pBot->getOrigin();
	m_iCurrentWaypoint = CWaypointLocations::NearestWaypoint(m_vPreviousPoint,CWaypointLocations::REACHABLE_RANGE,m_iLastFailedWpt,true,false,true, nullptr,false,m_pBot->getTeam());

	// TODO: figure out what this is actually intended to do
	while ( !m_currentRoute.empty() ) // reached goal!!
	{
		const int iRouteWaypoint = m_currentRoute.top();
		m_currentRoute.pop();
		if (m_iCurrentWaypoint == iRouteWaypoint && !m_currentRoute.empty())
		{
			m_iCurrentWaypoint = m_currentRoute.top();
			m_currentRoute.pop();
		}
	}

	if ( m_iCurrentWaypoint == -1 ) 
		m_iCurrentWaypoint = CWaypointLocations::NearestWaypoint(m_pBot->getOrigin(),CWaypointLocations::REACHABLE_RANGE,-1,true,false,true, nullptr,false,m_pBot->getTeam());
	// find waypoint in route
}
// update the bots current walk vector
void CWaypointNavigator :: updatePosition ()
{
	static Vector vWptOrigin;
	static float fRadius; //Unused? [APG]RoboCop[CL]
	static float fPrevBelief; //Unused? [APG]RoboCop[CL]
	static float fBelief; //Unused? [APG]RoboCop[CL]

	static QAngle aim;
	static Vector vaim;

	fPrevBelief = 0.0f;
	fBelief = 0.0f;

	if ( m_iCurrentWaypoint == -1 ) // invalid
	{
		m_pBot->stopMoving();	
		m_bOffsetApplied = false;
		return;
	}

	CWaypoint *pWaypoint = CWaypoints::getWaypoint(m_iCurrentWaypoint);

	if ( pWaypoint == nullptr)
	{
		m_bOffsetApplied = false;
		return;
	}

	aim = QAngle(0,pWaypoint->getAimYaw(),0);
	AngleVectors(aim,&vaim);

	fRadius = pWaypoint->getRadius();

	vWptOrigin = pWaypoint->getOrigin();

	if ( !m_bWorkingRoute )
	{
		static bool bTouched;
		const bool movetype_ok = CClassInterface::isMoveType(m_pBot->getEdict(),MOVETYPE_LADDER)||CClassInterface::isMoveType(m_pBot->getEdict(),MOVETYPE_FLYGRAVITY);

		//bTouched = false;

		bTouched = pWaypoint->touched(m_pBot->getOrigin(),m_vOffset,m_pBot->getTouchDistance(),!m_pBot->isUnderWater());

		if ( pWaypoint->hasFlag(CWaypointTypes::W_FL_LADDER) )
			bTouched = bTouched && movetype_ok;

		if ( bTouched )
		{
			const int iWaypointID = CWaypoints::getWaypointIndex(pWaypoint);

			fPrevBelief = getBelief(iWaypointID);

			// Bot passed into this waypoint safely, update belief

			if ( const bot_statistics_t *stats = m_pBot->getStats() )
			{
				if ( stats->stats.m_iEnemiesVisible > stats->stats.m_iTeamMatesVisible && stats->stats.m_iEnemiesInRange>0 )
					beliefOne(iWaypointID,BELIEF_DANGER,100.0f);
				else if ( stats->stats.m_iTeamMatesVisible > 0 && stats->stats.m_iTeamMatesInRange > 0 )
					beliefOne(iWaypointID,BELIEF_SAFETY,100.0f);
			}

			m_bOffsetApplied = false;
			m_bDangerPoint = false;
			
			if ( m_currentRoute.empty() ) // reached goal!!
			{
				// fix: bots jumping at wrong positions
				m_pBot->touchedWpt(pWaypoint,-1);

				m_vPreviousPoint = m_pBot->getOrigin();
				m_iPrevWaypoint = m_iCurrentWaypoint;
				m_iCurrentWaypoint = -1;

				if ( m_pBot->getSchedule()->isCurrentSchedule(SCHED_RUN_FOR_COVER) ||
					m_pBot->getSchedule()->isCurrentSchedule(SCHED_GOOD_HIDE_SPOT))
					m_pBot->reachedCoverSpot(pWaypoint->getFlags());
			}
			else
			{
				const int iWaypointFlagsPrev = CWaypoints::getWaypoint(m_iCurrentWaypoint)->getFlags();
				const int iPrevWpt = m_iPrevWaypoint;
				m_vPreviousPoint = m_pBot->getOrigin();
				m_iPrevWaypoint = m_iCurrentWaypoint;

				m_iCurrentWaypoint = m_currentRoute.top();
				m_currentRoute.pop();

				// fix: bots jumping at wrong positions
				m_pBot->touchedWpt(pWaypoint,m_iCurrentWaypoint,iPrevWpt);

				// fix : update pWaypoint as Current Waypoint
				pWaypoint = CWaypoints::getWaypoint(m_iCurrentWaypoint);

				if ( pWaypoint )
				{
					//caxanga334: Original code subtracted an int from Vector, SDK 2013 doesn't like that
					//The waypoint height is probably the Z axis, so I created a Vector with 0 for xy and waypoint height for z
					if ( iWaypointFlagsPrev & CWaypointTypes::W_FL_TELEPORT_CHEAT )
						CBotGlobals::teleportPlayer(m_pBot->getEdict(), pWaypoint->getOrigin() - Vector(0, 0, CWaypoint::WAYPOINT_HEIGHT / 2));
				}
				if ( m_iCurrentWaypoint != -1 )
				{ // random point, but more chance of choosing the most dangerous point
					m_bDangerPoint = randomDangerPath(&m_vDangerPoint);
				}

				fBelief = getBelief(m_iCurrentWaypoint);
			}
		}
	}
	else
		m_bOffsetApplied = false;

	m_pBot->walkingTowardsWaypoint(pWaypoint,&m_bOffsetApplied,m_vOffset);

	// fix for bots not finding goals
	if (m_fNextClearFailedGoals > 0.0f && m_fNextClearFailedGoals < engine->Time())
	{
		m_iFailedGoals.clear();
		m_fNextClearFailedGoals = 0.0f;
	}

	m_pBot->setMoveTo(vWptOrigin+m_vOffset);

	if ( pWaypoint && pWaypoint->isAiming() )
		m_pBot->setAiming(vWptOrigin+vaim*1024);
	
	/*if ( !m_pBot->hasEnemy() && (fBelief >= (fPrevBelief+10.0f)) ) 
		m_pBot->setLookAtTask(LOOK_LAST_ENEMY);
	else if ( !m_pBot->hasEnemy() && (fPrevBelief > (fBelief+10.0f)) )
	{
		m_pBot->setLookVector(pWaypoint->getOrigin() + pWaypoint->applyRadius());
		m_pBot->setLookAtTask(LOOK_VECTOR,randomFloat(1.0f,2.0f));
	}*/
}

void CWaypointNavigator :: clear()
{
	while (!m_currentRoute.empty()) {
		m_currentRoute.pop();
	}
	m_iFailedGoals.clear();
}
// free up memory
void CWaypointNavigator :: freeMapMemory ()
{
	beliefSave(true);
	clear();
}

void CWaypointNavigator :: freeAllMemory ()
{
	freeMapMemory();
}

bool CWaypointNavigator :: routeFound ()
{
	return !m_currentRoute.empty();
}

/////////////////////////////////////////////////////////

// draw paths from this waypoint (if waypoint drawing is on)
void CWaypoint::drawPaths(edict_t* pEdict, const unsigned short int iDrawType) const
{
	for (const int iWpt : *this)
	{
		if (CWaypoint* pWpt = CWaypoints::getWaypoint(iWpt))
		{
			drawPathBeam(pWpt, iDrawType);
		}
	}
}
// draws one path beam
void CWaypoint :: drawPathBeam (CWaypoint *to, const unsigned short int iDrawType) const
{
	static unsigned char r,g,b;

	r = g = b = 200;

	if ( to->hasSomeFlags(CWaypointTypes::W_FL_UNREACHABLE) )
	{
		r = 255;
		g = 100;
		b = 100;
	}

	switch ( iDrawType )
	{
	case DRAWTYPE_EFFECTS:
		g_pEffects->Beam( m_vOrigin, to->getOrigin(), CWaypoints::waypointTexture(), 
		0, 0, 1,
		1, PATHWAYPOINT_WIDTH, PATHWAYPOINT_WIDTH, 255, 
		1, r, g, b, 200, 10);	
		break;
#ifndef __linux__
	case DRAWTYPE_DEBUGENGINE3:
	case DRAWTYPE_DEBUGENGINE2:
	case DRAWTYPE_DEBUGENGINE:
		debugoverlay->AddLineOverlay (m_vOrigin, to->getOrigin(), 200,200,200, false, 1);
		break;
#endif
	default:
		break;
	}
}
/*
bool CWaypoint :: touched ( edict_t *pEdict )
{
	return touched(pEdict->m_pNetworkable->GetPVSInfo()->
}*/
// checks if a waypoint is touched
bool CWaypoint :: touched (const Vector& vOrigin, const Vector& vOffset, const float fTouchDist, const bool onground)
{
	static Vector v_dynamic;

	v_dynamic = m_vOrigin+vOffset;

	if ( hasFlag(CWaypointTypes::W_FL_TELEPORT_CHEAT) )
		return (vOrigin - getOrigin()).Length() < (MAX(fTouchDist,getRadius()));

	// on ground or ladder
	if ( onground )
	{
		if ( (vOrigin-v_dynamic).Length2D() <= fTouchDist )
		{
			if ( hasFlag(CWaypointTypes::W_FL_LADDER) )
				return vOrigin.z+rcbot_ladder_offs.GetFloat() > v_dynamic.z;

			return std::fabs(vOrigin.z-v_dynamic.z) <= WAYPOINT_HEIGHT;
		}
	}
	else // swimming
	{
		if ( (vOrigin-v_dynamic).Length() < fTouchDist )
			return true;
	}

	return false;
}

// TODO: Needs implemented properly [APG]RoboCop[CL]
void CWaypoint::botTouch(CBot* pBot)
{
}

// get the colour of this waypoint in WptColor format
WptColor CWaypointTypes ::getColour (const int iFlags)
{
	WptColor colour = WptColor(0,0,255); // normal waypoint

	bool bNoColour = true;

	for (CWaypointType* const& m_Type : m_Types)
	{
		if (m_Type->isBitsInFlags(iFlags) )
		{
			if ( bNoColour )
			{
				colour = m_Type->getColour();
				bNoColour = false;
			}
			else
				colour.mix(m_Type->getColour());
		}
	}

	return colour;
}
// draw this waypoint
void CWaypoint :: draw ( edict_t *pEdict, const bool bDrawPaths, const unsigned short int iDrawType )
{
	float fHeight = WAYPOINT_HEIGHT;
	float fDistance = 250.0f;

	Vector vAim;

	CBotMod *pCurrentMod = CBotGlobals::getCurrentMod();

	const WptColor colour = CWaypointTypes::getColour(m_iFlags);

	//////////////////////////////////////////

	unsigned char r = colour.r;
	unsigned char g = colour.g;
	unsigned char b = colour.b;
	unsigned char a = colour.a;

	const QAngle qAim = QAngle(0, static_cast<vec_t>(m_iAimYaw), 0);

	AngleVectors(qAim,&vAim);

	// top + bottom heights = fHeight
	fHeight /= 2;

	if ( m_iFlags & CWaypointTypes::W_FL_CROUCH )
		fHeight /= 2; // smaller again

	switch ( iDrawType )
	{
	case DRAWTYPE_BELIEF:
		{
			if ( CClients::clientsDebugging(BOT_DEBUG_NAV) )
			{
				if ( CClient *pClient = CClients::get(pEdict) )
				{
					pEdict = pClient->getDebugBot();

					if ( const CBot *pBot = CBots::getBotPointer(pEdict) )
					{
						const char belief = static_cast<char>(pBot->getNavigator()->getBelief(
							CWaypoints::getWaypointIndex(this)));

						// show danger - red = dangerous / blue = safe
						r = belief;
						b = MAX_BELIEF - static_cast<float>(belief);
						g = 0;
						a = 255;
					}
				}
			}
		}
		[[fallthrough]];
	case DRAWTYPE_DEBUGENGINE3:
		fDistance = 72.0f;
		[[fallthrough]];
	case DRAWTYPE_DEBUGENGINE2:
		// draw area
		if ( pEdict )
		{
			if ( distanceFrom(CBotGlobals::entityOrigin(pEdict)) < fDistance )
			{
				CWaypointTypes::printInfo(this,pEdict,1.0f);

#ifndef __linux__
				if ( m_iFlags )
				{
					if ( pCurrentMod->isWaypointAreaValid(m_iArea,m_iFlags) )
						debugoverlay->AddTextOverlayRGB(m_vOrigin + Vector(0,0,fHeight+4.0f),0,1,255,255,255,255,"%d",m_iArea);	
					else
						debugoverlay->AddTextOverlayRGB(m_vOrigin + Vector(0,0,fHeight+4.0f),0,1,255,0,0,255,"%d",m_iArea);
				}

				if ( CClients::clientsDebugging() )
				{
					if ( CClient *pClient = CClients::get(pEdict) )
					{
						pEdict = pClient->getDebugBot();

						if ( const CBot *pBot = CBots::getBotPointer(pEdict) )
						{
							debugoverlay->AddTextOverlayRGB(m_vOrigin + Vector(0, 0, fHeight + 8.0f), 0, 1,
											0, 0, 255, 255, "%0.4f", pBot->getNavigator()->getBelief(
											CWaypoints::getWaypointIndex(this)));
						}

					}
				}

#endif

			}
		}
		// this will drop down -- don't break
		[[fallthrough]];
	case DRAWTYPE_DEBUGENGINE:

#ifndef __linux__
		// draw waypoint
		debugoverlay->AddLineOverlay (m_vOrigin - Vector(0,0,fHeight), m_vOrigin + Vector(0,0,fHeight), r,g,b, false, 1);

		// draw aim
		debugoverlay->AddLineOverlay (m_vOrigin + Vector(0,0,fHeight/2), m_vOrigin + Vector(0,0,fHeight/2) + vAim*48, r,g,b, false, 1);

		// draw radius
		if (m_fRadius > 0.0f)
		{
			debugoverlay->AddBoxOverlay(m_vOrigin,Vector(-m_fRadius,-m_fRadius,-fHeight),Vector(m_fRadius,m_fRadius,fHeight),QAngle(0,0,0),r,g,b,40,1);
		}
#endif
		break;
	case DRAWTYPE_EFFECTS:
		g_pEffects->Beam( m_vOrigin - Vector(0,0,fHeight), m_vOrigin + Vector(0,0,fHeight), CWaypoints::waypointTexture(), 
			0, 0, 1,
			1, WAYPOINT_WIDTH, WAYPOINT_WIDTH, 255, 
			1, r, g, b, a, 10);//*/

		/*g_pEffects->Beam( m_vOrigin + Vector(0,0,fHeight/2), m_vOrigin + Vector(0,0,fHeight/2) + vAim*48 CWaypoints::waypointTexture(), 
			0, 0, 1,
			1, WAYPOINT_WIDTH/2, WAYPOINT_WIDTH/2, 255, 
			1, r, g, b, a, 10);//*/
		break;

	}

	/*
( const Vector &Start, const Vector &End, int nModelIndex, 
		int nHaloIndex, unsigned char frameStart, unsigned char frameRate,
		float flLife, unsigned char width, unsigned char endWidth, unsigned char fadeLength, 
		unsigned char noise, unsigned char red, unsigned char green,
		unsigned char blue, unsigned char brightness, unsigned char speed) = 0;
*/

	if ( bDrawPaths )
		drawPaths ( pEdict,iDrawType );
}
// clear the waypoints possible paths
void CWaypoint :: clearPaths ()
{
	m_thePaths.clear();
}
// get the distance from this waypoint from vector position vOrigin
float CWaypoint :: distanceFrom (const Vector& vOrigin) const
{
	return (m_vOrigin - vOrigin).Length();
}
///////////////////////////////////////////////////
void CWaypoints :: updateWaypointPairs (std::vector<edict_wpt_pair_t>* pPairs, const int iWptFlag, const char* szClassname)
{
	const int iSize = numWaypoints();
	edict_wpt_pair_t pair;
	CTraceFilterWorldAndPropsOnly filter;

	CWaypoint* pWpt = m_theWaypoints;
	const trace_t* trace_result = CBotGlobals::getTraceResult();

	for ( int i = 0; i < iSize; i ++ )
	{
		if ( pWpt->isUsed() && pWpt->hasFlag(iWptFlag) )
		{
			pair.pWaypoint = pWpt;
			pair.pEdict = CClassInterface::FindEntityByClassnameNearest(pWpt->getOrigin(),szClassname,300.0f);

			if ( pair.pEdict != nullptr)
			{
				Vector vOrigin = CBotGlobals::entityOrigin(pair.pEdict);

				CBotGlobals::traceLine(vOrigin,vOrigin-Vector(0,0,CWaypointLocations::REACHABLE_RANGE),MASK_SOLID_BRUSHONLY,&filter);
				// updates trace_result

				pair.v_ground = trace_result->endpos + Vector(0,0,48.0f);

				pPairs->emplace_back(pair);
			}
		}

		pWpt++;
	}
}
/////////////////////////////////////////////////////////////////////////////////////
// save waypoints (visibilitymade saves having to work out visibility again)
// pPlayer is the person who called the command to save, NULL if automatic
bool CWaypoints :: save (const bool bVisiblityMade, const edict_t* pPlayer, const char* pszAuthor, const char* pszModifier)
{
	char filename[1024];

	CBotGlobals::buildFileName(filename,CBotGlobals::getMapName(),BOT_WAYPOINT_FOLDER,BOT_WAYPOINT_EXTENSION,true);

	std::fstream bfp = CBotGlobals::openFile(filename, std::fstream::out | std::fstream::binary);

	if ( !bfp )
	{
		return false; // give up
	}

	const int iSize = numWaypoints();

	// write header
	// ----
	CWaypointHeader header;
	CWaypointAuthorInfo authorinfo;

	int flags = 0;

	if ( bVisiblityMade )
		flags |= W_FILE_FL_VISIBILITY;

	//////////////////////////////////////////////
	header.iFlags = flags;
	header.iNumWaypoints = iSize;
	header.iVersion = WAYPOINT_VERSION;

	if ( pszAuthor != nullptr)
		std::strncpy(authorinfo.szAuthor,pszAuthor,31);
	else
	{
		std::strncpy(authorinfo.szAuthor,CWaypoints::getAuthor(),31);
	}

	if ( pszModifier != nullptr)
		std::strncpy(authorinfo.szModifiedBy,pszModifier,31);
	else
	{
		std::strncpy(authorinfo.szModifiedBy,CWaypoints::getModifier(),31);
	}

	authorinfo.szAuthor[31] = 0;
	authorinfo.szModifiedBy[31] = 0;

	if ( !bVisiblityMade && pszAuthor== nullptr && pszModifier== nullptr)
	{
		char szAuthorName[32];
		std::strcpy(szAuthorName,"(unknown)");

		if ( pPlayer != nullptr)
		{
			std::strcpy(szAuthorName,CClients::get(pPlayer)->getName());
		}

		if ( authorinfo.szAuthor[0] == 0 ) // no author
		{
			std::strncpy(authorinfo.szAuthor,szAuthorName,31);
			authorinfo.szAuthor[31] = 0;

			std::memset(authorinfo.szModifiedBy,0,32);
		}
		else if ( std::strcmp(szAuthorName,authorinfo.szAuthor) != 0 )
		{
			// modified
			std::strncpy(authorinfo.szModifiedBy,szAuthorName,31);
			authorinfo.szModifiedBy[31] = 0;
		}
	}

	std::strcpy(header.szFileType,BOT_WAYPOINT_FILE_TYPE);
	std::strcpy(header.szMapName,CBotGlobals::getMapName());
	//////////////////////////////////////////////

	bfp.write(reinterpret_cast<char*>(&header), sizeof(CWaypointHeader));
	bfp.write(reinterpret_cast<char*>(&authorinfo), sizeof(CWaypointAuthorInfo));

	for ( int i = 0; i < iSize; i ++ )
	{
		CWaypoint *pWpt = &m_theWaypoints[i];

		// save individual waypoint and paths
		pWpt->save(bfp);
	}

	bfp.close();

	//CWaypointDistances::reset();

	CWaypointDistances::save();

	return true;
}

// load waypoints
bool CWaypoints :: load (const char *szMapName)
{
	char filename[1024];	

	std::strcpy(m_szWelcomeMessage,"No waypoints for this map");

	// open explicit map name waypoints
	if ( szMapName == nullptr)
		CBotGlobals::buildFileName(filename,CBotGlobals::getMapName(),BOT_WAYPOINT_FOLDER,BOT_WAYPOINT_EXTENSION,true);
	else
		CBotGlobals::buildFileName(filename,szMapName,BOT_WAYPOINT_FOLDER,BOT_WAYPOINT_EXTENSION,true);

	std::fstream bfp = CBotGlobals::openFile(filename, std::fstream::in | std::fstream::binary);

	if ( !bfp )
	{
		return false; // give up
	}

	CWaypointHeader header;
	CWaypointAuthorInfo authorinfo;

	std::memset(authorinfo.szAuthor,0, sizeof(authorinfo.szAuthor));
	std::memset(authorinfo.szModifiedBy, 0, sizeof(authorinfo.szModifiedBy));

	// read header
	// -----------

	bfp.read(reinterpret_cast<char*>(&header), sizeof(CWaypointHeader));

	if ( !FStrEq(header.szFileType,BOT_WAYPOINT_FILE_TYPE) )
	{
		logger->Log(LogLevel::ERROR, "Error loading waypoints: File type mismatch");
		return false;
	}
	if ( header.iVersion > WAYPOINT_VERSION )
	{
		logger->Log(LogLevel::ERROR, "Error loading waypoints: Waypoint version too new");
		return false;
	}

	if ( szMapName )
	{
		if ( !FStrEq(header.szMapName,szMapName) )
		{
			logger->Log(LogLevel::ERROR, "Error loading waypoints: Map name mismatch");
			return false;
		}
	}
	else if ( !FStrEq(header.szMapName,CBotGlobals::getMapName()) )
	{
		logger->Log(LogLevel::ERROR, "Error loading waypoints: Map name mismatch");
		return false;
	}

	if ( header.iVersion > 3 )
	{
		// load author information
		bfp.read(reinterpret_cast<char*>(&authorinfo), sizeof(CWaypointAuthorInfo));

		snprintf(m_szWelcomeMessage, sizeof(m_szWelcomeMessage), "Waypoints by %s", authorinfo.szAuthor);

		if ( authorinfo.szModifiedBy[0] != 0 )
		{
			std::strcat(m_szWelcomeMessage," modified by ");
			std::strcat(m_szWelcomeMessage,authorinfo.szModifiedBy);
		}
	}
	else
		snprintf(m_szWelcomeMessage, sizeof(m_szWelcomeMessage), "Waypoints Loaded");

	const int iSize = header.iNumWaypoints;

	// ok lets read the waypoints
	// initialize
	
	CWaypoints::init(authorinfo.szAuthor,authorinfo.szModifiedBy);

	m_iNumWaypoints = iSize;

	bool bWorkVisibility = true;

	// if we're loading from another map, just load visibility, save effort!
	if ( szMapName == nullptr && header.iFlags & W_FILE_FL_VISIBILITY )
		bWorkVisibility = !m_pVisibilityTable->ReadFromFile(iSize);

	for ( int i = 0; i < iSize; i ++ )
	{
		CWaypoint *pWpt = &m_theWaypoints[i];		

		pWpt->load(bfp, header.iVersion);

		if ( pWpt->isUsed() ) // not a deleted waypoint
		{
			// add to waypoint locations for fast searching and drawing
			CWaypointLocations::AddWptLocation(pWpt,i);
		}
	}

	bfp.close();

	m_pVisibilityTable->setWorkVisiblity(bWorkVisibility);

	if ( bWorkVisibility ) // say a message
		logger->Log(LogLevel::INFO, "No waypoint visibility file -- working out waypoint visibility information...");

	// if we're loading from another map just do this again!
	if ( szMapName == nullptr)
		CWaypointDistances::load();

	// script coupled to waypoints too
	//CPoints::loadMapScript();

	return true;
}

void CWaypoint :: init ()
{
	//m_thePaths.clear();
	m_iFlags = 0;
	m_vOrigin = Vector(0,0,0);
	m_bUsed = false; // ( == "deleted" )
	setAim(0);
	m_thePaths.clear();
	m_iArea = 0;
	m_fRadius = 0.0f;	
	m_bUsed = true;
	m_fNextCheckGroundTime = 0.0f;
	m_bHasGround = false;
	m_OpensLaterInfo.clear();
	m_bIsReachable = true; 
	m_fCheckReachableTime = 0.0f;
}

void CWaypoint :: save (std::fstream &bfp )
{
	bfp.write(reinterpret_cast<char*>(&m_vOrigin), sizeof(Vector));
	// aim of vector (used with certain waypoint types)
	bfp.write(reinterpret_cast<char*>(&m_iAimYaw), sizeof(int));
	bfp.write(reinterpret_cast<char*>(&m_iFlags), sizeof(int));
	// not deleted
	bfp.write(reinterpret_cast<char*>(&m_bUsed), sizeof(bool));

	int iPaths = numPaths();
	bfp.write(reinterpret_cast<char*>(&iPaths), sizeof(int));

	for (const int iPath : *this)
	{
		int iPathCopy = iPath;
		bfp.write(reinterpret_cast<char*>(&iPathCopy), sizeof(int));
	}

	if ( CWaypoints::WAYPOINT_VERSION >= 2 )
	{
		bfp.write(reinterpret_cast<char*>(&m_iArea), sizeof(int));
	}

	if ( CWaypoints::WAYPOINT_VERSION >= 3 ) 
	{
		bfp.write(reinterpret_cast<char*>(&m_fRadius), sizeof(float));
	}
}

void CWaypoint :: load (std::fstream &bfp, const int iVersion )
{
	int iPaths;

	bfp.read(reinterpret_cast<char*>(&m_vOrigin), sizeof(Vector));
	// aim of vector (used with certain waypoint types)
	bfp.read(reinterpret_cast<char*>(&m_iAimYaw), sizeof(int));
	bfp.read(reinterpret_cast<char*>(&m_iFlags), sizeof(int));
	// not deleted
	bfp.read(reinterpret_cast<char*>(&m_bUsed), sizeof(bool));
	bfp.read(reinterpret_cast<char*>(&iPaths), sizeof(int));

	for ( int n = 0; n < iPaths; n ++ )
	{		
		int iPath;
		bfp.read(reinterpret_cast<char*>(&iPath), sizeof(int));
		addPathTo(iPath);
	}

	if ( iVersion >= 2 )
	{
		bfp.read(reinterpret_cast<char*>(&m_iArea), sizeof(int));
	}

	if ( iVersion >= 3 ) 
	{
		bfp.read(reinterpret_cast<char*>(&m_fRadius),sizeof(float));
	}
}

bool CWaypoint :: checkGround ()
{
	if ( m_fNextCheckGroundTime < engine->Time() )
	{
		CBotGlobals::quickTraceline(nullptr,m_vOrigin,m_vOrigin-Vector(0,0,80.0f));
		m_bHasGround = CBotGlobals::getTraceResult()->fraction < 1.0f;
		m_fNextCheckGroundTime = engine->Time() + 1.0f;
	}

	return m_bHasGround;
}
// draw waypoints to this client pClient
void CWaypoints :: drawWaypoints( CClient *pClient )
{
	const float fTime = engine->Time();
	//////////////////////////////////////////
	// TODO:
	// draw time currently part of CWaypoints
	// once we can send sprites to individual players
	// make draw waypoints time part of CClient
	if ( m_fNextDrawWaypoints > fTime )
		return;

	m_fNextDrawWaypoints = engine->Time() + 1.0f;
	/////////////////////////////////////////////////
	pClient->updateCurrentWaypoint();

	CWaypointLocations::DrawWaypoints(pClient,CWaypointLocations::REACHABLE_RANGE);

	if ( pClient->isPathWaypointOn() )
	{
		// valid waypoint
		if ( const CWaypoint* pWpt = getWaypoint(pClient->currentWaypoint()) )
			pWpt->drawPaths(pClient->getPlayer(),pClient->getDrawType());
	}
}

void CWaypoints :: init (const char *pszAuthor, const char *pszModifiedBy)
{
	if ( pszAuthor != nullptr)
	{
		std::strncpy(m_szAuthor,pszAuthor,31);
		m_szAuthor[31] = 0;
	}
	else
		m_szAuthor[0] = 0;

	if ( pszModifiedBy != nullptr)
	{
		std::strncpy(m_szModifiedBy,pszModifiedBy,31);
		m_szModifiedBy[31] = 0;
	}
	else
		m_szModifiedBy[0] = 0;

	m_iNumWaypoints = 0;
	m_fNextDrawWaypoints = 0;

	for (CWaypoint& m_theWaypoint : m_theWaypoints)
		m_theWaypoint.init();

	Q_memset(m_theWaypoints, 0, sizeof(CWaypoint) * MAX_WAYPOINTS);

	CWaypointLocations::Init();
	CWaypointDistances::reset();
	m_pVisibilityTable->ClearVisibilityTable();
}

void CWaypoints :: setupVisibility ()
{
	m_pVisibilityTable = new CWaypointVisibilityTable();
	m_pVisibilityTable->init();
}

void CWaypoints :: freeMemory ()
{
	if ( m_pVisibilityTable )
	{
		m_pVisibilityTable->FreeVisibilityTable();

		delete m_pVisibilityTable;
	}
	m_pVisibilityTable = nullptr;
}

void CWaypoints :: precacheWaypointTexture ()
{
	m_iWaypointTexture = engine->PrecacheModel( "sprites/lgtning.vmt" );
}

///////////////////////////////////////////////////////
// return nearest waypoint not visible to pinch point
CWaypoint *CWaypoints :: getPinchPointFromWaypoint (const Vector& vPlayerOrigin, const Vector& vPinchOrigin)
{
	const int iWpt = CWaypointLocations::GetCoverWaypoint(vPlayerOrigin,vPinchOrigin, nullptr,&vPinchOrigin);

	return getWaypoint(iWpt);
}

CWaypoint *CWaypoints :: getNestWaypoint ( int iTeam, int iArea, bool bForceArea, CBot *pBot )
{
	//m_theWaypoints
	return nullptr;
}

void CWaypoints :: deleteWaypoint (const int iIndex)
{	
	// mark as not used
	m_theWaypoints[iIndex].setUsed(false);	
	m_theWaypoints[iIndex].clearPaths();

	// remove from waypoint locations
	const Vector vOrigin = m_theWaypoints[iIndex].getOrigin();
	const float fOrigin[3] = { vOrigin.x, vOrigin.y, vOrigin.z };
	CWaypointLocations::DeleteWptLocation(iIndex,fOrigin);

	// delete any paths pointing to this waypoint
	deletePathsTo(iIndex);
}

void CWaypoints :: shiftVisibleAreas ( edict_t *pPlayer, const int from, const int to )
{
	for ( int i = 0; i < m_iNumWaypoints; i ++ )
	{
		CWaypoint *pWpt = &m_theWaypoints[i];

		if ( !pWpt->isUsed() )
			continue;

		if ( pWpt->getArea() == from )
		{
			CBotGlobals::quickTraceline(pPlayer,CBotGlobals::entityOrigin(pPlayer),pWpt->getOrigin());

			if ( CBotGlobals::getTraceResult()->fraction >= 1.0f  )
				pWpt->setArea(to);	
		}
	}
}

void CWaypoints :: shiftAreas (const int val)
{
	for ( int i = 0; i < m_iNumWaypoints; i ++ )
	{
		CWaypoint *pWpt = &m_theWaypoints[i];

		if ( pWpt->getFlags() > 0 )
		{
		   pWpt->setArea(pWpt->getArea()+val);
		}
	}
}

int CWaypoints::getClosestFlagged(const int iFlags, const Vector& vOrigin, const int iTeam, float* fReturnDist, const unsigned char* failedwpts)
{
	const int iSize = numWaypoints();

	float fDist = 8192.0f;
	float distance;
	int iwpt = -1;
	const int iFrom = CWaypointLocations::NearestWaypoint(vOrigin, fDist, -1, true, false, true, nullptr, false, iTeam);

	CBotMod* pCurrentMod = CBotGlobals::getCurrentMod();
	for (int i = 0; i < static_cast<int>(iSize); i++)
	{
		CWaypoint* pWpt = &m_theWaypoints[i];
		if (i == static_cast<int>(iFrom))
			continue;
		if (failedwpts[i] == 1)
			continue;
		if (pWpt->isUsed() && pWpt->forTeam(iTeam))
		{
			if (pWpt->hasFlag(iFlags))
			{
				// BUG FIX for DOD:S 
				if (!pCurrentMod->isWaypointAreaValid(pWpt->getArea(), iFlags))
					continue;

				if (iFrom == -1)
					distance = (pWpt->getOrigin() - vOrigin).Length();
				else
					distance = CWaypointDistances::getDistance(iFrom, i);

				if (distance < fDist)
				{
					fDist = distance;
					iwpt = i;
				}
			}
		}
	}

	if ( fReturnDist )
		*fReturnDist = fDist;

	return iwpt;
}

void CWaypoints :: deletePathsTo (const int iWpt)
{
	const CWaypoint *pWaypoint = CWaypoints::getWaypoint(iWpt);

	int iNumPathsTo = pWaypoint->numPathsToThisWaypoint();
	WaypointList pathsTo;

	// this will go into an evil loop unless we do this first
	// and use a temporary copy as a side effect of performing
	// a remove will affect the original array
	for (std::size_t i = 0; i < static_cast<std::size_t>(iNumPathsTo); i++)
	{
		pathsTo.emplace_back(pWaypoint->getPathToThisWaypoint(static_cast<int>(i)));
	}

	for (const int iOther : pathsTo)
	{
		CWaypoint* pOther = getWaypoint(iOther);

		pOther->removePathTo(iWpt);
	}
	/*

	short int iNumWaypoints = (short int)numWaypoints();

	for ( short int i = 0; i < iNumWaypoints; i ++ )
		m_theWaypoints[i].removePathTo(iWpt);*/
}

// Fixed; 23/01
void CWaypoints :: deletePathsFrom (const int iWpt)
{
	m_theWaypoints[iWpt].clearPaths();
}

int CWaypoints :: addWaypoint ( CClient *pClient, const char *type1, const char *type2, const char *type3, const char *type4, const bool bUseTemplate )
{
	int iFlags = 0;
	int iIndex; // waypoint index
	int iArea;
	const Vector vWptOrigin = pClient->getOrigin();
	const QAngle playerAngles = CBotGlobals::playerAngles (pClient->getPlayer());
	float fMaxDistance = 0.0f; // distance for auto type

	CBotMod *pCurrentMod = CBotGlobals::getCurrentMod();

	if ( bUseTemplate )
		iArea = pClient->getWptCopyArea();
	else
		iArea = pClient->getWptArea();
// override types and area here
	if ( type1 && *type1 )
	{
		const CWaypointType *t = CWaypointTypes::getType(type1);

		if ( t )
			iFlags |= t->getBits();
		else if ( std::atoi(type1) > 0 )
			iArea = std::atoi(type1);

		if ( type2 && *type2 )
		{
			t = CWaypointTypes::getType(type2);
			if ( t )
				iFlags |= t->getBits();
			else if ( std::atoi(type2) > 0 )
				iArea = std::atoi(type2);

			if ( type3 && *type3 )
			{
				t = CWaypointTypes::getType(type3);
				if ( t )
					iFlags |= t->getBits();
				else if ( std::atoi(type3) > 0 )
					iArea = std::atoi(type3);

				if ( type4 && *type4 )
				{
					t = CWaypointTypes::getType(type4);

					if ( t )
						iFlags |= t->getBits();
					else if ( std::atoi(type4) > 0 )
						iArea = std::atoi(type4);

				}
			}
		}
	}

	const int iPrevFlags = iFlags; // to detect change

	//IPlayerInfo *p = playerinfomanager->GetPlayerInfo(pClient->getPlayer());

	/*CBasePlayer *pPlayer = (CBasePlayer*)(CBaseEntity::Instance(pClient->getPlayer()));

	 // on a ladder
	if ( pPlayer->GetMoveType() == MOVETYPE_LADDER ) 
		iFlags |= CWaypoint::W_FL_LADDER;

	if ( pPlayer->GetFlags() & FL_DUCKING )
		iFlags |= CWaypoint::W_FL_CROUCH;		*/

	float maxRange = rcbot_wpt_autotype_detection_range.GetFloat();
	maxRange = std::clamp(maxRange, 80.0f, 512.0f); // clamp the variable to sane values

	if ( rcbot_wpt_autotype.GetInt() && (!bUseTemplate || rcbot_wpt_autotype.GetInt()==2) )
	{
		if ( IPlayerInfo *pPlayerInfo = playerinfomanager->GetPlayerInfo(pClient->getPlayer()) )
		{
			if ( pPlayerInfo->GetLastUserCommand().buttons & IN_DUCK )
				iFlags |= CWaypointTypes::W_FL_CROUCH;
		}

		for (int i = 0; i < gpGlobals->maxEntities; i++)
		{
			edict_t* pEdict = rcbot2utils::EdictOfIndex(i);

			if (rcbot2utils::IsValidEdict(pEdict))
			{
				const float rangeToOrigin = (rcbot2utils::GetEntityOrigin(pEdict) - vWptOrigin).Length();
				const float rangeToCenter = (rcbot2utils::GetWorldSpaceCenter(pEdict) - vWptOrigin).Length(); // for brush entities
				const float fDistance = std::min(rangeToOrigin, rangeToCenter); // select the smallest between the two distances

				if (fDistance <= maxRange)
				{
					fMaxDistance = std::max(fDistance, fMaxDistance);

					pCurrentMod->addWaypointFlags(pClient->getPlayer(), pEdict, &iFlags, &iArea, &fMaxDistance);
				}
				else
				{
					// distances are unreliable for brush entities, also do a bounds check if the distance above failed
					Vector vWptTop = vWptOrigin;
					vWptTop.z += CWaypoint::WAYPOINT_HEIGHT;

					if (rcbot2utils::PointIsWithinTrigger(pEdict, vWptOrigin) || rcbot2utils::PointIsWithinTrigger(pEdict, vWptTop))
					{
						// Waypoint origin collides with a entity bound
						fMaxDistance = std::max(fDistance, fMaxDistance);

						pCurrentMod->addWaypointFlags(pClient->getPlayer(), pEdict, &iFlags, &iArea, &fMaxDistance);
					}
				}
			}
		}
	}


	if ( bUseTemplate )
	{
		iIndex = addWaypoint(pClient->getPlayer(), vWptOrigin, pClient->getWptCopyFlags(), pClient->isAutoPathOn(),
			playerAngles.y, iArea, pClient->getWptCopyRadius()); // sort flags out
	}
	else
	{
		iIndex = addWaypoint(pClient->getPlayer(), vWptOrigin, iFlags, pClient->isAutoPathOn(), playerAngles.y,
							 iArea, iFlags != iPrevFlags ? fMaxDistance / 2 : 0); // sort flags out	
	}

	pClient->playSound("weapons/crossbow/hit1");

	return iIndex;
}

int CWaypoints :: addWaypoint (edict_t *pPlayer, const Vector& vOrigin, const int iFlags, const bool bAutoPath, const int iYaw, const int iArea, float fRadius)
{
	const int iIndex = freeWaypointIndex();

	if ( iIndex == -1 )	
	{
		logger->Log(LogLevel::ERROR, "Waypoints full!");
		return -1;
	}

	if ( fRadius == 0.0f && rcbot_wpt_autoradius.GetFloat() > 0 )
		fRadius = rcbot_wpt_autoradius.GetFloat();

	///////////////////////////////////////////////////
	m_theWaypoints[iIndex] = CWaypoint(vOrigin,iFlags);	
	m_theWaypoints[iIndex].setAim(iYaw);
	m_theWaypoints[iIndex].setArea(iArea);
	m_theWaypoints[iIndex].setRadius(fRadius);
	// increase max waypoints used
	if ( iIndex == m_iNumWaypoints )
		m_iNumWaypoints++;	
	///////////////////////////////////////////////////

	const float fOrigin[3] = {vOrigin.x,vOrigin.y,vOrigin.z};

	CWaypointLocations::AddWptLocation(iIndex,fOrigin);
	m_pVisibilityTable->workVisibilityForWaypoint(iIndex,true);

	if ( bAutoPath && !(iFlags & CWaypointTypes::W_FL_UNREACHABLE) )
	{
		CWaypointLocations::AutoPath(pPlayer,iIndex);
	}

	return iIndex;
}

void CWaypoints :: removeWaypoint (const int iIndex)
{
	if ( iIndex >= 0 )
		m_theWaypoints[iIndex].setUsed(false);
}

int CWaypoints::numWaypoints()
{
	return m_iNumWaypoints;
}

// TODO: Needs implemented properly [APG]RoboCop[CL]
bool CWaypoints::checkReachable(CWaypoint* pWaypoint, int iStart)
{
	return false;
}

///////////

int CWaypoints::nearestWaypointGoal(const int iFlags, const Vector& origin, float fDist, const int iTeam)
{
	static int size;

	float distance;
	int iWpt = -1;

	size = numWaypoints();

	CBotMod *pCurrentMod = CBotGlobals::getCurrentMod();

	for ( int i = 0; i < size; i ++ )
	{
		CWaypoint* pWpt = &m_theWaypoints[i];

		if ( pWpt->isUsed() && pWpt->forTeam(iTeam) )
		{
			if ( iFlags == -1 || pWpt->hasFlag(iFlags) )
			{
				// FIX DODS bug
				if (pCurrentMod->isWaypointAreaValid(pWpt->getArea(), iFlags)) // CTeamFortress2Mod::m_ObjectiveResource.isWaypointAreaValid(pWpt->getArea()))
				{
					if ( (distance = pWpt->distanceFrom(origin)) < fDist)
					{
						fDist = distance;
						iWpt = i;
					}
				}
			}
		}
	}

	return iWpt;
}

CWaypoint *CWaypoints :: randomRouteWaypoint (const CBot *pBot, const Vector& vOrigin, const Vector& vGoal, const int iTeam, const int iArea)
{
	static int size;
	static CWaypointNavigator *pNav;
	
	pNav = static_cast<CWaypointNavigator*>(pBot->getNavigator());

	size = numWaypoints();

	std::vector<CWaypoint*> goals;

	for ( int i = 0; i < size; i ++ )
	{
		CWaypoint *pWpt = &m_theWaypoints[i];

		if ( pWpt->isUsed() && pWpt->forTeam(iTeam) )// && (pWpt->getArea() == iArea) )
		{
			if ( pWpt->hasFlag(CWaypointTypes::W_FL_ROUTE) )
			{
				if (pWpt->getArea() != iArea )
					continue;

				// CHECK THAT ROUTE WAYPOINT IS USEFUL...

				Vector vRoute = pWpt->getOrigin();

				if ( (vRoute - vOrigin).Length() < (vGoal - vOrigin).Length()+128.0f )
				{
				//if ( CWaypointDistances::getDistance() )
					/*Vector vecLOS;
					float flDot;
					Vector vForward;
					// in fov? Check angle to edict
					vForward = vGoal - vOrigin;
					vForward = vForward/vForward.Length(); // normalise

					vecLOS = vRoute - vOrigin;
					vecLOS = vecLOS/vecLOS.Length(); // normalise

					flDot = DotProduct (vecLOS , vForward );

					if ( flDot > 0.17f ) // 80 degrees*/
					goals.emplace_back(pWpt);
				}
			}
		}
	}

	if ( !goals.empty() )
	{
		return pNav->chooseBestFromBelief(goals);
	}
	return nullptr;
}

constexpr int MAX_DEPTH = 10;
/*
void CWaypointNavigator::runAwayFrom ( int iId )
{
	CWaypoint *pRunTo = CWaypoints::getNextCoverPoint(CWaypoints::getWaypoint(m_iCurrentWaypoint),CWaypoints::getWaypoint(iId)) ;

	if ( pRunTo )
	{
		if ( pRunTo->touched(m_pBot->getOrigin(),Vector(0,0,0),48.0f) )
			m_iCurrentWaypoint = CWaypoints::getWaypointIndex(pRunTo);
		else
			m_pBot->setMoveTo(pRunTo->getOrigin());
	}

}*/

CWaypoint *CWaypoints::getNextCoverPoint ( CBot *pBot, CWaypoint *pCurrent, CWaypoint *pBlocking )
{
	int iMaxDist = -1;
	float fMaxDist = 0.0f;
	float fDist = 0.0f;

	for (const int iNext : *pCurrent)
	{
		CWaypoint* pNext = CWaypoints::getWaypoint(iNext);

		if ( pNext == pBlocking )
			continue;

		if ( !pBot->canGotoWaypoint(pCurrent->getOrigin(),pNext,pCurrent) )
			continue;

		if ( iMaxDist == -1 || (fDist=pNext->distanceFrom(pBlocking->getOrigin())) > fMaxDist )
		{
			fMaxDist = fDist;
			iMaxDist = iNext;
		}
	}

	if ( iMaxDist == -1 )
		return nullptr;

	return CWaypoints::getWaypoint(iMaxDist);
}

CWaypoint *CWaypoints :: nearestPipeWaypoint (const Vector& vTarget, const Vector& vOrigin, int *iAiming)
{
	// 1 : find nearest waypoint to vTarget
	// 2 : loop through waypoints find visible waypoints to vTarget
	// 3 : loop through visible waypoints find another waypoint invisible to vTarget but visible to waypoint 2

	const short iTarget = static_cast<short>(CWaypointLocations::NearestWaypoint(vTarget,BLAST_RADIUS, -1, true, true));
	const CWaypoint *pTarget = CWaypoints::getWaypoint(iTarget);
	//vector<short int> waypointlist;
	int inearest = -1;

	if ( pTarget == nullptr)
		return nullptr;

	const CWaypointVisibilityTable *pTable = CWaypoints::getVisiblity();

	const short numwaypoints = static_cast<short>(numWaypoints());

	float finearestdist = 9999.0f;
	float fjnearestdist = 9999.0f;
	float fidist;
	float fjdist;

	for (short int i = 0; i < numwaypoints; i ++ )
	{
		if ( iTarget == i )
			continue;

		CWaypoint* pTempi = CWaypoints::getWaypoint(i);

		if ( (fidist=pTarget->distanceFrom(pTempi->getOrigin())) > finearestdist )
			continue;

		if ( pTable->GetVisibilityFromTo(iTarget,i) )
		{
			for (short int j = 0; j < numwaypoints; j ++ )
			{				
				if ( j == i )
					continue;
				if ( j == iTarget )
					continue;

				const CWaypoint* pTempj = CWaypoints::getWaypoint(j);

				if ( (fjdist=pTempj->distanceFrom(vOrigin)) > fjnearestdist )
					continue;

				if ( pTable->GetVisibilityFromTo(i,j) && !pTable->GetVisibilityFromTo(iTarget,j) )
				{					
					finearestdist = fidist;
					fjnearestdist = fjdist;
					inearest = j;
					*iAiming = i;
				}
			}
		}
	}

	return CWaypoints::getWaypoint(inearest);

}

void CWaypoints :: autoFix (const bool bAutoFixNonArea)
{
	const int *iNumAreas = CTeamFortress2Mod::m_ObjectiveResource.m_iNumControlPoints;

	if ( iNumAreas == nullptr)
		return;

	const int iNumCps = *iNumAreas + 1;

	for ( int i = 0; i < numWaypoints(); i ++ )
	{
		if ( m_theWaypoints[i].isUsed() && m_theWaypoints[i].getFlags() > 0 )
		{
			if (m_theWaypoints[i].getArea() > iNumCps ||
				(bAutoFixNonArea &&
					m_theWaypoints[i].getArea() == 0 &&
					m_theWaypoints[i].hasSomeFlags(CWaypointTypes::W_FL_SENTRY |
						CWaypointTypes::W_FL_DEFEND |
						CWaypointTypes::W_FL_SNIPER |
						CWaypointTypes::W_FL_CAPPOINT |
						CWaypointTypes::W_FL_TELE_EXIT)))
			{
				m_theWaypoints[i].setArea(CTeamFortress2Mod::m_ObjectiveResource.NearestArea(m_theWaypoints[i].getOrigin()));
				CBotGlobals::botMessage(nullptr,0,"Changed Waypoint id %d area to (area = %d)",i,m_theWaypoints[i].getArea());
			}
		}
	}
}

void CWaypoints :: checkAreas ( edict_t *pActivator )
{
	const int *iNumAreas = CTeamFortress2Mod::m_ObjectiveResource.m_iNumControlPoints;

	if ( iNumAreas == nullptr)
		return;

	const int iNumCps = *iNumAreas + 1;

	for ( int i = 0; i < numWaypoints(); i ++ )
	{
		if ( m_theWaypoints[i].isUsed() && m_theWaypoints[i].getFlags() > 0 )
		{
			if ( m_theWaypoints[i].getArea() > iNumCps )
			{
				CBotGlobals::botMessage(pActivator,0,"Invalid Waypoint id %d (area = %d)",i,m_theWaypoints[i].getArea());
			}
		}
	}
}

CWaypoint* CWaypoints::randomWaypointGoalNearestArea(const int iFlags, const int iTeam, const int iArea, const bool bForceArea, 
													 const CBot* pBot, const bool bHighDanger, const Vector* origin,
													 const int iIgnore, const bool bIgnoreBelief, int iWpt1)
{
	static int size; 
	CWaypoint *pWpt;
	AStarNode *node;
	float fDist;
	
	size = numWaypoints();

	// TODO: inline AStarNode entries
	std::vector<AStarNode*> goals;

	CBotMod *pCurrentMod = CBotGlobals::getCurrentMod();

	if ( iWpt1 == -1 )
	   iWpt1 = CWaypointLocations::NearestWaypoint(*origin,200.0f,-1);

	for ( int i = 0; i < size; i ++ )
	{
		if ( i == iIgnore )
			continue;

		pWpt = &m_theWaypoints[i];

		if ( pWpt->isUsed() && pWpt->forTeam(iTeam) )// && (pWpt->getArea() == iArea) )
		{
			if ( iFlags == -1 || pWpt->hasSomeFlags(iFlags) )
			{
				//DOD:S Bug
				if (!bForceArea && !pCurrentMod->isWaypointAreaValid(pWpt->getArea(), iFlags))
					continue;
				if ( bForceArea && pWpt->getArea() != iArea )
					continue;

				node = new AStarNode();

				if ( iWpt1 != -1 )
				{
					fDist = CWaypointDistances::getDistance(iWpt1,i);					
				}
				else 
				{
					fDist = pWpt->distanceFrom(*origin);
				}
				
				if ( fDist == 0.0f )
					fDist = 0.1f;

				node->setWaypoint(i);
				node->setHeuristic(131072.0f/(fDist*fDist));
			
				goals.emplace_back(node);
			}
		}
	}

	pWpt = nullptr;

	if ( !goals.empty() )
	{
		if ( pBot )
		{
			const CWaypointNavigator* pNav = static_cast<CWaypointNavigator*>(pBot->getNavigator());
			pWpt = pNav->chooseBestFromBeliefBetweenAreas(goals, bHighDanger, bIgnoreBelief);
		}
		else
		{
			// Ensure the index is of type std::size_t
			std::size_t randomIndex = static_cast<std::size_t>(randomInt(0, static_cast<int>(goals.size()) - 1));
			pWpt = CWaypoints::getWaypoint(goals[randomIndex]->getWaypoint());
		}
		//pWpt = goals.Random();
	}

	for (AStarNode* const& goal : goals)
	{
		node = goal;
		delete node;
	}

	return pWpt;
}

CWaypoint* CWaypoints::randomWaypointGoalBetweenArea(const int iFlags, const int iTeam, const int iArea, const bool bForceArea, const CBot* pBot,
													 const bool bHighDanger, const Vector* org1, const Vector* org2, const bool
													 bIgnoreBelief, int iWpt1, int iWpt2)
{
	static int size; 
	CWaypoint *pWpt;
	AStarNode *node;

	if ( iWpt1 == -1 )
		iWpt1 = CWaypointLocations::NearestWaypoint(*org1,200.0f,-1);
	if ( iWpt2 == -1 )
		iWpt2 = CWaypointLocations::NearestWaypoint(*org2,200.0f,-1);

	size = numWaypoints();

	// TODO: inline AStarNode instead of doing manual `new`s
	std::vector<AStarNode*> goals;

	for ( int i = 0; i < size; i ++ )
	{
		pWpt = &m_theWaypoints[i];

		if ( pWpt->isUsed() && pWpt->forTeam(iTeam) )// && (pWpt->getArea() == iArea) )
		{
			if ( iFlags == -1 || pWpt->hasSomeFlags(iFlags) )
			{

				if ( !bForceArea && !CTeamFortress2Mod::m_ObjectiveResource.isWaypointAreaValid(pWpt->getArea()) )
					continue;
				if ( bForceArea && pWpt->getArea() != iArea )
					continue;

				float fCost;

				node = new AStarNode();

				node->setWaypoint(i);

				if ( iWpt1 != -1 )
					fCost = 131072.0f/CWaypointDistances::getDistance(iWpt1,i);
				else
					fCost = 131072.0f/pWpt->distanceFrom(*org1);

				if ( iWpt2 != -1 )
					fCost +=  131072.0f/CWaypointDistances::getDistance(iWpt2,i);
				else
					fCost += 131072.0f/pWpt->distanceFrom(*org2);

				node->setHeuristic(fCost);
			
				goals.emplace_back(node);
			}
		}
	}

	pWpt = nullptr;

	if ( !goals.empty() )
	{
		if ( pBot )
		{
			const CWaypointNavigator* pNav = static_cast<CWaypointNavigator*>(pBot->getNavigator());

			pWpt = pNav->chooseBestFromBeliefBetweenAreas(goals, bHighDanger, bIgnoreBelief);
		}
		else
		{
			pWpt = CWaypoints::getWaypoint(goals[static_cast<std::size_t>(randomInt(0, static_cast<int>(goals.size())))]->getWaypoint());
		}

		//pWpt = goals.Random();
	}

	for (AStarNode* const& goal : goals)
	{
		node = goal;
		delete node;
	}

	return pWpt;
}

CWaypoint* CWaypoints::randomWaypointGoal(const int iFlags, const int iTeam, const int iArea, const bool bForceArea,
										  const CBot* pBot, const bool bHighDanger, const int iSearchFlags, const int iIgnore)
{
	static int size; 
	CWaypoint *pWpt;

	size = numWaypoints();

	std::vector<CWaypoint*> goals;

	CBotMod *pCurrentMod = CBotGlobals::getCurrentMod();

	for ( int i = 0; i < size; i ++ )
	{
		if ( iIgnore == i )
			continue;

		pWpt = &m_theWaypoints[i];

		if ( pWpt->isUsed() && pWpt->forTeam(iTeam) )// && (pWpt->getArea() == iArea) )
		{
			if ( iFlags == -1 || pWpt->hasSomeFlags(iFlags) )
			{
				if (!bForceArea && !pCurrentMod->isWaypointAreaValid(pWpt->getArea(), iFlags))
					continue;
				if ( bForceArea && pWpt->getArea() != iArea )
					continue;

				goals.emplace_back(pWpt);
			}
		}
	}

	pWpt = nullptr;

	if ( !goals.empty() )
	{
		if ( pBot )
		{
			const CWaypointNavigator* pNav = static_cast<CWaypointNavigator*>(pBot->getNavigator());

			pWpt = pNav->chooseBestFromBelief(goals, bHighDanger, iSearchFlags);
		}
		else
		{
			pWpt = goals[static_cast<std::size_t>(randomInt(0, static_cast<int>(goals.size()) - 1))];
		}
	}

	return pWpt;
}

int CWaypoints :: randomFlaggedWaypoint (const int iTeam)
{
	return getWaypointIndex(randomWaypointGoal(-1,iTeam));
}

///////////

// get the next free slot to save a waypoint to
int CWaypoints :: freeWaypointIndex ()
{
	for ( int i = 0; i < MAX_WAYPOINTS; i ++ )
	{
		if ( !m_theWaypoints[i].isUsed() )
			return i;
	}

	return -1;
}

bool CWaypoint :: checkReachable ()
{
	if ( m_fCheckReachableTime < engine->Time() )
	{
		const int numPathsTo = numPathsToThisWaypoint();
		int i;

		for ( i = 0; i < numPathsTo; i ++ )
		{
			CWaypoint* pOther = CWaypoints::getWaypoint(getPathToThisWaypoint(i));

			if ( pOther->getFlags() == 0 )
				break;

			if ( pOther->getFlags() & CWaypointTypes::W_FL_WAIT_GROUND )
			{
				if ( pOther->checkGround() )
					break;
			}
		
			if ( getFlags() & CWaypointTypes::W_FL_OPENS_LATER )
			{
				if ( pOther->isPathOpened(m_vOrigin) )
					break;
			}
		}

		m_bIsReachable = i != numPathsTo;
		m_fCheckReachableTime = engine->Time() + 1.0f;
	}

	return m_bIsReachable;
}

int CWaypoint :: numPaths () const
{
	return static_cast<int>(m_thePaths.size());
}

int CWaypoint :: getPath (const int i) const
{
	return m_thePaths[static_cast<std::size_t>(i)];
}

bool CWaypoint :: isPathOpened (const Vector& vPath)
{
	for (wpt_opens_later_t& info : m_OpensLaterInfo)
	{
		if ( info.vOrigin == vPath )
		{
			if ( info.fNextCheck < engine->Time() )
			{
				info.bVisibleLastCheck = CBotGlobals::checkOpensLater(m_vOrigin,vPath);
				info.fNextCheck = engine->Time() + 2.0f;
			}

			return info.bVisibleLastCheck;
		}
	}

	// not found -- add now
	wpt_opens_later_t newinfo;

	newinfo.fNextCheck = engine->Time() + 2.0f;
	newinfo.vOrigin = vPath;
	newinfo.bVisibleLastCheck = CBotGlobals::checkOpensLater(m_vOrigin,vPath);

	m_OpensLaterInfo.emplace_back(newinfo);

	return newinfo.bVisibleLastCheck;
}

void CWaypoint :: addPathFrom ( int iWaypointIndex )
{
	m_PathsTo.emplace_back(iWaypointIndex);
}

void CWaypoint :: removePathFrom (const int iWaypointIndex)
{
	m_PathsTo.erase(std::remove(m_PathsTo.begin(), m_PathsTo.end(), iWaypointIndex), m_PathsTo.end());
}

int CWaypoint :: numPathsToThisWaypoint () const
{
	return static_cast<int>(m_PathsTo.size());
}

int CWaypoint :: getPathToThisWaypoint (const int i) const
{
	return m_PathsTo[static_cast<std::size_t>(i)];
}

bool CWaypoint :: addPathTo ( int iWaypointIndex )
{
	CWaypoint *pTo = CWaypoints::getWaypoint(iWaypointIndex);

	if ( pTo == nullptr)
		return false;
	// already in list
	if (std::find(m_thePaths.begin(), m_thePaths.end(), iWaypointIndex) != m_thePaths.end())
		return false;
	// dont have a path loop
	if ( this == pTo )
		return false;

	m_thePaths.emplace_back(iWaypointIndex);
	pTo->addPathFrom(CWaypoints::getWaypointIndex(this));

	return true;
}

Vector CWaypoint :: applyRadius () const
{
	if ( m_fRadius > 0 )
		return {randomFloat(-m_fRadius,m_fRadius),randomFloat(m_fRadius,m_fRadius),0};

	return {0,0,0};
}

void CWaypoint :: removePathTo (const int iWaypointIndex)
{
	CWaypoint *pOther = CWaypoints::getWaypoint(iWaypointIndex);

	if ( pOther != nullptr)
	{
		m_thePaths.erase(std::remove(m_thePaths.begin(), m_thePaths.end(), iWaypointIndex), m_thePaths.end());
		pOther->removePathFrom(CWaypoints::getWaypointIndex(this));
	}
}

void CWaypoint :: info ( edict_t *pEdict )
{
	CWaypointTypes::printInfo(this,pEdict);
}

bool CWaypoint ::isAiming() const
{
	return (m_iFlags & (CWaypointTypes::W_FL_DEFEND | 
		CWaypointTypes::W_FL_ROCKET_JUMP | 
		CWaypointTypes::W_FL_DOUBLEJUMP | 
		CWaypointTypes::W_FL_SENTRY | // or machine gun (DOD)
		CWaypointTypes::W_FL_SNIPER | 
		CWaypointTypes::W_FL_TELE_EXIT | 
		CWaypointTypes::W_FL_TELE_ENTRANCE )) > 0;
}

/////////////////////////////////////
// Waypoint Types
/////////////////////////////////////

CWaypointType *CWaypointTypes :: getType(const char *szType)
{
	for (CWaypointType* const& m_Type : m_Types)
	{
		if ( FStrEq(m_Type->getName(),szType) )
			return m_Type;
	}

	return nullptr;
}

void CWaypointTypes :: showTypesOnConsole(edict_t *pPrintTo)
{
	const CBotMod *pMod = CBotGlobals::getCurrentMod();

	CBotGlobals::botMessage(pPrintTo,0,"Available waypoint types");

	for (CWaypointType* const& m_Type : m_Types)
	{
		const char *name = m_Type->getName();
		const char *description = m_Type->getDescription();

		if (m_Type->forMod(pMod->getModId()) )
			CBotGlobals::botMessage(pPrintTo,0,"\"%s\" (%s)",name,description);
	}
}

void CWaypointTypes:: addType ( CWaypointType *type )
{
	m_Types.emplace_back(type);
}

CWaypointType *CWaypointTypes :: getTypeByIndex (const std::size_t iIndex)
{
	if ( iIndex < m_Types.size() )
	{
		return m_Types[static_cast<std::size_t>(iIndex)];
	}

	return nullptr;
}

CWaypointType *CWaypointTypes :: getTypeByFlags (const int iFlags)
{
	const CBotMod *pMod = CBotGlobals::getCurrentMod();

	for (CWaypointType* const& m_Type : m_Types)
	{
		if ( !m_Type->forMod(pMod->getModId()) )
			continue;

		if (m_Type->getBits() == iFlags )
			return m_Type;
	}

	return nullptr;
}

std::size_t CWaypointTypes::getNumTypes()
{
	return m_Types.size();
}

void CWaypointTypes :: setup ()
{	
	addType(new CWaypointType(W_FL_NOBLU,"noblueteam","TF2 blue team can't use this waypoint",WptColor(255,0,0),(1<<MOD_TF2)));
	addType(new CWaypointType(W_FL_NOALLIES,"noallies","DOD:S allies team can't use this waypoint",WptColor(255,0,0),(1<<MOD_DOD)));

	addType(new CWaypointType(W_FL_FLAG,"flag","bot will find a flag here",WptColor(255,255,0),(1<<MOD_TF2)));
	addType(new CWaypointType(W_FL_HEALTH,"health","bot can sometimes get health here",WptColor(255,255,255),(1<<MOD_TF2)|(1<<MOD_HLDM2)|(1<<MOD_SYNERGY)));
	addType(new CWaypointType(W_FL_ROCKET_JUMP,"rocketjump","TF2 a bot can rocket jump here",WptColor(10,100,0),(1<<MOD_TF2)));
	addType(new CWaypointType(W_FL_AMMO,"ammo","bot can sometimes get ammo here",WptColor(50,100,10),(1<<MOD_TF2)|(1<<MOD_HLDM2)|(1<<MOD_SYNERGY)));
	addType(new CWaypointType(W_FL_RESUPPLY,"resupply","TF2 bot can always get ammo and health here",WptColor(255,100,255),(1<<MOD_TF2)));
	addType(new CWaypointType(W_FL_SENTRY,"sentry","TF2 engineer bot can build here",WptColor(255,0,0),(1<<MOD_TF2)));
	addType(new CWaypointType(W_FL_DOUBLEJUMP,"doublejump","TF2 scout can double jump here",WptColor(10,10,100),(1<<MOD_TF2)));
	addType(new CWaypointType(W_FL_TELE_ENTRANCE,"teleentrance","TF2 engineer bot can build tele entrance here",WptColor(50,50,150),(1<<MOD_TF2)));
	addType(new CWaypointType(W_FL_TELE_EXIT,"teleexit","TF2 engineer bot can build tele exit here",WptColor(100,100,255),(1<<MOD_TF2)));
	addType(new CWaypointType(W_FL_AREAONLY,"areaonly","bot will only use this waypoint at certain areas of map",WptColor(150,200,150),(1<<MOD_TF2)));
	addType(new CWaypointType(W_FL_ROUTE,"route","bot will attempt to go through one of these",WptColor(100,100,100),(1<<MOD_TF2)|(1<<MOD_DOD)|(1<<MOD_SYNERGY)|(1<<MOD_CSS)));
	addType(new CWaypointType(W_FL_NO_FLAG,"noflag","TF2 bot will lose flag if he goes thorugh here",WptColor(200,100,50),(1<<MOD_TF2)));
	addType(new CWaypointType(W_FL_COVER_RELOAD,"cover_reload","DOD:S bots can take cover here while shooting an enemy and reload. They can also stand up and shoot the enemy after reloading",WptColor(200,100,50),(1<<MOD_DOD)));
	addType(new CWaypointType(W_FL_FLAGONLY,"flagonly","TF2 bot needs the flag to go through here",WptColor(180,50,80),(1<<MOD_TF2)));

	addType(new CWaypointType(W_FL_NORED,"noredteam","TF2 red team can't use this waypoint",WptColor(0,0,128),(1<<MOD_TF2)));
	addType(new CWaypointType(W_FL_NOAXIS,"noaxis","DOD:S axis team can't use this waypoint",WptColor(255,0,0),(1<<MOD_DOD)));
	addType(new CWaypointType(W_FL_DEFEND,"defend","bot will defend at this position",WptColor(160,50,50)));	
	addType(new CWaypointType(W_FL_SNIPER,"sniper","a bot can snipe here",WptColor(0,255,0)));
	addType(new CWaypointType(W_FL_MACHINEGUN,"machinegun","DOD:S machine gunner will deploy gun here",WptColor(255,0,0),(1<<MOD_DOD)));
	addType(new CWaypointType(W_FL_CROUCH,"crouch","bot will duck here",WptColor(200,100,0)));
	addType(new CWaypointType(W_FL_PRONE,"prone","DOD:S bots prone here",WptColor(0,200,0),(1<<MOD_DOD)));
	addType(new CWaypointType(W_FL_JUMP,"jump","bot will jump here",WptColor(255,255,255)));

	addType(new CWaypointType(W_FL_UNREACHABLE,"unreachable","bot can't go here (used for visibility purposes only)",WptColor(200,200,200)));
	addType(new CWaypointType(W_FL_LADDER,"ladder","bot will climb a ladder here",WptColor(255,255,0)));
	addType(new CWaypointType(W_FL_FALL,"fall","Bots might kill themselves if they fall down here with low health",WptColor(128,128,128)));
	addType(new CWaypointType(W_FL_CAPPOINT,"capture","TF2/DOD:S bot will find a capture point here",WptColor(255,255,0),(1<<MOD_TF2)|(1<<MOD_DOD)));
	addType(new CWaypointType(W_FL_BOMBS_HERE,"bombs","DOD:S bots can pickup bombs here",WptColor(255,100,255),(1<<MOD_DOD)));
	addType(new CWaypointType(W_FL_BOMB_TO_OPEN,"bombtoopen","DOD:S bot needs to blow up this point to move on",WptColor(50,200,30),(1<<MOD_DOD)));
	addType(new CWaypointType(W_FL_BREAKABLE,"breakable","Bots need to break something with a rocket to get through here",WptColor(100,255,50)));
	addType(new CWaypointType(W_FL_OPENS_LATER,"openslater","this waypoint is available when a door is open only",WptColor(100,100,200)));
	addType(new CWaypointType(W_FL_WAIT_GROUND,"waitground","bot will wait until there is ground below",WptColor(150,150,100)));
	addType(new CWaypointType(W_FL_LIFT,"lift","bot needs to wait on a lift here",WptColor(50,80,180)));

	addType(new CWaypointType(W_FL_SPRINT,"sprint","bots will sprint here",WptColor(255,255,190),((1<<MOD_DOD)|(1<<MOD_HLDM2)|(1<<MOD_SYNERGY))));
	addType(new CWaypointType(W_FL_TELEPORT_CHEAT,"teleport","bots will teleport to the next waypoint (cheat)",WptColor(255,255,255)));
	addType(new CWaypointType(W_FL_OWNER_ONLY,"owneronly","only bot teams who own the area of the waypoint can use it",WptColor(0,150,150)));
	addType(new CWaypointType(W_FL_USE,"use","Bots will try to use a button or door here.",WptColor(255,170,0)));

	// Synergy waypoint types
	addType(new CWaypointType(W_FL_GOAL,"goal","Bots will try to reach this waypoint.",WptColor(100,255,50),(1<<MOD_SYNERGY)));
	//addType(new CWaypointType(W_FL_USE,"use","Bots will try to use a button or door here.",WptColor(255,170,0),(1<<MOD_SYNERGY)));

	// Counter-Strike: Source waypoint types
	addType(new CWaypointType(W_FL_NOTERRORIST, "noterror", "CSS terrorist team can't use this waypoint", WptColor(0,0,128), (1<<MOD_CSS)));
	addType(new CWaypointType(W_FL_NOCOUNTERTR, "nocountertr", "CSS counter-terrorist team can't use this waypoint", WptColor(255,0,0), (1<<MOD_CSS)));
	addType(new CWaypointType(W_FL_RESCUEZONE, "rescue", "CSS bots will take hostages to this waypoint", WptColor(0,255,230), (1<<MOD_CSS)));
	addType(new CWaypointType(W_FL_GOAL, "goal", "CSS bots will find the map goal here", WptColor(255,255,0), (1<<MOD_CSS)));
	addType(new CWaypointType(W_FL_DOOR, "door", "CSS bots will check if they need to open a door", WptColor(255,120,0), (1<<MOD_CSS)));
	addType(new CWaypointType(W_FL_NO_HOSTAGES, "nohostages", "CSS CT bots escorting hostages can't use this waypoint", WptColor(200,230,20), (1<<MOD_CSS)));

	//addType(new CWaypointType(W_FL_ATTACKPOINT,"squad_attackpoint","Tactical waypoint -- each squad will go to different attack points and signal others to go",WptColor(90,90,90)));
}

void CWaypointTypes :: freeMemory ()
{
	for (CWaypointType*& m_Type : m_Types)
	{
		delete m_Type;
		m_Type = nullptr;
	}

	m_Types.clear();
}

void CWaypointTypes:: printInfo ( CWaypoint *pWpt, edict_t *pPrintTo, const float duration )
{
	const CBotMod *pCurrentMod = CBotGlobals::getCurrentMod();
	char szMessage[1024];
	Q_snprintf(szMessage, 1024, "Waypoint ID %d (Area = %d | Radius = %0.1f)[", CWaypoints::getWaypointIndex(pWpt),
			   pWpt->getArea(), static_cast<double>(pWpt->getRadius()));	

	if ( pWpt->getFlags() )
	{
		bool bComma = false;

		for (CWaypointType* const& m_Type : m_Types)
		{
			if (m_Type->forMod(pCurrentMod->getModId()) && m_Type->isBitsInFlags(pWpt->getFlags()) )
			{
				if ( bComma )
					std::strcat(szMessage,",");

				std::strcat(szMessage, m_Type->getName());
				//std::strcat(szMessage," (");
				//std::strcat(szMessage,m_Types[i]->getDescription());
				//std::strcat(szMessage,")");				
				bComma = true;
			}
		}
	}
	else
	{
		std::strcat(szMessage,"No Waypoint Types");
	}

	std::strcat(szMessage,"]");

#ifndef __linux__
	debugoverlay->AddTextOverlay(pWpt->getOrigin()+Vector(0,0,24),duration,szMessage);
#endif
	//CRCBotPlugin :: HudTextMessage (pPrintTo,"wptinfo","Waypoint Info",szMessage,Color(255,0,0,255),1,2);
}
/*
CCrouchWaypointType :: CCrouchWaypointType()
{
	CWaypointType(W_FL_CROUCH,"crouch","bot will duck here",WptColor(200,100,0));
}

void CCrouchWaypointType :: giveTypeToWaypoint ( CWaypoint *pWaypoint )
{

}

void CCrouchWaypointType :: removeTypeFromWaypoint ( CWaypoint *pWaypoint )
{

}
*/
void CWaypointTypes :: displayTypesMenu ( edict_t *pPrintTo )
{

}

void CWaypointTypes:: selectedType ( CClient *pClient )
{

}

/*void CWaypointType :: giveTypeToWaypoint ( CWaypoint *pWaypoint )
{

}

void CWaypointType :: removeTypeFromWaypoint ( CWaypoint *pWaypoint )
{

}*/

CWaypointType :: CWaypointType (const int iBit, const char *szName, const char *szDescription, const WptColor& vColour, const int iModBits, const int iImportance)
{
	m_iBit = iBit;
	m_szName = CStrings::getString(szName);
	m_szDescription = CStrings::getString(szDescription);
	m_vColour = vColour;
	m_iMods = iModBits;
	m_iImportance = iImportance;
}

bool CWaypoint :: forTeam (const int iTeam)
{
	CBotMod *pMod = CBotGlobals::getCurrentMod();

	return pMod->checkWaypointForTeam(this,iTeam);
	/*
	if ( iTeam == TF2_TEAM_BLUE )
		return (m_iFlags & CWaypointTypes::W_FL_NOBLU)==0;
	else if ( iTeam == TF2_TEAM_RED )
		return (m_iFlags & CWaypointTypes::W_FL_NORED)==0;

	return true;	*/
}

class CTestBot : public CBotTF2
{
public:
	CTestBot(edict_t *pEdict, const int iTeam, int iClass)
	{
		CBotTF2::init();
		std::strcpy(m_szBotName,"Test Bot");
		m_iClass = static_cast<TF_Class>(iClass); 
		m_iTeam = iTeam; 
		m_pEdict = pEdict;
		CBotTF2::setup();
	}
};

void CWaypointTest :: go ( edict_t *pPlayer )
{
	CBot *pBots[2];

	pBots[0] = new CTestBot(pPlayer,2,9);
	pBots[1] = new CTestBot(pPlayer,3,9);

	for ( int iBot = 0; iBot < 2; iBot ++ )
	{
		CBot* pBot = pBots[iBot];

		IBotNavigator* pNav = pBot->getNavigator();

		for ( int i = 0; i < CWaypoints::MAX_WAYPOINTS; i ++ )
		{
			CWaypoint* pWpt1 = CWaypoints::getWaypoint(i);
			
			int iCheck = 0;

			if ( !pWpt1->forTeam(iBot+2) )
				continue;

			if ( !pBot->canGotoWaypoint(Vector(0,0,0),pWpt1) )
				continue;
		
			// simulate bot situations on the map
			// e.g. bot is at sentry point A wanting more ammo at resupply X
			if ( pWpt1->hasFlag(CWaypointTypes::W_FL_SENTRY) )
				iCheck = CWaypointTypes::W_FL_RESUPPLY|CWaypointTypes::W_FL_AMMO;
			if ( pWpt1->hasSomeFlags(CWaypointTypes::W_FL_RESUPPLY|CWaypointTypes::W_FL_AMMO) )
				iCheck = CWaypointTypes::W_FL_SENTRY|CWaypointTypes::W_FL_TELE_ENTRANCE|CWaypointTypes::W_FL_TELE_EXIT;
			
			if ( iCheck != 0 )
			{
				for ( int j = 0; j < CWaypoints::MAX_WAYPOINTS; j ++ )
				{

					if ( i == j )
						continue;

					CWaypoint* pWpt2 = CWaypoints::getWaypoint(j);

					if ( !pWpt2->forTeam(iBot+2) )
						continue;

					pWpt2 = CWaypoints::getWaypoint(j);

					if ( !pBot->canGotoWaypoint(Vector(0,0,0),pWpt2) )
						continue;

					if ( pWpt2->getArea() != 0 && pWpt2->getArea() != pWpt1->getArea() )
						continue;

					if ( pWpt2->hasSomeFlags(iCheck) )
					{
						bool bfail = false;
						constexpr bool brestart = true;
						constexpr bool bnointerruptions = true;

						while ( pNav->workRoute(
								pWpt1->getOrigin(),
								pWpt2->getOrigin(),
								&bfail,
								brestart,
								bnointerruptions,j) 
							== 
							false 
							) {}

						if ( bfail )
						{
							// log this one
							CBotGlobals::botMessage(pPlayer,0,"Waypoint Test: Route fail from '%d' to '%d'",i,j);
						}
					}
				}
			}
		}
	}

	delete pBots[0];
	delete pBots[1];
}
