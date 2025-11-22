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
#include "bot_globals.h"
#include "bot_getprop.h"
#include "bot_css_buying.h"
#include "bot_css_bot.h"
#include "bot_weapons.h"
#include "bot_profile.h"
#include "bot_cvars.h"
#include "logging.h"

const char *szCSSBuyItem[CS_BUY_MAX+1] =
{
    "primammo",
    "secammo",
    "vest",
    "vesthelm",
    "defuser",
    "nvgs", // 5
    "usp",
    "glock",
    "p228",
    "fiveseven",
    "elite", // 10
    "deagle",
    "m3",
    "xm1014",
    "tmp",
    "mac10", // 15
    "mp5navy",
    "ump45",
    "p90",
    "famas",
    "galil", // 20
    "ak47",
    "m4a1",
    "aug",
    "sg552",
    "scout", // 25
    "awp",
    "sg550",
    "g3sg1",
    "m249",
    "hegrenade", // 30
    "smokegrenade",
    "flashbang",
    "MAX"
};

// Array with item prices
const int iCSSItemPrice[CS_BUY_MAX+1] =
{
    0, // Primary Ammo
    0, // Secondary Ammo
    650, // Vest
    1000, // Vesthelm
    200, // DEFUSER
    1250, // 5 NV Goggles
    500, // USP
    400, // GLOCK
    600, // P228
    750, // FIVE SEVEN
    800, // 10 Elites
    650, // DEAGLE
    1700, // M3
    3000, // XM1014
    1250, // TMP
    1400, // 15 Mac 10
    1500, // MP5
    1700, // UMP
    2350, // P90
    2250, // FAMAS
    2000, // 20 GALIL
    2500, // AK47
    3100, // M4A1
    3500, // AUG
    3500, // SG 552
    2750, // 25 Scout
    4750, // Awp
    4200, // SG 550
    5000, // G3SG1
    5750, // M249
    300, // 30 HE GRENADE
    300, // SMOKE GRENADE
    200, // FLASHBANG GRENADE
    0, // MAX
};

CCSSBotBuying::CCSSBotBuying(CBot *pBot)
{
    m_pBot = pBot;
    m_deaths = 0;
    selectRandomBuyProfile();
}

CCSSBotBuying::~CCSSBotBuying()
{
    m_pBot = nullptr;
    m_money = 0;
    m_fLastBuyTime = 0.0f;
    m_pPrimaryWeapon = nullptr;
    m_pSecondaryWeapon = nullptr;
    m_eProfile = CS_BUY_PROFILE_INVALID;
}

// Reset
void CCSSBotBuying::reset()
{
    m_money = 0;
    m_deaths = 0;
    m_fLastBuyTime = 0.0f;
    m_pPrimaryWeapon = nullptr;
    m_pSecondaryWeapon = nullptr;
    m_eProfile = CS_BUY_PROFILE_INVALID;
}

// Update
void CCSSBotBuying::update()
{
    m_money = CClassInterface::getCSPlayerMoney(m_pBot->getEdict());
    m_pPrimaryWeapon = m_pBot->getWeapons()->getCurrentWeaponInSlot(CS_WEAPON_SLOT_PRIMARY);
    m_pSecondaryWeapon = m_pBot->getWeapons()->getCurrentWeaponInSlot(CS_WEAPON_SLOT_SECONDARY);
}

// Runs the buying logic
void CCSSBotBuying::execute()
{
    m_fLastBuyTime = engine->Time();
    std::queue<eCSSBuyItem> buyqueue;
    int cost = 0;

    // Eco
    if(m_money < rcbot_css_economy_eco_limit.GetInt())
        return;

    buildBuyList(buyqueue, cost);
    processBuyList(buyqueue);
    update();
}

/**
 * Buys a single item
 * 
 * @param item      The item to buy
 **/
void CCSSBotBuying::buyItem(eCSSBuyItem item)
{
	char buffer[32];
	sprintf(buffer, "buy %s", szCSSBuyItem[item]);
	helpers->ClientCommand(m_pBot->getEdict(), buffer);    
}

/**
 * Gets how much an item costs to buy
 * 
 * @param item      The item to get the price
 **/
int CCSSBotBuying::getPrice(eCSSBuyItem item)
{
    return iCSSItemPrice[item];
}

/**
 * Checks if the bot can buy this item
 * 
 * @param item      The item to check
 * @param money     Overrides the amount of money the bot has
 **/
bool CCSSBotBuying::canAfford(eCSSBuyItem item, int money)
{
    if(money == -1)
    {
        return m_money >= iCSSItemPrice[item];
    }
    else
    {
        return money >= iCSSItemPrice[item];
    }
}


/**
 * Does the bot wants to buy?
 * 
 * @return      TRUE if the bot wants to buy, FALSE otherwise
 **/
bool CCSSBotBuying::wantsToBuy()
{
    if(m_fLastBuyTime < 0.0f)
        return true;

    return m_fLastBuyTime + 45.0f <= engine->Time() && CClassInterface::isCSPlayerInBuyZone(m_pBot->getEdict());
}

/**
 * Selects a random buy profile
 **/
void CCSSBotBuying::selectRandomBuyProfile()
{
    m_eProfile = (eCSSBuyProfile)randomInt(CS_BUY_PROFILE_SMG, CS_BUY_PROFILE_MAX-1);
}

void CCSSBotBuying::setBuyProfile(eCSSBuyProfile profile)
{
    m_eProfile = profile;
}

eCSSBuyProfile CCSSBotBuying::getBuyProfile()
{
    return m_eProfile;
}

/**
 * Called when a round starts
 **/
void CCSSBotBuying::onRoundStart()
{
    m_fLastBuyTime = -1.0f;
}

/**
 * Called when the bot dies
 **/  
void CCSSBotBuying::onDeath()
{
    m_deaths++;

    // Random chance to switch buying profile when killed
    if(randomInt(1,50) <= 1 + m_deaths)
    {
        selectRandomBuyProfile();
        m_deaths = 0;
    }
}

bool CCSSBotBuying::shouldBuyArmor(eCSSBuyType buytype)
{
    int armor = CClassInterface::getCSPlayerArmor(m_pBot->getEdict());
    bool hashelm = CClassInterface::CSPlayerHasHelmet(m_pBot->getEdict());

    if(buytype == CS_BUY_TYPE_ECO)
        return false;

    if(!hashelm)
        return true;

    // Low skill bots have a random chance to not buy armor
    if(m_pBot->getProfile()->m_fAimSkill <= 0.25f && randomInt(0,1))
        return false;

    return armor < 70;
}

bool CCSSBotBuying::shouldBuyDefuser(eCSSBuyType buytype)
{
    bool hasdefuser = CClassInterface::CSPlayerHasDefuser(m_pBot->getEdict());

    if(hasdefuser)
        return false;

    if(m_pBot->getTeam() != CS_TEAM_COUNTERTERRORIST)
        return false;

    if(buytype == CS_BUY_TYPE_ECO)
        return false;

    if(!CCounterStrikeSourceMod::isMapType(CS_MAP_BOMBDEFUSAL))
        return false;

    if(buytype == CS_BUY_TYPE_FORCE && m_pBot->getProfile()->m_fAimSkill >= 0.50f)
        return randomInt(0,1) == 1;

    // Bots with higher skill are more likely to buy defuser
    return randomFloat(0.0f, 1.0f) <= m_pBot->getProfile()->m_fAimSkill;
}

eCSSBuyType CCSSBotBuying::determineBuyType()
{
    switch (m_eProfile)
    {
        case CS_BUY_PROFILE_SMG:
        {
            if(m_money >= 4000)
            {
                return CS_BUY_TYPE_FULL;
            }
            else if(m_money >= 2600)
            {
                return CS_BUY_TYPE_FORCE;
            }
            else
            {
                return CS_BUY_TYPE_ECO;
            }
            break;
        }
        case CS_BUY_PROFILE_SHOTGUNNER:
        {
            if(m_money >= 4500)
            {
                return CS_BUY_TYPE_FULL;
            }
            else if(m_money >= 2000)
            {
                return CS_BUY_TYPE_FORCE;
            }
            else
            {
                return CS_BUY_TYPE_ECO;
            }
            break;
        }
        case CS_BUY_PROFILE_RIFLE:
        {
            if(m_money >= 4500)
            {
                return CS_BUY_TYPE_FULL;
            }
            else if(m_money >= 3800)
            {
                return CS_BUY_TYPE_FORCE;
            }
            else
            {
                return CS_BUY_TYPE_ECO;
            }
            break;
        }
        case CS_BUY_PROFILE_SCOPEDRIFLE:
        {
            if(m_money >= 5000)
            {
                return CS_BUY_TYPE_FULL;
            }
            else if(m_money >= 3800)
            {
                return CS_BUY_TYPE_FORCE;
            }
            else
            {
                return CS_BUY_TYPE_ECO;
            }
            break;
        }
        case CS_BUY_PROFILE_SNIPER:
        {
            if(m_money >= 6000)
            {
                return CS_BUY_TYPE_FULL;
            }
            else if(m_money >= 2500)
            {
                return CS_BUY_TYPE_FORCE;
            }
            else
            {
                return CS_BUY_TYPE_ECO;
            }
            break;
        }
        case CS_BUY_PROFILE_AUTOSNIPER:
        {
            if(m_money >= 6500)
            {
                return CS_BUY_TYPE_FULL;
            }
            else if(m_money >= 5500)
            {
                return CS_BUY_TYPE_FORCE;
            }
            else
            {
                return CS_BUY_TYPE_ECO;
            }
            break;
        }
        case CS_BUY_PROFILE_MACHINEGUNNER:
        {
            if(m_money >= 7000)
            {
                return CS_BUY_TYPE_FULL;
            }
            else if(m_money >= 6500)
            {
                return CS_BUY_TYPE_FORCE;
            }
            else
            {
                return CS_BUY_TYPE_ECO;
            }
            break;
        }
        default:
        {
            if(m_money >= 5000)
            {
                return CS_BUY_TYPE_FULL;
            }
            else
            {
                return CS_BUY_TYPE_ECO;
            }
            break;
        }
    }
}

void CCSSBotBuying::buildBuyList(std::queue<eCSSBuyItem> &queue, int &cost)
{
    eCSSBuyType buytype = determineBuyType();
    bool buydefuser = shouldBuyDefuser(buytype);

    if(shouldBuyArmor(buytype))
    {
        queue.push(CS_BUY_ARMOR_VESTHELM);
        cost += getPrice(CS_BUY_ARMOR_VESTHELM);
    }

    if(shouldBuyDefuser(buytype))
    {
        queue.push(CS_BUY_MISC_DEFUSER);
        cost += getPrice(CS_BUY_MISC_DEFUSER);
    }

    if(!m_pPrimaryWeapon)
    {
        switch (m_eProfile)
        {
            case CS_BUY_PROFILE_SMG:
            {
                if(buytype == CS_BUY_TYPE_FULL)
                {
                    queue.push(CS_BUY_SMG_P90);
                    cost += getPrice(CS_BUY_SMG_P90);
                }
                else if(buytype == CS_BUY_TYPE_FORCE)
                {
                    if(canAfford(CS_BUY_SMG_UMP45, m_money - cost))
                    {
                        queue.push(CS_BUY_SMG_UMP45);
                        cost += getPrice(CS_BUY_SMG_UMP45);
                    }
                    else if(canAfford(CS_BUY_SMG_MP5NAVY, m_money - cost))
                    {
                        queue.push(CS_BUY_SMG_MP5NAVY);
                        cost += getPrice(CS_BUY_SMG_MP5NAVY);
                    }
                    else if(m_pBot->getTeam() == CS_TEAM_TERRORIST && canAfford(CS_BUY_SMG_MAC10, m_money - cost))
                    {
                        queue.push(CS_BUY_SMG_MAC10);
                        cost += getPrice(CS_BUY_SMG_MAC10);
                    }
                    else if(m_pBot->getTeam() == CS_TEAM_COUNTERTERRORIST && canAfford(CS_BUY_SMG_TMP, m_money - cost))
                    {
                        queue.push(CS_BUY_SMG_TMP);
                        cost += getPrice(CS_BUY_SMG_TMP);
                    }
                }
                break;
            }
            case CS_BUY_PROFILE_SHOTGUNNER:
            {
                if(buytype == CS_BUY_TYPE_FULL)
                {
                    queue.push(CS_BUY_SHOTGUN_XM1014);
                    cost += getPrice(CS_BUY_SHOTGUN_XM1014);
                }
                else if(buytype == CS_BUY_TYPE_FORCE)
                {
                    if(canAfford(CS_BUY_SHOTGUN_M3, m_money - cost))
                    {
                        queue.push(CS_BUY_SHOTGUN_M3);
                        cost += getPrice(CS_BUY_SHOTGUN_M3);
                    }
                    else if(canAfford(CS_BUY_PISTOL_DEAGLE, m_money - cost))
                    {
                        queue.push(CS_BUY_PISTOL_DEAGLE);
                        cost += getPrice(CS_BUY_PISTOL_DEAGLE);
                    }
                }
                break;
            }
            case CS_BUY_PROFILE_RIFLE:
            {
                if(buytype == CS_BUY_TYPE_FULL)
                {
                    if(m_pBot->getTeam() == CS_TEAM_TERRORIST)
                    {
                        queue.push(CS_BUY_RIFLE_AK47);
                        cost += getPrice(CS_BUY_RIFLE_AK47);
                    }
                    else
                    {
                        queue.push(CS_BUY_RIFLE_M4A1);
                        cost += getPrice(CS_BUY_RIFLE_M4A1);
                    }
                }
                else if(buytype == CS_BUY_TYPE_FORCE)
                {
                    if(m_pBot->getTeam() == CS_TEAM_TERRORIST && canAfford(CS_BUY_RIFLE_GALIL, m_money - cost))
                    {
                        queue.push(CS_BUY_RIFLE_GALIL);
                        cost += getPrice(CS_BUY_RIFLE_GALIL);
                    }
                    else if(m_pBot->getTeam() == CS_TEAM_COUNTERTERRORIST && canAfford(CS_BUY_RIFLE_FAMAS, m_money - cost))
                    {
                        queue.push(CS_BUY_RIFLE_FAMAS);
                        cost += getPrice(CS_BUY_RIFLE_FAMAS);
                    }
                    else if(canAfford(CS_BUY_SMG_MP5NAVY, m_money - cost))
                    {
                        queue.push(CS_BUY_SMG_MP5NAVY);
                        cost += getPrice(CS_BUY_SMG_MP5NAVY);
                    }
                }
                break;
            }
            case CS_BUY_PROFILE_SCOPEDRIFLE:
            {
                if(buytype == CS_BUY_TYPE_FULL)
                {
                    if(m_pBot->getTeam() == CS_TEAM_TERRORIST)
                    {
                        queue.push(CS_BUY_RIFLE_SG552);
                        cost += getPrice(CS_BUY_RIFLE_SG552);
                    }
                    else
                    {
                        queue.push(CS_BUY_RIFLE_AUG);
                        cost += getPrice(CS_BUY_RIFLE_AUG);
                    }
                }
                else if(buytype == CS_BUY_TYPE_FORCE)
                {
                    if(m_pBot->getTeam() == CS_TEAM_TERRORIST && canAfford(CS_BUY_RIFLE_GALIL, m_money - cost))
                    {
                        queue.push(CS_BUY_RIFLE_GALIL);
                        cost += getPrice(CS_BUY_RIFLE_GALIL);
                    }
                    else if(m_pBot->getTeam() == CS_TEAM_COUNTERTERRORIST && canAfford(CS_BUY_RIFLE_FAMAS, m_money - cost))
                    {
                        queue.push(CS_BUY_RIFLE_FAMAS);
                        cost += getPrice(CS_BUY_RIFLE_FAMAS);
                    }
                    else if(canAfford(CS_BUY_SMG_MP5NAVY, m_money - cost))
                    {
                        queue.push(CS_BUY_SMG_MP5NAVY);
                        cost += getPrice(CS_BUY_SMG_MP5NAVY);
                    }
                }
                break;
            }
            case CS_BUY_PROFILE_SNIPER:
            {
                if(buytype == CS_BUY_TYPE_FULL)
                {
                    if(buytype == CS_BUY_TYPE_FULL)
                    {
                        queue.push(CS_BUY_SNIPER_AWP);
                        cost += getPrice(CS_BUY_SNIPER_AWP);
                    }
                }
                else if(buytype == CS_BUY_TYPE_FORCE)
                {
                    if(canAfford(CS_BUY_SNIPER_SCOUT, m_money - cost))
                    {
                        queue.push(CS_BUY_SNIPER_SCOUT);
                        cost += getPrice(CS_BUY_SNIPER_SCOUT);
                    }
                    else if(canAfford(CS_BUY_SMG_UMP45, m_money - cost))
                    {
                        queue.push(CS_BUY_SMG_UMP45);
                        cost += getPrice(CS_BUY_SMG_UMP45);
                    }
                }
                break;
            }
            case CS_BUY_PROFILE_AUTOSNIPER:
            {
                if(buytype == CS_BUY_TYPE_FULL)
                {
                    if(m_pBot->getTeam() == CS_TEAM_TERRORIST)
                    {
                        queue.push(CS_BUY_SNIPER_G3SG1);
                        cost += getPrice(CS_BUY_SNIPER_G3SG1);
                    }
                    else
                    {
                        queue.push(CS_BUY_SNIPER_SG550);
                        cost += getPrice(CS_BUY_SNIPER_SG550);
                    }
                }
                else if(buytype == CS_BUY_TYPE_FORCE)
                {
                    if(m_pBot->getTeam() == CS_TEAM_TERRORIST && canAfford(CS_BUY_SMG_MAC10, m_money - cost))
                    {
                        queue.push(CS_BUY_SMG_MAC10);
                        cost += getPrice(CS_BUY_SMG_MAC10);
                    }
                    else if(m_pBot->getTeam() == CS_TEAM_COUNTERTERRORIST && canAfford(CS_BUY_SMG_TMP, m_money - cost))
                    {
                        queue.push(CS_BUY_SMG_TMP);
                        cost += getPrice(CS_BUY_SMG_TMP);
                    }
                }
                break;
            }
            case CS_BUY_PROFILE_MACHINEGUNNER:
            {
                if(buytype == CS_BUY_TYPE_FULL)
                {
                    queue.push(CS_BUY_MG_M249);
                    cost += getPrice(CS_BUY_MG_M249);
                }
                else if(buytype == CS_BUY_TYPE_FORCE)
                {
                    if(m_pBot->getTeam() == CS_TEAM_TERRORIST && canAfford(CS_BUY_PISTOL_ELITES, m_money - cost))
                    {
                        queue.push(CS_BUY_PISTOL_ELITES);
                        cost += getPrice(CS_BUY_PISTOL_ELITES);
                    }
                    else if(m_pBot->getTeam() == CS_TEAM_COUNTERTERRORIST && canAfford(CS_BUY_PISTOL_FIVESEVEN, m_money - cost))
                    {
                        queue.push(CS_BUY_PISTOL_FIVESEVEN);
                        cost += getPrice(CS_BUY_PISTOL_FIVESEVEN);
                    }
                }
                break;
            }
        }
    }
}

void CCSSBotBuying::processBuyList(std::queue<eCSSBuyItem> &queue)
{
    eCSSBuyItem item = CS_BUY_MAX;

    while(!queue.empty())
    {
        item = queue.front();
        buyItem(item);
        queue.pop();
        logger->Log(LogLevel::DEBUG, "[CSS-BUY] Buying item \"%s\"", szCSSBuyItem[item]);
    }
}