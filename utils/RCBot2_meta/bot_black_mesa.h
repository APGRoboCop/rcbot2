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
#ifndef __BMS_RCBOT_H__
#define __BMS_RCBOT_H__

#include "bot.h" // Ensure this header is included for the CBot base class

 // Bot for Black Mesa
class CBotBMS : public CBot
{
public:
    // Check if this is a Black Mesa bot
    static bool isBMS() { return true; }

    // Override virtual functions from CBot
    void modThink() override;
    void init(bool bVarInit = false) override;
    void setup() override;
    bool startGame() override;
    void died(edict_t* pKiller, const char* pszWeapon) override;
    void spawnInit() override;
    bool isEnemy(edict_t* pEdict, bool bCheckWeapons = true) override;

    // Static function to handle when a bot kills another entity
    static void killed(edict_t* pVictim);

private:
    // Add any private members or methods here
};

#endif // __BMS_RCBOT_H__