/**
 * Phase 5 Squad & Team Coordination Test Plugin
 * Tests RCBot2 squad management natives
 */
#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>
#include <rcbot2>

public Plugin myinfo = {
	name = "RCBot2 Phase 5 Test",
	author = "RCBot2 Development Team",
	description = "Test plugin for Phase 5 squad and team coordination natives",
	version = "5.0.0",
	url = "https://github.com/APGRoboCop/rcbot2"
};

public void OnPluginStart() {
	RegAdminCmd("sm_rcbot_test_phase5", CMD_TestPhase5, ADMFLAG_ROOT, "Run all Phase 5 squad tests");
	RegAdminCmd("sm_rcbot_test_squad_create", CMD_TestSquadCreate, ADMFLAG_ROOT, "Test squad creation");
	RegAdminCmd("sm_rcbot_test_squad_manage", CMD_TestSquadManage, ADMFLAG_ROOT, "Test squad management");
	RegAdminCmd("sm_rcbot_test_squad_query", CMD_TestSquadQuery, ADMFLAG_ROOT, "Test squad queries");
}

public Action CMD_TestPhase5(int client, int args) {
	ReplyToCommand(client, "\n===== Phase 5 Squad Tests =====");

	CMD_TestSquadCreate(client, 0);
	CMD_TestSquadManage(client, 0);
	CMD_TestSquadQuery(client, 0);

	ReplyToCommand(client, "===== All Phase 5 Tests Complete =====\n");
	return Plugin_Handled;
}

public Action CMD_TestSquadCreate(int client, int args) {
	ReplyToCommand(client, "\n===== Squad Creation Tests =====");

	// Create two test bots
	int bot1 = RCBot2_CreateBot("SquadLeader");
	if (bot1 == -1) {
		ReplyToCommand(client, "[FAIL] Could not create bot1");
		return Plugin_Handled;
	}
	ReplyToCommand(client, "[PASS] Created bot1 #%d (SquadLeader)", bot1);

	int bot2 = RCBot2_CreateBot("SquadMember");
	if (bot2 == -1) {
		ReplyToCommand(client, "[FAIL] Could not create bot2");
		return Plugin_Handled;
	}
	ReplyToCommand(client, "[PASS] Created bot2 #%d (SquadMember)", bot2);

	// Test RCBot2_CreateSquad
	int squadId = RCBot2_CreateSquad(bot1);
	if (squadId != -1) {
		ReplyToCommand(client, "[PASS] Created squad with leader #%d (squad ID: %d)", bot1, squadId);
	} else {
		ReplyToCommand(client, "[FAIL] Could not create squad");
		return Plugin_Handled;
	}

	// Verify leader is in squad
	if (RCBot2_IsInSquad(bot1)) {
		ReplyToCommand(client, "[PASS] Leader bot is in squad");
	} else {
		ReplyToCommand(client, "[FAIL] Leader bot is not in squad");
	}

	// Test adding member to squad
	if (RCBot2_AddBotToSquad(bot2, bot1)) {
		ReplyToCommand(client, "[PASS] Added bot #%d to squad", bot2);
	} else {
		ReplyToCommand(client, "[FAIL] Could not add bot to squad");
	}

	// Verify member is in squad
	if (RCBot2_IsInSquad(bot2)) {
		ReplyToCommand(client, "[PASS] Member bot is in squad");
	} else {
		ReplyToCommand(client, "[FAIL] Member bot is not in squad");
	}

	ReplyToCommand(client, "===== Squad Creation Tests Complete =====\n");
	return Plugin_Handled;
}

public Action CMD_TestSquadManage(int client, int args) {
	ReplyToCommand(client, "\n===== Squad Management Tests =====");

	// Find existing bots
	int leader = FindBotByName("SquadLeader");
	int member = FindBotByName("SquadMember");

	if (leader == -1 || member == -1) {
		ReplyToCommand(client, "[SKIP] No test bots found - run sm_rcbot_test_squad_create first");
		return Plugin_Handled;
	}

	// Test getting squad leader
	int foundLeader = RCBot2_GetBotSquadLeader(member);
	if (foundLeader == leader) {
		ReplyToCommand(client, "[PASS] Got correct squad leader: #%d", foundLeader);
	} else {
		ReplyToCommand(client, "[WARN] Squad leader mismatch (expected %d, got %d)", leader, foundLeader);
	}

	// Test removing member from squad
	if (RCBot2_RemoveBotFromSquad(member)) {
		ReplyToCommand(client, "[PASS] Removed bot #%d from squad", member);

		// Verify removal
		if (!RCBot2_IsInSquad(member)) {
			ReplyToCommand(client, "[PASS] Verified bot is no longer in squad");
		} else {
			ReplyToCommand(client, "[FAIL] Bot still reports being in squad");
		}
	} else {
		ReplyToCommand(client, "[FAIL] Could not remove bot from squad");
	}

	// Test re-adding member
	if (RCBot2_AddBotToSquad(member, leader)) {
		ReplyToCommand(client, "[PASS] Re-added bot #%d to squad", member);
	} else {
		ReplyToCommand(client, "[FAIL] Could not re-add bot to squad");
	}

	ReplyToCommand(client, "===== Squad Management Tests Complete =====\n");
	return Plugin_Handled;
}

public Action CMD_TestSquadQuery(int client, int args) {
	ReplyToCommand(client, "\n===== Squad Query Tests =====");

	int leader = FindBotByName("SquadLeader");
	if (leader == -1) {
		ReplyToCommand(client, "[SKIP] No squad leader found");
		return Plugin_Handled;
	}

	// Test RCBot2_GetSquadMemberCount
	int memberCount = RCBot2_GetSquadMemberCount(leader);
	ReplyToCommand(client, "[INFO] Squad has %d members", memberCount);

	if (memberCount > 0) {
		// Test RCBot2_GetSquadMembers
		int[] members = new int[10];
		int count = RCBot2_GetSquadMembers(leader, members, 10);

		if (count > 0) {
			ReplyToCommand(client, "[PASS] Retrieved %d squad members:", count);
			for (int i = 0; i < count; i++) {
				char name[64];
				if (IsClientInGame(members[i])) {
					GetClientName(members[i], name, sizeof(name));
					ReplyToCommand(client, "  Member %d: #%d (%s)", i + 1, members[i], name);
				}
			}
		} else {
			ReplyToCommand(client, "[WARN] Could not retrieve squad members");
		}
	}

	// Test destroying squad
	ReplyToCommand(client, "\n[INFO] Testing squad destruction...");
	if (RCBot2_DestroySquad(leader)) {
		ReplyToCommand(client, "[PASS] Destroyed squad");

		// Verify destruction
		if (!RCBot2_IsInSquad(leader)) {
			ReplyToCommand(client, "[PASS] Leader no longer in squad");
		} else {
			ReplyToCommand(client, "[WARN] Leader still reports being in squad");
		}

		// Check member count after destruction
		int newCount = RCBot2_GetSquadMemberCount(leader);
		if (newCount == 0) {
			ReplyToCommand(client, "[PASS] Squad member count is now 0");
		} else {
			ReplyToCommand(client, "[WARN] Squad still has %d members", newCount);
		}
	} else {
		ReplyToCommand(client, "[FAIL] Could not destroy squad");
	}

	ReplyToCommand(client, "===== Squad Query Tests Complete =====\n");
	return Plugin_Handled;
}

// Helper functions

int FindBotByName(const char[] name) {
	char botName[64];
	for (int i = 1; i <= MaxClients; i++) {
		if (IsClientInGame(i) && IsRCBot2Client(i)) {
			GetClientName(i, botName, sizeof(botName));
			if (StrEqual(botName, name, false)) {
				return i;
			}
		}
	}
	return -1;
}
