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
#ifndef __BOT_EVENT_H__
#define __BOT_EVENT_H__

#include "bot_const.h"

#include <memory>
#include <vector>

class CBotEventInterface;
class IBotEventInterface;

class KeyValues;
class IGameEvent;

class CBotEvent
{
public:
	virtual ~CBotEvent() = default;

	CBotEvent()
	{
		m_pActivator = nullptr;
		m_iEventId = -1;
		m_szType = nullptr;
		m_iModId = MOD_ANY;
	}

	void setMod (const eModId iModId)
	{
		m_iModId = iModId;
	}

	bool forCurrentMod () const;

	void setType (const char* szType);

	inline bool isType ( const char *szType ) const;

	void setActivator ( edict_t *pEdict ) { m_pActivator = pEdict;}

	virtual void execute ( IBotEventInterface *pEvent ) {
	}

	void setEventId (const int iEventId)
	{
		m_iEventId = iEventId;
	}

	bool isEventId (const int iEventId) const
	{
		return forCurrentMod() && m_iEventId == iEventId;
	}

	bool hasEventId () const
	{
		return m_iEventId != -1;
	}

	const char *getName () const
	{
		return m_szType;
	}
protected:
	edict_t *m_pActivator;
private:
	const char *m_szType;
	int m_iEventId;	
	eModId m_iModId;
};

class CRoundStartEvent : public CBotEvent
{
public:
	CRoundStartEvent()
	{
		setType("round_start");
		setMod(MOD_CSS);
	}

	void execute ( IBotEventInterface *pEvent ) override;
};

class CRoundFreezeEndEvent : public CBotEvent
{
public:
	CRoundFreezeEndEvent()
	{
		setType("round_freeze_end");
		setMod(MOD_CSS);
	}

	void execute ( IBotEventInterface *pEvent ) override;
};

class CPostInventoryApplicationTF2 : public CBotEvent
{
public:
	CPostInventoryApplicationTF2()
	{
		setType("post_inventory_application");
		setMod(MOD_TF2);
	}

	void execute ( IBotEventInterface *pEvent ) override;
};

class CTF2RoundWinEvent : public CBotEvent
{
public:
	CTF2RoundWinEvent()
	{
		setType("teamplay_round_win");
		setMod(MOD_TF2);
	}

	void execute ( IBotEventInterface *pEvent ) override;
};

class CPlayerHurtEvent : public CBotEvent
{
public:
	CPlayerHurtEvent()
	{
		setType("player_hurt");
	}

	void execute ( IBotEventInterface *pEvent ) override;
};

class CPlayerDeathEvent : public CBotEvent
{
public:
	CPlayerDeathEvent()
	{
		setType("player_death");
	}

	void execute ( IBotEventInterface *pEvent ) override;
};

class CBombPickupEvent : public CBotEvent
{
public:
	CBombPickupEvent()
	{
		setType("bomb_pickup");
		setMod(MOD_CSS);
	}

	void execute ( IBotEventInterface *pEvent ) override;
};

class CCSSBombPlantedEvent : public CBotEvent
{
public:
	CCSSBombPlantedEvent()
	{
		setType("bomb_planted");
		setMod(MOD_CSS);
	}

	void execute ( IBotEventInterface *pEvent ) override;
};

class CPlayerFootstepEvent : public CBotEvent
{
public:
	CPlayerFootstepEvent()
	{
		setType("player_footstep");
	}

	void execute ( IBotEventInterface *pEvent ) override;
};

class CPlayerSpawnEvent : public CBotEvent
{
public:
	CPlayerSpawnEvent()
	{
		setType("player_spawn");
		setMod(MOD_ANY);
	}
	
	void execute ( IBotEventInterface *pEvent ) override;
};

class CBombDroppedEvent : public CBotEvent
{
public:
	CBombDroppedEvent()
	{
		setType("bomb_dropped");
		setMod(MOD_CSS);
	}

	void execute ( IBotEventInterface *pEvent ) override;
};

class COverTimeBegin : public CBotEvent
{
public:
	COverTimeBegin()
	{
		setType("teamplay_overtime_begin");
		setMod(MOD_TF2);
	}

	void execute ( IBotEventInterface *pEvent ) override;
};

class CPlayerHealed : public CBotEvent
{
public:
	CPlayerHealed()
	{
		setType("player_healed");
		setMod(MOD_TF2);
	}

	void execute ( IBotEventInterface *pEvent ) override;
};

class CPlayerTeleported : public CBotEvent
{
public:
	CPlayerTeleported()
	{
		setType("player_teleported");
		setMod(MOD_TF2);
	}

	void execute ( IBotEventInterface *pEvent ) override;
};

class CWeaponFireEvent : public CBotEvent
{
public:
	CWeaponFireEvent()
	{
		setType("weapon_fire");
	}

	void execute ( IBotEventInterface *pEvent ) override;
};

class CTF2ObjectSapped : public CBotEvent
{
public:
	CTF2ObjectSapped()
	{
		setType("player_sapped_object");
		setMod(MOD_TF2);
	}

	void execute ( IBotEventInterface *pEvent ) override;
};

class CTF2ObjectDestroyed : public CBotEvent
{
public:
	CTF2ObjectDestroyed()
	{
		setType("object_destroyed");
		setMod(MOD_TF2);
	}

	void execute ( IBotEventInterface *pEvent ) override;
};

class CTF2PointCaptured : public CBotEvent
{
public:
	CTF2PointCaptured()
	{
		setType("teamplay_point_captured");
		setMod(MOD_TF2);
	}

	void execute ( IBotEventInterface *pEvent ) override;
};

class CTF2RoundActive : public CBotEvent
{
public:
	CTF2RoundActive()
	{
		setType("teamplay_round_active");
		setMod(MOD_TF2);
	}

	void execute ( IBotEventInterface *pEvent ) override;
};

	

class CTF2PointStopCapture : public CBotEvent
{
public:
	CTF2PointStopCapture()
	{
		setType("teamplay_capture_broken");
		setMod(MOD_TF2);
	}

	void execute ( IBotEventInterface *pEvent ) override;
};

class CTF2PointBlockedCapture : public CBotEvent
{
public:
	CTF2PointBlockedCapture()
	{
		setType("teamplay_capture_blocked");
		setMod(MOD_TF2);
	}

	void execute ( IBotEventInterface *pEvent ) override;
};

class CTF2PointStartCapture : public CBotEvent
{
public:
	CTF2PointStartCapture()
	{
		setType("teamplay_point_startcapture");
		setMod(MOD_TF2);
	}

	void execute ( IBotEventInterface *pEvent ) override;
};


class CTF2MVMWaveFailedEvent : public CBotEvent
{
public:
	CTF2MVMWaveFailedEvent()
	{
		setType("mvm_wave_failed");
		setMod(MOD_TF2);
	}

	void execute ( IBotEventInterface *pEvent ) override;
};

class CTF2MVMWaveCompleteEvent : public CBotEvent
{
public:
	CTF2MVMWaveCompleteEvent()
	{
		setType("mvm_wave_complete");
		setMod(MOD_TF2);
	}

	void execute ( IBotEventInterface *pEvent ) override;
};

class CTF2PointStartTouch : public CBotEvent
{
public:
	CTF2PointStartTouch()
	{
		setType("controlpoint_starttouch");
		setMod(MOD_TF2);
	}

	void execute ( IBotEventInterface *pEvent ) override;
};

class CTF2PointEndTouch : public CBotEvent
{
public:
	CTF2PointEndTouch()
	{
		setType("controlpoint_endtouch");
		setMod(MOD_TF2);
	}

	void execute ( IBotEventInterface *pEvent ) override;
};

class CTF2RoundStart : public CBotEvent
{
public:
	CTF2RoundStart()
	{
		setType("teamplay_round_start");
		setMod(MOD_TF2);
	}

	void execute ( IBotEventInterface *pEvent ) override;
};

class CTF2SetupFinished : public CBotEvent
{
public:
	CTF2SetupFinished()
	{
		setType("teamplay_setup_finished");
		setMod(MOD_TF2);
	}

	void execute ( IBotEventInterface *pEvent ) override;
};

class CBulletImpactEvent : public CBotEvent
{
public:
	CBulletImpactEvent()
	{
		setType("bullet_impact");
		setMod(MOD_TF2);
	}

	void execute ( IBotEventInterface *pEvent ) override;
};

class CTF2ObjectDestroyedEvent : public CBotEvent
{
public:
	CTF2ObjectDestroyedEvent()
	{
		setType("object_destroyed");
		setMod(MOD_TF2);
	}

	void execute ( IBotEventInterface *pEvent ) override;
};

class CTF2BuiltObjectEvent : public CBotEvent
{
public:
	CTF2BuiltObjectEvent()
	{
		setType("player_builtobject");
		setMod(MOD_TF2);
	}

	void execute ( IBotEventInterface *pEvent ) override;
};

class CTF2UpgradeObjectEvent : public CBotEvent
{
public:
	CTF2UpgradeObjectEvent()
	{
		setType("player_upgradedobject");
		setMod(MOD_TF2);
	}

	void execute ( IBotEventInterface *pEvent ) override;
};

class CTF2ChangeClass : public CBotEvent
{
public:
	CTF2ChangeClass()
	{
		setType("player_changeclass");
	}

	void execute ( IBotEventInterface *pEvent ) override;
};

class CBossSummonedEvent : public CBotEvent
{
public:
	CBossSummonedEvent(const char *psztype)
	{
		setType(psztype);
	}

	void execute ( IBotEventInterface *pEvent ) override;
};

class CBossKilledEvent : public CBotEvent
{
public:
	CBossKilledEvent(const char *psztype)
	{
		setType(psztype);
	}

	void execute ( IBotEventInterface *pEvent ) override;

};

class CTF2PointLocked : public CBotEvent
{
public:
	CTF2PointLocked()
	{
		setType("teamplay_point_locked");
		setMod(MOD_TF2);
	}

	void execute ( IBotEventInterface *pEvent ) override;
};

class CTF2PointUnlocked : public CBotEvent
{
public:
	CTF2PointUnlocked()
	{
		setType("teamplay_point_unlocked");
		setMod(MOD_TF2);
	}

	void execute ( IBotEventInterface *pEvent ) override;
};

class CTF2MannVsMachineAlarm : public CBotEvent
{
public:
	CTF2MannVsMachineAlarm()
	{
		setType("mvm_bomb_alarm_triggered");
		setMod(MOD_TF2);
	}

	void execute ( IBotEventInterface *pEvent ) override;
};


class CFlagEvent : public CBotEvent
{
public:
	CFlagEvent()
	{
		setType("teamplay_flag_event");
		setMod(MOD_TF2);
	}

	void execute ( IBotEventInterface *pEvent ) override;
};

class CFlagCaptured : public CBotEvent
{
public:
	CFlagCaptured()
	{
		setType("ctf_flag_captured");
		setMod(MOD_TF2);
	}

	void execute ( IBotEventInterface *pEvent ) override;
};

	/*
	[RCBot] [DEBUG GAME_EVENT] [BEGIN "dod_stats_weapon_attack"]
	[RCBot] [DEBUG GAME_EVENT] 	attacker = 5
	[RCBot] [DEBUG GAME_EVENT] 	weapon = 14
	[RCBot] [DEBUG GAME_EVENT] [END "dod_stats_weapon_attack"]*/
class CDODFireWeaponEvent : public CBotEvent
{
public:
	CDODFireWeaponEvent()
	{
		setType("dod_stats_weapon_attack");
		setMod(MOD_DOD);
	}

	void execute ( IBotEventInterface *pEvent ) override;
};

class CDODBombExploded : public CBotEvent
{
public:
	CDODBombExploded()
	{
		setType("dod_bomb_exploded");
		setMod(MOD_DOD);
	}

	void execute ( IBotEventInterface *pEvent ) override;
};

class CDODBombPlanted : public CBotEvent
{
public:
	CDODBombPlanted()
	{
		setType("dod_bomb_planted");
		setMod(MOD_DOD);
	}

	void execute ( IBotEventInterface *pEvent ) override;
};

class CDODBombDefused : public CBotEvent
{
public:
	CDODBombDefused()
	{
		setType("dod_bomb_defused");
		setMod(MOD_DOD);
	}

	void execute ( IBotEventInterface *pEvent ) override;
};

class CDODPointCaptured : public CBotEvent
{
public:
	CDODPointCaptured()
	{
		setType("dod_point_captured");
		setMod(MOD_DOD);
	}

	void execute ( IBotEventInterface *pEvent ) override;
};

class CDODChangeClass : public CBotEvent
{
public:
	CDODChangeClass()
	{
		setType("player_changeclass");
		setMod(MOD_DOD);
	}

	void execute ( IBotEventInterface *pEvent ) override;
};

class CDODRoundStart : public CBotEvent
{
public:
	CDODRoundStart()
	{
		setType("dod_round_start");
		setMod(MOD_DOD);
	}

	void execute ( IBotEventInterface *pEvent ) override;
};

class CDODRoundActive : public CBotEvent
{
public:
	CDODRoundActive()
	{
		setType("dod_round_active");
		setMod(MOD_DOD);
	}

	void execute ( IBotEventInterface *pEvent ) override;
};

class CDODRoundWin : public CBotEvent
{
public:
	CDODRoundWin()
	{
		setType("dod_round_win");
		setMod(MOD_DOD);
	}

	void execute ( IBotEventInterface *pEvent ) override;
};

class CDODRoundOver : public CBotEvent
{
public:
	CDODRoundOver()
	{
		setType("dod_game_over");
		setMod(MOD_DOD);
	}

	void execute ( IBotEventInterface *pEvent ) override;
};

typedef enum : std::uint8_t
{
	TYPE_KEYVALUES = 0,
	TYPE_IGAMEEVENT = 1
}eBotEventType;

class IBotEventInterface
{
public:
	virtual ~IBotEventInterface() = default;
	virtual float getFloat ( const char *keyName = nullptr, float defaultValue = 0 ) = 0;
	virtual int getInt ( const char *keyName = nullptr, int defaultValue = 0 ) = 0;
	virtual const char *getString ( const char *keyName = nullptr, const char *defaultValue = nullptr ) = 0;
	virtual const char *getName () = 0;
	virtual void setInt ( const char *keyName, int value ) = 0;
};

class CGameEventInterface1 : public IBotEventInterface
{
public:
	CGameEventInterface1 ( KeyValues *pEvent )
	{
		m_pEvent = pEvent;
	}

	float getFloat ( const char *keyName = nullptr, const float defaultValue = 0 ) override
	{
		return m_pEvent->GetFloat(keyName,defaultValue);
	}
	int getInt ( const char *keyName = nullptr, const int defaultValue = 0 ) override
	{
		return m_pEvent->GetInt(keyName,defaultValue);
	}
	void setInt ( const char *keyName, const int value ) override
	{
		m_pEvent->SetInt(keyName,value);
	}
	const char *getString ( const char *keyName = nullptr, const char *defaultValue = nullptr ) override
	{
		return m_pEvent->GetString(keyName,defaultValue);
	}
	const char *getName () override
	{
		return m_pEvent->GetName();
	}

private:
	KeyValues *m_pEvent;
};

class CGameEventInterface2 : public IBotEventInterface
{
public:
	CGameEventInterface2 ( IGameEvent *pEvent )
	{
		m_pEvent = pEvent;
	}

	float getFloat ( const char *keyName = nullptr, const float defaultValue = 0 ) override
	{
		return m_pEvent->GetFloat(keyName,defaultValue);
	}
	int getInt ( const char *keyName = nullptr, const int defaultValue = 0 ) override
	{
		return m_pEvent->GetInt(keyName,defaultValue);
	}
	void setInt ( const char *keyName, const int value ) override
	{
		m_pEvent->SetInt(keyName,value);
	}
	const char *getString ( const char *keyName = nullptr, const char *defaultValue = nullptr ) override
	{
		return m_pEvent->GetString(keyName,defaultValue);
	}
	//Arguments needing filled? [APG]RoboCop[CL]
	const char *getName () override
	{
		return m_pEvent->GetName();
	}
private:
	IGameEvent *m_pEvent;
};

class CBotEvents
{
public:
	static void setupEvents ();

	static void executeEvent( void *pEvent, eBotEventType iType );

	static void freeMemory ();

	static void addEvent ( std::unique_ptr<CBotEvent> pEvent );

private:
	static std::vector<std::unique_ptr<CBotEvent>> m_theEvents;
};
#endif