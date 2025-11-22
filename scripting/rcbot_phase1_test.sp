/**
 * Phase 1 Integration Test Plugin
 * Tests all Phase 1 SourceMod natives for RCBot2
 */
#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>
#include <rcbot2>

public Plugin myinfo = {
	name = "RCBot2 Phase 1 Test",
	author = "RCBot2 Development Team",
	description = "Test plugin for Phase 1 SourceMod integration natives",
	version = "1.0.0",
	url = "https://github.com/APGRoboCop/rcbot2"
};

public void OnPluginStart() {
	RegAdminCmd("sm_rcbot_test_all", CMD_TestAll, ADMFLAG_ROOT, "Run all Phase 1 tests");
	RegAdminCmd("sm_rcbot_test_control", CMD_TestControl, ADMFLAG_ROOT, "Test bot command & control");
	RegAdminCmd("sm_rcbot_test_weapons", CMD_TestWeapons, ADMFLAG_ROOT, "Test weapon management");
	RegAdminCmd("sm_rcbot_test_tasks", CMD_TestTasks, ADMFLAG_ROOT, "Test task system queries");
}

public Action CMD_TestAll(int client, int args) {
	ReplyToCommand(client, "[RCBot] Running comprehensive Phase 1 tests...");

	CMD_TestControl(client, 0);
	CMD_TestWeapons(client, 0);
	CMD_TestTasks(client, 0);

	ReplyToCommand(client, "[RCBot] All Phase 1 tests completed!");
	return Plugin_Handled;
}

public Action CMD_TestControl(int client, int args) {
	ReplyToCommand(client, "\n===== Bot Command & Control Tests =====");

	// Find or create an RCBot
	int bot = FindRCBot();
	if (bot == -1) {
		bot = RCBot2_CreateBot("TestBot");
		if (bot == -1) {
			ReplyToCommand(client, "[FAIL] Could not create RCBot");
			return Plugin_Handled;
		}
		ReplyToCommand(client, "[PASS] Created RCBot #%d", bot);
	} else {
		ReplyToCommand(client, "[INFO] Using existing RCBot #%d", bot);
	}

	// Test RCBot2_GetBotTeam
	int team = RCBot2_GetBotTeam(bot);
	ReplyToCommand(client, "[TEST] Bot team: %d", team);

	// Test RCBot2_GetBotHealth
	int health = RCBot2_GetBotHealth(bot);
	ReplyToCommand(client, "[TEST] Bot health: %d", health);

	// Test RCBot2_GetBotOrigin
	float origin[3];
	if (RCBot2_GetBotOrigin(bot, origin)) {
		ReplyToCommand(client, "[PASS] Bot origin: %.1f, %.1f, %.1f", origin[0], origin[1], origin[2]);
	} else {
		ReplyToCommand(client, "[FAIL] Could not get bot origin");
	}

	// Test RCBot2_GetBotEyeAngles
	float angles[3];
	if (RCBot2_GetBotEyeAngles(bot, angles)) {
		ReplyToCommand(client, "[PASS] Bot eye angles: %.1f, %.1f, %.1f", angles[0], angles[1], angles[2]);
	} else {
		ReplyToCommand(client, "[FAIL] Could not get bot eye angles");
	}

	// Test RCBot2_GetBotEnemy
	int enemy = RCBot2_GetBotEnemy(bot);
	if (enemy == -1) {
		ReplyToCommand(client, "[INFO] Bot has no enemy");
	} else {
		ReplyToCommand(client, "[INFO] Bot enemy: %d", enemy);
	}

	// Test RCBot2_SetBotEnemy (if there's a valid player)
	int targetPlayer = FindValidTarget(bot);
	if (targetPlayer != -1) {
		if (RCBot2_SetBotEnemy(bot, targetPlayer)) {
			ReplyToCommand(client, "[PASS] Set bot enemy to #%d", targetPlayer);

			// Verify it was set
			int verifyEnemy = RCBot2_GetBotEnemy(bot);
			if (verifyEnemy == targetPlayer) {
				ReplyToCommand(client, "[PASS] Enemy setting verified");
			} else {
				ReplyToCommand(client, "[WARN] Enemy not set correctly (got %d, expected %d)", verifyEnemy, targetPlayer);
			}

			// Clear enemy
			if (RCBot2_SetBotEnemy(bot, -1)) {
				ReplyToCommand(client, "[PASS] Cleared bot enemy");
			}
		} else {
			ReplyToCommand(client, "[FAIL] Could not set bot enemy");
		}
	} else {
		ReplyToCommand(client, "[SKIP] No valid target to test enemy setting");
	}

	// Test RCBot2_ForceBotAction (Jump)
	if (RCBot2_ForceBotAction(bot, RCBotAction_Jump)) {
		ReplyToCommand(client, "[PASS] Forced bot to jump");
	} else {
		ReplyToCommand(client, "[FAIL] Could not force jump");
	}

	// Test RCBot2_ForceBotAction (Crouch)
	if (RCBot2_ForceBotAction(bot, RCBotAction_Crouch)) {
		ReplyToCommand(client, "[PASS] Forced bot to crouch");
	} else {
		ReplyToCommand(client, "[FAIL] Could not force crouch");
	}

	// Test RCBot2_GotoOrigin
	float testOrigin[3];
	testOrigin[0] = origin[0] + 100.0;
	testOrigin[1] = origin[1];
	testOrigin[2] = origin[2];

	if (RCBot2_GotoOrigin(bot, testOrigin)) {
		ReplyToCommand(client, "[PASS] Commanded bot to move to %.1f, %.1f, %.1f", testOrigin[0], testOrigin[1], testOrigin[2]);
	} else {
		ReplyToCommand(client, "[FAIL] Could not command bot to move");
	}

	// Test RCBot2_StopMovement
	if (RCBot2_StopMovement(bot)) {
		ReplyToCommand(client, "[PASS] Stopped bot movement");
	} else {
		ReplyToCommand(client, "[FAIL] Could not stop bot movement");
	}

	ReplyToCommand(client, "===== Control Tests Complete =====\n");
	return Plugin_Handled;
}

public Action CMD_TestWeapons(int client, int args) {
	ReplyToCommand(client, "\n===== Weapon Management Tests =====");

	int bot = FindRCBot();
	if (bot == -1) {
		ReplyToCommand(client, "[FAIL] No RCBot found. Create one first.");
		return Plugin_Handled;
	}

	// Test RCBot2_GetCurrentWeapon
	char weapon[64];
	if (RCBot2_GetCurrentWeapon(bot, weapon, sizeof(weapon))) {
		ReplyToCommand(client, "[PASS] Current weapon: %s", weapon);
	} else {
		ReplyToCommand(client, "[INFO] Bot has no weapon equipped");
	}

	// Test RCBot2_ForceAttack (primary)
	if (RCBot2_ForceAttack(bot, true, 0.5)) {
		ReplyToCommand(client, "[PASS] Forced primary attack for 0.5s");
	} else {
		ReplyToCommand(client, "[FAIL] Could not force primary attack");
	}

	// Test RCBot2_ForceAttack (secondary)
	if (RCBot2_ForceAttack(bot, false, 0.2)) {
		ReplyToCommand(client, "[PASS] Forced secondary attack for 0.2s");
	} else {
		ReplyToCommand(client, "[FAIL] Could not force secondary attack");
	}

	// Test RCBot2_ForceReload
	if (RCBot2_ForceReload(bot)) {
		ReplyToCommand(client, "[PASS] Forced bot to reload");
	} else {
		ReplyToCommand(client, "[FAIL] Could not force reload");
	}

	// Test RCBot2_SelectWeapon (game-specific weapons would be tested in actual gameplay)
	ReplyToCommand(client, "[INFO] Weapon selection test skipped (requires game-specific weapon names)");

	ReplyToCommand(client, "===== Weapon Tests Complete =====\n");
	return Plugin_Handled;
}

public Action CMD_TestTasks(int client, int args) {
	ReplyToCommand(client, "\n===== Task System Tests =====");

	int bot = FindRCBot();
	if (bot == -1) {
		ReplyToCommand(client, "[FAIL] No RCBot found. Create one first.");
		return Plugin_Handled;
	}

	// Test RCBot2_GetCurrentSchedule
	RCBotSchedule schedule = RCBot2_GetCurrentSchedule(bot);
	ReplyToCommand(client, "[INFO] Current schedule: %d (%s)", schedule, GetScheduleName(schedule));

	// Test RCBot2_HasSchedule
	bool hasAttack = RCBot2_HasSchedule(bot, RCBotSchedule_Attack);
	ReplyToCommand(client, "[INFO] Has attack schedule: %s", hasAttack ? "Yes" : "No");

	bool hasDefend = RCBot2_HasSchedule(bot, RCBotSchedule_Defend);
	ReplyToCommand(client, "[INFO] Has defend schedule: %s", hasDefend ? "Yes" : "No");

	// Test RCBot2_ClearSchedule
	if (RCBot2_ClearSchedule(bot)) {
		ReplyToCommand(client, "[PASS] Cleared bot schedule");

		// Verify schedule was cleared
		RCBotSchedule newSchedule = RCBot2_GetCurrentSchedule(bot);
		if (newSchedule == RCBotSchedule_None) {
			ReplyToCommand(client, "[PASS] Schedule clear verified");
		} else {
			ReplyToCommand(client, "[WARN] Schedule still active: %d", newSchedule);
		}
	} else {
		ReplyToCommand(client, "[INFO] Could not clear schedule (may be empty)");
	}

	ReplyToCommand(client, "===== Task Tests Complete =====\n");
	return Plugin_Handled;
}

// Helper functions

int FindRCBot() {
	for (int i = 1; i <= MaxClients; i++) {
		if (IsClientInGame(i) && IsRCBot2Client(i)) {
			return i;
		}
	}
	return -1;
}

int FindValidTarget(int bot) {
	for (int i = 1; i <= MaxClients; i++) {
		if (i == bot) continue;
		if (!IsClientInGame(i)) continue;
		if (!IsPlayerAlive(i)) continue;

		// Found a valid target
		return i;
	}
	return -1;
}

const char[] GetScheduleName(RCBotSchedule schedule) {
	switch (schedule) {
		case RCBotSchedule_None: return "None";
		case RCBotSchedule_Attack: return "Attack";
		case RCBotSchedule_RunForCover: return "RunForCover";
		case RCBotSchedule_GotoOrigin: return "GotoOrigin";
		case RCBotSchedule_GoodHideSpot: return "GoodHideSpot";
		case RCBotSchedule_GetHealth: return "GetHealth";
		case RCBotSchedule_Build: return "Build";
		case RCBotSchedule_Heal: return "Heal";
		case RCBotSchedule_GetMetal: return "GetMetal";
		case RCBotSchedule_Snipe: return "Snipe";
		case RCBotSchedule_Upgrade: return "Upgrade";
		case RCBotSchedule_UseTele: return "UseTele";
		case RCBotSchedule_SapBuilding: return "SapBuilding";
		case RCBotSchedule_UseDispenser: return "UseDispenser";
		case RCBotSchedule_Pickup: return "Pickup";
		case RCBotSchedule_GetAmmo: return "GetAmmo";
		case RCBotSchedule_FindFlag: return "FindFlag";
		case RCBotSchedule_LookAfterSentry: return "LookAfterSentry";
		case RCBotSchedule_Defend: return "Defend";
		case RCBotSchedule_AttackPoint: return "AttackPoint";
		case RCBotSchedule_DefendPoint: return "DefendPoint";
		case RCBotSchedule_Backstab: return "Backstab";
		case RCBotSchedule_RemoveSapper: return "RemoveSapper";
		case RCBotSchedule_Follow: return "Follow";
		case RCBotSchedule_InvestigateNoise: return "InvestigateNoise";
		case RCBotSchedule_Other: return "Other";
		default: return "Unknown";
	}
}
