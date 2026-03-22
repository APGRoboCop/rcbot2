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
#include "bot.h"
#include "bot_strings.h"
#include "bot_globals.h"
#include "bot_profile.h"
#include "bot_visibles.h"
#include "bot_navigator.h"
#include "bot_kv.h"

#include "rcbot/logging.h"

std::vector <CBotProfile*> CBotProfiles :: m_Profiles;
CBotProfile *CBotProfiles :: m_pDefaultProfile = nullptr;

CBotProfile :: CBotProfile (const CBotProfile &other)
{
	*this = other;

	m_szName = CStrings::getString(other.m_szName);
	m_szModel = CStrings::getString(other.m_szModel);
}

CBotProfile :: CBotProfile (
		const char* szName, 
		const char* szModel,
		const int iTeam,
		const int iVisionTicks,
		const int iPathTicks,
		const int iVisionTicksClients,
		const int iSensitivity,
		const float fBraveness,
		const float fAimSkill,
		const int iClass )
{ 
	m_iVisionTicksClients = iVisionTicksClients;
	m_iSensitivity = iSensitivity;
	m_fBraveness = fBraveness;
	m_fAimSkill = fAimSkill;
	m_szName = CStrings::getString(szName);
	m_szModel = CStrings::getString(szModel);
	m_iPathTicks = iPathTicks;
	m_iVisionTicks = iVisionTicks;
	m_iTeam = iTeam;
	m_iClass = iClass;
}

void CBotProfiles :: deleteProfiles ()
{
	for (CBotProfile*& m_Profile : m_Profiles)
	{
		delete m_Profile;
		m_Profile = nullptr;
	}

	m_Profiles.clear();

	delete m_pDefaultProfile;
	m_pDefaultProfile = nullptr;
}

// find profiles and setup list
void CBotProfiles :: setupProfiles ()
{
	// Setup Default profile
	// TODO: to maybe balance the aim skill and reaction times from being too high [APG]RoboCop[CL]
	m_pDefaultProfile = new CBotProfile(
		DEFAULT_BOT_NAME, // name
		"default", // model (team in HL2DM)
		-1, // iTeam
		CBotVisibles::DEFAULT_MAX_TICKS, // vis ticks
		IBotNavigator::MAX_PATH_TICKS, // path ticks
		10, // visrevs clients
		8, // sensitivity
		0.7f, // braveness
		0.85f, // aim skill
		-1 // class
		);	

	// read profiles
	unsigned iId = 1;
	bool bDone = false;

	while ( iId < 999 && !bDone )
	{
		char szId[4];
		char filename[512];
		snprintf(szId, sizeof(szId), "%d", iId);
		CBotGlobals::buildFileName(filename,szId,BOT_PROFILE_FOLDER,BOT_CONFIG_EXTENSION);

		if ( std::fstream fp = CBotGlobals::openFile(filename, std::fstream::in) )
		{
			// copy defaults
			CBotProfile read = *m_pDefaultProfile;
			CRCBotKeyValueList kvl;

			logger->Log(LogLevel::INFO, "Reading bot profile \"%s\"", filename);

			kvl.parseFile(fp);

			kvl.getInt("team", &read.m_iTeam);
			kvl.getString("model", &read.m_szModel);
			kvl.getString("name", &read.m_szName);
			kvl.getInt("visionticks", &read.m_iVisionTicks);
			kvl.getInt("pathticks", &read.m_iPathTicks);
			kvl.getInt("visionticks_clients", &read.m_iVisionTicksClients);
			kvl.getInt("sensitivity", &read.m_iSensitivity);

			// config maps [ 0.0, 100.0 ] to [ 0.0, 1.0 ]
			float flWholeValuePercent;
			if (kvl.getFloat("aim_skill", &flWholeValuePercent)) {
				read.m_fAimSkill = flWholeValuePercent / 100.0f;
			} else if (kvl.getFloat("aimskill", &flWholeValuePercent)) {
				// *someone* wrote a broken bot profile generator.
				// most of the profiles did not actually have working aim skill values
				// we'll go ahead and allow it, but I have to express my displeasure about the matter in some way
				logger->Log(LogLevel::WARN,
						"Incorrect option 'aimskill' on bot profile \"%s\". "
						"Did you mean 'aim_skill'?", filename);
				read.m_fAimSkill = flWholeValuePercent / 100.0f;
			}
			
			if (kvl.getFloat("braveness", &flWholeValuePercent)) {
				read.m_fBraveness = flWholeValuePercent / 100.0f;
			}

			kvl.getInt("class", &read.m_iClass);

			m_Profiles.emplace_back(new CBotProfile(read));
		}
		else
		{
			bDone = true;
			logger->Log(LogLevel::DEBUG, "Bot profile \"%s\" not found", filename);
		}

		iId ++;
	}

}

CBotProfile *CBotProfiles :: getDefaultProfile ()
{
	if ( m_pDefaultProfile == nullptr)
		logger->Log(LogLevel::FATAL, "Default profile is NULL (Caused by memory problem, bad initialisation or overwrite) Exiting..");

	return m_pDefaultProfile;
}

// return a profile unused by a bot
CBotProfile* CBotProfiles::getRandomFreeProfile()
{
	std::vector<CBotProfile*> freeProfiles;

	for (CBotProfile*& m_Profile : m_Profiles)
	{
		if (!CBots::findBotByProfile(m_Profile))
			freeProfiles.emplace_back(m_Profile);
	}

	if (freeProfiles.empty())
		return nullptr;

	return freeProfiles[static_cast<std::size_t>(randomInt(0, static_cast<int>(freeProfiles.size()) - 1))];
}

// return first unused bot, concidering teams if they're unequal
CBotProfile* CBotProfiles::getChosenFreeProfile()
{
	int team;
	const int teamA = CBotGlobals::numPlayersOnTeam(2,false);
	const int teamB = CBotGlobals::numPlayersOnTeam(3,false);

	if (teamA < teamB) {
		team = 2;
	} else if (teamA > teamB) {
		team = 3;
	} else {
		team = 0;
	}

	for (CBotProfile*& m_Profile : m_Profiles)
	{
		if ((!CBots::findBotByProfile(m_Profile)) && ((team < 2) || (m_Profile->m_iTeam < 2) || (m_Profile->m_iTeam == team))) {
			return(m_Profile);
		}
	}

	// fall back to random if matching fails
	return CBotProfiles::getRandomFreeProfile();
}
