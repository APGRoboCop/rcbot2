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
#ifndef __RCBOT_WAYPOINT_H__
#define __RCBOT_WAYPOINT_H__

#include <cstdio>

// this must be before bot_client.h to avoid unknown override / missing type warnings
#include <vector>
using WaypointList = std::vector<int>;

#include "bot.h"
#include "bot_client.h"
#include "bot_mtrand.h"
#include "bot_wpt_color.h"

// #include "bot_navigator.h"

class CWaypointVisibilityTable;
class CClient;


class CWaypointAuthorInfo
{
public:
	char szAuthor[32];
	char szModifiedBy[32];
};

class CWaypointHeader
{
public:
	char szFileType[16];
	char szMapName[64];
	int iVersion;
	int iNumWaypoints;
	int iFlags;
};

struct edict_wpt_pair_t
{
	MyEHandle pEdict; // MyEHandle fixes problems with reused edict slots
	CWaypoint *pWaypoint;
	Vector v_ground;
};

enum : std::uint8_t
{
	DRAWTYPE_EFFECTS = 0,
	DRAWTYPE_DEBUGENGINE,
	DRAWTYPE_DEBUGENGINE2,
	DRAWTYPE_DEBUGENGINE3,
	DRAWTYPE_BELIEF,
	DRAWTYPE_MAX
};

class CWaypoint;


class CWaypointType
{
public:
	CWaypointType ( int iBit, const char *szName, const char *szDescription, const WptColor& vColour, int iModBits = BITS_MOD_ALL, int iImportance = 0 );

	const char* getName() const { return m_szName; }
	const char* getDescription() const { return m_szDescription; }

	bool isBitsInFlags(const int iFlags) const { return (iFlags & m_iBit) == m_iBit; }
	int getBits() const { return m_iBit; }
	void setMods (const int iMods){ m_iMods = iMods; }// input bitmask of mods (32 max)
	bool forMod(const int iMod) const { return (1 << iMod & m_iMods) == 1 << iMod; }
	WptColor getColour() const { return m_vColour; }
	int getImportance() const { return m_iImportance; }

	bool operator<(const CWaypointType& other) const
	{
		return m_iImportance < other.getImportance();
	}

	//virtual void giveTypeToWaypoint ( CWaypoint *pWaypoint );
	//virtual void removeTypeFromWaypoint ( CWaypoint *pWaypoint );

private:
	int m_iMods; // mods allowed
	int m_iBit; // bits used
	char *m_szName; // e.g. "jump"/"ladder"
	char *m_szDescription; // e.g. "will jump here"/"will climb here"
	int m_iImportance;
	WptColor m_vColour;
};


/*
class CCrouchWaypointType : public CWaypointType
{
public:
	CCrouchWaypointType();
	
	void giveTypeToWaypoint ( CWaypoint *pWaypoint );
	void removeTypeFromWaypoint ( CWaypoint *pWaypoint );
};*/

class CWaypointTypes
{
public:

// if you're adding a new waypoint type, don't forget to update CWaypointTypes :: setup()
	static constexpr int W_FL_NONE           = 0;
	static constexpr int W_FL_JUMP           = 1 << 0;
	static constexpr int W_FL_CROUCH         = 1 << 1;
	static constexpr int W_FL_UNREACHABLE    = 1 << 2;
	static constexpr int W_FL_LADDER         = 1 << 3;
	
	static constexpr int W_FL_FLAG           = 1 << 4;
	static constexpr int W_FL_RESCUEZONE     = 1 << 4; // Counter-Strike: Source --> Hostage rescue zone
	
	static constexpr int W_FL_CAPPOINT       = 1 << 5;
	static constexpr int W_FL_GOAL           = 1 << 5; // Synergy & Counter-Strike: Source --> Map Goal
	
	static constexpr int W_FL_NOBLU          = 1 << 6;
	static constexpr int W_FL_NOAXIS         = 1 << 6;
	static constexpr int W_FL_NOTERRORIST    = 1 << 6; // Counter-Strike: Source --> Terrorists cannot use this waypoint
	
	static constexpr int W_FL_NORED          = 1 << 7;
	static constexpr int W_FL_NOALLIES       = 1 << 7;
	static constexpr int W_FL_NOCOUNTERTR    = 1 << 7; // Counter-Strike: Source --> Counter-Terrorists cannot use this waypoint
	
	static constexpr int W_FL_HEALTH         = 1 << 8;
	static constexpr int W_FL_OPENS_LATER    = 1 << 9;
	
	static constexpr int W_FL_ROCKET_JUMP    = 1 << 10;
	static constexpr int W_FL_BOMB_TO_OPEN   = 1 << 10; // DOD:S
	static constexpr int W_FL_DOOR           = 1 << 10; // Counter-Strike: Source --> Check for door
	
	static constexpr int W_FL_SNIPER         = 1 << 11;
	static constexpr int W_FL_AMMO           = 1 << 12;
	
	static constexpr int W_FL_RESUPPLY       = 1 << 13;
	static constexpr int W_FL_BOMBS_HERE     = 1 << 13;
	
	static constexpr int W_FL_SENTRY         = 1 << 14;
	static constexpr int W_FL_MACHINEGUN     = 1 << 14;
	
	static constexpr int W_FL_DOUBLEJUMP     = 1 << 15;
	static constexpr int W_FL_PRONE          = 1 << 15;
	static constexpr int W_FL_TELE_ENTRANCE  = 1 << 16;
	static constexpr int W_FL_TELE_EXIT      = 1 << 17;
	static constexpr int W_FL_DEFEND         = 1 << 18;
	static constexpr int W_FL_AREAONLY       = 1 << 19;
	static constexpr int W_FL_ROUTE          = 1 << 20;
	static constexpr int W_FL_WAIT_GROUND    = 1 << 21;
	
	static constexpr int W_FL_NO_FLAG        = 1 << 22;
	static constexpr int W_FL_COVER_RELOAD   = 1 << 22; // DOD:S only
	static constexpr int W_FL_NO_HOSTAGES    = 1 << 22; // Counter-Strike: Source --> Bots escorting hostages cannot use this waypoint
	
	static constexpr int W_FL_LIFT           = 1 << 23;
	static constexpr int W_FL_FLAGONLY       = 1 << 24;
	static constexpr int W_FL_FALL           = 1 << 25;
	static constexpr int W_FL_BREAKABLE      = 1 << 26;
	static constexpr int W_FL_SPRINT         = 1 << 27;
	static constexpr int W_FL_TELEPORT_CHEAT = 1 << 28; // teleports bots to the next waypoint (cheat)
	static constexpr int W_FL_OWNER_ONLY     = 1 << 29; // Only owners of this area can use the waypoint
	static constexpr int W_FL_USE			 = 1 << 30; // Synergy: Use Button/Door
	//static const int W_FL_ATTACKPOINT = (1 << 30); // Tactical waypoint -- each squad will go to different attack points and signal others to go

    static void setup();

	static void addType(CWaypointType *type);

    static void printInfo(CWaypoint *pWpt, edict_t *pPrintTo, float duration = 6.0f);

    static void displayTypesMenu(edict_t *pPrintTo);

	static CWaypointType *getType(const char *szType);

    static void showTypesOnConsole(edict_t *pPrintTo);

    static void selectedType(CClient *pClient);

    static void freeMemory();

    static WptColor getColour(int iFlags);

    static CWaypointType *getTypeByIndex(std::size_t iIndex);

    static std::size_t getNumTypes();

    static CWaypointType *getTypeByFlags(int iFlags);

private:
	static std::vector<CWaypointType*> m_Types;
};

class CWaypointTest
{
public:
	void go(edict_t* pPlayer);
};

struct wpt_opens_later_t
{
	float fNextCheck;
	Vector vOrigin;
	bool bVisibleLastCheck;
};

class CWaypoint //: public INavigatorNode
{
public:
	//static const int MAX_PATHS = 8;
	// Waypoint flags (up to 32)
	
	static constexpr float WAYPOINT_HEIGHT = 72.0f;
	static constexpr int WAYPOINT_WIDTH = 8;
	static constexpr int PATHWAYPOINT_WIDTH = 4;

	CWaypoint ()
	{
		m_thePaths.clear();
		init();
		//m_iId = -1;
	}

	CWaypoint(const Vector& vOrigin, const int iFlags = 0, const int iYaw = 0)
	{
		m_thePaths.clear();
		init(); // this function resets a bunch of variables that are manually set again below - caxanga334
		m_bUsed = true;
		setAim(iYaw);
		m_fNextCheckGroundTime = 0.0f;
		m_bHasGround = false;
		m_fRadius = 0.0f;
		m_OpensLaterInfo.clear();
		m_bIsReachable = true;
		m_fCheckReachableTime = 0.0f;
		m_vOrigin = vOrigin;
		m_iFlags = iFlags;
		//m_iId = iId;
	}

	bool checkGround();

	void setAim(const int fYaw)
	{
		m_iAimYaw = fYaw;
	}

	float getAimYaw() const
	{
		return static_cast<float>(m_iAimYaw);
	}

	Vector getOrigin()
	{
		return m_vOrigin;
	}

	void init();

	void addFlag(const int iFlag)
	{
		m_iFlags |= iFlag;
	}

	void removeFlag(const int iFlag)
	{
		m_iFlags &= ~iFlag;
	}

	// removes all waypoint flags
	void removeFlags()
	{
		m_iFlags = 0;
	}

	bool hasFlag(const int iFlag) const
	{
		return (m_iFlags & iFlag) == iFlag;
	}

	bool hasSomeFlags(const int iFlag) const
	{
		return (m_iFlags & iFlag) > 0;
	}

	void move(const Vector& origin)
	{
		// move to new origin
		m_vOrigin = origin;
	}

	void checkAreas(edict_t *pActivator); // TODO: Needs implemented properly [APG]RoboCop[CL]

	// show info to player
	void info(edict_t *pEdict);

	// methods
	void touched(); // TODO: Needs implemented properly [APG]RoboCop[CL]

	void draw(edict_t* pEdict, bool bDrawPaths, unsigned short int iDrawType);

	bool addPathTo(int iWaypointIndex);
	void removePathTo(int iWaypointIndex);

	void addPathFrom(int iWaypointIndex);
	void removePathFrom(int iWaypointIndex);

	bool checkReachable();

	bool isPathOpened(const Vector& vPath);

	bool isUsed() const
	{
		return m_bUsed;
	}

	//bool touched ( edict_t *pEdict );
	bool touched(const Vector& vOrigin, const Vector& vOffset, float fTouchDist, bool onground = true);

	static void botTouch ( CBot *pBot ); // TODO: Needs implemented properly [APG]RoboCop[CL]

	void freeMapMemory()
	{
		m_thePaths.clear();
	}

	int getArea() const { return m_iArea; }
	void setArea(const int area) { m_iArea = area; }

	void drawPaths(edict_t* pEdict, unsigned short int iDrawType) const;

	void drawPathBeam(CWaypoint* to, unsigned short int iDrawType, bool bIncoming = false) const;

	void setUsed(const bool bUsed) { m_bUsed = bUsed; }

	inline void clearPaths();

	float distanceFrom(CWaypoint* other) const
	{
		return distanceFrom(other->getOrigin());
	}

	float distanceFrom(const Vector& vOrigin) const;

	int numPaths() const;

	int numPathsToThisWaypoint() const;
	int getPathToThisWaypoint(int i) const;

	int getPath(int i) const;

	WaypointList::const_iterator begin() const { return m_thePaths.begin(); }
	WaypointList::const_iterator end() const { return m_thePaths.end(); }

	void load(std::fstream& bfp, int iVersion);

	void save(std::fstream& bfp);

	int getFlags() const { return m_iFlags; }

	bool forTeam(int iTeam);

	float getRadius() const { return m_fRadius; }

	void setRadius(const float fRad) { m_fRadius = fRad; }

	Vector applyRadius() const;

	bool isAiming() const;

private:
	Vector m_vOrigin;
	// aim of vector (used with certain waypoint types)
	int m_iAimYaw;
	int m_iFlags;
	int m_iArea;
	float m_fRadius;
	// not deleted
	bool m_bUsed;
	// paths to other waypoints
	WaypointList m_thePaths;
	// for W_FL_WAIT_GROUND waypoints
	float m_fNextCheckGroundTime;
	bool m_bHasGround;
	// Update m_iNumPathsTo (For display)
	bool m_bIsReachable; 
	float m_fCheckReachableTime;
	WaypointList m_PathsTo; // paths to this waypoint from other waypoints

	std::vector<wpt_opens_later_t> m_OpensLaterInfo;
};

class CWaypoints
{
public:
	static constexpr int MAX_WAYPOINTS = 2048;
	static constexpr int WAYPOINT_VERSION = 5; // waypoint version 5 add author information

	static constexpr int W_FILE_FL_VISIBILITY = 1;

	static void init (const char *pszAuthor = nullptr, const char *pszModifiedBy = nullptr);

	static int getWaypointIndex(CWaypoint* pWpt)
	{
		if (pWpt == nullptr)
			return -1;
		/**
		* This should be nuked. RCBot2 should be upgraded to store waypoints on an unordered_map, the index becomes the key.
		* Also just store the waypoint index on the waypoint itself. Valve navmesh doesn't need to look up the area id, each area know it's own ID.
		* Better than dealing with this pointer math crap
		*/

		// return ((int)pWpt - (int)m_theWaypoints)/sizeof(CWaypoint);

		// This will probably not work -caxanga334
		return static_cast<int>((reinterpret_cast<std::intptr_t>(pWpt) - reinterpret_cast<std::intptr_t>(m_theWaypoints)) / sizeof(CWaypoint));
	}

	static void autoFix(bool bAutoFixNonArea);

	static void checkAreas(edict_t* pActivator);

	static void shiftVisibleAreas(edict_t* pPlayer, int from, int to);

	static void drawWaypoints(CClient* pClient);

	static int addWaypoint(CClient *pClient, const char *type1, const char *type2, const char *type3, const char *type4, bool bUseTemplate = false);

	static int addWaypoint(edict_t* pPlayer, const Vector& vOrigin, int iFlags = CWaypointTypes::W_FL_NONE, bool bAutoPath = false, int iYaw = 0, int iArea = 0, float fRadius = 0.0f);

	static void removeWaypoint(int iIndex);

	static int numWaypoints();

	static bool checkReachable(CWaypoint* pWaypoint, int iStart); // TODO: Needs implemented properly [APG]RoboCop[CL]

	static CWaypoint* nearestPipeWaypoint(const Vector& vTarget, const Vector& vOrigin, int* iAiming);

	static int freeWaypointIndex();

	static void deletePathsTo(int iWpt);
	static void deletePathsFrom(int iWpt);

	static void shiftAreas(int val);

	static CWaypoint *getWaypoint (const int iIndex)
	{
		if (!validWaypointIndex(iIndex))
			return nullptr;

		return &m_theWaypoints[iIndex];
	}

	static CWaypoint* getNextCoverPoint(CBot* pBot, CWaypoint* pCurrent, CWaypoint* pBlocking);

	// save waypoints
	static bool save ( bool bVisiblityMade, const edict_t *pPlayer = nullptr, const char *pszAuthor = nullptr, const char *pszModifier = nullptr);
	// load waypoints
	static bool load (const char *szMapName = nullptr);

	static bool validWaypointIndex (const int iIndex)
	{
		return iIndex >= 0 && iIndex < m_iNumWaypoints;
	}

	static void precacheWaypointTexture();

	static int waypointTexture() { return m_iWaypointTexture; }

	static void deleteWaypoint(int iIndex);

	static void freeMemory();

	static int getClosestFlagged(int iFlags, const Vector& vOrigin, int iTeam, float* fReturnDist = nullptr, const unsigned char* failedwpts = nullptr);

	static int nearestWaypointGoal(int iFlags, const Vector& origin, float fDist, int iTeam = 0);
	static CWaypoint* randomRouteWaypoint(const CBot* pBot, const Vector& vOrigin, const Vector& vGoal, int iTeam, int iArea);
	static CWaypoint* randomWaypointGoal(int iFlags, int iTeam = 0, int iArea = 0, bool bForceArea = false, const CBot* pBot = nullptr, bool bHighDanger = false, int iSearchFlags = 0, int iIgnore = -1);
	static CWaypoint* randomWaypointGoalBetweenArea(int iFlags, int iTeam, int iArea, bool bForceArea, const CBot* pBot, bool bHighDanger, const Vector* org1, const Vector* org2, bool bIgnoreBelief = false, int iWpt1 = -1, int iWpt2 = -1);
	static CWaypoint* randomWaypointGoalNearestArea(int iFlags, int iTeam, int iArea, bool bForceArea, const CBot* pBot, bool bHighDanger, const Vector* origin, int iIgnore = -1, bool bIgnoreBelief = false, int iWpt1 = -1);
	static int randomFlaggedWaypoint(int iTeam = 0);

	static CWaypointVisibilityTable* getVisiblity() { return m_pVisibilityTable; }
	static void setupVisibility();
	static CWaypoint* getPinchPointFromWaypoint(const Vector& vPlayerOrigin, const Vector& vPinchOrigin);
	static CWaypoint* getNestWaypoint(int iTeam, int iArea, bool bForceArea = false, CBot* pBot = nullptr);

	static void updateWaypointPairs(std::vector<edict_wpt_pair_t>* pPairs, int iWptFlag, const char* szClassname);
	static const char* getAuthor() { return m_szAuthor; }
	static const char* getModifier() { return m_szModifiedBy; }
	static const char* getWelcomeMessage() { return m_szWelcomeMessage; }

private:
	static CWaypoint m_theWaypoints[MAX_WAYPOINTS];
	static int m_iNumWaypoints;
	static float m_fNextDrawWaypoints;
	static int m_iWaypointTexture;
	static CWaypointVisibilityTable* m_pVisibilityTable;
	static char m_szAuthor[32];
	static char m_szModifiedBy[32];
	static char m_szWelcomeMessage[128];
};

#endif