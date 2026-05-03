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
#include "server_class.h"

#include "bot.h"

#include "in_buttons.h"

#include "bot_mods.h"
#include "bot_globals.h"
#include "bot_weapons.h"
#include "bot_configfile.h"
#include "bot_getprop.h"
#include "bot_dod_bot.h"
#include "bot_navigator.h"
#include "bot_waypoint.h"
#include "bot_waypoint_locations.h"
#include "bot_perceptron.h"

#include <algorithm>
#include <cstring>

 //Ignores the min/max macros in the windows headers
//#if SOURCE_ENGINE == SE_SDK2013 || SOURCE_ENGINE == SE_BMS
#include "valve_minmax_off.h"
//#endif

edict_t *CDODMod::m_pResourceEntity = nullptr;
CDODFlags CDODMod::m_Flags;
edict_t * CDODMod::m_pPlayerResourceEntity = nullptr;
float CDODMod::m_fMapStartTime = 0.0f;
edict_t * CDODMod::m_pGameRules = nullptr;
int CDODMod::m_iMapType = 0;
bool CDODMod::m_bCommunalBombPoint = false;
int CDODMod::m_iBombAreaAllies = 0;
int CDODMod::m_iBombAreaAxis = 0;
//CPerceptron *CDODMod::gNetAttackOrDefend = NULL;
float CDODMod::fAttackProbLookUp[MAX_DOD_FLAGS+1][MAX_DOD_FLAGS+1];
std::vector<edict_wpt_pair_t> CDODMod::m_BombWaypoints;
std::vector<edict_wpt_pair_t> CDODMod::m_BreakableWaypoints;


eDODVoiceCommand_t g_DODVoiceCommands[DOD_VC_INVALID] = 
{
	{DOD_VC_GOGOGO,"attack"},
	{DOD_VC_YES,"yessir"},
	{DOD_VC_DROPWEAP,"dropweapons"},
	{DOD_VC_HOLD,"hold"},
	{DOD_VC_NO,"negative"},
	{DOD_VC_DISPLACE,"displace"},
	{DOD_VC_GO_LEFT,"left"},
	{DOD_VC_NEED_BACKUP,"backup"},
	{DOD_VC_MGAHEAD,"mgahead"},
	{DOD_VC_GO_RIGHT,"right"},
	{DOD_VC_FIRE_IN_THE_HOLE,"fireinhole"},
	{DOD_VC_ENEMY_BEHIND,"enemybehind"},
	{DOD_VC_STICK_TOGETHER,"sticktogether"},
	{DOD_VC_USE_GRENADE,"usegrens"},
	{DOD_VC_ENEMY_DOWN,"wegothim"},
	{DOD_VC_COVERING_FIRE,"cover"},      
	{DOD_VC_SNIPER,"sniper"},
	{DOD_VC_NEED_MG,"moveupmg"},
	{DOD_VC_SMOKE,"usesmoke"},
	{DOD_VC_NICE_SHOT,"niceshot"},
	{DOD_VC_NEED_AMMO,"needammo"},
	{DOD_VC_GRENADE2,"grenade"},
	{DOD_VC_THANKS,"thanks"},
	{DOD_VC_USE_BAZOOKA,"usebazooka"},
	{DOD_VC_CEASEFIRE,"ceasefire"},
	{DOD_VC_AREA_CLEAR,"areaclear"},
	{DOD_VC_BAZOOKA,"bazookaspotted"}
};

//
//
//

// Returns true if team can go to waypoint
bool CDODMod :: checkWaypointForTeam(CWaypoint *pWpt, const int iTeam)
{
	return (!pWpt->hasFlag(CWaypointTypes::W_FL_NOALLIES)||iTeam!=TEAM_ALLIES)&&(!pWpt->hasFlag(CWaypointTypes::W_FL_NOAXIS)||iTeam!=TEAM_AXIS);
}

bool CDODMod :: shouldAttack (const int iTeam)
// uses the perceptron to return probability of attack
{
	static int iFlags_0;
	static int iFlags_1;
	static int iNumFlags;

	iNumFlags = m_Flags.getNumFlags();

	iFlags_0 = m_Flags.getNumFlagsOwned(iTeam == TEAM_ALLIES ? TEAM_AXIS : TEAM_ALLIES) /
		(iNumFlags * MAX_DOD_FLAGS);

	iFlags_1 = m_Flags.getNumFlagsOwned(iTeam) / (iNumFlags * MAX_DOD_FLAGS);

	return randomFloat(0.0f,1.0f) < fAttackProbLookUp[iFlags_0][iFlags_1];//gNetAttackOrDefend->getOutput();
}

////////////////////////////////////////////////
void CDODMod::initMod()
{
///-------------------------------------------------
	CBotGlobals::botMessage(nullptr, 0, "Training DOD:S capture decision 'NN' ... hold on...");

	const CBotNeuralNet nn(2, 2, 2, 1, 0.4f);

	CTrainingSet tset(2, 1, 4);

	tset.setScale(0.0f, 1.0f);

	tset.addSet();
	tset.in(1.0f / 5); // E - enemy flag ratio
	tset.in(1.0f / 5); // T - team flag ratio
	tset.out(0.9f); // probability of attack

	tset.addSet();
	tset.in(4.0f / 5); // E - enemy flag ratio
	tset.in(1.0f / 5); // T - team flag ratio
	tset.out(0.2f); // probability of attack (mostly defend)
	
	tset.addSet();
	tset.in(1.0f / 5); // E - enemy flag ratio
	tset.in(4.0f / 5); // T - team flag ratio
	tset.out(0.9f); // probability of attack

	tset.addSet();
	tset.in(0.5f); // E - enemy flag ratio
	tset.in(0.5f); // T - team flag ratio
	tset.out(0.6f); // probability of attack

	nn.batch_train(&tset, 1000);

	// create look up table for probabilities
	for (short i = 0; i <= MAX_DOD_FLAGS; i++)
	{
		for (short j = 0; j <= MAX_DOD_FLAGS; j++)
		{
			tset.init();
			tset.addSet();
			tset.in(i / static_cast<ga_nn_value>(MAX_DOD_FLAGS));
			tset.in(j / static_cast<ga_nn_value>(MAX_DOD_FLAGS));
			nn.execute(tset.getBatches()->in, &fAttackProbLookUp[i][j], 0.0f, 1.0f);
		}
	}

	tset.freeMemory();

	CBotGlobals::botMessage(nullptr, 0, "... done!");
	///-------------------------------------------------

    CWeapons::loadWeapons(m_szWeaponListName == nullptr ? "DOD" : m_szWeaponListName, DODWeaps.data());
	//CWeapons::loadWeapons("DOD", DODWeaps);
	/*
	for ( i = 0; i < DOD_WEAPON_MAX; i ++ )
		CWeapons::addWeapon(new CWeapon(DODWeaps[i]));*/

	m_pResourceEntity = nullptr;
}

void CDODMod :: mapInit ()
{
	CBotMod::mapInit();

	m_pResourceEntity = nullptr;
	m_pGameRules = nullptr;
	m_pPlayerResourceEntity = nullptr;
	m_Flags.init();
	m_fMapStartTime = engine->Time();
	m_iMapType = DOD_MAPTYPE_UNKNOWN;
	m_bCommunalBombPoint = false;
}

float CDODMod::getMapStartTime () 
{ 
	//if ( !m_pGameRules ) 
	//	return 0; 
	return m_fMapStartTime;
	//return CClassInterface::getRoundTime(m_pGameRules); 
}

int CDODMod::getHighestScore ()
{
	if ( !m_pPlayerResourceEntity )
		return 0;

	int highest = 0;

	for ( int i = 1; i <= gpGlobals->maxClients; i ++ )
	{
		edict_t* edict = INDEXENT(i);

		if ( edict && CBotGlobals::entityIsValid(edict) )
		{
			const int score = static_cast<short>(getScore(edict));

			highest = std::max(score, highest);
		}
	}

	return highest;
}

bool CDODFlags::isTeamMateDefusing (const edict_t *pIgnore, const int iTeam, const int id) const
{
	if ( m_pBombs[id][0] != nullptr)
		return isTeamMateDefusing(pIgnore,iTeam,CBotGlobals::entityOrigin(m_pBombs[id][0]));

	return false;
}

bool CDODFlags::isTeamMateDefusing (const edict_t *pIgnore, const int iTeam, const Vector& vOrigin)
{
	for ( int i = 1; i <= gpGlobals->maxClients; i ++ )
	{
		edict_t* pPlayer = INDEXENT(i);

		if ( pIgnore == pPlayer )
			continue;

		if ( !CBotGlobals::entityIsValid(pPlayer) )
			continue;

		IPlayerInfo* pPlayerinfo = playerinfomanager->GetPlayerInfo(pPlayer);
		const CBotCmd cmd = pPlayerinfo->GetLastUserCommand();

		if ( CClassInterface::isPlayerDefusingBomb_DOD(pPlayer) || cmd.buttons & IN_USE )
		{
			if ( CClassInterface::getTeam(pPlayer) != iTeam )
				continue;

			if ( (vOrigin - CBotGlobals::entityOrigin(pPlayer)).Length() < 128 )
			{
				return true;
			}
		}
	}

	return false;
}

bool CDODFlags::isTeamMatePlanting (const edict_t *pIgnore, const int iTeam, const Vector& vOrigin)
{
	for ( int i = 1; i <= gpGlobals->maxClients; i ++ )
	{
		edict_t* pPlayer = INDEXENT(i);

		if ( pIgnore == pPlayer )
			continue;

		if ( !CBotGlobals::entityIsValid(pPlayer) )
			continue;

		if ( CClassInterface::isPlayerPlantingBomb_DOD(pPlayer) )
		{
			if ( CClassInterface::getTeam(pPlayer) != iTeam )
				continue;

			if ( (vOrigin - CBotGlobals::entityOrigin(pPlayer)).Length() < 128 )
			{
				return true;
			}
		}
	}

	return false;
}

bool CDODFlags::isTeamMatePlanting (const edict_t *pIgnore, const int iTeam, const int id) const
{
	if ( m_pBombs[id][0] )
		return isTeamMatePlanting(pIgnore,iTeam,CBotGlobals::entityOrigin(m_pBombs[id][0]));

	return false;
}

int CDODFlags::findNearestObjective(const Vector& vOrigin) const
{
	float fNearest = 1024.0f;
	float fDistance;
	int iNearest = -1;

	for (int i = 0; i < m_iNumControlPoints; i++)
	{
		if ( m_iWaypoint[i] != -1 )
		{
			CWaypoint* waypoint = CWaypoints::getWaypoint(m_iWaypoint[i]);

			if (waypoint != nullptr)
			{
				if ((fDistance = (waypoint->getOrigin() - vOrigin).Length()) < fNearest)
				{
					fNearest = fDistance;
					iNearest = i;
				}
			}
		}
	}
	return iNearest;
}

// return the flag with the least danger (randomly)
bool CDODFlags::getRandomEnemyControlledFlag (const CBot *pBot, Vector *position, const int iTeam, int *id) const
{
	if ( id )
		*id = -1;

	IBotNavigator* pNav = pBot->getNavigator();

	float fTotal = 0.0f;

	for (int i = 0; i < m_iNumControlPoints; i++)
	{
		if ( m_iWaypoint[i] != -1 )
		{
			if (m_pFlags[i] == nullptr || m_iOwner[i] == iTeam)
				continue;

			if ( iTeam == TEAM_ALLIES && m_iAlliesReqCappers[i] == 0 )
				continue;

			if ( iTeam == TEAM_AXIS && m_iAxisReqCappers[i] == 0 )
				continue;

			if ( iTeam == TEAM_ALLIES )
				fTotal += (MAX_BELIEF + 1.0f - pNav->getBelief(m_iWaypoint[i])) / MAX_BELIEF * static_cast<float>(m_iNumAllies[i] + 1);
			else
				fTotal += (MAX_BELIEF + 1.0f - pNav->getBelief(m_iWaypoint[i])) / MAX_BELIEF * static_cast<float>(m_iNumAxis[i] + 1);
		}
	}

	if ( fTotal == 0.0f )
		return false;

	const float fRand = randomFloat(0, fTotal);
	fTotal = 0.0f;

	for (int i = 0; i < m_iNumControlPoints; i++)
	{
		if ( m_iWaypoint[i] != -1 )
		{
			if ( m_pFlags[i] == nullptr || m_iOwner[i] == iTeam )
				continue;

			if ( iTeam == TEAM_ALLIES && m_iAlliesReqCappers[i] == 0 )
				continue;

			if ( iTeam == TEAM_AXIS && m_iAxisReqCappers[i] == 0 )
				continue;

			if ( iTeam == TEAM_ALLIES )
				fTotal += (MAX_BELIEF + 1.0f - pNav->getBelief(m_iWaypoint[i])) / MAX_BELIEF * static_cast<float>(m_iNumAllies[i] + 1);
			else
				fTotal += (MAX_BELIEF + 1.0f - pNav->getBelief(m_iWaypoint[i])) / MAX_BELIEF * static_cast<float>(m_iNumAxis[i] + 1);
		}

		if ( fRand <= fTotal )
		{
			if ( id )
				*id = i;
			*position = m_vCPPositions[i];
			return true;
		}
	}

	return false;
}

bool CDODFlags::getRandomBombToDefuse(Vector& position, int iTeam, edict_t*& pBombTarget, int* id) const
{
	if (id)
		*id = -1;

	std::vector<int> possibleTargets;

	// Collect possible bomb targets
	for (int i = 0; i < m_iNumControlPoints; ++i)
	{
		if (m_iOwner[i] == iTeam && isBombPlanted(i) && !isBombBeingDefused(i) && m_pBombs[i][0] != nullptr)
		{
			for (int j = 0; j < getNumBombsRequired(i); ++j)
			{
				possibleTargets.emplace_back(i);
			}
		}
	}
	if (possibleTargets.empty())
	{
		return false;
	}

	// Select a random target
	const int selection = possibleTargets[randomInt(0, static_cast<int>(possibleTargets.size()) - 1)];

	// Determine the bomb target
	if (m_pBombs[selection][1] != nullptr &&
		CClassInterface::getDODBombState(m_pBombs[selection][1]) == DOD_BOMB_STATE_ACTIVE)
	{
		pBombTarget = m_pBombs[selection][1];
	}
	else
	{
		pBombTarget = m_pBombs[selection][0];
	}

	// Set position and ID
	position = CBotGlobals::entityOrigin(pBombTarget);

	if (id)
	{
		*id = selection;
	}

	return true;
}

//return random bomb with highest danger
bool CDODFlags:: getRandomBombToDefend ( CBot *pBot, Vector *position, const int iTeam, edict_t **pBombTarget, int *id ) const
{
	std::vector<int> iPossible; // int is control point entry

	if ( id )
		*id = -1;

	// more possibility to return bomb targets with no bomb already
	for (int i = 0; i < m_iNumControlPoints; i++)
	{
		if ( m_iOwner[i] != iTeam && isBombPlanted(i) && m_pBombs[i][0] != nullptr)
			for ( int j = 0; j < getNumBombsRequired(i); j ++ ) { iPossible.emplace_back(i); }
	}

	if (!iPossible.empty())
	{
		const int selection = iPossible[randomInt(0, static_cast<int>(iPossible.size()) - 1)];

		if ( m_pBombs[selection][1] != nullptr)
		{
			if ( CClassInterface::getDODBombState(m_pBombs[selection][1]) != 0 )
				*pBombTarget = m_pBombs[selection][1];
			else
				*pBombTarget = m_pBombs[selection][0];
		}
		else
			*pBombTarget = m_pBombs[selection][0];

		*position = CBotGlobals::entityOrigin(*pBombTarget);

		if ( id ) // area of the capture point
			*id = selection;
	}

	return !iPossible.empty();
}

// return random flag with lowest danger
bool CDODFlags::getRandomBombToPlant(CBot* pBot, Vector& position, const int iTeam, edict_t*& pBombTarget, int* id) const
{
	assert(pBot != nullptr);

	if (id)
		*id = -1;

	IBotNavigator* pNav = pBot->getNavigator();

	assert(pNav != nullptr);

	struct BombTargetCandidate
	{
		edict_t* target;
		float weight;
		int index;
	};

	std::vector<BombTargetCandidate> candidates;
	float totalWeight = 0.0f;

	for (int i = 0; i < m_iNumControlPoints; ++i)
	{
		if (m_iWaypoint[i] == -1 || m_pBombs[i][0] == nullptr || m_iOwner[i] == iTeam ||
			isBombPlanted(i) || m_iBombsRemaining[i] == 0)
		{
			continue;
		}

		const float belief = pNav->getBelief(m_iWaypoint[i]);
		const float weight = (MAX_BELIEF + 1.0f - belief) / MAX_BELIEF * static_cast<float>(getNumBombsRemaining(i));

		totalWeight += weight;
		edict_t* target = m_pBombs[i][0];

		if (m_pBombs[i][1] != nullptr && CClassInterface::getDODBombState(m_pBombs[i][1]) == DOD_BOMB_STATE_AVAILABLE)
		{
			target = m_pBombs[i][1];
		}

		candidates.push_back({ target, weight, i }); //TODO: maybe use emplace_back? [APG]RoboCop[CL]
	}

	if (candidates.empty())
	{
		return false;
	}

	const float randomValue = randomFloat(0.0f, totalWeight);
	float cumulativeWeight = 0.0f;

	for (const auto& [target, weight, index] : candidates)
	{
		cumulativeWeight += weight;

		if (randomValue <= cumulativeWeight)
		{
			pBombTarget = target;
			position = CBotGlobals::entityOrigin(pBombTarget);

			if (id)
				*id = index;

			return true;
		}
	}

	return false;
}

bool CDODFlags::getRandomTeamControlledFlag (const CBot *pBot, Vector *position, const int iTeam, int *id) const
{
	if ( id )
		*id = -1;

	IBotNavigator* pNav = pBot->getNavigator();

	float fTotal = 0.0f;

	for (int i = 0; i < m_iNumControlPoints; i++)
	{
		if ( m_iWaypoint[i] != -1 )
		{
			if ( m_pFlags[i] == nullptr || m_iOwner[i] != iTeam )
				continue;

			if ( iTeam == TEAM_AXIS )
				fTotal += (pNav->getBelief(m_iWaypoint[i])+MAX_BELIEF)/(MAX_BELIEF*2) * static_cast<float>(m_iNumAllies[i] + 1);
			else
				fTotal += (pNav->getBelief(m_iWaypoint[i])+MAX_BELIEF)/(MAX_BELIEF*2) * static_cast<float>(m_iNumAxis[i] + 1);
		}
	}

	if ( fTotal == 0.0f )
		return false;

	const float fRand = randomFloat(0, fTotal);
	fTotal = 0.0f;

	for (int i = 0; i < m_iNumControlPoints; i++)
	{
		if ( m_iWaypoint[i] != -1 )
		{
			if ( m_pFlags[i] == nullptr || m_iOwner[i] != iTeam )
				continue;

			if ( iTeam == TEAM_AXIS )
				fTotal += (pNav->getBelief(m_iWaypoint[i])+MAX_BELIEF)/(MAX_BELIEF*2) * static_cast<float>(m_iNumAllies[i] + 1);
			else
				fTotal += (pNav->getBelief(m_iWaypoint[i])+MAX_BELIEF)/(MAX_BELIEF*2) * static_cast<float>(m_iNumAxis[i] + 1);
		}

		if ( fRand <= fTotal )
		{
			if ( id )
				*id = i;
			*position = m_vCPPositions[i];
			return true;
		}
	}

	return false;
}

void CDODMod::freeMemory()
{
	m_BombWaypoints.clear();
	m_BreakableWaypoints.clear();
}

// returns map type
int CDODFlags::setup(edict_t *pResourceEntity)
{
	int iNumBombCaps = 0;
	int iNumFlags = 0;
	//bool *CPsVisible;

	m_iNumControlPoints = 0;

	std::memset(m_bBombPlanted,0,sizeof(bool)*MAX_DOD_FLAGS); // all false

	//CPsVisible = CClassInterface::getDODCPVisible(pResourceEntity);

	if ( pResourceEntity )
	{  
		// get the arrays from the resource entity
		CClassInterface::getDODFlagInfo(pResourceEntity,&m_iNumAxis,&m_iNumAllies,&m_iOwner,&m_iAlliesReqCappers,&m_iAxisReqCappers);
		CClassInterface::getDODBombInfo(pResourceEntity,&m_bBombPlanted_Unreliable,&m_iBombsRequired,&m_iBombsRemaining,&m_bBombBeingDefused);
		m_iNumControlPoints = CClassInterface::getDODNumControlPoints(pResourceEntity);
		// get the Capture point positions
		m_vCPPositions = CClassInterface::getDODCP_Positions(pResourceEntity);

	}

	//	string_t model;		
	//	const char *modelname;
	//	bool bVisible;
				

	// find the edicts of the flags using the origin and classname

	for ( int j = 0; j < m_iNumControlPoints; j ++ )
	{
		edict_t *pent;

		Vector vOrigin;

		int i = gpGlobals->maxClients;

		// find visible flags -- with a model
		while ( ++i < gpGlobals->maxEntities &&  m_pFlags[j] == nullptr)
		{
			pent = INDEXENT(i);

			if ( !pent || pent->IsFree() )
				continue;

			if ( std::strcmp(pent->GetClassName(),DOD_CLASSNAME_CONTROLPOINT) == 0 )
			{
				vOrigin = CBotGlobals::entityOrigin(pent);

				if ( vOrigin == m_vCPPositions[j] )
				{
					/*
					bVisible = ((CClassInterface::getEffects(pent) & EF_NODRAW) != EF_NODRAW);

					model = pent->GetIServerEntity()->GetModelName();
					modelname = model.ToCStr();

					if ( bVisible && modelname && *modelname )
					{*/

					if ( m_iAlliesReqCappers[j] || m_iAxisReqCappers[j] || m_iBombsRequired[j] )
					{
						m_pFlags[j] = pent;
					}

					break; // found it
				}
			}
		}

		// no flag for this point
		if ( m_pFlags[j] == nullptr) 
			continue;

		// don't need to check for bombs
		if ( m_iBombsRequired[j] == 0 )
			continue;

		// find bombs near flag
		i = gpGlobals->maxClients;

		while ( ++i < gpGlobals->maxEntities && (m_pBombs[j][0]== nullptr ||m_pBombs[j][1]== nullptr) )
		{
			pent = INDEXENT(i);

			if ( !pent || pent->IsFree() )
				continue;

			if ( std::strcmp(pent->GetClassName(),DOD_CLASSNAME_BOMBTARGET) == 0 )
			{
				vOrigin = CBotGlobals::entityOrigin(pent);

				if ( (vOrigin - m_vCPPositions[j]).Length() < 400.0f )
				{
					if ( m_pBombs[j][0] == nullptr)
					{
						m_pBombs[j][0] = pent;						
					}
					else
						m_pBombs[j][1] = pent;
				}
			}
		}
	}

	// find waypoints
	for ( int i = 0; i < m_iNumControlPoints; i ++ )
	{
		// if we don't know any cap waypoint yet here find one
		if ( m_iWaypoint[i] == -1 )
		{
			// get any nearby waypoint so the bot knows which waypoint to get danger from
			// look for the nearest waypoint which is a cap point
			m_iWaypoint[i] = CWaypointLocations::NearestWaypoint(m_vCPPositions[i],400.0f,-1,false,false,false,nullptr,false,0,false,false,Vector(0,0,0),CWaypointTypes::W_FL_CAPPOINT );

			// still no waypoint, search for any capture waypoint with the same area
			if ( m_iWaypoint[i] == -1 )
			{
				m_iWaypoint[i] = CWaypoints::getWaypointIndex(CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_CAPPOINT,0,i,true));
			}
		}
	}

	m_iNumAxisBombsOnMap = getNumPlantableBombs(TEAM_AXIS);
	m_iNumAlliesBombsOnMap = getNumPlantableBombs(TEAM_ALLIES);

	// sometimes m_iNumControlPoints is larger than it  should be. check the number of flags and bombs we found on the map
	for ( int i = 0; i < m_iNumControlPoints; i ++ )
	{
		if ( m_pFlags[i] != nullptr)
			iNumFlags++;
		if ( m_pBombs[i][0] != nullptr)
			iNumBombCaps++;
		if ( m_pBombs[i][1] != nullptr)
			iNumBombCaps++;
	}

	// update new number
	m_iNumControlPoints = iNumFlags;

	if ( iNumBombCaps >= iNumFlags )
		return DOD_MAPTYPE_BOMB;

	return DOD_MAPTYPE_FLAG;
}

int CDODMod ::getScore(edict_t *pPlayer)
{
	if ( m_pPlayerResourceEntity )
		return CClassInterface::getPlayerScoreDOD(m_pPlayerResourceEntity,pPlayer) + 
		CClassInterface::getPlayerObjectiveScoreDOD(m_pPlayerResourceEntity,pPlayer) - 
		CClassInterface::getPlayerDeathsDOD(pPlayer,m_pPlayerResourceEntity);

	return 0;
}

edict_t *CDODMod :: getBreakable (const CWaypoint *pWpt)
{
	const std::size_t size = m_BreakableWaypoints.size();

	for (std::size_t i = 0; i < size; i++)
	{
		if ( m_BreakableWaypoints[i].pWaypoint == pWpt )
			return m_BreakableWaypoints[i].pEdict;
	}

	return nullptr;
}

edict_t *CDODMod :: getBombTarget (const CWaypoint *pWpt)
{
	const std::size_t size = m_BombWaypoints.size();

	for (std::size_t i = 0; i < size; i++)
	{
		if ( m_BombWaypoints[i].pWaypoint == pWpt )
			return m_BombWaypoints[i].pEdict;
	}

	return nullptr;
}

void CDODMod::roundStart()
{
	if ( !m_pResourceEntity )
		m_pResourceEntity = CClassInterface::FindEntityByNetClass(gpGlobals->maxClients + 1, "CDODObjectiveResource");
	if ( !m_pPlayerResourceEntity )
		m_pPlayerResourceEntity = CClassInterface::FindEntityByNetClass(gpGlobals->maxClients + 1, "CDODPlayerResource");
	if ( !m_pGameRules )
		m_pGameRules = CClassInterface::FindEntityByNetClass(gpGlobals->maxClients + 1, "CDODGameRulesProxy");

	// find main map type
	m_iMapType = m_Flags.setup(m_pResourceEntity);

	//if ( m_iMapType == DOD_MAPTYPE_UNKNOWN )
	//{
		if ( CClassInterface::FindEntityByNetClass(gpGlobals->maxClients + 1,"CDODBombDispenserMapIcon") != nullptr)
		{
			// add bitmask
			m_iMapType |= DOD_MAPTYPE_BOMB;
			/*
			if ( m_iMapType == DOD_MAPTYPE_FLAG) 
				CRCBotPlugin::HudTextMessage(CClients::get(0)->getPlayer(),"RCBot detected Flag map","RCBot2","RCBot2 detected a flag map");
			else if ( m_iMapType == DOD_MAPTYPE_BOMB )
				CRCBotPlugin::HudTextMessage(CClients::get(0)->getPlayer(),"RCBot detected bomb map","RCBot2","RCBot2 detected a bomb map");
			else if ( m_iMapType == 3 )
				CRCBotPlugin::HudTextMessage(CClients::get(0)->getPlayer(),"RCBot detected flag map with bombs ","RCBot2","RCBot2 detected a flag capture map with bombs");
			*/

			const CWaypoint* pWaypointAllies = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_BOMBS_HERE,TEAM_ALLIES);
			const CWaypoint* pWaypointAxis = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_BOMBS_HERE,TEAM_AXIS);

			if ( pWaypointAllies && pWaypointAxis )
			{
				m_bCommunalBombPoint = pWaypointAllies->getArea() > 0 || pWaypointAxis->getArea() > 0;

				m_iBombAreaAllies = pWaypointAllies->getArea();
				m_iBombAreaAxis = pWaypointAxis->getArea();
			}
		}
		//else
		//	m_iMapType = DOD_MAPTYPE_FLAG;
	//}

	// find bombs at waypoints
	m_BombWaypoints.clear();
	m_BreakableWaypoints.clear();

	CWaypoints::updateWaypointPairs(&m_BombWaypoints,CWaypointTypes::W_FL_BOMB_TO_OPEN,"dod_bomb_target");
	CWaypoints::updateWaypointPairs(&m_BreakableWaypoints,CWaypointTypes::W_FL_BREAKABLE,"func_breakable");

	//m_Flags.updateAll();
}
// when a bomb explodes it might leave a part of the ground available
// find it and add it as a waypoint offset
Vector CDODMod :: getGround ( CWaypoint *pWaypoint )
{
	for (edict_wpt_pair_t& m_BombWaypoint : m_BombWaypoints)
	{
		if (m_BombWaypoint.pWaypoint == pWaypoint )
		{
			if (m_BombWaypoint.pEdict )
			{
				if ( CClassInterface::getDODBombState(m_BombWaypoint.pEdict) == 0 )
					return m_BombWaypoint.v_ground;
				
				break;
			}
		}
	}

	return pWaypoint->getOrigin();
}

void CDODMod :: addWaypointFlags (edict_t *pPlayer, edict_t *pEdict, int *iFlags, int *iArea, float *fMaxDistance)
{
	if (isBombMap())
	{
		const int id = m_Flags.getBombID(pEdict);

		if ( id != -1 )
		{
			*iFlags |= CWaypointTypes::W_FL_CAPPOINT;
			*iArea = id;
		}
	}
	
	if (isFlagMap())
	{
		const int id = m_Flags.getFlagID(pEdict);

		if ( id != -1 )
		{
			*iFlags |= CWaypointTypes::W_FL_CAPPOINT;
			*iArea = id;
		}
	}
}

void CDODMod :: modFrame()
{
	// Experimental and needs tested with maps [APG]RoboCop[CL]
	/*if (!m_pResourceEntity)
		m_pResourceEntity = CClassInterface::FindEntityByNetClass(gpGlobals->maxClients+1, "CDODObjectiveResource");
	if ( !m_pPlayerResourceEntity )
		m_pPlayerResourceEntity = CClassInterface::FindEntityByNetClass(gpGlobals->maxClients+1, "CDODPlayerResource");
	if ( !m_pGameRules )
		m_pGameRules = CClassInterface::FindEntityByNetClass(gpGlobals->maxClients+1, "CDODGameRulesProxy");*/
}


int CDODMod ::numClassOnTeam(const int iTeam, const int iClass)
{
	int num = 0;

	for ( int i = 1; i <= CBotGlobals::maxClients(); i ++ )
	{
		edict_t* pEdict = INDEXENT(i);

		if ( !pEdict )
			continue;

		if ( CBotGlobals::entityIsValid(pEdict) )
		{
			if ( CClassInterface::getTeam(pEdict) == iTeam )
			{
				if ( CClassInterface::getPlayerClassDOD(pEdict) == iClass )
					num++;
			}
		}
	}

	return num;
}

void CDODMod ::clientCommand( edict_t *pEntity, const int argc, const char *pcmd, const char *arg1, const char *arg2 )
{
	if ( argc == 1 )
	{
		if ( std::strncmp(pcmd,"voice_",6) == 0 )
		{
			for ( byte i = 0; i < DOD_VC_INVALID; i ++ )
			{
				if ( std::strcmp(&pcmd[6],g_DODVoiceCommands[i].pcmd) == 0 )
				{
					u_VOICECMD vcmd;
					vcmd.voicecmd = i;

					CBroadcastVoiceCommand voicecmd = CBroadcastVoiceCommand(pEntity,vcmd.voicecmd); 

					CBots::botFunction(&voicecmd);

					break;
				}
			}
		}
	}
}

bool CDODMod :: isBreakableRegistered ( edict_t *pBreakable, const int iTeam )
{
	static CWaypoint *pWpt;

	for (edict_wpt_pair_t& m_BreakableWaypoint : m_BreakableWaypoints)
	{
		if (m_BreakableWaypoint.pEdict == pBreakable )
		{
			pWpt = m_BreakableWaypoint.pWaypoint;

			if ( pWpt->hasFlag(CWaypointTypes::W_FL_NOALLIES) )
				return iTeam != TEAM_ALLIES;
			if ( pWpt->hasFlag(CWaypointTypes::W_FL_NOAXIS) )
				return iTeam != TEAM_AXIS;

			return true;
		}
	}

	return false;
}

void CDODMod :: getTeamOnlyWaypointFlags (const int iTeam, int *iOn, int *iOff)
{
	if ( iTeam == TEAM_ALLIES )
	{
		*iOn = CWaypointTypes::W_FL_NOAXIS;
		*iOff = CWaypointTypes::W_FL_NOALLIES;
	}
	else if ( iTeam == TEAM_AXIS )
	{
		*iOn = CWaypointTypes::W_FL_NOALLIES;
		*iOff = CWaypointTypes::W_FL_NOAXIS;
	}
}
