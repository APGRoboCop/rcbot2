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
#ifndef __RCBOT_WAYPOINT_VISIBILITY_H__
#define __RCBOT_WAYPOINT_VISIBILITY_H__

#include "bot_waypoint.h"

constexpr int g_iMaxVisibilityByte =
(CWaypoints::MAX_WAYPOINTS * CWaypoints::MAX_WAYPOINTS) / 8; // divide by 8 bits, need byte number

typedef struct
{
	int numwaypoints;
	int waypoint_version;
	char szMapName[64];
}wpt_vis_header_t;

class CWaypointVisibilityTable
{
public:
	CWaypointVisibilityTable()
	{
		m_VisTable = nullptr;
		bWorkVisibility = false;
		iCurFrom = 0;
		iCurTo = 0;
		m_iPrevPercent = 0;
		m_fNextShowMessageTime = 0;
	}

	void workVisibility();

	void init()
	{
		constexpr int iSize = g_iMaxVisibilityByte;

		/////////////////////////////
		// for "concurrent" reading of
		// visibility throughout frames
		bWorkVisibility = false;
		m_fNextShowMessageTime = 0;
		iCurFrom = 0;
		iCurTo = 0;
		////////////////////////////

		//create a heap...
		m_VisTable = new unsigned char[iSize];

		m_iPrevPercent = 0;
		memset(m_VisTable, 0, iSize);
	}

	bool SaveToFile() const;

	bool ReadFromFile(int numwaypoints) const;

	void workVisibilityForWaypoint(int i, int iNumWaypoints, bool bTwoway = false) const;

	bool GetVisibilityFromTo(const int iFrom, const int iTo) const
	{
		// work out the position
		const int iPosition = (iFrom * CWaypoints::MAX_WAYPOINTS) + iTo;

		const int iByte = iPosition / 8;
		const int iBit = iPosition % 8;

		if (iByte < g_iMaxVisibilityByte)
		{
			const unsigned char* ToReturn = (m_VisTable + iByte);

			return ((*ToReturn & (1 << iBit)) > 0);
		}

		return false;
	}

	void ClearVisibilityTable()
	{
		if (m_VisTable)
			memset(m_VisTable, 0, g_iMaxVisibilityByte);

		/////////////////////////////
		// for "concurrent" reading of
		// visibility throughout frames
		bWorkVisibility = false;
		iCurFrom = 0;
		iCurTo = 0;
		////////////////////////////
	}

	void FreeVisibilityTable()
	{
		if (m_VisTable != nullptr)
		{
			delete[] m_VisTable;
			m_VisTable = nullptr;
		}

		/////////////////////////////
		// for "concurrent" reading of
		// visibility throughout frames
		bWorkVisibility = false;
		iCurFrom = 0;
		iCurTo = 0;
		////////////////////////////
	}

	void SetVisibilityFromTo(const int iFrom, const int iTo, const bool bVisible) const
	{
		const int iPosition = (iFrom * CWaypoints::MAX_WAYPOINTS) + iTo;

		const int iByte = iPosition / 8;
		const int iBit = iPosition % 8;

		if (iByte < g_iMaxVisibilityByte)
		{
			unsigned char* ToChange = (m_VisTable + iByte);

			if (bVisible)
				*ToChange |= (1 << iBit);
			else
				*ToChange &= ~(1 << iBit);
		}
	}

	void WorkOutVisibilityTable();

	bool needToWorkVisibility() const
	{
		return bWorkVisibility;
	}

	void setWorkVisiblity(const bool bSet)
	{
		bWorkVisibility = bSet;
	}

private:

	bool bWorkVisibility;
	unsigned short int iCurFrom;
	unsigned short int iCurTo;
	static constexpr int WAYPOINT_VIS_TICKS = 64;
	unsigned char* m_VisTable;
	float m_fNextShowMessageTime;
	int m_iPrevPercent;
	// use a heap of 1 byte * size to keep things simple.
};
#endif