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
#ifndef __BOT_CONST_H__
#define __BOT_CONST_H__

#include "mathlib/vector.h"
#include "utlvector.h"
#include "shareddefs.h"
#include <cstdint>
#include <cstring>

#ifndef __linux__
constexpr char BOT_WELCOME_MESSAGE[] = "Welcome to RCBot by Cheeseh";
#else
constexpr char BOT_WELCOME_MESSAGE[] = "Welcome to RCBot by Cheeseh for Linux";
#endif

constexpr float BOT_DEFAULT_FOV = 80.0f;

#define __to_lower(a) (((a)>='A')&&((a)<='Z'))?('a'+((a)-'A')):(a)
#define __strlow(str) { char *__strx = str; while ( __strx && *__strx ) { *__strx = __to_lower(*__strx); __strx++; } }
//#define strlow(str) { unsigned short int len = strlen(str); unsigned short int i;	for ( i = 0; i < len; i ++ ) { str[i] = to_lower(str[i]); } }
#define __round(a) ((((a)-(int)(a)) >= 0.5) ? ((int)(a)+1) : ((int)(a)))

//#define RANDOM_INT(min,max) (min + round(((float)rand()/RAND_MAX)*(float)(max-min)))
//#define RANDOM_FLOAT(min,max) (min + ((float)rand()/RAND_MAX)*(float)(max-min))

constexpr char DEFAULT_BOT_NAME[] = "RCBot";

enum : std::uint8_t
{
	BOT_CONVAR_FLAGS_OFFSET = 20
};

enum : std::uint8_t
{
	BOT_WPT_TOUCH_DIST = 72 // distance for bot to touch waypoint
};

enum : std::uint8_t
{
	BOT_DEBUG_GAME_EVENT = 0,
	BOT_DEBUG_NAV = 1,
	BOT_DEBUG_SPEED = 2,
	BOT_DEBUG_VIS = 3,
	BOT_DEBUG_TASK = 4,
	BOT_DEBUG_BUTTONS = 5,
	BOT_DEBUG_USERCMD = 6,
	BOT_DEBUG_UTIL = 7,
	BOT_DEBUG_PROFILE = 8,
	BOT_DEBUG_EDICTS = 9,
	BOT_DEBUG_THINK = 10,
	BOT_DEBUG_LOOK = 11,
	BOT_DEBUG_HUD = 12,
	BOT_DEBUG_AIM = 13,
	BOT_DEBUG_CHAT = 14
};

// from sourcemod
enum RoundState : std::uint8_t
{
	// initialize the game, create teams
	RoundState_Init,
	//Before players have joined the game. Periodically checks to see if enough players are ready
	//to start a game. Also reverts to this when there are no active players
	RoundState_Pregame,
	//The game is about to start, wait a bit and spawn everyone
	RoundState_StartGame,
	//All players are respawned, frozen in place
	RoundState_Preround,
	//Round is on, playing normally
	RoundState_RoundRunning,
	//Someone has won the round
	RoundState_TeamWin,
	//Noone has won, manually restart the game, reset scores
	RoundState_Restart,
	//Noone has won, restart the game
	RoundState_Stalemate,
	//Game is over, showing the scoreboard etc
	RoundState_GameOver,
	//Game is over, doing bonus round stuff
	RoundState_Bonus,
	//Between rounds
	RoundState_BetweenRounds,
};

typedef enum : std::uint8_t
{
	LOOK_NONE = 0,
	LOOK_VECTOR,
	LOOK_WAYPOINT,
	LOOK_WAYPOINT_NEXT_ONLY,
	LOOK_AROUND,
	LOOK_ENEMY,
	LOOK_LAST_ENEMY,
	LOOK_HURT_ORIGIN,
	LOOK_EDICT,
	LOOK_GROUND,
	LOOK_SNIPE,
	LOOK_WAYPOINT_AIM,
	LOOK_BUILD,
	LOOK_NOISE,
	LOOK_MAX
}eLookTask;

extern const char *g_szLookTaskToString[LOOK_MAX];

constexpr char BOT_CONFIG_FOLDER[] = "config";
constexpr char BOT_MOD_FILE[] = "bot_mods";
constexpr char BOT_ACCESS_CLIENT_FILE[] = "accessclients";
constexpr char BOT_PROFILE_FOLDER[] = "profiles";
constexpr char BOT_WAYPOINT_FOLDER[] = "waypoints";
constexpr char BOT_AUXILERY_FOLDER[] = "aux_data";
constexpr char BOT_CONFIG_EXTENSION[] = "ini";

constexpr char BOT_WAYPOINT_EXTENSION[] = "rcw"; // extension for waypoint files
constexpr char BOT_WAYPOINT_FILE_TYPE[] = "RCBot2\0"; // for waypoint file header

constexpr char BOT_VISIBILITY_EXTENSION[] = "rcv"; // extension for visibility files

constexpr char BOT_TAG[] = "[RCBot] "; // for printing messages

/*
// Engine player info, no game related infos here
// If you change this, change the two byteswap defintions: 
// cdll_client_int.cpp and cdll_engine_int.cpp
typedef struct player_info_s
{
	DECLARE_BYTESWAP_DATADESC();
	// scoreboard information
	char			name[MAX_PLAYER_NAME_LENGTH];
	// local server user ID, unique while server is running
	int				userID;
	// global unique player identifer
	char			guid[SIGNED_GUID_LEN + 1];
	// friends identification number
	uint32			friendsID;
	// friends name
	char			friendsName[MAX_PLAYER_NAME_LENGTH];
	// true, if player is a bot controlled by game.dll
	bool			fakeplayer;
	// true if player is the HLTV proxy
	bool			ishltv;
	// custom files CRC for this player
	CRC32_t			customFiles[MAX_CUSTOM_FILES];
	// this counter increases each time the server downloaded a new file
	unsigned char	filesDownloaded;
} player_info_t;*/

typedef enum : std::uint8_t
{
	MOD_UNSUPPORTED = 0,
	MOD_HLDM2,
	MOD_CSS,
	MOD_FF,
	MOD_TF2,
	MOD_SVENCOOP2,
	MOD_TIMCOOP,
	MOD_HL1DMSRC,
	MOD_NS2,
	MOD_SYNERGY,
	MOD_DOD,
	MOD_CUSTOM,
	MOD_ANY,
	MOD_MAX
}eModId;

#define BITS_MOD_ALL ~(1<<MOD_MAX)

enum : std::uint8_t
{
	BOT_JUMP_HEIGHT = 45,
	MIN_COVER_MOVE_DIST = 128
};

#undef INDEXENT
#define INDEXENT(iEdictNum) engine->PEntityOfEntIndex(iEdictNum)

#undef ENTINDEX
#define ENTINDEX(pEdict) engine->IndexOfEdict(pEdict)

constexpr char BOT_FOLDER[] = "rcbot2";

typedef enum : std::uint8_t
{
	BOT_FUNC_FAIL = 0,
	BOT_FUNC_CONTINUE,
	BOT_FUNC_COMPLETE,
}eBotFuncState;

// bot condition bits
constexpr int CONDITION_ENEMY_OBSCURED = 1 << 0;            // bot lost sight of enemy and can't see clearly
constexpr int CONDITION_NO_WEAPON = 1 << 1;                 // bot doesn't have a weapon
constexpr int CONDITION_OUT_OF_AMMO = 1 << 2;               // bot has no ammo
constexpr int CONDITION_SEE_CUR_ENEMY = 1 << 3;             // bot can see current enemy
constexpr int CONDITION_ENEMY_DEAD = 1 << 4;                // bot's enemy is dead
constexpr int CONDITION_SEE_WAYPOINT = 1 << 5;              // bot can see the current waypoint
constexpr int CONDITION_NEED_AMMO = 1 << 6;                 // bot needs ammo (low)
constexpr int CONDITION_NEED_HEALTH = 1 << 7;               // bot needs health
constexpr int CONDITION_SEE_LOOK_VECTOR = 1 << 8;           // bot can see his 'look' vector
constexpr int CONDITION_POINT_CAPTURED = 1 << 9;            // a point has been captured recently
constexpr int CONDITION_PUSH = 1 << 10;                     // bots are more likely to attack and stop sniping etc
constexpr int CONDITION_LIFT = 1 << 11;                     // bot is on a lift
constexpr int CONDITION_SEE_PLAYERTOHELP = 1 << 12;         // heal for non-TF mods
constexpr int CONDITION_SEE_LAST_ENEMY_POS = 1 << 13;       // bots can see the last place they saw an enemy
constexpr int CONDITION_CHANGED = 1 << 14;                  // bots want to change their course of action
constexpr int CONDITION_COVERT = 1 << 15;                   // bots are more sensitive to enemies and more likely to take alternate paths
constexpr int CONDITION_RUN = 1 << 16;                      // bots have to run e.g. there is a grenade nearby
constexpr int CONDITION_GREN = 1 << 17;                     // bots will be more likely to throw a grenade
constexpr int CONDITION_NEED_BOMB = 1 << 18;                // bot needs a bomb for dod:s bomb maps
constexpr int CONDITION_SEE_ENEMY_HEAD = 1 << 19;           // bot can aim for a headshot
constexpr int CONDITION_PRONE = 1 << 20;                    // bot needs to go prone (lie down)
constexpr int CONDITION_PARANOID = 1 << 21;                 // bot is paranoid of spies or unknown enemy
constexpr int CONDITION_SEE_SQUAD_LEADER = 1 << 22;         // bot can see his leader
constexpr int CONDITION_SQUAD_LEADER_DEAD = 1 << 23;        // bot's leader is dead
constexpr int CONDITION_SQUAD_LEADER_INRANGE = 1 << 24;     // bot's leader is in range
constexpr int CONDITION_SQUAD_IDLE = 1 << 25;               // bot's squad isn't doing anything fun
constexpr int CONDITION_DEFENSIVE = 1 << 26;                // bot leader told me to defend
constexpr int CONDITION_BUILDING_SAPPED = 1 << 27;          // one of engineer's buildings sapped
constexpr int CONDITION_SEE_ENEMY_GROUND = 1 << 28;         // can see enemy ground so aim for it if i have explosive

// Number of bits allocated to bot conditions (+1 from last bitshift)
constexpr int NUM_CONDITIONS = 29;

// Alias for TF: medic bot can see his player he wants to heal
constexpr int CONDITION_SEE_HEAL = CONDITION_SEE_PLAYERTOHELP;

////////////////////////
constexpr float BLAST_RADIUS = 200.0f;
///////////////////////
typedef enum : std::uint8_t
{
	STATE_IDLE = 0,
	STATE_RUNNING,
	STATE_FAIL,
	STATE_COMPLETE
}eTaskState;
////////////////////

#endif