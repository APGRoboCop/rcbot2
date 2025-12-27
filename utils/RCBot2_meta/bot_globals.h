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

#ifndef __BOT_GLOBALS_H__
#define __BOT_GLOBALS_H__

#include "bot_mods.h"
#include "bot_const.h" // for Mod id
#include "bot_commands.h" // for main rcbot command

#include <fstream>

#if defined(_WIN64) || defined(_WIN32)
#include <cctype>
#endif

enum : std::uint16_t
{
	MAX_MAP_STRING_LEN = 64,
	MAX_PATH_LEN = 512,
	MAX_ENTITIES = 2048
};

class CBotGlobals
{
public:
	CBotGlobals ();

	static void init ();
	static bool initModFolder ();

	static bool gameStart ();	

	static QAngle entityEyeAngles ( edict_t *pEntity );

	static QAngle playerAngles ( edict_t *pPlayer );

	static bool isPlayer (const edict_t* pEdict)
	{
		static int index;

		index = ENTINDEX(pEdict);

		return index>0&&index<=gpGlobals->maxClients;
	}

	static bool walkableFromTo (edict_t *pPlayer, const Vector& v_src, const Vector& v_dest);

	static void teleportPlayer (const edict_t *pPlayer, const Vector& v_dest);

	static float yawAngleFromEdict(edict_t *pEntity, const Vector& vOrigin);
	//static float getAvoidAngle(edict_t *pEdict,Vector origin);

	// make folders for a file if they don't exist
	static bool makeFolders (const char* szFile);
	// just open file but also make folders if possible
	static std::fstream openFile (const char *szFile, std::ios_base::openmode mode);
	// get the proper location
	static void buildFileName ( char *szOutput, const char *szFile, const char *szFolder = nullptr, const char *szExtension = nullptr, bool bModDependent = false );
	// add a directory delimiter to the string like '/' (linux) or '\\' (windows) or
	static void addDirectoryDelimiter ( char *szString );
	// print a message to client pEntity with bot formatting
	static void botMessage ( edict_t *pEntity, int iErr, const char *fmt, ... );	
	
	static void fixFloatAngle ( float *fAngle );

	static float DotProductFromOrigin ( edict_t *pEnemy, const Vector& pOrigin );
	static float DotProductFromOrigin (const Vector& vPlayer, const Vector& vFacing, const QAngle& eyes );

	static int numPlayersPlaying();
	static int numPlayersOnTeam(int iTeam, bool bAliveOnly);
	static int numBotsOnTeam(int iTeam, bool bAliveOnly);
	static void setMapName ( const char *szMapName );
	static char *getMapName (); 

	static bool IsMapRunning () { return m_bMapRunning; }
	static void setMapRunning (const bool bSet) { m_bMapRunning = bSet; }

	static bool isNetworkable ( edict_t *pEntity );

	static bool entityIsValid ( edict_t *pEntity )
	{
		return pEntity && !pEntity->IsFree() && pEntity->GetNetworkable() != nullptr && pEntity->GetIServerEntity() != nullptr && pEntity->m_NetworkSerialNumber != 0;	
	}

	static void serverSay (const char* fmt, ... );

	static bool isAlivePlayer ( edict_t *pEntity );

	static bool setWaypointDisplayType ( int iType );

	static void fixFloatDegrees360 ( float *pFloat );

	static edict_t *findPlayerByTruncName ( const char *name );

// linux fix
	static CBotMod *getCurrentMod ()
	{
		return m_pCurrentMod;
	}

	////////////////////////////////////////////////////////////////////////
	// useful functions
	static bool boundingBoxTouch2d ( 
		const Vector2D &a1, const Vector2D &a2,
		const Vector2D &bmins, const Vector2D &bmaxs );

	static bool onOppositeSides2d ( 
		const Vector2D &amins, const Vector2D &amaxs,
		const Vector2D &bmins, const Vector2D &bmaxs );

	static bool linesTouching2d ( 
		const Vector2D &amins, const Vector2D &amaxs,
		const Vector2D &bmins, const Vector2D &bmaxs );

	static bool boundingBoxTouch3d (
		const Vector &a1, const Vector &a2,
		const Vector &bmins, const Vector &bmaxs );

	static bool onOppositeSides3d (
		const Vector &amins, const Vector &amaxs,
		const Vector &bmins, const Vector &bmaxs );

	static bool linesTouching3d (
		const Vector &amins, const Vector &amaxs,
		const Vector &bmins, const Vector &bmaxs );

	static float grenadeWillLand (const Vector& vOrigin, const Vector& vEnemy, float fProjSpeed = 400.0f, float fGrenadePrimeTime = 5.0f, const float *fAngle = nullptr);
	////////////////////////////////////////////////////////////////////////

	/*static Vector forwardVec ();
	static Vector rightVec ();
	static Vector upVec ();*/
	////////
	static trace_t *getTraceResult () { return &m_TraceResult; }
	static bool isVisibleHitAllExceptPlayer ( edict_t *pPlayer, const Vector& vSrc, const Vector& vDest, edict_t *pDest = nullptr);
	static bool isVisible ( edict_t *pPlayer, const Vector& vSrc, const Vector& vDest);
	static bool isVisible ( edict_t *pPlayer, const Vector& vSrc, edict_t *pDest);
	static bool isShotVisible ( edict_t *pPlayer, const Vector& vSrc, const Vector& vDest, edict_t *pDest );
	static bool isVisible (const Vector& vSrc, const Vector& vDest);
	static void traceLine (const Vector& vSrc, const Vector& vDest, unsigned mask, ITraceFilter *pFilter);
	static float quickTraceline ( edict_t *pIgnore, const Vector& vSrc, const Vector& vDest ); // return fFraction
	static bool traceVisible (edict_t *pEnt);
	////////
	static Vector entityOrigin ( edict_t *pEntity ) 
	{ 
		return pEntity->GetIServerEntity()->GetCollideable()->GetCollisionOrigin(); 
	}
	static int getTeam ( edict_t *pEntity );
	static bool entityIsAlive ( edict_t *pEntity );
	static bool isBrushEntity( edict_t *pEntity );
	static int countTeamMatesNearOrigin (const Vector& vOrigin, float fRange, int iTeam, const edict_t *pIgnore = nullptr);
	static int numClients ();
	static void levelInit();

	static void setClientMax (const int iMaxClients) { m_iMaxClients = iMaxClients; }

	static void setEventVersion (const int iVersion) { m_iEventVersion = iVersion; }

	static bool isEventVersion (const int iVersion) { return m_iEventVersion == iVersion; }

	static bool getTeamplayOn () { return m_bTeamplay; }

	static void setTeamplay (const bool bOn) { m_bTeamplay = bOn; }

	static bool isMod (const eModId iMod) { return m_iCurrentMod == iMod; }

	static char *modFolder (){return m_szModFolder;}

	static int maxClients () {return m_iMaxClients;}

	static edict_t *playerByUserId(int iUserId);

	static bool isCurrentMod ( eModId modid ); //TODO: not implemented? [APG]RoboCop[CL]

	static bool checkOpensLater (const Vector& vSrc, const Vector& vDest );

	static bool setupMapTime ( ) { return m_fMapStartTime == 0.0f; }

	static bool isBreakableOpen ( edict_t *pBreakable );

	static Vector getVelocity (const edict_t *pPlayer );

	static bool isBoundsDefinedInEntitySpace(edict_t* pEntity)
	{
		const ICollideable* pCollideable = pEntity->GetCollideable(); // Store the result in a local pointer [APG]RoboCop[CL]
		return (pCollideable->GetSolidFlags() & FSOLID_FORCE_WORLD_ALIGNED) == 0 &&
			pCollideable->GetSolid() != SOLID_BBOX &&
			pCollideable->GetSolid() != SOLID_NONE;
	}
	
	static Vector getOBBCenter( edict_t *pEntity );

	static Vector collisionToWorldSpace( const Vector &in, edict_t *pEntity );

	static Vector worldCenter( edict_t *pEntity );

	static bool pointIsWithin( edict_t *pEntity, const Vector &vPoint );

	////////
	static CBotSubcommands *m_pCommands;

	static void readRCBotFolder();
	
	static bool dirExists(const char *path);

	static bool str_is_empty(const char *s) {
		while (*s != '\0') {
			if (!isspace(*s))
				return false;
			s++;
		}

		return true;
	}

	// Entity finding utilities
	static edict_t* findEntityByClassname(edict_t* pStart, const char* szClassname);
	static edict_t* findEntityByClassnameNearest(const Vector& vOrigin, const char* szClassname, float fRadius, edict_t* pStart = nullptr);

private:
	static eModId m_iCurrentMod;
	static CBotMod *m_pCurrentMod;
	static char *m_szModFolder;
	static char m_szMapName[MAX_MAP_STRING_LEN];
	static int m_iDebugLevels;
	static bool m_bMapRunning;
	static trace_t m_TraceResult;
	static int m_iMaxClients;
	static int m_iEventVersion;
	static int m_iWaypointDisplayType;
	static bool m_bTeamplay;
	static float m_fMapStartTime;
	static char *m_szRCBotFolder;

	/*static Vector m_vForward;
	static Vector m_vRight;
	static Vector m_vUp;*/
};

#endif
