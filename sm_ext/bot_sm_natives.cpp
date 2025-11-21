// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include "bot_sm_natives.h"
#include "bot_sm_ext.h"

#include "bot.h"
#include "bot_profile.h"
#include "bot_waypoint.h"

enum RCBotProfileVar : std::uint8_t {
	RCBotProfile_iVisionTicks,
	RCBotProfile_iPathTicks,
	RCBotProfile_iVisionTicksClients,
	RCBotProfile_iSensitivity,
	RCBotProfile_fBraveness,
	RCBotProfile_fAimSkill,
	RCBotProfile_iClass,
};

int* GetIntProperty(CBotProfile* profile, RCBotProfileVar profileVar);
float* GetFloatProperty(CBotProfile* profile, RCBotProfileVar profileVar);

cell_t sm_RCBotIsWaypointAvailable(IPluginContext *pContext, const cell_t *params) {
	return CWaypoints::numWaypoints() > 0;
}

cell_t sm_RCBotCreate(IPluginContext *pContext, const cell_t *params) {
	char *name;
	pContext->LocalToString(params[1], &name);

	const int slot = CBots::createDefaultBot(name);
	
	// player slots are off-by-one (though this calculation is performed in the function)
	return (slot != -1)? slot + 1 : -1;
}

/* native void RCBot2_SetProfileInt(int client, RCBotProfileVar property, int value); */
cell_t sm_RCBotSetProfileInt(IPluginContext* pContext, const cell_t* params) {
	const int client = params[1];
	const RCBotProfileVar profileVar = static_cast<RCBotProfileVar>(params[2]);
	if (client < 1 || client > gpGlobals->maxClients) {
		pContext->ThrowNativeError("Invalid client index %d", client);
		return 0; // Ensure the function exits
	}

	const CBot* bot = CBots::getBot(client - 1);
	if (!bot) {
		pContext->ThrowNativeError("Client index %d is not a RCBot", client);
		return 0; // Ensure the function exits
	}

	CBotProfile* profile = bot->getProfile();
	int* value = GetIntProperty(profile, profileVar);
	if (!value) {
		pContext->ThrowNativeError("RCBot property %d is not an integer property", profileVar);
		return 0;
	}
	*value = params[3];

	return 0;
}

/* native void RCBot2_SetProfileFloat(int client, RCBotProfileVar property, float value); */
cell_t sm_RCBotSetProfileFloat(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];
	const RCBotProfileVar profileVar = static_cast<RCBotProfileVar>(params[2]);
	if (client < 1 || client > gpGlobals->maxClients) {
		pContext->ThrowNativeError("Invalid client index %d", client);
	}

	const CBot* bot = CBots::getBot(client - 1);
	if (!bot) {
		return pContext->ThrowNativeError("Client index %d is not a RCBot", client);
	}

	CBotProfile* profile = bot->getProfile();
	float* value = GetFloatProperty(profile, profileVar);
	if (!value) {
		pContext->ThrowNativeError("RCBot property %d is not a float property", profileVar);
		return 0;
	}
	*value = sp_ctof(params[3]);
	
	return 0;
}

/* native int RCBot2_GetProfileInt(int client, RCBotProfileVar property); */
cell_t sm_RCBotGetProfileInt(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];
	const RCBotProfileVar profileVar = static_cast<RCBotProfileVar>(params[2]);
	if (client < 1 || client > gpGlobals->maxClients) {
		pContext->ThrowNativeError("Invalid client index %d", client);
		return 0;
	}

	const CBot* bot = CBots::getBot(client - 1);
	if (!bot) {
		pContext->ThrowNativeError("Client index %d is not a RCBot", client);
		return 0;
	}
	
	CBotProfile* profile = bot->getProfile();
	const int* value = GetIntProperty(profile, profileVar);
	if (!value) {
		pContext->ThrowNativeError("RCBot property %d is not an integer property", profileVar);
		return 0;
	}
	return *value;
}

/* native float RCBot2_GetProfileFloat(int client, RCBotProfileVar property); */
cell_t sm_RCBotGetProfileFloat(IPluginContext* pContext, const cell_t* params) {
	const int client = params[1];
	const RCBotProfileVar profileVar = static_cast<RCBotProfileVar>(params[2]);
	if (client < 1 || client > gpGlobals->maxClients) {
		pContext->ThrowNativeError("Invalid client index %d", client);
		return 0;
	}

	const CBot* bot = CBots::getBot(client - 1);
	if (!bot) {
		pContext->ThrowNativeError("Client index %d is not a RCBot", client);
		return 0;
	}

	CBotProfile* profile = bot->getProfile();
	const float* value = GetFloatProperty(profile, profileVar);
	if (!value) {
		pContext->ThrowNativeError("RCBot property %d is not a float property", profileVar);
		return 0;
	}
	return sp_ftoc(*value);
}

cell_t sm_RCBotIsClientBot(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];
	if (client < 1 || client > gpGlobals->maxClients) {
		pContext->ThrowNativeError("Invalid client index %d", client);
		return 0;
	}
	return CBots::getBot(client - 1) != nullptr;
}

int* GetIntProperty(CBotProfile* profile, const RCBotProfileVar profileVar) {
	switch (profileVar) {
		case RCBotProfile_iVisionTicks:
			return &profile->m_iVisionTicks;
		case RCBotProfile_iPathTicks:
			return &profile->m_iPathTicks;
		case RCBotProfile_iClass:
			return &profile->m_iClass;
		case RCBotProfile_iVisionTicksClients:
			return &profile->m_iVisionTicksClients;
		case RCBotProfile_iSensitivity:
			return &profile->m_iSensitivity;
	}
	return nullptr;
}

float* GetFloatProperty(CBotProfile* profile, const RCBotProfileVar profileVar) {
	switch (profileVar) {
		case RCBotProfile_fBraveness:
			return &profile->m_fBraveness;
		case RCBotProfile_fAimSkill:
			return &profile->m_fAimSkill;
	}
	return nullptr;
}

// ========== Enhanced SourceMod Natives - Roadmap Medium-Term Goals ==========

/* native bool RCBot2_KickBot(int client); */
cell_t sm_RCBotKickBot(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];
	if (client < 1 || client > gpGlobals->maxClients) {
		pContext->ThrowNativeError("Invalid client index %d", client);
		return 0;
	}

	CBot* bot = CBots::getBot(client - 1);
	if (!bot) {
		pContext->ThrowNativeError("Client index %d is not a RCBot", client);
		return 0;
	}

	CBots::kickBot(bot->getEdict());
	return 1;
}

/* native int RCBot2_KickAll(); */
cell_t sm_RCBotKickAll(IPluginContext *pContext, const cell_t *params) {
	int kicked = 0;

	for (int i = 0; i < gpGlobals->maxClients; i++) {
		CBot* bot = CBots::getBot(i);
		if (bot) {
			CBots::kickBot(bot->getEdict());
			kicked++;
		}
	}

	return kicked;
}

/* native int RCBot2_GetBotCount(); */
cell_t sm_RCBotGetBotCount(IPluginContext *pContext, const cell_t *params) {
	int count = 0;

	for (int i = 0; i < gpGlobals->maxClients; i++) {
		if (CBots::getBot(i) != nullptr) {
			count++;
		}
	}

	return count;
}

/* native void RCBot2_SetSkill(int client, float skill); */
cell_t sm_RCBotSetSkill(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];
	if (client < 1 || client > gpGlobals->maxClients) {
		pContext->ThrowNativeError("Invalid client index %d", client);
		return 0;
	}

	const CBot* bot = CBots::getBot(client - 1);
	if (!bot) {
		pContext->ThrowNativeError("Client index %d is not a RCBot", client);
		return 0;
	}

	const float skill = sp_ctof(params[2]);

	// Skill affects multiple profile parameters
	CBotProfile* profile = bot->getProfile();
	if (profile) {
		// Clamp skill to 0.0 - 1.0 range
		const float clampedSkill = (skill < 0.0f) ? 0.0f : ((skill > 1.0f) ? 1.0f : skill);

		profile->m_fAimSkill = clampedSkill;
		profile->m_fBraveness = clampedSkill * 0.8f; // Skill affects braveness too
		// Higher skill = faster reactions
		profile->m_iVisionTicks = static_cast<int>(20.0f - (clampedSkill * 15.0f)); // 5-20 ticks
		profile->m_iPathTicks = static_cast<int>(10.0f - (clampedSkill * 8.0f)); // 2-10 ticks
	}

	return 0;
}

/* native float RCBot2_GetSkill(int client); */
cell_t sm_RCBotGetSkill(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];
	if (client < 1 || client > gpGlobals->maxClients) {
		pContext->ThrowNativeError("Invalid client index %d", client);
		return 0;
	}

	const CBot* bot = CBots::getBot(client - 1);
	if (!bot) {
		pContext->ThrowNativeError("Client index %d is not a RCBot", client);
		return 0;
	}

	CBotProfile* profile = bot->getProfile();
	if (profile) {
		return sp_ftoc(profile->m_fAimSkill);
	}

	return sp_ftoc(0.5f); // Default medium skill
}

/* native int RCBot2_GetTeam(int client); */
cell_t sm_RCBotGetTeam(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];
	if (client < 1 || client > gpGlobals->maxClients) {
		pContext->ThrowNativeError("Invalid client index %d", client);
		return 0;
	}

	const CBot* bot = CBots::getBot(client - 1);
	if (!bot) {
		pContext->ThrowNativeError("Client index %d is not a RCBot", client);
		return 0;
	}

	return bot->getTeam();
}

/* native int RCBot2_GetClass(int client); */
cell_t sm_RCBotGetClass(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];
	if (client < 1 || client > gpGlobals->maxClients) {
		pContext->ThrowNativeError("Invalid client index %d", client);
		return 0;
	}

	const CBot* bot = CBots::getBot(client - 1);
	if (!bot) {
		pContext->ThrowNativeError("Client index %d is not a RCBot", client);
		return 0;
	}

	CBotProfile* profile = bot->getProfile();
	if (profile) {
		return profile->m_iClass;
	}

	return 0; // Unknown class
}

/* native bool RCBot2_SetClass(int client, int classId); */
cell_t sm_RCBotSetClass(IPluginContext *pContext, const cell_t *params) {
	const int client = params[1];
	if (client < 1 || client > gpGlobals->maxClients) {
		pContext->ThrowNativeError("Invalid client index %d", client);
		return 0;
	}

	CBot* bot = CBots::getBot(client - 1);
	if (!bot) {
		pContext->ThrowNativeError("Client index %d is not a RCBot", client);
		return 0;
	}

	const int classId = params[2];

	// Set the desired class in the profile
	CBotProfile* profile = bot->getProfile();
	if (profile) {
		profile->m_iClass = classId;
		// Note: The bot will change class on next respawn based on this profile setting
		return 1;
	}

	return 0;
}
