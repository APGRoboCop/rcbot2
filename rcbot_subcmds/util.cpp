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
 */

#include <array>
#include <string>

CBotCommandInline SearchCommand("search", CMD_ACCESS_UTIL, [](const CClient* pClient, const BotCommandArgs& args)
	{
		edict_t* pPlayer = pClient->getPlayer();

		for (int i = 0; i < gpGlobals->maxEntities; ++i)
		{
			edict_t* pEdict = INDEXENT(i);

			if (pEdict && !pEdict->IsFree() && pEdict->m_pNetworkable && pEdict->GetIServerEntity())
			{
				const float fDistance = (CBotGlobals::entityOrigin(pEdict) - CBotGlobals::entityOrigin(pPlayer)).Length();
				if (fDistance < 128)
				{
					float fVelocity;
					Vector v;

					if (CClassInterface::getVelocity(pEdict, &v))
						fVelocity = v.Length();
					else
						fVelocity = 0;

					string_t model = pEdict->GetIServerEntity()->GetModelName();

					CBotGlobals::botMessage(pPlayer, 0, "(%d) D:%.2f C:'%s', Mid:%d, Mn:'%s' Health=%d, Tm:%d, Fl:%d, Spd=%.2f",
						i, fDistance, pEdict->GetClassName(), pEdict->GetIServerEntity()->GetModelIndex(), model.ToCStr(),
						static_cast<int>(CClassInterface::getPlayerHealth(pEdict)), CClassInterface::getTeam(pEdict),
						pEdict->m_fStateFlags, fVelocity);
				}
			}
		}

		return COMMAND_ACCESSED;
	});

CBotCommandInline SetTeleportUtilCommand("set_teleport", CMD_ACCESS_UTIL, [](CClient* pClient, const BotCommandArgs& args)
	{
		if (pClient)
		{
			pClient->setTeleportVector();
			engine->ClientPrintf(pClient->getPlayer(), "Teleport Position Remembered!");
			return COMMAND_ACCESSED;
		}

		return COMMAND_ERROR;
	}, "usage: remembers where you want to teleport");

CBotCommandInline TeleportUtilCommand("teleport", CMD_ACCESS_UTIL, [](CClient* pClient, const BotCommandArgs& args)
	{
		if (pClient)
		{
			const Vector* vTeleport = pClient->getTeleportVector();

			if (vTeleport != nullptr)
			{
				CBotGlobals::teleportPlayer(pClient->getPlayer(), *vTeleport);
				CBotGlobals::botMessage(pClient->getPlayer(), 0, "teleported to your remembered location");

				return COMMAND_ACCESSED;
			}
			else
			{
				CBotGlobals::botMessage(pClient->getPlayer(), 0, "You need to set a teleport location first with set_teleport.");
			}
		}

		return COMMAND_ERROR;
	}, "usage: first use set_teleport, then this command to go there");

CBotCommandInline NoClipCommand("noclip", CMD_ACCESS_UTIL, [](const CClient* pClient, const BotCommandArgs& args)
	{
		if (pClient)
		{
			if (edict_t* pEntity = pClient->getPlayer())
			{
				byte* movetype = CClassInterface::getMoveTypePointer(pEntity);
				if (!movetype)
					return COMMAND_ERROR;

				constexpr byte MOVETYPE_MASK = 15;
				bool bNoClipEnabled;

				if ((*movetype & MOVETYPE_MASK) != MOVETYPE_NOCLIP)
				{
					*movetype &= ~MOVETYPE_MASK;
					*movetype |= MOVETYPE_NOCLIP;
					bNoClipEnabled = true;
				}
				else
				{
					*movetype &= ~MOVETYPE_MASK;
					*movetype |= MOVETYPE_WALK;
					bNoClipEnabled = false;
				}

				std::array<char, 256> msg;
				snprintf(msg.data(), msg.size(), "%s used no_clip, which is now %s", pClient->getName(), bNoClipEnabled ? "enabled" : "disabled");

				CBotGlobals::botMessage(pEntity, 0, msg.data());
				return COMMAND_ACCESSED;
			}
		}
		return COMMAND_ERROR;
	}, "fly through walls , yeah!");

CBotCommandInline GodModeUtilCommand("god", CMD_ACCESS_UTIL, [](const CClient* pClient, const BotCommandArgs& args)
	{
		if (pClient)
		{
			if (edict_t* pEntity = pClient->getPlayer())
			{
				if (int* playerflags = CClassInterface::getPlayerFlagsPointer(pEntity))
				{
					*playerflags ^= FL_GODMODE;

					std::array<char, 256> msg;
					snprintf(msg.data(), msg.size(), "god mode %s", (*playerflags & FL_GODMODE) ? "enabled" : "disabled");

					CBotGlobals::botMessage(pEntity, 0, msg.data());

					return COMMAND_ACCESSED;
				}
			}
		}

		return COMMAND_ERROR;
	}, "usage: toggle for invulnerability!");

CBotCommandInline NoTouchCommand("notouch", CMD_ACCESS_UTIL, [](const CClient* pClient, const BotCommandArgs& args)
	{
		if (pClient)
		{
			if (edict_t* pEntity = pClient->getPlayer())
			{
				if (int* playerflags = CClassInterface::getPlayerFlagsPointer(pEntity))
				{
					*playerflags ^= FL_DONTTOUCH;

					std::array<char, 256> msg;
					snprintf(msg.data(), msg.size(), "notouch mode %s", (*playerflags & FL_DONTTOUCH) ? "enabled" : "disabled");
					CBotGlobals::botMessage(pEntity, 0, msg.data());

					return COMMAND_ACCESSED;
				}
			}
		}

		return COMMAND_ERROR;
	}, "don't set off capture points etc");

CBotSubcommands UtilSubcommands("util", CMD_ACCESS_DEDICATED, {
	&SearchCommand,
	&SetTeleportUtilCommand,
	&TeleportUtilCommand,
	&NoClipCommand,
	&GodModeUtilCommand,
	&NoTouchCommand
	});