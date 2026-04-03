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
#include "bot_waypoint.h"
#include "bot_waypoint_visibility.h"
#include "bot_globals.h"
#include "bot_compress.h"

#include <cstdio>
#include <cstring>

#include "rcbot/logging.h"

 /*unsigned char *CWaypointVisibilityTable :: m_VisTable = NULL;
 bool CWaypointVisibilityTable :: bWorkVisibility = false;
 int CWaypointVisibilityTable :: iCurFrom = 0;
 int CWaypointVisibilityTable :: iCurTo = 0;*/

void CWaypointVisibilityTable::workVisibility()
{
	int iTicks = 0;
	const unsigned short int iSize = static_cast<unsigned short int>(CWaypoints::numWaypoints());

	for (/*iCurFrom = iCurFrom*/; iCurFrom < iSize; iCurFrom++)
	{
		for (/*iCurTo = iCurTo*/; iCurTo < iSize; iCurTo++)
		{
			CWaypoint* pWaypoint1 = CWaypoints::getWaypoint(iCurFrom);
			CWaypoint* pWaypoint2 = CWaypoints::getWaypoint(iCurTo);

			SetVisibilityFromTo(iCurFrom, iCurTo,
				CBotGlobals::isVisible(pWaypoint1->getOrigin(), pWaypoint2->getOrigin()));

			iTicks++;

			if (iTicks >= WAYPOINT_VIS_TICKS)
			{
				if (m_fNextShowMessageTime < engine->Time())
				{
					const int percent = (iCurFrom / iSize) * 100;

					if (m_iPrevPercent != percent)
					{
						logger->Log(LogLevel::INFO, "Working out visibility... %d%%", percent);
						m_fNextShowMessageTime = engine->Time() + 2.5f;
						m_iPrevPercent = percent;
					}
				}

				return;
			}
		}

		iCurTo = 0;
	}

	if (iCurFrom == iSize)
	{
		// finished
		logger->Log(LogLevel::INFO, "Finished working out visibility. Saving...");
		/////////////////////////////
		// for "concurrent" reading of
		// visibility throughout frames
		bWorkVisibility = false;
		iCurFrom = 0;
		iCurTo = 0;

		// save waypoints with visibility flag now
		if (SaveToFile())
		{
			CWaypoints::save(true);
			logger->Log(LogLevel::INFO, "Saved waypoints with visibility information");
		}
		else
			logger->Log(LogLevel::ERROR, "Couldn't save waypoints with visibility information");
		////////////////////////////
	}
}

void CWaypointVisibilityTable::workVisibilityForWaypoint(const int i, const int iNumWaypoints, const bool bTwoway) const
{
	static CWaypoint* Waypoint1;
	static CWaypoint* Waypoint2;
	static bool bVisible;

	Waypoint1 = CWaypoints::getWaypoint(i);

	if (!Waypoint1->isUsed())
		return;

	for (int j = 0; j < iNumWaypoints; j++)
	{
		if (i == j)
		{
			SetVisibilityFromTo(i, j, true);
			continue;
		}

		Waypoint2 = CWaypoints::getWaypoint(j);

		if (!Waypoint2->isUsed())
			continue;

		bVisible = CBotGlobals::isVisible(Waypoint1->getOrigin(), Waypoint2->getOrigin());

		SetVisibilityFromTo(i, j, bVisible);

		if (bTwoway)
			SetVisibilityFromTo(j, i, bVisible);
	}
}

void CWaypointVisibilityTable::WorkOutVisibilityTable()
{
	const int iNumWaypoints = CWaypoints::numWaypoints();

	ClearVisibilityTable();

	// loop through all waypoint possibilities.
	for (int i = 0; i < iNumWaypoints; i++)
	{
		workVisibilityForWaypoint(i, iNumWaypoints, false);
	}
}

bool CWaypointVisibilityTable::SaveToFile() const
{
	char filename[1024];
	wpt_vis_header_t header;

	CBotGlobals::buildFileName(filename, CBotGlobals::getMapName(), BOT_AUXILERY_FOLDER, BOT_VISIBILITY_EXTENSION, true);

	header.numwaypoints = CWaypoints::numWaypoints();
	std::strncpy(header.szMapName, CBotGlobals::getMapName(), 63);
	header.waypoint_version = CWaypoints::WAYPOINT_VERSION;

	// Build a combined buffer: header + vis table data
	const std::size_t totalSize = sizeof(wpt_vis_header_t) + sizeof(byte) * g_iMaxVisibilityByte;
	unsigned char* pBuf = static_cast<unsigned char*>(std::malloc(totalSize));

	if (!pBuf)
	{
		logger->Log(LogLevel::ERROR, "Can't allocate buffer for Waypoint Visibility save!");
		return false;
	}

	std::memcpy(pBuf, &header, sizeof(wpt_vis_header_t));
	std::memcpy(pBuf + sizeof(wpt_vis_header_t), m_VisTable, sizeof(byte) * g_iMaxVisibilityByte);

	CBotGlobals::makeFolders(filename);
	const bool bResult = RCBot_CompressedSave(filename, pBuf, totalSize);

	std::free(pBuf);

	if (!bResult)
		logger->Log(LogLevel::ERROR, "Can't save Waypoint Visibility table!");

	return bResult;
}

bool CWaypointVisibilityTable::ReadFromFile(const int numwaypoints) const
{
	char filename[1024];

	wpt_vis_header_t header;

	CBotGlobals::buildFileName(filename, CBotGlobals::getMapName(), BOT_AUXILERY_FOLDER, BOT_VISIBILITY_EXTENSION, true);

	const std::size_t totalSize = sizeof(wpt_vis_header_t) + sizeof(byte) * g_iMaxVisibilityByte;
	unsigned char* pBuf = static_cast<unsigned char*>(std::malloc(totalSize));

	if (!pBuf)
	{
		logger->Log(LogLevel::ERROR, "Can't allocate buffer for Waypoint Visibility load!");
		return false;
	}

	if (!RCBot_CompressedLoad(filename, pBuf, totalSize))
	{
		std::free(pBuf);
		return false;
	}

	std::memcpy(&header, pBuf, sizeof(wpt_vis_header_t));

	if (header.numwaypoints != numwaypoints ||
		header.waypoint_version != CWaypoints::WAYPOINT_VERSION ||
		std::strncmp(header.szMapName, CBotGlobals::getMapName(), 63) != 0)
	{
		std::free(pBuf);
		return false;
	}

	std::memcpy(m_VisTable, pBuf + sizeof(wpt_vis_header_t), sizeof(byte) * g_iMaxVisibilityByte);

	std::free(pBuf);
	return true;
}