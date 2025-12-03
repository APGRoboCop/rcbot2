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
#ifndef __RCBOT_NAVIGATOR_H__
#define __RCBOT_NAVIGATOR_H__

#include <vector>
#include <queue>
#include <stack>

#include "bot.h"
#include "bot_waypoint.h"

#include "bot_belief.h"

class CNavMesh {
public:
	// other methods...
	static void freeMemory()
	{
		// implementation of freeing memory
	}
};
class CWaypointVisibilityTable;

constexpr float MAX_BELIEF = 200.0f;

class INavigatorNode
{
public:
	Vector getOrigin () { return m_vOrigin; }
protected:
	Vector m_vOrigin;
};

class IBotNavigator
{
public:
	virtual ~IBotNavigator() = default;
	virtual void init () = 0;

	// returns true when working out route finishes, not if successful
	virtual bool workRoute (const Vector& vFrom, const Vector& vTo, bool* bFail, bool bRestart = true, bool bNoInterruptions = false, int iGoalId =
		                        -1, int iConditions = 0, int iDangerId = -1) = 0;

	virtual void rollBackPosition () = 0;

	virtual void failMove () = 0;

	virtual float getNextYaw () = 0;

	virtual bool getNextRoutePoint ( Vector *vPoint ) = 0;

	// Simplified navigation to origin (for SourceMod natives)
	virtual void gotoOrigin(const Vector& vOrigin) = 0;

	Vector getPreviousPoint () { return m_vPreviousPoint; }

	virtual bool hasNextPoint () = 0;

	virtual int getCurrentWaypointID () = 0;

	virtual int getCurrentGoalID () = 0;

	virtual Vector getNextPoint () = 0;

	virtual void updatePosition () = 0;

	virtual bool canGetTo (const Vector& vOrigin) = 0;

	virtual int getCurrentFlags () { return 0; }
	virtual int getPathFlags ( int iPath ) { return 0; }

	virtual float distanceTo (const Vector& vOrigin) = 0;

	virtual float distanceTo ( CWaypoint *pWaypoint ) = 0;

	//virtual void goBack () = 0;

	virtual void freeMapMemory () = 0;		

	virtual void freeAllMemory () = 0;

	virtual bool routeFound () = 0;

	virtual void clear () = 0;

	virtual void getFailedGoals (WaypointList **goals) = 0;

	Vector getGoalOrigin () { return m_vGoal; }

	virtual bool nextPointIsOnLadder () { return false; }

	virtual bool beliefLoad ( ) { return false; }

	virtual bool beliefSave ( bool bOverride = false ) { return false; }

	virtual void belief (const Vector& origin, const Vector& vOther, float fBelief, float fStrength, BotBelief iType) = 0;

	// nearest cover position to vOrigin only
	virtual bool getCoverPosition (const Vector& vCoverOrigin, Vector* vCover) = 0;
	// nearest cover postion to both vectors
	virtual bool getHideSpotPosition (const Vector& vCoverOrigin, Vector* vCover) = 0;

	virtual float getCurrentBelief ( ) { return 0; }

	virtual float getBelief ( int index ) { return 0; }

	virtual void beliefOne ( int iWptIndex, BotBelief iBeliefType, float fDist ) {
	}

	virtual int numPaths ( ) { return 0; }

	virtual Vector getPath ( int pathid ) { return {0,0,0}; }

	virtual bool randomDangerPath (Vector *vec) { return false; }

	bool getDangerPoint ( Vector *vec ) const
	{ *vec = m_bDangerPoint ? m_vDangerPoint : Vector(0,0,0); return m_bDangerPoint; }

	bool wantToLoadBelief () const { return m_bLoadBelief; }
	virtual bool wantToSaveBelief () { return false; }
	float getGoalDistance () const { return m_fGoalDistance; }

	static constexpr int MAX_PATH_TICKS = 200;

protected:
	Vector m_vGoal;
	float m_fGoalDistance = 0.0f;
	Vector m_vPreviousPoint;
	Vector m_vDangerPoint;
	bool m_bDangerPoint = false;
	int m_iBeliefTeam = 0;
	bool m_bBeliefChanged = false;
	bool m_bLoadBelief = false;
};

enum : std::uint8_t
{
	FL_ASTAR_CLOSED = 1,
	FL_ASTAR_PARENT = 2,
	FL_ASTAR_OPEN = 4,
	FL_HEURISTIC_SET = 8
};

class AStarNode
{
public:
	AStarNode() { std::memset(this,0,sizeof(AStarNode)); }
	///////////////////////////////////////////////////////
	void close () { setFlag(FL_ASTAR_CLOSED); }
	void unClose () { removeFlag(FL_ASTAR_CLOSED); }
	bool isOpen () const { return hasFlag(FL_ASTAR_OPEN); }
	void unOpen () { removeFlag(FL_ASTAR_OPEN); }
	bool isClosed () const { return hasFlag(FL_ASTAR_CLOSED); }
	void open () { setFlag(FL_ASTAR_OPEN); }
	//////////////////////////////////////////////////////	
	void setHeuristic (const float fHeuristic) { m_fHeuristic = fHeuristic; setFlag(FL_HEURISTIC_SET); }
	bool heuristicSet () const { return hasFlag(FL_HEURISTIC_SET); }
	float getHeuristic() const { return m_fHeuristic; }

	////////////////////////////////////////////////////////
	void setFlag(const int iFlag) { m_iFlags |= iFlag; }
	bool hasFlag (const int iFlag) const { return (m_iFlags & iFlag) == iFlag; }
	void removeFlag (const int iFlag) { m_iFlags &= ~iFlag; }
	/////////////////////////////////////////////////////////
	int getParent () const
	{
		if ( hasFlag(FL_ASTAR_PARENT) ) return m_iParent;
		return -1;
	}

	void setParent (const int iParent) 
	{ 
		m_iParent = iParent; 

		if ( m_iParent == -1 )
			removeFlag(FL_ASTAR_PARENT); // no parent
		else
			setFlag(FL_ASTAR_PARENT);
	}
	////////////////////////////////////////////////////////
	float getCost () const { return m_fCost; }
	void setCost(const float fCost) { m_fCost = fCost; }
	////////////////////////////////////////////////////////
	// for comparison
	bool precedes (const AStarNode *other) const
	{
		return m_fCost+m_fHeuristic < other->getCost() + other->getHeuristic();
	}
	void setWaypoint (const int iWpt) { m_iWaypoint = iWpt; }
	int getWaypoint () const { return m_iWaypoint; }
private:
	float m_fCost;
	float m_fHeuristic;
	unsigned char m_iFlags;
	int m_iParent;
	int m_iWaypoint;
};
// Insertion sorted list
class AStarListNode
{
public:
	AStarListNode ( AStarNode *data )
	{
		m_Data = data;
		m_Next = nullptr;
	}
	AStarNode *m_Data;
	AStarListNode *m_Next;
};

class AStarOpenList
{
public:
	AStarOpenList()
	{
		m_Head = nullptr;
	}

	bool empty () const
	{
		return m_Head== nullptr;
	}

	AStarNode *top () const
	{
		if ( m_Head == nullptr)
			return nullptr;
		
		return m_Head->m_Data;
	}

	void pop ()
	{
		if ( m_Head != nullptr)
		{
			const AStarListNode *t = m_Head;

			m_Head = m_Head->m_Next;

			delete t;
		}
	}


	void add ( AStarNode *data )
	{
		AStarListNode *newNode = new AStarListNode(data);

		if ( m_Head == nullptr)
			m_Head = newNode;
		else
		{
			if ( data->precedes(m_Head->m_Data) )
			{
				newNode->m_Next = m_Head;
				m_Head = newNode;
			}
			else
			{
				AStarListNode* p = m_Head;
				AStarListNode* t = m_Head->m_Next;

				while ( t != nullptr)
				{
					if ( data->precedes(t->m_Data) )
					{
						p->m_Next = newNode;
						newNode->m_Next = t;
						break;
					}

					p = t;
					t = t->m_Next;
				}

				if ( t == nullptr)
					p->m_Next = newNode;

			}
		}
	}

	void destroy ()
	{
		while ( m_Head != nullptr)
		{
			const AStarListNode* t = m_Head;
			m_Head = m_Head->m_Next;
			delete t;
			t = nullptr;
		}

		m_Head = nullptr;
	}
	
private:
	AStarListNode *m_Head;
};

/*
struct AstarNodeCompare : binary_function<AStarNode*, AStarNode*, bool> 
{
  // Other stuff...
  bool operator()(AStarNode* x, AStarNode* y) const 
  {
	return y->betterCost(x);
  }
};

class AStarOpenList : public vector<AStarNode*> 
{
  AstarNodeCompare comp;
public:
  AStarOpenList(AstarNodeCompare cmp = AstarNodeCompare()) : comp(cmp) {
	make_heap(begin(), end(), comp);
  }
  AStarNode* top() { return front(); }
  void push(AStarNode* x) {
	emplace_back(x);
	push_heap(begin(), end(), comp);
  }
  void pop() {
	pop_heap(begin(), end(), comp);
	pop_back();
  }  
};*/


/*
bool operator<( const AStarNode & A, const AStarNode & B )
{
	return A.betterCost(&B);
}

bool operator<( const AStarNode * A, const AStarNode * B )
{
	return A->betterCost(B);
}*/

enum : std::uint8_t
{
	WPT_SEARCH_AVOID_SENTRIES = 1,
	WPT_SEARCH_AVOID_SNIPERS = 2,
	WPT_SEARCH_AVOID_TEAMMATE = 4
};

typedef struct
{
	int iFrom;
	int iTo;
	bool bValid;
	bool bSkipped;
}failedpath_t;

class CWaypointNavigator : public IBotNavigator
{
public:
	CWaypointNavigator(CBot* pBot)
	{
		CWaypointNavigator::init();
		m_pBot = pBot; 
		m_fNextClearFailedGoals = 0;
		m_bDangerPoint = false;
		m_iBeliefTeam = -1;
		m_bLoadBelief = true;
		m_bBeliefChanged = false;
		std::memset(&m_lastFailedPath,0,sizeof(failedpath_t));
	}

	void init () override;

	CWaypoint *chooseBestFromBelief (const std::vector<CWaypoint*>& goals, bool bHighDanger = false, int iSearchFlags = 0, int iTeam = 0) const;
	CWaypoint *chooseBestFromBeliefBetweenAreas (const std::vector<AStarNode*>& goals, bool bHighDanger = false, bool bIgnoreBelief = false) const;

	float getNextYaw () override;

	bool workRoute (const Vector& vFrom, const Vector& vTo, bool* bFail, bool bRestart = true, bool bNoInterruptions = false, int iGoalId =
		                -1, int iConditions = 0, int iDangerId = -1) override;

	bool getNextRoutePoint ( Vector *vPoint ) override;

	void gotoOrigin(const Vector& vOrigin) override
	{
		if (!m_pBot) return;
		bool bFail = false;
		workRoute(m_pBot->getOrigin(), vOrigin, &bFail, true, false);
	}

	void clear () override;

	Vector getNextPoint () override;

	void updatePosition () override;

	float getBelief (const int index) override
	{ if ( index >= 0 ) return m_fBelief[index]; return 0; }

	void failMove () override;

	bool hasNextPoint () override;

	void freeMapMemory () override;

	void freeAllMemory () override;

	bool canGetTo (const Vector& vOrigin) override;

	bool routeFound () override;

	void rollBackPosition () override;

	bool nextPointIsOnLadder () override;

	void open ( AStarNode *pNode );

	AStarNode *nextNode ();

	float distanceTo (const Vector& vOrigin) override;

	float distanceTo ( CWaypoint *pWaypoint ) override;

	Vector getCoverOrigin ( Vector vCover ); // TODO: Needs implemented properly [APG]RoboCop[CL]

	void clearOpenList ();

	float getCurrentBelief ( ) override;

	//virtual void goBack();
	
	void belief (const Vector& vOrigin, const Vector& vOther, float fBelief, float fStrength, BotBelief iType) override; //TODO: not implemented? [APG]RoboCop[CL]

	void beliefOne ( int iWptIndex, BotBelief iBeliefType, float fDist ) override; //TODO: not implemented? [APG]RoboCop[CL]

	// nearest cover position to vOrigin only
	bool getCoverPosition (const Vector& vCoverOrigin, Vector* vCover) override;
	// nearest cover postion to both vectors
	bool getHideSpotPosition (const Vector& vCoverOrigin, Vector* vCover) override;

	void getFailedGoals (WaypointList **goals) override { *goals = &m_iFailedGoals; }

	int numPaths ( ) override;

	Vector getPath ( int pathid ) override;

	bool randomDangerPath (Vector *vec) override;

	bool beliefLoad ( ) override;

	bool beliefSave ( bool bOverride = false ) override;

	bool wantToSaveBelief () override;

	int getCurrentWaypointID () override
	{
		return m_iCurrentWaypoint;
	}

	int getCurrentGoalID () override
	{
		return m_iGoalWaypoint;
	}

	int getCurrentFlags () override;
	int getPathFlags ( int iPath ) override;

private:
	CBot *m_pBot;

	//CWaypointVisibilityTable *m_pDangerNodes;

	//int m_iPrevWaypoint;
	int m_iCurrentWaypoint;
	int m_iPrevWaypoint;
	int m_iNextWaypoint;
	int m_iGoalWaypoint;
	bool m_bWorkingRoute;

	failedpath_t m_lastFailedPath;

	std::stack<int> m_currentRoute;
	std::queue<int> m_oldRoute;

	int m_iLastFailedWpt;

	AStarNode paths[CWaypoints::MAX_WAYPOINTS];
	AStarNode *curr;
	AStarNode *succ;

	WaypointList m_iFailedGoals;
	float m_fNextClearFailedGoals;

	float m_fBelief [CWaypoints::MAX_WAYPOINTS];

	AStarOpenList m_theOpenList;

	Vector m_vOffset;
	bool m_bOffsetApplied;
};

class CNavMeshNavigator : public IBotNavigator
{
public:
	//TODO: Add Nav Mesh support [APG]RoboCop[CL]
	CNavMesh* m_theNavMesh; // Add a member variable for the NavMesh instance

	CNavMeshNavigator();
	~CNavMeshNavigator() override;

	void CalculateRoute(Vector startNodeID, Vector goalNodeID);

	bool workRoute (const Vector& vFrom, const Vector& vTo, bool* bFail, bool bRestart = true, bool bNoInterruptions = false, int iGoalId =
		                -1, int iConditions = 0, int iDangerId = -1) override;

	void gotoOrigin(const Vector& vOrigin) override {}

	Vector getNextPoint () override;

	void updatePosition () override;

	void freeMapMemory () override;

	void freeAllMemory () override;

	bool routeFound () override;

	bool hasNextPoint () override;

	void rollBackPosition () override {}

	void init () override;

	void belief (const Vector& origin, const Vector& facing, const float fBelief, const float fStrength, const BotBelief iType) override {} //bir3yk

	//void rememberEnemyPosition ( Vector vOrigin );

	//Vector getEnemyPositionPinchPoint ( Vector vOrigin );
private:
	CNavMesh * m_pNavMesh;
};

#endif