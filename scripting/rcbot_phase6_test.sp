/**
 * Phase 6 Advanced Bot Management Test Plugin
 * Tests RCBot2 bot management natives
 */
#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>
#include <rcbot2>

public Plugin myinfo = {
	name = "RCBot2 Phase 6 Test",
	author = "RCBot2 Development Team",
	description = "Test plugin for Phase 6 advanced bot management natives",
	version = "6.0.0",
	url = "https://github.com/APGRoboCop/rcbot2"
};

public void OnPluginStart() {
	RegAdminCmd("sm_rcbot_test_phase6", CMD_TestPhase6, ADMFLAG_ROOT, "Run all Phase 6 tests");
	RegAdminCmd("sm_rcbot_test_bot_count", CMD_TestBotCount, ADMFLAG_ROOT, "Test bot counting");
	RegAdminCmd("sm_rcbot_test_bot_iterate", CMD_TestBotIteration, ADMFLAG_ROOT, "Test bot iteration");
	RegAdminCmd("sm_rcbot_test_bot_kick", CMD_TestBotKick, ADMFLAG_ROOT, "Test bot kicking");
}

public Action CMD_TestPhase6(int client, int args) {
	ReplyToCommand(client, "\n===== Phase 6 Bot Management Tests =====");

	CMD_TestBotCount(client, 0);
	CMD_TestBotIteration(client, 0);
	CMD_TestBotKick(client, 0);

	ReplyToCommand(client, "===== All Phase 6 Tests Complete =====\n");
	return Plugin_Handled;
}

public Action CMD_TestBotCount(int client, int args) {
	ReplyToCommand(client, "\n===== Bot Counting Tests =====");

	// Test RCBot2_CountBots (all bots)
	int totalBots = RCBot2_CountBots();
	ReplyToCommand(client, "[INFO] Total RCBots on server: %d", totalBots);

	if (totalBots == 0) {
		ReplyToCommand(client, "[INFO] No bots found - creating test bots...");

		// Create a few test bots
		for (int i = 1; i <= 3; i++) {
			char botName[32];
			Format(botName, sizeof(botName), "TestBot%d", i);
			int bot = RCBot2_CreateBot(botName);
			if (bot != -1) {
				ReplyToCommand(client, "[PASS] Created %s (client #%d)", botName, bot);
			}
		}

		// Recount
		totalBots = RCBot2_CountBots();
		ReplyToCommand(client, "[INFO] Total bots after creation: %d", totalBots);
	}

	// Count bots per team (if game uses teams)
	int team2Bots = RCBot2_CountBots(2);
	int team3Bots = RCBot2_CountBots(3);
	ReplyToCommand(client, "[INFO] Team 2 bots: %d, Team 3 bots: %d", team2Bots, team3Bots);

	ReplyToCommand(client, "===== Bot Counting Tests Complete =====\n");
	return Plugin_Handled;
}

public Action CMD_TestBotIteration(int client, int args) {
	ReplyToCommand(client, "\n===== Bot Iteration Tests =====");

	int botCount = RCBot2_CountBots();
	if (botCount == 0) {
		ReplyToCommand(client, "[INFO] No bots to iterate");
		return Plugin_Handled;
	}

	ReplyToCommand(client, "[INFO] Iterating through %d bots:", botCount);

	for (int i = 0; i < botCount; i++) {
		int bot = RCBot2_GetBotByIndex(i);

		if (bot == -1) {
			ReplyToCommand(client, "[WARN] GetBotByIndex(%d) returned -1", i);
			continue;
		}

		if (!IsClientInGame(bot)) {
			ReplyToCommand(client, "[WARN] Bot #%d not in game", bot);
			continue;
		}

		// Test RCBot2_GetBotName
		char botName[64];
		if (RCBot2_GetBotName(bot, botName, sizeof(botName))) {
			int team = GetClientTeam(bot);
			int health = GetClientHealth(bot);
			ReplyToCommand(client, "  [%d] Client #%d: %s (Team %d, HP %d)",
				i, bot, botName, team, health);
		} else {
			ReplyToCommand(client, "[WARN] Could not get name for bot #%d", bot);
		}
	}

	ReplyToCommand(client, "===== Bot Iteration Tests Complete =====\n");
	return Plugin_Handled;
}

public Action CMD_TestBotKick(int client, int args) {
	ReplyToCommand(client, "\n===== Bot Kicking Tests =====");

	int botCount = RCBot2_CountBots();
	if (botCount == 0) {
		ReplyToCommand(client, "[INFO] No bots to kick");
		return Plugin_Handled;
	}

	ReplyToCommand(client, "[INFO] Current bot count: %d", botCount);

	// Get first bot
	int bot = RCBot2_GetBotByIndex(0);
	if (bot == -1) {
		ReplyToCommand(client, "[FAIL] Could not get first bot");
		return Plugin_Handled;
	}

	char botName[64];
	if (RCBot2_GetBotName(bot, botName, sizeof(botName))) {
		ReplyToCommand(client, "[INFO] Attempting to kick bot: %s (client #%d)", botName, bot);

		// Test RCBot2_KickBot
		if (RCBot2_KickBot(bot)) {
			ReplyToCommand(client, "[PASS] Successfully kicked bot #%d", bot);

			// Give it a moment, then recount
			CreateTimer(0.5, Timer_CheckKick, GetClientUserId(client));
		} else {
			ReplyToCommand(client, "[FAIL] Could not kick bot #%d", bot);
		}
	} else {
		ReplyToCommand(client, "[FAIL] Could not get bot name");
	}

	ReplyToCommand(client, "===== Bot Kicking Tests Complete =====\n");
	return Plugin_Handled;
}

public Action Timer_CheckKick(Handle timer, int userid) {
	int client = GetClientOfUserId(userid);
	if (client == 0) {
		return Plugin_Stop;
	}

	int newCount = RCBot2_CountBots();
	PrintToChat(client, "[INFO] Bot count after kick: %d", newCount);
	PrintToServer("[INFO] Bot count after kick: %d", newCount);

	return Plugin_Stop;
}
