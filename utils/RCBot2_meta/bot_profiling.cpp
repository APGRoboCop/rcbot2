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
#include "bot_const.h"
#include "bot_profiling.h"
#include "bot_strings.h"
#include "bot_client.h"

#include <algorithm>
#include <chrono>
#include <string>
#include <algorithm>

//caxanga334: SDK 2013 contains macros for std::min and std::max which causes errors when compiling
//#if SOURCE_ENGINE == SE_SDK2013 || SOURCE_ENGINE == SE_BMS
#include "valve_minmax_off.h"
//#endif

 // List of all timers
CProfileTimer CProfileTimers::m_Timers[PROFILING_TIMERS] =
{
    CProfileTimer("CBots::botThink()"), // BOTS_THINK_TIMER
    CProfileTimer("CBot::think()"), // BOT_THINK_TIMER
    CProfileTimer("Nav::findRoute()"), // BOT_ROUTE_TIMER
    CProfileTimer("updateVisibles()") // BOT_VISION_TIMER
};

// Initialise update time
float CProfileTimers::m_fNextUpdate = 0;

// Nuke this on x64
/*#if !defined(PLATFORM_64BITS)

// if windows USE THE QUERYPERFORMANCECOUNTER
#ifdef _WIN32
inline unsigned __int64 RDTSC()
{
    _asm    _emit 0x0F
    _asm    _emit 0x31
}
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
// #define rdtsc _emit  0x0f _asm _emit  0x31
// inline unsigned long long rdtsc()
//    __asm__ volatile (".byte 0x0f, 0x31" : "=A" ());
//    {
//    }
extern __inline__ unsigned long long int rdtsc()
{
    unsigned long long int x;
    __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
    return x;
}
#endif

#endif*/

CProfileTimer::CProfileTimer(const char* szFunction)
    : start_cycle(0),
    end_cycle(0),
    m_average(0),
    m_min(std::numeric_limits<unsigned long long>::max()),
    m_max(0),
    m_last(0),
    m_overall(0),
    m_szFunction(CStrings::getString(szFunction)),
    m_iInvoked(0)
{
}

// "Begin" Timer i.e. update time
void CProfileTimer::Start()
{
    start_cycle = std::chrono::high_resolution_clock::now().time_since_epoch().count();
}

// Stop Timer, work out min/max values and set invoked
void CProfileTimer::Stop()
{
    end_cycle = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    m_last = end_cycle - start_cycle;

    m_max = std::max(m_last, m_max);

    if (m_iInvoked == 0 || m_last < m_min)
        m_min = m_last;

    m_overall += m_last;
    m_iInvoked++;
}

// Print the values, first work out average (use max/min/previous values), and work out percentage of power
void CProfileTimer::print(const double* high)
{
    if (m_iInvoked > 0 && m_szFunction)
    {
        char str[256];

        m_average = m_overall / m_iInvoked;
        const double percent = static_cast<double>(m_overall) / *high * 100.0;

        snprintf(str, 256, "%17s|%13llu|%10llu|%10llu|%10llu|%6.1f", m_szFunction, m_overall, m_min, m_max, m_average, percent);
        CClients::clientDebugMsg(BOT_DEBUG_PROFILE, str);

        // Reset values
        m_iInvoked = 0;
        m_max = 0;
        m_overall = 0;
    }
}

// Get the required timer
CProfileTimer* CProfileTimers::getTimer(const int id)
{
    if (id >= 0 && id < PROFILING_TIMERS)
        return &m_Timers[id];

    return nullptr;
}

// Do this every map start
void CProfileTimers::reset()
{
    m_fNextUpdate = 0;
}

// Update and show every x seconds
void CProfileTimers::updateAndDisplay()
{
    if (CClients::clientsDebugging(BOT_DEBUG_PROFILE))
    {
        if (m_fNextUpdate < engine->Time())
        {
            double highest = 1.0;

            for (const CProfileTimer& m_Timer : m_Timers)
            {
                highest = std::max<double>(static_cast<double>(m_Timer.getOverall()), highest);
            }

            // Next update in 1 second
            m_fNextUpdate = engine->Time() + 1.0f;

            CClients::clientDebugMsg(BOT_DEBUG_PROFILE, "|----------------PROFILING---UPDATE---------------------------------|");
            CClients::clientDebugMsg(BOT_DEBUG_PROFILE, "|------name------|---overall---|---min----|---max----|----avg---|-prct-|");

            for (CProfileTimer& m_Timer : m_Timers)
            {
                m_Timer.print(&highest);
            }
        }
    }
}