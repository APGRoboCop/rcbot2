#ifndef __RCBOT_GETPROP_H__
#define __RCBOT_GETPROP_H__

#include <engine_wrappers.h>

#include "bot_const.h"

typedef enum : std::uint8_t
{
	TELE_ENTRANCE = 0,
	TELE_EXIT
}eTeleMode;

typedef enum : std::int16_t
{
	GETPROP_UNDEF = (-1),
	GETPROP_TF2SCORE = 0,
	GETPROP_ENTITY_FLAGS = 1,
	GETPROP_TEAM = 2,
	GETPROP_PLAYERHEALTH = 3,
	GETPROP_EFFECTS = 4,
	GETPROP_AMMO = 5,
	GETPROP_TF2_NUMHEALERS = 6,
	GETPROP_TF2_CONDITIONS = 7,
	GETPROP_VELOCITY = 8,
	GETPROP_TF2CLASS = 9,
	GETPROP_TF2SPYMETER = 10, // CTFPlayer::
	GETPROP_TF2SPYDISGUISED_TEAM = 11, //CTFPlayer::m_nDisguiseTeam
	GETPROP_TF2SPYDISGUISED_CLASS = 12, //CTFPlayer::m_nDisguiseClass
	GETPROP_TF2SPYDISGUISED_TARGET = 13, //CTFPlayer::m_iDisguiseTargetIndex - nosoop fix for VScript update
	GETPROP_TF2SPYDISGUISED_DIS_HEALTH = 14, //CTFPlayer::m_iDisguiseHealth
	GETPROP_TF2MEDIGUN_HEALING = 15,
	GETPROP_TF2MEDIGUN_TARGETTING = 16,
	//SETPROP_SET_TICK_BASE,
	GETPROP_TF2TELEPORTERMODE = 17,
	GETPROP_CURRENTWEAPON = 18,
	GETPROP_TF2UBERCHARGE_LEVEL = 19,
	GETPROP_TF2SENTRYHEALTH = 20,
	GETPROP_TF2DISPENSERHEALTH = 21,
	GETPROP_TF2TELEPORTERHEALTH = 22,
	GETPROP_TF2OBJECTCARRIED = 23,
	GETPROP_TF2OBJECTUPGRADELEVEL = 24,
	GETPROP_TF2OBJECTUPGRADEMETAL = 25,
	GETPROP_TF2OBJECTMAXHEALTH = 26,
	GETPROP_TF2DISPMETAL = 27,
	GETPROP_TF2MINIBUILDING = 28,
	GETPROP_MAXSPEED = 29,
	GETPROP_CONSTRAINT_SPEED = 30,
	GETPROP_TF2OBJECTBUILDING = 31,
	GETPROP_HL2DM_PHYSCANNON_ATTACHED = 32,
	GETPROP_HL2DM_PHYSCANNON_OPEN = 33,
	GETPROP_HL2DM_PLAYER_AUXPOWER = 34,
	GETPROP_HL2DM_LADDER_ENT = 35,
	GETPROP_WEAPONLIST = 36,
	GETPROP_WEAPONSTATE = 37,
	GETPROP_WEAPONCLIP1 = 38,
	GETPROP_WEAPONCLIP2 = 39,
	GETPROP_WEAPON_AMMOTYPE1 = 40,
	GETPROP_WEAPON_AMMOTYPE2 = 41,
	GETPROP_DOD_PLAYERCLASS = 42,
	GETPROP_DOD_DES_PLAYERCLASS = 43,
	GETPROP_DOD_STAMINA = 44,
	GETPROP_DOD_PRONE = 45,
	GETPROP_SEQUENCE = 46,
	GETPROP_CYCLE = 47,
	GETPROP_ENTITYFLAGS = 48,
	GETPROP_DOD_CP_NUMCAPS = 49,
	GETPROP_DOD_CP_POSITIONS = 50,
	GETPROP_DOD_CP_ALLIES_REQ_CAP = 51,
	GETPROP_DOD_CP_AXIS_REQ_CAP = 52,
	GETPROP_DOD_CP_NUM_AXIS = 53,
	GETPROP_DOD_CP_NUM_ALLIES = 54,
	GETPROP_DOD_CP_OWNER = 55,
	GETPROP_DOD_SNIPER_ZOOMED = 56,
	GETPROP_DOD_MACHINEGUN_DEPLOYED = 57,
	GETPROP_DOD_ROCKET_DEPLOYED = 58,
	GETPROP_DOD_SEMI_AUTO = 59,
	GETPROP_MOVETYPE = 60,
	GETPROP_DOD_GREN_THROWER = 61,
	GETPROP_DOD_SCORE = 62,
	GETPROP_DOD_OBJSCORE = 63,
	GETPROP_DOD_DEATHS = 64,
	GETPROP_DOD_SMOKESPAWN_TIME = 65,
	GETPROP_DOD_ROUNDTIME = 66,
	GETPROP_DOD_K98ZOOM = 67,
	GETPROP_DOD_GARANDZOOM = 68,
	GETPROP_DOD_ALLIESBOMBING = 69,
	GETPROP_DOD_AXISBOMBING = 70,
	GETPROP_DOD_BOMBSPLANTED = 71,
	GETPROP_DOD_BOMBSREQ = 72,
	GETPROP_DOD_BOMBSDEFUSED = 73,
	GETPROP_DOD_BOMBSREMAINING = 74,
	GETPROP_DOD_PLANTINGBOMB = 75,
	GETPROP_DOD_DEFUSINGBOMB = 76,
	GETPROP_ALL_ENTOWNER = 77,
	GETPROP_DOD_BOMB_STATE = 78,
	GETPROP_DOD_BOMB_TEAM = 79,
	GETPROP_DOD_CP_VISIBLE = 80,
	GETPROP_GROUND_ENTITY = 81,
	GETPROP_ORIGIN = 82,
	GETPROP_TAKEDAMAGE = 83,
	GETPROP_SENTRY_ENEMY = 84,
	GETPROP_WATERLEVEL = 85,
	GETPROP_TF2OBJECTSHELLS = 86,
	GETPROP_TF2OBJECTROCKETS = 87,
	GETPROP_TF2_TELEPORT_RECHARGETIME = 88,
	GETPROP_TF2_TELEPORT_RECHARGEDURATION = 89,
	GETPROP_TF2_OBJTR_m_vCPPositions = 90,
	GETPROP_TF2_OBJTR_m_bCPIsVisible = 91,
	GETPROP_TF2_OBJTR_m_iTeamIcons = 92,
	GETPROP_TF2_OBJTR_m_iTeamOverlays = 93,
	GETPROP_TF2_OBJTR_m_iTeamReqCappers = 94,
	GETPROP_TF2_OBJTR_m_flTeamCapTime = 95,
	GETPROP_TF2_OBJTR_m_iPreviousPoints = 96,
	GETPROP_TF2_OBJTR_m_bTeamCanCap = 97,
	GETPROP_TF2_OBJTR_m_iTeamBaseIcons = 98,
	GETPROP_TF2_OBJTR_m_iBaseControlPoints = 99,
	GETPROP_TF2_OBJTR_m_bInMiniRound = 100,
	GETPROP_TF2_OBJTR_m_iWarnOnCap = 101,
	GETPROP_TF2_OBJTR_m_iCPGroup = 102,
	GETPROP_TF2_OBJTR_m_bCPLocked = 103,
	GETPROP_TF2_OBJTR_m_bTrackAlarm = 104,
	GETPROP_TF2_OBJTR_m_flUnlockTimes = 105,
	GETPROP_TF2_OBJTR_m_flCPTimerTimes = 106,
	GETPROP_TF2_OBJTR_m_iNumTeamMembers = 107,
	GETPROP_TF2_OBJTR_m_iCappingTeam = 108,
	GETPROP_TF2_OBJTR_m_iTeamInZone = 109,
	GETPROP_TF2_OBJTR_m_bBlocked = 110,
	GETPROP_TF2_OBJTR_m_iOwner = 111,
	GETPROP_TF2_OBJTR_m_bCPCapRateScalesWithPlayers = 112,
	GETPROP_TF2_OBJTR_m_iNumControlPoints = 113,
	GETPROP_TF2_OBJTR_m_bPlayingMiniRounds = 114,
	GETPROP_TF2_RNDTM_m_flTimerEndTime = 115,
	GETPROP_TF2_RNDTM_m_nSetupTimeLength = 116,
	GETPROP_TF2_RNDTM_m_bInSetup = 117,
	GETPROP_PIPEBOMB_OWNER = 118,
	GETPROP_TF2_TAUNTYAW = 119,
	GETPROP_TF2_HIGHFIVE = 120,
	GETPROP_TF2_HIGHFIVE_PARTNER = 121,
	GETPROP_SENTRYGUN_PLACING = 122,
	GETPROP_TF2_ISCARRYINGOBJ = 123,
	GETPROP_TF2_GETCARRIEDOBJ = 124,
	GETPROP_TF2_ITEMDEFINITIONINDEX = 125,
	GETPROP_TF2_DISGUISEWEARABLE = 126,
	GETPROP_TF2_RAGEMETER = 127,
	GETPROP_TF2_RAGEDRAINING = 128,
	GETPROP_SIMULATIONTIME = 129,
	GETPROP_TF2_INUPGRADEZONE = 130,
	GETPROP_TF2_ENERGYDRINKMETER = 131,
	GETPROP_TF2_MEDIEVALMODE = 132,
	GETPROP_TF2_ACTIVEWEAPON = 133,
	GETPROP_TF2_BUILDER_TYPE = 134,
	GETPROP_TF2_BUILDER_MODE = 135,
	GETPROP_TF2_CHARGE_RESIST_TYPE = 136,
	GETPROP_TF2_ROUNDSTATE = 137,
	GETPROP_TF2DESIREDCLASS = 138, //Jrob
	GETPROP_SYN_PLAYER_VEHICLE = 139,
	GETPROP_SYN_VEHICLE_DRIVER = 140,
	GETPROP_SYN_SUITPOWER = 141,
	GETPROP_CSS_MONEY = 142,
	GETPROP_CSS_INBUYZONE = 143,
	GETPROP_CSS_INBOMBZONE = 144,
	GETPROP_CSS_INHOSTAGERESCUEZONE = 145,
	GETPROP_CSS_ARMOR = 146,
	GETPROP_CSS_HASDEFUSER = 147,
	GETPROP_CSS_HASHELMET = 148,
	GETPROP_CSS_BOMBTICKING = 149,
	GETPROP_PLAYER_FOV = 150,
	GETPROP_PLAYER_LIFESTATE = 151,
	GETPROP_CSS_HOSTAGE_HEALTH = 152,
	GETPROP_CSS_HOSTAGE_RESCUED = 153,
	GETPROP_CSS_HOSTAGE_LEADER = 154,
	GET_PROPDATA_MAX = 155
} getpropdata_id;

bool UTIL_FindSendPropInfo(const ServerClass *pInfo, const char *szType, unsigned *offset);
ServerClass *UTIL_FindServerClass(const char *name);
void UTIL_FindServerClassPrint(const char*name_cmd);
void UTIL_FindServerClassnamePrint(const char *name_cmd);
void UTIL_FindPropPrint(const char *prop_name);
unsigned UTIL_FindInDataMap(const datamap_t* pMap, const char* name);
datamap_t* CBaseEntity_GetDataDescMap(CBaseEntity* pEntity);
datamap_t* VGetDataDescMap(CBaseEntity* pThisPtr, int offset);

class CClassInterfaceValue
{
public:
	CClassInterfaceValue ()
	{
		m_data = nullptr; 
		m_class = nullptr;
		m_value = nullptr;
		m_offset = 0;
		m_preoffset = 0;
	}

	CClassInterfaceValue (const char *key, const char *value, const unsigned preoffset)
	{
		init(key,value,preoffset);
	}

	void init (const char* key, const char* value, unsigned preoffset = 0);

	void findOffset ( );

	void getData ( void *edict, bool bIsEdict = true );

	edict_t *getEntity ( edict_t *edict );

	CBaseHandle *getEntityHandle ( edict_t *edict );

	bool getBool(void *edict, const bool defaultvalue, const bool bIsEdict = true)
	{ 
		getData(edict, bIsEdict);
		
		if ( !m_data ) 
			return defaultvalue; 
		
		try
		{
			return *static_cast<bool*>(m_data); 
		}

		catch(...)
		{
			return defaultvalue;
		}
	}

	bool *getBoolPointer ( edict_t *edict ) 
	{ 
		getData(edict);  
				
		if ( !m_data ) 
			return nullptr; 

		return static_cast<bool*>(m_data); 
	}

	void *getVoidPointer ( edict_t *edict ) 
	{ 
		getData(edict);  
				
		if ( !m_data ) 
			return nullptr; 

		return m_data; 
	}

	float getFloat ( edict_t *edict, const float defaultvalue ) 
	{ 
		getData(edict); 
		
		if ( !m_data ) 
			return defaultvalue; 
		
		return *static_cast<float*>(m_data); 
	}

	float *getFloatPointer ( edict_t *edict ) 
	{ 
		getData(edict); 
		
		if ( !m_data ) 
			return nullptr; 
		
		return static_cast<float*>(m_data); 
	}

	char *getString (edict_t *edict ) 
	{ 
		getData(edict); 

		return static_cast<char*>(m_data); 
	}

	Vector *getVectorPointer ( edict_t *edict )
	{
		getData(edict);

		if ( m_data )
		{
			return static_cast<Vector*>(m_data);
		}

		return nullptr;
	}

	bool getVector ( edict_t *edict, Vector *v )
	{
		getData(edict);

		if ( m_data )
		{
			static float *x;
			x = static_cast<float*>(m_data);
			*v = Vector(*x,*(x+1),*(x+2));

			return true;
		}

		return false;
	}

	int getInt(void *edict, const int defaultvalue, const bool bIsEdict = true)
	{ 
		getData(edict, bIsEdict);
		
		if ( !m_data ) 
			return defaultvalue; 

		try
		{
			return *static_cast<int*>(m_data);
		}

		catch ( ... )
		{
			return defaultvalue;
		}
	}

	int *getIntPointer ( edict_t *edict ) 
	{ 
		getData(edict); 

		return static_cast<int*>(m_data); 
	}

	byte *getBytePointer ( edict_t *edict ) 
	{ 
		getData(edict); 

		return static_cast<byte*>(m_data); 
	}

	float getFloatFromInt ( edict_t *edict, const float defaultvalue )
	{
		getData(edict); 

		if ( !m_data ) 
			return defaultvalue; 

		return static_cast<float>(*static_cast<int*>(m_data));
	}

	static void resetError () { m_berror = false; }
	static bool isError () { return m_berror; }

	int getOffset() const
	{
		return static_cast<int>(m_offset);
	}
private:
	unsigned m_offset;
	unsigned m_preoffset;
	void *m_data;
	char *m_class;
	char *m_value;

	static bool m_berror;
};


extern CClassInterfaceValue g_GetProps[GET_PROPDATA_MAX];
class CTFObjectiveResource;
class CTeamRoundTimer;
#define DEFINE_GETPROP(id,classname,value,preoffs)\
 g_GetProps[id] = CClassInterfaceValue( CClassInterfaceValue ( classname, value, preoffs ) )

class CClassInterface
{
public:
	static void init ();

	static const char *FindEntityNetClass(int start, const char *classname);
	static edict_t *FindEntityByNetClass(int start, const char *classname);
	static edict_t *FindEntityByNetClassNearest(const Vector& vstart, const char *classname);
	static edict_t *FindEntityByClassnameNearest(const Vector& vstart, const char *classname, float fMinDist = 8192.0f, const edict_t *pOwner = nullptr);

	// TF2
	static int getTF2Score (const edict_t* edict);
	static void setupCTeamRoundTimer ( CTeamRoundTimer *pTimer );
	static float getRageMeter ( edict_t *edict ) { return g_GetProps[GETPROP_TF2_RAGEMETER].getFloat(edict,0); }
	static int getFlags ( edict_t *edict ) { return g_GetProps[GETPROP_ENTITY_FLAGS].getInt(edict,0); }
	static void setFlags ( edict_t *edict, int flags ) {
		int* pFlags = g_GetProps[GETPROP_ENTITY_FLAGS].getIntPointer(edict);
		if (pFlags) *pFlags = flags;
	}
	static void addFlags ( edict_t *edict, int flags ) {
		int* pFlags = g_GetProps[GETPROP_ENTITY_FLAGS].getIntPointer(edict);
		if (pFlags) *pFlags |= flags;
	}
	static int getTeam ( edict_t *edict ) { return g_GetProps[GETPROP_TEAM].getInt(edict,0); }
	static float getPlayerHealth ( edict_t *edict ) { return g_GetProps[GETPROP_PLAYERHEALTH].getFloatFromInt(edict,0); }
	static int getEffects ( edict_t *edict ) { return g_GetProps[GETPROP_EFFECTS].getInt(edict,0); }
	static int *getAmmoList ( edict_t *edict ) { return g_GetProps[GETPROP_AMMO].getIntPointer(edict); }
	//static unsigned findOffset(const char *szType,const char *szClass);
	static int getTF2NumHealers ( edict_t *edict ) { return g_GetProps[GETPROP_TF2_NUMHEALERS].getInt(edict,0); }
	static int getTF2Conditions ( edict_t *edict ) { return g_GetProps[GETPROP_TF2_CONDITIONS].getInt(edict,0); }
	static bool getVelocity ( edict_t *edict, Vector *v ) {return g_GetProps[GETPROP_VELOCITY].getVector(edict,v); }
	static int getTF2Class ( edict_t *edict ) { return g_GetProps[GETPROP_TF2CLASS].getInt(edict,0); }
	static float TF2_getEnergyDrinkMeter(edict_t * edict) { return g_GetProps[GETPROP_TF2_ENERGYDRINKMETER].getFloat(edict, 0); }
	static edict_t *TF2_getActiveWeapon(edict_t *edict) { return g_GetProps[GETPROP_TF2_ACTIVEWEAPON].getEntity(edict); }
	// set weapon
	static void TF2_setActiveWeapon(edict_t* edict, edict_t* pWeapon) //-caxanga334
	{
		CBaseHandle* pHandle = g_GetProps[GETPROP_TF2_ACTIVEWEAPON].getEntityHandle(edict);
		pHandle->Set(pWeapon->GetNetworkable()->GetEntityHandle());
	}

	static void TF2_SetBuilderType(edict_t *pBuilder, const int itype)
	{
		int *pitype = g_GetProps[GETPROP_TF2_BUILDER_TYPE].getIntPointer(pBuilder);

		*pitype = itype;
			//, ]
	}

	static int getChargeResistType(edict_t *pMedigun)
	{
		return g_GetProps[GETPROP_TF2_CHARGE_RESIST_TYPE].getInt(pMedigun, 0);
	}

	static void TF2_SetBuilderMode(edict_t *pBuilder, const int imode)
	{
		int *pitype = g_GetProps[GETPROP_TF2_BUILDER_MODE].getIntPointer(pBuilder);

		*pitype = imode;
		//GETPROP_TF2_BUILDER_MODE, ]
	}
	//Jrob
	static int getTF2DesiredClass(edict_t *edict) { return g_GetProps[GETPROP_TF2DESIREDCLASS].getInt(edict, 0); }

	static void setTF2Class(edict_t *edict, const int _class)
	{
		int* p = g_GetProps[GETPROP_TF2DESIREDCLASS].getIntPointer(edict);
		if (p != nullptr) *p = _class;
	}
	//end Jrob
	static bool TF2_IsMedievalMode(void*gamerules) { return g_GetProps[GETPROP_TF2_MEDIEVALMODE].getBool(gamerules, false, false);}
	static int TF2_getRoundState(void *gamerules) { return g_GetProps[GETPROP_TF2_ROUNDSTATE].getInt(gamerules, 0, false); }
	static float getTF2SpyCloakMeter ( edict_t *edict ) { return g_GetProps[GETPROP_TF2SPYMETER].getFloat(edict,0); }
	static int getWaterLevel ( edict_t *edict ) { return g_GetProps[GETPROP_WATERLEVEL].getInt(edict,0); }

	static void updateSimulationTime ( edict_t *edict )
	{
		if ( float *m_flSimulationTime = g_GetProps[GETPROP_SIMULATIONTIME].getFloatPointer(edict) )
			*m_flSimulationTime = gpGlobals->curtime;
	}

	static bool *getDODCPVisible ( edict_t *pResource ) { return g_GetProps[GETPROP_DOD_CP_VISIBLE].getBoolPointer(pResource); }
	static bool getTF2SpyDisguised( edict_t *edict, int *_class, int *_team, int *_index, int *_health ) 
	{ 
		CClassInterfaceValue::resetError();
		if ( _team )
		*_team = g_GetProps[GETPROP_TF2SPYDISGUISED_TEAM].getInt(edict,0); 

		if ( _class )
		*_class = g_GetProps[GETPROP_TF2SPYDISGUISED_CLASS].getInt(edict,0); 

		if  ( _index )//nosoop fix for VScript update
		{
			const edict_t* hTarget = g_GetProps[GETPROP_TF2SPYDISGUISED_TARGET].getEntity(edict);
			*_index = IndexOfEdict(hTarget);
		}

		if ( _health )
		*_health = g_GetProps[GETPROP_TF2SPYDISGUISED_DIS_HEALTH].getInt(edict,0);

		return !CClassInterfaceValue::isError();
	}

	static int TF2_getItemDefinitionIndex(edict_t *edict)
	{
		return g_GetProps[GETPROP_TF2_ITEMDEFINITIONINDEX].getInt(edict, 0);
	}

	static bool isCarryingObj ( edict_t *edict ) { return g_GetProps[GETPROP_TF2_ISCARRYINGOBJ].getBool(edict,false); }
	static edict_t *getCarriedObj ( edict_t *edict ) { return g_GetProps[GETPROP_TF2_GETCARRIEDOBJ].getEntity(edict); }
	static bool getMedigunHealing ( edict_t *edict ) { return g_GetProps[GETPROP_TF2MEDIGUN_HEALING].getBool(edict,false); }
	static edict_t *getMedigunTarget ( edict_t *edict ) { return g_GetProps[GETPROP_TF2MEDIGUN_TARGETTING].getEntity(edict); }
	static edict_t *getSentryEnemy ( edict_t *edict ) { return g_GetProps[GETPROP_SENTRY_ENEMY].getEntity(edict); }
	static edict_t *getOwner ( edict_t *edict ) { return g_GetProps[GETPROP_ALL_ENTOWNER].getEntity(edict); }
	static bool isMedigunTargetting ( edict_t *pgun, const edict_t *ptarget) { return g_GetProps[GETPROP_TF2MEDIGUN_TARGETTING].getEntity(pgun) == ptarget; }
	//static void setTickBase ( edict_t *edict, int tickbase ) { return ;
	static int isTeleporterMode (edict_t *edict, const eTeleMode mode ) { return g_GetProps[GETPROP_TF2TELEPORTERMODE].getInt(edict,-1) == static_cast<int>(mode); }
	static edict_t *getCurrentWeapon (edict_t *player) { return g_GetProps[GETPROP_CURRENTWEAPON].getEntity(player); }
	static int getUberChargeLevel (edict_t *pWeapon) { return static_cast<int>(g_GetProps[GETPROP_TF2UBERCHARGE_LEVEL].getFloat(pWeapon, 0) * 100.0f); }
	//static void test ();
	static float getSentryHealth ( edict_t *edict ) { return g_GetProps[GETPROP_TF2SENTRYHEALTH].getFloatFromInt(edict,100); }
	static float getDispenserHealth ( edict_t *edict ) { return g_GetProps[GETPROP_TF2DISPENSERHEALTH].getFloatFromInt(edict,100); }
	static float getTeleporterHealth ( edict_t *edict ) { return g_GetProps[GETPROP_TF2TELEPORTERHEALTH].getFloatFromInt(edict,100); }
	static bool isObjectCarried ( edict_t *edict ) { return g_GetProps[GETPROP_TF2OBJECTCARRIED].getBool(edict,false); }
	static int getTF2UpgradeLevel ( edict_t *edict ) { return g_GetProps[GETPROP_TF2OBJECTUPGRADELEVEL].getInt(edict,0); }
	static int getTF2SentryUpgradeMetal ( edict_t *edict ) { return g_GetProps[GETPROP_TF2OBJECTUPGRADEMETAL].getInt(edict,0); }
	static int getTF2SentryShells ( edict_t *edict ) { return g_GetProps[GETPROP_TF2OBJECTSHELLS].getInt(edict,0); }
	static int getTF2SentryRockets ( edict_t *edict ) { return g_GetProps[GETPROP_TF2OBJECTROCKETS].getInt(edict,0); }
	
	static bool getTF2ObjectiveResource ( CTFObjectiveResource *pResource );

	static float getTF2TeleRechargeTime(edict_t *edict) { return g_GetProps[GETPROP_TF2_TELEPORT_RECHARGETIME].getFloat(edict,0); }
	static float getTF2TeleRechargeDuration(edict_t *edict) { return g_GetProps[GETPROP_TF2_TELEPORT_RECHARGEDURATION].getFloat(edict,0); }

	static int getTF2GetBuildingMaxHealth ( edict_t *edict ) { return g_GetProps[GETPROP_TF2OBJECTMAXHEALTH].getInt(edict,0); }
	static int getTF2DispMetal ( edict_t *edict ) { return g_GetProps[GETPROP_TF2DISPMETAL].getInt(edict,0); }
	static bool getTF2BuildingIsMini ( edict_t *edict ) { return g_GetProps[GETPROP_TF2MINIBUILDING].getBool(edict,false); }
	static float getMaxSpeed(edict_t *edict) { return g_GetProps[GETPROP_MAXSPEED].getFloat(edict,0); }
	static float getSpeedFactor(edict_t *edict) { return g_GetProps[GETPROP_CONSTRAINT_SPEED].getFloat(edict,0); }
	static bool isObjectBeingBuilt(edict_t *edict) { return g_GetProps[GETPROP_TF2OBJECTBUILDING].getBool(edict,false); }
	static edict_t *getGroundEntity(edict_t *edict) { return g_GetProps[GETPROP_GROUND_ENTITY].getEntity(edict); }
	static edict_t *gravityGunObject(edict_t *pgun) { return g_GetProps[GETPROP_HL2DM_PHYSCANNON_ATTACHED].getEntity(pgun); }
	static bool gravityGunOpen(edict_t *pgun) { return g_GetProps[GETPROP_HL2DM_PHYSCANNON_OPEN].getBool(pgun,false); }
	static float auxPower (edict_t *player) { return g_GetProps[GETPROP_HL2DM_PLAYER_AUXPOWER].getFloat(player,0);}
	static edict_t *onLadder ( edict_t *player ) { return g_GetProps[GETPROP_HL2DM_LADDER_ENT].getEntity(player);}
	static CBaseHandle *getWeaponList ( edict_t *player ) { return g_GetProps[GETPROP_WEAPONLIST].getEntityHandle(player);}
	static int getWeaponState ( edict_t *pgun ) { return g_GetProps[GETPROP_WEAPONSTATE].getInt(pgun,0); }

	static edict_t *getPipeBombOwner ( edict_t *pPipeBomb ) { return g_GetProps[GETPROP_PIPEBOMB_OWNER].getEntity(pPipeBomb); }

	static int getDODBombState ( edict_t *pBombTarget ) { return g_GetProps[GETPROP_DOD_BOMB_STATE].getInt(pBombTarget,0); }
	static int getDODBombTeam ( edict_t *pBombTarget ) { return g_GetProps[GETPROP_DOD_BOMB_TEAM].getInt(pBombTarget,0); }
	static int *getWeaponClip1Pointer ( edict_t *pgun ) { return g_GetProps[GETPROP_WEAPONCLIP1].getIntPointer(pgun); }
	static int *getWeaponClip2Pointer ( edict_t *pgun ) { return g_GetProps[GETPROP_WEAPONCLIP2].getIntPointer(pgun); }
	static int getOffset(const int id) { return g_GetProps[id].getOffset(); }
	static void getWeaponClip ( edict_t *pgun, int *iClip1, int *iClip2 ) { *iClip1 = g_GetProps[GETPROP_WEAPONCLIP1].getInt(pgun,0); *iClip2 = g_GetProps[GETPROP_WEAPONCLIP2].getInt(pgun,0); }
	static void getAmmoTypes ( edict_t *pgun, int *iAmmoType1, int *iAmmoType2 ) { *iAmmoType1 = g_GetProps[GETPROP_WEAPON_AMMOTYPE1].getInt(pgun,-1); *iAmmoType2 = g_GetProps[GETPROP_WEAPON_AMMOTYPE2].getInt(pgun,-1);}

	static int getPlayerClassDOD(edict_t *player) { return g_GetProps[GETPROP_DOD_PLAYERCLASS].getInt(player,0); }

	static void getPlayerInfoDOD(edict_t *player, bool *m_bProne, float *m_flStamina)
	{
		*m_bProne = g_GetProps[GETPROP_DOD_PRONE].getBool(player,false);
		if ( m_flStamina )
			*m_flStamina = g_GetProps[GETPROP_DOD_STAMINA].getFloat(player,0);
	}

	static float getAnimCycle ( edict_t *edict) 
	{	
		return g_GetProps[GETPROP_CYCLE].getFloat(edict,0);
	}

	static void getAnimatingInfo ( edict_t *edict, float *flCycle, int *iSequence ) 
	{	
		*flCycle = g_GetProps[GETPROP_CYCLE].getFloat(edict,0);
		*iSequence = g_GetProps[GETPROP_SEQUENCE].getInt(edict,false);
	}

	static int getPlayerFlags (edict_t *player) { return g_GetProps[GETPROP_ENTITYFLAGS].getInt(player,0);}
	static int *getPlayerFlagsPointer (edict_t *player) { return g_GetProps[GETPROP_ENTITYFLAGS].getIntPointer(player);}

	static int getDODNumControlPoints ( edict_t *pResource )
	{
		return g_GetProps[GETPROP_DOD_CP_NUMCAPS].getInt(pResource,0);
	}

	static Vector *getOrigin ( edict_t *pPlayer )
	{
		return g_GetProps[GETPROP_ORIGIN].getVectorPointer(pPlayer);
	}

	static void setOrigin ( edict_t *pPlayer, const Vector& vOrigin )
	{
		Vector *vEntOrigin = g_GetProps[GETPROP_ORIGIN].getVectorPointer(pPlayer);

		*vEntOrigin = vOrigin;
	}

	static Vector *getDODCP_Positions ( edict_t *pResource )
	{
		return g_GetProps[GETPROP_DOD_CP_POSITIONS].getVectorPointer(pResource);
	}

	static void getDODFlagInfo (edict_t *pResource, int **m_iNumAxis, int **m_iNumAllies, int **m_iOwner, int **m_iNumAlliesReq, int **m_iNumAxisReq )
	{
		*m_iNumAxis = g_GetProps[GETPROP_DOD_CP_NUM_AXIS].getIntPointer(pResource);
		*m_iNumAllies = g_GetProps[GETPROP_DOD_CP_NUM_ALLIES].getIntPointer(pResource);
		*m_iOwner = g_GetProps[GETPROP_DOD_CP_OWNER].getIntPointer(pResource);
		*m_iNumAlliesReq = g_GetProps[GETPROP_DOD_CP_ALLIES_REQ_CAP].getIntPointer(pResource);
		*m_iNumAxisReq = g_GetProps[GETPROP_DOD_CP_AXIS_REQ_CAP].getIntPointer(pResource);
	}

	static void 	getDODBombInfo ( edict_t *pResource, bool **m_bBombPlanted, int **m_iBombsRequired, int **m_iBombsRemaining, bool **m_bBombBeingDefused)
	{
		*m_bBombPlanted = g_GetProps[GETPROP_DOD_BOMBSPLANTED].getBoolPointer(pResource);
		*m_iBombsRequired = g_GetProps[GETPROP_DOD_BOMBSREQ].getIntPointer(pResource);
		*m_iBombsRemaining = g_GetProps[GETPROP_DOD_BOMBSREMAINING].getIntPointer(pResource);
		*m_bBombBeingDefused = g_GetProps[GETPROP_DOD_BOMBSDEFUSED].getBoolPointer(pResource);
	}

	static float getTF2TauntYaw ( edict_t *edict ) { return g_GetProps[GETPROP_TF2_TAUNTYAW].getFloat(edict,0); }
	static bool getTF2HighFiveReady ( edict_t *edict ) { return g_GetProps[GETPROP_TF2_HIGHFIVE].getBool(edict,false); }
	static edict_t *getHighFivePartner ( edict_t *edict ) { return g_GetProps[GETPROP_TF2_HIGHFIVE_PARTNER].getEntity(edict); }

	static int getDesPlayerClassDOD(edict_t *player) { return g_GetProps[GETPROP_DOD_DES_PLAYERCLASS].getInt(player,0); }

	static bool isSniperWeaponZoomed (edict_t *weapon) { return g_GetProps[GETPROP_DOD_SNIPER_ZOOMED].getBool(weapon,false); }
	static bool isMachineGunDeployed (edict_t *weapon) { return g_GetProps[GETPROP_DOD_MACHINEGUN_DEPLOYED].getBool(weapon,false); }
	static bool isRocketDeployed ( edict_t *weapon ) { return g_GetProps[GETPROP_DOD_ROCKET_DEPLOYED].getBool(weapon,false); }

	static bool isMoveType ( edict_t *pent, const int movetype )
	{
		return (g_GetProps[GETPROP_MOVETYPE].getInt(pent,0) & 15) == movetype;
	}

	static byte getTakeDamage ( edict_t *pent )
	{
		return static_cast<byte>(g_GetProps[GETPROP_TAKEDAMAGE].getInt(pent, 0));
	}

	static byte *getTakeDamagePointer ( edict_t *pent )
	{
		return g_GetProps[GETPROP_TAKEDAMAGE].getBytePointer(pent);
	}

	static int getMoveType ( edict_t *pent )
	{
		return g_GetProps[GETPROP_MOVETYPE].getInt(pent,0) & 15;
	}

	static byte *getMoveTypePointer ( edict_t *pent )
	{
		return g_GetProps[GETPROP_MOVETYPE].getBytePointer(pent);
	}

	static edict_t *getGrenadeThrower ( edict_t *gren )
	{
		return g_GetProps[GETPROP_DOD_GREN_THROWER].getEntity(gren);
	}

	static int getPlayerScoreDOD (edict_t* resource, const edict_t* pPlayer)
	{
		const int *score_array = g_GetProps[GETPROP_DOD_SCORE].getIntPointer(resource);

		return score_array != nullptr ? score_array[static_cast<std::size_t>(ENTINDEX(pPlayer))] : 0;
	}

	static int getPlayerObjectiveScoreDOD (edict_t* resource, const edict_t* pPlayer)
	{
		const int *score_array = g_GetProps[GETPROP_DOD_OBJSCORE].getIntPointer(resource);

		return score_array != nullptr ? score_array[static_cast<std::size_t>(ENTINDEX(pPlayer))] : 0;
	}

	static int getPlayerDeathsDOD (edict_t* resource, const edict_t* pPlayer)
	{
		const int *score_array = g_GetProps[GETPROP_DOD_DEATHS].getIntPointer(resource);

		return score_array != nullptr ? score_array[static_cast<std::size_t>(ENTINDEX(pPlayer))] : 0;
	}

	static float getSmokeSpawnTime ( edict_t *pSmoke )
	{
		return g_GetProps[GETPROP_DOD_SMOKESPAWN_TIME].getFloat(pSmoke,0);
	}

	static float getRoundTime ( edict_t *pGamerules )
	{
		return g_GetProps[GETPROP_DOD_ROUNDTIME].getFloat(pGamerules,0);
	}

	static bool isGarandZoomed ( edict_t *pGarand )
	{
		return g_GetProps[GETPROP_DOD_GARANDZOOM].getBool(pGarand,false);
	}

	static bool isK98Zoomed( edict_t *pK98 )
	{
		return g_GetProps[GETPROP_DOD_K98ZOOM].getBool(pK98,false);
	}
	// HL2DM
	//static void 

	static bool areAlliesBombing (edict_t *pRes) 
	{
		return g_GetProps[GETPROP_DOD_ALLIESBOMBING].getBool(pRes,false);
	}

	static bool areAxisBombing (edict_t *pRes) 
	{
		return g_GetProps[GETPROP_DOD_AXISBOMBING].getBool(pRes,false);
	}

	static int *isBombPlantedList (edict_t *pRes) 
	{
		return g_GetProps[GETPROP_DOD_BOMBSPLANTED].getIntPointer(pRes);
	}

	static int *getNumBombsRequiredList (edict_t *pRes) 
	{
		return g_GetProps[GETPROP_DOD_BOMBSREQ].getIntPointer(pRes);
	}

	static int *isBombDefusingList (edict_t *pRes) 
	{
		return g_GetProps[GETPROP_DOD_BOMBSDEFUSED].getIntPointer(pRes);
	}

	static int *getNumBombsRemaining ( edict_t *pRes )
	{
		return g_GetProps[GETPROP_DOD_BOMBSREMAINING].getIntPointer(pRes);
	}

	static bool isPlayerDefusingBomb_DOD(edict_t *pPlayer)
	{
		return g_GetProps[GETPROP_DOD_DEFUSINGBOMB].getBool(pPlayer,false);
	}

	static bool isPlayerPlantingBomb_DOD(edict_t *pPlayer)
	{
		return g_GetProps[GETPROP_DOD_PLANTINGBOMB].getBool(pPlayer,false);
	}

	static bool isSentryGunBeingPlaced (edict_t *pSentry )
	{
		return g_GetProps[GETPROP_SENTRYGUN_PLACING].getBool(pSentry,false);
	}

	/**
	 * Gets the player FOV
	 * 
	 * @param pPlayer	The player to retreive FOV from
	 * @return			The player's FOV
	 **/
	static int getPlayerFOV(edict_t *pPlayer)
	{
		return g_GetProps[GETPROP_PLAYER_FOV].getInt(pPlayer, 0);
	}

	/**
	 * Gets the player life state
	 * 
	 * @param pPlayer	The player to retreive life state from
	 * @return			The player's life state
	 **/
	static int getPlayerLifeState(edict_t *pPlayer)
	{
		return g_GetProps[GETPROP_PLAYER_LIFESTATE].getInt(pPlayer, 0);
	}

	// Synergy

	/**
	 * Gets the player vehicle entity
	 * 
	 * @param pPlayer	The player to retreive the vehicle from
	 * @return			The player's vehicle
	 **/
	static edict_t* getSynPlayerVehicle(edict_t* pPlayer)
	{
		return g_GetProps[GETPROP_SYN_PLAYER_VEHICLE].getEntity(pPlayer);
	}

	/**
	 * Gets the vehicle current driver
	 * 
	 * @param pVehicle	The vehicle to get the driver from
	 * @return			The vehicle current driver
	 **/
	static edict_t* getSynVehicleDriver(edict_t* pVehicle)
	{
		return g_GetProps[GETPROP_SYN_VEHICLE_DRIVER].getEntity(pVehicle);
	}	

	/**
	 * Gets the player HEV suit power level
	 * 
	 * @param pPlayer	The player to retreive the suit power from
	 * @return			The player's current suit power level
	 **/
	static float getSynPlayerSuitPower(edict_t* pPlayer)
	{
		return g_GetProps[GETPROP_SYN_SUITPOWER].getFloat(pPlayer, 0.0f);
	}

	// Counter-Strike: Source

	/**
	 * Gets the amount of money the player has
	 * 
	 * @param pPlayer	The player's pointer
	 * @return			The player's current money value
	 **/
	static int getCSPlayerMoney(edict_t* pPlayer)
	{
		return g_GetProps[GETPROP_CSS_MONEY].getInt(pPlayer, 0);
	}

	/**
	 * Checks if the player is inside a buy zone
	 * 
	 * @param pPlayer	The player's pointer
	 * @return			TRUE if the player is inside a buy zone
	 **/
	static bool isCSPlayerInBuyZone(edict_t* pPlayer)
	{
		return g_GetProps[GETPROP_CSS_INBUYZONE].getBool(pPlayer, false);
	}

	/**
	 * Checks if the player is inside a bomb zone
	 * 
	 * @param pPlayer	The player's pointer
	 * @return			TRUE if the player is inside a bomb zone
	 **/
	static bool isCSPlayerInBombZone(edict_t* pPlayer)
	{
		return g_GetProps[GETPROP_CSS_INBOMBZONE].getBool(pPlayer, false);
	}

	/**
	 * Checks if the player is inside a hostage rescue zone
	 * 
	 * @param pPlayer	The player's pointer
	 * @return			TRUE if the player is inside a hostage rescue zone
	 **/
	static bool isCSPlayerInHostageRescueZone(edict_t* pPlayer)
	{
		return g_GetProps[GETPROP_CSS_INHOSTAGERESCUEZONE].getBool(pPlayer, false);
	}

	/**
	 * Gets the amount of armor a player has
	 * 
	 * @param pPlayer	The player's pointer
	 * @return			The player's current armor value
	 **/
	static int getCSPlayerArmor(edict_t* pPlayer)
	{
		return g_GetProps[GETPROP_CSS_ARMOR].getInt(pPlayer, 0);
	}

	/**
	 * Checks if the player has a defuse kit
	 * 
	 * @param pPlayer	The player's pointer
	 * @return			TRUE if the player has a defuse kit
	 **/
	static bool CSPlayerHasDefuser(edict_t* pPlayer)
	{
		return g_GetProps[GETPROP_CSS_HASDEFUSER].getBool(pPlayer, false);
	}

	/**
	 * Checks if the player has helmet armor
	 * 
	 * @param pPlayer	The player's pointer
	 * @return			TRUE if the player has helmet armor
	 **/
	static bool CSPlayerHasHelmet(edict_t* pPlayer)
	{
		return g_GetProps[GETPROP_CSS_HASHELMET].getBool(pPlayer, false);
	}

	/**
	 * Checks if the bomb is ticking
	 * 
	 * @param pBomb		The planted bomb entity
	 * @return			TRUE if the bomb is ticking (returns FALSE is the bomb was defused)
	 **/
	static bool isCSBombTicking(edict_t* pBomb)
	{
		return g_GetProps[GETPROP_CSS_BOMBTICKING].getBool(pBomb, false);
	}

	/**
	 * Checks how much health a specific hostage has
	 * 
	 * @param pHostage		The hostage entity edict
	 * @return				How much health the given hostage has
	 **/
	static int getCSHostageHealth(edict_t* pHostage)
	{
		return g_GetProps[GETPROP_CSS_HOSTAGE_HEALTH].getInt(pHostage, 0);
	}

	/**
	 * Checks if the specified hostage has been rescued
	 * 
	 * @param pHostage		The hostage entity edict
	 * @return				TRUE if the given hostage has been rescued
	 **/
	static bool isCSHostageRescued(edict_t* pHostage)
	{
		return g_GetProps[GETPROP_CSS_HOSTAGE_RESCUED].getBool(pHostage, false);
	}

	/**
	 * Gets the edict of the player currently leading the hostage
	 * 
	 * @param pHostage		The hostage entity edict
	 * @return				Edict pointer of the player leading the hostage
	 **/
	static edict_t* getCSHostageLeader(edict_t* pHostage)
	{
		return g_GetProps[GETPROP_CSS_HOSTAGE_LEADER].getEntity(pHostage);
	}

private:
	static CClassInterfaceValue g_GetProps[GET_PROPDATA_MAX];

};

// For reference: https://github.com/alliedmodders/sourcemod/blob/master/core/smn_entities.cpp
class CDataInterface
{
public:
	/**
	* Gets the entity's given prop via datamaps
	* 
	* @param pEntity	The entity to read
	* @param prop		The property to read
	* @return			The property value
	**/
	static int GetEntPropInt(CBaseEntity *pEntity, const char *prop)
	{
		const datamap_t* pDataMap = CBaseEntity_GetDataDescMap(pEntity);
		const unsigned offset = UTIL_FindInDataMap(pDataMap, prop);
		const int propvalue = *reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(pEntity) + offset); // TODO: bit count?
		return propvalue;
	}

	/**
	* Gets the entity's given prop via datamaps
	* 
	* @param pEntity	The entity to read
	* @param prop		The property to read
	* @return			The property value
	**/
	static float GetEntPropFloat(CBaseEntity *pEntity, const char *prop)
	{
		const datamap_t* pDataMap = CBaseEntity_GetDataDescMap(pEntity);
		const unsigned offset = UTIL_FindInDataMap(pDataMap, prop);
		const float propvalue = *reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(pEntity) + offset);
		return propvalue;
	}

	/**
	* Gets the entity's given prop via datamaps
	* 
	* @param pEntity	The entity to read
	* @param prop		The property to read
	* @return			The property value or NULL if the edict is invalid
	**/
	static edict_t *GetEntPropEdict(CBaseEntity *pEntity, const char *prop)
	{
		const datamap_t* pDataMap = CBaseEntity_GetDataDescMap(pEntity);
		const unsigned offset = UTIL_FindInDataMap(pDataMap, prop);
		edict_t *pEdict = *reinterpret_cast<edict_t**>(reinterpret_cast<uint8_t*>(pEntity) + offset);
		if(!pEdict || pEdict->IsFree())
		{
			return nullptr;
		}
			
		return pEdict;
	}

	/**
	* Gets the entity's given prop via datamaps
	* 
	* @param pEntity	The entity to read
	* @param prop		The property to read
	* @return			The property value
	**/
	static Vector *GetEntPropVector(CBaseEntity *pEntity, const char *prop)
	{
		const datamap_t* pDataMap = CBaseEntity_GetDataDescMap(pEntity);
		const unsigned offset = UTIL_FindInDataMap(pDataMap, prop);
		Vector *propvalue = reinterpret_cast<Vector*>(reinterpret_cast<uint8_t*>(pEntity) + offset);
		return propvalue;
	}

	/**
	* Gets the entity's health via datamaps
	* 
	* @param pEntity	The entity to get the health from
	* @return			The entity current health (m_iHealth) value
	**/
	static int GetEntityHealth(CBaseEntity* pEntity)
	{
		const datamap_t* pDataMap = CBaseEntity_GetDataDescMap(pEntity);
		const unsigned offset = UTIL_FindInDataMap(pDataMap, "m_iHealth");
		const int iHealth = *reinterpret_cast<int*>(reinterpret_cast<char*>(pEntity) + offset);
		return iHealth;
	}
	/**
	* Gets the entity's maxhealth via datamaps
	* 
	* @param pEntity	The entity to get the health from
	* @return			The entity current max health (m_iMaxHealth) value
	**/
	static int GetEntityMaxHealth(CBaseEntity* pEntity)
	{
		const datamap_t* pDataMap = CBaseEntity_GetDataDescMap(pEntity);
		const unsigned offset = UTIL_FindInDataMap(pDataMap, "m_iMaxHealth");
		const int iMaxHealth = *reinterpret_cast<int*>(reinterpret_cast<char*>(pEntity) + offset);
		return iMaxHealth;
	}
	/**
	* Gets the entity's health as percentage
	* 
	* @param pEntity	The entity to get the health from
	* @return			The entity health as percentage, range 1 to 0, where 1 is full health
	**/
	static float GetEntityHealthPercent(CBaseEntity* pEntity)
	{
		const datamap_t* pDataMap = CBaseEntity_GetDataDescMap(pEntity);
		const unsigned offset = UTIL_FindInDataMap(pDataMap, "m_iHealth");
		//int offset2 = UTIL_FindInDataMap(pDataMap, "m_iMaxHealth"); //unused? [APG]RoboCop[CL]
		const int iHealth = *reinterpret_cast<int*>(reinterpret_cast<char*>(pEntity) + offset);
		const int iMaxHealth = *reinterpret_cast<int*>(reinterpret_cast<char*>(pEntity) + offset);
		return static_cast<float>(iHealth) / static_cast<float>(iMaxHealth);
	}
};

#endif