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
#ifndef __BOT_FORTRESS_H__
#define __BOT_FORTRESS_H__

#include "bot_utility.h"

//#include <stack>

enum : std::uint16_t
{
	TF2_ROCKETSPEED = 1100,
	TF2_GRENADESPEED = 1065,
	TF2_MAX_SENTRYGUN_RANGE = 1024,
	TF2_STICKYGRENADE_MAX_DISTANCE = 1600
};

class CBotWeapon;
class CWaypoint;
class CBotUtility;

enum : std::uint8_t
{
	TF2_SLOT_PRMRY = 0,	// primary
	TF2_SLOT_SCNDR = 1,	// secondary
	TF2_SLOT_MELEE = 2,
	TF2_SLOT_PDA = 3,
	TF2_SLOT_PDA2 = 4,
	TF2_SLOT_HAT = 5,
	TF2_SLOT_MISC = 6,
	TF2_SLOT_ACTION = 7,
	TF2_SLOT_MAX = 8
};

enum : std::uint8_t
{
	TF2_TEAM_BLUE = 3,
	TF2_TEAM_RED = 2
};

enum : std::uint8_t
{
	RESIST_BULLETS = 0,
	RESIST_EXPLO = 1,
	RESIST_FIRE = 2
};

enum : std::uint8_t
{
	TF2_SENTRY_LEVEL1_HEALTH = 150,
	TF2_SENTRY_LEVEL2_HEALTH = 180,
	TF2_SENTRY_LEVEL3_HEALTH = 216
};

enum : std::uint8_t
{
	TF2_DISPENSER_LEVEL1_HEALTH = 150,
	TF2_DISPENSER_LEVEL2_HEALTH = 180,
	TF2_DISPENSER_LEVEL3_HEALTH = 216
};

// Naris @ Alliedmodders.net
/*
enum TFCond
{
	TFCond_Slowed = 0,
	TFCond_Zoomed,
	TFCond_Disguising,
	TFCond_Disguised,
	TFCond_Cloaked,
	TFCond_Ubercharged,
	TFCond_TeleportedGlow,
	TFCond_Taunting,
	TFCond_UberchargeFading,
	TFCond_Unknown1, //9
	TFCond_CloakFlicker = 9,
	TFCond_Teleporting,
	TFCond_Kritzkrieged,
	TFCond_Unknown2, //12
	TFCond_TmpDamageBonus = 12,
	TFCond_DeadRingered,
	TFCond_Bonked,
	TFCond_Dazed,
	TFCond_Buffed,
	TFCond_Charging,
	TFCond_DemoBuff,
	TFCond_CritCola,
	TFCond_InHealRadius,
	TFCond_Healing,
	TFCond_OnFire,
	TFCond_Overhealed,
	TFCond_Jarated,
	TFCond_Bleeding,
	TFCond_DefenseBuffed,
	TFCond_Milked,
	TFCond_MegaHeal,
	TFCond_RegenBuffed,
	TFCond_MarkedForDeath,
	TFCond_NoHealingDamageBuff,
	TFCond_SpeedBuffAlly, // 32
	TFCond_HalloweenCritCandy,
	TFCond_CritCanteen,
	TFCond_CritDemoCharge,
	TFCond_CritHype,
	TFCond_CritOnFirstBlood,
	TFCond_CritOnWin,
	TFCond_CritOnFlagCapture,
	TFCond_CritOnKill,
	TFCond_RestrictToMelee,
	TFCond_DefenseBuffNoCritBlock,
	TFCond_Reprogrammed,
	TFCond_CritMmmph,
	TFCond_DefenseBuffMmmph,
	TFCond_FocusBuff,
	TFCond_DisguiseRemoved,
	TFCond_MarkedForDeathSilent,
	TFCond_DisguisedAsDispenser,
	TFCond_Sapped,
	TFCond_UberchargedHidden,
	TFCond_UberchargedCanteen,
	TFCond_HalloweenBombHead,
	TFCond_HalloweenThriller,
	TFCond_RadiusHealOnDamage,
	TFCond_CritOnDamage,
	TFCond_UberchargedOnTakeDamage,
	TFCond_UberBulletResist,
	TFCond_UberBlastResist,
	TFCond_UberFireResist,
	TFCond_SmallBulletResist,
	TFCond_SmallBlastResist,
	TFCond_SmallFireResist,
	TFCond_Stealthed, // 64
	TFCond_MedigunDebuff,
	TFCond_StealthedUserBuffFade,
	TFCond_BulletImmune,
	TFCond_BlastImmune,
	TFCond_FireImmune,
	TFCond_PreventDeath,
	TFCond_MVMBotRadiowave,
	TFCond_HalloweenSpeedBoost,
	TFCond_HalloweenQuickHeal,
	TFCond_HalloweenGiant,
	TFCond_HalloweenTiny,
	TFCond_HalloweenInHell,
	TFCond_HalloweenGhostMode,

	TFCond_DodgeChance = 79,
	TFCond_Parachute,
	TFCond_BlastJumping,
	TFCond_HalloweenKart,
	TFCond_HalloweenKartDash,
	TFCond_BalloonHead,
	TFCond_MeleeOnly,
	TFCond_SwimmingCurse,
	TFCond_HalloweenKartNoTurn,
	TFCond_HalloweenKartCage,
	TFCond_HasRune,
	TFCond_RuneStrength,
	TFCond_RuneHaste,
	TFCond_RuneRegen,
	TFCond_RuneResist,
	TFCond_RuneVampire,
	TFCond_RuneWarlock,
	TFCond_RunePrecision, // 96
	TFCond_RuneAgility,
};*/

enum
{
	TF2_PLAYER_BONKED = 1<<14,
	TF2_PLAYER_SLOWED = 1 << 0,	// 1
	TF2_PLAYER_ZOOMED = 1 << 1,	// 2
	TF2_PLAYER_DISGUISING = 1 << 2,	// 4
	TF2_PLAYER_DISGUISED = 1 << 3,	// 8
	TF2_PLAYER_CLOAKED = 1 << 4,	// 16
	TF2_PLAYER_INVULN = 1 << 5,	// 32
	TF2_PLAYER_TELEGLOW = 1 << 6,	// 64
	TF2_PLAYER_KRITS = 524288,
	TF2_PLAYER_HEALING = 2097152,
	TF2_PLAYER_TAUNTING = 1 << 7,	// 128
	TF2_PLAYER_TELEPORTING = 1<<10,	// 1024 Player is teleporting
	TF2_PLAYER_ONFIRE = 4194304 // fix may 2013
};

//#define TF2_SPY_FOV_KNIFEATTACK 90.0f

typedef enum : std::uint8_t
{
	TF_VC_MEDIC = 0,
	TF_VC_INCOMING = 1,
	TF_VC_HELP = 2,
	TF_VC_THANKS = 4,
	TF_VC_SPY = 5,
	TF_VC_BATTLECRY = 6,
	TF_VC_GOGOGO = 8,
	TF_VC_SENTRYAHEAD = 9,
	TF_VC_CHEERS = 10,
	TF_VC_MOVEUP = 12,
	TF_VC_TELEPORTERHERE = 13,//TODO: allow engineers to hearcommands [APG]RoboCop[CL]
	TF_VC_JEERS = 14,
	TF_VC_GOLEFT = 16,
	TF_VC_DISPENSERHERE = 17,// TODO: allow engineers to hearcommands [APG]RoboCop[CL]
	TF_VC_POSITIVE = 18,
	TF_VC_GORIGHT = 20,
	TF_VC_SENTRYHERE = 21,
	TF_VC_NEGATIVE = 22,
	TF_VC_YES = 24,
	TF_VC_ACTIVATEUBER = 25,
	TF_VC_NICESHOT = 26,
	TF_VC_NO = 28,
	TF_VC_UBERREADY = 29,
	TF_VC_GOODJOB = 30,
	TF_VC_INVALID = 31
}eTFVoiceCMD;


typedef enum : std::uint8_t
{
	TF_TRAP_TYPE_NONE,
	TF_TRAP_TYPE_WPT,
	TF_TRAP_TYPE_POINT,
	TF_TRAP_TYPE_FLAG,
	TF_TRAP_TYPE_PL,
	TF_TRAP_TYPE_ENEMY
}eDemoTrapType;

//TODO: To allow the proper slot# values to be used,
//the class names should be changed to the slot names [APG]RoboCop[CL]

/*typedef enum
{
	TF_CLASS_UNDEFINED = 0,
	TF_CLASS_SCOUT = 1,
	TF_CLASS_SOLDIER = 2,
	TF_CLASS_PYRO = 3,
	TF_CLASS_DEMOMAN = 4,
	TF_CLASS_HWGUY = 5,
	TF_CLASS_MEDIC = 6,
	TF_CLASS_ENGINEER = 7,
	TF_CLASS_SNIPER = 8,
	TF_CLASS_SPY = 9,
	TF_CLASS_MAX = 10
}TF_Class;*/

typedef enum : std::uint8_t
{
	TF_CLASS_UNDEFINED = 0,
	TF_CLASS_SCOUT,
	TF_CLASS_SNIPER,
	TF_CLASS_SOLDIER,
	TF_CLASS_DEMOMAN,
	TF_CLASS_MEDIC,
	TF_CLASS_HWGUY,
	TF_CLASS_PYRO,
	TF_CLASS_SPY,
	TF_CLASS_ENGINEER,
	TF_CLASS_MAX
}TF_Class;

/*enum
{
	TF_TEAM_SPEC = 0,
	TF_TEAM_BLUE = 1,
	TF_TEAM_RED = 2,
	TF_TEAM_GREEN = 3,
	TF_TEAM_YELLOW = 4
};

typedef enum
{
	ENGI_DISP = 0,
	ENGI_ENTRANCE,
	ENGI_EXIT,
	ENGI_SENTRY,
	ENGI_SAPPER
}eEngiBuild;*/
typedef enum : std::uint8_t
{
	ENGI_DISP = 0,
	ENGI_TELE,
	ENGI_SENTRY,
	ENGI_SAPPER,
	ENGI_EXIT,
	ENGI_ENTRANCE,
}eEngiBuild;

typedef enum : std::uint8_t
{
	ENGI_BUILD,
	ENGI_DESTROY
}eEngiCmd;

class CBotTF2FunctionEnemyAtIntel : public IBotFunction
{
public:
	CBotTF2FunctionEnemyAtIntel(const int iTeam, const Vector& vPos, const int type, edict_t* pPlayer = nullptr, const int capindex = -1) : m_vPos(vPos)
	{
		m_iTeam = iTeam;
		m_iType = type;
		m_pPlayer = pPlayer;
		m_iCapIndex = capindex;
	}

	void execute (CBot *pBot) override;
private:
	int m_iTeam;
	Vector m_vPos;
	int m_iType;
	edict_t *m_pPlayer;
	int m_iCapIndex;
};

class CBroadcastSpySap : public IBotFunction
{
public:
	CBroadcastSpySap (edict_t *pSpy) { m_pSpy = pSpy; }
	void execute ( CBot *pBot ) override;

private:
	edict_t *m_pSpy;
};

class CBroadcastOvertime : public IBotFunction
{
public:
	CBroadcastOvertime () = default;
	void execute (CBot *pBot) override;
};

class CBroadcastFlagReturned : public IBotFunction
{
public:
	CBroadcastFlagReturned (const int iTeam) { m_iTeam = iTeam; }
	void execute ( CBot *pBot ) override;

private:
	int m_iTeam;
};

class CBroadcastFlagDropped : public IBotFunction
{
public:
	CBroadcastFlagDropped(const int iTeam, const Vector& origin)
		: m_vOrigin(origin), m_iTeam(iTeam)
	{
	}

	void execute ( CBot *pBot ) override;

private:
	Vector m_vOrigin;
	int m_iTeam;
};

class CBroadcastFlagCaptured : public IBotFunction
{
public:
	CBroadcastFlagCaptured(const int iTeam) { m_iTeam = iTeam; }

	void execute ( CBot *pBot ) override;
private:
	int m_iTeam;
};

class CBroadcastRoundStart : public IBotFunction
{
public:
	CBroadcastRoundStart (const bool bFullReset) { m_bFullReset = bFullReset; }
	void execute ( CBot *pBot ) override;
private:
	bool m_bFullReset;
};

class CBroadcastCapturedPoint : public IBotFunction
{
public:
	CBroadcastCapturedPoint ( int iPoint, int iTeam, const char *szName );

	void execute ( CBot *pBot ) override;
private:
	int m_iPoint;
	int m_iTeam;
	const char *m_szName;
};

enum : std::uint8_t
{
	EVENT_FLAG_PICKUP = 0,
	EVENT_CAPPOINT = 1
};

class CBotFortress : public CBot
{
public:	

	CBotFortress();

	//virtual bool wantToZoom () { return m_bWantToZoom; }

	//virtual void wantToZoom ( bool bSet ) { m_bWantToZoom = bSet; }

	void enemyLost (edict_t *pEnemy) override;

	void updateConditions() override;

	void shot ( edict_t *pEnemy ) override;

	virtual int engiBuildObject ( int *iState, eEngiBuild iObject, float *fTime, int *iTries );

	float getEnemyFactor ( edict_t *pEnemy ) override { return CBot::getEnemyFactor(pEnemy); }

	void checkDependantEntities() override;

	int getMetal () const;

	//virtual Vector getAimVector ( edict_t *pEntity ) { return CBot::getAimVector(pEntity); }

	void modAim ( edict_t *pEntity, Vector &v_origin, Vector *v_desired_offset, Vector &v_size, const float fDist, const float fDist2D ) override
	{
		CBot::modAim(pEntity,v_origin,v_desired_offset,v_size,fDist,fDist2D);
	}

	void touchedWpt ( CWaypoint *pWaypoint, int iNextWaypoint = -1, int iPrevWaypoint = -1 ) override { CBot::touchedWpt(pWaypoint); }

	edict_t *getHealingEntity () { return m_pHeal; }

	void clearHealingEntity () { m_pHeal = nullptr; }

	unsigned maxEntityIndex () override { return gpGlobals->maxEntities; }

	void init (bool bVarInit=false) override;

	virtual void foundSpy (edict_t *pEdict, TF_Class iDisguise );

	void getTasks (const unsigned iIgnore = 0) override { CBot :: getTasks(iIgnore); }

	void died ( edict_t *pKiller, const char *pszWeapon ) override;

	void killed ( edict_t *pVictim, char *weapon ) override;

	void modThink () override;

	bool isBuilding (const edict_t *pBuilding );

	float getHealFactor ( edict_t *pPlayer );

	bool wantToFollowEnemy () override;

	virtual void checkBuildingsValid (bool bForce = false) {}

	virtual void checkHealingValid ();

// linux fix 2
	virtual edict_t *findEngineerBuiltObject ( eEngiBuild iBuilding, int index ) { return nullptr; }

	virtual void engineerBuild ( eEngiBuild iBuilding, eEngiCmd iEngiCmd ) {}

	virtual void spyDisguise (const int iTeam, const byte iClass) {}

	virtual bool lookAfterBuildings (float *fTime) { return false; }

	void nextLookAfterSentryTime (const float fTime) { m_fLookAfterSentryTime = fTime; }

	edict_t *getSentry () { return m_pSentryGun; }

	virtual bool hasEngineerBuilt ( eEngiBuild iBuilding ) {return false;}

	virtual void engiBuildSuccess ( eEngiBuild iBuilding, int index ) {}

	virtual bool healPlayer ( edict_t *pPlayer, edict_t *pPrevPlayer ) { return false; }
	virtual bool upgradeBuilding ( edict_t *pBuilding, bool removesapper = false) {return false;}

	virtual bool isCloaked () { return false; }
	virtual bool isDisguised () { return false; }


	CBotWeapon *getCurrentWeapon() const override
	{
		return CBot::getCurrentWeapon();
	}

	bool handleAttack(CBotWeapon *pWeapon, edict_t *pEnemy) override { return CBot::handleAttack(pWeapon, pEnemy); }

	void resetAttackingEnemy() { m_pAttackingEnemy = nullptr; }

	bool setVisible ( edict_t *pEntity, bool bVisible ) override;

	virtual void setClass ( TF_Class _class );

	edict_t *seeFlag (const bool reset = false) { if ( reset ) { m_pFlag = nullptr; } return m_pFlag; }

	bool canAvoid ( edict_t *pEntity ) override;

	bool isEnemy ( edict_t *pEdict,bool bCheckWeapons = true ) override;

	bool startGame () override;

	void spawnInit () override;

	bool isTF () override { return true; }

	bool isTF2 () override { return false; }

	bool hurt ( edict_t *pAttacker, const int iHealthNow, const bool bDontHide  = false ) override
	{
		return CBot::hurt(pAttacker,iHealthNow,bDontHide);
	}

	void chooseClass ();

	virtual TF_Class getClass () { return TF_CLASS_UNDEFINED; }

	virtual void updateClass () {}

	void currentlyDead () override;

	void onInventoryApplication () override {}

	void pickedUpFlag ();

	bool hasFlag () const { return m_bHasFlag; }

	void droppedFlag () { m_bHasFlag = false; }

	void medicCalled ( edict_t *pPlayer );

	bool isAlive () override;

	void enemyDown (edict_t *pEnemy) override
	{ 
		CBot::enemyDown(pEnemy);

		if ( pEnemy == m_pPrevSpy )
		{
			m_pPrevSpy = nullptr;
			m_fSeeSpyTime = 0.0f;
		}
	}

	bool isTeleporterUseful ( edict_t *pTele ) const;

	bool waitForFlag ( Vector *vOrigin, float *fWait, bool bFindFlag );

	void flagDropped (const Vector& vOrigin );
	void teamFlagDropped (const Vector& vOrigin );
	void teamFlagPickup () const;

	virtual bool wantToListenToPlayer ( edict_t *pPlayer, int iWeaponID = -1 ) { return true; }
	bool wantToListenToPlayerFootsteps ( edict_t *pPlayer ) override { return true; }
	bool wantToInvestigateSound () override { return true; }

	void flagReset () { m_fLastKnownFlagTime = 0.0f; }
	void teamFlagReset () { m_fLastKnownTeamFlagTime = 0.0f; }

	bool canGotoWaypoint (const Vector& vPrevWaypoint, CWaypoint* pWaypoint, CWaypoint* pPrev = nullptr) override
	{
		return CBot::canGotoWaypoint(vPrevWaypoint,pWaypoint,pPrev);
	}

	void setup () override;

	virtual bool needHealth();

	virtual bool needAmmo ();

	void waitBackstab ();

	void wantToDisguise ( bool bSet );

	bool select_CWeapon ( CWeapon *pWeapon ) override { return CBot::select_CWeapon(pWeapon); }
	bool selectBotWeapon ( CBotWeapon *pBotWeapon ) override { return CBot::selectBotWeapon(pBotWeapon); }

	bool getIgnoreBox ( Vector *vLoc, float *fSize ) override;

	// found a new enemy
	void enemyFound (edict_t *pEnemy) override {CBot::enemyFound(pEnemy); }

	bool wantToNest ();

	bool overrideAmmoTypes () const override { return false; }

	bool wantToCloak();

	bool wantToUnCloak();

	bool someoneCalledMedic ();

	void waitCloak ();

	void detectedAsSpy ( edict_t *pDetector, bool bDisguiseComprimised );

	// return an enemy sentry gun / special visible (e.g.) for quick checking
	edict_t *getVisibleSpecial () override;

	bool isBeingHealed () const { return m_bIsBeingHealed; }

	void handleWeapons () override { CBot::handleWeapons(); }

	void seeFriendlyDie ( edict_t *pDied, edict_t *pKiller, CWeapon *pWeapon ) override { CBot::seeFriendlyDie(pDied,pKiller,pWeapon); }
	void seeFriendlyKill ( edict_t *pTeamMate, edict_t *pDied, CWeapon *pWeapon ) override { CBot::seeFriendlyKill(pTeamMate,pDied,pWeapon); }

	void voiceCommand (byte voiceCmd) override {}

	void seeFriendlyHurtEnemy ( edict_t *pTeammate, edict_t *pEnemy, CWeapon *pWeapon ) override;
	
	bool incomingRocket ( float fRange );

	void hearPlayerAttack( edict_t *pAttacker, const int iWeaponID ) override { CBot::hearPlayerAttack(pAttacker,iWeaponID); }
protected:
	virtual void selectTeam ();

	virtual void selectClass ();

	virtual void callMedic ();

	static bool isClassOnTeam ( int iClass, int iTeam );

	int getSpyDisguiseClass ( int iTeam ) const;

	virtual bool thinkSpyIsEnemy ( edict_t *pEdict, TF_Class iDisguise );

	bool checkStuck () override { return CBot::checkStuck(); }

	float m_fCallMedic;
	float m_fTauntTime;
	float m_fTaunting;
	float m_fDefendTime;

	float m_fHealFactor;

	MyEHandle m_pHeal;
	MyEHandle m_pLastHeal;
	MyEHandle m_pSentryGun;
	MyEHandle m_pDispenser;
	MyEHandle m_pTeleEntrance;
	MyEHandle m_pTeleExit;

	MyEHandle m_pAmmo;
	MyEHandle m_pHealthkit;

	MyEHandle m_pNearestDisp;
	MyEHandle m_pNearestEnemySentry;
	MyEHandle m_pNearestAllySentry;
	MyEHandle m_pNearestEnemyTeleporter;
	MyEHandle m_pNearestEnemyDisp;
	MyEHandle m_pNearestTeleEntrance;
	MyEHandle m_pNearestPipeGren;
	MyEHandle m_pAttackingEnemy;

	MyEHandle m_pFlag;
	MyEHandle m_pPrevSpy;

	float m_fFrenzyTime;
	float m_fSpyCloakTime;
	float m_fSpyUncloakTime;
	float m_fSeeSpyTime;
	float m_fLastSeeSpyTime;
	float m_fSpyDisguiseTime;
	float m_fLastSaySpy;
	float m_fPickupTime;
	float m_fLookAfterSentryTime;

	TF_Class m_iPrevSpyDisguise;

	Vector m_vLastSeeSpy;

	// valid flag point area
	Vector m_vLastKnownFlagPoint;
	Vector m_vLastKnownTeamFlagPoint;

	Vector m_vTeleportEntrance;
	bool m_bEntranceVectorValid;
	Vector m_vSentryGun;
	bool m_bSentryGunVectorValid;
	Vector m_vDispenser;
	bool m_bDispenserVectorValid;
	Vector m_vTeleportExit;
	bool m_bTeleportExitVectorValid;

	// 1 minute wait
	float m_fLastKnownFlagTime;
	float m_fLastKnownTeamFlagTime;

	float m_fBackstabTime;

	TF_Class m_iClass;

	float m_fUpdateClass;
	float m_fUseTeleporterTime;

	bool m_bHasFlag;	
	float m_fSnipeAttackTime;

	// time left before the bot decides if it wants to change class
	float m_fChangeClassTime;
	// bot should check if he can change class now
	bool m_bCheckClass;
	MyEHandle m_pLastCalledMedic;
	CBotLastSee m_pLastSeeMedic;
	/*MyEHandle m_pLastSeeMedic;
	Vector m_vLastSeeMedic;
	float m_fLastSeeMedicTime;*/
	float m_fLastCalledMedicTime;
	bool m_bIsBeingHealed;
	float m_fMedicUpdatePosTime;
	Vector m_vMedicPosition;

	bool m_bCanBeUbered;
	float m_fCheckHealTime;

	float m_fClassDisguiseFitness[10]; // classes disguised as fitness
	float m_fClassDisguiseTime[10];
	float m_fDisguiseTime;
	unsigned short m_iDisguiseClass;
	float m_fSentryPlaceTime;
	unsigned m_iSentryKills;
	float m_fTeleporterEntPlacedTime;
	float m_fTeleporterExtPlacedTime;
	unsigned m_iTeleportedPlayers;

	// list of spies who I saw were attacked by my team-mates recently
	// for use with spy checking
	float m_fSpyAttackedList[RCBOT_MAXPLAYERS];
	// List of spies I saw cloak in front of me
	float m_fSpyLastUncloakedList[RCBOT_MAXPLAYERS];

	int m_iTeam;

	float m_fWaitTurnSentry;			// amount of time to wait before engineer turns their sentry before building

	// currently unused
	float m_fCallMedicTime[RCBOT_MAXPLAYERS]; // for every player ID is kept the last time they called medic

	int m_iLastFailSentryWpt;
	int m_iLastFailTeleExitWpt;

	MyEHandle m_pHealer;

	float m_fHealingMoveTime;

	MyEHandle m_pLastEnemySentry;
	MyEHandle m_NearestEnemyRocket;
	MyEHandle m_NearestEnemyGrenade;

	float m_fLastSentryEnemyTime;
	//bool m_bWantToZoom;
};
//
//
//
//

class CBotTF2 : public CBotFortress
{
public:

	//
	CBotTF2();

	CBotWeapon *getCurrentWeapon() const override;

	void onInventoryApplication() override;

	void MannVsMachineWaveComplete();
	void MannVsMachineAlarmTriggered (const Vector& vLoc);

	bool sentryRecentlyHadEnemy () const;

	void highFivePlayer ( edict_t *pPlayer, float fYaw ) const;

	bool hurt ( edict_t *pAttacker, int iHealthNow, bool bDontHide  = false ) override;

	void updateAttackDefendPoints ();

	void updateAttackPoints ();
	void updateDefendPoints ();

	// found a new enemy
	void enemyFound (edict_t *pEnemy) override;

	void enemyAtIntel ( Vector vPos, int type = EVENT_FLAG_PICKUP, int iArea = -1 );

	bool isTF2 () override { return true; }

	void checkDependantEntities () override;

	bool wantToListenToPlayerAttack ( edict_t *pPlayer, int iWeaponID = -1 ) override;
	bool wantToListenToPlayerFootsteps ( edict_t *pPlayer ) override;

	bool wantToInvestigateSound () override;

	void getDefendArea ( std::vector<int> *m_iAreas );

	void getAttackArea ( std::vector<int> *m_iAreas );

	int getCurrentAttackArea () const { return m_iCurrentAttackArea; }
	int getCurrentDefendArea () const { return m_iCurrentDefendArea; }

	void pointsUpdated ( );

	eBotFuncState rocketJump(int *iState,float *fTime); //TODO: not implemented? [APG]RoboCop[CL]

	bool wantToFollowEnemy () override;

	void resetCloakTime () { m_fSpyCloakTime = 0.0f; }

	float getEnemyFactor ( edict_t *pEnemy ) override;

	void foundSpy (edict_t *pEdict, TF_Class iDisguise) override;

	void touchedWpt ( CWaypoint *pWaypoint, int iNextWaypoint = -1, int iPrevWaypoint = -1 ) override;

	bool handleAttack ( CBotWeapon *pWeapon, edict_t *pEnemy ) override;

	void engiBuildSuccess ( eEngiBuild iBuilding, int index ) override;

	bool lookAfterBuildings (float *fTime) override;

	void spawnInit () override;

	bool setVisible ( edict_t *pEntity, bool bVisible ) override;

	//Vector getAimVector ( edict_t *pEntity );
	void modAim ( edict_t *pEntity, Vector &v_origin, 
				  Vector *v_desired_offset, Vector &v_size,
				  float fDist, float fDist2D) override;

	void modThink () override;

	bool isCloaked () override;

	bool executeAction ( CBotUtility *util );//eBotAction id, CWaypoint *pWaypointResupply, CWaypoint *pWaypointHealth, CWaypoint *pWaypointAmmo );

	void setClass ( TF_Class _class ) override;

	bool isDisguised () override;

	void checkBuildingsValid (bool bForce = false) override;

	edict_t *findEngineerBuiltObject ( eEngiBuild iBuilding, int index ) override;

	bool isEnemy ( edict_t *pEdict,bool bCheckWeapons = true ) override;

	bool isTF () override { return true; }

	void taunt ( bool bOverride = false );

	void callMedic () override;

	void roundReset (bool bFullReset);

	void pointCaptured ( int iPoint, int iTeam, const char *szPointName );

	void engineerBuild ( eEngiBuild iBuilding, eEngiCmd iEngiCmd ) override;

	void spyDisguise (int iTeam, byte iClass) override;

	bool hasEngineerBuilt ( eEngiBuild iBuilding ) override;

	void getTasks ( unsigned iIgnore = 0 ) override;

	void died ( edict_t *pKiller, const char *pszWeapon ) override;

	void killed ( edict_t *pVictim, char *weapon ) override;

	void capturedFlag ();

	void pointCaptured ();

	void waitRemoveSap ();
	
	void roundWon ( int iTeam, bool bFullRound );

	void changeClass ();

	bool needAmmo() override;

	void buildingDestroyed ( int iType, edict_t *pAttacker, edict_t *pEdict );

	TF_Class getClass () override;

	void updateClass () override;

	bool healPlayer ( edict_t* pPlayer, edict_t* pPrevPlayer ) override;
	
	bool upgradeBuilding ( edict_t *pBuilding, bool removesapper = false ) override;

	void setup () override;

	void buildingSapped ( eEngiBuild building, edict_t *pSapper, edict_t *pSpy );

	void sapperDestroyed ( edict_t *pSapper ) const;
	
	bool canGotoWaypoint (const Vector& vPrevWaypoint, CWaypoint* pWaypoint, CWaypoint* pPrev = nullptr) override;

	bool deployStickies ( eDemoTrapType type, const Vector& vStand, const Vector& vLocation, const Vector& vSpread, Vector *vPoint, int *iState, int *iStickyNum, bool *bFail, float *fTime, int wptindex );

	void detonateStickies (bool isJumping = false);

	void setStickyTrapType (const Vector& vLocation, const eDemoTrapType iTrapType) { m_vStickyLocation = vLocation; m_iTrapType = iTrapType; }

	bool canDeployStickies ();

	bool thinkSpyIsEnemy ( edict_t *pEdict, TF_Class iDisguise ) override;

	void seeFriendlyDie ( edict_t *pDied, edict_t *pKiller, CWeapon *pWeapon ) override;
	void seeFriendlyKill ( edict_t *pTeamMate, edict_t *pDied, CWeapon *pWeapon ) override;

	void voiceCommand (byte voiceCmd) override;

	void handleWeapons () override;

	bool select_CWeapon ( CWeapon *pWeapon ) override;
	bool selectBotWeapon ( CBotWeapon *pBotWeapon ) override;

	void checkStuckonSpy ();

	bool checkStuck () override;

	void init (bool bVarInit=false) override;

	bool checkAttackPoint ();

	bool canAvoid ( edict_t *pEntity ) override;

	void hearVoiceCommand ( edict_t *pPlayer, byte voiceCmd ) override;
		
	void checkBeingHealed ( );

	void spyCloak ();

	void spyUnCloak ();

	void healedPlayer ( edict_t *pPlayer, float fAmount );

	void teleportedPlayer ();

	bool isCarrying () const { return m_bIsCarryingObj; }

	void updateCarrying ();

	void resetCarryTime () { m_fCarryTime = engine->Time(); }

	void MvM_Update(); // Mann vs Machine think function!

	bool MvM_IsReady() const;

	void MvM_Upgrade (); // TODO: To allow bots to menuselect in order to buy upgrades? [APG]RoboCop[CL]

private:
	// time for next jump
	float m_fDoubleJumpTime;
	// time bot has taken to sap something
	float m_fSpySapTime;
	// 
	int m_iCurrentDefendArea;
	int m_iCurrentAttackArea;
	//
	//bool m_bBlockPushing;
	//float m_fBlockPushTime;
	//
	MyEHandle m_pDefendPayloadBomb;
	MyEHandle m_pPushPayloadBomb;
	MyEHandle m_pRedPayloadBomb;
	MyEHandle m_pBluePayloadBomb;

	// if demoman has already deployed stickies this is true
	// once the demoman explodes them then this becomes false
	// and it can deploy stickies again
	//bool m_bDeployedStickies;
	eDemoTrapType m_iTrapType;
	int m_iTrapCPIndex;
	Vector m_vStickyLocation;
	float m_fRemoveSapTime;
	float m_fRevMiniGunTime;
	float m_fNextRevMiniGunTime;

	float m_fRevMiniGunBelief;
	float m_fCloakBelief;
	
	//
	MyEHandle m_pCloakedSpy;

	float m_fAttackPointTime; // used in cart maps

	 float m_prevSentryHealth;
	 float m_prevDispHealth;
	 float m_prevTeleExtHealth;
	 float m_prevTeleEntHealth;

	 float m_fDispenserHealAmount;
	 float m_fDispenserPlaceTime;

	 int m_iSentryArea;
	 int m_iDispenserArea;
	 int m_iTeleEntranceArea;
	 int m_iTeleExitArea;

	 eTFVoiceCMD m_nextVoicecmd;

	bool m_bIsCarryingTeleExit;
	bool m_bIsCarryingSentry;
	bool m_bIsCarryingDisp;
	bool m_bIsCarryingTeleEnt;
	bool m_bIsCarryingObj;

	float m_fCarryTime;

	float m_fCheckNextCarrying;

	void *m_pVTable;

	float m_fUseBuffItemTime;

	int m_iDesiredResistType;

	int m_iMvMUpdateTime; // Tick based update time
};

class CBotFF : public CBotFortress
{
public:
	CBotFF() = default;

	void modThink() override;

	bool isEnemy(edict_t* pEdict, bool bCheckWeapons = true) override;

	bool isTF() override { return true; }

};

#endif
