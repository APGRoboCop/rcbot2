/**
 * RCBot2 Extended Features Demonstration Plugin
 *
 * Demonstrates the extended SourceMod integration features including:
 * - Phase 6: Profile management and statistics
 * - Phase 7: Weapon preferences
 * - TF2 Event Forwards
 * - DOD:S and HL2DM game-specific natives
 */

#include <sourcemod>
#include <rcbot2>
#include <rcbot2_tf2>
#include <rcbot2_dod>
#include <rcbot2_hldm>

#pragma semicolon 1
#pragma newdecls required

public Plugin myinfo = {
	name = "RCBot2 Extended Features Demo",
	author = "RCBot2 Team",
	description = "Demonstrates extended SourceMod integration features",
	version = "1.0.0",
	url = "https://github.com/APGRoboCop/rcbot2"
};

public void OnPluginStart() {
	// Register commands
	RegAdminCmd("sm_rcbot_stats", Cmd_BotStats, ADMFLAG_ROOT, "Show bot statistics");
	RegAdminCmd("sm_rcbot_saveprofile", Cmd_SaveProfile, ADMFLAG_ROOT, "Save bot profile");
}

//=============================================================================
// Event Handlers - Core Events
//=============================================================================

public void RCBot2_OnBotSpawn(int client) {
	PrintToChatAll("[RCBot2] Bot %N has spawned", client);
}

public void RCBot2_OnBotDeath(int client, int attacker) {
	if (attacker > 0 && attacker <= MaxClients && IsClientInGame(attacker)) {
		PrintToChatAll("[RCBot2] Bot %N was killed by %N", client, attacker);
	}
}

public void RCBot2_OnBotKill(int client, int victim) {
	if (victim > 0 && victim <= MaxClients && IsClientInGame(victim)) {
		PrintToChatAll("[RCBot2] Bot %N killed %N", client, victim);
	}
}

//=============================================================================
// Event Handlers - TF2 Events
//=============================================================================

public void RCBot2_TF2_OnBotBuildingPlaced(int client, int buildingType, int entity) {
	char buildingName[32];
	switch(buildingType) {
		case 0: buildingName = "Dispenser";
		case 1: buildingName = "Teleporter";
		case 2: buildingName = "Sentry";
		default: buildingName = "Unknown";
	}
	PrintToChatAll("[RCBot2 TF2] Engineer bot %N placed a %s", client, buildingName);
}

public void RCBot2_TF2_OnBotUberDeployed(int client, int target) {
	if (target > 0 && target <= MaxClients && IsClientInGame(target)) {
		PrintToChatAll("[RCBot2 TF2] Medic bot %N deployed uber on %N", client, target);
	}
}

public void RCBot2_TF2_OnBotClassChanged(int client, int oldClass, int newClass) {
	PrintToChatAll("[RCBot2 TF2] Bot %N changed class from %d to %d", client, oldClass, newClass);
}

//=============================================================================
// Commands
//=============================================================================

public Action Cmd_BotStats(int client, int args) {
	for (int i = 1; i <= MaxClients; i++) {
		if (IsClientInGame(i) && IsRCBot2Client(i)) {
			int kills = RCBot2_GetBotKills(i);
			int deaths = RCBot2_GetBotDeaths(i);
			float kd = deaths > 0 ? float(kills) / float(deaths) : float(kills);

			ReplyToCommand(client, "[RCBot2] %N - Kills: %d | Deaths: %d | K/D: %.2f",
				i, kills, deaths, kd);
		}
	}
	return Plugin_Handled;
}

public Action Cmd_SaveProfile(int client, int args) {
	if (args < 2) {
		ReplyToCommand(client, "Usage: sm_rcbot_saveprofile <bot_name> <filename>");
		return Plugin_Handled;
	}

	char targetName[64], filename[128];
	GetCmdArg(1, targetName, sizeof(targetName));
	GetCmdArg(2, filename, sizeof(filename));

	int target = FindBotByName(targetName);
	if (target == -1) {
		ReplyToCommand(client, "[RCBot2] Bot not found: %s", targetName);
		return Plugin_Handled;
	}

	if (RCBot2_SaveProfile(target, filename)) {
		ReplyToCommand(client, "[RCBot2] Profile saved for %N to %s", target, filename);
	} else {
		ReplyToCommand(client, "[RCBot2] Failed to save profile");
	}

	return Plugin_Handled;
}

//=============================================================================
// Helper Functions
//=============================================================================

int FindBotByName(const char[] name) {
	char botName[64];
	for (int i = 1; i <= MaxClients; i++) {
		if (IsClientInGame(i) && IsRCBot2Client(i)) {
			GetClientName(i, botName, sizeof(botName));
			if (StrEqual(name, botName, false)) {
				return i;
			}
		}
	}
	return -1;
}
