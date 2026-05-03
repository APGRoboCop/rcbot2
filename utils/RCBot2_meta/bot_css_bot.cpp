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
#include "ndebugoverlay.h"
#include "bot_squads.h"
#include "bot_css_bot.h"
#include "bot_css_buying.h"
#include "in_buttons.h"
#include "bot_buttons.h"
#include "bot_globals.h"
#include "bot_profile.h"
#include "bot_getprop.h"
#include "bot_mtrand.h"
#include "bot_mods.h"
#include "bot_task.h"
#include "bot_schedule.h"
#include "bot_weapons.h"
#include "bot_waypoint.h"
#include "bot_waypoint_locations.h"
#include "bot_navigator.h"
#include "bot_perceptron.h"
#include "bot_plugin_meta.h"
#include "bot_waypoint_visibility.h"
#include "bot_visibles.h"

#include "rcbot/logging.h"

#include <cstring>

extern IServerGameEnts *servergameents; // for accessing the server game entities

void CCSSBot::init(const bool bVarInit)
{
	CBot::init(bVarInit);// require this

	// initialize stuff for counter-strike source bot
	m_pBuyManager = nullptr;
}

void CCSSBot::setup()
{
	CBot::setup(); // require this

	// setup data structures for counter-strike source bot
	// this only gets called ONCE (when bot joins the game)

	engine->SetFakeClientConVarValue(m_pEdict,"cl_team","default");
	engine->SetFakeClientConVarValue(m_pEdict,"cl_autohelp","0");

	m_pBuyManager = new CCSSBotBuying(this);
}

void CCSSBot::freeMapMemory()
{
	CBot::freeMapMemory();

	if(m_pBuyManager != nullptr)
	{
		m_pBuyManager->reset();
		delete m_pBuyManager;
		m_pBuyManager = nullptr;
	}
}

void CCSSBot::updateConditions()
{
	CBot::updateConditions();

	if(m_pEnemy.get() != nullptr)
	{
		if(m_pVisibles->isVisible(m_pEnemy))
		{
			m_fVisibleEnemyTime = engine->Time();
		}
		else if(m_fVisibleEnemyTime + 3.0f <= engine->Time())
		{
			enemyLost(m_pEnemy);
			setLastEnemy(m_pEnemy);
			m_pEnemy = nullptr;
		}

		if(engine->IndexOfEdict(m_pEnemy.get()) <= gpGlobals->maxClients)
		{
			// CSS Hack: Dead players are always "Alive" with 1 health.
			if(CClassInterface::getPlayerLifeState(m_pEnemy.get()) == LIFE_DEAD || CClassInterface::getPlayerLifeState(m_pEnemy.get()) == LIFE_DYING)
			{
				updateCondition(CONDITION_ENEMY_DEAD);
				m_pEnemy = nullptr;
			}
		}
	}
}

bool CCSSBot::isAlive()
{
	if (!CBot::isAlive())
		return false;

	if(CClassInterface::getPlayerLifeState(getEdict()) == LIFE_DEAD || CClassInterface::getPlayerLifeState(getEdict()) == LIFE_DYING)
		return false;

	return getOrigin() != Vector(0,0,0);
}

bool CCSSBot::isEnemy(edict_t *pEdict,bool bCheckWeapons)
{
	if(rcbot_notarget.GetBool())
		return false;

	if(ENTINDEX(pEdict) > CBotGlobals::maxClients())
	{
		if (pEdict->GetUnknown() && pEdict == INDEXENT(m_hNearestBreakable.GetEntryIndex()) && CClassInterface::getPlayerHealth(pEdict) > 0)
		{
			if (distanceFrom(CBotGlobals::worldCenter(pEdict)) < (rcbot_jump_obst_dist.GetFloat() * 2))
			{
				if (BotFunc_BreakableIsEnemy(INDEXENT(m_hNearestBreakable.GetEntryIndex()), pEdict) ||
					((CBotGlobals::worldCenter(pEdict) - m_vMoveTo).Length() + 48) < (getOrigin() - m_vMoveTo).Length())
				{
					return true;
				}
			}
		}
		return false;
	}

	if(pEdict->IsFree())
		return false;

	if(!CBotGlobals::isNetworkable(pEdict))
		return false;
 
	IPlayerInfo *p = playerinfomanager->GetPlayerInfo(pEdict);

	if(p == nullptr)
		return false;

	if(m_pEdict == pEdict)
		return false;

	if(!CBotGlobals::entityIsAlive(pEdict))
		return false;

	if(CClassInterface::getPlayerLifeState(pEdict) == LIFE_DEAD || CClassInterface::getPlayerLifeState(pEdict) == LIFE_DYING)
		return false;

	return p->GetTeamIndex() != getTeam();
}

bool CCSSBot::startGame()
{
	const int team = m_pPlayerInfo->GetTeamIndex();

	if(team <= CS_TEAM_SPECTATOR)
	{
		selectTeam();
		selectModel();
	}

	return true;
}

void CCSSBot::died(edict_t *pKiller, const char *pszWeapon)
{
	spawnInit();
	if(m_pBuyManager)
	{
		m_pBuyManager->onDeath();
}
}

void CCSSBot::spawnInit()
{
	CBot::spawnInit();

	if(m_pBuyManager)
	{
		m_pBuyManager->update();
	}

	m_fVisibleEnemyTime = 0.0f;
	m_pCurrentWeapon = nullptr;
	m_fNextAttackTime = engine->Time();
	m_fCheckStuckTime = engine->Time() + 6.0f;
	m_fNextThinkSlow = engine->Time() + 1.0f;
	m_hNearestBreakable.Term();
	updateCondition(CONDITION_CHANGED); // Re-execute the utility system
}

void CCSSBot::listenForPlayers()
{
	edict_t *pListenNearest = nullptr;
	float fMaxFactor = 0.0f;
	Vector vVelocity;
	bool bIsNearestAttacking = false;

	if(m_bListenPositionValid && m_fListenTime > engine->Time()) // already listening to something ?
	{
		setLookAtTask(LOOK_NOISE);
		return;
	}

	m_bListenPositionValid = false;

	for(int i = 1; i <= gpGlobals->maxClients; i ++)
	{
		edict_t* pPlayer = INDEXENT(i);

		if(pPlayer == m_pEdict)
			continue; // don't listen to self

		if(pPlayer->IsFree())
			continue;

		const CClient* pClient = CClients::get(pPlayer);

		if(!pClient->isUsed())
			continue;

		IPlayerInfo* p = playerinfomanager->GetPlayerInfo(pPlayer);

		// 05/07/09 fix crash bug
		if(!p || !p->IsConnected() || p->IsDead() || p->IsObserver() || !p->IsPlayer())
			continue;

		if(CClassInterface::getPlayerLifeState(pPlayer) == LIFE_DEAD || CClassInterface::getPlayerLifeState(pPlayer) == LIFE_DYING)
			continue;

		// Ignore teammates
		if(p->GetTeamIndex() == getTeam())
			continue;

		const float fDist = distanceFrom(pPlayer);

		if(fDist > rcbot_listen_dist.GetFloat())
			continue;
		
		float fFactor = 0.0f;

		const CBotCmd lastUserCommand = p->GetLastUserCommand(); // Renamed from `cmd` to `lastUserCommand`to avoid conflict - [APG]RoboCop[CL]
		if (lastUserCommand.buttons & IN_ATTACK)
		{
			if (wantToListenToPlayerAttack(pPlayer))
				fFactor += 1000.0f;
		}
		// Later in the code
		if (fFactor > fMaxFactor)
		{
			fMaxFactor = fFactor;
			pListenNearest = pPlayer;
			bIsNearestAttacking = lastUserCommand.buttons & IN_ATTACK; // Updated variable name
		}

		if(cmd.buttons & IN_ATTACK)
		{
			if(wantToListenToPlayerAttack(pPlayer))
				fFactor += 1000.0f;
		}
		
		// can't see this player and I'm on my own
		if(wantToListenToPlayerFootsteps(pPlayer) && !isVisible(pPlayer) && ( m_bStatsCanUse && m_StatsCanUse.stats.m_iTeamMatesVisible==0))
		{
			CClassInterface::getVelocity(pPlayer,&vVelocity);

			const float fVelocity = vVelocity.Length();

			if(fVelocity > rcbot_footstep_speed.GetFloat())
				fFactor += vVelocity.Length();
		}

		if(fFactor == 0.0f)
			continue;

		// add inverted distance to the factor (i.e. closer = better)
		fFactor += rcbot_listen_dist.GetFloat() - fDist;

		if(fFactor > fMaxFactor)
		{
			fMaxFactor = fFactor;
			pListenNearest = pPlayer;
			bIsNearestAttacking = cmd.buttons & IN_ATTACK;
		}
	}

	if(pListenNearest != nullptr)
	{
		listenToPlayer(pListenNearest,false,bIsNearestAttacking);
	}
}

void CCSSBot::selectTeam() const
{
	const char* cmd = "jointeam 0";
	helpers->ClientCommand(m_pEdict,cmd);
}

void CCSSBot::selectModel() const
{
	const char* cmd = "joinclass 0";
	helpers->ClientCommand(m_pEdict,cmd);
}

/**
 * Executes the 'say' command
 * 
 * @param message		The message the bot will say
 * @return				No return
 **/
void CCSSBot::say(const char *message)
{
	char buffer[256];
	snprintf(buffer, sizeof(buffer), "say \"%s\"", message);
	helpers->ClientCommand(m_pEdict,buffer);
}

/**
 * Executes the 'say_team' command
 * 
 * @param message		The message the bot will say
 * @return				No return
 **/
void CCSSBot::sayteam(const char *message)
{
	char buffer[256];
	snprintf(buffer, sizeof(buffer), "say_team \"%s\"", message);
	helpers->ClientCommand(m_pEdict,buffer);
}

/**
 * Executes the buy logic
 **/
void CCSSBot::runBuy()
{
	m_pBuyManager->update();
	m_pBuyManager->execute();
}

/**
 * Update visible entities
 **/
bool CCSSBot::setVisible(edict_t *pEntity, const bool bVisible)
{
	static float fDist;

	const bool bValid = CBot::setVisible(pEntity, bVisible);

	if (CBotGlobals::isBrushEntity(pEntity))
		fDist = distanceFrom(CBotGlobals::worldCenter(pEntity));
		else
		fDist = distanceFrom(pEntity);

	// if no draw effect it is invisible
	if (bValid && bVisible && !(CClassInterface::getEffects(pEntity) & EF_NODRAW))
	{
		const char* szClassname = pEntity->GetClassName();

		if ((std::strncmp(szClassname, "func_breakable", 14) == 0 || std::strncmp(szClassname, "func_breakable_surf", 19) == 0))
		{
			if ((INDEXENT(m_hNearestBreakable.GetEntryIndex()) == nullptr || fDist < distanceFrom(CBotGlobals::worldCenter(INDEXENT(m_hNearestBreakable.GetEntryIndex())))))
			{
				m_hNearestBreakable.Init(engine->IndexOfEdict(pEntity), pEntity->m_NetworkSerialNumber);
			}
		}
	}
	else
	{
		if (INDEXENT(m_hNearestBreakable.GetEntryIndex()) == pEntity)
			m_hNearestBreakable.Term();
	}

	return bValid;
}

/**
 * Gets the bot primary weapon (will fallback to secondary if the bot doesn't have a primary)
 **/
CBotWeapon *CCSSBot::getPrimaryWeapon()
{
	static CBotWeapon *primary;
	static CBotWeapon *secondary;
	primary = m_pWeapons->getCurrentWeaponInSlot(CS_WEAPON_SLOT_PRIMARY);
	secondary = m_pWeapons->getCurrentWeaponInSlot(CS_WEAPON_SLOT_SECONDARY);

	if(primary)
		return primary;

	if(secondary)
		return secondary;

	return nullptr;
}

/**
 * Checks if the bot is carrying a sniper weapon
 * 
 * @return		TRUE if the bot is carrying a sniper, FALSE otherwise
 **/
bool CCSSBot::IsSniper()
{
	const CBotWeapon *weapon = getPrimaryWeapon();

	if(!weapon)
		return false;

	switch (weapon->getID())
	{
		case CS_WEAPON_AWP:
		case CS_WEAPON_SCOUT:
		case CS_WEAPON_G3SG1:
		case CS_WEAPON_SG550:
		{
			return true;
		}
	}

	return false;
}

/**
 * Custom primary attack function for Counter-Strike: Source bots
 * 
 * @param hold		Hold the attack button? (full-auto)
 * @return			No return
 **/
void CCSSBot::primaryattackCS(const bool hold)
{
	if(hold)
	{
		primaryAttack(hold);
	}
	else
	{
		if(m_fNextAttackTime <= engine->Time())
		{
			tapButton(IN_ATTACK);
			//CClients::clientDebugMsg(this, BOT_DEBUG_AIM, "[CSS-ATTACK] Primary Fire!");
			m_fNextAttackTime = engine->Time() + getNextAttackDelay(); // 50 ms delay between shots
		}
		else
		{
			//CClients::clientDebugMsg(this, BOT_DEBUG_AIM, "[CSS-ATTACK] Wait!");
			letGoOfButton(IN_ATTACK);
		}
	}
}

void CCSSBot::handleWeapons()
{
	//
	// Handle attacking at this point
	//
	if (m_pEnemy && !hasSomeConditions(CONDITION_ENEMY_DEAD) && 
		hasSomeConditions(CONDITION_SEE_CUR_ENEMY) && wantToShoot() && 
		isVisible(m_pEnemy) && isEnemy(m_pEnemy))
	{
		CBotWeapon* pWeapon = getBestWeapon(m_pEnemy, true, true, rcbot_melee_only.GetBool());

		if(m_bWantToChangeWeapon && pWeapon != nullptr && pWeapon != getCurrentWeapon() && pWeapon->getWeaponIndex())
		{
			selectWeapon(pWeapon->getWeaponIndex());
		}

		setLookAtTask(LOOK_ENEMY);

		if(!handleAttack(pWeapon, m_pEnemy))
		{
			m_pEnemy = nullptr;
			m_pOldEnemy = nullptr;
			wantToShoot(false);
		}
	}
}

bool CCSSBot::handleAttack(CBotWeapon *pWeapon, edict_t *pEnemy)
{
	if(pWeapon)
	{
		clearFailedWeaponSelect();

		if(pWeapon->isMelee())
		{
			if (CBotGlobals::isBrushEntity(pEnemy))
			{
				setMoveTo(CBotGlobals::worldCenter(pEnemy));
			}
			else
			{
			setMoveTo(CBotGlobals::entityOrigin(pEnemy));
			}
			setMoveSpeed(CClassInterface::getMaxSpeed(m_pEdict)); // in case some task changed the move speed
		}

		if(pWeapon->isZoomable() && !CCounterStrikeSourceMod::isScoped(this))
			secondaryAttack(false);

		primaryattackCS(false);
	}
	else
	{
		primaryattackCS(false);
	}

	return true;
}

float CCSSBot::getNextAttackDelay()
{
	static constexpr float max = 4096.0f;
	static float dist;
	static float delay;
	delay = 0.050f; // Base delay

	dist = distanceFrom(getEnemy());
	delay = dist/max;
	//clamp(delay, 0.050f, 0.300f);
	delay = clamp(delay, 0.050f, 0.300f); // Uses the macro from mathlib.h [APG]RoboCop[CL]

	//CClients::clientDebugMsg(this, BOT_DEBUG_AIM, "[CSS-ATTACK] Next Attack Delay: %2.4f", delay);

	return delay;
}

void CCSSBot::modAim(edict_t *pEntity, Vector &v_origin, Vector *v_desired_offset, Vector &v_size, const float fDist, const float fDist2D)
{
	static bool aimforhead;
	static CBotWeapon* pWp;

	CBot::modAim(pEntity, v_origin, v_desired_offset, v_size, fDist, fDist2D);

	aimforhead = true;
	pWp = getCurrentWeapon();

	if (pWp) // Check if pWp is not nullptr - [APG]RoboCop[CL]
	{
		switch (pWp->getID())
		{
		case CS_WEAPON_AWP:
		case CS_WEAPON_SUPERSHOTGUN:
		case CS_WEAPON_AUTOSHOTGUN:
			{
				aimforhead = false;
				break;
			}
		}

		if (pWp->isMelee())
		{
			aimforhead = false;
		}
	}

	if(hasSomeConditions(CONDITION_SEE_ENEMY_HEAD) && aimforhead)
	{
		v_desired_offset->z = v_desired_offset->z + (v_size.z-1);
	}
	else
	{
		v_desired_offset->z = v_desired_offset->z + (v_size.z-16);
	}
	
	if (pEntity == INDEXENT(m_hNearestBreakable.GetEntryIndex()))
	{
		v_desired_offset->x = 0;
		v_desired_offset->y = 0;
		v_desired_offset->z = 0;
	}
}

void CCSSBot::modThink()
{
	m_pCurrentWeapon = CClassInterface::getCurrentWeapon(m_pEdict);
	static int team;
	team = getTeam();

	if(m_pCurrentWeapon)
	{
		const CBotWeapon *weapon = m_pWeapons->getWeapon(CWeapons::getWeapon(m_pCurrentWeapon->GetClassName()));
		if(weapon && weapon->getClip1(this) == 0 && !weapon->isMelee() && weapon->getID() != CS_WEAPON_C4)
		{
			letGoOfButton(IN_ATTACK);
			tapButton(IN_RELOAD);
			updateCondition(CONDITION_OUT_OF_AMMO);
		}
		else
		{
			removeCondition(CONDITION_OUT_OF_AMMO);
		}
	}

	if(onLadder())
	{
		setMoveLookPriority(MOVELOOK_OVERRIDE);
		setLookAtTask(LOOK_WAYPOINT);
		m_pButtons->holdButton(IN_FORWARD,0,1,0);
		setMoveLookPriority(MOVELOOK_MODTHINK);
	}

	// Team Specific thinking
	switch (team)
	{
		case CS_TEAM_COUNTERTERRORIST:
		{
			if(!CCounterStrikeSourceMod::wasBombFound() && CCounterStrikeSourceMod::canHearPlantedBomb(this))
			{
				CCounterStrikeSourceMod::setBombFound(true);
				updateCondition(CONDITION_CHANGED);
				debugMsg(BOT_DEBUG_THINK, "[CSS-BOT] Found bomb!");
			}
			break;
		}
	}

	if(m_fNextThinkSlow <= engine->Time())
	{
		modThinkSlow();
	}

	if(getEnemy() != nullptr && isVisible(getEnemy()))
	{
		const CBotWeapon *currentweapon = getCurrentWeapon();

		if(!hasSomeConditions(CONDITION_OUT_OF_AMMO))
		{
			if(currentweapon && !currentweapon->isMelee())
			{
				stopMoving();
			}
		}
	}
}

void CCSSBot::modThinkSlow()
{
	static Vector velocity;
	static float fvelocity;

	m_fNextThinkSlow = engine->Time() + 1.0f;

	velocity = Vector(0,0,0);
	CClassInterface::getVelocity(getEdict(), &velocity);
	fvelocity = velocity.Length();

	if(fvelocity >= 16.0f && CCounterStrikeSourceMod::isScoped(this))
	{
		secondaryAttack(false);
	}
}

void CCSSBot::getTasks(unsigned iIgnore)
{
	static CBotUtilities utils;
	static CBotUtility* next;
	static bool bCheckCurrent;
	static int team;

	if(!hasSomeConditions(CONDITION_CHANGED) && !m_pSchedules->isEmpty())
		return;

	removeCondition(CONDITION_CHANGED);
	bCheckCurrent = true; // important for checking current schedule
	team = getTeam();
	setMoveSpeed(CClassInterface::getMaxSpeed(m_pEdict)); // Some tasks changes the bot move speed, reset it back.

	// Utilities

	switch (team)
	{
		case CS_TEAM_COUNTERTERRORIST: // CT specific utilities
		{
			if(CCounterStrikeSourceMod::isMapType(CS_MAP_BOMBDEFUSAL))
			{
				ADD_UTILITY(BOT_UTIL_DEFEND_BOMB, !CCounterStrikeSourceMod::isBombPlanted() && bot_defrate.GetFloat() <= randomFloat(0.0f, 1.0f), 0.80f)
				ADD_UTILITY(BOT_UTIL_SEARCH_FOR_BOMB, !CCounterStrikeSourceMod::wasBombFound() && CCounterStrikeSourceMod::isBombPlanted(), 0.81f)
				ADD_UTILITY(BOT_UTIL_DEFUSE_BOMB, CCounterStrikeSourceMod::wasBombFound(), 0.85f)
			}
			else if(CCounterStrikeSourceMod::isMapType(CS_MAP_HOSTAGERESCUE))
			{
				ADD_UTILITY(BOT_UTIL_GET_HOSTAGE, CCounterStrikeSourceMod::canRescueHostages(), 0.85f)
				ADD_UTILITY(BOT_UTIL_RESCUE, IsLeadingHostage(), 0.84f)
			}
			break;
		}
		case CS_TEAM_TERRORIST: // TR specific utilities
		{
			if(CCounterStrikeSourceMod::isMapType(CS_MAP_BOMBDEFUSAL))
			{
				ADD_UTILITY(BOT_UTIL_PLANT_BOMB, CCounterStrikeSourceMod::isBombCarrier(this), 0.80f)
				ADD_UTILITY(BOT_UTIL_PICKUP_BOMB, CCounterStrikeSourceMod::isBombDropped(), 0.80f)
				ADD_UTILITY(BOT_UTIL_DEFEND_NEAREST_BOMB, CCounterStrikeSourceMod::isBombPlanted(), 0.85f)
			}
			else if (CCounterStrikeSourceMod::isMapType(CS_MAP_HOSTAGERESCUE))
			{
				ADD_UTILITY(BOT_UTIL_GUARD_RESCUE_ZONE, CCounterStrikeSourceMod::canRescueHostages(), 0.70f)
			}
			break;
		}
	}

	ADD_UTILITY(BOT_UTIL_SNIPE, IsSniper(), randomFloat(0.7900f, 0.8200f))

	// Combat Utilities
	ADD_UTILITY(BOT_UTIL_ENGAGE_ENEMY, hasSomeConditions(CONDITION_SEE_CUR_ENEMY) && !hasSomeConditions(CONDITION_OUT_OF_AMMO), 1.00f)
	ADD_UTILITY(BOT_UTIL_WAIT_LAST_ENEMY, hasSomeConditions(CONDITION_ENEMY_OBSCURED), 0.95f)
	ADD_UTILITY(BOT_UTIL_HIDE_FROM_ENEMY, hasSomeConditions(CONDITION_SEE_CUR_ENEMY) && hasSomeConditions(CONDITION_OUT_OF_AMMO), 0.98f)

	// Generic Utilities
	ADD_UTILITY(BOT_UTIL_BUY, m_pBuyManager->wantsToBuy(), 1.0f) // Buy weapons
	ADD_UTILITY(BOT_UTIL_ROAM, true, 0.001f) // Roam around

	utils.execute();

	while ((next = utils.nextBest()) != nullptr)
	{
		if(!m_pSchedules->isEmpty() && bCheckCurrent)
		{
			if(m_CurrentUtil != next->getId())
				m_pSchedules->freeMemory();
			else
				break;
		}

		bCheckCurrent = false;

		if(executeAction(next->getId()))
		{
			m_CurrentUtil = next->getId();

			if(m_fUtilTimes[next->getId()] < engine->Time())
				m_fUtilTimes[next->getId()] = engine->Time() + randomFloat(0.1f, 2.0f); // saves problems with consistent failing

			if(CClients::clientsDebugging(BOT_DEBUG_UTIL))
			{
				int i = 0;
				CClients::clientDebugMsg(this,BOT_DEBUG_UTIL,"-------- getTasks(%s) --------",m_szBotName);

				do
				{
					CClients::clientDebugMsg(this,BOT_DEBUG_UTIL,"%s = %0.3f",g_szUtils[next->getId()],next->getUtility(),this);
				}while ((++i<20) && ((next = utils.nextBest()) != nullptr));

				CClients::clientDebugMsg(this,BOT_DEBUG_UTIL,"----END---- getTasks(%s) ----END----",m_szBotName);
			}
			break;
		}
	}

	utils.freeMemory();
}

bool CCSSBot::executeAction(const eBotAction iAction)
{
	switch (iAction)
	{
		case BOT_UTIL_ENGAGE_ENEMY:
		{
			CBotSchedule* pSched = new CBotSchedule();
			pSched->setID(SCHED_ATTACK);
			pSched->addTask(new CCSSEngageEnemyTask(m_pEnemy.get()));
			m_pSchedules->add(pSched);
			return true;
			//break;
		}
		case BOT_UTIL_WAIT_LAST_ENEMY:
		{
			CBotSchedule* pSched = new CBotSchedule();
			CBotTask* pTask = new CBotWaitTask(randomFloat(2.0f, 4.0f), m_vLastSeeEnemy);
			pTask->setCompleteInterrupt(CONDITION_ENEMY_DEAD);
			pTask->setFailInterrupt(CONDITION_SEE_CUR_ENEMY);
			pSched->setID(SCHED_WAIT_FOR_ENEMY);
			pSched->addTask(pTask);
			m_pSchedules->add(pSched);
			return true;
			//break;
		}
		case BOT_UTIL_HIDE_FROM_ENEMY:
		{
			CBotSchedule *pSched = new CBotSchedule();
			pSched->setID(SCHED_RUN_FOR_COVER);
			const int cover = CWaypointLocations::GetCoverWaypoint(getOrigin(), CBotGlobals::entityOrigin(m_pEnemy.get()), nullptr, nullptr, 0, 8.0f, 1024.0f);
			if(cover != -1)
			{
				CBotTask *pTask = new CFindPathTask(cover);
				pTask->setCompleteInterrupt(CONDITION_ENEMY_DEAD, CONDITION_OUT_OF_AMMO);
				pSched->addTask(pTask);
				m_pSchedules->add(pSched);
				return true;
			}
			break;			
		}
		case BOT_UTIL_BUY:
		{
			CBotSchedule* pSched = new CBotSchedule();
			pSched->setID(SCHED_BUY);
			pSched->addTask(new CCSSPerformBuyTask());
			m_pSchedules->add(pSched);
			return true;
			//break;
		}
		case BOT_UTIL_PLANT_BOMB:
		{
			if(CWaypoint* pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_GOAL))
			{
				CWaypoint * pRoute = nullptr;
				if(m_fUseRouteTime <= engine->Time())
				{
					pRoute = CWaypoints::randomRouteWaypoint(this, getOrigin(), pWaypoint->getOrigin(), getTeam(), pWaypoint->getArea());
				}
				m_pSchedules->add(new CCSSPlantBombSched(pWaypoint, pRoute));
				return true;
			}
			break;
		}
		case BOT_UTIL_PICKUP_BOMB:
		{
			if(edict_t *pBomb = CCounterStrikeSourceMod::getBomb())
			{
				m_pSchedules->add(new CBotPickupSched(pBomb));
				return true;
			}
			break;
		}
		case BOT_UTIL_DEFEND_NEAREST_BOMB: /** T: Defend planted bomb **/
		{
			CBotSchedule* pSched = new CBotSchedule();
			pSched->setID(SCHED_DEFENDPOINT);
			edict_t *pBomb = CCounterStrikeSourceMod::getBomb();
			const Vector vBomb = CBotGlobals::entityOrigin(pBomb);
			if(pBomb)
			{
				// Find the nearest bomb waypoint to retreive the area from
				if(CWaypoint *pBombWpt = CWaypoints::getWaypoint(CWaypoints::nearestWaypointGoal(CWaypointTypes::W_FL_GOAL, vBomb, 256.0f, 0)))
				{
					if(CWaypoint *pDefend = CWaypoints::randomWaypointGoalNearestArea(CWaypointTypes::W_FL_DEFEND, getTeam(), pBombWpt->getArea(), true, this, false, &vBomb))
					{
						pSched->addTask(new CFindPathTask(CWaypoints::getWaypointIndex(pDefend)));
						pSched->addTask(new CCSSGuardTask(getPrimaryWeapon(), pDefend->getOrigin(), pDefend->getAimYaw(), false, 0.0f, pDefend->getFlags()));
						m_pSchedules->add(pSched);
						CClients::clientDebugMsg(this, BOT_DEBUG_UTIL, "[BOT_UTIL_DEFEND_NEAREST_BOMB] Bomb Waypoint (%i) Defend Waypoint (%i)", 
						CWaypoints::getWaypointIndex(pBombWpt), CWaypoints::getWaypointIndex(pDefend));
						return true;
					}
				}
			}
			break;
		}
		case BOT_UTIL_DEFEND_BOMB: /** CT: Defend bomb site **/
		{
			CBotSchedule* pSched = new CBotSchedule();
			pSched->setID(SCHED_DEFENDPOINT);
			if(const CWaypoint *pGoal = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_GOAL, getTeam()))
			{
				if(CWaypoint *pDefend = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_DEFEND, getTeam(), pGoal->getArea(), true, this))
				{
					pSched->addTask(new CFindPathTask(CWaypoints::getWaypointIndex(pDefend)));
					pSched->addTask(new CCSSGuardTask(getPrimaryWeapon(), pDefend->getOrigin(), pDefend->getAimYaw(), false, 0.0f, pDefend->getFlags()));
					m_pSchedules->add(pSched);
					return true;
				}
			}
			break;		
		}
		case BOT_UTIL_DEFUSE_BOMB:
		{
			if(edict_t *pBomb = CCounterStrikeSourceMod::getBomb())
			{
				CBotSchedule *pSched = new CBotSchedule();
				CBotTask *pFindPath = new CFindPathTask(pBomb);
				CBotTask *pMoveTask = new CMoveToTask(pBomb);
				pFindPath->setFailInterrupt(CONDITION_SEE_CUR_ENEMY);
				pMoveTask->setFailInterrupt(CONDITION_SEE_CUR_ENEMY);
				pSched->setID(SCHED_BOMB);
				pSched->addTask(pFindPath);
				pSched->addTask(pMoveTask);
				pSched->addTask(new CCSSDefuseTheBombTask(CBotGlobals::entityOrigin(pBomb)));
				m_pSchedules->add(pSched);
				return true;
			}
			break;
		}
		case BOT_UTIL_SEARCH_FOR_BOMB:
		{
			CBotSchedule* pSched = new CBotSchedule();

			pSched->setID(SCHED_GOTO_ORIGIN);

			if(CWaypoint* pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_GOAL))
			{
				CWaypoint* pRoute = CWaypoints::randomRouteWaypoint(this, getOrigin(), pWaypoint->getOrigin(), getTeam(),
																	pWaypoint->getArea());
				if(m_fUseRouteTime <= engine->Time())
				{
					if(pRoute)
					{
						const int iRoute = CWaypoints::getWaypointIndex(pRoute); // Route waypoint
						pSched->addTask(new CFindPathTask(iRoute, LOOK_WAYPOINT));
						pSched->addTask(new CMoveToTask(pRoute->getOrigin()));
						m_pSchedules->add(pSched);
						m_fUseRouteTime = engine->Time() + 30.0f;
					}
				}

				const int iWaypoint = CWaypoints::getWaypointIndex(pWaypoint);
				pSched->addTask(new CFindPathTask(iWaypoint, LOOK_WAYPOINT));
				pSched->addTask(new CMoveToTask(pWaypoint->getOrigin()));
				m_pSchedules->add(pSched);

				return true;
			}
			break;
		}
		case BOT_UTIL_GET_HOSTAGE:
		{
			// Select a random hostage to rescue
			CBotSchedule* pSched = new CBotSchedule();
			edict_t* pHostage = CCounterStrikeSourceMod::getRandomHostage();
			pSched->setID(SCHED_GOTONEST);

			if (pHostage)
			{
				const Vector vHostage = CBotGlobals::entityOrigin(pHostage);
				CWaypoint* pRoute = CWaypoints::randomRouteWaypoint(this, getOrigin(), vHostage, getTeam(), 0);
				if((m_fUseRouteTime <= engine->Time()))
				{
					if(pRoute)
					{
						const int iRoute = CWaypoints::getWaypointIndex(pRoute); // Route waypoint
						pSched->addTask(new CFindPathTask(iRoute, LOOK_WAYPOINT));
						m_fUseRouteTime = engine->Time() + 30.0f;
					}
				}

				pSched->addTask(new CFindPathTask(pHostage));
				pSched->addTask(new CMoveToTask(pHostage));
				pSched->addTask(new CBotHL2DMUseButton(pHostage));
				pSched->addTask(new CBotWaitTask(1.0f));
				m_pSchedules->add(pSched);
				return true;
			}

			break;
			}
				case BOT_UTIL_GUARD_RESCUE_ZONE:
				{
					edict_t* pRescueZone = CClassInterface::FindEntityByClassnameNearest(getOrigin(), "func_hostage_rescue");
					CBotSchedule* pSched = new CBotSchedule();
					pSched->setID(SCHED_GOTONEST);
					m_fUtilTimes[BOT_UTIL_GUARD_RESCUE_ZONE] = engine->Time() + randomFloat(20.0f, 40.0f);

					if (pRescueZone)
					{
						const Vector vRescue = CBotGlobals::worldCenter(pRescueZone);
						pSched->addTask(new CFindPathTask(vRescue));
						pSched->addTask(new CMoveToTask(vRescue));
						pSched->addTask(new CBotWaitTask(0.50f));
						m_pSchedules->add(pSched);
						return true;
					}

					break;
				}
		case BOT_UTIL_RESCUE:
		{
			CBotSchedule* pSched = new CBotSchedule();
			pSched->setID(SCHED_GOTO_ORIGIN);

			CWaypoint* pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_RESCUEZONE, getTeam(), 0, false, this,
																  false);

			if(pWaypoint)
			{
				CWaypoint* pRoute = CWaypoints::randomRouteWaypoint(this, getOrigin(), pWaypoint->getOrigin(), getTeam(),
																	pWaypoint->getArea());
				if((m_fUseRouteTime <= engine->Time()))
				{
					if(pRoute)
					{
						const int iRoute = CWaypoints::getWaypointIndex(pRoute); // Route waypoint
						pSched->addTask(new CFindPathTask(iRoute, LOOK_WAYPOINT));
						m_fUseRouteTime = engine->Time() + 30.0f;
					}
				}

				pSched->addTask(new CFindPathTask(CWaypoints::getWaypointIndex(pWaypoint)));
				pSched->addTask(new CBotWaitTask(3.0f));
				m_pSchedules->add(pSched);
				return true;
			}
			break;
		}
		case BOT_UTIL_SNIPE:
		{
			CBotSchedule* pSched = new CBotSchedule();
			pSched->setID(SCHED_SNIPE);

			if(CWaypoint* pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_SNIPER, getTeam(), 0, false))
			{
				CFindPathTask *pFindPath = new CFindPathTask(CWaypoints::getWaypointIndex(pWaypoint));
				//pFindPath->setInterruptFunction(new CBotCSSRoamInterrupt());
				CCSSGuardTask *pGuard = new CCSSGuardTask(getPrimaryWeapon(), pWaypoint->getOrigin(), pWaypoint->getAimYaw(), false, 0.0f, pWaypoint->getFlags());
				pSched->addTask(pFindPath);
				pSched->addTask(pGuard);
				m_pSchedules->add(pSched);
				return true;
			}
			break;
		}
		case BOT_UTIL_ROAM:
		{
			CBotSchedule* pSched = new CBotSchedule();

			pSched->setID(SCHED_GOTO_ORIGIN);

			if(CWaypoint* pWaypoint = CWaypoints::randomWaypointGoal(-1))
			{
				const int iWaypoint = CWaypoints::getWaypointIndex(pWaypoint);
				CFindPathTask *pFindPath = new CFindPathTask(iWaypoint);
				//pFindPath->setInterruptFunction(new CBotCSSRoamInterrupt());
				pSched->addTask(pFindPath);
				m_pSchedules->add(pSched);

				return true;
			}
			break;
		}
	}

	return false;
}

// Called when the round starts
void CCSSBot::onRoundStart()
{
	m_pBuyManager->onRoundStart();
}

/**
 * Checks if this bot is escorting/leading a hostage to a rescue zone
 * 
 * @return TRUE if this bot is escorting/leading a hostage to a rescue zone
 **/
bool CCSSBot::IsLeadingHostage()
{
	const std::vector<CBaseHandle> hostages = CCounterStrikeSourceMod::getHostageVector();
	//edict_t *pHostage = nullptr;

	if(getTeam() != CS_TEAM_COUNTERTERRORIST)
		return false;

	if(hostages.empty())
		return false;

	for(const CBaseHandle& i : hostages)
	{
		edict_t *pHostage = INDEXENT(i.GetEntryIndex());

		if(CBotGlobals::entityIsValid(pHostage) && CClassInterface::getCSHostageLeader(pHostage) == m_pEdict)
		{
			return true;
		}
	}

	return false;
}

void CCSSBot::touchedWpt(CWaypoint *pWaypoint, const int iNextWaypoint, const int iPrevWaypoint)
{
	if(iNextWaypoint != -1 && pWaypoint->hasFlag(CWaypointTypes::W_FL_DOOR)) // Use waypoint: Check for door
	{
		if(CWaypoint *pNext = CWaypoints::getWaypoint(iNextWaypoint))
		{
			/**
			 * Traces a line between the current waypoint and the next waypoint. If a door is blocking the path, try to open it.
			 **/
			CTraceFilterHitAll filter;
			const trace_t *tr = CBotGlobals::getTraceResult();
			CBotGlobals::traceLine(pWaypoint->getOrigin() + Vector(0, 0, CWaypoint::WAYPOINT_HEIGHT / 2),
								   pNext->getOrigin() + Vector(0, 0, CWaypoint::WAYPOINT_HEIGHT / 2), MASK_PLAYERSOLID,
								   &filter);
			if(tr->fraction < 1.0f)
			{
				if(tr->m_pEnt)
				{
					edict_t *pDoor = servergameents->BaseEntityToEdict(tr->m_pEnt);
					const char *szclassname = pDoor->GetClassName();
					if(std::strncmp(szclassname, "prop_door_rotating", 18) == 0 || std::strncmp(szclassname, "func_door", 9) == 0 || std::strncmp(szclassname, "func_door_rotating", 18) == 0)
					{
						m_pSchedules->addFront(new CSynOpenDoorSched(pDoor));
					}
				}
			}
		}
	}

	CBot::touchedWpt(pWaypoint, iNextWaypoint, iPrevWaypoint);
}

bool CCSSBot::canGotoWaypoint(const Vector& vPrevWaypoint, CWaypoint* pWaypoint, CWaypoint* pPrev)
{
	if (pWaypoint->hasFlag(CWaypointTypes::W_FL_NO_HOSTAGES) || pWaypoint->hasFlag(CWaypointTypes::W_FL_LADDER))
	{
		return !IsLeadingHostage();
	}

	return CBot::canGotoWaypoint(vPrevWaypoint, pWaypoint, pPrev);
}