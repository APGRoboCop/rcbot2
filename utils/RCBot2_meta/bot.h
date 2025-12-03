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

//=============================================================================//
//
// HPB_bot2.h - bot header file (Copyright 2004, Jeffrey "botman" Broome)
//
//=============================================================================//

#ifndef __RCBOT2_H__
#define __RCBOT2_H__

#include <cstdint>
//#include "cbase.h"
//#include "baseentity.h"
#include "toolframework/itoolentity.h"
#include "filesystem.h"
#include "interface.h"
#include "engine/iserverplugin.h"
#include "iplayerinfo.h"
#ifdef __linux__
#include "shake.h"    //bir3yk
#endif
#include "IEngineTrace.h" // for traceline functions
#ifndef SM_EXT
#include "IEffects.h"
#endif
#include "vplane.h"
#include "eiface.h"
#ifdef __linux__
#include "shareddefs.h" //bir3yk
#endif
#include "igameevents.h"
#include "Color.h"
class CBasePlayer; // forward declaration required by imovehelper.h (included by usercmd.h) -caxanga334
#include "usercmd.h"

#include "bot_utility.h"
#include "bot_const.h"
#include "bot_ehandle.h"

#include <queue>
#include <bitset>
#include <limits>

#if defined(_WIN64) || defined(_WIN32) && !defined snprintf
#define snprintf _snprintf
#endif

#define MAX_AMMO_TYPES 32
#define MAX_VOICE_CMDS 32
#define MIN_WPT_TOUCH_DIST 16.0f

// Interfaces from the engine
extern IVEngineServer *engine;  // helper functions (messaging clients, loading content, making entities, running commands, etc)
extern IFileSystem *filesystem;  // file I/O 
extern IGameEventManager2 *gameeventmanager;
extern IPlayerInfoManager *playerinfomanager;  // game dll interface to interact with players
extern IServerPluginHelpers *helpers;  // special 3rd party plugin helpers from the engine
extern IServerGameClients* gameclients;
extern IEngineTrace *enginetrace;
#ifndef SM_EXT
extern IEffects *g_pEffects; //Redundant? [APG]RoboCopCL]
#endif
extern IBotManager *g_pBotManager;
extern CGlobalVars *gpGlobals;

enum : std::uint8_t
{
	GET_HEALTH = 0,
	GET_TEAM = 1,
	GET_AMMO = 2,
	T_OFFSETMAX = 3
};

// use a fixed bitset for all the conditions in bot_const.h
using ConditionBitSet = std::bitset<NUM_CONDITIONS>;

class CBotSquad;

// static function
bool BotFunc_BreakableIsEnemy ( edict_t *pBreakable, edict_t *pEdict );

// TODO: this is from game/server/util.h, remove once we can get it included
// Misc useful
inline bool FStrEq(const char *sz1, const char *sz2)
{
	return sz1 == sz2 || Q_stricmp(sz1, sz2) == 0;
}

/////////// Voice commands

class IBotFunction
{
public:
	virtual ~IBotFunction() = default;
	virtual void execute ( CBot *pBot ) = 0;
};

class CBroadcastVoiceCommand : public IBotFunction
{
public:
	CBroadcastVoiceCommand (edict_t *pPlayer, const byte voicecmd) { m_pPlayer = pPlayer; m_VoiceCmd = voicecmd; }
	void execute ( CBot *pBot ) override;

private:
	edict_t *m_pPlayer;
	byte m_VoiceCmd;
};

typedef union
{
	 struct
	 {
		  unsigned v1:2; // menu
		  unsigned v2:3; // extra info
		  unsigned unused:3;
	 }b1;

	 byte voicecmd;
}u_VOICECMD;

typedef union
{
	 struct
	 {
		  unsigned said_in_position:1; 
		  unsigned said_move_out:1;
		  unsigned said_area_clear:1;
		  unsigned unused:5;
	 }b1;

	 byte dat;
}squad_u;

// events
class CRCBotEventListener : public IGameEventListener2
{
	void Event( IGameEvent *pevent );
};

//extern IVDebugOverlay *debugoverlay;

class CBotButtons;
class CBotGAValues;
//class CBotMemoryNode;
//class CBotMemoryPop;
class CBotNeuralNet;
class CBotProfile;
class CBotSchedules;
class CBotStuckValues;
class CBotVisibles;
class CBotWeapon;
class CBotWeapons;
class CFindEnemyFunc;
class CGA;
class CPerceptron;
class CTrainingSet;
class CWaypoint;
class CWeapon;
class IBotNavigator;

enum : std::uint8_t
{
	MOVELOOK_DEFAULT = 0,
	MOVELOOK_THINK = 1,
	MOVELOOK_MODTHINK = 2,
	MOVELOOK_TASK = 3,
	MOVELOOK_LISTEN = 4,
	MOVELOOK_EVENT = 5,
	MOVELOOK_ATTACK = 6,
	MOVELOOK_OVERRIDE = 6
};

class CBotLastSee
{
public:
	CBotLastSee ()
	{
		reset();
	}

	void reset ()
	{
		m_pLastSee = nullptr; // edict
		m_fLastSeeTime = 0.0f; // time
	}

	CBotLastSee ( edict_t *pEdict );

	void update ();

	bool check (const edict_t *pEdict)
	{
		return pEdict == m_pLastSee.get();
	}

	bool hasSeen ( float fTime );

	Vector getLocation () const;
private:
	MyEHandle m_pLastSee; // edict
	float m_fLastSeeTime; // time
	Vector m_vLastSeeLoc; // location
	Vector m_vLastSeeVel; // velocity
};

typedef union bot_statistics_t 
{
  int data;
  struct 
  {
    byte m_iTeamMatesInRange;
	byte m_iEnemiesInRange;
	byte m_iEnemiesVisible;
	byte m_iTeamMatesVisible;
  } stats;
} bot_statistics_s;


/*
class CBotSquads 
{
public:
	CBotSquads ()
	{
	}

	void freeMapMemory()
	{
		unsigned i;

		for ( i = 0; i < m_theSquads.size(); i ++ )
			m_theSquads[i].freeMapMemory();

		m_theSquads.clear();
	}
private:
	vector<CBotSquad> m_theSquads;
}

class CBotSquad
{
public:
	CBotSquad ( edict_t *pLeader )
	{
		m_Leader = pLeader;
		m_Members.clear();
	}

	void addMember ( edict_t *pMember )
	{
		m_Members.emplace_back(MyEHandle(pMember));
	}

	void think ()
	{
		if ( !CBotGlobals::entityIsValid(m_Leader) || CBotGlobals::entityIsAlive(m_Leader) )
		{
			m_Leader = nullptr;

			// find a new leader
			unsigned i;

			for ( i = 0; i < m_Members.size(); i ++ )
			{
				if ( m_Members[i].get() )
				break;
			}
		}
	}

	bool killme()
	{
		return (m_Leader.get() == nullptr) && (m_Members.size() == 0);
	}

private:
	MyEHandle m_Leader;
	vector<MyEHandle> m_Members;
};
*/

class CBot 
{
public:
	virtual ~CBot() = default;

	static const float m_fAttackLowestHoldTime;
	static const float m_fAttackHighestHoldTime;
    static const float m_fAttackLowestLetGoTime;
	static const float m_fAttackHighestLetGoTime;

	CBot();

    void clearFailedWeaponSelect () { m_iPrevWeaponSelectFailed = 0; }
    void failWeaponSelect () { m_iPrevWeaponSelectFailed ++; }

	void debugMsg ( int iLev, const char *szMsg ) const;

	virtual unsigned maxEntityIndex ( ) { return RCBOT_MAXPLAYERS; }

// linux fix 1
	virtual void onInventoryApplication (){}
	
	// return distance from this origin

	int isDesiredClass (const int iClass) const
	{
		return m_iDesiredClass == iClass;
	}

	virtual void handleWeapons ();

    Vector getOrigin () const
    {
		return m_pController->GetLocalOrigin();
	}

	// linux fix 2
    float distanceFrom(const Vector& vOrigin) const
    {
		return (vOrigin - m_pController->GetLocalOrigin()).Length();
	}

    float distanceFrom(edict_t *pEntity) const
    {
		return (pEntity->GetCollideable()->GetCollisionOrigin() - m_pController->GetLocalOrigin()).Length();
		//return distanceFrom(CBotGlobals::entityOrigin(pEntity));
	}
	float distanceFrom2D(const Vector& vOrigin) const
	{
		return (vOrigin - m_pController->GetLocalOrigin()).Length2D();
	}

    float distanceFrom2D(edict_t *pEntity) const
    {
		return (pEntity->GetCollideable()->GetCollisionOrigin() - m_pController->GetLocalOrigin()).Length2D();
		//return distanceFrom(CBotGlobals::entityOrigin(pEntity));
	}

	/*
	 * init()
	 *
	 * initialize all bot variables 
	 * (this is called when bot is made for the first time)
	 */
	virtual void init (bool bVarInit=false);
	// setup buttons and data structures
	virtual void setup ();

    /*
	 * runPlayerMove()
	 *
	 * make bot move in the game
	 * botman : see CBasePlayer::RunNullCommand() for example of PlayerRunCommand()...
	 */
	inline void runPlayerMove();

	/*
	 * called when a bot dies
	 */
	virtual void died ( edict_t *pKiller, const char *pszWeapon );
	virtual void killed ( edict_t *pVictim, char *weapon );

	virtual int getTeam () const;

	float getCreateTime() const
	{
		return m_fTimeCreated;
	}

	bool isUnderWater ( ) const;

	CBotWeapon *getBestWeapon ( edict_t *pEnemy, bool bAllowMelee = true, bool bAllowMeleeFallback = true, bool bMeleeOnly = false, bool bExplosivesOnly = false ) const;

	virtual void modThink () {
	}

	virtual bool isEnemy ( edict_t *pEdict, bool bCheckWeapons = true ) { return false; }

    bool hasSomeConditions (const int iConditions) const
    {
		return (m_iConditions & static_cast<ConditionBitSet>(iConditions)).any();
	}

	virtual bool handleAttack ( CBotWeapon *pWeapon, edict_t *pEnemy );

	float DotProductFromOrigin (const Vector& pOrigin ) const;

	bool FVisible ( edict_t *pEdict, bool bCheckHead = false );
	bool isVisible (const edict_t *pEdict ) const;

    void setEnemy ( edict_t *pEnemy )
	{
		m_pEnemy = pEnemy;
	}

    int getConditions () const
    {
		static_assert(NUM_CONDITIONS <= std::numeric_limits<int>::digits, "Condition bitset is larger than int");
		return static_cast<int>(m_iConditions.to_ulong());
	}

    bool hasAllConditions (const int iConditions) const
    {
		return (m_iConditions & static_cast<ConditionBitSet>(iConditions)) == iConditions;
	}

    void updateCondition (const int iCondition)
	{
		m_iConditions |= iCondition;
	}

    void removeCondition (const int iCondition)
	{
		m_iConditions &= ~iCondition;
	}

	 bool FInViewCone ( edict_t *pEntity ) const;	

	/*
	 * make bot start the gmae, e.g join a team first
	 */
	virtual bool startGame ();
	virtual bool checkStuck ();
	virtual void currentlyDead ();

	/*
	 * initialize this bot as a new bot with the edict of pEdict
	 */
	bool createBotFromEdict(edict_t *pEdict, CBotProfile *pProfile);

	/*
	 * returns true if bot is used in game
	 */
    bool inUse () const
    {
		return m_bUsed && m_pEdict!= nullptr;
	}

	edict_t *getEdict () const;

	void setEdict ( edict_t *pEdict);

	bool FVisible (const Vector &vOrigin, edict_t *pDest = nullptr) const;

	Vector getEyePosition () const;

	void think ();

	virtual void friendlyFire( edict_t *pEdict ) {}

	virtual void freeMapMemory ();
	virtual void freeAllMemory ();

	///////////////////////////////
    bool moveToIsValid () const
    {
		return m_bMoveToIsValid;
	}

    bool lookAtIsValid () const
    {
		return m_bLookAtIsValid;
	}

	Vector getMoveTo() const
	{
		return m_vMoveTo;
	}

    bool moveFailed ()
	{
		const bool ret = m_bFailNextMove;

		m_bFailNextMove = false;

		return ret;
	}

	edict_t *getAvoidEntity () { return m_pAvoidEntity; }

	void setAvoidEntity ( edict_t *pEntity ) { m_pAvoidEntity = pEntity; }

	virtual void updateConditions ();

	virtual bool canAvoid ( edict_t *pEntity );

    bool hasEnemy () { return m_pEnemy && hasSomeConditions(CONDITION_SEE_CUR_ENEMY); }
    edict_t *getEnemy () const { return m_pEnemy; }


    void setMoveTo ( const Vector& vNew )
	{
		if ( m_iMoveLookPriority >= m_iMovePriority )
		{
			m_vMoveTo = vNew;
			m_bMoveToIsValid = true;
			m_iMovePriority = m_iMoveLookPriority;
		}
	}

	// this allows move speed to be changed in tasks
    void setMoveSpeed (const float fNewSpeed)
	{
		if ( m_iMoveLookPriority >= m_iMoveSpeedPriority )
		{
			m_fIdealMoveSpeed = fNewSpeed;
			m_iMoveSpeedPriority = m_iMoveLookPriority;
		}
	}

	void findEnemy ( edict_t *pOldEnemy = nullptr);
	virtual void enemyFound ( edict_t *pEnemy );

	virtual void checkDependantEntities ();

    IBotNavigator *getNavigator () const { return m_pNavigator; }

    void setMoveLookPriority (const int iPriority) { m_iMoveLookPriority = iPriority; }

    void stopMoving () 
	{ 
		if ( m_iMoveLookPriority >= m_iMovePriority )
		{
			m_bMoveToIsValid = false; 
			m_iMovePriority = m_iMoveLookPriority;
			m_fWaypointStuckTime = 0;
			m_fCheckStuckTime = engine->Time() + 4.0f;
		}
	}

    void stopLooking () 
	{ 
		if ( m_iMoveLookPriority >= m_iLookPriority )
		{
			m_bLookAtIsValid = false; 
			m_iLookPriority = m_iMoveLookPriority;
		}
	}

    void setLookAtTask (const eLookTask lookTask, const float fTime = 0) 
	{ 
		if ( m_iMoveLookPriority >= m_iLookPriority && (fTime > 0 || m_fLookSetTime < engine->Time()) )
		{
			m_iLookPriority = m_iMoveLookPriority;
			m_iLookTask = lookTask; 

			if ( fTime > 0 )
				m_fLookSetTime = engine->Time() + fTime;
		}	
	}

	virtual void enemyLost (edict_t *pEnemy) {}

	void setLastEnemy (edict_t *pEnemy);

	virtual void enemyDown (edict_t *pEnemy) 
	{ 
		if ( pEnemy == m_pEnemy ) 
			updateCondition(CONDITION_ENEMY_DEAD); 
		if ( pEnemy == m_pLastEnemy )
		{
			m_pLastEnemy = nullptr;
		}
	}
	//////////////////////
	virtual bool isCSS () { return false; }
	virtual bool isHLDM () { return false; }
	virtual bool isTF () { return false; }
	virtual bool isSYN () { return false; }

	virtual void spawnInit ();

	QAngle eyeAngles () const;

	virtual bool isAlive ();

	bool onLadder () const;

    bool currentEnemy ( edict_t *pEntity ) { return m_pEnemy == pEntity; }

	Vector getAimVector ( edict_t *pEntity );
	virtual void modAim ( edict_t *pEntity, Vector &v_origin, 
		Vector *v_desired_offset, Vector &v_size,
		float fDist, float fDist2D);

    Vector *getGoalOrigin ()
	{
		return &m_vGoal;
	}

    bool hasGoal () const
    {
		return m_bHasGoal;
	}

	bool isHoldingPrimaryAttack() const;

	void primaryAttack (bool bHold=false, float fTime =0.0f) const;
	void secondaryAttack(bool bHold=false, float fTime = 0.0f) const;
	void jump () const;
	void duck (bool hold = false) const;
	void use () const;
	void reload() const;

	virtual bool setVisible ( edict_t *pEntity, bool bVisible );

	virtual bool hurt ( edict_t *pAttacker, int iHealthNow, bool bDontHide = false );
	virtual void shot ( edict_t *pEnemy );
	virtual void shotmiss ();
	//inline void setAvoidEntity (edict_t *pEntity) { m_pAvoidEntity = pEntity; };
	
	int getPlayerID () const; // return player ID on server
	int getHealth () const;

	float getHealthPercent () const;

    CBotSchedules *getSchedule () const { return m_pSchedules; }

	virtual void reachedCoverSpot (int flags);

	virtual bool wantToFollowEnemy ();

	virtual void seeFriendlyHurtEnemy ( edict_t *pTeammate, edict_t *pEnemy, CWeapon *pWeapon ) {}
	virtual void seeEnemyHurtFriendly ( edict_t *pTeammate, edict_t *pEnemy, CWeapon *pWeapon ) {}
	virtual void seeFriendlyDie ( edict_t *pDied, edict_t *pKiller, CWeapon *pWeapon ) {}
	virtual void seeFriendlyKill ( edict_t *pTeamMate, edict_t *pDied, CWeapon *pWeapon ) {}

    void selectWeapon (const int iWeaponId ) { m_iSelectWeapon = iWeaponId; }

	void selectWeaponName ( const char *szWeaponName ) const;

	virtual CBotWeapon *getCurrentWeapon() const;

	void kill () const;

	bool isUsingProfile (const CBotProfile *pProfile ) const;

    CBotProfile *getProfile () const { return m_pProfile; }

	IPlayerInfo* getPlayerInfo() const { return m_pPlayerInfo; }

	float getFov() const { return m_fFov; }

	void setFov(float fov) { m_fFov = fov; }

	CBotVisibles* getVisibles() const { return m_pVisibles; }

	virtual bool canGotoWaypoint (const Vector& vPrevWaypoint, CWaypoint* pWaypoint, CWaypoint* pPrev = nullptr);
	
	void updatePosition() const;

	void tapButton ( int iButton ) const;

	int getAmmo(const std::size_t iIndex) const {
		if (!m_iAmmo) return 0;
		if (iIndex == static_cast<std::size_t>(-1)) return 0; // Ensure this check is meaningful
		return m_iAmmo[iIndex];
	}

    void lookAtEdict ( edict_t *pEdict ) { m_pLookEdict = pEdict; }

	virtual bool select_CWeapon ( CWeapon *pWeapon );
	virtual bool selectBotWeapon ( CBotWeapon *pBotWeapon );

	MyEHandle m_pLookEdict;

	CBotWeapons *getWeapons () const { return m_pWeapons; }

	virtual float getEnemyFactor ( edict_t *pEnemy );

	virtual void checkCanPickup ( edict_t *pPickup );

	virtual void touchedWpt ( CWaypoint *pWaypoint, int iNextWaypoint = -1, int iPrevWaypoint = -1 );

    void setAiming (const Vector& aiming) { m_vWaypointAim = aiming; }

	Vector getAiming() const { return m_vWaypointAim; }

    void setLookVector (const Vector& vLook) { m_vLookVector = vLook; }

	Vector getLookVector() const { return m_vLookVector; }

    void resetLookAroundTime () { m_fLookAroundTime = 0.0f; }

	Vector snipe (const Vector& vAiming);

	//inline void dontAvoid () { m_fAvoidTime = engine->Time() + 1.0f; }

	float m_fWaypointStuckTime;

    float getSpeed () const { return m_vVelocity.Length2D(); }

	void updateStatistics (); // updates number of teammates/enemies nearby/visible
	virtual void listenForPlayers ();
	// listens to this player
	void listenToPlayer (edict_t *pPlayer, bool bIsEnemy = false, bool bIsAttacking = false );
	virtual bool wantToListenToPlayerAttack ( edict_t *pPlayer, int iWeaponID = -1 ) { return true; }
	virtual bool wantToListenToPlayerFootsteps ( edict_t *pPlayer ) { return true; }

	virtual bool wantToInvestigateSound ();
    void wantToInvestigateSound (const bool bSet) { m_bWantToInvestigateSound = bSet; }
    bool wantToShoot () const { return m_bOpenFire; }
    void wantToShoot (const bool bSet) { m_bOpenFire = bSet; }
    void wantToListen (const bool bSet) { m_bWantToListen = bSet; }
	bool wantToListen () const;
    void wantToChangeWeapon (const bool bSet) { m_bWantToChangeWeapon = bSet; }

	int nearbyFriendlies (float fDistance);
	
	bool isFacing (const Vector& vOrigin) const;

	bool isOnLift () const;

	virtual bool isDOD () { return false; }
	virtual bool isTF2 () { return false; }

	// return an enemy sentry gun / special visible (e.g.) for quick checking - voffset is the 'head'
	virtual edict_t *getVisibleSpecial ();

	void updateDanger ( float fBelief );

    void reduceTouchDistance()
    {
	    if (m_fWaypointTouchDistance > MIN_WPT_TOUCH_DIST) { m_fWaypointTouchDistance *= 0.9f; }
    }

    void resetTouchDistance (const float fDist) { m_fWaypointTouchDistance = fDist; }

    float getTouchDistance () const { return m_fWaypointTouchDistance; }

    CBotCmd *getUserCMD () { return &cmd; }

	void forceGotoWaypoint ( int wpt ) const;

	// bot is defending -- mod specific stuff
	virtual void defending () {}

	virtual void hearVoiceCommand ( edict_t *pPlayer, byte voiceCmd );

	virtual void grenadeThrown ();

	virtual void voiceCommand (byte voiceCmd);

	void addVoiceCommand (byte voiceCmd);

	void letGoOfButton ( int button ) const;

	virtual bool overrideAmmoTypes () { return true; }

	virtual void debugBot ( char *msg );

	virtual bool walkingTowardsWaypoint ( CWaypoint *pWaypoint, bool *bOffsetApplied, Vector &vOffset );

	void setCoverFrom ( edict_t *pCoverFrom ) { m_pLastCoverFrom = MyEHandle(pCoverFrom); }

	virtual void areaClear( ) { }

    void resetAreaClear () { m_uSquadDetail.b1.said_area_clear = false; }


    bool inSquad (const CBotSquad *pSquad) const
    {
		return m_pSquad == pSquad;
	}

    bool inSquad () const
    {
		return m_pSquad != nullptr;
	}

	bool isSquadLeader () const;

    void setSquadIdleTime (const float fTime)
	{
		m_fSquadIdleTime = fTime;
	}

	void clearSquad ();

    void setSquad ( CBotSquad *pSquad )
	{
		m_pSquad = pSquad;
	}

	CBotSquad* getSquad() const
	{
		return m_pSquad;
	}

	void SquadInPosition ( );
	virtual void sayInPosition() { }
	virtual void sayMoveOut() { }

	bot_statistics_t *getStats() { if ( m_bStatsCanUse ) return &m_StatsCanUse; return nullptr; }

	virtual void hearPlayerAttack( edict_t *pAttacker, int iWeaponID );

    bool isListeningToPlayer (const edict_t *pPlayer) 
	{
		return m_PlayerListeningTo.get() == pPlayer;
	}

    IBotController *getController () const 
	{
		return m_pController;
	}

	void updateUtilTime ( int util );

	virtual bool getIgnoreBox ( Vector *vLoc, float *fSize )
	{
		return false;
	}

	bool recentlyHurt ( float fTime ) const;

	eBotAction getCurrentUtil () const { return m_CurrentUtil;}

	bool recentlySpawned ( float fTime ) const;

protected:
    void setLookAt (const Vector& vNew)
	{
		m_vLookAt = vNew;
		m_bLookAtIsValid = true;
	}

	static void checkEntity ( edict_t **pEdict );
	/////////////////////////
	void doMove ();
	void doLook ();

	virtual void getLookAtVector ();

	void doButtons ();
	/////////////////////////

	void changeAngles (float fSpeed, const float* fIdeal, float* fCurrent, float* fUpdate);

	// look for new tasks
	virtual void getTasks (unsigned iIgnore=0);

	// really only need 249 bits (32 bytes) + WEAPON_SUBTYPE_BITS (whatever that is)
	static constexpr int CMD_BUFFER_SIZE = 64; 
	///////////////////////////////////
	// bots edict
	edict_t *m_pEdict;
	MyEHandle m_pAvoidEntity;
	//float m_fLastPrintDebugInfo;
	// is bot used in the game?
	bool m_bUsed;
	// time the bot was made in the server
	float m_fTimeCreated;
	// next think time
	float m_fNextThink;

	float m_fFov;
	
	bool m_bInitAlive;
	bool m_bThinkStuck;

	int m_iMovePriority;
	int m_iLookPriority;
	int m_iMoveSpeedPriority;

	int m_iMoveLookPriority;

	int *m_iAmmo;
	bool m_bLookedForEnemyLast;

	//CBotStuckValues *m_pGAvStuck;
	//CGA *m_pGAStuck;
	//CPerceptron *m_pThinkStuck;
	Vector m_vStuckPos;
	//int m_iTimesStuck;
	float m_fAvoidTime;
	///////////////////////////////////
	// current impulse command
	byte m_iImpulse;
	// buttons held
	int m_iButtons;
	// bots forward move speed
	float m_fForwardSpeed;
	// bots side move speed
	float m_fSideSpeed;
	// bots upward move speed (e.g in water)
	float m_fUpSpeed;
	float m_fAimMoment; // aiming "mouse" momentum

	float m_fLookAtTimeStart;
	float m_fLookAtTimeEnd;
	// Look task can't be changed if this is greater than Time()
	float m_fLookSetTime; 
	float m_fLookAroundTime;

	int m_iFlags;
	// Origin a second ago to check if stuck
	Vector m_vLastOrigin;
	// Generated velocity found from last origin (not always correct)
	Vector m_vVelocity;
	// next update time (1 second update)
	float m_fUpdateOriginTime;
	float m_fStuckTime;
	float m_fCheckStuckTime;
	float m_fNextUpdateStuckConstants;

	float m_fStrafeTime;
	float m_fLastSeeEnemy;
	float m_fLastUpdateLastSeeEnemy;

	float m_fUpdateDamageTime;
	// Damage bot accumulated over the last second or so
	int m_iAccumulatedDamage;
	int m_iPrevHealth;
	///////////////////////////////////
	ConditionBitSet m_iConditions;

	// bot tasks etc -- complex actuators
	CBotSchedules *m_pSchedules;
	// buttons held -- simple actuators
	CBotButtons *m_pButtons;
	// Navigation used for this bot -- environment sensor 1
	IBotNavigator *m_pNavigator;
	// bot visible list -- environment sensor 2
	CBotVisibles *m_pVisibles;
	// visible functions -- sensory functions
	CFindEnemyFunc *m_pFindEnemyFunc;
	// weapons storage -- sensor
	CBotWeapons *m_pWeapons;
	////////////////////////////////////
	IPlayerInfo *m_pPlayerInfo; //-- sensors
	IBotController *m_pController; //-- actuators
	CBotCmd cmd; // actuator command
	////////////////////////////////////
	MyEHandle m_pEnemy; // current enemy
	MyEHandle m_pOldEnemy;
	Vector m_vLastSeeEnemy;
	Vector m_vLastSeeEnemyBlastWaypoint;
	MyEHandle m_pLastEnemy; // enemy we were fighting before we lost it
	//edict_t *m_pAvoidEntity; // avoid this guy
	Vector m_vHurtOrigin;
	Vector m_vLookVector;
	Vector m_vLookAroundOffset;
	MyEHandle m_pPickup;
	Vector m_vWaypointAim;

	Vector m_vMoveTo;
	Vector m_vLookAt;
	Vector m_vGoal; // goal vector
	bool m_bHasGoal;
	QAngle m_vViewAngles;
	float m_fNextUpdateAimVector;
	float m_fStartUpdateAimVector;
	Vector m_vAimVector;
	Vector m_vPrevAimVector;
	Vector m_vLastDiedOrigin;
	bool m_bPrevAimVectorValid;

	eLookTask m_iLookTask;

	bool m_bMoveToIsValid;
	bool m_bLookAtIsValid;

	float m_fIdealMoveSpeed;

	float m_fLastWaypointVisible;

	bool m_bFailNextMove;

	int m_iSelectWeapon;

	int m_iDesiredTeam;
	int m_iDesiredClass;

	// bots profile data
	CBotProfile *m_pProfile;

	float m_fPercentMoved;

	/////////////////////////////////

	char m_szBotName[64];

	/////////////////////////////////
	Vector m_vListenPosition; // listening player position, heard someone shoot
	bool m_bListenPositionValid;
	float m_fListenTime;
	MyEHandle m_PlayerListeningTo;
	float m_fWantToListenTime;
	bool m_bOpenFire;
	unsigned m_iPrevWeaponSelectFailed;

	bool m_bWantToListen;
	float m_fListenFactor; // the current weight of bots listening vector (higher = better)
	float m_fUseRouteTime;

	bool m_bWantToChangeWeapon;

	bool m_bAvoidRight;
	float m_fAvoidSideSwitch;
	float m_fHealClickTime;

	int m_iSpecialVisibleId;
	float m_fCurrentDanger;
	float m_fLastHurtTime;

	float m_fUtilTimes[BOT_UTIL_MAX];	
	
	float m_fWaypointTouchDistance;

	eBotAction m_CurrentUtil;
	//CBotNeuralNet *stucknet;
	//CTrainingSet *stucknet_tset;

	std::queue<int> m_nextVoicecmd;
	float m_fNextVoiceCommand;
	float m_fLastVoiceCommand[MAX_VOICE_CMDS];

	float m_fTotalAimFactor;
	Vector m_vAimOffset;
	MyEHandle m_pLastCoverFrom;

	bot_statistics_t m_Stats; // this updates progressively
	bot_statistics_t m_StatsCanUse; // this updates fully every 5 seconds max
	bool m_bStatsCanUse;
	float m_fStatsTime;
	short int m_iStatsIndex;

	CBotSquad *m_pSquad;
	float m_fSquadIdleTime;
	squad_u m_uSquadDetail;
	CBotWeapon *m_pPrimaryWeapon;
	float m_fLastSeeEnemyPlayer;
	// if bot's profile sensitivity is too small it may not complete tasks
	// this is true during tasks that need high sensitivity e.g. rocket jumping
	bool m_bIncreaseSensitivity; 
	float m_fSpawnTime;
	bool m_bWantToInvestigateSound;

	float m_fEnemyAimLerp;
	float m_fEnemyAimLerpTime;
	Vector m_vEnemyAimLerpVelocity;
};

class CBots
{
public:
	static void botThink ();

	static CBot *getBotPointer (const edict_t *pEdict );
	static CBot *getBot ( int slot );

	static void freeMapMemory ();
	static void freeAllMemory ();

	static CBot *findBotByProfile (const CBotProfile *pProfile );

	static void init ();

	static bool controlBot ( edict_t *pEdict );
	static bool controlBot ( const char *szOldName, const char *szName, const char *szTeam, const char *szClass );

	static bool createBot (const char *szClass, const char *szTeam, const char *szName);
	static int createDefaultBot(const char* szName);

	static int numBots ();

	static int slotOfEdict (const edict_t* pEdict);

	static void roundStart ();

	static void kickChosenBot (unsigned count = 1);
	static void kickRandomBot (unsigned count = 1);
	static void kickChosenBotOnTeam ( int team );
	static void kickRandomBotOnTeam ( int team );
	static void kickBot (edict_t *pEdict);

	static void mapInit ();

	static bool needToAddBot ();
	static bool needToKickBot ();

	static void setMaxBots (const int iMax) { m_iMaxBots = iMax; }
	static int getMaxBots () { return m_iMaxBots; }

	static void setMinBots (const int iMin) { m_iMinBots = iMin; }
	static int getMinBots () { return m_iMinBots; }

	static float getAddKickBotTime() { return m_flAddKickBotTime; }

	static void botFunction ( IBotFunction *function );

	static void runPlayerMoveAll ();

	static CBot* get(const std::size_t iIndex) { return m_Bots[iIndex]; }

	static CBot* get(const edict_t* pPlayer)
	{
		return m_Bots[static_cast<std::size_t>(slotOfEdict(pPlayer))];
	}

	static int levelInit(); //TODO: Not implemented? [APG]RoboCop[CL]

private:
	static CBot **m_Bots;

	//config
	static int m_iMaxBots;
	static int m_iMinBots;

	// add or kick bot time
	static float m_flAddKickBotTime;
};

// this is yoinked from server/util.h
abstract_class IEntityFactory
{
protected:
	~IEntityFactory() = default;

public:
	// TODO: Investigate why making this destructor virtual causes a crash - [APG]RoboCop[CL]
	//virtual ~IEntityFactory() = default; //Unstable
	virtual IServerNetworkable *Create( const char *pClassName ) = 0;
	virtual void Destroy( IServerNetworkable *pNetworkable ) = 0;
	virtual unsigned GetEntitySize() = 0;
};

abstract_class IEntityFactoryDictionary
{
protected:
	~IEntityFactoryDictionary() = default;

public:
	//virtual ~IEntityFactoryDictionary() = default;
	virtual void InstallFactory( IEntityFactory *pFactory, const char *pClassName ) = 0;
	virtual IServerNetworkable *Create( const char *pClassName ) = 0;
	virtual void Destroy( const char *pClassName, IServerNetworkable *pNetworkable ) = 0;
	virtual IEntityFactory *FindFactory( const char *pClassName ) = 0;
	virtual const char *GetCannonicalName( const char *pClassName ) = 0;
};

#endif // __RCBOT2_H__
