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
#include "bot_cvars.h"
#include "bot_visibles.h"
#include "bot_globals.h"
#include "bot_profile.h"
#include "bot_client.h"
#include "bot_profiling.h"
#include "bot_getprop.h"

#include "ndebugoverlay.h"

#include <algorithm>
#include <cstring>

//caxanga334: SDK 2013 contains macros for std::min and std::max which causes errors when compiling
//#if SOURCE_ENGINE == SE_SDK2013 || SOURCE_ENGINE == SE_BMS
#include "valve_minmax_off.h"
//#endif

extern IVDebugOverlay *debugoverlay;
////////////////////////////////////////////

byte CBotVisibles :: m_bPvs[MAX_MAP_CLUSTERS/8];

////////////////////////////////////////

/*
void CTF2FindFlagFunc :: execute ( edict_t *pEntity )
{
	if ( m_pBot->
	if ( std::strcmp(pEntity->GetClassName(),"");
}

void CTF2FindFlagFunc :: init ()
{
	m_pBest = nullptr;
	m_fBestFactor = 0;
}*/


////////////////////////////////////////

void CFindEnemyFunc :: execute ( edict_t *pEntity )
{
	if ( m_pBot->isEnemy(pEntity) )
	{
		const float fFactor = getFactor(pEntity);

		if ( !m_pBest || fFactor < m_fBestFactor )
		{
			m_pBest = pEntity;
			m_fBestFactor = fFactor;
		}
	}
}

float CFindEnemyFunc :: getFactor ( edict_t *pEntity ) const
{
	return m_pBot->getEnemyFactor(pEntity);
}

void CFindEnemyFunc :: setOldEnemy ( edict_t *pEntity )
{
	m_pBest = pEntity;
	m_fBestFactor = getFactor(pEntity);
}

void CFindEnemyFunc :: init ()
{
	m_pBest = nullptr;
	m_fBestFactor = 0;
}

///////////////////////////////////////////

CBotVisibles :: CBotVisibles ( CBot *pBot ) 
{
	m_pBot = pBot;
	m_iMaxIndex = m_pBot->maxEntityIndex();
	m_iMaxSize = m_iMaxIndex/8+1;
	m_iIndicesVisible = new unsigned char[m_iMaxSize];
	reset();
}

CBotVisibles :: ~CBotVisibles () 
{
	m_pBot = nullptr;
	delete[] m_iIndicesVisible;
	m_iIndicesVisible = nullptr;
}

void CBotVisibles::eachVisible(CVisibleFunc* pFunc) const
{
    for (edict_t* pEnt : m_VisibleSet)
    {
        pFunc->execute(pEnt);
    }
}

void CBotVisibles::reset()
{
    std::fill_n(m_iIndicesVisible, m_iMaxSize, 0);
    m_VisibleSet.clear();
    m_iCurrentIndex = CBotGlobals::maxClients() + 1;
    m_iCurPlayer = 1;
}

void CBotVisibles::debugString(char* string)
{
    //char szEntities[1024];
    //char szNum[10]{};

    string[0] = 0;

    /**
     * I don't trust this implementation, so I'll just comment it out for now.
     * TODO: modify to use `std::set<T> m_VisibleSet` instead of the now-removed
     * `dataStack<T> m_VisibleList`
     */
     // dataStack<edict_t*> tempStack = m_VisibleList;

     // while ( !tempStack.IsEmpty() )
     // {
         // edict_t *pEnt = tempStack.ChooseFromStack();

         // if ( !pEnt )
             // continue;

         // sprintf(szNum,"%d,",ENTINDEX(pEnt));
         // std::strcat(string,szNum);
     // }
}
/*
@param	pEntity		entity to check
@param	iTicks		the pointer to the bot's traceline ticker
@param	bVisible	returns if the entity is visible or not
@param  iIndex      saves recalling INDEXENT
*/

void CBotVisibles::checkVisible(edict_t* pEntity, int* iTicks, bool* bVisible, const int iIndex, const bool bCheckHead) const
{
	// reset
    *bVisible = false;

    // update
    if (CBotGlobals::entityIsValid(pEntity))
    {
        //if ( CClients::clientsDebugging() && CClients::get(0)->isDebuggingBot(m_pBot) && (ENTINDEX(pEntity)<CBotGlobals::maxClients()) )
        //	debugoverlay->AddLineOverlay(m_pBot->getOrigin(),CBotGlobals::entityOrigin(pEntity),255,255,255,false,1);			

        // if in view cone
        if (m_pBot->FInViewCone(pEntity))
        {
            static Vector vEntityOrigin;
            static int clusterIndex;
            static bool playerInPVS;
			// from Valve developer community wiki
			// http://developer.valvesoftware.com/wiki/Transforming_the_Multiplayer_SDK_into_Coop

            // update tick -- counts the number of PVS done (cpu intensive)
            *iTicks += 1;

            clusterIndex = engine->GetClusterForOrigin(m_pBot->getOrigin());
            engine->GetPVSForCluster(clusterIndex, sizeof m_bPvs, m_bPvs);

            // caxanga334: entityOrigin returns v(0,0,0) for brush entities.
            if (CBotGlobals::isBrushEntity(pEntity))
                vEntityOrigin = CBotGlobals::worldCenter(pEntity);
            else
                vEntityOrigin = CBotGlobals::entityOrigin(pEntity);

            constexpr int Z_OFFSET = 32;

            // for some reason the origin is their feet. add body height
            if (iIndex <= gpGlobals->maxClients)
                vEntityOrigin += Vector(0, 0, Z_OFFSET);

            playerInPVS = engine->CheckOriginInPVS(vEntityOrigin, m_bPvs, sizeof m_bPvs);//engine->CheckBoxInPVS( vectorSurroundMins, vectorSurroundMaxs, m_bPvs, sizeof( m_bPvs ) );

            if (playerInPVS)
            {
                *bVisible = m_pBot->FVisible(pEntity, bCheckHead);

#ifndef __linux__
                if (CClients::clientsDebugging(BOT_DEBUG_VIS) && CClients::get(0)->isDebuggingBot(m_pBot->getEdict()) && ENTINDEX(pEntity) <= CBotGlobals::maxClients())
                {
                    if (*bVisible)
                        debugoverlay->AddTextOverlay(CBotGlobals::entityOrigin(pEntity), 0, 0.1f, "VISIBLE");
                    else
                        debugoverlay->AddTextOverlayRGB(CBotGlobals::entityOrigin(pEntity), 0, 0.1f, 255, 0, 0, 200, "INVISIBLE");
                }
#endif
            }
			//else if ( CClients::clientsDebugging() && CClients::get(0)->isDebuggingBot(m_pBot) && (ENTINDEX(pEntity)<CBotGlobals::maxClients()))
			//	debugoverlay->AddTextOverlay(CBotGlobals::entityOrigin(pEntity),0,0.1,"INVISIBLE: playerInPVS false");
        }
		//else if ( CClients::clientsDebugging() && CClients::get(0)->isDebuggingBot(m_pBot) && (ENTINDEX(pEntity)<CBotGlobals::maxClients()) )
		//	debugoverlay->AddTextOverlay(CBotGlobals::entityOrigin(pEntity),0,0.1,"INVISIBLE: FInViewCone false");
    }
}

void CBotVisibles::updateVisibles()
{
    static bool bVisible;
    static edict_t* pEntity;
    static edict_t* pGroundEntity;

    static int iTicks;
	static int iMaxTicks;  //m_pBot->getProfile()->getVisionTicks();
    static int iStartIndex;
    static int iMaxClientTicks;
    static int iStartPlayerIndex;
    static int iSpecialIndex;

	//update ground entity
    pGroundEntity = CClassInterface::getGroundEntity(m_pBot->getEdict());

    if (pGroundEntity && ENTINDEX(pGroundEntity) > 0)
    {
        setVisible(pGroundEntity, true);
        m_pBot->setVisible(pGroundEntity, true);
    }

    iTicks = 0;

    if (rcbot_supermode.GetBool())
        iMaxTicks = 100;
    else
		iMaxTicks = m_pBot->getProfile()->m_iVisionTicks;// bot_visrevs.GetInt();

    iStartIndex = m_iCurrentIndex;

    if (rcbot_supermode.GetBool())
        iMaxClientTicks = gpGlobals->maxClients / 2 + 1;
    else
		iMaxClientTicks = m_pBot->getProfile()->m_iVisionTicksClients; // bot_visrevs_clients.GetInt();

    iMaxTicks = std::max(iMaxTicks, 2);
    iMaxClientTicks = std::max(iMaxClientTicks, 1);

#ifdef _DEBUG
    CProfileTimer* timer = CProfileTimers::getTimer(BOT_VISION_TIMER);

    if (CClients::clientsDebugging(BOT_DEBUG_PROFILE))
    {
        timer->Start();
    }
#endif

    iStartPlayerIndex = m_iCurPlayer;

    if (m_pBot->moveToIsValid())
    {
        const Vector vMoveTo = m_pBot->getMoveTo();
        if (m_pBot->FVisible(vMoveTo))
            m_pBot->updateCondition(CONDITION_SEE_WAYPOINT);
        else
            m_pBot->removeCondition(CONDITION_SEE_WAYPOINT);
    }

    // we'll start searching some players first for quick player checking
    while (iTicks < iMaxClientTicks)
    {
        pEntity = INDEXENT(m_iCurPlayer);

        if (pEntity != pGroundEntity)
        {
            if (CBotGlobals::entityIsValid(pEntity) && pEntity != m_pBot->getEdict())
            {
                checkVisible(pEntity, &iTicks, &bVisible, m_iCurPlayer);
                setVisible(pEntity, bVisible);
                m_pBot->setVisible(pEntity, bVisible);
            }
        }

        m_iCurPlayer++;

        if (m_iCurPlayer > CBotGlobals::maxClients())
            m_iCurPlayer = 1;

        if (iStartPlayerIndex == m_iCurPlayer)
            break;
    }

    iMaxTicks = std::min(iMaxTicks, m_iMaxIndex);

    if (m_iCurPlayer >= m_iCurrentIndex)
        return;

	// get entities belonging to players too
	// we've captured them elsewhere in another data structure which is quicker to find 
    pEntity = m_pBot->getVisibleSpecial();
    iSpecialIndex = 0;

    if (pEntity)
    {
        if (CBotGlobals::entityIsValid(pEntity))
        {
            iSpecialIndex = ENTINDEX(pEntity);
            checkVisible(pEntity, &iTicks, &bVisible, iSpecialIndex, true);

            setVisible(pEntity, bVisible);
            m_pBot->setVisible(pEntity, bVisible);
        }
    }

    while (iTicks < iMaxTicks)
    {
        bVisible = false;

        pEntity = INDEXENT(m_iCurrentIndex);

        if (pEntity != pGroundEntity && m_iCurrentIndex != iSpecialIndex)
        {
            if (CBotGlobals::entityIsValid(pEntity))
            {
                checkVisible(pEntity, &iTicks, &bVisible, m_iCurrentIndex);

                setVisible(pEntity, bVisible);
                m_pBot->setVisible(pEntity, bVisible);
            }
        }

        m_iCurrentIndex++;

        if (m_iCurrentIndex >= m_iMaxIndex)
            m_iCurrentIndex = CBotGlobals::maxClients() + 1;

		if (m_iCurrentIndex == iStartIndex)
			break; // back to where we started
    }

#ifdef _DEBUG
    if (CClients::clientsDebugging(BOT_DEBUG_PROFILE))
    {
        timer->Stop();
    }
#endif
}

bool CBotVisibles::isVisible(const edict_t* pEdict) const
{
    const int iIndex = ENTINDEX(pEdict) - 1;
    const std::size_t iByte = static_cast<std::size_t>(iIndex / 8); // Use size_t for indexing [APG]RoboCop[CL]
    const int iBit = iIndex % 8;

    if (iIndex < 0 || iByte > m_iMaxSize)
        return false;

    return (m_iIndicesVisible[iByte] & (1 << iBit)) != 0;
}

void CBotVisibles::setVisible(edict_t* pEdict, const bool bVisible)
{
    const int iIndex = ENTINDEX(pEdict) - 1;
	const std::size_t iByte = static_cast<std::size_t>(iIndex / 8); // Use size_t for indexing [APG]RoboCop[CL]
    const int iBit = iIndex % 8;
    const int iFlag = 1 << iBit;

    if (bVisible)
    {
        // visible now
        if ((m_iIndicesVisible[iByte] & iFlag) != iFlag)
            m_VisibleSet.insert(pEdict);

        m_iIndicesVisible[iByte] |= iFlag;
    }
    else
    {
        // not visible anymore
        if (pEdict && (m_iIndicesVisible[iByte] & iFlag) == iFlag)
            m_VisibleSet.erase(pEdict);

        m_iIndicesVisible[iByte] &= ~iFlag;
    }
}
