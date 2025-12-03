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
#ifndef __HLDM_RCBOT_H__
#define __HLDM_RCBOT_H__

#include "bot_utility.h"

 // bot for HLDM
class CHLDMBot : public CBot
{
public:
	bool handleAttack(CBotWeapon* pWeapon, edict_t* pEnemy) override;

	void handleWeapons() override;

	bool isHLDM() override { return true; }

	void modThink() override;

	void init(bool bVarInit = false) override;
	void setup() override;

	bool startGame() override;

	void died(edict_t* pKiller, const char* pszWeapon) override;
	void killed(edict_t* pVictim, char* weapon) override;

	void spawnInit() override;

	bool isEnemy(edict_t* pEdict, bool bCheckWeapons = true) override;

	void getTasks(unsigned iIgnore = 0) override;
	bool executeAction(eBotAction iAction);

	float getArmorPercent() const { return (0.01f * m_pPlayerInfo->GetArmorValue()); }

	bool setVisible(edict_t* pEntity, bool bVisible) override;

	unsigned maxEntityIndex() override { return gpGlobals->maxEntities; }

	void enemyLost(edict_t* pEnemy) override;

	void setFailedObject(edict_t* pent)
	{
		m_FailedPhysObj = pent;

		if (m_NearestPhysObj == pent)
			m_NearestPhysObj = nullptr;
	}

	bool checkStuck() override;

	bool willCollide(edict_t* pEntity, bool* bCanJump, float* fTime) const;

	edict_t* getFailedObject() const { return m_FailedPhysObj; }

	void touchedWpt(CWaypoint* pWaypoint, int iNextWaypoint = -1, int iPrevWaypoint = -1) override;

	// SourceMod natives getters
	float getSprintTime() const { return m_fSprintTime; }
	edict_t* getCarryingObject() const { return m_pCarryingObject; }
	edict_t* getCurrentWeaponEdict() const { return m_pCurrentWeapon; }
	const MyEHandle& getNearestPhysObj() const { return m_NearestPhysObj; }
	const MyEHandle& getNearestBreakable() const { return m_NearestBreakable; }
	const MyEHandle& getNearbyWeapon() const { return m_pNearbyWeapon; }
	const MyEHandle& getHealthKit() const { return m_pHealthKit; }
	const MyEHandle& getBattery() const { return m_pBattery; }
	MyEHandle& getAmmoCrate() { return m_pAmmoCrate; }
	int getClip1() const { return m_iClip1; }
	int getClip2() const { return m_iClip2; }

private:
	// blah blah
	MyEHandle m_NearestPhysObj;
	MyEHandle m_NearestBreakable;
	edict_t* m_FailedPhysObj = nullptr;
	float m_fSprintTime = 0.0f;
	MyEHandle m_pHealthCharger;
	MyEHandle m_pHealthKit;
	MyEHandle m_pAmmoKit; // nearest healthkit
	MyEHandle m_pBattery; // nearest battery
	MyEHandle m_pCharger; // nearest charger
	MyEHandle m_pNearbyWeapon;
	MyEHandle m_pNearestButton;
	//MyEHandle m_pNearestBreakable;
	MyEHandle m_pAmmoCrate;
	edict_t* m_pCurrentWeapon = nullptr;

	float m_fUseButtonTime = 0.0f;
	float m_fUseCrateTime = 0.0f;

	CBaseHandle* m_Weapons = nullptr;

	float m_fFixWeaponTime = 0.0f;

	int m_iClip1 = 0;
	int m_iClip2 = 0;

	edict_t* m_pCarryingObject = nullptr; // using grav gun
	float m_fCachedNormSpeed = 0.0f; // hl2_normspeed cvar value cache -caxanga334
};

#endif