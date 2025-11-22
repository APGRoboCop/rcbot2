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

#include "bot.h"
#include "bot_getprop.h"
#include "bot_globals.h"
#include "bot_weapons.h"

#include <cmath>
#include <cstring>
#include <cstdio>
#include <memory>
#include <string>
#include <algorithm>
#include <array>
#include <vector>

#include "rcbot/logging.h"

constexpr std::array<const char*, 25> g_szDODWeapons = {
	"weapon_amerknife",
	"weapon_spade",
	"weapon_colt",
	"weapon_p38",
	"weapon_m1carbine",
	"weapon_c96",
	"weapon_garand",
	"weapon_k98",
	"weapon_thompson",
	"weapon_mp40",
	"weapon_bar",
	"weapon_mp44",
	"weapon_spring",
	"weapon_k98_scoped",
	"weapon_30cal",
	"weapon_mg42",
	"weapon_bazooka",
	"weapon_pschreck",
	"weapon_riflegren_us",
	"weapon_riflegren_ger",
	"weapon_frag_us",
	"weapon_frag_ger",
	"weapon_smoke_us",
	"weapon_smoke_ger",
	"weapon_basebomb"
};

constexpr std::array<const char*, 12> g_szHL2DMWeapons = {
	"weapon_pistol",
	"weapon_crowbar",
	"weapon_357",
	"weapon_smg1",
	"weapon_ar2",
	"weapon_frag",
	"weapon_stunstick",
	"weapon_crossbow",
	"weapon_rpg",
	"weapon_slam",
	"weapon_shotgun",
	"weapon_physcannon"
};

//TODO: Add Black Mesa weapons support [APG]RoboCop[CL]
/*constexpr std::array<const char*, 15> g_szBMSWeapons = {
	"weapon_357",
	"weapon_assassin_glock",
	"weapon_crossbow",
	"weapon_crowbar",
	"weapon_frag",
	"weapon_glock",
	"weapon_gluon",
	"weapon_hivehand",
	"weapon_mp5",
	"weapon_rpg",
	"weapon_satchel",
	"weapon_shotgun",
	"weapon_snark",
	"weapon_tau",
	"weapon_tripmine"
};*/

constexpr std::array<const char*, 17> g_szSYNWeapons = {
	"weapon_pistol", // 0
	"weapon_crowbar",
	"weapon_pipe",
	"weapon_357",
	"weapon_deagle",
	"weapon_smg1", // 5
	"weapon_mp5k",
	"weapon_ar2",
	"weapon_frag",
	"weapon_stunstick",
	"weapon_crossbow", // 10
	"weapon_rpg",
	"weapon_slam",
	"weapon_shotgun",
	"weapon_physcannon",
	"weapon_mg1", // 15
	"weapon_bugbait"
};

constexpr std::array<const char*, 29> g_szCSWeapons = {
	"weapon_knife", // 0
	"weapon_usp",
	"weapon_glock",
	"weapon_p228",
	"weapon_fiveseven",
	"weapon_elite", // 5
	"weapon_deagle",
	"weapon_m3",
	"weapon_xm1014",
	"weapon_tmp",
	"weapon_mac10", // 10
	"weapon_mp5navy",
	"weapon_ump45",
	"weapon_p90",
	"weapon_famas",
	"weapon_galil", // 15
	"weapon_ak47",
	"weapon_m4a1",
	"weapon_aug",
	"weapon_sg552",
	"weapon_scout", // 20
	"weapon_awp",
	"weapon_sg550",
	"weapon_g3sg1",
	"weapon_m249",
	"weapon_hegrenade", // 25
	"weapon_flashbang",
	"weapon_smokegrenade",
	"weapon_c4"
};

/*  0, 0, 1, 2, 6, 3,
  4, 5, 8, 8, 9, 8,
  7, 5, 10, 11, 12, 12,
  21, 22, 13, 14, 17, 18*/

std::vector<WeaponsData_t> DODWeaps = {

	// slot, id , weapon name, flags, min dist, max dist, ammo index, preference
	{1,DOD_WEAPON_AMERKNIFE, g_szDODWeapons[0],	WEAP_FL_PRIM_ATTACK | WEAP_FL_MELEE | WEAP_FL_UNDERWATER,0,100,-1,1,0},
	{1,DOD_WEAPON_SPADE, g_szDODWeapons[1],		WEAP_FL_PRIM_ATTACK | WEAP_FL_MELEE | WEAP_FL_UNDERWATER,0,100,-1,1,0},
	{2,DOD_WEAPON_COLT, g_szDODWeapons[2],		WEAP_FL_PRIM_ATTACK | WEAP_FL_UNDERWATER,0,800,1,2,0},
	{2,DOD_WEAPON_P38, g_szDODWeapons[3],		WEAP_FL_PRIM_ATTACK | WEAP_FL_UNDERWATER,0,800,2,2,0},
	{3,DOD_WEAPON_M1, g_szDODWeapons[4],		WEAP_FL_PRIM_ATTACK,0,1600,6,4,0},
	{3,DOD_WEAPON_C96, g_szDODWeapons[5],		WEAP_FL_PRIM_ATTACK,0,1600,-1,4,0},
	{3,DOD_WEAPON_GARAND, g_szDODWeapons[6],	WEAP_FL_PRIM_ATTACK | WEAP_FL_ZOOMABLE,0,1600,-1,3,0},
	{3,DOD_WEAPON_K98, g_szDODWeapons[7],		WEAP_FL_PRIM_ATTACK | WEAP_FL_ZOOMABLE,0,1600,-1,3,0},
	{3,DOD_WEAPON_THOMPSON, g_szDODWeapons[8],	WEAP_FL_PRIM_ATTACK | WEAP_FL_MELEE_SEC_ATT,0,900,-1,3,0},
	{3,DOD_WEAPON_MP40, g_szDODWeapons[9],		WEAP_FL_PRIM_ATTACK | WEAP_FL_MELEE_SEC_ATT,0,1600,-1,4,0},
	{3,DOD_WEAPON_BAR, g_szDODWeapons[10],		WEAP_FL_PRIM_ATTACK,0,1600,-1,3,0},
	{3,DOD_WEAPON_MP44, g_szDODWeapons[11],		WEAP_FL_PRIM_ATTACK,0,1600,-1,3,0},
	{3,DOD_WEAPON_SPRING, g_szDODWeapons[12],	WEAP_FL_PRIM_ATTACK | WEAP_FL_SCOPE | WEAP_FL_CANTFIRE_NORM | WEAP_FL_ZOOMABLE,0,3200,-1,3,0},
	{3,DOD_WEAPON_K98_SCOPED, g_szDODWeapons[13],	WEAP_FL_PRIM_ATTACK | WEAP_FL_SCOPE | WEAP_FL_CANTFIRE_NORM | WEAP_FL_ZOOMABLE,0,3200,-1,4,0},
	{3,DOD_WEAPON_20CAL, g_szDODWeapons[14],		WEAP_FL_PRIM_ATTACK | WEAP_FL_DEPLOYABLE | WEAP_FL_HIGH_RECOIL,0,2000,-1,4,0},
	{3,DOD_WEAPON_MG42, g_szDODWeapons[15],			WEAP_FL_PRIM_ATTACK | WEAP_FL_DEPLOYABLE | WEAP_FL_HIGH_RECOIL,0,2000,-1,4,0},
	{3,DOD_WEAPON_BAZOOKA, g_szDODWeapons[16],		WEAP_FL_PROJECTILE | WEAP_FL_EXPLOSIVE | WEAP_FL_PRIM_ATTACK | WEAP_FL_CANTFIRE_NORM | WEAP_FL_DEPLOYABLE,500,3200,-1,5,1300},
	{3,DOD_WEAPON_PSCHRECK, g_szDODWeapons[17],		WEAP_FL_PROJECTILE | WEAP_FL_EXPLOSIVE | WEAP_FL_PRIM_ATTACK | WEAP_FL_CANTFIRE_NORM | WEAP_FL_DEPLOYABLE,500,3200,-1,5,1300},
	{3,DOD_WEAPON_RIFLEGREN_US, g_szDODWeapons[18],	WEAP_FL_EXPLOSIVE_SEC | WEAP_FL_PRIM_ATTACK,500,1800,-1,4,0},
	{3,DOD_WEAPON_RIFLEGREN_GER, g_szDODWeapons[19],	WEAP_FL_EXPLOSIVE_SEC | WEAP_FL_PRIM_ATTACK,500,1800,-1,4,0},
	{3,DOD_WEAPON_FRAG_US, g_szDODWeapons[20],		WEAP_FL_PROJECTILE | WEAP_FL_GRENADE | WEAP_FL_EXPLOSIVE | WEAP_FL_NONE,0,1200,-1,1,0},
	{3,DOD_WEAPON_FRAG_GER, g_szDODWeapons[21],		WEAP_FL_PROJECTILE | WEAP_FL_GRENADE | WEAP_FL_EXPLOSIVE | WEAP_FL_NONE,0,1200,-1,1,0},
	{3,DOD_WEAPON_SMOKE_US, g_szDODWeapons[22],		WEAP_FL_PROJECTILE | WEAP_FL_GRENADE,0,1200,-1,1,0},
	{3,DOD_WEAPON_SMOKE_GER, g_szDODWeapons[23],	WEAP_FL_PROJECTILE | WEAP_FL_GRENADE,0,1200,-1,1,0},
	{3,DOD_WEAPON_BOMB, g_szDODWeapons[24], WEAP_FL_NONE,0,0,-1,1,0},
	{ 0, 0, "\0", 0, 0, 0, 0, 0, 0 }//signal last weapon
};


std::vector<WeaponsData_t> HL2DMWeaps = {

	// slot, id , weapon name, flags, min dist, max dist, ammo index, preference
	{2,HL2DM_WEAPON_PISTOL,		g_szHL2DMWeapons[0],	WEAP_FL_PRIM_ATTACK | WEAP_FL_UNDERWATER,0,1000,-1,1,0},
	{1,HL2DM_WEAPON_CROWBAR,	g_szHL2DMWeapons[1],	WEAP_FL_PRIM_ATTACK | WEAP_FL_MELEE | WEAP_FL_UNDERWATER,0,128,-1,1,0},
	{2,HL2DM_WEAPON_357,		g_szHL2DMWeapons[2],	WEAP_FL_PRIM_ATTACK,0,768,-1,2,0},
	{3,HL2DM_WEAPON_SMG1,		g_szHL2DMWeapons[3],	WEAP_FL_PRIM_ATTACK | WEAP_FL_SEC_ATTACK,0,1400,-1,2,0},
	{2,HL2DM_WEAPON_AR2,		g_szHL2DMWeapons[4],	WEAP_FL_PRIM_ATTACK | WEAP_FL_SEC_ATTACK,0,1400,-1,3,0},
	{1,HL2DM_WEAPON_FRAG,		g_szHL2DMWeapons[5],	WEAP_FL_GRENADE | WEAP_FL_EXPLOSIVE,0,180,-1,1,0},
	{2,HL2DM_WEAPON_STUNSTICK,	g_szHL2DMWeapons[6],	WEAP_FL_PRIM_ATTACK | WEAP_FL_MELEE | WEAP_FL_UNDERWATER,0,128,-1,1,0},
	{3,HL2DM_WEAPON_CROSSBOW,	g_szHL2DMWeapons[7],	WEAP_FL_PRIM_ATTACK | WEAP_FL_SCOPE | WEAP_FL_UNDERWATER,0,2000,-1,2,0},
	{2,HL2DM_WEAPON_RPG,		g_szHL2DMWeapons[8],	WEAP_FL_PROJECTILE | WEAP_FL_EXPLOSIVE | WEAP_FL_PRIM_ATTACK | WEAP_FL_UNDERWATER,400,2000,-1,3,1000.0f},
	{1,HL2DM_WEAPON_SLAM,		g_szHL2DMWeapons[9],	WEAP_FL_EXPLOSIVE,0,180,-1,1,0},
	{2,HL2DM_WEAPON_SHOTGUN,	g_szHL2DMWeapons[10],	WEAP_FL_PRIM_ATTACK,0,768,-1,2,0},
	{1,HL2DM_WEAPON_PHYSCANNON,	g_szHL2DMWeapons[11],	WEAP_FL_GRAVGUN | WEAP_FL_PRIM_ATTACK,0,768,-1,4,0},
	{ 0, 0, "\0", 0, 0, 0, 0, 0, 0 }//signal last weapon
};

//SENTRYGUN ID = 34
//TODO: Add Black Mesa weapons support [APG]RoboCop[CL]
/*std::vector<WeaponsData_t> BMSWeaps = {

	// slot, id , weapon name, flags, min dist, max dist, ammo index, preference
	{2,BMS_WEAPON_GLOCK,		g_szBMSWeapons[0],	WEAP_FL_PRIM_ATTACK | WEAP_FL_UNDERWATER,0,1000,-1,1,0},
	{1,BMS_WEAPON_CROWBAR,	g_szBMSWeapons[1],	WEAP_FL_PRIM_ATTACK | WEAP_FL_MELEE | WEAP_FL_UNDERWATER,0,128,-1,1,0},
	{2,BMS_WEAPON_PYTHON,		g_szBMSWeapons[2],	WEAP_FL_PRIM_ATTACK,0,768,-1,2,0},
	{3,BMS_WEAPON_MP5,		g_szBMSWeapons[3],	WEAP_FL_PRIM_ATTACK | WEAP_FL_SEC_ATTACK,0,1400,-1,2,0},
	//Chaingun?
	{3,BMS_WEAPON_CROSSBOW,	g_szBMSWeapons[5],	WEAP_FL_PRIM_ATTACK | WEAP_FL_SCOPE | WEAP_FL_UNDERWATER,0,2000,-1,2,0},
	{3,BMS_WEAPON_SHOTGUN,	g_szBMSWeapons[6],	WEAP_FL_PRIM_ATTACK,0,768,-1,2,0},
	{4,BMS_WEAPON_RPG,		g_szBMSWeapons[7],	WEAP_FL_PROJECTILE | WEAP_FL_EXPLOSIVE | WEAP_FL_PRIM_ATTACK | WEAP_FL_UNDERWATER,400,2000,-1,3,1000.0f},
	{4,BMS_WEAPON_TAU,		g_szBMSWeapons[8],	WEAP_FL_PRIM_ATTACK | WEAP_FL_SEC_ATTACK,0,1400,-1,3,0},
	{4,BMS_WEAPON_GLUON,		g_szBMSWeapons[9],	WEAP_FL_PRIM_ATTACK | WEAP_FL_SEC_ATTACK,0,1400,-1,3,0},
	//Hornetgun?
	{5,BMS_WEAPON_HANDGRENADE,		g_szBMSWeapons[11],	WEAP_FL_GRENADE | WEAP_FL_EXPLOSIVE,0,180,-1,1,0},
	//Tripmine?
	{5,BMS_WEAPON_SATCHEL,		g_szBMSWeapons[13],	WEAP_FL_EXPLOSIVE,0,180,-1,1,0},
	//Snark?
	{ 0, 0, "\0", 0, 0, 0, 0, 0, 0 }//signal last weapon
};*/

std::vector<WeaponsData_t> TF2Weaps = {

	// slot, id , weapon name, flags, min dist, max dist, ammo index, preference
	{TF2_SLOT_MELEE,TF2_WEAPON_BAT,		"tf_weapon_bat",	WEAP_FL_PRIM_ATTACK | WEAP_FL_MELEE | WEAP_FL_UNDERWATER,0,180,0,1,0},
	{TF2_SLOT_MELEE,TF2_WEAPON_BOTTLE,		"tf_weapon_bottle",	WEAP_FL_PRIM_ATTACK | WEAP_FL_MELEE | WEAP_FL_UNDERWATER,0,180,0,1,0},
	{TF2_SLOT_MELEE,TF2_WEAPON_FIREAXE,			"tf_weapon_fireaxe",	WEAP_FL_PRIM_ATTACK | WEAP_FL_MELEE | WEAP_FL_UNDERWATER,0,180,0,1,0},
	{TF2_SLOT_MELEE,TF2_WEAPON_CLUB,				"tf_weapon_club",	WEAP_FL_PRIM_ATTACK | WEAP_FL_MELEE | WEAP_FL_UNDERWATER,0,180,0,1,0},
	{TF2_SLOT_MELEE,TF2_WEAPON_KNIFE,				"tf_weapon_knife",	WEAP_FL_PRIM_ATTACK | WEAP_FL_MELEE | WEAP_FL_UNDERWATER,0,220,0,2,0},
	{TF2_SLOT_MELEE,TF2_WEAPON_FISTS,				"tf_weapon_fists",	WEAP_FL_PRIM_ATTACK | WEAP_FL_MELEE | WEAP_FL_UNDERWATER,0,180,0,1,0},
	{TF2_SLOT_MELEE,TF2_WEAPON_SHOVEL,				"tf_weapon_shovel",	WEAP_FL_PRIM_ATTACK | WEAP_FL_MELEE | WEAP_FL_UNDERWATER,0,180,0,1,0},
	{TF2_SLOT_MELEE,TF2_WEAPON_WRENCH,				"tf_weapon_wrench",	WEAP_FL_PRIM_ATTACK | WEAP_FL_MELEE | WEAP_FL_UNDERWATER,0,180,3,1,0},
	{TF2_SLOT_MELEE,TF2_WEAPON_BONESAW,			"tf_weapon_bonesaw",	WEAP_FL_PRIM_ATTACK | WEAP_FL_MELEE | WEAP_FL_UNDERWATER,0,180,0,1,0},
	{TF2_SLOT_PRMRY,TF2_WEAPON_SHOTGUN_PRIMARY,	"tf_weapon_shotgun_primary",	WEAP_FL_KILLPIPEBOMBS | WEAP_FL_PRIM_ATTACK | WEAP_FL_UNDERWATER,0,800,1,2,0},
	{TF2_SLOT_SCNDR,TF2_WEAPON_SHOTGUN_SOLDIER,	"tf_weapon_shotgun_soldier",	WEAP_FL_KILLPIPEBOMBS | WEAP_FL_PRIM_ATTACK | WEAP_FL_UNDERWATER,0,500,2,2,0},
	{TF2_SLOT_SCNDR,TF2_WEAPON_SHOTGUN_HWG,		"tf_weapon_shotgun_hwg",	WEAP_FL_KILLPIPEBOMBS | WEAP_FL_PRIM_ATTACK | WEAP_FL_UNDERWATER,0,800,2,2,0},
	{TF2_SLOT_SCNDR,TF2_WEAPON_SHOTGUN_PYRO,		"tf_weapon_shotgun_pyro",	WEAP_FL_KILLPIPEBOMBS | WEAP_FL_PRIM_ATTACK | WEAP_FL_UNDERWATER,0,800,2,2,0},
	{TF2_SLOT_PRMRY,TF2_WEAPON_SCATTERGUN,			"tf_weapon_scattergun",	WEAP_FL_KILLPIPEBOMBS | WEAP_FL_PRIM_ATTACK | WEAP_FL_UNDERWATER,0,800,1,3,0},
	{TF2_SLOT_PRMRY,TF2_WEAPON_SNIPERRIFLE,		"tf_weapon_sniperrifle",	WEAP_FL_SCOPE | WEAP_FL_PRIM_ATTACK,1000,4000,1,3,0},
	{TF2_SLOT_PRMRY,TF2_WEAPON_SNIPERRIFLE_DECAP, "tf_weapon_sniperrifle_decap", WEAP_FL_SCOPE | WEAP_FL_PRIM_ATTACK, 1000, 4000, 1, 3, 0 },
	{TF2_SLOT_PRMRY,TF2_WEAPON_MINIGUN,			"tf_weapon_minigun",	WEAP_FL_PRIM_ATTACK | WEAP_FL_HOLDATTACK,120,1800,1,3,0},
	{TF2_SLOT_SCNDR,TF2_WEAPON_SMG,				"tf_weapon_smg",	WEAP_FL_KILLPIPEBOMBS | WEAP_FL_PRIM_ATTACK | WEAP_FL_UNDERWATER,0,1000,2,2,0},
	{TF2_SLOT_PRMRY,TF2_WEAPON_SYRINGEGUN,			"tf_weapon_syringegun_medic",	WEAP_FL_PRIM_ATTACK | WEAP_FL_UNDERWATER,0,1000,1,2,0},
	{TF2_SLOT_PRMRY,TF2_WEAPON_ROCKETLAUNCHER,		"tf_weapon_rocketlauncher",	WEAP_FL_PRIM_ATTACK | WEAP_FL_EXPLOSIVE | WEAP_FL_UNDERWATER,BLAST_RADIUS,4096,1,3,TF2_ROCKETSPEED},
	{TF2_SLOT_PRMRY,TF2_WEAPON_GRENADELAUNCHER,	"tf_weapon_grenadelauncher",	WEAP_FL_PROJECTILE | WEAP_FL_PRIM_ATTACK | WEAP_FL_EXPLOSIVE | WEAP_FL_UNDERWATER,100,1200,1,2,TF2_GRENADESPEED},
	{TF2_SLOT_SCNDR,TF2_WEAPON_PIPEBOMBS,			"tf_weapon_pipebomblauncher",	WEAP_FL_NONE,0,1000,2,1,0},
	{TF2_SLOT_PRMRY,TF2_WEAPON_FLAMETHROWER,		"tf_weapon_flamethrower",	WEAP_FL_DEFLECTROCKETS | WEAP_FL_PRIM_ATTACK | WEAP_FL_MELEE | WEAP_FL_HOLDATTACK | WEAP_FL_SPECIAL,0,400,1,3,0},
	{TF2_SLOT_SCNDR,TF2_WEAPON_PISTOL,				"tf_weapon_pistol",	WEAP_FL_KILLPIPEBOMBS | WEAP_FL_PRIM_ATTACK | WEAP_FL_UNDERWATER,0,2000,2,1,0},
	{TF2_SLOT_SCNDR,TF2_WEAPON_PISTOL_SCOUT,		"tf_weapon_pistol_scout",	WEAP_FL_KILLPIPEBOMBS | WEAP_FL_PRIM_ATTACK | WEAP_FL_UNDERWATER,0,1800,2,2,0},
	{TF2_SLOT_SCNDR,TF2_WEAPON_REVOLVER,			"tf_weapon_revolver",	WEAP_FL_KILLPIPEBOMBS | WEAP_FL_PRIM_ATTACK | WEAP_FL_UNDERWATER,0,1400,2,1,0},

	// Custom Weapons

	{TF2_SLOT_PRMRY, TF2_WEAPON_POMSON6000, "tf_weapon_drg_pomson", WEAP_FL_PRIM_ATTACK | WEAP_FL_UNDERWATER, 0, 800, 1, 2, 0 },
	//{TF2_SLOT_PDA,TF2_WEAPON_PDA_ENGI_BUILD,		"tf_weapon_pda_engineer_build",	WEAP_FL_NONE,0,100,0,1,0},
	// this class is used with all classes that can use shotgun but the slot might be different
	{TF2_SLOT_SCNDR, TF2_WEAPON_SHOTGUN, "tf_weapon_shotgun", WEAP_FL_KILLPIPEBOMBS | WEAP_FL_PRIM_ATTACK | WEAP_FL_UNDERWATER, 0, 500, 2, 2, 0 },
	//{TF2_SLOT_OTHER,TF2_WEAPON_PDA_ENGI_DESTROY,	"tf_weapon_pda_engineer_destroy",	WEAP_FL_NONE,0,100,0,1,0},
	//{TF2_SLOT_PDA, TF2_WEAPON_PDA_SPY, "tf_weapon_pda_spy", WEAP_FL_NONE, 0, 100, 0, 1, 0 },
	{TF2_SLOT_PRMRY, TF2_WEAPON_FRONTIERJUSTICE, "tf_weapon_sentry_revenge", WEAP_FL_KILLPIPEBOMBS | WEAP_FL_PRIM_ATTACK | WEAP_FL_UNDERWATER, 0, 800, 1, 2, 0 },
	{TF2_SLOT_PDA, TF2_WEAPON_BUILDER, "tf_weapon_builder", WEAP_FL_NONE, 0, 100, 0, 1, 0 },
	{TF2_SLOT_SCNDR,TF2_WEAPON_MEDIGUN,			"tf_weapon_medigun",	WEAP_FL_NONE,0,100,0,1,0},
	{TF2_SLOT_PDA, TF2_WEAPON_INVIS, "tf_weapon_invis", WEAP_FL_NONE, 0, 100, 0, 1, 0 },
	{TF2_SLOT_SCNDR,TF2_WEAPON_BUFF_ITEM,	"tf_weapon_buff_item",	WEAP_FL_NONE,0,100,0,1,0},
	{TF2_SLOT_SCNDR, TF2_WEAPON_FLAREGUN, "tf_weapon_flaregun", WEAP_FL_PRIM_ATTACK, 0, 1600, 2, 2, TF2_GRENADESPEED },
	{TF2_SLOT_PDA, TF2_WEAPON_SENTRYGUN, "obj_sentrygun", 0, 0, 0, 0, 0, 0 },
	{TF2_SLOT_MELEE,TF2_WEAPON_SAXXY,		"saxxy",	WEAP_FL_PRIM_ATTACK | WEAP_FL_MELEE | WEAP_FL_UNDERWATER,0,150,0,1,0},
	//Bat Wood AKA The Sandman
	{TF2_SLOT_MELEE,TF2_WEAPON_BAT_WOOD,		"tf_weapon_bat_wood",	WEAP_FL_PRIM_ATTACK | WEAP_FL_MELEE | WEAP_FL_UNDERWATER,0,180,0,1,0},
	{TF2_SLOT_SCNDR,TF2_WEAPON_LUNCHBOX_DRINK,		"tf_weapon_lunchbox_drink",	WEAP_FL_NONE,0,180,0,1,0},
	{TF2_SLOT_PRMRY, TF2_WEAPON_BOW, "tf_weapon_compound_bow", WEAP_FL_SCOPE | WEAP_FL_PRIM_ATTACK | WEAP_FL_PROJECTILE, 400, 2500, 1, 3, 1875},
	{TF2_SLOT_SCNDR,TF2_WEAPON_JAR,		"tf_weapon_jar",	WEAP_FL_NONE,0,180,0,1,0},
	{TF2_SLOT_MELEE,TF2_WEAPON_BAT_FISH,		"tf_weapon_bat_fish",	WEAP_FL_PRIM_ATTACK | WEAP_FL_MELEE | WEAP_FL_UNDERWATER,0,180,0,1,0},
	{TF2_SLOT_PRMRY,TF2_WEAPON_DIRECTHIT,		"tf_weapon_rocketlauncher_directhit",	WEAP_FL_PRIM_ATTACK | WEAP_FL_EXPLOSIVE | WEAP_FL_UNDERWATER,BLAST_RADIUS,4096,1,3,static_cast<float>(TF2_ROCKETSPEED) * 1.8f},
	{TF2_SLOT_MELEE,TF2_WEAPON_SWORD,		"tf_weapon_sword",	WEAP_FL_PRIM_ATTACK | WEAP_FL_MELEE | WEAP_FL_UNDERWATER,0,190,0,1,0},
	{TF2_SLOT_MELEE,TF2_WEAPON_KATANA,		"tf_weapon_katana",	WEAP_FL_PRIM_ATTACK | WEAP_FL_MELEE | WEAP_FL_UNDERWATER,0,210,0,1,0},
	{TF2_SLOT_PRMRY, TF2_WEAPON_COWMANGLER, "tf_weapon_particle_cannon", WEAP_FL_KILLPIPEBOMBS | WEAP_FL_PRIM_ATTACK | WEAP_FL_UNDERWATER, 0, 1500, 1, 2, TF2_ROCKETSPEED },
	{TF2_SLOT_PRMRY,TF2_WEAPON_CROSSBOW,		"tf_weapon_crossbow",	WEAP_FL_PRIM_ATTACK,600,2500,1,3,0},
	{TF2_SLOT_MELEE,TF2_WEAPON_CLEAVER,		"tf_weapon_cleaver",	WEAP_FL_PRIM_ATTACK | WEAP_FL_MELEE | WEAP_FL_UNDERWATER,0,150,0,1,0},
	{TF2_SLOT_MELEE,TF2_WEAPON_BAT_GIFTWRAP,		"tf_weapon_bat_giftwrap",	WEAP_FL_PRIM_ATTACK | WEAP_FL_MELEE | WEAP_FL_UNDERWATER,0,150,0,1,0},
	{TF2_SLOT_SCNDR,TF2_WEAPON_RAYGUN,		"tf_weapon_raygun",	WEAP_FL_PRIM_ATTACK,100,2000,2,2,0},
	{0, 0, "\0", 0, 0, 0, 0, 0, 0}//signal last weapon
};


std::vector<WeaponsData_t> SYNERGYWeaps = {

	// slot, id , weapon name, flags, min dist, max dist, ammo index, preference
	{2, SYN_WEAPON_PISTOL, g_szSYNWeapons[0], WEAP_FL_PRIM_ATTACK | WEAP_FL_UNDERWATER, 0, 1000, -1, 1, 0},
	{1, SYN_WEAPON_CROWBAR, g_szSYNWeapons[1], WEAP_FL_PRIM_ATTACK | WEAP_FL_MELEE | WEAP_FL_UNDERWATER, 0, 128, -1, 1, 0},
	{1, SYN_WEAPON_LEADPIPE, g_szSYNWeapons[2], WEAP_FL_PRIM_ATTACK | WEAP_FL_MELEE | WEAP_FL_UNDERWATER, 0, 128, -1, 1, 0},
	{2, SYN_WEAPON_357, g_szSYNWeapons[3], WEAP_FL_PRIM_ATTACK, 0, 768, -1, 2, 0},
	{2, SYN_WEAPON_DESERTEAGLE, g_szSYNWeapons[4], WEAP_FL_PRIM_ATTACK, 0, 768, -1, 2, 0},
	{3, SYN_WEAPON_SMG1, g_szSYNWeapons[5], WEAP_FL_PRIM_ATTACK | WEAP_FL_SEC_ATTACK, 0, 1400, -1, 2, 0},
	{3, SYN_WEAPON_MP5K, g_szSYNWeapons[6], WEAP_FL_PRIM_ATTACK, 0, 1400, -1, 2, 0},
	{3, SYN_WEAPON_AR2, g_szSYNWeapons[7], WEAP_FL_PRIM_ATTACK | WEAP_FL_SEC_ATTACK, 0, 1400, -1, 3, 0},
	{5, SYN_WEAPON_FRAG, g_szSYNWeapons[8], WEAP_FL_GRENADE | WEAP_FL_EXPLOSIVE, 0, 180, -1, 1, 0},
	{1, SYN_WEAPON_STUNSTICK, g_szSYNWeapons[9], WEAP_FL_PRIM_ATTACK | WEAP_FL_MELEE | WEAP_FL_UNDERWATER, 0, 128, -1, 1, 0},
	{4, SYN_WEAPON_CROSSBOW, g_szSYNWeapons[10], WEAP_FL_PRIM_ATTACK | WEAP_FL_SCOPE | WEAP_FL_UNDERWATER, 0, 2000, -1, 2, 0},
	{5, SYN_WEAPON_RPG, g_szSYNWeapons[11], WEAP_FL_PROJECTILE | WEAP_FL_EXPLOSIVE | WEAP_FL_PRIM_ATTACK | WEAP_FL_UNDERWATER, 400, 2000, -1, 3, 1000.0f},
	{5, SYN_WEAPON_SLAM, g_szSYNWeapons[12], WEAP_FL_EXPLOSIVE, 0, 180, -1, 1, 0},
	{4, SYN_WEAPON_SHOTGUN, g_szSYNWeapons[13], WEAP_FL_PRIM_ATTACK, 0, 768, -1, 2, 0},
	{6, SYN_WEAPON_PHYSCANNON, g_szSYNWeapons[14], WEAP_FL_GRAVGUN | WEAP_FL_PRIM_ATTACK, 0, 768, -1, 4, 0},
	{4, SYN_WEAPON_MG1, g_szSYNWeapons[15], WEAP_FL_PRIM_ATTACK, 0, 1000, -1, 3, 0},
	{6, SYN_WEAPON_BUGBAIT, g_szSYNWeapons[16], WEAP_FL_PRIM_ATTACK | WEAP_FL_PROJECTILE, 0, 300, -1, 3, 1000.0f},
	{0, 0, "\0", 0, 0, 0, 0, 0, 0} // signal last weapon
};

std::vector<WeaponsData_t> CSSWeaps = {

	// slot, id , weapon name, flags, min dist, max dist, ammo index, preference, projectile speed
	{2,CS_WEAPON_KNIFE,				g_szCSWeapons[0],	WEAP_FL_PRIM_ATTACK | WEAP_FL_SEC_ATTACK | WEAP_FL_UNDERWATER | WEAP_FL_MELEE | WEAP_FL_MELEE_SEC_ATT,0,128,-1,1,0},
	{1,CS_WEAPON_USP,				g_szCSWeapons[1],	WEAP_FL_PRIM_ATTACK,0,3072,-1,2,0},
	{1,CS_WEAPON_GLOCK,				g_szCSWeapons[2],	WEAP_FL_PRIM_ATTACK,0,3072,-1,2,0},
	{1,CS_WEAPON_228C,				g_szCSWeapons[3],	WEAP_FL_PRIM_ATTACK,0,3072,-1,2,0},
	{1,CS_WEAPON_FIVESEVEN,			g_szCSWeapons[4],	WEAP_FL_PRIM_ATTACK,0,3072,-1,2,0},
	{1,CS_WEAPON_ELITES,			g_szCSWeapons[5],	WEAP_FL_PRIM_ATTACK,0,3072,-1,2,0},
	{1,CS_WEAPON_DEAGLE,			g_szCSWeapons[6],	WEAP_FL_PRIM_ATTACK,0,3072,-1,2,0},
	{0,CS_WEAPON_SUPERSHOTGUN,		g_szCSWeapons[7],	WEAP_FL_PRIM_ATTACK,0,1024,-1,3,0},
	{0,CS_WEAPON_AUTOSHOTGUN,		g_szCSWeapons[8],	WEAP_FL_PRIM_ATTACK,0,1024,-1,3,0},
	{0,CS_WEAPON_TMP,				g_szCSWeapons[9],	WEAP_FL_PRIM_ATTACK,0,2048,-1,3,0},
	{0,CS_WEAPON_MAC10,				g_szCSWeapons[10],	WEAP_FL_PRIM_ATTACK,0,2048,-1,3,0},
	{0,CS_WEAPON_MP5,				g_szCSWeapons[11],	WEAP_FL_PRIM_ATTACK,0,2048,-1,3,0},
	{0,CS_WEAPON_UMP45,				g_szCSWeapons[12],	WEAP_FL_PRIM_ATTACK,0,2048,-1,3,0},
	{0,CS_WEAPON_P90,				g_szCSWeapons[13],	WEAP_FL_PRIM_ATTACK,0,2048,-1,3,0},
	{0,CS_WEAPON_FAMAS,				g_szCSWeapons[14],	WEAP_FL_PRIM_ATTACK,0,4096,-1,3,0},
	{0,CS_WEAPON_GALIL,				g_szCSWeapons[15],	WEAP_FL_PRIM_ATTACK,0,4096,-1,3,0},
	{0,CS_WEAPON_AK47,				g_szCSWeapons[16],	WEAP_FL_PRIM_ATTACK,0,4096,-1,3,0},
	{0,CS_WEAPON_M4A1,				g_szCSWeapons[17],	WEAP_FL_PRIM_ATTACK,0,4096,-1,3,0},
	{0,CS_WEAPON_AUG,				g_szCSWeapons[18],	WEAP_FL_PRIM_ATTACK | WEAP_FL_SCOPE | WEAP_FL_ZOOMABLE,0,4096,-1,3,0},
	{0,CS_WEAPON_SG552,				g_szCSWeapons[19],	WEAP_FL_PRIM_ATTACK | WEAP_FL_SCOPE | WEAP_FL_ZOOMABLE,0,4096,-1,3,0},
	{0,CS_WEAPON_SCOUT,				g_szCSWeapons[20],	WEAP_FL_PRIM_ATTACK | WEAP_FL_SCOPE | WEAP_FL_ZOOMABLE,512,4096,-1,3,0},
	{0,CS_WEAPON_AWP,				g_szCSWeapons[21],	WEAP_FL_PRIM_ATTACK | WEAP_FL_SCOPE | WEAP_FL_ZOOMABLE,512,4096,-1,3,0},
	{0,CS_WEAPON_SG550,				g_szCSWeapons[22],	WEAP_FL_PRIM_ATTACK | WEAP_FL_SCOPE | WEAP_FL_ZOOMABLE,512,4096,-1,3,0},
	{0,CS_WEAPON_G3SG1,				g_szCSWeapons[23],	WEAP_FL_PRIM_ATTACK | WEAP_FL_SCOPE | WEAP_FL_ZOOMABLE,512,4096,-1,3,0},
	{0,CS_WEAPON_M249,				g_szCSWeapons[24],	WEAP_FL_PRIM_ATTACK,0,2048,-1,3,0},
	{3,CS_WEAPON_HE_GRENADE,		g_szCSWeapons[25],	WEAP_FL_PRIM_ATTACK | WEAP_FL_PROJECTILE | WEAP_FL_EXPLOSIVE | WEAP_FL_GRENADE,0,2048,-1,1,0},
	{3,CS_WEAPON_FLASH_GRENADE,		g_szCSWeapons[26],	WEAP_FL_PRIM_ATTACK | WEAP_FL_PROJECTILE | WEAP_FL_GRENADE,0,2048,-1,1,0},
	{3,CS_WEAPON_SMOKE_GRENADE,		g_szCSWeapons[27],	WEAP_FL_PRIM_ATTACK | WEAP_FL_PROJECTILE | WEAP_FL_GRENADE,0,2048,-1,1,0},
	{4,CS_WEAPON_C4,				g_szCSWeapons[28],	WEAP_FL_PRIM_ATTACK | WEAP_FL_SPECIAL | WEAP_FL_HOLDATTACK,0,1024,-1,1,0},
	{ 0, 0, "\0", 0, 0, 0, 0, 0, 0 }//signal last weapon
};

bool CBotWeapon::needToReload(const CBot* pBot) const
{
	if (m_iClip1)
	{
		return *m_iClip1 == 0 && getAmmo(pBot) > 0;
	}

	return false;
}

// static init (all weapons in game)
std::vector<CWeapon*> CWeapons::m_theWeapons;

int CBotWeapon::getAmmo(const CBot* pBot, const int type) const
{
	if (type == AMMO_PRIM)
		return pBot->getAmmo(static_cast<std::size_t>(m_pWeaponInfo->getAmmoIndex1()));

	if (type == AMMO_SEC)
		return pBot->getAmmo(static_cast<std::size_t>(m_pWeaponInfo->getAmmoIndex2()));

	return 0;
}

// Example of using range-based for loop and nullptr
bool CBotWeapons::hasExplosives() const
{
	for (const CBotWeapon& weapon : m_theWeapons)
	{
		// find weapon info from weapon id
		if (weapon.hasWeapon() && weapon.isExplosive() && weapon.getAmmo(m_pBot) > 1)
		{
			return true;
		}
	}

	return false;
}

bool CBotWeapons::hasWeapon(int id) const
{
	return std::any_of(std::begin(m_theWeapons), std::end(m_theWeapons), [id](const CBotWeapon& weapon) {
		return weapon.getWeaponInfo() != nullptr && weapon.hasWeapon() && weapon.getID() == id;
		});
}

// Bot Weapons
CBotWeapons::CBotWeapons(CBot* pBot)
{
	m_pBot = pBot;
	clearWeapons();
	/*
	for ( int i = 0; i < MAX_WEAPONS; i ++ )
	{
	// find weapon info from weapon id
	m_theWeapons[i].setWeapon(CWeapons::getWeapon(i));
	}
	*/
	m_fUpdateWeaponsTime = 0;
	m_iWeaponsSignature = 0x0;
}

edict_t* CWeapons::findWeapon(edict_t* pPlayer, const char* szWeaponName)
{
	const CBaseHandle* m_Weapons = CClassInterface::getWeaponList(pPlayer);
	const CBaseHandle* m_Weapon_iter = m_Weapons;
	// loop through the weapons array and see if it is in the CBaseCombatCharacter

	for (unsigned short int j = 0; j < MAX_WEAPONS; j++)
	{
		edict_t* pWeapon = INDEXENT(m_Weapon_iter->GetEntryIndex());

		// TODO get familiar with validity of handles / edicts
		if (pWeapon && !pWeapon->IsFree() && std::strcmp(pWeapon->GetClassName(), szWeaponName) == 0)
			return pWeapon;

		m_Weapon_iter++;
	}

	return nullptr;
}

bool CBotWeapons::update(const bool bOverrideAllFromEngine)
{
	std::uintptr_t iWeaponsSignature = 0x0; // check sum of weapons
	edict_t* pWeapon;

	const CBaseHandle* m_Weapons = CClassInterface::getWeaponList(m_pBot->getEdict());
	const CBaseHandle* m_Weapon_iter = m_Weapons;

	for (short int i = 0; i < MAX_WEAPONS; i++)
	{
		// create a 'hash' of current weapons
		pWeapon = m_Weapon_iter == nullptr ? nullptr : INDEXENT(m_Weapon_iter->GetEntryIndex());
		iWeaponsSignature += reinterpret_cast<std::uintptr_t>(pWeapon) +
			static_cast<std::uintptr_t>(pWeapon == nullptr ? 0 : static_cast<unsigned>(CClassInterface::getWeaponState(pWeapon)));

		if (m_Weapon_iter != nullptr) {
			m_Weapon_iter++;
		}
	}

	// if weapons have changed this will be different
	if (iWeaponsSignature != m_iWeaponsSignature) // m_fUpdateWeaponsTime < engine->Time() )
	{
		this->clearWeapons();

		CBotWeapon* m_BotWeapon_iter = m_theWeapons; //`m_BotWeapon_iter` not used? [APG]RoboCop[CL]

		// loop through the weapons array and see if it is in the CBaseCombatCharacter
		for (unsigned short int i = 0; i < MAX_WEAPONS; i++)
		{
			m_Weapon_iter = &m_Weapons[i];
			bool bFound = false; //`bFound` not used? [APG]RoboCop[CL]

			pWeapon = INDEXENT(m_Weapon_iter->GetEntryIndex());

			if (!pWeapon || pWeapon->IsFree())
			{
				continue;
			}

			const int iWeaponState = CClassInterface::getWeaponState(pWeapon);

			const char* pszClassname = pWeapon->GetClassName();

			CWeapon* pWeaponInfo = CWeapons::getWeapon(pszClassname);

			if (pWeaponInfo != nullptr)
			{
				if (iWeaponState != WEAPON_NOT_CARRIED)
				{
					CBotWeapon* pAdded = addWeapon(pWeaponInfo, i, pWeapon, bOverrideAllFromEngine);
					pAdded->setHasWeapon(true);
				}
			}
		}

		// check again in 1 second
		m_fUpdateWeaponsTime = engine->Time() + 1.0f;

		m_iWeaponsSignature = iWeaponsSignature;

		return true; // change
	}

	return false;
}
/*
bool CBotWeapons ::update ( bool bOverrideAllFromEngine )
{
	// create mask of weapons data
	short int i = 0;
	unsigned short int iWeaponsSignature = 0x0; // check sum of weapons
	edict_t *pWeapon;
	CBaseHandle *m_Weapons = CClassInterface::getWeaponList(m_pBot->getEdict());
	CBaseHandle *m_Weapon_iter;

	m_Weapon_iter = m_Weapons;

	for ( i = 0; i < MAX_WEAPONS; i ++ )
	{
		// create a 'hash' of current weapons
		pWeapon = (m_Weapon_iter==nullptr) ? nullptr : INDEXENT(m_Weapon_iter->GetEntryIndex());
		iWeaponsSignature += ((unsigned)pWeapon) + ((pWeapon == nullptr) ? 0 : (unsigned)CClassInterface::getWeaponState(pWeapon));
		m_Weapon_iter++;
	}

	// if weapons have changed this will be different
	if ( iWeaponsSignature != m_iWeaponsSignature ) // m_fUpdateWeaponsTime < engine->Time() )
	{
		int iWeaponState;
		unsigned short int i,j;
		bool bFound;

		CBaseHandle *m_Weapons = CClassInterface::getWeaponList(m_pBot->getEdict());
		CBotWeapon *m_BotWeapon_iter = m_theWeapons;

		// loop through the weapons array and see if it is in the CBaseCombatCharacter
		for ( i = 0; i < MAX_WEAPONS; i ++ )
		{
			m_Weapon_iter = m_Weapons;
			iWeaponState = 0;
			bFound = false;
			pWeapon = nullptr;

			if ( (m_BotWeapon_iter->getWeaponInfo()!=nullptr) && (m_BotWeapon_iter->getWeaponInfo()->getWeaponName()!=nullptr) )
			{
				for ( j = 0; j < MAX_WEAPONS; j ++ )
				{
					pWeapon = INDEXENT(m_Weapon_iter->GetEntryIndex());

					if ( pWeapon && CBotGlobals::entityIsValid(pWeapon) && strcmp(pWeapon->GetClassName(),m_BotWeapon_iter->getWeaponInfo()->getWeaponName())==0 )
					{
						iWeaponState = CClassInterface::getWeaponState(pWeapon);
						// found it
						bFound = true;
						break;
					}

					m_Weapon_iter++;
				}

				if ( bFound && pWeapon && (iWeaponState != WEAPON_NOT_CARRIED) )
				{
					if ( !m_BotWeapon_iter->hasWeapon() )
						addWeapon(m_BotWeapon_iter->getID(),pWeapon,bOverrideAllFromEngine);
					else if ( m_BotWeapon_iter->getWeaponEntity() != pWeapon )
						m_BotWeapon_iter->setWeaponEntity(pWeapon,bOverrideAllFromEngine);
				}
				else
				{
					if ( m_BotWeapon_iter->hasWeapon() )
						m_BotWeapon_iter->setHasWeapon(false);
				}
			}

			m_BotWeapon_iter++;
		}

		// check again in 1 second
		m_fUpdateWeaponsTime = engine->Time() + 1.0f;

		m_iWeaponsSignature = iWeaponsSignature;

		return true; // change
	}

	return false;
}*/

CBotWeapon* CBotWeapons::getBestWeapon(edict_t* pEnemy, const bool bAllowMelee, const bool bAllowMeleeFallback, const bool bMeleeOnly, const bool bExplosivesOnly, const bool bIgnorePrimaryMinimum)
{
	CBotWeapon* m_theBestWeapon = nullptr;
	CBotWeapon* m_FallbackMelee = nullptr;
	int iBestPreference = 0;
	Vector vEnemyOrigin;

	if (pEnemy)
		vEnemyOrigin = CBotGlobals::entityOrigin(pEnemy);
	else
		vEnemyOrigin = m_pBot->getOrigin();

	float flDist = 0;

	if (pEnemy)
		flDist = m_pBot->distanceFrom(vEnemyOrigin);

	for (CBotWeapon& m_theWeapon : m_theWeapons)
	{
		CBotWeapon* pWeapon = &m_theWeapon;

		if (!pWeapon->hasWeapon())
			continue;

		if (bMeleeOnly && !pWeapon->isMelee())
			continue;

		if (!bAllowMelee && pWeapon->isMelee())
			continue;

		if (bExplosivesOnly && !pWeapon->isExplosive())
			continue;

		if (!pWeapon->isMelee() || pWeapon->isSpecial())
		{
			if (pWeapon->outOfAmmo(m_pBot))
				continue;
		}

		if (!pWeapon->canAttack())
			continue;

		if (m_pBot->isUnderWater() && !pWeapon->canUseUnderWater())
			continue;

		if (!pWeapon->primaryInRange(flDist))
		{
			if (pWeapon->isMelee() && !pWeapon->isSpecial())
				m_FallbackMelee = pWeapon;

			if (!pWeapon->isExplosive() || !bIgnorePrimaryMinimum)
				continue; // ignore explosive range if I'm invincible
		}

		if (pWeapon->getPreference() > iBestPreference)
		{
			iBestPreference = pWeapon->getPreference();
			m_theBestWeapon = pWeapon;
		}
	}

	if (bMeleeOnly || (bAllowMeleeFallback && (m_theBestWeapon == nullptr && flDist < 400.0f &&
		std::fabs(vEnemyOrigin.z - m_pBot->getOrigin().z) < static_cast<float>(BOT_JUMP_HEIGHT))))
	{
		m_theBestWeapon = m_FallbackMelee;
	}

	return m_theBestWeapon;
}

void CBotWeapon::setWeaponEntity(edict_t* pent, const bool bOverrideAmmoTypes)
{
	m_pEnt = pent;
	m_iClip1 = CClassInterface::getWeaponClip1Pointer(pent);
	m_iClip2 = CClassInterface::getWeaponClip2Pointer(pent);

	if (bOverrideAmmoTypes)
	{
		int iAmmoType1, iAmmoType2;
		CClassInterface::getAmmoTypes(pent, &iAmmoType1, &iAmmoType2);
		m_pWeaponInfo->setAmmoIndex(iAmmoType1, iAmmoType2);
	}

	setWeaponIndex(ENTINDEX(m_pEnt));
}

CBotWeapon* CBotWeapons::addWeapon(CWeapon* pWeaponInfo, const int iId, edict_t* pent, const bool bOverrideAll)
{
	m_theWeapons[iId].setHasWeapon(true);
	m_theWeapons[iId].setWeapon(pWeaponInfo);

	if (!m_theWeapons[iId].getWeaponInfo())
		return nullptr;

	const char* classname = pWeaponInfo->getWeaponName();

	const Vector origin = m_pBot->getOrigin();

	// if entity comes from the engine use the entity
	if (pent)
	{
		m_theWeapons[iId].setWeaponEntity(pent, bOverrideAll);
	}
	else // find the weapon entity
	{
		for (int i = gpGlobals->maxClients + 1; i <= gpGlobals->maxEntities; i++)
		{
			edict_t* pEnt = INDEXENT(i);

			if (pEnt && CBotGlobals::entityIsValid(pEnt))
			{
				if (CBotGlobals::entityOrigin(pEnt) == origin)
				{
					if (std::strcmp(pEnt->GetClassName(), classname) == 0)
					{
						m_theWeapons[iId].setWeaponEntity(pEnt, bOverrideAll);// .setWeaponIndex(ENTINDEX(pEnt));

						return &m_theWeapons[iId];
					}
				}
			}
		}
	}

	return &m_theWeapons[iId];
}
/*
void CBotWeapons :: addWeapon ( int iId, edict_t *pent, bool bOverrideAll )
{
int i = 0;
Vector origin;
const char *classname;
CWeapon *pWeapon;
edict_t *pEnt = nullptr;

m_theWeapons[iId].setHasWeapon(true);

pWeapon = m_theWeapons[iId].getWeaponInfo();

if ( !pWeapon )
return;

classname = pWeapon->getWeaponName();

origin = m_pBot->getOrigin();

// if entity comes from the engine use the entity
if ( pent )
{
m_theWeapons[iId].setWeaponEntity(pent,bOverrideAll);
}
else // find the weapon entity
{
for ( i = (gpGlobals->maxClients+1); i <= gpGlobals->maxEntities; i ++ )
{
pEnt = INDEXENT(i);

if ( pEnt && CBotGlobals::entityIsValid(pEnt) )
{
if ( CBotGlobals::entityOrigin(pEnt) == origin )
{
if ( strcmp(pEnt->GetClassName(),classname) == 0 )
{
m_theWeapons[iId].setWeaponEntity(pEnt, bOverrideAll);// .setWeaponIndex(ENTINDEX(pEnt));

return;
}
}
}
}
}
}*/

CBotWeapon* CBotWeapons::getWeapon(const CWeapon* pWeapon)
{
	for (CBotWeapon& m_theWeapon : m_theWeapons)
	{
		if (m_theWeapon.getWeaponInfo() == pWeapon)
			return &m_theWeapon;
	}

	return nullptr;
}

CBotWeapon* CBotWeapons::getCurrentWeaponInSlot(const int iSlot)
{
	for (CBotWeapon& m_theWeapon : m_theWeapons)
	{
		if (m_theWeapon.hasWeapon() && m_theWeapon.getWeaponInfo() && m_theWeapon.getWeaponInfo()->getSlot() == iSlot)
			return &m_theWeapon;
	}

	return nullptr;
}

constexpr std::array<const char*, 21> g_szWeaponFlags = {
	"primary_attack",
	"secondary_attack",
	"explosive",
	"melee",
	"underwater",
	"hold_attack",
	"special",
	"can_kill_pipes",
	"can_deflect_rockets",
	"is_grav_gun",
	"has_explosive_secondary",
	"is_zoomable",
	"is_deployable_dods",
	"has_melee_secondary",
	"has_fire_select_mode_dods",
	"cant_be_fired_unzoomed_undeployed_dods",
	"is_grenade",
	"has_high_recoil_dods",
	"has_scope",
	"weapon_fires_projectile",
	"\0"
};

std::unordered_map<std::string, int> createWeaponFlagMap() {
	std::unordered_map<std::string, int> flagMap;
	for (std::size_t i = 0; i < g_szWeaponFlags.size(); ++i) {
		flagMap[g_szWeaponFlags[i]] = 1 << i;
	}
	return flagMap;
}

const std::unordered_map<std::string, int> g_weaponFlagMap = createWeaponFlagMap();

void CWeapons::loadWeapons(const char* szWeaponListName, const WeaponsData_t* pDefault) {
	if (szWeaponListName && szWeaponListName[0] != '\0') {
		auto keyValuesDeleter = [](KeyValues* kv) {
			if (kv) {
				kv->deleteThis();
			}
			};

		const std::unique_ptr<KeyValues, decltype(keyValuesDeleter)> kv(new KeyValues("Weapons"), keyValuesDeleter);
		char szFilename[1024];

		CBotGlobals::buildFileName(szFilename, "weapons", BOT_CONFIG_FOLDER, "ini", false);

		if (kv && kv->LoadFromFile(filesystem, szFilename, nullptr)) {
			if (KeyValues* weaponListKey = kv->FindKey(szWeaponListName)) {

				for (KeyValues* subKey = weaponListKey->GetFirstSubKey(); subKey != nullptr; subKey = subKey->GetNextTrueSubKey()) {
					WeaponsData_t newWeapon;
					std::memset(&newWeapon, 0, sizeof(WeaponsData_t));

					if (const char* szKeyName = subKey->GetName()) {
						std::string lowered(szKeyName);
						std::transform(lowered.begin(), lowered.end(), lowered.begin(), ::tolower);

						newWeapon.szWeaponName = CStrings::getString(lowered.c_str());
						newWeapon.iId = subKey->GetInt("id");
						newWeapon.iSlot = subKey->GetInt("slot");
						newWeapon.minPrimDist = subKey->GetFloat("minPrimDist");
						newWeapon.maxPrimDist = subKey->GetFloat("maxPrimDist");
						newWeapon.m_fProjSpeed = subKey->GetFloat("m_fProjSpeed");
						newWeapon.m_iAmmoIndex = subKey->GetInt("m_iAmmoIndex");
						newWeapon.m_iPreference = subKey->GetInt("m_iPreference");

						if (KeyValues* flags = subKey->FindKey("flags")) {
							for (const auto& flag : g_weaponFlagMap) {
								if (flags->GetInt(flag.first.c_str(), 0) == 1) {
									newWeapon.m_iFlags |= flag.second;
								}
							}
						}

						addWeapon(new CWeapon(&newWeapon));
					}
					else {
						logger->Log(LogLevel::ERROR, "Error: szKeyName is null.\n");
					}
				}
			}
		}
		else {
			logger->Log(LogLevel::ERROR, "Error: Failed to allocate KeyValues.\n");
		}
	}

	if (pDefault) {
		if (m_theWeapons.empty()) {
			while (pDefault->szWeaponName[0] != '\0') {
				addWeapon(new CWeapon(pDefault));
				++pDefault;
			}
		}
	}
}

void CBotWeapons::clearWeapons()
{
	for (CBotWeapon& m_theWeapon : m_theWeapons)
	{
		m_theWeapon = CBotWeapon(); // Assign a default-constructed instance [APG]RoboCop[CL]
		// Alternatively, call a reset or clear method if available
		// m_theWeapon.reset();
	}
}

// returns weapon with highest priority even if no ammo
CBotWeapon* CBotWeapons::getPrimaryWeapon()
{
	CBotWeapon* pBest = nullptr;

	for (CBotWeapon& m_theWeapon : m_theWeapons)
	{
		CBotWeapon* pWeap = &m_theWeapon;

		if (!pWeap->hasWeapon())
			continue;

		if (pBest == nullptr || pBest->getPreference() < pWeap->getPreference())
		{
			pBest = pWeap;
		}
	}

	return pBest;
}

CBotWeapon* CBotWeapons::getActiveWeapon(const char* szWeaponName, edict_t* pWeaponUpdate, const bool bOverrideAmmoTypes)
{
	CBotWeapon* toReturn = nullptr;

	if (szWeaponName && *szWeaponName)
	{
		if (const CWeapon* pWeapon = CWeapons::getWeapon(szWeaponName))
		{
			for (CBotWeapon& m_theWeapon : m_theWeapons)
			{
				const CWeapon* p = m_theWeapon.getWeaponInfo();

				if (!p)
					continue;

				if (std::strcmp(p->getWeaponName(), szWeaponName) == 0)
				{
					toReturn = &m_theWeapon;
					break;
				}
			}

			if (pWeaponUpdate && toReturn)
			{
				toReturn->setWeaponEntity(pWeaponUpdate, bOverrideAmmoTypes);
			}
		}
	}

	return toReturn;
}
/*
bool CBotWeaponGravGun ::outOfAmmo (CBot *pBot)
{
	if ( m_pEnt )
		(return CClassInterface::gravityGunObject(m_pEnt)==nullptr);

	return true;
}
*/
bool CBotWeapon::outOfAmmo(const CBot* pBot) const
{
	if (m_pWeaponInfo && m_pWeaponInfo->isGravGun() && m_pEnt)
		return CClassInterface::gravityGunObject(m_pEnt) == nullptr;

	// if I have something in my clip now
	// I am okay, otherwise return ammo in list
	if (m_iClip1 && *m_iClip1 > 0)
		return false;

	return getAmmo(pBot) == 0;
}
/*
bool CBotWeapon :: needToReload(CBot *pBot)
{
	return getAmmo(pBot)==0;
}*/
////////////////////////////////////////
// CWeapons

class IWeaponFunc
{
public:
	virtual ~IWeaponFunc() = default;
	virtual void execute(CWeapon* pWeapon) = 0;
};

class CGetWeapID : public IWeaponFunc
{
public:
	CGetWeapID(const int iId)
	{
		m_iId = iId;
		m_pFound = nullptr;
	}

	void execute(CWeapon* pWeapon) override
	{
		if (m_iId == pWeapon->getID())
			m_pFound = pWeapon;
	}

	CWeapon* get() const
	{
		return m_pFound;
	}

private:
	int m_iId;
	CWeapon* m_pFound;
};

class CGetWeapCName : public IWeaponFunc
{
public:
	CGetWeapCName(const char* szWeapon)
	{
		m_pFound = nullptr;
		m_szWeapon = szWeapon;
	}

	void execute(CWeapon* pWeapon) override
	{
		if (pWeapon->isWeaponName(m_szWeapon))
			m_pFound = pWeapon;
	}

	CWeapon* get() const
	{
		return m_pFound;
	}
private:
	const char* m_szWeapon;
	CWeapon* m_pFound;
};

class CGetWeapShortName : public IWeaponFunc
{
public:
	CGetWeapShortName(const char* szWeapon)
	{
		m_pFound = nullptr;
		m_szWeapon = szWeapon;
	}

	void execute(CWeapon* pWeapon) override
	{
		if (pWeapon->isShortWeaponName(m_szWeapon))
			m_pFound = pWeapon;
	}

	CWeapon* get() const
	{
		return m_pFound;
	}
private:
	const char* m_szWeapon;
	CWeapon* m_pFound;
};

CWeapon* CWeapons::getWeapon(const int iId)
{
	CGetWeapID pFunc = CGetWeapID(iId);
	eachWeapon(&pFunc);
	return pFunc.get();
}

CWeapon* CWeapons::getWeapon(const char* szWeapon)
{
	CGetWeapCName pFunc = CGetWeapCName(szWeapon);
	eachWeapon(&pFunc);
	return pFunc.get();
}

CWeapon* CWeapons::getWeaponByShortName(const char* szWeapon)
{
	CGetWeapShortName pFunc = CGetWeapShortName(szWeapon);
	eachWeapon(&pFunc);
	return pFunc.get();
}

void CWeapons::eachWeapon(IWeaponFunc* pFunc)
{
	for (CWeapon* const& m_theWeapon : m_theWeapons)
	{
		pFunc->execute(m_theWeapon);
	}
}

void CWeapons::freeMemory()
{
	for (CWeapon*& m_theWeapon : m_theWeapons)
	{
		delete m_theWeapon;
		m_theWeapon = nullptr;
	}

	m_theWeapons.clear();
}