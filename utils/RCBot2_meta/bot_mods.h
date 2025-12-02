/*
 *    part of https://rcbot2.svn.sourceforge.net/svnroot/rcbot2
 *
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
#ifndef __BOT_MODS_H__
#define __BOT_MODS_H__

#include "bot_const.h"
#include "bot_strings.h"
#include "bot_fortress.h"
#include "bot_dod_bot.h"
#include "bot_waypoint.h"
#include "bot_tf2_points.h"
#include "bot_cvars.h"

#include <cstring>

enum : std::uint8_t
{
	MAX_CAP_POINTS = 32
};

enum : std::uint8_t
{
	DOD_MAPTYPE_UNKNOWN = 0,
	DOD_MAPTYPE_FLAG = 1,
	DOD_MAPTYPE_BOMB = 2
};

enum : std::uint8_t
{
	BOT_ADD_METHOD_DEFAULT = 0,
	BOT_ADD_METHOD_PUPPET = 1
};

constexpr const char* BOT_ADD_PUPPET_COMMAND = "bot";

class CBotNeuralNet;

#include <vector>


/*
		CSS
		TF2
		HL2DM
		HL1DM
		FF
		COOP
		ZOMBIE
*/
typedef enum : std::uint8_t
{
	BOTTYPE_GENERIC = 0,
	BOTTYPE_CSS,
	BOTTYPE_TF2,
	BOTTYPE_HL2DM,
	BOTTYPE_HL1DM,
	BOTTYPE_FF,
	BOTTYPE_COOP,
	BOTTYPE_ZOMBIE,
	BOTTYPE_DOD,
	BOTTYPE_NS2,
	BOTTYPE_SYN,
	BOTTYPE_BMS,
	BOTTYPE_MAX
}eBotType;

class CBotMod
{
public:
	CBotMod() 
	{
		m_szModFolder = nullptr;
		m_szSteamFolder = nullptr;
		m_szWeaponListName = nullptr;
		m_iModId = MOD_UNSUPPORTED;
		m_iBotType = BOTTYPE_GENERIC;
		m_bPlayerHasSpawned = false;
		m_bBotCommand_ResetCheatFlag = false;
	}

	virtual ~CBotMod() = default;
	
	virtual bool checkWaypointForTeam(CWaypoint *pWpt, int iTeam)
	{
		return true; // okay -- no teams!!
	}
	// linux fix
	void setup ( const char *szModFolder, eModId iModId, eBotType iBotType, const char *szWeaponListName );

	bool isModFolder (const char* szModFolder) const;

	char *getModFolder () const;

	virtual const char *getPlayerClass ()
	{
		return "CBasePlayer";
	}

	eModId getModId () const; //TODO: not implemented? [APG]RoboCop[CL]

	virtual bool isAreaOwnedByTeam (const int iArea, int iTeam) { return iArea == 0; }

	eBotType getBotType () const { return m_iBotType; }

	virtual void addWaypointFlags (edict_t *pPlayer, edict_t *pEdict, int *iFlags, int *iArea, float *fMaxDistance)
	{
	}

////////////////////////////////
	virtual void initMod ();

	virtual void mapInit ();

	virtual bool playerSpawned ( edict_t *pPlayer );

	virtual void clientCommand ( edict_t *pEntity, int argc,const char *pcmd, const char *arg1, const char *arg2 ) {}

	virtual void modFrame () { }

	virtual void freeMemory() {}

	virtual bool isWaypointAreaValid ( int iWptArea, int iWptFlags ) { return true; }

	virtual void getTeamOnlyWaypointFlags ( int iTeam, int *iOn, int *iOff )
	{
		*iOn = 0;
		*iOff = 0;
	}

	bool needResetCheatFlag () const
	{
		return m_bBotCommand_ResetCheatFlag;
	}
private:
	char *m_szModFolder;
	char *m_szSteamFolder;
	eModId m_iModId;
	eBotType m_iBotType;
protected:
	char *m_szWeaponListName;
	bool m_bPlayerHasSpawned;
	bool m_bBotCommand_ResetCheatFlag;
};

//TODO: Add Black Mesa Source support [APG]RoboCop[CL]

///////////////////
/*
class CDODFlag
{
public:
	CDODFlag()
	{
		m_pEdict = nullptr;
		m_iId = -1;
	}

	void setup (edict_t *pEdict, int id)
	{
		m_pEdict = pEdict;
		m_iId = id;
	}

	inline bool isFlag ( edict_t *pEdict ) { return m_pEdict == pEdict; }

	void update ();

private:
	edict_t *m_pEdict;
	int m_iId;
};
*/
enum : std::uint8_t
{
	MAX_DOD_FLAGS = 8
};

class CDODFlags
{
public:
	CDODFlags()
	{
		init();
	}

	void init ()
	{
		m_iNumControlPoints = 0;
		m_vCPPositions = nullptr;

		m_iAlliesReqCappers = nullptr;
		m_iAxisReqCappers = nullptr;
		m_iNumAllies = nullptr;
		m_iNumAxis = nullptr;
		m_iOwner = nullptr;
		m_bBombPlanted_Unreliable = nullptr;
		m_iBombsRequired = nullptr;
		m_iBombsRemaining = nullptr;
		m_bBombBeingDefused = nullptr;
		m_iNumAxisBombsOnMap = 0;
		m_iNumAlliesBombsOnMap = 0;
		std::memset(m_bBombPlanted,0,sizeof(bool)*MAX_DOD_FLAGS);
		std::memset(m_pFlags,0,sizeof(edict_t*)*MAX_DOD_FLAGS);
		std::memset(m_pBombs,0,sizeof(edict_t*)*MAX_DOD_FLAGS*8);

		for (int& i : m_iWaypoint)
		{
			i = -1;
		}
	}

	int getNumFlags () const { return m_iNumControlPoints; }
	int getNumFlagsOwned (const int iTeam) const
	{
		int count = 0;

		for (int i = 0; i < m_iNumControlPoints; i++)
		{
			if ( m_iOwner[i] == iTeam )
				count++;
		}

		return count;
	}

	int setup (edict_t *pResourceEntity);

	bool getRandomEnemyControlledFlag (const CBot *pBot, Vector *position, int iTeam, int *id = nullptr) const;
	bool getRandomTeamControlledFlag (const CBot *pBot, Vector *position, int iTeam, int *id = nullptr) const;

	bool getRandomBombToDefuse (Vector& position, int iTeam, edict_t*& pBombTarget, int* id = nullptr) const;
	bool getRandomBombToPlant (CBot* pBot, Vector& position, int iTeam, edict_t*& pBombTarget, int* id = nullptr) const;
	bool getRandomBombToDefend (CBot* pBot, Vector* position, int iTeam, edict_t** pBombTarget, int* id = nullptr) const;

	int findNearestObjective (const Vector& vOrigin) const;

	int getWaypointAtFlag (const int iFlagId) const
	{
		return m_iWaypoint[iFlagId];
	}

	int getNumBombsToDefend (const int iTeam) const
	{
		int count = 0;

		for (int i = 0; i < m_iNumControlPoints; i++)
		{
			if ( canDefendBomb(iTeam,i) )
				count++;
		}

		return count;
	}

	int getNumBombsToDefuse (const int iTeam) const
	{
		int count = 0;

		for (int i = 0; i < m_iNumControlPoints; i++)
		{
			if ( canDefuseBomb(iTeam,i) )
				count++;
		}

		return count;
	}

	int getNumPlantableBombs (const int iTeam) const
	{
		int count = 0;

		for (int i = 0; i < m_iNumControlPoints; i++)
		{
			if ( canPlantBomb(iTeam,i) )
				count += getNumBombsRequired(i);
		}

		return count;
	}

	bool isBombExplodeImminent(const int id) const
	{
		return engine->Time() - m_fBombPlantedTime[id] > DOD_BOMB_EXPLODE_IMMINENT_TIME;
	}

	void setBombPlanted (const int id, const bool val)
	{
		m_bBombPlanted[id] = val;

		if ( val )
			m_fBombPlantedTime[id] = engine->Time();
		else
			m_fBombPlantedTime[id] = 0;
	}

	int getNumBombsToPlant (const int iTeam) const
	{
		int count = 0;

		for (int i = 0; i < m_iNumControlPoints; i++)
		{
			if ( canPlantBomb(iTeam,i) )
				count += getNumBombsRemaining(i);
		}

		return count;
	}

	bool ownsFlag (const edict_t *pFlag, const int iTeam) const { return ownsFlag(getFlagID(pFlag),iTeam); }

	bool ownsFlag (const int iFlag, const int iTeam) const
	{
		if ( iFlag == -1 )
			return false;

		return m_iOwner[iFlag] == iTeam;
	}

	int numFlagsOwned (const int iTeam) const
	{
		int count = 0;

		for (int i = 0; i < m_iNumControlPoints; i++)
		{
			if ( m_iOwner[i] == iTeam )
				count++;
		}

		return count;
	}

	int numCappersRequired (const edict_t *pFlag, const int iTeam) const { return numCappersRequired(getFlagID(pFlag),iTeam); }

	int numCappersRequired (const int iFlag, const int iTeam) const
	{
		if ( iFlag == -1 )
			return 0;

		return iTeam == TEAM_ALLIES ? m_iAlliesReqCappers[iFlag] : m_iAxisReqCappers[iFlag];
	}

	bool isBombPlanted (const int iId) const
	{
		if ( iId == -1 )
			return false;

		return m_bBombPlanted[iId];
	}

	bool isBombPlanted (const edict_t *pBomb) const
	{
		return isBombPlanted(getBombID(pBomb));
	}

	bool canDefendBomb (const int iTeam, const int iId) const
	{
		return m_pBombs[iId][0]!= nullptr &&m_iOwner[iId]!=iTeam && isBombPlanted(iId);
	}

	bool canDefuseBomb (const int iTeam, const int iId) const
	{
		return m_pBombs[iId][0]!= nullptr &&m_iOwner[iId]==iTeam && isBombPlanted(iId);
	}

	bool canPlantBomb (const int iTeam, const int iId) const
	{
		return m_pBombs[iId][0]!= nullptr &&m_iOwner[iId]!=iTeam && !isBombPlanted(iId);
	}

	bool isTeamMateDefusing (const edict_t *pIgnore, int iTeam, int id ) const;
	bool isTeamMatePlanting (const edict_t *pIgnore, int iTeam, int id ) const;

	static bool isTeamMateDefusing (const edict_t *pIgnore, int iTeam, const Vector& vOrigin );
	static bool isTeamMatePlanting (const edict_t *pIgnore, int iTeam, const Vector& vOrigin );

	int getNumBombsRequired (const int iId) const
	{
		if ( iId == -1 )
			return false;

		return m_iBombsRequired[iId];
	}

	int getNumBombsRequired (const edict_t *pBomb) const
	{
		return getNumBombsRequired(getBombID(pBomb));
	}

	int getNumBombsRemaining (const int iId) const
	{
		if ( iId == -1 )
			return false;

		return m_iBombsRemaining[iId];
	}

	int getNumBombsRemaining (const edict_t *pBomb) const
	{
		return getNumBombsRemaining(getBombID(pBomb));
	}

	bool isBombBeingDefused (const int iId) const
	{
		if ( iId == -1 )
			return false;

		return m_bBombBeingDefused[iId];
	}

	bool isBombBeingDefused (const edict_t *pBomb) const
	{
		return isBombBeingDefused(getBombID(pBomb));
	}

	int numEnemiesAtCap (const edict_t *pFlag, const int iTeam) const { return numEnemiesAtCap(getFlagID(pFlag),iTeam); }

	int numFriendliesAtCap (const edict_t *pFlag, const int iTeam) const { return numFriendliesAtCap(getFlagID(pFlag),iTeam); }

	int numFriendliesAtCap (const int iFlag, const int iTeam) const
	{
		if ( iFlag == -1 )
			return 0;

		return iTeam == TEAM_ALLIES ? m_iNumAllies[iFlag] : m_iNumAxis[iFlag];
	}

	int numEnemiesAtCap (const int iFlag, const int iTeam) const
	{
		if ( iFlag == -1 )
			return 0;

		return iTeam == TEAM_ALLIES ? m_iNumAxis[iFlag] : m_iNumAllies[iFlag];
	}

	edict_t *getFlagByID (const int id) const
	{
		if ( id >= 0 && id < m_iNumControlPoints )
			return m_pFlags[id];

		return nullptr;
	}

	int getFlagID (const edict_t *pent) const
	{
		for (int i = 0; i < m_iNumControlPoints; i++)
		{
			if ( m_pFlags[i] == pent )
				return i;
		}

		return -1;
	}

	int getBombID (const edict_t *pent) const
	{
		if ( pent == nullptr)
			return -1;

		for (int i = 0; i < m_iNumControlPoints; i++)
		{
			if ( m_pBombs[i][0] == pent || m_pBombs[i][1] == pent )
				return i;
		}

		return -1;
	}

	bool isFlag (const edict_t *pent) const
	{
		return getFlagID(pent) != -1;
	}

	bool isBomb (const edict_t *pent) const
	{
		return getBombID(pent) != -1;
	}

	int getNumBombsOnMap (const int iTeam) const
	{
		if ( iTeam == TEAM_ALLIES )
			return m_iNumAlliesBombsOnMap;
		return m_iNumAxisBombsOnMap;
	}

	void reset ()
	{
		// time up
		m_iNumControlPoints = 0;
	}

private:
	edict_t *m_pFlags[MAX_DOD_FLAGS];
	edict_t* m_pBombs[MAX_DOD_FLAGS][8]; // maximum of 8 bombs per capture point increased for stability [APG]RoboCop[CL]
	int m_iWaypoint[MAX_DOD_FLAGS];

	int m_iNumControlPoints;
	Vector *m_vCPPositions;

	int *m_iAlliesReqCappers;
	int *m_iAxisReqCappers;
	int *m_iNumAllies;
	int *m_iNumAxis;
	int *m_iOwner;

	// reply on this one
	bool *m_bBombPlanted_Unreliable;
	bool m_bBombPlanted[MAX_DOD_FLAGS];
	float m_fBombPlantedTime[MAX_DOD_FLAGS];
	int *m_iBombsRequired;
	int *m_iBombsRemaining;
	bool *m_bBombBeingDefused;
	int m_iNumAlliesBombsOnMap;
	int m_iNumAxisBombsOnMap;
};

class CDODMod : public CBotMod
{
public:
	CDODMod()
	{
		setup("dod",MOD_DOD,BOTTYPE_DOD,"DOD");
	}

	static void roundStart ();

	bool checkWaypointForTeam(CWaypoint *pWpt, int iTeam) override;
	
	static int numClassOnTeam( int iTeam, int iClass );

	static int getScore(edict_t *pPlayer);

	static int getHighestScore ();

	void clientCommand ( edict_t *pEntity, int argc, const char *pcmd, const char *arg1, const char *arg2 ) override;

	static float getMapStartTime ();

	static bool isBombMap () { return (m_iMapType & DOD_MAPTYPE_BOMB) == DOD_MAPTYPE_BOMB; }
	static bool isFlagMap () { return (m_iMapType & DOD_MAPTYPE_FLAG) == DOD_MAPTYPE_FLAG; }
	static bool mapHasBombs () { return (m_iMapType & DOD_MAPTYPE_BOMB) == DOD_MAPTYPE_BOMB; }

	static bool isCommunalBombPoint () { return m_bCommunalBombPoint; }
	static int getBombPointArea (const int iTeam) { if ( iTeam == TEAM_ALLIES ) return m_iBombAreaAllies; return m_iBombAreaAxis; } 

	void addWaypointFlags (edict_t *pPlayer, edict_t *pEdict, int *iFlags, int *iArea, float *fMaxDistance ) override;

	static CDODFlags m_Flags;

	static bool shouldAttack ( int iTeam ); // uses the neural net to return probability of attack

	static edict_t *getBombTarget (const CWaypoint *pWpt );
	static edict_t *getBreakable (const CWaypoint *pWpt );

	void getTeamOnlyWaypointFlags ( int iTeam, int *iOn, int *iOff ) override;

	static bool isBreakableRegistered ( edict_t *pBreakable, int iTeam );

	static CWaypoint *getBombWaypoint ( edict_t *pBomb )
	{
		for (edict_wpt_pair_t& m_BombWaypoint : m_BombWaypoints)
		{
			if (m_BombWaypoint.pEdict == pBomb )
				return m_BombWaypoint.pWaypoint;
		}

		return nullptr;
	}

	static bool isPathBomb ( edict_t *pBomb )
	{
		for (edict_wpt_pair_t& m_BombWaypoint : m_BombWaypoints)
		{
			if (m_BombWaypoint.pEdict == pBomb )
				return true;
		}

		return false;
	}

	// for getting the ground of bomb to open waypoints
	// the ground might change
	static Vector getGround ( CWaypoint *pWaypoint );

	//to do for snipers and machine gunners
	/*static unsigned short int getNumberOfClassOnTeam ( int iClass );
	static unsigned short int getNumberOfPlayersOnTeam ( int iClass );*/

protected:

	void initMod () override;

	void mapInit () override;

	void modFrame () override;

	void freeMemory () override;

	static edict_t *m_pResourceEntity;
	static edict_t *m_pPlayerResourceEntity;
	static edict_t *m_pGameRules;
	static float m_fMapStartTime;
	static int m_iMapType;
	static bool m_bCommunalBombPoint; // only one bomb suuply point for both teams
	static int m_iBombAreaAllies;
	static int m_iBombAreaAxis;

	static std::vector<edict_wpt_pair_t> m_BombWaypoints;
	static std::vector<edict_wpt_pair_t> m_BreakableWaypoints;

									// enemy			// team
	static float fAttackProbLookUp[MAX_DOD_FLAGS+1][MAX_DOD_FLAGS+1];
};

typedef enum : std::uint8_t
{
	CS_MAP_DEATHMATCH = 0, // Generic Maps
	CS_MAP_BOMBDEFUSAL, // Bomb Defusal maps
	CS_MAP_HOSTAGERESCUE, // Hostage Rescue maps
	CS_MAP_MAX
}eCSSMapType;

class CCounterStrikeSourceMod : public CBotMod
{
public:
	CCounterStrikeSourceMod()
	{
		setup("cstrike", MOD_CSS, BOTTYPE_CSS, "CSS");
	}

	const char *getPlayerClass () override
	{
		return "CCSPlayer";
	}

	void initMod() override;
	void mapInit() override;
	bool checkWaypointForTeam(CWaypoint *pWpt, int iTeam) override;
	static void onRoundStart();
	static void onFreezeTimeEnd();
	static void onBombPlanted();
	static bool isMapType(const eCSSMapType MapType) { return MapType == m_MapType; }
	static bool isBombCarrier(const CBot *pBot);

	static float getRemainingRoundTime()
	{
		return m_fRoundStartTime + mp_roundtime.GetFloat() * 60.0f - engine->Time();
	}

	static float getRemainingBombTime()
	{
		return m_fBombPlantedTime + mp_c4timer.GetFloat() - engine->Time();
	}

	static bool isBombPlanted()
	{
		return m_bIsBombPlanted;
	}

	static edict_t *getBomb()
	{
		return engine->PEntityOfEntIndex(m_hBomb.GetEntryIndex());
	}
	static bool isBombDropped();
	static bool isBombDefused();

	static bool wasBombFound()
	{
		return m_bBombWasFound;
	}

	static void setBombFound(const bool set)
	{
		m_bBombWasFound = set;
	}
	static bool canHearPlantedBomb(const CBot *pBot);
	static bool isScoped(const CBot *pBot);
	static void updateHostages();
	static edict_t *getRandomHostage();
	static bool canRescueHostages();

	static std::vector<CBaseHandle> getHostageVector()
	{
		return m_hHostages;
	}
	//void entitySpawn ( edict_t *pEntity );
private:
	static eCSSMapType m_MapType; // Map Type
	static float m_fRoundStartTime; // The time when the round started
	static float m_fBombPlantedTime; // The time when the bomb was planted
	static bool m_bIsBombPlanted; // Is the bomb planted?
	static bool m_bBombWasFound; // Did the CTs locate the bomb?
	static CBaseHandle m_hBomb; // The bomb. Experimental CBaseHandle instead of MyEHandle
	static std::vector<CBaseHandle> m_hHostages; // Vector with hostage handles
};

class CTimCoopMod : public CBotMod
{
public:
	CTimCoopMod()
	{
		setup("SourceMods",MOD_TIMCOOP,BOTTYPE_COOP,"HL2DM");
	}

	//void initMod ();

	//void mapInit ();

	//void entitySpawn ( edict_t *pEntity );
};

class CSvenCoop2Mod : public CBotMod
{
public:
	CSvenCoop2Mod()
	{
		setup("SourceMods",MOD_SVENCOOP2,BOTTYPE_COOP,"SVENCOOP2");
	}

	//void initMod ();

	//void mapInit ();

	//void entitySpawn ( edict_t *pEntity );
};

class CFortressForeverMod : public CBotMod
{
public:
	CFortressForeverMod()
	{
		setup("FortressForever", MOD_FF, BOTTYPE_FF, "FF");
	}
};

class CHLDMSourceMod : public CBotMod
{
public:
	CHLDMSourceMod()
	{
		setup("hl1mp",MOD_HL1DMSRC,BOTTYPE_HL1DM,"HLDMSRC");
	}
};

class CSynergyMod : public CBotMod
{
public:
	CSynergyMod()
	{
		setup("synergy",MOD_SYNERGY,BOTTYPE_SYN,"SYNERGY");
	}

	void initMod () override;
	void mapInit () override;

	const char *getPlayerClass () override
	{
		return "CSynergyPlayer";
	}

	//void entitySpawn ( edict_t *pEntity );

	static bool IsEntityLocked(edict_t *pEntity);
	static bool IsCombineMinePlayerPlaced(edict_t *pMine);
	static bool IsCombineMineDisarmed(edict_t *pMine);
	static bool IsCombineMineArmed(edict_t *pMine);
	static bool IsCombineMineHeldByPhysgun(edict_t *pMine);
};

#define NEWENUM typedef enum {

typedef enum : std::uint8_t
{
	TF_MAP_DM = 0,
	TF_MAP_CTF,
	TF_MAP_CP,
	TF_MAP_TC,
	TF_MAP_CART,
	TF_MAP_CARTRACE,
	TF_MAP_ARENA,
	TF_MAP_SAXTON,
	TF_MAP_PIPEBALL,
	TF_MAP_KOTH,
	TF_MAP_SD, // special delivery : added 15 jul 12
	TF_MAP_TR,
	TF_MAP_MVM,
	TF_MAP_RD,
	TF_MAP_BUMPERCARS,
	TF_MAP_PD, // Player Destruction
	TF_MAP_ZI, // Scream Fortress XV (Oct 9th, 2023) update for Zombie Infection maps - [APG]RoboCop[CL]
	TF_MAP_PASS, //TODO: add support for those gamemodes [APG]RoboCop[CL]
	TF_MAP_CPPL, // CP+PL Hybrid maps - RussiaTails
	TF_MAP_GG, // GunGame maps - RussiaTails
	TF_MAP_MAX
}eTFMapType;

// These must be MyEHandles because they may be destroyed at any time
typedef struct
{
	MyEHandle entrance;
	MyEHandle exit;
	MyEHandle sapper;
	float m_fLastTeleported;
	int m_iWaypoint;
//	short builder;
}tf_tele_t;

typedef struct
{
	MyEHandle sentry;
	MyEHandle sapper;
//	short builder;
}tf_sentry_t;

typedef struct
{
	MyEHandle disp;
	MyEHandle sapper;
//	short builder;
}tf_disp_t;


class CTeamControlPointRound;
class CTeamControlPointMaster;
class CTeamControlPoint;
class CTeamRoundTimer;

class CTeamFortress2Mod : public CBotMod
{
public:
	CTeamFortress2Mod()
	{
		setup("tf",MOD_TF2,BOTTYPE_TF2,"TF2");

		m_pResourceEntity = nullptr;
	}

	void mapInit () override;

	void modFrame () override;

	bool isAreaOwnedByTeam (int iArea, int iTeam) override;

	static void updatePointMaster ();

	void clientCommand ( edict_t *pEntity, int argc, const char *pcmd, const char *arg1, const char *arg2 ) override;

	const char *getPlayerClass () override
	{
		return "CTFPlayer";
	}

	void initMod () override;

	static void roundStart (); // TODO: Needs implemented properly [APG]RoboCop[CL]

	static int getTeam ( edict_t *pEntity );

	static TF_Class getSpyDisguise ( edict_t *pPlayer ); //TODO: not implemented? [APG]RoboCop[CL]

	static int getSentryLevel ( edict_t *pSentry );
	static int getDispenserLevel ( edict_t *pDispenser );

	static bool isDispenser ( edict_t *pEntity, int iTeam, bool checkcarrying = false );

	static bool isPayloadBomb ( edict_t *pEntity, int iTeam );

	static int getTeleporterWaypoint (const edict_t *pTele );

	bool isWaypointAreaValid ( int iWptArea, int iWptFlags ) override;

	static bool isSuddenDeath();

	static bool isHealthKit (const edict_t* pEntity);

	static bool isAmmo (const edict_t* pEntity);

	static int getArea (); // get current area of map // TODO: Needs implemented properly? [APG]RoboCop[CL]

	static void setArea (const int area) { m_iArea = area; }

	static bool isSentry ( edict_t *pEntity, int iTeam, bool checkcarrying = false );
	static bool isTankBoss(const edict_t* pEntity);
	static void checkMVMTankBoss(edict_t *pEntity);
	static bool isTeleporter ( edict_t *pEntity, int iTeam, bool checkcarrying = false );

	static void updateTeleportTime (const edict_t* pOwner);
	static float getTeleportTime (const edict_t* pOwner);

	static bool isTeleporterEntrance ( edict_t *pEntity, int iTeam, bool checkcarrying = false );

	static bool isTeleporterExit ( edict_t *pEntity, int iTeam, bool checkcarrying = false );

	static bool isMapType (const eTFMapType iMapType) { return iMapType == m_MapType; }

	static bool isFlag ( edict_t *pEntity, int iTeam );

	static bool withinEndOfRound ( float fTime );

	static bool isPipeBomb ( edict_t *pEntity, int iTeam);

	static bool isHurtfulPipeGrenade ( edict_t *pEntity, edict_t *pPlayer, bool bCheckOwner = true );

	static bool isRocket ( edict_t *pEntity, int iTeam );

	static int getEnemyTeam ( int iTeam );

	static bool buildingNearby ( int iTeam, const Vector& vOrigin );

// Naris @ AlliedModders .net

	static bool TF2_IsPlayerZoomed(edict_t *pPlayer);

	static bool TF2_IsPlayerSlowed(edict_t *pPlayer);

	static bool TF2_IsPlayerDisguised(edict_t *pPlayer);

	static bool TF2_IsPlayerCloaked(edict_t *pPlayer);

	static bool TF2_IsPlayerInvuln(edict_t *pPlayer);

	static bool TF2_IsPlayerKrits(edict_t *pPlayer);

	static bool TF2_IsPlayerOnFire(edict_t *pPlayer);

	static bool TF2_IsPlayerTaunting(edict_t *pPlayer);

	static float TF2_GetPlayerSpeed(edict_t *pPlayer, TF_Class iClass );  //TODO: not implemented? [APG]RoboCop[CL]

	static void teleporterBuilt ( edict_t *pOwner, eEngiBuild type, edict_t *pBuilding );  //TODO: not implemented? [APG]RoboCop[CL]

	static edict_t *getTeleporterExit (const edict_t *pTele );

	static void setPointOpenTime ( int time );

	static void setSetupTime ( int time );

	static void resetSetupTime ();

	static bool isArenaPointOpen ();

	static bool hasRoundStarted ();

	static int getHighestScore ();

	static edict_t *nearestDispenser (const Vector& vOrigin, int team );

	static void flagPickedUp (int iTeam, edict_t *pPlayer);
	static void flagReturned (int iTeam);

	static void setAttackDefendMap (const bool bSet) { m_bAttackDefendMap = bSet; }
	static bool isAttackDefendMap () { return m_bAttackDefendMap; }

	void addWaypointFlags (edict_t *pPlayer, edict_t *pEdict, int *iFlags, int *iArea, float *fMaxDistance ) override;

	void getTeamOnlyWaypointFlags ( int iTeam, int *iOn, int *iOff ) override;

	static bool getFlagLocation ( int iTeam, Vector *vec );

	static bool getDroppedFlagLocation (const int iTeam, Vector *vec)
	{
		if ( iTeam == TF2_TEAM_BLUE )
		{
			*vec = m_vFlagLocationBlue;
			return m_bFlagLocationValidBlue;
		}
		if ( iTeam == TF2_TEAM_RED )
		{
			*vec = m_vFlagLocationRed;
			return m_bFlagLocationValidRed;
		}

		return false;
	}

	static void flagDropped (const int iTeam, const Vector& vLoc)
	{
		if ( iTeam == TF2_TEAM_BLUE )
		{
			m_pFlagCarrierBlue = nullptr;
			m_vFlagLocationBlue = vLoc;
			m_bFlagLocationValidBlue = true;
		}
		else if ( iTeam == TF2_TEAM_RED )
		{
			m_pFlagCarrierRed = nullptr;
			m_vFlagLocationRed = vLoc;
			m_bFlagLocationValidRed = true;
		}

		m_iFlagCarrierTeam = iTeam;
	}

	static void roundStarted ()
	{
		m_bHasRoundStarted = true;
		m_bRoundOver = false;
		m_iWinningTeam = 0; 
	}

	static void roundWon (const int iWinningTeam)
	{
		m_bHasRoundStarted = false;
		m_bRoundOver = true;
		m_iWinningTeam = iWinningTeam;
		m_iLastWinningTeam = m_iWinningTeam;
	}

	static bool wonLastRound(const int iTeam)
	{
		return m_iLastWinningTeam == iTeam;
	}

	static bool isLosingTeam (const int iTeam)
	{
		return !m_bHasRoundStarted && m_bRoundOver && m_iWinningTeam && m_iWinningTeam != iTeam; 
	}

	static void roundReset ();

	static bool isFlagCarrier (edict_t *pPlayer)
	{
		return m_pFlagCarrierBlue==pPlayer||m_pFlagCarrierRed==pPlayer;
	}

	static edict_t *getFlagCarrier (const int iTeam)
	{
		if ( iTeam == TF2_TEAM_BLUE )
			return m_pFlagCarrierBlue;
		if ( iTeam == TF2_TEAM_RED )
			return m_pFlagCarrierRed;

		return nullptr;
	}

	static bool isFlagCarried (const int iTeam)
	{
		if ( iTeam == TF2_TEAM_BLUE )
			return m_pFlagCarrierBlue != nullptr;
		if ( iTeam == TF2_TEAM_RED )
			return m_pFlagCarrierRed != nullptr;

		return false;
	}

	static void sapperPlaced(const edict_t* pOwner, eEngiBuild type, edict_t* pSapper);  //TODO: all 4 lines not implemented? [APG]RoboCop[CL]
	static void sapperDestroyed(edict_t *pOwner,eEngiBuild type, const edict_t *pSapper);
	static void sentryBuilt(const edict_t* pOwner, eEngiBuild type, edict_t* pBuilding);
	static void dispenserBuilt(const edict_t* pOwner, eEngiBuild type, edict_t* pBuilding);

	static CWaypoint *getBestWaypointMVM (const CBot *pBot, int iFlags );

	static edict_t *getMySentryGun (const edict_t* pOwner)
	{
		const int id = ENTINDEX(pOwner)-1;

		if ( id>=0 )
		{
			return m_SentryGuns[id].sentry.get();
		}

		return nullptr;
	}

	static edict_t *getSentryOwner (const edict_t *pSentry)
	{
		//for ( short int i = 1; i <= gpGlobals->maxClients; i ++ )
		for ( short int i = 0; i < RCBOT_MAXPLAYERS; i ++ )
		{			
			if ( m_SentryGuns[i].sentry.get() == pSentry )
				return INDEXENT(i+1);
		}

		return nullptr;
	}

	static bool isMySentrySapped (const edict_t* pOwner) 
	{
		const int id = ENTINDEX(pOwner)-1;

		if ( id>=0 )
		{
			return m_SentryGuns[id].sentry.get()!= nullptr &&m_SentryGuns[id].sapper.get()!= nullptr;
		}

		return false;
	}

	static edict_t *getSentryGun (const int id)
	{
		return m_SentryGuns[id].sentry.get();
	}

	static edict_t *getTeleEntrance (const int id)
	{
		return m_Teleporters[id].entrance.get();
	}

	static bool isMyTeleporterSapped (const edict_t* pOwner)
	{
		const int id = ENTINDEX(pOwner)-1;

		if ( id>=0 )
		{
			return (m_Teleporters[id].exit.get()!= nullptr ||m_Teleporters[id].entrance.get()!= nullptr)&&m_Teleporters[id].sapper.get()!= nullptr;
		}

		return false;
	}

	static bool isMyDispenserSapped (const edict_t* pOwner)
	{
		const int id = ENTINDEX(pOwner)-1;

		if ( id>=0 )
		{
			return m_Dispensers[id].disp.get()!= nullptr &&m_Dispensers[id].sapper.get()!= nullptr;
		}

		return false;
	}

	static bool isSentrySapped (const edict_t *pSentry)
	{
		for (tf_sentry_t& m_SentryGun : m_SentryGuns)
		{
			if (m_SentryGun.sentry.get() == pSentry )
				return m_SentryGun.sapper.get()!= nullptr;
		}

		return false;
	}

	static bool isTeleporterSapped (const edict_t *pTele)
	{
		for (tf_tele_t& m_Teleporter : m_Teleporters)
		{
			if (m_Teleporter.entrance.get() == pTele || m_Teleporter.exit.get() == pTele )
				return m_Teleporter.sapper.get()!= nullptr;
		}

		return false;
	}

	static bool isDispenserSapped (const edict_t *pDisp)
	{
		for (tf_disp_t& m_Dispenser : m_Dispensers)
		{
			if (m_Dispenser.disp.get() == pDisp )
				return m_Dispenser.sapper.get()!= nullptr;
		}

		return false;
	}

	static edict_t *findResourceEntity ();

	static void addCapDefender (const edict_t* pPlayer, const int iCapIndex)
	{
		m_iCapDefenders[iCapIndex] |= 1 << (ENTINDEX(pPlayer) - 1);
	}

	static void removeCapDefender (const edict_t* pPlayer, const int iCapIndex)
	{
		m_iCapDefenders[iCapIndex] &= ~(1 << (ENTINDEX(pPlayer) - 1));
	}

	static void resetDefenders ()
	{
		std::memset(m_iCapDefenders,0,sizeof(int)*MAX_CONTROL_POINTS);
	}

	static bool isDefending ( edict_t *pPlayer );//, int iCapIndex = -1 );

	static bool isCapping ( edict_t *pPlayer );//, int iCapIndex = -1 );
	
	static void addCapper (const int cp, const int capper)
	{
		if (capper > 0 && cp >= 0 && cp < MAX_CAP_POINTS)
			m_Cappers[cp] |= 1 << (capper - 1);
	}

	static void removeCappers (const int cp)
	{
		m_Cappers[cp] = 0;
	}

	static void resetCappers ()
	{
		std::memset(m_Cappers,0,sizeof(int)*MAX_CONTROL_POINTS);
	}

	static int numPlayersOnTeam ( int iTeam, bool bAliveOnly = false ); //TODO: Experimental [APG]RoboCop[CL]
	static int numClassOnTeam ( int iTeam, int iClass );

	static int getFlagCarrierTeam () { return m_iFlagCarrierTeam; }
	static bool canTeamPickupFlag_SD(int iTeam,bool bGetUnknown);

	static edict_t *getBuildingOwner (eEngiBuild object, short index); //TODO: not implemented? [APG]RoboCop[CL]
	static edict_t *getBuilding (eEngiBuild object, const edict_t* pOwner); //TODO: not implemented? [APG]RoboCop[CL]

	static bool isBoss ( edict_t *pEntity, float *fFactor = nullptr);

	static void initBoss (const bool bSummoned) { m_bBossSummoned = bSummoned; m_pBoss = nullptr; }

	static bool isBossSummoned () { return m_bBossSummoned; }

	static bool isSentryGun ( edict_t *pEdict );

	static edict_t *getMediGun ( edict_t *pPlayer );

	static void findMediGun ( edict_t *pPlayer );


	bool checkWaypointForTeam(CWaypoint *pWpt, int iTeam) override;
	

	static bool isFlagAtDefaultState () { return bFlagStateDefault; }
	static void resetFlagStateToDefault() { bFlagStateDefault = true; }
	static void setDontClearPoints (const bool bClear) { m_bDontClearPoints = bClear; }
	static bool dontClearPoints () { return m_bDontClearPoints; }
	static CTFObjectiveResource m_ObjectiveResource;

	static CTeamControlPointRound *getCurrentRound() { return m_pCurrentRound; }

	static CTeamControlPointMaster *getPointMaster () { return m_PointMaster;}

	static void updateRedPayloadBomb ( edict_t *pent );
	static void updateBluePayloadBomb ( edict_t *pent ); 

	static edict_t *getPayloadBomb ( int team );

	static void MVMAlarmSounded () { m_bMVMAlarmSounded = true; }
	static void MVMAlarmReset () { m_bMVMAlarmSounded = false; }
	static float getMVMCapturePointRadius ( )
	{
		return m_fMVMCapturePointRadius;
	}
	static bool getMVMCapturePoint ( Vector *vec )
	{
		if ( m_bMVMCapturePointValid )
		{
			*vec = m_vMVMCapturePoint;
			return true;
		}

		return getFlagLocation(TF2_TEAM_BLUE,vec);
	}

	static bool isMedievalMode();

private:


	static float TF2_GetClassSpeed(int iClass);

	static CTeamControlPointMaster *m_PointMaster;
	static CTeamControlPointRound *m_pCurrentRound;
	static MyEHandle m_PointMasterResource;
	static CTeamRoundTimer m_Timer;

	static eTFMapType m_MapType;	

	static MyEHandle m_pPayLoadBombRed;
	static MyEHandle m_pPayLoadBombBlue;

	static tf_tele_t m_Teleporters[RCBOT_MAXPLAYERS];	// used to let bots know who made a teleport ans where it goes
	static tf_sentry_t m_SentryGuns[RCBOT_MAXPLAYERS];	// used to let bots know if sentries have been sapped or not
	static tf_disp_t  m_Dispensers[RCBOT_MAXPLAYERS];	// used to let bots know where friendly/enemy dispensers are

	static int m_iArea;

	static float m_fSetupTime;

	static float m_fRoundTime;

	static MyEHandle m_pFlagCarrierRed;
	static MyEHandle m_pFlagCarrierBlue;

	static float m_fPointTime;
	static float m_fArenaPointOpenTime;

	static MyEHandle m_pResourceEntity;
	static MyEHandle m_pGameRules;
	static bool m_bAttackDefendMap;

	static int m_Cappers[MAX_CONTROL_POINTS];
	static int m_iCapDefenders[MAX_CONTROL_POINTS];

	static bool m_bHasRoundStarted;

	static int m_iFlagCarrierTeam;
	static MyEHandle m_pBoss;
	static bool m_bBossSummoned;
	static bool bFlagStateDefault;

	static MyEHandle pMediGuns[RCBOT_MAXPLAYERS];
	static bool m_bDontClearPoints;

	static bool m_bRoundOver;
	static int m_iWinningTeam;
	static int m_iLastWinningTeam;
	static Vector m_vFlagLocationBlue;
	static bool m_bFlagLocationValidBlue;
	static Vector m_vFlagLocationRed;
	static bool m_bFlagLocationValidRed;


	static bool m_bMVMFlagStartValid;
	static Vector m_vMVMFlagStart;
	static bool m_bMVMCapturePointValid;
	static Vector m_vMVMCapturePoint;
	static bool m_bMVMAlarmSounded;
	static float m_fMVMCapturePointRadius;
	static int m_iCapturePointWptID;
	static int m_iFlagPointWptID;

	static MyEHandle m_pNearestTankBoss;
	static float m_fNearestTankDistance;
	static Vector m_vNearestTankLocation;

};

class CHalfLifeDeathmatchMod : public CBotMod
{
public:
	CHalfLifeDeathmatchMod()
	{
		setup("hl2mp", MOD_HLDM2, BOTTYPE_HL2DM, "HL2DM");
	}

	void initMod () override;

	void mapInit () override;

	bool playerSpawned ( edict_t *pPlayer ) override;

	static edict_t *getButtonAtWaypoint (const CWaypoint *pWaypoint)
	{
		for (edict_wpt_pair_t& m_LiftWaypoint : m_LiftWaypoints)
		{
			if (m_LiftWaypoint.pWaypoint == pWaypoint )
				return m_LiftWaypoint.pEdict;
		}

		return nullptr;
	}

	//void entitySpawn ( edict_t *pEntity );
private:
	static std::vector<edict_wpt_pair_t> m_LiftWaypoints;
};

/*
class CNaturalSelection2Mod : public CBotMod
{
public:
	CNaturalSelection2Mod() 
	{
		setup("ns2",MOD_NS2,BOTTYPE_NS2);
	}
// linux fix

	virtual const char *getPlayerClass ()
	{
		return "CBaseNS2Player";
	}

	virtual bool isAreaOwnedByTeam (int iArea, int iTeam) { return (iArea == 0); }

	virtual void addWaypointFlags (edict_t *pPlayer, edict_t *pEdict, int *iFlags, int *iArea, float *fMaxDistance ){ return; }

////////////////////////////////
	virtual void initMod ();

	virtual void mapInit ();

	virtual bool playerSpawned ( edict_t *pPlayer );

	virtual void clientCommand ( edict_t *pEntity, int argc,const char *pcmd, const char *arg1, const char *arg2 ) {};

	virtual void modFrame () { };

	virtual void freeMemory() {};

	virtual bool isWaypointAreaValid ( int iWptArea, int iWptFlags ) { return true; }

	virtual void getTeamOnlyWaypointFlags ( int iTeam, int *iOn, int *iOff )
	{
		*iOn = 0;
		*iOff = 0;
	}
};
*/

class CBotMods
{
public:

	static void parseFile ();

	static void readMods();

	static void freeMemory ();

	static CBotMod *getMod ( char *szModFolder );

private:
	static std::vector<CBotMod*> m_Mods;
};

#endif
