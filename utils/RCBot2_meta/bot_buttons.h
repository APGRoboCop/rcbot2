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
#ifndef __BOT_BUTTONS_H__
#define __BOT_BUTTONS_H__

#include <mem.h>
#include <memory>
#include <vector>

class CBotButton
{
public:
	CBotButton (const int iId)
	{
		std::memset(this,0,sizeof(CBotButton));
		m_iButtonId = iId;
		m_bTapped = false;		
	}

	void tap () { m_bTapped = true; }

	bool held (const float fTime) const
	{
		return m_bTapped || (fTime >= m_fTimeStart && fTime <= m_fTimeEnd);// && (!m_fLetGoTime||(fTime > m_fLetGoTime));
	}

	bool canPress (const float fTime) const
	{
		return !m_bTapped || m_fLetGoTime < fTime;
	}

	int getID () const
	{
		return m_iButtonId;
	}

	void letGo ()
	{
		m_fTimeStart = 0.0f;
		m_fTimeEnd = 0.0f;
		m_fLetGoTime = 0.04f; // bit of latency
		m_bTapped = false;
	}

	void unTap () { m_bTapped = false; }

	void hold (float fFrom = 0.0f, float fFor = 1.0f, float fLetGoTime = 0.0f);
private:
	int m_iButtonId;
	float m_fTimeStart;
	float m_fTimeEnd;
	float m_fLetGoTime;

	bool m_bTapped;
};

class CBotButtons
{
public:
	CBotButtons();

	void freeMemory ()
	{
		// Smart pointers automatically delete their contents
		m_theButtons.clear();
	}

	void letGo (int iButtonId) const;
	void holdButton (int iButtonId, float fFrom = 0.0f, float fFor = 1.0f, float fLetGoTime = 0.0f) const;

	inline void add ( std::unique_ptr<CBotButton> theButton );

	bool holdingButton ( int iButtonId ) const;
	bool canPressButton ( int iButtonId ) const;

	void tap ( int iButtonId ) const;

	void letGoAllButtons (const bool bVal) { m_bLetGoAll = bVal; }

	int getBitMask () const;

	////////////////////////////

	void attack (float fFor = 1.0f, float fFrom = 0) const;
	void jump (float fFor = 1.0f, float fFrom = 0) const;
	void duck (float fFor = 1.0f, float fFrom = 0) const;

	// ML Controller helper methods (capitalized wrappers)
	void Reset() { letGoAllButtons(true); }
	void Tap(int iButtonId) const { tap(iButtonId); }
	void Hold(int iButtonId) const { holdButton(iButtonId); }

private:
	std::vector<std::unique_ptr<CBotButton>> m_theButtons;
	bool m_bLetGoAll;
};
#endif