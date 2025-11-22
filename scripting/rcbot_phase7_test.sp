/**
 * Phase 7 Perception & AI Configuration Test Plugin
 * Tests RCBot2 perception control and condition management natives
 */
#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>
#include <rcbot2>

public Plugin myinfo = {
	name = "RCBot2 Phase 7 Test",
	author = "RCBot2 Development Team",
	description = "Test plugin for Phase 7 perception and condition management natives",
	version = "7.0.0",
	url = "https://github.com/APGRoboCop/rcbot2"
};

public void OnPluginStart() {
	RegAdminCmd("sm_rcbot_test_phase7", CMD_TestPhase7, ADMFLAG_ROOT, "Run all Phase 7 tests");
	RegAdminCmd("sm_rcbot_test_fov", CMD_TestFOV, ADMFLAG_ROOT, "Test FOV get/set");
	RegAdminCmd("sm_rcbot_test_perception", CMD_TestPerception, ADMFLAG_ROOT, "Test visible enemies and nearby allies");
	RegAdminCmd("sm_rcbot_test_conditions", CMD_TestConditions, ADMFLAG_ROOT, "Test condition management");
}

public Action CMD_TestPhase7(int client, int args) {
	ReplyToCommand(client, "\n===== Phase 7 Perception & AI Configuration Tests =====");

	CMD_TestFOV(client, 0);
	CMD_TestPerception(client, 0);
	CMD_TestConditions(client, 0);

	ReplyToCommand(client, "===== All Phase 7 Tests Complete =====\n");
	return Plugin_Handled;
}

public Action CMD_TestFOV(int client, int args) {
	ReplyToCommand(client, "\n===== FOV Tests =====");

	// Find a bot to test with
	int bot = FindFirstBot();
	if (bot == -1) {
		ReplyToCommand(client, "[INFO] No bots found - creating test bot...");
		bot = RCBot2_CreateBot("TestBot_Phase7");
		if (bot == -1) {
			ReplyToCommand(client, "[FAIL] Could not create test bot");
			return Plugin_Handled;
		}
		CreateTimer(0.5, Timer_TestFOVDelayed, GetClientUserId(client));
		return Plugin_Handled;
	}

	// Test RCBot2_GetBotFOV
	float currentFOV = RCBot2_GetBotFOV(bot);
	ReplyToCommand(client, "[INFO] Bot #%d current FOV: %.1f degrees", bot, currentFOV);

	// Test RCBot2_SetBotFOV - narrow FOV
	if (RCBot2_SetBotFOV(bot, 60.0)) {
		float newFOV = RCBot2_GetBotFOV(bot);
		if (FloatAbs(newFOV - 60.0) < 0.1) {
			ReplyToCommand(client, "[PASS] Successfully set FOV to 60.0 degrees");
		} else {
			ReplyToCommand(client, "[FAIL] FOV not set correctly (got %.1f, expected 60.0)", newFOV);
		}
	} else {
		ReplyToCommand(client, "[FAIL] RCBot2_SetBotFOV returned false");
	}

	// Test wide FOV
	if (RCBot2_SetBotFOV(bot, 120.0)) {
		float wideFOV = RCBot2_GetBotFOV(bot);
		if (FloatAbs(wideFOV - 120.0) < 0.1) {
			ReplyToCommand(client, "[PASS] Successfully set FOV to 120.0 degrees");
		} else {
			ReplyToCommand(client, "[FAIL] FOV not set correctly (got %.1f, expected 120.0)", wideFOV);
		}
	} else {
		ReplyToCommand(client, "[FAIL] RCBot2_SetBotFOV returned false");
	}

	// Restore default FOV
	RCBot2_SetBotFOV(bot, 80.0);
	ReplyToCommand(client, "[INFO] Restored FOV to 80.0 degrees");

	ReplyToCommand(client, "===== FOV Tests Complete =====\n");
	return Plugin_Handled;
}

public Action Timer_TestFOVDelayed(Handle timer, int userid) {
	int client = GetClientOfUserId(userid);
	if (client == 0) {
		return Plugin_Stop;
	}
	CMD_TestFOV(client, 0);
	return Plugin_Stop;
}

public Action CMD_TestPerception(int client, int args) {
	ReplyToCommand(client, "\n===== Perception Tests =====");

	int bot = FindFirstBot();
	if (bot == -1) {
		ReplyToCommand(client, "[INFO] No bots found - create a bot first");
		return Plugin_Handled;
	}

	// Test RCBot2_GetVisibleEnemies
	int enemies[MAXPLAYERS];
	int enemyCount = RCBot2_GetVisibleEnemies(bot, enemies, sizeof(enemies));
	ReplyToCommand(client, "[INFO] Bot #%d can see %d enemies", bot, enemyCount);

	if (enemyCount > 0) {
		for (int i = 0; i < enemyCount; i++) {
			char name[64];
			GetClientName(enemies[i], name, sizeof(name));
			ReplyToCommand(client, "  - Enemy #%d: %s (client #%d)", i + 1, name, enemies[i]);
		}
		ReplyToCommand(client, "[PASS] Successfully retrieved visible enemies");
	} else {
		ReplyToCommand(client, "[INFO] No visible enemies found (expected in empty server)");
	}

	// Test RCBot2_GetNearbyAllies (500 unit radius)
	int allies[MAXPLAYERS];
	int allyCount = RCBot2_GetNearbyAllies(bot, allies, sizeof(allies), 500.0);
	ReplyToCommand(client, "[INFO] Bot #%d has %d allies within 500 units", bot, allyCount);

	if (allyCount > 0) {
		for (int i = 0; i < allyCount; i++) {
			char name[64];
			GetClientName(allies[i], name, sizeof(name));
			ReplyToCommand(client, "  - Ally #%d: %s (client #%d)", i + 1, name, allies[i]);
		}
		ReplyToCommand(client, "[PASS] Successfully retrieved nearby allies");
	} else {
		ReplyToCommand(client, "[INFO] No nearby allies found");
	}

	// Test with larger radius
	int alliesWide[MAXPLAYERS];
	int allyCountWide = RCBot2_GetNearbyAllies(bot, alliesWide, sizeof(alliesWide), 2000.0);
	ReplyToCommand(client, "[INFO] Bot #%d has %d allies within 2000 units", bot, allyCountWide);

	ReplyToCommand(client, "===== Perception Tests Complete =====\n");
	return Plugin_Handled;
}

public Action CMD_TestConditions(int client, int args) {
	ReplyToCommand(client, "\n===== Condition Management Tests =====");

	int bot = FindFirstBot();
	if (bot == -1) {
		ReplyToCommand(client, "[INFO] No bots found - create a bot first");
		return Plugin_Handled;
	}

	// Test getting current conditions
	int conditions[32];
	int condCount = RCBot2_GetConditions(bot, conditions, sizeof(conditions));
	ReplyToCommand(client, "[INFO] Bot #%d has %d active conditions", bot, condCount);

	if (condCount > 0) {
		ReplyToCommand(client, "[INFO] Active conditions:");
		for (int i = 0; i < condCount; i++) {
			char condName[64];
			GetConditionName(conditions[i], condName, sizeof(condName));
			ReplyToCommand(client, "  - %s (0x%X)", condName, conditions[i]);
		}
	}

	// Test RCBot2_SetCondition
	if (RCBot2_SetCondition(bot, view_as<int>(RCBotCond_NeedHealth))) {
		ReplyToCommand(client, "[PASS] Successfully set NeedHealth condition");

		// Verify it was set
		if (RCBot2_HasCondition(bot, view_as<int>(RCBotCond_NeedHealth))) {
			ReplyToCommand(client, "[PASS] Bot now has NeedHealth condition");
		} else {
			ReplyToCommand(client, "[FAIL] Condition not detected by HasCondition");
		}
	} else {
		ReplyToCommand(client, "[FAIL] Could not set NeedHealth condition");
	}

	// Test setting multiple conditions
	RCBot2_SetCondition(bot, view_as<int>(RCBotCond_NeedAmmo));
	RCBot2_SetCondition(bot, view_as<int>(RCBotCond_Push));

	// Get conditions again
	int newCondCount = RCBot2_GetConditions(bot, conditions, sizeof(conditions));
	ReplyToCommand(client, "[INFO] Bot now has %d active conditions", newCondCount);

	// Test RCBot2_RemoveCondition
	if (RCBot2_RemoveCondition(bot, view_as<int>(RCBotCond_NeedHealth))) {
		ReplyToCommand(client, "[PASS] Successfully removed NeedHealth condition");

		// Verify it was removed
		if (!RCBot2_HasCondition(bot, view_as<int>(RCBotCond_NeedHealth))) {
			ReplyToCommand(client, "[PASS] Bot no longer has NeedHealth condition");
		} else {
			ReplyToCommand(client, "[FAIL] Condition still detected after removal");
		}
	} else {
		ReplyToCommand(client, "[FAIL] Could not remove NeedHealth condition");
	}

	// Clean up test conditions
	RCBot2_RemoveCondition(bot, view_as<int>(RCBotCond_NeedAmmo));
	RCBot2_RemoveCondition(bot, view_as<int>(RCBotCond_Push));

	ReplyToCommand(client, "===== Condition Management Tests Complete =====\n");
	return Plugin_Handled;
}

// Helper function to find the first RCBot
int FindFirstBot() {
	for (int i = 1; i <= MaxClients; i++) {
		if (IsClientInGame(i) && IsRCBot2Client(i)) {
			return i;
		}
	}
	return -1;
}

// Helper function to get condition name
void GetConditionName(int condition, char[] name, int maxlen) {
	switch (condition) {
		case (1 << 0): Format(name, maxlen, "EnemyObscured");
		case (1 << 1): Format(name, maxlen, "NoWeapon");
		case (1 << 2): Format(name, maxlen, "OutOfAmmo");
		case (1 << 3): Format(name, maxlen, "SeeCurrentEnemy");
		case (1 << 4): Format(name, maxlen, "EnemyDead");
		case (1 << 5): Format(name, maxlen, "SeeWaypoint");
		case (1 << 6): Format(name, maxlen, "NeedAmmo");
		case (1 << 7): Format(name, maxlen, "NeedHealth");
		case (1 << 8): Format(name, maxlen, "SeeLookVector");
		case (1 << 9): Format(name, maxlen, "PointCaptured");
		case (1 << 10): Format(name, maxlen, "Push");
		case (1 << 11): Format(name, maxlen, "Lift");
		case (1 << 12): Format(name, maxlen, "SeePlayerToHelp");
		case (1 << 13): Format(name, maxlen, "SeeLastEnemyPos");
		case (1 << 14): Format(name, maxlen, "Changed");
		case (1 << 15): Format(name, maxlen, "Covert");
		case (1 << 16): Format(name, maxlen, "Run");
		case (1 << 17): Format(name, maxlen, "Grenade");
		case (1 << 18): Format(name, maxlen, "NeedBomb");
		case (1 << 19): Format(name, maxlen, "SeeEnemyHead");
		case (1 << 20): Format(name, maxlen, "Prone");
		case (1 << 21): Format(name, maxlen, "Paranoid");
		case (1 << 22): Format(name, maxlen, "SeeSquadLeader");
		case (1 << 23): Format(name, maxlen, "SquadLeaderDead");
		case (1 << 24): Format(name, maxlen, "SquadLeaderInRange");
		case (1 << 25): Format(name, maxlen, "SquadIdle");
		case (1 << 26): Format(name, maxlen, "Defensive");
		case (1 << 27): Format(name, maxlen, "BuildingSapped");
		case (1 << 28): Format(name, maxlen, "SeeEnemyGround");
		default: Format(name, maxlen, "Unknown(0x%X)", condition);
	}
}
