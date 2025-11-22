// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include "bot_tf2_points.h"
#include "bot.h"
#include "bot_cvars.h"
#include "bot_fortress.h"
#include "bot_getprop.h"
#include "bot_globals.h"
#include "bot_waypoint.h"
#include "bot_waypoint_locations.h"
#include "engine_wrappers.h"

#include <cstring>

class CBotFuncResetAttackPoint : public IBotFunction
{
public:
	CBotFuncResetAttackPoint(const int team) { iTeam = team; }
	void execute ( CBot *pBot ) override
	{
		if ( pBot->getTeam() == iTeam )
			static_cast<CBotTF2*>(pBot)->updateAttackPoints();
	}
private:
	int iTeam;	
};

class CBotFuncResetDefendPoint : public IBotFunction
{
public:
	CBotFuncResetDefendPoint(const int team) { iTeam = team; }
	void execute ( CBot *pBot ) override
	{
		if ( pBot->getTeam() == iTeam )
			static_cast<CBotTF2*>(pBot)->updateDefendPoints();
	}
private:
	int iTeam;	
};


class CBotFuncPointsUpdated : public IBotFunction
{
public:
	void execute ( CBot *pBot ) override
	{
		static_cast<CBotTF2*>(pBot)->pointsUpdated();
	}	
};

void CTFObjectiveResource::updatePoints()
{
	static CBotFuncResetAttackPoint resetBlueAttack(TF2_TEAM_BLUE);
	static CBotFuncResetDefendPoint resetBlueDefend(TF2_TEAM_BLUE);
	static CBotFuncResetDefendPoint resetRedAttack(TF2_TEAM_RED);
	static CBotFuncResetDefendPoint resetRedDefend(TF2_TEAM_RED);
	static CBotFuncPointsUpdated pointsUpdated;
	bool bChanged = false;

	m_iMonitorPoint[0] = -1;
	m_iMonitorPoint[1] = -1;

	CTeamFortress2Mod::m_ObjectiveResource.resetValidWaypointAreas();

	if ( CTeamFortress2Mod::m_ObjectiveResource.updateAttackPoints(TF2_TEAM_BLUE) )
	{
		CBots::botFunction(&resetBlueAttack);
		bChanged = true;
	}

	if ( CTeamFortress2Mod::m_ObjectiveResource.updateAttackPoints(TF2_TEAM_RED) )
	{
		CBots::botFunction(&resetRedAttack);
		bChanged = true;
	}

	if ( CTeamFortress2Mod::m_ObjectiveResource.updateDefendPoints(TF2_TEAM_BLUE) )
	{
		CBots::botFunction(&resetBlueDefend);
		bChanged = true;
	}

	if ( CTeamFortress2Mod::m_ObjectiveResource.updateDefendPoints(TF2_TEAM_RED) )
	{
		CBots::botFunction(&resetRedDefend);
		bChanged = true;
	}

	if ( bChanged )
		CBots::botFunction(&pointsUpdated);

	CTeamFortress2Mod::m_ObjectiveResource.updateValidWaypointAreas();

}
// INPUT = Waypoint Area
bool CTFObjectiveResource :: isWaypointAreaValid (const int wptarea, const int waypointflags) const
{
//	CWaypoint *pWaypoint;

	if ( wptarea == 0 )
		return true;

	// Translate Waypoint Area to Index
	if ( wptarea < 0 || wptarea > MAX_CONTROL_POINTS )
		return false;

const int cpindex = m_WaypointAreaToIndexTranslation[wptarea];

	if ( cpindex == -1 )
		return false;

	if ( waypointflags & CWaypointTypes::W_FL_AREAONLY )
	{
		// AND
		return (m_ValidPoints[0][0][cpindex].bValid && m_ValidPoints[1][1][cpindex].bValid) || 
			   (m_ValidPoints[0][1][cpindex].bValid && m_ValidPoints[1][0][cpindex].bValid);
	}

	// OR
	return m_ValidAreas[cpindex];
	/*
	for ( int i = 0; i < MAX_CONTROL_POINTS; i ++ )
	{
		pWaypoint = CWaypoints::getWaypoint(m_iControlPointWpt[i]);

		if ( pWaypoint && (pWaypoint->getArea() == wptarea) )
		{
			return m_ValidAreas[i];
		}
	}
	
	// can't find return default
	return m_ValidAreas[wptarea-1];*/
}

bool CTFObjectiveResource::isCPValidWptArea (const int iWptArea, const int iTeam, const ePointAttackDefend_s type) const
{
	if ( iWptArea == 0 )
		return true;

	if ( iWptArea < 1 || iWptArea > MAX_CONTROL_POINTS )
		return false;

	return isCPValid(m_WaypointAreaToIndexTranslation[iWptArea],iTeam,type);
}

// Returns TRUE if waypoint area is worth attacking or defending at this moment
bool CTFObjectiveResource::testProbWptArea (const int iWptArea, const int iTeam) const
{
	const int iCpIndex = m_WaypointAreaToIndexTranslation[iWptArea];

	if ( iTeam != TF2_TEAM_BLUE && iTeam != TF2_TEAM_RED )
		return true;

	if ( iWptArea == 0 )
		return true;

	if ( iWptArea < 1 || iWptArea > MAX_CONTROL_POINTS )
		return true;

	return isCPValid(iCpIndex,iTeam,TF2_POINT_ATTACK) ? randomFloat(0.0f,1.0f) > m_ValidPoints[iTeam-2][TF2_POINT_ATTACK][iCpIndex].fProb : isCPValid(iCpIndex,iTeam,TF2_POINT_DEFEND) ? randomFloat(0.0f,1.0f) > m_ValidPoints[iTeam-2][TF2_POINT_DEFEND][iCpIndex].fProb : true;
}

bool CTFObjectiveResource::isCPValid (const int iCPIndex, const int iTeam, const ePointAttackDefend_s type) const
{
	if ( iCPIndex < 0 || iCPIndex >= MAX_CONTROL_POINTS )
		return false;

	return m_ValidPoints[iTeam-2][type][iCPIndex].bValid;
}

// TODO:  - Base on waypoint danger
// base on base point -- if already have attack point and base point -- less focus on base point
int CTFObjectiveResource::getRandomValidPointForTeam (const int team, const ePointAttackDefend_s type)
{
	std::vector<int> points; // point indices

	float fTotal = 0.0f;

	if (team < 2 || team > 3)
		return 0;

	if ( m_iNumControlPoints == nullptr)
		return 0;

	const int iotherteam = team == 2 ? 3 : 2;

	TF2PointProb_t* arr = m_ValidPoints[team - 2][type];

	for (int i = 0; i < *m_iNumControlPoints; i++)
	{
		if ( arr[i].bValid == true )
		{
			points.emplace_back(i);

			if ( type == TF2_POINT_ATTACK )
			{
				if (GetCappingTeam(i) == team)
					arr[i].fProbMultiplier = 3.0f;
				else if (getLastCaptureTime(i) + 10.0f > gpGlobals->curtime )
					arr[i].fProbMultiplier = 2.0f;
			}
			else
			{
				if (GetCappingTeam(i) == iotherteam)
				{
					const int numplayers = GetNumPlayersInArea(i,iotherteam);

					// IF this is not base point and a lot of players are here, reduce probability of defending
					if ( i != GetBaseControlPointForTeam(team) && numplayers > 1  )
					{
						arr[i].fProbMultiplier = 1.0f - static_cast<float>(numplayers) / static_cast<float>(gpGlobals->maxClients) / 4;

						if ( arr[i].fProbMultiplier <= 0.0f )
							arr[i].fProbMultiplier = 0.1f;
					}
					else // Otherwise there aren't any playres on or is base and has been attacked recently
						arr[i].fProbMultiplier = 4.0f;
				}
				else if (getLastCaptureTime(i) + 10.0f > gpGlobals->curtime )
					arr[i].fProbMultiplier = 2.0f;
			}

			fTotal += arr[i].fProb*arr[i].fProbMultiplier;
		}
	}

	const float fRand = randomFloat(0.0f,fTotal);

	fTotal = 0.0f;

	for (const int index : points)
	{
		fTotal += arr[index].fProb*arr[index].fProbMultiplier;

		if ( fTotal > fRand )
		{
			return m_IndexToWaypointAreaTranslation[index];
		}
	}

	// no points
	return 0;
}

void CTeamRoundTimer::reset()
{
	// Manually reset the object's state
	std::memset(this, 0, sizeof(CTeamRoundTimer));

	m_Resource = CClassInterface::FindEntityByNetClass(gpGlobals->maxClients + 1, "CTeamRoundTimer");

	if (m_Resource.get() != nullptr)
	{
		CClassInterface::setupCTeamRoundTimer(this);
	}
}

bool CTeamControlPointRound :: isPointInRound ( edict_t *point_pent )
{
	for (int i = 0; i < m_ControlPoints.Count(); i++)
	{
		if ( const CBaseHandle* hndl = &m_ControlPoints[i] )
		{ 
			edict_t* pPoint = INDEXENT(hndl->GetEntryIndex());
			const CBaseEntity *pent = pPoint->GetUnknown()->GetBaseEntity();
			if ( point_pent->GetUnknown()->GetBaseEntity() == pent )
				return true;
		}
	}

	return false;
}

CTeamControlPointRound *CTeamControlPointMaster:: getCurrentRound ( )
{
	if ( m_iCurrentRoundIndex == -1 )
		return nullptr;

	CBaseEntity *pent = m_ControlPointRounds[m_iCurrentRoundIndex];

	//extern IServerGameEnts *servergameents;

	//edict_t *p = servergameents->BaseEntityToEdict(pent);
	
	extern IServerGameEnts *servergameents; // servergameents not used? [APG]RoboCop[CL]
	extern IServerTools *servertools;
	
	// HACK: we use one of the known CBaseEntity-sized entities to compute the offset to the first subclass member for CTeamControlPointMaster / CTeamControlPointRound
	const std::size_t baseEntityOffset = servertools->GetEntityFactoryDictionary()->FindFactory("simple_physics_brush")->GetEntitySize();

	return reinterpret_cast<CTeamControlPointRound*>(reinterpret_cast<std::uintptr_t>(pent) + baseEntityOffset);
}

//////////////////


void CTFObjectiveResource::setup ()
{
	CClassInterface::getTF2ObjectiveResource(this);

	//std::memset(m_pControlPoints,0,sizeof(edict_t*)*MAX_CONTROL_POINTS);
	std::memset(m_pControlPoints, 0, sizeof(*m_pControlPoints) * MAX_CONTROL_POINTS);
	std::memset(m_iControlPointWpt,0xFF,sizeof(int)*MAX_CONTROL_POINTS);
	std::memset(m_fLastCaptureTime,0,sizeof(float)*MAX_CONTROL_POINTS);
	// Find control point entities

	Vector vOrigin;

	int i = gpGlobals->maxClients;

	std::memset(m_IndexToWaypointAreaTranslation,0,sizeof(int)*MAX_CONTROL_POINTS);
	std::memset(m_WaypointAreaToIndexTranslation,0xFF,sizeof(int)*(MAX_CONTROL_POINTS+1));

	// find visible flags -- with a model
	while ( ++i < gpGlobals->maxEntities )
	{
		edict_t* pent = INDEXENT(i);

		if ( !pent || pent->IsFree() )
			continue;
			
		if ( std::strcmp(pent->GetClassName(),"team_control_point") == 0 )
		{
			vOrigin = CBotGlobals::entityOrigin(pent);

			for ( int j = 0; j < *m_iNumControlPoints; j ++ )
			{
				if ( m_pControlPoints[j].get() != nullptr)
					continue;

				if ( vOrigin == m_vCPPositions[j] )
				{
					m_pControlPoints[j] = MyEHandle(pent);
					//m_pControlPointClass[j] = CTeamControlPoint::getPoint(pent);
				}
			}
		}
	}

	for (int j = 0; j < *m_iNumControlPoints; j++)
	{
		vOrigin = m_vCPPositions[j];

		if ( m_iControlPointWpt[j] == -1 )
		{
			const int iWpt = CWaypointLocations::NearestWaypoint(vOrigin, 1024.0f, -1, false, false, false, nullptr, false, 0, false,
			                                                     false, Vector(0, 0, 0), CWaypointTypes::W_FL_CAPPOINT);
			const CWaypoint* pWaypoint = CWaypoints::getWaypoint(iWpt);
			m_iControlPointWpt[j] = iWpt;

			// For compatibility -- old waypoints are already set with an area, so take the area from the waypoint here
			// in the future waypoints will automatically be set to the waypoint area anyway
			if ( pWaypoint )
			{
				const int iArea = pWaypoint->getArea();
				m_IndexToWaypointAreaTranslation[j] = iArea;

				if ( iArea >= 1 && iArea < MAX_CONTROL_POINTS )
					m_WaypointAreaToIndexTranslation[iArea] = j;
			}
			else
			{
				m_IndexToWaypointAreaTranslation[j] = 0;
				m_WaypointAreaToIndexTranslation[j+1] = -1;
			}
		}
	}
	
	m_bInitialised = true;
}

int CTFObjectiveResource :: getControlPointArea ( edict_t *pPoint )
{
	for (int j = 0; j < *m_iNumControlPoints; j++)
	{
		if ( m_pControlPoints[j] == pPoint )
			return j+1; // return waypoint area (+1)
	}

	return 0;
}
void CTFObjectiveResource::	debugprint () const
{
	edict_t *pEdict = CClients::getListenServerClient();

	CBotGlobals::botMessage(pEdict,0,"m_iNumControlPoints = %d",*m_iNumControlPoints);
	CBotGlobals::botMessage(pEdict,0,"m_bBlocked[8]\t[%s,%s,%s,%s,%s,%s,%s,%s]",m_bBlocked[0]?"Y":"N",m_bBlocked[1]?"Y":"N",m_bBlocked[2]?"Y":"N",m_bBlocked[3]?"Y":"N",m_bBlocked[4]?"Y":"N",m_bBlocked[5]?"Y":"N",m_bBlocked[6]?"Y":"N",m_bBlocked[7]?"Y":"N");
	CBotGlobals::botMessage(pEdict,0,"m_bCPLocked[byte]\t[%d]",*m_bCPLocked);
	CBotGlobals::botMessage(pEdict,0,"m_bCPLocked[8]\t[%s,%s,%s,%s,%s,%s,%s,%s]",m_bCPLocked[0]?"Y":"N",m_bCPLocked[1]?"Y":"N",m_bCPLocked[2]?"Y":"N",m_bCPLocked[3]?"Y":"N",m_bCPLocked[4]?"Y":"N",m_bCPLocked[5]?"Y":"N",m_bCPLocked[6]?"Y":"N",m_bCPLocked[7]?"Y":"N");
	CBotGlobals::botMessage(pEdict,0,"m_bCPIsVisible[8]\t[%s,%s,%s,%s,%s,%s,%s,%s]",m_bCPIsVisible[0]?"Y":"N",m_bCPIsVisible[1]?"Y":"N",m_bCPIsVisible[2]?"Y":"N",m_bCPIsVisible[3]?"Y":"N",m_bCPIsVisible[4]?"Y":"N",m_bCPIsVisible[5]?"Y":"N",m_bCPIsVisible[6]?"Y":"N",m_bCPIsVisible[7]?"Y":"N");
	CBotGlobals::botMessage(pEdict,0,"m_iOwner[8]\t[%s,%s,%s,%s,%s,%s,%s,%s]",m_iOwner[0]==2?"red":m_iOwner[0]==3?"blue":"unassigned",m_iOwner[1]==2?"red":m_iOwner[1]==3?"blue":"unassigned",m_iOwner[2]==2?"red":m_iOwner[2]==3?"blue":"unassigned",m_iOwner[3]==2?"red":m_iOwner[3]==3?"blue":"unassigned",m_iOwner[4]==2?"red":m_iOwner[4]==3?"blue":"unassigned",m_iOwner[5]==2?"red":m_iOwner[5]==3?"blue":"unassigned",m_iOwner[6]==2?"red":m_iOwner[6]==3?"blue":"unassigned",m_iOwner[7]==2?"red":m_iOwner[7]==3?"blue":"unassigned");
}

int CTFObjectiveResource::NearestArea (const Vector& vOrigin) const
{
	int iNearest = -1;
	float fNearest = 2048.0f;
	float fDist; //TODO: fDist could be reduced? [APG]RoboCop[CL]

	for (int i = 0; i < *m_iNumControlPoints; i++)
	{
		if ( (fDist = (m_vCPPositions[i]-vOrigin).Length()) < fNearest )
		{
			fNearest = fDist;
			iNearest = i;
		}
	}

	if ( iNearest == -1 )
		return 0;

	// Add one for waypoint area
	return m_IndexToWaypointAreaTranslation[iNearest];
}

/*CTeamControlPoint *CTeamControlPoint::getPoint ( edict_t *pent )
{
	return (CTeamControlPoint*)((((unsigned long)pent) + rcbot_const_point_offset.GetInt())); //MAP_CLASS(CTeamControlPoint,(((unsigned long)pent) + offset),knownoffset);
}*/


bool CTFObjectiveResource :: updateDefendPoints (const int team)
{
	/*int other = (team==2)?3:2;

	return GetCurrentAttackPoint(other);
	*/
	int signature = 0;
	int other;
	int prev;
	const bool isPayLoadMap = CTeamFortress2Mod::isMapType(TF_MAP_CART)||CTeamFortress2Mod::isMapType(TF_MAP_CARTRACE);

	//CTeamControlPoint *pPoint;
	//CTeamControlPointMaster *pMaster = CTeamFortress2Mod::getPointMaster();
	CTeamControlPointRound *pRound = CTeamFortress2Mod::getCurrentRound();
	
	if ( m_ObjectiveResource.get() == nullptr) // not set up yet
		return false;
	if ( team == 0 ) // invalid team
		return false;

	TF2PointProb_t* arr = m_ValidPoints[team - 2][TF2_POINT_DEFEND];

	// reset array
	std::memset(arr,0,sizeof(TF2PointProb_t)*MAX_CONTROL_POINTS);

	for (int i = 0; i < *m_iNumControlPoints; i++)
	{
		arr[i].fProbMultiplier = 1.0f;
		arr[i].fProb = 1.0f;
		std::memset(arr[i].iPrev,0xFF,sizeof(int)*MAX_PREVIOUS_POINTS);

		// not visible
		if ( m_bCPIsVisible[i] == 0 )
			continue;
		// not unlocked
		if ( m_flUnlockTimes[i] > gpGlobals->curtime )
			continue;
		// not in round
		if ( m_pControlPoints[i] && pRound && !pRound->isPointInRound(m_pControlPoints[i]) )
			continue;
		//int reqcappers = GetRequiredCappers(i,team);

		//if ( m_pControlPoints[i] )
		//	pPoint = CTeamControlPoint::getPoint(m_pControlPoints[i]);
		
		// We own this point
		if ( GetOwningTeam(i) == team )
		{
			// The other team can capture
			other = team==2?3:2;

			if ( TeamCanCapPoint(i,other) )
			{
				// if the other team has capture the previous points
				if ( (prev = GetPreviousPointForPoint(i,other,0)) != -1 )
				{
					if ( prev == i )
					{
						arr[i].bValid = true;
						continue;
					}
					bool bEnemyCanCap = true;

					for (int j = 0; j < MAX_PREVIOUS_POINTS; j++)
					{
						// need to go through each previous point to update the array
						// DONT BREAK!!!
						prev = GetPreviousPointForPoint(i,other,j);
						arr[i].iPrev[j] = prev;

						if ( prev == -1 )
							continue;
						if ( GetOwningTeam(prev) != other )
							bEnemyCanCap = false;
					}

					if ( !bEnemyCanCap )
					{
						arr[i].bPrev = true;	
						arr[i].bValid = false;
						// Check later by checking prev points
					}
					else
					{
						arr[i].bValid = true;
					}
					/*
						// other team has captured previous points
						if ( j == 3 )
						{
							arr[i].bValid = true;
							continue;
						}
						else
						{
							continue;*/
				}
				else
				{
					if ( CTeamFortress2Mod::isAttackDefendMap() )
						arr[i].bValid = true;
					else
					{
						const int basepoint = GetBaseControlPointForTeam(team);
						arr[i].bValid = true;						

						if ( i == basepoint )
						{
							// check that all other points are owned
							int iNumOwned = 0;
							int iNumAvailable = 0;

							for ( int j = 0; j < *m_iNumControlPoints; j ++ )
							{
								// not visible
								if ( m_bCPIsVisible[j] == 0 )
									continue;
								// not unlocked
								if ( m_flUnlockTimes[j] > gpGlobals->curtime )
									continue;
								// not in round
								if ( m_pControlPoints[j] && pRound && !pRound->isPointInRound(m_pControlPoints[j]) )
									continue;

								if ( GetOwningTeam(j) == other )
									iNumOwned ++;

								iNumAvailable++;
							}

							if ( iNumOwned == iNumAvailable-1 )
							{								
								// other team can capture
								arr[i].fProb = 1.0f;
							}
							else if ( iNumOwned == iNumAvailable-2 )
							{
								// other team can capture this as the next point
								arr[i].fProb = bot_defrate.GetFloat();
							}
							else // still valid but very low probability of ever defending here
								arr[i].fProb = 0.001f;
						}
					}
				}
			}
			else
			{
				//arr[i].bValid = true;
				//arr[i].fProb = 0.1f; // 10% of the time defend here
			}
		}
	}
	// do another search through the previous points
	for (int i = 0; i < *m_iNumControlPoints; i++)
	{
		if ( arr[i].bPrev )
		{
			int iNumPrevPointsAvail = 0;

			// Check this points prevous points
			for (const int j : arr[i].iPrev)
			{
				if (j != -1 )
				{
					// the previous point is not valid
					if ( arr[j].bValid )
						iNumPrevPointsAvail++;
				}
			}

			// only one more point to go until this point
			if ( iNumPrevPointsAvail == 1 )
			{
				// this point is next because the current valid points are required
				arr[i].bNextPoint = true;
		
				// other team can capture this as the next point
				// lower chance of defending the next point before round has started!!! Get everyone up!!
				arr[i].fProb = CTeamFortress2Mod::hasRoundStarted() ? bot_defrate.GetFloat() : bot_defrate.GetFloat()*0.5f;
			}
		}
	}

	for (int i = 0; i < *m_iNumControlPoints; i++)
	{
		if ( arr[i].bNextPoint )
			arr[i].bValid = true;
		else if ( arr[i].bValid )
		{
			bool bfound = false;

			// find this point in one of the previous points
			for (int j = 0; j < *m_iNumControlPoints; j++)
			{
				if ( i == j )
					continue;

				if ( arr[j].bPrev )
				{
					for (const int k : arr[j].iPrev)
					{
						if (k == i )
						{
							bfound = true;
							break;
						}
					}	
				}

				if ( bfound )
					break;

			}

			if ( bfound )
			{
				arr[i].fProb = 1.0f;
			}
			else
			{
				arr[i].fProb = 0.1f;
			}
		}		
	}

	// In Payload give lower numbers higher priority 
	if ( isPayLoadMap )
	{
		float fMaxProb = 1.0f;
		bool bFirst = true;

		other = team==2?3:2;

		for (int i = 0; i < *m_iNumControlPoints; i++)
		{
			if ( arr[i].bValid )
			{
				edict_t *pPayloadBomb = CTeamFortress2Mod::getPayloadBomb(other);

				if ( pPayloadBomb != nullptr)
				{
					if ( bFirst )
					{
						// TODO: update probability depending on distance to payload bomb
						const float fDist = (CBotGlobals::entityOrigin(pPayloadBomb) - m_vCPPositions[i]).Length();

						bFirst = false;

						if ( fDist > rcbot_tf2_payload_dist_retreat.GetFloat() )
						{
							arr[i].fProb = 1.0f;

							if ( !CTeamFortress2Mod::hasRoundStarted() )
								fMaxProb = 0.1f;
							else
								fMaxProb = fMaxProb/4;
						}
						else
						{
							arr[i].fProb = bot_defrate.GetFloat();

							const int j = i + 1;

							if ( j < *m_iNumControlPoints )
							{
								if ( arr[j].bValid == false )
								{
									if (!pRound || (m_pControlPoints[j] && pRound->isPointInRound(m_pControlPoints[j])))
										arr[j].bValid = true; // this is the next point - move back lads
								}
							}
						}
					}
					else
					{
						arr[i].fProb = fMaxProb;
						fMaxProb = fMaxProb/4;
					}
				}
				else
				{
					arr[i].fProb = fMaxProb;
					fMaxProb = fMaxProb/4;
				}
				
				//arr[i].fProb *= arr[i].fProb; // square it
			}
		}
	}

	// update signature
	for (int i = 0; i < *m_iNumControlPoints; i++)
	{
		const byte *barr = reinterpret_cast<byte*>(&arr[i]);

		for ( byte j = 0; j < sizeof(TF2PointProb_t); j ++ )
			signature = signature + (barr[j]*(i+1)+j);
	}

	if ( signature != m_PointSignature[team-2][TF2_POINT_DEFEND] )
	{
		m_PointSignature[team-2][TF2_POINT_DEFEND] = signature;
		return true;
	}

	return false;
}

void CTFObjectiveResource :: think ()
{
	if ( m_bInitialised && m_fNextCheckMonitoredPoint < engine->Time() )
	{
		bool bupdate = m_fUpdatePointTime < engine->Time();

		int team = 0;

		do
		{
			if ( m_iMonitorPoint[team] != -1 )
			{
				for (int j = 0; j < MAX_PREVIOUS_POINTS; j++)
				{
					const int prev = GetPreviousPointForPoint(m_iMonitorPoint[team],team+2,j);

					if ( prev != -1 && GetOwningTeam(prev)!=team+2 )
					{
						bupdate = true;
						break;
					}
				}
			}
			team++;
		}while (team < 2 && bupdate==false);

		if ( bupdate )
		{
			updatePoints();

			m_fNextCheckMonitoredPoint = engine->Time() + 5.0f;
			m_fUpdatePointTime = engine->Time() + rcbot_tf2_autoupdate_point_time.GetFloat();
		}
		else
			m_fNextCheckMonitoredPoint = engine->Time() + 1.0f;
	}
	
}

// return true if bots should change attack point
bool CTFObjectiveResource :: updateAttackPoints (const int team)
{	
	int prev;
	int signature = 0;
	CTeamControlPointRound *pRound = CTeamFortress2Mod::getCurrentRound();

	if ( m_ObjectiveResource.get() == nullptr) // not set up yet
		return false;
	if ( team == 0 )
		return false;

	TF2PointProb_t* arr = m_ValidPoints[team - 2][TF2_POINT_ATTACK];

	// reset array
	std::memset(arr,0,sizeof(TF2PointProb_t)*MAX_CONTROL_POINTS);
	std::memset(arr->iPrev,0xFF,sizeof(int)*MAX_PREVIOUS_POINTS);

	if ( team == TF2_TEAM_RED && CTeamFortress2Mod::isAttackDefendMap() )
	{
		// no attacking for red on this map
		return false;
	}

	for (int i = 0; i < *m_iNumControlPoints; i++)
	{
		arr[i].fProb = 1.0f;
		arr[i].fProbMultiplier = 1.0f;
		std::memset(arr[i].iPrev,0xFF,sizeof(int)*MAX_PREVIOUS_POINTS);

		// not visible
		if ( m_bCPIsVisible[i] == 0 )
			continue;
		// not unlocked
		if ( m_flUnlockTimes[i] > engine->Time() )
			continue;
		// not in round
		if ( m_pControlPoints[i] && pRound && !pRound->isPointInRound(m_pControlPoints[i]) )
			continue;

		// We don't own this point
		if ( GetOwningTeam(i) != team )
		{
			// we can capture
			if ( TeamCanCapPoint(i,team) )
			{
				// if we have captured the previous points we can capture
				if ( (prev = GetPreviousPointForPoint(i,team,0)) != -1 )
				{
					if ( prev == i )
					{
						/*int other = (team==2)?3:2;

						// find the base point
						int basepoint = GetBaseControlPointForTeam(other);

						if ( i == basepoint )
						{
							arr[i].fProb = 0.25f;
						}*/

						arr[i].bValid = true;
					}
					else
					{
						bool bCanCap = true;

						for ( int j = 0; j < MAX_PREVIOUS_POINTS; j ++ )
						{
							// need to go through each previous point to update the array
							// DONT BREAK!!!
							prev = GetPreviousPointForPoint(i,team,j);
							arr[i].iPrev[j] = prev;

							if ( prev == -1 )
								continue;
							if ( GetOwningTeam(prev) != team )
								bCanCap = false;
						}

						if ( !bCanCap )
						{
							arr[i].bPrev = true;	
							arr[i].bValid = false;
							// Check later by checking prev points
						}
						else
						{
							arr[i].bValid = true;

							m_iMonitorPoint[team-2] = i;
						}
					}
				}
				else
				{
					if ( !CTeamFortress2Mod::isAttackDefendMap() )
					{
						// if its not an attack defend map check previous points are owned
						const int other = team==2?3:2;

						// find the base point
						const int basepoint = GetBaseControlPointForTeam(other);

						/*if ( i == basepoint )
						{
							arr[i].bValid = true;
							arr[i].fProb = 0.25f;
						}
						else */
					
						if ( basepoint == 0 )
						{
							bool allowned = true;

							// make sure bot owns all points above this point
							for ( int x = i+1; x < *m_iNumControlPoints; x ++ )
							{
								if ( GetOwningTeam(x) != team )
								{
									allowned = false;
									break;
								}
							}				

							if ( allowned )
								arr[i].bValid  = true;
						
							continue;
						}
						if ( basepoint == *m_iNumControlPoints-1 )
						{
							bool allowned = true;
							// make sure team owns all points below this point
							for ( int x = 0; x < i; x ++ )
							{
								if ( GetOwningTeam(x) != team )
								{
									allowned = false;
									break;
								}
							}				

							if ( allowned )
								arr[i].bValid  = true;

							continue;
						}
					}

					arr[i].bValid  = true;
				}
			}
		}
	}

	// Flush out less important cap points
	for (int i = 0; i < *m_iNumControlPoints; i++)
	{
		if ( arr[i].bValid )
		{
			bool bfound = false;

			// find this point in one of the previous points
			for (int j = 0; j < *m_iNumControlPoints; j++)
			{
				if ( i == j )
					continue;

				if ( arr[j].bPrev )
				{
					for (const int k : arr[j].iPrev)
					{
						if (k == i )
						{
							bfound = true;
							break;
						}
					}	
				}

				if ( bfound )
					break;

			}

			if ( bfound )
			{
				arr[i].fProb = 1.0f;
			}
			else
			{
				arr[i].fProb = 0.1f;
			}
		}		
	}

	// In Payload give lower numbers higher priority 
	if ( CTeamFortress2Mod::isMapType(TF_MAP_CART) || CTeamFortress2Mod::isMapType(TF_MAP_CARTRACE) )
	{
		for (int i = 0; i < *m_iNumControlPoints; i++)
		{
			if ( arr[i].bValid )
			{
				arr[i].fProb = static_cast<float>(*m_iNumControlPoints + 1 - i);
				arr[i].fProb *= arr[i].fProb; // square it
			}
		}
	}

	for (int i = 0; i < *m_iNumControlPoints; i++)
	{
		const byte *barr = reinterpret_cast<byte*>(&arr[i]);

		for ( byte j = 0; j < sizeof(TF2PointProb_t); j ++ )
			signature = signature + (barr[j]*(i+1)+j);
	}

	if ( signature != m_PointSignature[team-2][TF2_POINT_ATTACK] )
	{
		m_PointSignature[team-2][TF2_POINT_ATTACK] = signature;
		return true;
	}

	return false;

}

void CTFObjectiveResource :: updateCaptureTime(const int index)
{
	m_fLastCaptureTime[index] = engine->Time();
}

float CTFObjectiveResource :: getLastCaptureTime(const int index) const
{
	return m_fLastCaptureTime[index];
}
