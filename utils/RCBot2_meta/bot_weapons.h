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
#ifndef __BOT_WEAPONS_H__
#define __BOT_WEAPONS_H__

#include <vector>
#include <cstdint>

extern const char* g_szTF2Weapons[];

#include "shareddefs.h"

class CBot;

extern int m_TF2AmmoIndices[];

typedef struct
{
	int iSlot;
	int iId;
	const char* szWeaponName;
	int m_iFlags;
	float minPrimDist;
	float maxPrimDist;
	int m_iAmmoIndex;
	int m_iPreference;
	float m_fProjSpeed;
}WeaponsData_t;

enum : std::uint8_t
{
	TF2_WEAPON_BAT = 0,
	TF2_WEAPON_BOTTLE, //1
	TF2_WEAPON_FIREAXE, //2
	TF2_WEAPON_CLUB, //3
	TF2_WEAPON_KNIFE, //4
	TF2_WEAPON_FISTS, //5
	TF2_WEAPON_SHOVEL, //6
	TF2_WEAPON_WRENCH,//7
	TF2_WEAPON_BONESAW,//8
	TF2_WEAPON_SHOTGUN_PRIMARY,//9
	TF2_WEAPON_SHOTGUN_SOLDIER,//10
	TF2_WEAPON_SHOTGUN_HWG,//11
	TF2_WEAPON_SHOTGUN_PYRO,//12
	TF2_WEAPON_SCATTERGUN,//13
	TF2_WEAPON_SNIPERRIFLE,//14
	TF2_WEAPON_MINIGUN,//15
	TF2_WEAPON_SMG,//16
	TF2_WEAPON_SYRINGEGUN,//17
	TF2_WEAPON_ROCKETLAUNCHER,//18
	TF2_WEAPON_GRENADELAUNCHER,//19
	TF2_WEAPON_PIPEBOMBS,//20
	TF2_WEAPON_FLAMETHROWER,//21
	TF2_WEAPON_PISTOL,//22
	TF2_WEAPON_PISTOL_SCOUT,//23
	TF2_WEAPON_REVOLVER,//24
	TF2_WEAPON_POMSON6000, //TF2_WEAPON_PDA_ENGI_BUILD,//25
	TF2_WEAPON_SHOTGUN,//TF2_WEAPON_PDA_ENGI_DESTROY,//26
	TF2_WEAPON_FRONTIERJUSTICE,//TF2_WEAPON_PDA_SPY,//27
	TF2_WEAPON_BUILDER,//28
	TF2_WEAPON_MEDIGUN,//29
	TF2_WEAPON_INVIS,//30
	TF2_WEAPON_FLAREGUN,//31
	TF2_WEAPON_BUFF_ITEM,//32
	TF2_WEAPON_SAXXY,//33
	TF2_WEAPON_SENTRYGUN,//34
	TF2_WEAPON_BAT_WOOD,//=44,
	TF2_WEAPON_LUNCHBOX_DRINK,//=46,
	TF2_WEAPON_BOW,//=56,
	TF2_WEAPON_JAR,//=58,
	TF2_WEAPON_DIRECTHIT,//=127,
	TF2_WEAPON_SWORD,//=132,
	TF2_WEAPON_BAT_FISH,//=221,
	TF2_WEAPON_KATANA,//=357,
	TF2_WEAPON_COWMANGLER,
	TF2_WEAPON_CROSSBOW,
	TF2_WEAPON_CLEAVER,
	TF2_WEAPON_BAT_GIFTWRAP,
	TF2_WEAPON_RAYGUN,
	TF2_WEAPON_SNIPERRIFLE_DECAP,
	TF2_WEAPON_MAX
};
/*
enum
{
	TF2_WEAPON_BAT = 0,
	TF2_WEAPON_BONESAW,
	TF2_WEAPON_BOTTLE,
	TF2_WEAPON_BUILDER,
	TF2_WEAPON_CLUB,
	TF2_WEAPON_FIREAXE,
	TF2_WEAPON_FISTS,
	TF2_WEAPON_FLAMETHROWER,
	TF2_WEAPON_GRENADELAUNCHER,
	TF2_WEAPON_INVIS,
	TF2_WEAPON_KNIFE,
	TF2_WEAPON_MEDIGUN,
	TF2_WEAPON_MINIGUN,
	TF2_WEAPON_OBJECTSSELECTION,
	TF2_WEAPON_PDA_ENGI_BUILD,
	TF2_WEAPON_PDA_ENGI_DESTROY,
	TF2_WEAPON_PDA_SPY,
	TF2_WEAPON_PIPEBOMBS,
	TF2_WEAPON_PISTOL,
	TF2_WEAPON_PISTOL_SCOUT,
	TF2_WEAPON_REVOLVER,
	TF2_WEAPON_ROCKETLAUNCHER,
	TF2_WEAPON_SCATTERGUN,
	TF2_WEAPON_SHOTGUN_HWG,
	TF2_WEAPON_SHOTGUN_PRIMARY,
	TF2_WEAPON_SHOTGUN_PYRO,
	TF2_WEAPON_SHOTGUN_SOLDIER,
	TF2_WEAPON_SHOVEL,
	TF2_WEAPON_SMG,
	TF2_WEAPON_SNIPERRIFLE,
	TF2_WEAPON_SYRINGEGUN,
	TF2_WEAPON_WRENCH,
	TF2_WEAPON_ENGIDESTROY,
	TF2_WEAPON_ENGIBUILD,
	TF2_WEAPON_SENTRYGUN,
	TF2_WEAPON_MAX
};
*/
enum : std::uint8_t
{
	HL2DM_WEAPON_PISTOL = 0,
	HL2DM_WEAPON_CROWBAR,
	HL2DM_WEAPON_357,
	HL2DM_WEAPON_SMG1,
	HL2DM_WEAPON_AR2,
	HL2DM_WEAPON_FRAG,
	HL2DM_WEAPON_STUNSTICK,
	HL2DM_WEAPON_CROSSBOW,
	HL2DM_WEAPON_RPG,
	HL2DM_WEAPON_SLAM,
	HL2DM_WEAPON_SHOTGUN,
	HL2DM_WEAPON_PHYSCANNON,
	HL2DM_WEAPON_MAX
};

//TODO: Add Black Mesa weapons support [APG]RoboCop[CL]
/*enum : std::uint8_t
{
	BMS_WEAPON_GLOCK = 0,
	BMS_WEAPON_CROWBAR,
	BMS_WEAPON_PYTHON,
	BMS_WEAPON_MP5,
	BMS_WEAPON_CHAINGUN,
	BMS_WEAPON_CROSSBOW,
	BMS_WEAPON_SHOTGUN,
	BMS_WEAPON_RPG,
	BMS_WEAPON_TAU,
	BMS_WEAPON_GLUON,
	BMS_WEAPON_HORNETGUN,
	BMS_WEAPON_HANDGRENADE,
	BMS_WEAPON_TRIPMINE,
	BMS_WEAPON_SATCHEL,
	BMS_WEAPON_SNARK
};*/

enum : std::uint8_t
{
	DOD_WEAPON_AMERKNIFE = 0,
	DOD_WEAPON_SPADE,
	DOD_WEAPON_COLT,
	DOD_WEAPON_P38,
	DOD_WEAPON_M1,
	DOD_WEAPON_C96,
	DOD_WEAPON_GARAND,
	DOD_WEAPON_K98,
	DOD_WEAPON_THOMPSON,
	DOD_WEAPON_MP40,
	DOD_WEAPON_BAR,
	DOD_WEAPON_MP44,
	DOD_WEAPON_SPRING,
	DOD_WEAPON_K98_SCOPED,
	DOD_WEAPON_20CAL,
	DOD_WEAPON_MG42,
	DOD_WEAPON_BAZOOKA,
	DOD_WEAPON_PSCHRECK,
	DOD_WEAPON_RIFLEGREN_US,
	DOD_WEAPON_RIFLEGREN_GER,
	DOD_WEAPON_FRAG_US,
	DOD_WEAPON_FRAG_GER,
	DOD_WEAPON_SMOKE_US,
	DOD_WEAPON_SMOKE_GER,
	DOD_WEAPON_BOMB,
	DOD_WEAPON_MAX
};

enum : std::uint8_t
{
	SYN_WEAPON_PISTOL = 0,
	SYN_WEAPON_CROWBAR,
	SYN_WEAPON_LEADPIPE,
	SYN_WEAPON_357,
	SYN_WEAPON_DESERTEAGLE,
	SYN_WEAPON_SMG1,
	SYN_WEAPON_MP5K,
	SYN_WEAPON_AR2,
	SYN_WEAPON_FRAG,
	SYN_WEAPON_STUNSTICK,
	SYN_WEAPON_CROSSBOW,
	SYN_WEAPON_RPG,
	SYN_WEAPON_SLAM,
	SYN_WEAPON_SHOTGUN,
	SYN_WEAPON_PHYSCANNON,
	SYN_WEAPON_MG1,
	SYN_WEAPON_BUGBAIT,
	SYN_WEAPON_MAX
};

enum : std::uint8_t
{
	CS_WEAPON_KNIFE = 0,
	CS_WEAPON_USP,
	CS_WEAPON_GLOCK,
	CS_WEAPON_228C,
	CS_WEAPON_FIVESEVEN,
	CS_WEAPON_ELITES,
	CS_WEAPON_DEAGLE,
	CS_WEAPON_SUPERSHOTGUN, // M3 pump action shotgun
	CS_WEAPON_AUTOSHOTGUN, // XM1014 auto shotgun
	CS_WEAPON_TMP,
	CS_WEAPON_MAC10,
	CS_WEAPON_MP5,
	CS_WEAPON_UMP45,
	CS_WEAPON_P90,
	CS_WEAPON_FAMAS,
	CS_WEAPON_GALIL,
	CS_WEAPON_AK47,
	CS_WEAPON_M4A1,
	CS_WEAPON_AUG,
	CS_WEAPON_SG552, // Krieg 552 (T Rifle)
	CS_WEAPON_SCOUT,
	CS_WEAPON_AWP,
	CS_WEAPON_SG550, // CT Auto Sniper
	CS_WEAPON_G3SG1, // T Auto Sniper
	CS_WEAPON_M249,
	CS_WEAPON_HE_GRENADE,
	CS_WEAPON_FLASH_GRENADE,
	CS_WEAPON_SMOKE_GRENADE,
	CS_WEAPON_C4,
	CS_WEAPON_MAX
};

enum
{
	WEAP_FL_NONE = 0,
	WEAP_FL_PRIM_ATTACK = 1 << 0,
	WEAP_FL_SEC_ATTACK = 1 << 1,
	WEAP_FL_EXPLOSIVE = 1 << 2,	// weapon is an explosive weapon eg. rpg
	WEAP_FL_MELEE = 1 << 3,	//
	WEAP_FL_UNDERWATER = 1 << 4,	// weapon can be used under water
	WEAP_FL_HOLDATTACK = 1 << 5,	// weapon must hold attack (e.g. minigun)
	WEAP_FL_SPECIAL = 1 << 6,	//
	WEAP_FL_KILLPIPEBOMBS = 1 << 7,	// weapon can destroy pipe bombs (tf2)
	WEAP_FL_DEFLECTROCKETS = 1 << 8,	// weapon can deflect rocekts (tf2)
	WEAP_FL_GRAVGUN = 1 << 9,	// weapon is a grav gun
	WEAP_FL_EXPLOSIVE_SEC = 1 << 10,	// weapon has an explosive secondary attack
	WEAP_FL_ZOOMABLE = 1 << 11,	// weapon can be zoomed
	WEAP_FL_DEPLOYABLE = 1 << 12,	// weapon can be deployed
	WEAP_FL_MELEE_SEC_ATT = 1 << 13,	// weapon has a melee secondary attack
	WEAP_FL_FIRE_SELECT = 1 << 14,	// weapon can choose fire mode
	WEAP_FL_CANTFIRE_NORM = 1 << 15,	// weapon can't be fired normally, needs to be zoomed/deployed
	WEAP_FL_GRENADE = 1 << 16,
	WEAP_FL_HIGH_RECOIL = 1 << 17,	// can't be fired at long distance, but ok when deployed
	WEAP_FL_SCOPE = 1 << 18,	// has a scope . i.e. sniper rifle
	WEAP_FL_PROJECTILE = 1 << 19 // affected by gravity
};

extern std::vector<WeaponsData_t> TF2Weaps;
extern std::vector<WeaponsData_t> HL2DMWeaps;
extern std::vector<WeaponsData_t> DODWeaps;
extern std::vector<WeaponsData_t> BMSWeaps;
extern std::vector<WeaponsData_t> SYNERGYWeaps;
extern std::vector<WeaponsData_t> CSSWeaps;

class CWeapon
{
public:
	CWeapon(const WeaponsData_t* data)
	{
		m_iSlot = data->iSlot;
		setID(data->iId);
		setName(data->szWeaponName);

		setFlags(data->m_iFlags);

		// shoot distance (default)
		m_fPrimMinWeaponShootDist = data->minPrimDist;
		m_fPrimMaxWeaponShootDist = data->maxPrimDist;

		m_fSecMinWeaponShootDist = 0.0f;
		m_fSecMaxWeaponShootDist = 512.0f;

		m_fProjectileSpeed = data->m_fProjSpeed;

		m_iAmmoIndex1 = data->m_iAmmoIndex;
		m_iAmmoIndex2 = -1;

		m_iPreference = data->m_iPreference;
	}

	/*CWeapon( int iSlot, const char *szWeaponName, int iId, int iFlags = 0, int iAmmoIndex = -1, float minPrim =0.0f, float maxPrim = 4096.0f, int iPref = 0, int iAmmoIndex2 = -1 )
	{
		m_iSlot = iSlot;
		setID(iId);
		setName(szWeaponName);

		setFlags(iFlags);

		// shoot distance (default)
		m_fPrimMinWeaponShootDist = minPrim;
		m_fPrimMaxWeaponShootDist = maxPrim;

		m_fSecMinWeaponShootDist = 0.0f;
		m_fSecMaxWeaponShootDist = 512.0f;
		m_iAmmoIndex1 = iAmmoIndex;
		m_iAmmoIndex2 = iAmmoIndex2;

		m_iPreference = iPref;
	}*/

	void setName(const char* szWeaponName)
	{
		m_szWeaponName = szWeaponName;
	}

	bool isWeaponName(const char* szWeaponName) const
	{
		return !strcmp(szWeaponName, getWeaponName());
	}

	bool isShortWeaponName(const char* szWeaponName) const
	{
		if (std::strlen(szWeaponName) > std::strlen(m_szWeaponName))
			return false;

		const std::size_t start = std::strlen(m_szWeaponName) - std::strlen(szWeaponName);

		return std::strcmp(&m_szWeaponName[start], szWeaponName) == 0;
	}

	bool canDestroyPipeBombs() const
	{
		return hasAllFlags(WEAP_FL_KILLPIPEBOMBS);
	}

	bool isScoped() const
	{
		return hasAllFlags(WEAP_FL_SCOPE);
	}

	bool canDeflectRockets() const
	{
		return hasAllFlags(WEAP_FL_DEFLECTROCKETS);
	}

	void setID(const int iId)
	{
		m_iWeaponId = iId;
	}

	void setFlags(const int iFlags)
	{
		m_iFlags = iFlags;
	}

	bool primaryInRange(const float fDistance) const
	{
		return fDistance > m_fPrimMinWeaponShootDist && fDistance < m_fPrimMaxWeaponShootDist;
	}

	bool primaryGreaterThanRange(const float fDistance) const
	{
		return fDistance < m_fPrimMaxWeaponShootDist;
	}

	float primaryMaxRange() const
	{
		return m_fPrimMaxWeaponShootDist;
	}

	bool hasHighRecoil() const
	{
		return hasAllFlags(WEAP_FL_HIGH_RECOIL);
	}

	bool isZoomable() const
	{
		return hasAllFlags(WEAP_FL_ZOOMABLE);
	}

	bool isProjectile() const
	{
		return hasAllFlags(WEAP_FL_PROJECTILE);
	}

	bool isExplosive() const
	{
		return hasAllFlags(WEAP_FL_EXPLOSIVE);
	}

	bool isDeployable() const
	{
		return hasAllFlags(WEAP_FL_DEPLOYABLE);
	}

	bool canUseUnderWater() const
	{
		return hasAllFlags(WEAP_FL_UNDERWATER);
	}

	bool isGravGun() const
	{
		return hasAllFlags(WEAP_FL_GRAVGUN);
	}

	bool mustHoldAttack() const
	{
		return hasAllFlags(WEAP_FL_HOLDATTACK);
	}

	bool isGrenade() const
	{
		return hasAllFlags(WEAP_FL_GRENADE);
	}

	bool isMelee() const
	{
		return hasAllFlags(WEAP_FL_MELEE);
	}

	bool isMeleeSecondary() const
	{
		return hasAllFlags(WEAP_FL_MELEE_SEC_ATT);
	}

	bool needsDeployedOrZoomed() const
	{
		return hasAllFlags(WEAP_FL_CANTFIRE_NORM);
	}

	bool canAttack() const
	{
		return hasAllFlags(WEAP_FL_PRIM_ATTACK);
	}

	bool isSpecial() const
	{
		return hasAllFlags(WEAP_FL_SPECIAL);
	}

	bool secondaryInRange(const float fDistance) const
	{
		return fDistance > m_fSecMinWeaponShootDist && fDistance < m_fSecMaxWeaponShootDist;
	}

	int getPreference() const
	{
		return m_iPreference;
	}

	const char* getWeaponName() const
	{
		return m_szWeaponName;
	}

	int getID() const
	{
		return m_iWeaponId;
	}

	void setPrimaryRange(const float fMinRange, const float fMaxRange)
	{
		m_fPrimMinWeaponShootDist = fMinRange;
		m_fPrimMaxWeaponShootDist = fMaxRange;
	}

	void setSecondaryRange(const float fMinRange, const float fMaxRange)
	{
		m_fSecMinWeaponShootDist = fMinRange;
		m_fSecMaxWeaponShootDist = fMaxRange;
	}

	int getAmmoIndex1() const
	{
		return m_iAmmoIndex1;
	}

	int getAmmoIndex2() const
	{
		return m_iAmmoIndex2;
	}

	int getSlot() const
	{
		return m_iSlot;
	}

	void setAmmoIndex(const int iAmmoIndex1, const int iAmmoIndex2 = -1)
	{
		m_iAmmoIndex1 = iAmmoIndex1;
		m_iAmmoIndex2 = iAmmoIndex2;
	}

	bool canUseSecondary() const
	{
		return hasSomeFlags(WEAP_FL_SEC_ATTACK);
	}

	float getProjectileSpeed() const
	{
		return m_fProjectileSpeed;
	}

private:
	bool hasAllFlags(const int iFlags) const
	{
		return (m_iFlags & iFlags) == iFlags;
	}

	bool hasSomeFlags(const int iFlags) const
	{
		return (m_iFlags & iFlags) > 0;
	}

	const char* m_szWeaponName; // classname

	int m_iWeaponId;			// identification
	int m_iFlags;				// flags
	int m_iAmmoIndex1;
	int m_iAmmoIndex2;
	int m_iPreference;
	int m_iSlot;

	// shoot distance
	float m_fPrimMinWeaponShootDist;
	float m_fPrimMaxWeaponShootDist;

	float m_fSecMinWeaponShootDist;
	float m_fSecMaxWeaponShootDist;

	float m_fProjectileSpeed;
};

class IWeaponFunc;

class CWeapons
{
public:
	CWeapons()
	{
		m_theWeapons.clear();
	}

	static void addWeapon(CWeapon* pWeapon) { m_theWeapons.emplace_back(pWeapon); }

	static CWeapon* getWeapon(int iId);

	static CWeapon* getWeapon(const char* szWeapon);

	static CWeapon* getWeaponByShortName(const char* szWeapon);

	static void eachWeapon(IWeaponFunc* pFunc);

	static void freeMemory();

	static edict_t* findWeapon(edict_t* pPlayer, const char* szWeaponName);

	static void loadWeapons(const char* szWeaponListName, const WeaponsData_t* pDefault);

private:
	// available weapons in game
	static std::vector<CWeapon*> m_theWeapons;
};

enum : std::uint8_t
{
	AMMO_PRIM = 1,
	AMMO_SEC = 2
};

////////////////////////////////////////////////////////////
// Weapon but with bot holding it and ammo information etc
////////////////////////////////////////////////////////////
class CBotWeapon
{
public:
	CBotWeapon()
	{
		m_pWeaponInfo = nullptr;
		m_bHasWeapon = false;
		m_iWeaponIndex = 0;
		m_pEnt = nullptr;
		m_iClip1 = nullptr;
		m_iClip2 = nullptr;
	}

	void setWeapon(CWeapon* pWeapon)
	{
		m_pWeaponInfo = pWeapon;
	}

	float getPrimaryMaxRange() const
	{
		return m_pWeaponInfo->primaryMaxRange();
	}

	bool primaryInRange(const float fDistance) const
	{
		return m_pWeaponInfo->primaryInRange(fDistance);
	}

	bool primaryGreaterThanRange(const float fDistance) const
	{
		return m_pWeaponInfo->primaryGreaterThanRange(fDistance);
	}

	bool needsDeployedOrZoomed() const
	{
		return m_pWeaponInfo->needsDeployedOrZoomed();
	}

	bool isGravGun() const
	{
		return m_pWeaponInfo->isGravGun();
	}

	bool isExplosive() const
	{
		return m_pWeaponInfo->isExplosive();
	}

	bool isZoomable() const
	{
		return m_pWeaponInfo->isZoomable();
	}

	bool canUseUnderWater() const
	{
		return m_pWeaponInfo->canUseUnderWater();
	}

	bool hasHighRecoil() const
	{
		return m_pWeaponInfo->hasHighRecoil();
	}

	int getID() const
	{
		return m_pWeaponInfo->getID();
	}

	bool isSpecial() const
	{
		return m_pWeaponInfo->isSpecial();
	}

	float primaryMaxRange() const
	{
		return m_pWeaponInfo->primaryMaxRange();
	}

	bool isDeployable() const
	{
		return m_pWeaponInfo->isDeployable();
	}

	bool mustHoldAttack() const
	{
		return m_pWeaponInfo->mustHoldAttack();
	}

	bool canDestroyPipeBombs() const
	{
		return m_pWeaponInfo->canDestroyPipeBombs();
	}

	bool isProjectile() const
	{
		return m_pWeaponInfo->isProjectile();
	}

	float getProjectileSpeed() const
	{
		return m_pWeaponInfo->getProjectileSpeed();
	}

	bool canDeflectRockets() const
	{
		return m_pWeaponInfo->canDeflectRockets();
	}

	bool canUseSecondary() const
	{
		return m_pWeaponInfo->canUseSecondary();
	}

	bool isMelee() const
	{
		return m_pWeaponInfo->isMelee();
	}

	bool isMeleeSecondary() const
	{
		return m_pWeaponInfo->isMeleeSecondary();
	}

	bool secondaryInRange(const float fDistance) const
	{
		return m_pWeaponInfo->secondaryInRange(fDistance);
	}

	int getPreference() const
	{
		return m_pWeaponInfo->getPreference();
	}

	bool outOfAmmo(const CBot* pBot) const;

	bool needToReload(const CBot* pBot) const;

	void setHasWeapon(const bool bHas)
	{
		m_bHasWeapon = bHas;
	}

	bool hasWeapon() const
	{
		return m_bHasWeapon;
	}

	bool canAttack() const
	{
		return m_pWeaponInfo->canAttack();
	}

	int getAmmo(const CBot* pBot, int type = AMMO_PRIM) const;

	int getClip1(CBot* pBot) const
	{
		if (m_iClip1)
			return *m_iClip1;

		return 0;
	}

	int getClip2(CBot* pBot) const
	{
		if (m_iClip2)
			return *m_iClip2;

		return 0;
	}

	CWeapon* getWeaponInfo() const { return m_pWeaponInfo; }

	const char* getWeaponName() const
	{
		if (m_pWeaponInfo)
			return m_pWeaponInfo->getWeaponName();
		return nullptr;
	}

	int getWeaponIndex() const { return m_iWeaponIndex; }

	void setWeaponIndex(const int iIndex) { m_iWeaponIndex = iIndex; } // Entity Index

	void setWeaponEntity(edict_t* pent, bool bOverrideAmmoTypes = true);

	edict_t* getWeaponEntity() const { return m_pEnt; }

private:

	// link to weapon info
	CWeapon* m_pWeaponInfo;

	int m_iWeaponIndex;

	bool m_bHasWeapon;

	edict_t* m_pEnt;

	int* m_iClip1;
	int* m_iClip2;
};

// Weapons that
class CBotWeapons
{
public:
	/////////////////////////////////////
	CBotWeapons(CBot* pBot);    // // constructor
	/////////////////////////////////////
	CBotWeapon* getBestWeapon(edict_t* pEnemy, bool bAllowMelee = true, bool bAllowMeleeFallback = true, bool bMeleeOnly = false, bool bExplosivesOnly = false, bool bIgnorePrimaryMinimum = false);

	CBotWeapon* addWeapon(CWeapon* pWeaponInfo, int iId, edict_t* pent, bool bOverrideAll = true);

	CBotWeapon* getWeapon(const CWeapon* pWeapon);

	CBotWeapon* getActiveWeapon(const char* szWeaponName, edict_t* pWeaponUpdate = nullptr, bool bOverrideAmmoTypes = true);

	CBotWeapon* getCurrentWeaponInSlot(int iSlot);

	CBotWeapon* getGrenade()
	{
		for (CBotWeapon& m_theWeapon : m_theWeapons)
		{
			if (m_theWeapon.hasWeapon())
			{
				if (m_theWeapon.getWeaponInfo())
				{
					if (m_theWeapon.getWeaponInfo()->isGrenade())
						return &m_theWeapon;
				}
			}
		}

		return nullptr;
	}

	void clearWeapons();

	bool hasWeapon(int id) const;

	bool hasExplosives() const;

	// returns true if there is a change to the weapons
	bool update(bool bOverrideAllFromEngine = true); // update from sendprop

	CBotWeapon* getPrimaryWeapon(); // return the most important weapon bot is holding if if out o ammo
	void resetSignature() { m_iWeaponsSignature = 0; }
private:
	// bot that has these weapons
	CBot* m_pBot;

	// checksum mask of weapons bot already has so we know if we need to update or not
	std::uintptr_t m_iWeaponsSignature;

	// weapons local to the bot only
	// (holds ammo/preference etc and link to actual weapon)
	CBotWeapon m_theWeapons[MAX_WEAPONS];//[MAX_WEAPONS];

	float m_fUpdateWeaponsTime;
};

#endif