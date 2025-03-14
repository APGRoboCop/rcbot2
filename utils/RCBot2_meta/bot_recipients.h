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
#ifndef __BOT_RECIPIENTS_H__
#define __BOT_RECIPIENTS_H__

#include <cstddef>
#include <edict.h>

#include "bot_const.h"
#include "engine_wrappers.h"
#include "irecipientfilter.h"

class CEdictRecipient : public IRecipientFilter
{
public:
	CEdictRecipient (edict_t *pEdict, const bool bReliable)
	{
		m_pEdict = pEdict;
		m_bReliable = bReliable;
	}

	// require destructor
	~CEdictRecipient () override
	{
		m_pEdict = nullptr;
		m_bReliable = false;
	}

	bool	IsReliable() const override { return m_bReliable; }

	bool	IsInitMessage() const override { return false; }

	int		GetRecipientCount() const override { return 1; }
	int		GetRecipientIndex( int slot ) const override { return ENTINDEX(m_pEdict); } // only 1 stored
private:
	edict_t *m_pEdict;
	bool m_bReliable;
};
#endif