/**
 * Phase 2 TF2 Integration Test Plugin
 * Tests all TF2-specific SourceMod natives for RCBot2
 */
#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>
#include <rcbot2>
#include <rcbot2_tf2>

public Plugin myinfo = {
	name = "RCBot2 TF2 Test",
	author = "RCBot2 Development Team",
	description = "Test plugin for TF2-specific SourceMod integration natives",
	version = "2.0.0",
	url = "https://github.com/APGRoboCop/rcbot2"
};

public void OnPluginStart() {
	RegAdminCmd("sm_rcbot_tf2_test_all", CMD_TestAllTF2, ADMFLAG_ROOT, "Run all TF2 tests");
	RegAdminCmd("sm_rcbot_tf2_test_class", CMD_TestClass, ADMFLAG_ROOT, "Test TF2 class management");
	RegAdminCmd("sm_rcbot_tf2_test_engi", CMD_TestEngineer, ADMFLAG_ROOT, "Test TF2 Engineer functions");
	RegAdminCmd("sm_rcbot_tf2_test_medic", CMD_TestMedic, ADMFLAG_ROOT, "Test TF2 Medic functions");
	RegAdminCmd("sm_rcbot_tf2_test_spy", CMD_TestSpy, ADMFLAG_ROOT, "Test TF2 Spy functions");
	RegAdminCmd("sm_rcbot_tf2_test_general", CMD_TestGeneral, ADMFLAG_ROOT, "Test TF2 general functions");
}

public Action CMD_TestAllTF2(int client, int args) {
	ReplyToCommand(client, "[RCBot TF2] Running comprehensive TF2 tests...");

	CMD_TestClass(client, 0);
	CMD_TestEngineer(client, 0);
	CMD_TestMedic(client, 0);
	CMD_TestSpy(client, 0);
	CMD_TestGeneral(client, 0);

	ReplyToCommand(client, "[RCBot TF2] All TF2 tests completed!");
	return Plugin_Handled;
}

public Action CMD_TestClass(int client, int args) {
	ReplyToCommand(client, "\n===== TF2 Class Management Tests =====");

	int bot = FindRCBot();
	if (bot == -1) {
		bot = RCBot2_CreateBot("TF2TestBot");
		if (bot == -1) {
			ReplyToCommand(client, "[FAIL] Could not create RCBot");
			return Plugin_Handled;
		}
		ReplyToCommand(client, "[PASS] Created RCBot #%d", bot);
	} else {
		ReplyToCommand(client, "[INFO] Using existing RCBot #%d", bot);
	}

	// Test RCBot2_TF2_GetClass
	TFClassType currentClass = RCBot2_TF2_GetClass(bot);
	ReplyToCommand(client, "[INFO] Current class: %s", GetClassName(currentClass));

	// Test RCBot2_TF2_SetClass (Soldier)
	if (RCBot2_TF2_SetClass(bot, TFClass_Soldier)) {
		ReplyToCommand(client, "[PASS] Set bot class to Soldier");

		// Verify
		TFClassType verifyClass = RCBot2_TF2_GetClass(bot);
		if (verifyClass == TFClass_Soldier) {
			ReplyToCommand(client, "[PASS] Class change verified");
		} else {
			ReplyToCommand(client, "[WARN] Class not changed (got %s)", GetClassName(verifyClass));
		}
	} else {
		ReplyToCommand(client, "[FAIL] Could not set class");
	}

	// Test various classes
	TFClassType testClasses[] = {TFClass_Scout, TFClass_Medic, TFClass_Engineer, TFClass_Spy};
	char classNames[][] = {"Scout", "Medic", "Engineer", "Spy"};

	for (int i = 0; i < sizeof(testClasses); i++) {
		if (RCBot2_TF2_SetClass(bot, testClasses[i])) {
			ReplyToCommand(client, "[PASS] Set class to %s", classNames[i]);
		} else {
			ReplyToCommand(client, "[FAIL] Could not set class to %s", classNames[i]);
		}
	}

	ReplyToCommand(client, "===== Class Tests Complete =====\n");
	return Plugin_Handled;
}

public Action CMD_TestEngineer(int client, int args) {
	ReplyToCommand(client, "\n===== TF2 Engineer Tests =====");

	int bot = FindRCBot();
	if (bot == -1) {
		ReplyToCommand(client, "[FAIL] No RCBot found");
		return Plugin_Handled;
	}

	// Set bot to Engineer
	if (!RCBot2_TF2_SetClass(bot, TFClass_Engineer)) {
		ReplyToCommand(client, "[FAIL] Could not set bot to Engineer");
		return Plugin_Handled;
	}

	ReplyToCommand(client, "[INFO] Bot set to Engineer");

	// Test building construction
	if (RCBot2_TF2_EngineerBuild(bot, TFObject_Sentry, TFEngiCmd_Build)) {
		ReplyToCommand(client, "[PASS] Commanded bot to build Sentry");
	} else {
		ReplyToCommand(client, "[FAIL] Could not command Sentry build");
	}

	if (RCBot2_TF2_EngineerBuild(bot, TFObject_Dispenser, TFEngiCmd_Build)) {
		ReplyToCommand(client, "[PASS] Commanded bot to build Dispenser");
	} else {
		ReplyToCommand(client, "[FAIL] Could not command Dispenser build");
	}

	if (RCBot2_TF2_EngineerBuild(bot, TFObject_Teleporter, TFEngiCmd_Build)) {
		ReplyToCommand(client, "[PASS] Commanded bot to build Teleporter");
	} else {
		ReplyToCommand(client, "[FAIL] Could not command Teleporter build");
	}

	// Test building queries
	bool hasSentry = RCBot2_TF2_HasBuilding(bot, TFObject_Sentry);
	ReplyToCommand(client, "[INFO] Has Sentry: %s", hasSentry ? "Yes" : "No");

	bool hasDispenser = RCBot2_TF2_HasBuilding(bot, TFObject_Dispenser);
	ReplyToCommand(client, "[INFO] Has Dispenser: %s", hasDispenser ? "Yes" : "No");

	bool hasTele = RCBot2_TF2_HasBuilding(bot, TFObject_Teleporter);
	ReplyToCommand(client, "[INFO] Has Teleporter: %s", hasTele ? "Yes" : "No");

	// Test getting building entities
	int sentryEnt = RCBot2_TF2_GetBuilding(bot, TFObject_Sentry);
	if (sentryEnt != -1) {
		ReplyToCommand(client, "[PASS] Found Sentry entity: %d", sentryEnt);
	} else {
		ReplyToCommand(client, "[INFO] No Sentry entity found");
	}

	// Test destruction
	if (RCBot2_TF2_EngineerBuild(bot, TFObject_Sentry, TFEngiCmd_Destroy)) {
		ReplyToCommand(client, "[PASS] Commanded bot to destroy Sentry");
	} else {
		ReplyToCommand(client, "[INFO] Could not command Sentry destruction");
	}

	ReplyToCommand(client, "===== Engineer Tests Complete =====\n");
	return Plugin_Handled;
}

public Action CMD_TestMedic(int client, int args) {
	ReplyToCommand(client, "\n===== TF2 Medic Tests =====");

	int bot = FindRCBot();
	if (bot == -1) {
		ReplyToCommand(client, "[FAIL] No RCBot found");
		return Plugin_Handled;
	}

	// Set bot to Medic
	if (!RCBot2_TF2_SetClass(bot, TFClass_Medic)) {
		ReplyToCommand(client, "[FAIL] Could not set bot to Medic");
		return Plugin_Handled;
	}

	ReplyToCommand(client, "[INFO] Bot set to Medic");

	// Test healing target management
	int healTarget = RCBot2_TF2_GetHealTarget(bot);
	if (healTarget == -1) {
		ReplyToCommand(client, "[INFO] Bot is not healing anyone");
	} else {
		ReplyToCommand(client, "[INFO] Bot healing target: %d", healTarget);
	}

	// Find a valid heal target
	int targetPlayer = FindValidHealTarget(bot);
	if (targetPlayer != -1) {
		if (RCBot2_TF2_SetHealTarget(bot, targetPlayer)) {
			ReplyToCommand(client, "[PASS] Set heal target to client #%d", targetPlayer);

			// Verify
			int verifyTarget = RCBot2_TF2_GetHealTarget(bot);
			if (verifyTarget == targetPlayer) {
				ReplyToCommand(client, "[PASS] Heal target verified");
			} else {
				ReplyToCommand(client, "[WARN] Heal target mismatch (got %d, expected %d)", verifyTarget, targetPlayer);
			}

			// Clear heal target
			if (RCBot2_TF2_SetHealTarget(bot, -1)) {
				ReplyToCommand(client, "[PASS] Cleared heal target");
			}
		} else {
			ReplyToCommand(client, "[FAIL] Could not set heal target");
		}
	} else {
		ReplyToCommand(client, "[SKIP] No valid heal target found");
	}

	// Test ubercharge query
	float uber = RCBot2_TF2_GetUberCharge(bot);
	if (uber >= 0.0) {
		ReplyToCommand(client, "[PASS] Ubercharge: %.1f%%", uber);
	} else {
		ReplyToCommand(client, "[INFO] Could not get ubercharge (not implemented or bot not in game)");
	}

	ReplyToCommand(client, "===== Medic Tests Complete =====\n");
	return Plugin_Handled;
}

public Action CMD_TestSpy(int client, int args) {
	ReplyToCommand(client, "\n===== TF2 Spy Tests =====");

	int bot = FindRCBot();
	if (bot == -1) {
		ReplyToCommand(client, "[FAIL] No RCBot found");
		return Plugin_Handled;
	}

	// Set bot to Spy
	if (!RCBot2_TF2_SetClass(bot, TFClass_Spy)) {
		ReplyToCommand(client, "[FAIL] Could not set bot to Spy");
		return Plugin_Handled;
	}

	ReplyToCommand(client, "[INFO] Bot set to Spy");

	// Test disguise
	if (RCBot2_TF2_Disguise(bot, 2, TFClass_Scout)) {  // Team RED (2), disguise as Scout
		ReplyToCommand(client, "[PASS] Commanded bot to disguise as RED Scout");
	} else {
		ReplyToCommand(client, "[FAIL] Could not command disguise");
	}

	if (RCBot2_TF2_Disguise(bot, 3, TFClass_Soldier)) {  // Team BLU (3), disguise as Soldier
		ReplyToCommand(client, "[PASS] Commanded bot to disguise as BLU Soldier");
	} else {
		ReplyToCommand(client, "[FAIL] Could not command disguise");
	}

	// Test disguise status
	bool isDisguised = RCBot2_TF2_IsDisguised(bot);
	ReplyToCommand(client, "[INFO] Bot disguised: %s", isDisguised ? "Yes" : "No");

	// Test cloak status
	bool isCloaked = RCBot2_TF2_IsCloaked(bot);
	ReplyToCommand(client, "[INFO] Bot cloaked: %s", isCloaked ? "Yes" : "No");

	// Test cloak timer reset
	if (RCBot2_TF2_ResetCloakTime(bot)) {
		ReplyToCommand(client, "[PASS] Reset cloak timer");
	} else {
		ReplyToCommand(client, "[INFO] Could not reset cloak timer");
	}

	ReplyToCommand(client, "===== Spy Tests Complete =====\n");
	return Plugin_Handled;
}

public Action CMD_TestGeneral(int client, int args) {
	ReplyToCommand(client, "\n===== TF2 General Tests =====");

	int bot = FindRCBot();
	if (bot == -1) {
		ReplyToCommand(client, "[FAIL] No RCBot found");
		return Plugin_Handled;
	}

	// Test call medic
	if (RCBot2_TF2_CallMedic(bot)) {
		ReplyToCommand(client, "[PASS] Bot called for Medic");
	} else {
		ReplyToCommand(client, "[FAIL] Could not call Medic");
	}

	// Test taunt (normal)
	if (RCBot2_TF2_Taunt(bot, false)) {
		ReplyToCommand(client, "[PASS] Bot taunted (normal)");
	} else {
		ReplyToCommand(client, "[INFO] Bot could not taunt (conditions not met)");
	}

	// Test taunt (forced)
	if (RCBot2_TF2_Taunt(bot, true)) {
		ReplyToCommand(client, "[PASS] Bot taunted (forced)");
	} else {
		ReplyToCommand(client, "[FAIL] Could not force taunt");
	}

	ReplyToCommand(client, "===== General Tests Complete =====\n");
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

int FindValidHealTarget(int bot) {
	for (int i = 1; i <= MaxClients; i++) {
		if (i == bot) continue;
		if (!IsClientInGame(i)) continue;
		if (!IsPlayerAlive(i)) continue;

		// Found a valid target
		return i;
	}
	return -1;
}

const char[] GetClassName(TFClassType class) {
	switch (class) {
		case TFClass_Unknown: return "Unknown";
		case TFClass_Scout: return "Scout";
		case TFClass_Sniper: return "Sniper";
		case TFClass_Soldier: return "Soldier";
		case TFClass_Demoman: return "Demoman";
		case TFClass_Medic: return "Medic";
		case TFClass_Heavy: return "Heavy";
		case TFClass_Pyro: return "Pyro";
		case TFClass_Spy: return "Spy";
		case TFClass_Engineer: return "Engineer";
		default: return "Invalid";
	}
}
