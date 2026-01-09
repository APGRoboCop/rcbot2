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

#include "bot.h"
#include "bot_cvars.h"
#include "bot_globals.h"
#include "bot_strings.h"
#include "bot_waypoint_locations.h"
#include "bot_getprop.h"
#include "bot_weapons.h"

#include "ndebugoverlay.h"
//#include "filesystem.h"

#include "rcbot/logging.h"

#ifndef __linux__
#include <direct.h> // for mkdir
#else
#include <fcntl.h>
#endif

#include <sys/stat.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>

//caxanga334: SDK 2013 contains macros for std::min and std::max which causes errors when compiling
//#if SOURCE_ENGINE == SE_SDK2013 || SOURCE_ENGINE == SE_BMS
#include "valve_minmax_off.h"
//#endif

extern IServerGameEnts *servergameents;

///////////
trace_t CBotGlobals :: m_TraceResult;
char * CBotGlobals :: m_szModFolder = nullptr;
eModId CBotGlobals :: m_iCurrentMod = MOD_UNSUPPORTED;
CBotMod *CBotGlobals :: m_pCurrentMod = nullptr;
bool CBotGlobals :: m_bMapRunning = false;
int CBotGlobals :: m_iMaxClients = 0;
int CBotGlobals :: m_iEventVersion = 1;
int CBotGlobals :: m_iWaypointDisplayType = 0;
char CBotGlobals :: m_szMapName[MAX_MAP_STRING_LEN];
bool CBotGlobals :: m_bTeamplay = false;
char *CBotGlobals :: m_szRCBotFolder = nullptr;

///////////

extern IVDebugOverlay *debugoverlay;

class CTraceFilterVis : public CTraceFilter
{
public:
	virtual ~CTraceFilterVis() = default;

	CTraceFilterVis(edict_t *pPlayer, edict_t *pHit = nullptr)
	{
		m_pPlayer = pPlayer;
		m_pHit = pHit;
	}

	bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask ) override
	{ 
		if ( m_pPlayer && pServerEntity == static_cast<IHandleEntity*>(m_pPlayer->GetIServerEntity()) )
			return false;

		if ( m_pHit && pServerEntity == static_cast<IHandleEntity*>(m_pHit->GetIServerEntity()) )
			return false;

		return true; 
	}

	TraceType_t	GetTraceType() const override
	{
		return TRACE_EVERYTHING;
	}
private:
	edict_t *m_pPlayer;
	edict_t *m_pHit;
};

CBotGlobals :: CBotGlobals ()
{
	init();
}

void CBotGlobals :: init ()
{
	m_iCurrentMod = MOD_UNSUPPORTED;
	m_szModFolder[0] = 0;
}

bool CBotGlobals ::isAlivePlayer ( edict_t *pEntity )
{
	return pEntity && ENTINDEX(pEntity) && ENTINDEX(pEntity) <= gpGlobals->maxClients && entityIsAlive(pEntity);
}

//new map
void CBotGlobals :: setMapName ( const char *szMapName ) 
{ 
	std::strncpy(m_szMapName,szMapName,MAX_MAP_STRING_LEN-1); 
	m_szMapName[MAX_MAP_STRING_LEN-1] = 0; 	
}

char *CBotGlobals :: getMapName () 
{ 
	return m_szMapName; 
}

bool CBotGlobals :: isCurrentMod (const eModId modid)
{
	return m_pCurrentMod->getModId() == modid;
}

int CBotGlobals ::numPlayersPlaying()
{
	int num = CBotGlobals::numClients();

	if (rcbot_ignore_spectators.GetBool())
	{
		for ( int i = 1; i <= CBotGlobals::maxClients(); i ++ )
		{
			edict_t* pEdict = INDEXENT(i);

			if ( !pEdict )
				continue;

			if ( CBotGlobals::entityIsValid(pEdict) )
			{
				if ( CClassInterface::getTeam(pEdict) >= 2 )
					continue;
				if ( CBots::getBotPointer(pEdict) != nullptr )
					continue;
				num--;
			}
		}
	}

	return num;
}

int CBotGlobals ::numPlayersOnTeam(const int iTeam, const bool bAliveOnly)
{
	int num = 0;

	for ( int i = 1; i <= CBotGlobals::maxClients(); i ++ )
	{
		edict_t* pEdict = INDEXENT(i);

		if ( !pEdict )
			continue;

		if ( CBotGlobals::entityIsValid(pEdict) )
		{
			if ( CClassInterface::getTeam(pEdict) == iTeam )
			{
				if ( bAliveOnly )
				{
					if ( CBotGlobals::entityIsAlive(pEdict) )
						num++;
				}
				else 
					num++;
			}
		}
	}
	return num;
}

int CBotGlobals ::numBotsOnTeam(const int iTeam, const bool bAliveOnly)
{
	int num = 0;

	for ( int i = 1; i <= CBotGlobals::maxClients(); i ++ )
	{
		edict_t* pEdict = INDEXENT(i);

		if ( !pEdict )
			continue;

		if ( CBotGlobals::entityIsValid(pEdict) && CBots::getBotPointer(pEdict) != nullptr )
		{
			if ( CClassInterface::getTeam(pEdict) == iTeam )
			{
				if ( bAliveOnly )
				{
					if ( CBotGlobals::entityIsAlive(pEdict) )
						num++;
				}
				else
					num++;
			}
		}
	}
	return num;
}

bool CBotGlobals::dirExists(const char *path)
{
#if defined(_WIN64) || defined(_WIN32)

	struct _stat info;

	if (_stat(path, &info) != 0)
		return false;
	if (info.st_mode & _S_IFDIR)
		return true;
	return false;

#elif __linux__

	struct stat info;

	if (stat(path, &info) != 0)
		return false;
	else if (info.st_mode & S_IFDIR)
		return true;
	else
		return false;

#endif
}

void CBotGlobals::readRCBotFolder()
{
	std::unique_ptr<KeyValues, void(*)(KeyValues*)> mainkv(new KeyValues("Metamod Plugin"), [](KeyValues* kv) { kv->deleteThis(); });

	if (mainkv->LoadFromFile(filesystem, "addons/metamod/rcbot2.vdf", "MOD")) {
		char folder[256] = "\0";
		const char* szRCBotFolder = mainkv->GetString("rcbot2path");

		if (szRCBotFolder && *szRCBotFolder) {
			logger->Log(LogLevel::INFO, "RCBot Folder -> trying %s", szRCBotFolder);

			if (!dirExists(szRCBotFolder)) {
				snprintf(folder, sizeof folder, "%s/%s", CBotGlobals::modFolder(), szRCBotFolder);

				szRCBotFolder = CStrings::getString(folder);
				logger->Log(LogLevel::INFO, "RCBot Folder -> trying %s", szRCBotFolder);

				if (!dirExists(szRCBotFolder)) {
					logger->Log(LogLevel::ERROR, "RCBot Folder -> not found ...");
				}
			}

			m_szRCBotFolder = CStrings::getString(szRCBotFolder);
		}
	}
}

float CBotGlobals :: grenadeWillLand (const Vector& vOrigin, const Vector& vEnemy, const float fProjSpeed, const float fGrenadePrimeTime, const float *fAngle)
{
	static float g;
	Vector v_comp = vEnemy-vOrigin;
	const float fDistance = v_comp.Length();

	v_comp = v_comp/fDistance;

	g = sv_gravity.IsValid()? sv_gravity.GetFloat() : 800.0f;

	if ( fAngle == nullptr)
	{
		return 0.0f;
	}
	// use angle -- work out time
	// work out angle
	float vhorz;
	float vvert;

	SinCos(DEG2RAD(*fAngle),&vvert,&vhorz);

	vhorz *= fProjSpeed;
	vvert *= fProjSpeed;

	const float t = fDistance/vhorz;

	// within one second of going off
	if ( std::fabs(t-fGrenadePrimeTime) < 1.0f )
	{
		const float ffinaly =  vOrigin.z + vvert*t - g*0.5f*(t*t);

		return std::fabs(ffinaly - vEnemy.z) < BLAST_RADIUS; // ok why not
	}

	return 0.0f;
}

// TODO :: put in CClients ?
edict_t *CBotGlobals :: findPlayerByTruncName ( const char *name )
// find a player by a truncated name "name".
// e.g. name = "Jo" might find a player called "John"
{
	const std::size_t length = std::strlen(name);
	for( int i = 1; i <= maxClients(); i ++ )
	{
		edict_t* pent = INDEXENT(i);

		if( pent && CBotGlobals::isNetworkable(pent) )
		{
			char arg_lwr[128];
			char pent_lwr[128];

			std::strcpy(arg_lwr,name);

			IPlayerInfo* pInfo = playerinfomanager->GetPlayerInfo(pent);
			
			if ( pInfo == nullptr)
				continue;

			std::strcpy(pent_lwr,pInfo->GetName());

			__strlow(arg_lwr)
			__strlow(pent_lwr)

			if( std::strncmp( arg_lwr,pent_lwr,length) == 0 )
			{
				return pent;
			}
		}
	}

	return nullptr;
}

class CTraceFilterHitAllExceptPlayers : public CTraceFilter
{
public:
	virtual ~CTraceFilterHitAllExceptPlayers() = default;

	bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask ) override
	{ 
		return pServerEntity->GetRefEHandle().GetEntryIndex() <= gpGlobals->maxClients; 
	}
};

//-----------------------------------------------------------------------------
// traceline methods
//-----------------------------------------------------------------------------
class CTraceFilterSimple : public CTraceFilter
{
public:
	virtual ~CTraceFilterSimple() = default;

	CTraceFilterSimple( const IHandleEntity *passentity1, const IHandleEntity *passentity2, const int collisionGroup )
	{
		m_pPassEnt1 = passentity1;
		
		if ( passentity2 )
			m_pPassEnt2 = passentity2;

		m_collisionGroup = collisionGroup;
	}

	bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask ) override
	{
		if ( m_pPassEnt1 == pHandleEntity )
			return false;
		if ( m_pPassEnt2 == pHandleEntity )
			return false;
#if defined(_DEBUG) && !defined(__linux__)
		if ( CClients::clientsDebugging(BOT_DEBUG_VIS) )
		{
			static edict_t *edict;
			
			edict = INDEXENT(pHandleEntity->GetRefEHandle().GetEntryIndex());

			debugoverlay->AddTextOverlayRGB(CBotGlobals::entityOrigin(edict),0,2.0f,255,100,100,200,"Traceline hit %s",edict->GetClassName());
		}
#endif
		return true;
	}
	//virtual void SetPassEntity( const IHandleEntity *pPassEntity ) { m_pPassEnt = pPassEntity; }
	//virtual void SetCollisionGroup( int iCollisionGroup ) { m_collisionGroup = iCollisionGroup; }

	//const IHandleEntity *GetPassEntity(){ return m_pPassEnt;}

private:
	const IHandleEntity *m_pPassEnt1;
	const IHandleEntity *m_pPassEnt2;
	int m_collisionGroup;
};

bool CBotGlobals :: checkOpensLater (const Vector& vSrc, const Vector& vDest)
{
	CTraceFilterSimple traceFilter(nullptr, nullptr, MASK_PLAYERSOLID );

	traceLine (vSrc,vDest,MASK_PLAYERSOLID,&traceFilter);

	return traceVisible(nullptr);
}


bool CBotGlobals :: isVisibleHitAllExceptPlayer ( edict_t *pPlayer, const Vector& vSrc, const Vector& vDest, edict_t *pDest )
{
	const IHandleEntity *ignore = pPlayer->GetIServerEntity();

	CTraceFilterSimple traceFilter( ignore, (pDest== nullptr ? nullptr :pDest->GetIServerEntity()), MASK_ALL );

	traceLine (vSrc,vDest,MASK_SHOT|MASK_VISIBLE,&traceFilter);

	return traceVisible(pDest);
}

bool CBotGlobals :: isVisible (edict_t *pPlayer, const Vector& vSrc, const Vector& vDest)
{
	CTraceFilterWorldAndPropsOnly filter;

	traceLine (vSrc,vDest,MASK_SOLID_BRUSHONLY|CONTENTS_OPAQUE,&filter);

	return traceVisible(nullptr);
}

bool CBotGlobals :: isVisible (edict_t *pPlayer, const Vector& vSrc, edict_t *pDest)
{
	//CTraceFilterWorldAndPropsOnly filter;//	CTraceFilterHitAll filter;

	CTraceFilterWorldAndPropsOnly filter;

	if ( isBrushEntity(pDest) )
		traceLine (vSrc,worldCenter(pDest),MASK_SOLID_BRUSHONLY|CONTENTS_OPAQUE,&filter);
	else
	traceLine (vSrc,entityOrigin(pDest),MASK_SOLID_BRUSHONLY|CONTENTS_OPAQUE,&filter);

	return traceVisible(pDest);
}

bool CBotGlobals :: isShotVisible (edict_t *pPlayer, const Vector& vSrc, const Vector& vDest, edict_t *pDest)
{
	//CTraceFilterWorldAndPropsOnly filter;//	CTraceFilterHitAll filter;

	CTraceFilterVis filter = CTraceFilterVis(pPlayer,pDest);

	traceLine (vSrc,vDest,MASK_SHOT,&filter);

	return traceVisible(pDest);
}

bool CBotGlobals :: isVisible (const Vector& vSrc, const Vector& vDest)
{
	CTraceFilterWorldAndPropsOnly filter;

	traceLine (vSrc,vDest,MASK_SOLID_BRUSHONLY|CONTENTS_OPAQUE,&filter);

	return traceVisible(nullptr);
}

void CBotGlobals :: traceLine (const Vector& vSrc, const Vector& vDest, const unsigned mask, ITraceFilter *pFilter)
{
	Ray_t ray;
	std::memset(&m_TraceResult,0,sizeof(trace_t));
	ray.Init( vSrc, vDest );
	enginetrace->TraceRay( ray, mask, pFilter, &m_TraceResult );
}

float CBotGlobals :: quickTraceline (edict_t *pIgnore, const Vector& vSrc, const Vector& vDest)
{
	CTraceFilterVis filter = CTraceFilterVis(pIgnore);

	Ray_t ray;
	std::memset(&m_TraceResult,0,sizeof(trace_t));
	ray.Init( vSrc, vDest );
	enginetrace->TraceRay( ray, MASK_NPCSOLID_BRUSHONLY, &filter, &m_TraceResult );
	return m_TraceResult.fraction;
}

float CBotGlobals :: DotProductFromOrigin ( edict_t *pEnemy, const Vector& pOrigin )
{
	static Vector vecLOS;
	static float flDot;

	Vector vForward;

	IPlayerInfo* p = playerinfomanager->GetPlayerInfo(pEnemy);

	if (!p )
		return 0;

	const QAngle eyes = p->GetAbsAngles();

	// in fov? Check angle to edict
	AngleVectors(eyes,&vForward);
	
	vecLOS = pOrigin - CBotGlobals::entityOrigin(pEnemy);
	vecLOS = vecLOS/vecLOS.Length();
	
	flDot = DotProduct (vecLOS , vForward );
	
	return flDot; 
}


float CBotGlobals :: DotProductFromOrigin (const Vector& vPlayer, const Vector& vFacing, const QAngle& eyes)
{
	static Vector vecLOS;
	static float flDot;

	Vector vForward;

	// in fov? Check angle to edict
	AngleVectors(eyes,&vForward);
	
	vecLOS = vFacing - vPlayer;
	vecLOS = vecLOS/vecLOS.Length();
	
	flDot = DotProduct (vecLOS , vForward );
	
	return flDot; 
}

bool CBotGlobals :: traceVisible (edict_t *pEnt)
{
	return m_TraceResult.fraction >= 1.0f || (m_TraceResult.m_pEnt && pEnt && m_TraceResult.m_pEnt == pEnt->GetUnknown()->GetBaseEntity());
}

bool CBotGlobals::initModFolder() {
	char szGameFolder[512];
	engine->GetGameDir(szGameFolder, 512);

	const std::size_t iLength = std::strlen(CStrings::getString(szGameFolder));
	std::size_t pos = iLength - 1;

	while (pos > 0 && szGameFolder[pos] != '\\' && szGameFolder[pos] != '/') {
		pos--;
	}
	pos++;

	m_szModFolder = CStrings::getString(&szGameFolder[pos]);
	return true;
}

bool CBotGlobals :: gameStart ()
{
	logger->Log(LogLevel::INFO, "[DIAG-gameStart] Starting gameStart(), engine=%p", static_cast<void*>(engine));

	char szGameFolder[512];
	logger->Log(LogLevel::INFO, "[DIAG-gameStart] Calling engine->GetGameDir...");
	engine->GetGameDir(szGameFolder,512);
	logger->Log(LogLevel::INFO, "[DIAG-gameStart] GetGameDir returned: %s", szGameFolder);
	/*
	CFileSystemPassThru a;
	a.InitPassThru(filesystem,true);
	a.GetCurrentDirectoryA(szSteamFolder,512);
*/
	//filesystem->GetCurrentDirectory(szSteamFolder,512);

	logger->Log(LogLevel::INFO, "[DIAG-gameStart] Calling CStrings::getString...");
	const std::size_t iLength = std::strlen(CStrings::getString(szGameFolder));
	logger->Log(LogLevel::INFO, "[DIAG-gameStart] Game folder length: %zu", iLength);

	std::size_t pos = iLength - 1;

	while ( pos > 0 && szGameFolder[pos] != '\\' && szGameFolder[pos] != '/' )
	{
		pos--;
	}
	pos++;

	m_szModFolder = CStrings::getString(&szGameFolder[pos]);
	logger->Log(LogLevel::INFO, "[DIAG-gameStart] Mod folder: %s", m_szModFolder);

	logger->Log(LogLevel::INFO, "[DIAG-gameStart] Reading mods...");
	CBotMods::readMods();
	logger->Log(LogLevel::INFO, "[DIAG-gameStart] Mods read, getting mod for folder...");

	m_pCurrentMod = CBotMods::getMod(m_szModFolder);
	logger->Log(LogLevel::INFO, "[DIAG-gameStart] Current mod pointer: %p", static_cast<void*>(m_pCurrentMod));

	if ( m_pCurrentMod != nullptr)
	{
		m_iCurrentMod = m_pCurrentMod->getModId();
		logger->Log(LogLevel::INFO, "[DIAG-gameStart] Mod ID: %d, calling initMod()...", static_cast<int>(m_iCurrentMod));

		m_pCurrentMod->initMod();
		logger->Log(LogLevel::INFO, "[DIAG-gameStart] initMod() complete, calling CBots::init()...");

		CBots::init();
		logger->Log(LogLevel::INFO, "[DIAG-gameStart] CBots::init() complete, returning true");

		return true;
	}
	logger->Log(LogLevel::ERROR, "Mod not found. Please edit the bot_mods.ini in the bot config folder (gamedir = %s)",m_szModFolder);

	return false;
}

void CBotGlobals :: levelInit ()
{
}

int CBotGlobals :: countTeamMatesNearOrigin (const Vector& vOrigin, const float fRange, const int iTeam, const edict_t *pIgnore)
{
	int iCount = 0;

	for ( int i = 1; i <= CBotGlobals::maxClients(); i ++ )
	{
		edict_t *pEdict = INDEXENT(i);

		if ( pEdict->IsFree() )
			continue;

		if ( pEdict == pIgnore )
			continue;

		IPlayerInfo* p = playerinfomanager->GetPlayerInfo(pEdict);

		if ( !p || !p->IsConnected() || p->IsDead() || p->IsObserver() || !p->IsPlayer() )
			continue;

		if ( CClassInterface::getTeam(pEdict) == iTeam )
		{
			Vector vPlayer = entityOrigin(pEdict);

			if ( (vOrigin - vPlayer).Length() <= fRange )
				iCount++;
		}
	}

	return iCount;
}

int CBotGlobals :: numClients ()
{
	int iCount = 0;
	int iIndex = 0;

	for ( int i = 1; i <= CBotGlobals::maxClients(); i ++ )
	{
		edict_t *pEdict = INDEXENT(i);

		if ( !pEdict )
			continue;

		IPlayerInfo *p = playerinfomanager->GetPlayerInfo(pEdict);
		if (!p || p->IsHLTV())
			continue;
		
		if ( engine->GetPlayerUserId(pEdict) > 0 ) {
			iIndex = i;
			iCount++;
		}
	}

	return iCount;
}

bool CBotGlobals :: entityIsAlive ( edict_t *pEntity )
{
	static short int index;

	index = ENTINDEX(pEntity);

	if ( index && index <= gpGlobals->maxClients )
	{
		IPlayerInfo *p = playerinfomanager->GetPlayerInfo(pEntity);

		if ( !p )
			return false;

		return !p->IsDead() && p->GetHealth()>0;
	}

	return pEntity->GetIServerEntity() && pEntity->GetClassName() && *pEntity->GetClassName();
	//CBaseEntity *pBaseEntity = CBaseEntity::Instance(pEntity);
	//return pBaseEntity->IsAlive();
}

bool CBotGlobals :: isBrushEntity ( edict_t *pEntity )
{
	const char* szModel = pEntity->GetIServerEntity()->GetModelName().ToCStr();
	return szModel[0] == '*';
}

edict_t *CBotGlobals :: playerByUserId(const int iUserId)
{
	for ( int i = 1; i <= maxClients(); i ++ )
	{
		if ( edict_t *pEdict = INDEXENT(i) )
		{
			if ( engine->GetPlayerUserId(pEdict) == iUserId )
				return pEdict;
		}
	}

	return nullptr;
}

int CBotGlobals :: getTeam ( edict_t *pEntity )
{
	IPlayerInfo *p = playerinfomanager->GetPlayerInfo(pEntity);
	return p->GetTeamIndex();
}

bool CBotGlobals :: isNetworkable ( edict_t *pEntity )
{
	static IServerEntity *pServerEnt;

	pServerEnt = pEntity->GetIServerEntity();

	return pServerEnt && pServerEnt->GetNetworkable() != nullptr;
}

/*
inline Vector CBotGlobals :: entityOrigin ( edict_t *pEntity )
{
	return pEntity->GetIServerEntity()->GetCollideable()->GetCollisionOrigin();
	
	Vector vOrigin;

	if ( pEntity && pEntity->GetIServerEntity() && pEntity->GetIServerEntity()->GetCollideable() )//fix?
		vOrigin = pEntity->GetIServerEntity()->GetCollideable()->GetCollisionOrigin();
	else
		vOrigin = Vector(0,0,0);

	return vOrigin;
}*/

void CBotGlobals::serverSay(const char* fmt, ...)
{
	if (!fmt) return;

	va_list argptr;

	static char string[1024];

	va_start(argptr, fmt);

	std::strcpy(string, "say \"");

	vsnprintf(&string[5], sizeof(string) - 6, fmt, argptr);

	va_end(argptr);

	std::strcat(string, "\"");

	engine->ServerCommand(string);
}

// TODO :: put into CClient
bool CBotGlobals :: setWaypointDisplayType (const int iType)
{
	if ( iType >= 0 && iType <= 1 )
	{
		m_iWaypointDisplayType = iType;
		return true;
	}

	return false;
}

// work on this
bool CBotGlobals :: walkableFromTo (edict_t *pPlayer, const Vector& v_src, const Vector& v_dest)
{
	CTraceFilterVis filter = CTraceFilterVis(pPlayer);
	const float fDistance = std::sqrt((v_dest - v_src).LengthSqr());
	const CClient *pClient = CClients::get(pPlayer);
	Vector vcross = v_dest - v_src;
	float fWidth = rcbot_wptplace_width.GetFloat();

	if ( v_dest == v_src )
		return true;

	// minimum
	fWidth = std::max(fWidth, 2.0f);

	if ( pClient->autoWaypointOn() )
		fWidth = 4.0f;

	vcross = vcross / vcross.Length();
	vcross = vcross.Cross(Vector(0,0,1));
	vcross = vcross * (fWidth*0.5f);

	const Vector vleftsrc = v_src - vcross;
	const Vector vrightsrc = v_src + vcross;

	const Vector vleftdest = v_dest - vcross;
	const Vector vrightdest = v_dest + vcross;

	if ( fDistance > CWaypointLocations::REACHABLE_RANGE )
		return false;

	//if ( !CBotGlobals::isVisible(v_src,v_dest) )
	//	return false;

	// can swim there?
	if (enginetrace->GetPointContents( v_src ) == CONTENTS_WATER &&
		enginetrace->GetPointContents( v_dest ) == CONTENTS_WATER)
	{
		return true;
	}

	// find the ground
	CBotGlobals::traceLine(v_src,v_src-Vector(0,0,256.0f),MASK_NPCSOLID_BRUSHONLY,&filter);
#ifndef __linux__
	debugoverlay->AddLineOverlay(v_src,v_src-Vector(0,0,256.0f),255,0,255,false,3);
#endif
	const Vector v_ground_src = CBotGlobals::getTraceResult()->endpos + Vector(0,0,1);

	CBotGlobals::traceLine(v_dest,v_dest-Vector(0,0,256.0f),MASK_NPCSOLID_BRUSHONLY,&filter);
#ifndef __linux__
	debugoverlay->AddLineOverlay(v_dest,v_dest-Vector(0,0,256.0f),255,255,0,false,3);
#endif
	const Vector v_ground_dest = CBotGlobals::getTraceResult()->endpos + Vector(0,0,1);

	if ( !CBotGlobals::isVisible(pPlayer,v_ground_src,v_ground_dest) )
	{
#ifndef __linux__
		debugoverlay->AddLineOverlay(v_ground_src,v_ground_dest,0,255,255,false,3);		
#endif
		const trace_t *tr = CBotGlobals::getTraceResult();

		// no slope there
		if ( tr->endpos.z > v_src.z )
		{
#ifndef __linux__
			debugoverlay->AddTextOverlay((v_ground_src+v_ground_dest)/2,0,3,"ground fail");
#endif

			CBotGlobals::traceLine(tr->endpos,tr->endpos-Vector(0,0,45),MASK_NPCSOLID_BRUSHONLY,&filter);

			const Vector v_jsrc = tr->endpos;

#ifndef __linux__
			debugoverlay->AddLineOverlay(v_jsrc,v_jsrc-Vector(0,0,45),255,255,255,false,3);	
#endif
			// can't jump there
			if ( v_jsrc.z - tr->endpos.z + (v_dest.z-v_jsrc.z) > 45.0f )
			{
				//if ( (tr->endpos.z > (v_src.z+45)) && (fDistance > 64.0f) )
				//{
#ifndef __linux__
					debugoverlay->AddTextOverlay(tr->endpos,0,3,"jump fail");
#endif
					// check for slope or stairs
					Vector v_norm = v_dest-v_src;
					v_norm = v_norm/std::sqrt(v_norm.LengthSqr());

					for (int iDistCheck = 0; static_cast<float>(iDistCheck) * 45 < fDistance; ++iDistCheck) {
						float fDistCheck = static_cast<float>(iDistCheck) * 45.0f;
					
						Vector v_checkpoint = v_src + v_norm * fDistCheck;

						// check jump height again
						CBotGlobals::traceLine(v_checkpoint,v_checkpoint-Vector(0,0,45.0f),MASK_NPCSOLID_BRUSHONLY,&filter);

						if ( CBotGlobals::traceVisible(nullptr) )
						{
#ifndef __linux__
							debugoverlay->AddTextOverlay(tr->endpos,0,3,"step/jump fail");
#endif
							return false;
						}
					}
				//}
			}
		}
	}

	return CBotGlobals::isVisible(pPlayer,vleftsrc,vleftdest) && CBotGlobals::isVisible(pPlayer,vrightsrc,vrightdest);

	//return true;
}

bool CBotGlobals :: boundingBoxTouch2d ( const Vector2D &a1, const Vector2D &a2,
										const Vector2D &bmins, const Vector2D &bmaxs )
{
	const Vector2D amins = Vector2D(std::min(a1.x, a2.x), std::min(a1.y, a2.y));
	const Vector2D amaxs = Vector2D(std::max(a1.x, a2.x), std::max(a1.y, a2.y));

	return (bmins.x >= amins.x && bmins.y >= amins.y && (bmins.x <= amaxs.x && bmins.y <= amaxs.y)) ||
		(bmaxs.x >= amins.x && bmaxs.y >= amins.y && (bmaxs.x <= amaxs.x && bmaxs.y <= amaxs.y));
}

bool CBotGlobals :: boundingBoxTouch3d ( const Vector &a1, const Vector &a2,
										const Vector &bmins, const Vector &bmaxs )
{
	const Vector amins = Vector(std::min(a1.x, a2.x), std::min(a1.y, a2.y), std::min(a1.z, a2.z));
	const Vector amaxs = Vector(std::max(a1.x, a2.x), std::max(a1.y, a2.y), std::max(a1.z, a2.z));

	return (bmins.x >= amins.x && bmins.y >= amins.y && bmins.z >= amins.z && (bmins.x <= amaxs.x && bmins.y <= amaxs.y && bmins.z <= amaxs.z)) ||
		(bmaxs.x >= amins.x && bmaxs.y >= amins.y && bmaxs.z >= amins.z && (bmaxs.x <= amaxs.x && bmaxs.y <= amaxs.y && bmaxs.z <= amaxs.z));	
}
bool CBotGlobals :: onOppositeSides2d (
		const Vector2D &amins, const Vector2D &amaxs,
		const Vector2D &bmins, const Vector2D &bmaxs )
{
	const float g = (amaxs.x - amins.x) * (bmins.y - amins.y) - 
			(amaxs.y - amins.y) * (bmins.x - amins.x);

	const float h = (amaxs.x - amins.x) * (bmaxs.y - amins.y) - 
			(amaxs.y - amins.y) * (bmaxs.x - amins.x);

  return g * h <= 0.0f;
}

bool CBotGlobals :: onOppositeSides3d (
		const Vector &amins, const Vector &amaxs,
		const Vector &bmins, const Vector &bmaxs )
{
	amins.Cross(bmins);
	amaxs.Cross(bmaxs);

	const float g = (amaxs.x - amins.x) * (bmins.y - amins.y) * (bmins.z - amins.z) - 
			(amaxs.z - amins.z) * (amaxs.y - amins.y) * (bmins.x - amins.x);

	const float h = (amaxs.x - amins.x) * (bmaxs.y - amins.y) * (bmaxs.z - amins.z) - 
			(amaxs.z - amins.z) * (amaxs.y - amins.y) * (bmaxs.x - amins.x);

  return g * h <= 0.0f;
}

bool CBotGlobals :: linesTouching2d (const Vector2D &amins, const Vector2D &amaxs,
										const Vector2D &bmins, const Vector2D &bmaxs)
{
	return onOppositeSides2d(amins,amaxs,bmins,bmaxs) && boundingBoxTouch2d(amins,amaxs,bmins,bmaxs);
}

bool CBotGlobals :: linesTouching3d (const Vector &amins, const Vector &amaxs,
										const Vector &bmins, const Vector &bmaxs)
{
	return onOppositeSides3d(amins,amaxs,bmins,bmaxs) && boundingBoxTouch3d(amins,amaxs,bmins,bmaxs);
}

void CBotGlobals :: botMessage ( edict_t *pEntity, const int iErr, const char *fmt, ... )
{
	va_list argptr; 
	static char string[1024];

	va_start (argptr, fmt);
	vsnprintf(string, sizeof(string), fmt, argptr);
	va_end (argptr); 

	const char *bot_tag = BOT_TAG;
	const std::size_t len = std::strlen(string);
	const std::size_t taglen = std::strlen(BOT_TAG);
	// add tag -- push tag into string
	for ( std::size_t i = len + taglen; i >= taglen; i -- )
		string[i] = string[i-taglen];

	string[len+taglen+1] = 0;

	for ( std::size_t i = 0; i < taglen; i ++ )
		string[i] = bot_tag[i];

	std::strcat(string,"\n");

	if ( pEntity )
	{
		engine->ClientPrintf(pEntity,string);
	}
	else
	{
		if ( iErr )
		{
			Warning("%s", string);
		}
		else
			Msg("%s", string);
	}
}

bool CBotGlobals :: makeFolders (const char* szFile)
{
	//Should be `const char*` to fix strings [APG]RoboCop[CL]
#ifndef __linux__
	const char* delimiter = "\\";
#else
	const char* delimiter = "/";
#endif

	char szFolderName[1024];
	std::size_t folderNameSize = 0;
	szFolderName[0] = 0;

	const std::size_t iLen = std::strlen(szFile);

	std::size_t i = 0;

	while ( i < iLen )
	{
		while ( i < iLen && szFile[i] != *delimiter )
		{
			szFolderName[folderNameSize++]=szFile[i];
			i++;
		}

		if ( i == iLen )
			return true;

		i++;
		szFolderName[folderNameSize++] =* delimiter; //next
		szFolderName[folderNameSize] = 0;
		
#ifndef __linux__
		_mkdir(szFolderName);
#else
		if ( mkdir(szFolderName, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0 ) {
			logger->Log(LogLevel::INFO, "Trying to create folder '%s' successful", szFolderName);
		} else {
			if (dirExists(szFolderName)) {
				logger->Log(LogLevel::DEBUG, "Folder '%s' already exists", szFolderName);
			} else {
				logger->Log(LogLevel::ERROR, "Trying to create folder '%s' failed", szFolderName);
			}
		}
#endif   
	}

	return true;
}

void CBotGlobals :: addDirectoryDelimiter ( char *szString )
{
#ifndef __linux__
	std::strcat(szString,"\\");
#else
	std::strcat(szString,"/");
#endif
}

bool CBotGlobals :: isBreakableOpen ( edict_t *pBreakable )
{
	return (CClassInterface::getEffects(pBreakable) & EF_NODRAW) == EF_NODRAW;
}

Vector CBotGlobals:: getVelocity (const edict_t *pPlayer)
{
	if ( CClient *pClient = CClients::get(pPlayer) )
		return pClient->getVelocity();

	return {0,0,0};
}

/**
 * Clone of CCollisionProperty::OBBCenter( ) --- see game/shared/collisionproperty.h
 * 
 * @param pEntity		Entity to get OBB center
 **/
Vector CBotGlobals::getOBBCenter( edict_t *pEntity )
{
	Vector result = Vector(0,0,0);
	VectorLerp(pEntity->GetCollideable()->OBBMins(), pEntity->GetCollideable()->OBBMaxs(), 0.5f, result);
	return result;
}

Vector CBotGlobals::collisionToWorldSpace( const Vector &in, edict_t *pEntity )
{
	Vector result = Vector(0,0,0);

	if(!isBoundsDefinedInEntitySpace(pEntity) || pEntity->GetCollideable()->GetCollisionAngles() == vec3_angle)
	{
		VectorAdd(in, pEntity->GetCollideable()->GetCollisionOrigin(), result);
	}
	else
	{
		VectorTransform(in, pEntity->GetCollideable()->CollisionToWorldTransform(), result);
	}

	return result;
}

/**
 * Gets the entity world center. Clone of WorldSpaceCenter()
 * @param pEntity	The entity to get the center from
 * @return			Center vector
 **/
Vector CBotGlobals::worldCenter( edict_t *pEntity )
{
	Vector result = getOBBCenter(pEntity);
	result = collisionToWorldSpace(result, pEntity);
	return result;
}

/**
 * Checks if a point is within a trigger
 * 
 * @param pEntity	The trigger entity
 * @param vPoint	The point to be tested
 * @return			True if the given point is within pEntity
 **/
bool CBotGlobals::pointIsWithin( edict_t *pEntity, const Vector &vPoint )
{
	Ray_t ray;
	trace_t tr;
	ICollideable *pCollide = pEntity->GetCollideable();
	ray.Init(vPoint, vPoint);
	enginetrace->ClipRayToCollideable(ray, MASK_ALL, pCollide, &tr);
	return tr.startsolid;
}

std::fstream CBotGlobals::openFile(const char* szFile, const std::ios_base::openmode mode)
{
	std::fstream fp;
	fp.open(szFile, mode);

	if (!fp)
	{
		logger->Log(LogLevel::INFO, "file not found/opening error '%s' mode %d", szFile, mode);

		makeFolders(szFile);

		// try again
		fp.open(szFile, mode);

		if (!fp)
			logger->Log(LogLevel::WARN, "failed to make folders for %s", szFile);
		} else {
		logger->Log(LogLevel::INFO, "Opened file '%s' mode %i", szFile, mode); // `mode %i` fix by caxanga334
	}

	return fp;
}

void CBotGlobals :: buildFileName ( char *szOutput, const char *szFile, const char *szFolder, const char *szExtension, const bool bModDependent )
{
	if (m_szRCBotFolder == nullptr)
	{
#ifdef HOMEFOLDER
		char home[512];
		home[0] = 0;
#endif
		szOutput[0] = 0;

#if defined(HOMEFOLDER) && defined(__linux__)
		char *lhome = getenv ("HOME");

		if (lhome != nullptr) 
		{
			std::strncpy(home,lhome,511);
			home[511] = 0; 
		}
		else
			std::strcpy(home,".");
#endif

#if defined(HOMEFOLDER) && defined(_WIN64) 
		ExpandEnvironmentStringsA("%userprofile%", home, 511);
#endif

#ifdef HOMEFOLDER
		std::strcat(szOutput, home);
		addDirectoryDelimiter(szOutput);
#endif

		/*#ifndef HOMEFOLDER
			std::strcat(szOutput,"..");
			#endif HOMEFOLDER*/

		std::strcat(szOutput, BOT_FOLDER);
	}
	else
		std::strcpy(szOutput, m_szRCBotFolder);

	if (const std::size_t len = std::strlen(szOutput); len > 0 && szOutput[len - 1] != '\\' && szOutput[len - 1] != '/')
		addDirectoryDelimiter(szOutput);

	if ( szFolder )
	{
		std::strcat(szOutput,szFolder);
		addDirectoryDelimiter(szOutput);
	}

	if ( bModDependent )
	{
		std::strcat(szOutput,CBotGlobals::modFolder());
		addDirectoryDelimiter(szOutput);
	}

	std::strcat(szOutput,szFile);

	if ( szExtension )
	{
		std::strcat(szOutput,".");
		std::strcat(szOutput,szExtension);
	}

	//if (m_szRCBotFolder != NULL)
	//	filesystem->RelativePathToFullPath(szOutput, NULL, szOutput, 512, FILTER_CULLPACK);
}

QAngle CBotGlobals::playerAngles ( edict_t *pPlayer )
{
	IPlayerInfo *pPlayerInfo = playerinfomanager->GetPlayerInfo(pPlayer);
	const CBotCmd lastCmd = pPlayerInfo->GetLastUserCommand();
	return lastCmd.viewangles;
}

QAngle CBotGlobals :: entityEyeAngles ( edict_t *pEntity )
{
	return playerinfomanager->GetPlayerInfo(pEntity)->GetAbsAngles();
	//CBaseEntity *pBaseEntity = CBaseEntity::Instance(pEntity);

	//return pBaseEntity->EyeAngles();
}

void CBotGlobals :: fixFloatAngle ( float *fAngle ) 
{ 
	if ( *fAngle > 180 ) 
	{
		*fAngle = *fAngle - 360;
	} 
	else if ( *fAngle < -180 )
	{
		*fAngle = *fAngle + 360;
	}
}

void CBotGlobals :: fixFloatDegrees360 ( float *pFloat )
{
	if ( *pFloat > 360 )
		*pFloat -= 360;
	else if ( *pFloat < 0 )
		*pFloat += 360;
}

float CBotGlobals :: yawAngleFromEdict (edict_t *pEntity, const Vector& vOrigin)
{
	/*
	float fAngle;
	QAngle qBotAngles = entityEyeAngles(pEntity);
	Vector v2;
	Vector v1 = (vOrigin - entityOrigin(pEntity));
	Vector t;

	v1 = v1 / v1.Length();

	AngleVectors(qBotAngles,&v2);

	fAngle = atan2((v1.x*v2.y) - (v1.y*v2.x), (v1.x*v2.x) + (v1.y * v2.y));

	fAngle = RAD2DEG(fAngle);

	return (float)fAngle;*/

	float fAngle;
	float fYaw;
	const QAngle qBotAngles = playerAngles(pEntity);
	QAngle qAngles;
	Vector vPlayerOrigin;

	gameclients->ClientEarPosition(pEntity,&vPlayerOrigin);

	const Vector vAngles = vOrigin - vPlayerOrigin;

	VectorAngles(vAngles/vAngles.Length(),qAngles);

	fYaw = qAngles.y;
	CBotGlobals::fixFloatAngle(&fYaw);

	fAngle = qBotAngles.y - fYaw;

	CBotGlobals::fixFloatAngle(&fAngle);

	return fAngle;

}

void CBotGlobals::teleportPlayer (const edict_t *pPlayer, const Vector& v_dest)
{
	if ( CClient *pClient = CClients::get(pPlayer) )
		pClient->teleportTo(v_dest);
}

edict_t* CBotGlobals::findEntityByClassname(edict_t* pStart, const char* szClassname)
{
	int startIndex = pStart ? ENTINDEX(pStart) + 1 : 0;

	for (int i = startIndex; i < MAX_ENTITIES; i++)
	{
		edict_t* pEdict = engine->PEntityOfEntIndex(i);
		if (!entityIsValid(pEdict))
			continue;

		const char* className = pEdict->GetClassName();
		if (className && Q_stristr(className, szClassname))
			return pEdict;
	}
	return nullptr;
}

edict_t* CBotGlobals::findEntityByClassnameNearest(const Vector& vOrigin, const char* szClassname, float fRadius, edict_t* pStart)
{
	edict_t* pNearest = nullptr;
	float fNearestDist = fRadius * fRadius;

	int startIndex = pStart ? ENTINDEX(pStart) + 1 : 0;

	for (int i = startIndex; i < MAX_ENTITIES; i++)
	{
		edict_t* pEdict = engine->PEntityOfEntIndex(i);
		if (!entityIsValid(pEdict))
			continue;

		const char* className = pEdict->GetClassName();
		if (!className)
			continue;

		// Support wildcard matching (e.g., "weapon_*")
		bool matches = false;
		const char* wildcard = strchr(szClassname, '*');
		if (wildcard)
		{
			size_t prefixLen = wildcard - szClassname;
			matches = (Q_strnicmp(className, szClassname, prefixLen) == 0);
		}
		else
		{
			matches = (Q_stricmp(className, szClassname) == 0);
		}

		if (matches)
		{
			Vector vEntityOrigin = entityOrigin(pEdict);
			float fDist = (vEntityOrigin - vOrigin).LengthSqr();
			if (fDist < fNearestDist)
			{
				fNearestDist = fDist;
				pNearest = pEdict;
			}
		}
	}
	return pNearest;
}

/*

static void TeleportEntity( CBaseEntity *pSourceEntity, TeleportListEntry_t &entry, const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity )
{
	CBaseEntity *pTeleport = entry.pEntity;
	Vector prevOrigin = entry.prevAbsOrigin;
	QAngle prevAngles = entry.prevAbsAngles;

	int nSolidFlags = pTeleport->GetSolidFlags();
	pTeleport->AddSolidFlags( FSOLID_NOT_SOLID );

	// I'm teleporting myself
	if ( pSourceEntity == pTeleport )
	{
		if ( newAngles )
		{
			pTeleport->SetLocalAngles( *newAngles );
			if ( pTeleport->IsPlayer() )
			{
				CBasePlayer *pPlayer = (CBasePlayer *)pTeleport;
				pPlayer->SnapEyeAngles( *newAngles );
			}
		}

		if ( newVelocity )
		{
			pTeleport->SetAbsVelocity( *newVelocity );
			pTeleport->SetBaseVelocity( vec3_origin );
		}

		if ( newPosition )
		{
			pTeleport->AddEffects( EF_NOINTERP );
			UTIL_SetOrigin( pTeleport, *newPosition );
		}
	}
	else
	{
		// My parent is teleporting, just update my position & physics
		pTeleport->CalcAbsolutePosition();
	}
	IPhysicsObject *pPhys = pTeleport->VPhysicsGetObject();
	bool rotatePhysics = false;

	// handle physics objects / shadows
	if ( pPhys )
	{
		if ( newVelocity )
		{
			pPhys->SetVelocity( newVelocity, nullptr );
		}
		const QAngle *rotAngles = &pTeleport->GetAbsAngles();
		// don't rotate physics on players or bbox entities
		if (pTeleport->IsPlayer() || pTeleport->GetSolid() == SOLID_BBOX )
		{
			rotAngles = &vec3_angle;
		}
		else
		{
			rotatePhysics = true;
		}

		pPhys->SetPosition( pTeleport->GetAbsOrigin(), *rotAngles, true );
	}

	g_pNotify->ReportTeleportEvent( pTeleport, prevOrigin, prevAngles, rotatePhysics );

	pTeleport->SetSolidFlags( nSolidFlags );
}
*/
