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
#ifndef __RCBOT_SQUADS_H__
#define __RCBOT_SQUADS_H__

#include "vector.h"
#include "bot_ehandle.h"

constexpr float SQUAD_DEFAULT_SPREAD = 80.0f;// say 50 units between each member...?; 

enum eSquadForm : std::uint8_t
{
	SQUAD_FORM_NONE = 0,
	SQUAD_FORM_WEDGE,
	SQUAD_FORM_LINE,
	SQUAD_FORM_ECH_LEFT,
	SQUAD_FORM_ECH_RIGHT,
	SQUAD_FORM_COLUMN,
	SQUAD_FORM_VEE
};

enum eCombatType : std::uint8_t
{
	COMBAT_NONE = 0,
	COMBAT_STEALTH,
	COMBAT_COMBAT,
	COMBAT_CROUCH,
	COMBAT_PRONE
};

enum eTacticType : std::uint8_t
{
	TACTIC_FREE = 0,
	TACTIC_IDLE,
	TACTIC_DEFEND,
	TACTIC_ATTACK
};

class CBotSquad
{
public:

	CBotSquad(edict_t* pLeader, edict_t* pMember)
		: m_fDesiredSpread(0.0f), bCanFire(false), m_CombatType(COMBAT_NONE), m_bIsWaitingForOther(false)                           // Initialize with a default value
	{
		SetLeader(pLeader);
		AddMember(pMember);

		Init();
	}


	~CBotSquad() = default;

	void Init ();

	void ReturnAllToFormation ();

	void SetLeader ( edict_t *pLeader )
	{
		m_pLeader = pLeader;
	}

	edict_t *getMember (std::size_t iMember);

	void ToggleFireMode ()
	{
		bCanFire = !bCanFire;

		/*if ( bCanFire )
			BotPrintTalkMessageOne(m_pLeader.Get(),"Squad can now OPEN FIRE");
		else
			BotPrintTalkMessageOne(m_pLeader.Get(),"Squad is now HOLDING FIRE");*/
	}

	bool SquadCanShoot () const
	{
		return bCanFire;
	}

	bool IsStealthMode () const
	{
		return m_CombatType == COMBAT_STEALTH;
	}

	bool IsProneMode () const
	{
		return m_CombatType == COMBAT_PRONE;
	}

	bool IsCrouchMode () const
	{
		return m_CombatType == COMBAT_CROUCH;
	}

	// Returns true if the squad leader entity is valid. False if not. -caxanga334
	bool IsSquadLeaderValid()
	{
		edict_t* pLeader = m_pLeader.get();

		if (!pLeader || pLeader->IsFree() || pLeader->GetIServerEntity() == nullptr)
		{
			return false;
		}

		return true;
	}

	edict_t *GetLeader ()
	{
		return m_pLeader.get();
	}

	void SetCombatType (const eCombatType iCombatType)
	{
		const edict_t *pLeader = GetLeader();

		m_CombatType = iCombatType;

		if ( !pLeader /*|| (pLeader->v.flags & FL_FAKECLIENT)*/ )
			return;

		char szCombatType[16];

		szCombatType[0] = 0;

		switch ( m_CombatType )
		{
		case COMBAT_STEALTH:
			strcpy(szCombatType,"STEALTH");
			break;
		case COMBAT_PRONE:
			strcpy(szCombatType,"PRONE");
			break;
		case COMBAT_CROUCH:
			strcpy(szCombatType,"CROUCH");
			break;
		case COMBAT_NONE:
		case COMBAT_COMBAT:
			strcpy(szCombatType,"NORMAL");
			break;
		}

		//BotPrintTalkMessageOne ( pLeader, "Combat mode is now %s\n", szCombatType );
	}

	void ChangeLeader ();

	void removeMember (const edict_t* pMember);

	bool IsLeader (const edict_t *pLeader)
	{
		return GetLeader() == pLeader;
	}

	void AddMember ( edict_t *pEdict );

	std::size_t numMembers() const;

	bool IsMember (const edict_t* pEdict);

	bool isFormation (const eSquadForm theFormation) const
	{
		return m_theDesiredFormation == theFormation;
	}

	void ChangeFormation (const eSquadForm theNewFormation)
	{
		m_theDesiredFormation = theNewFormation;
	}

	float GetSpread () const
	{
		return m_fDesiredSpread;
	}

	void ChangeSpread (const float fNewSpread)
	{
		m_fDesiredSpread = fNewSpread;
	}

	int GetFormationPosition (const edict_t* pEdict);

	Vector GetFormationVector (const edict_t* pEdict);

	void UpdateAngles ();

	bool isDefensive () const { return m_Tactics == TACTIC_DEFEND; }

	void setTactic (const eTacticType iTactics) { m_Tactics = iTactics; }

	// Squad is waiting for another squad to Syncronize
	bool isWaitingForOtherSquad () const
	{
		return m_bIsWaitingForOther;
	}

	// Squad is given the go by another squad
	void givenGo ()
	{
		m_bIsWaitingForOther = false;
	}

private:
	// use 'EHandles' for squads
	// as players might leave and stuff...
	MyEHandle m_pLeader;
	std::deque<MyEHandle> m_SquadMembers; // followers?

	eSquadForm m_theDesiredFormation;
	float m_fDesiredSpread;
	bool bCanFire;

	eCombatType m_CombatType;

	QAngle m_vLeaderAngle;
	eTacticType m_Tactics = TACTIC_IDLE;
	bool m_bIsWaitingForOther;
};

//-------------------

class CBotSquads
{
public:

	static void FreeMemory ();

	static void removeSquadMember (CBotSquad* pSquad, const edict_t* pMember);

	// AddSquadMember can have many effects
	// 1. scenario: squad leader exists as squad leader
	//              assign bot to squad
	// 2. scenario: 'squad leader' exists as squad member in another squad
	//              assign bot to 'squad leaders' squad
	// 3. scenario: no squad has 'squad leader' 
	//              make a new squad
	static CBotSquad *AddSquadMember ( edict_t *pLeader, edict_t *pMember );

	static CBotSquad *SquadJoin ( edict_t *pLeader, edict_t *pMember );

	static CBotSquad *FindSquadByLeader (const edict_t *pLeader );

	static void RemoveSquad ( CBotSquad *pSquad );

	static void UpdateAngles ();

	static void ChangeLeader ( CBotSquad *pSquad );

private:
	static std::deque<CBotSquad*> m_theSquads;
};
/*
class CBotSquadE
{
public:
	CBotSquadE ()
	{
		m_fIdleTime = 0.0f;
		m_bIsIdle = false;
	}

	addMember ( edict_t *pEdict )
	{
	}

	void think ()
	{

	}

private:
	edict_t 
	float m_fIdleTime;
	bool m_bIsIdle;
	int m_uMembers;
}

class CBotSquadsE
{
public:
	void think ();
private:
	static dataStack<CBotSquad*> m_theSquads;
}
*/
#endif