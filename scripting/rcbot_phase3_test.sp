/**
 * Phase 3 Navigation & Pathfinding Test Plugin
 * Tests waypoint and navigation natives for RCBot2
 */
#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>
#include <rcbot2>

public Plugin myinfo = {
	name = "RCBot2 Phase 3 Test",
	author = "RCBot2 Development Team",
	description = "Test plugin for Phase 3 navigation and pathfinding natives",
	version = "3.0.0",
	url = "https://github.com/APGRoboCop/rcbot2"
};

public void OnPluginStart() {
	RegAdminCmd("sm_rcbot_test_phase3_all", CMD_TestAllPhase3, ADMFLAG_ROOT, "Run all Phase 3 tests");
	RegAdminCmd("sm_rcbot_test_waypoints", CMD_TestWaypoints, ADMFLAG_ROOT, "Test waypoint query functions");
	RegAdminCmd("sm_rcbot_test_navigation", CMD_TestNavigation, ADMFLAG_ROOT, "Test bot navigation functions");
	RegAdminCmd("sm_rcbot_test_pathfinding", CMD_TestPathfinding, ADMFLAG_ROOT, "Test pathfinding functions");
}

public Action CMD_TestAllPhase3(int client, int args) {
	ReplyToCommand(client, "[RCBot Phase 3] Running all Phase 3 tests...");

	CMD_TestWaypoints(client, 0);
	CMD_TestNavigation(client, 0);
	CMD_TestPathfinding(client, 0);

	ReplyToCommand(client, "[RCBot Phase 3] All tests completed!");
	return Plugin_Handled;
}

public Action CMD_TestWaypoints(int client, int args) {
	ReplyToCommand(client, "\n===== Waypoint Query Tests =====");

	// Test RCBot2_GetWaypointCount
	int waypointCount = RCBot2_GetWaypointCount();
	if (waypointCount > 0) {
		ReplyToCommand(client, "[PASS] Total waypoints loaded: %d", waypointCount);
	} else {
		ReplyToCommand(client, "[INFO] No waypoints loaded for this map");
		ReplyToCommand(client, "===== Waypoint Tests Complete =====\n");
		return Plugin_Handled;
	}

	// Test RCBot2_GetNearestWaypoint with client position
	if (IsClientInGame(client)) {
		float vOrigin[3];
		GetClientAbsOrigin(client, vOrigin);

		// Test with unlimited distance
		int nearestWpt = RCBot2_GetNearestWaypoint(vOrigin, 0.0);
		if (nearestWpt != -1) {
			ReplyToCommand(client, "[PASS] Nearest waypoint to you: #%d", nearestWpt);

			// Test RCBot2_GetWaypointOrigin
			float wptOrigin[3];
			if (RCBot2_GetWaypointOrigin(nearestWpt, wptOrigin)) {
				float distance = GetVectorDistance(vOrigin, wptOrigin);
				ReplyToCommand(client, "[PASS] Waypoint #%d origin: (%.1f, %.1f, %.1f) - Distance: %.1f units",
					nearestWpt, wptOrigin[0], wptOrigin[1], wptOrigin[2], distance);
			} else {
				ReplyToCommand(client, "[FAIL] Could not get waypoint #%d origin", nearestWpt);
			}

			// Test RCBot2_GetWaypointFlags
			int flags = RCBot2_GetWaypointFlags(nearestWpt);
			ReplyToCommand(client, "[PASS] Waypoint #%d flags: 0x%X", nearestWpt, flags);
			PrintWaypointFlags(client, flags);
		} else {
			ReplyToCommand(client, "[INFO] No nearest waypoint found");
		}

		// Test with limited distance (200 units)
		int nearWpt200 = RCBot2_GetNearestWaypoint(vOrigin, 200.0);
		if (nearWpt200 != -1) {
			ReplyToCommand(client, "[PASS] Nearest waypoint within 200 units: #%d", nearWpt200);
		} else {
			ReplyToCommand(client, "[INFO] No waypoint within 200 units");
		}
	}

	// Test invalid waypoint index
	float invalidOrigin[3];
	if (!RCBot2_GetWaypointOrigin(99999, invalidOrigin)) {
		ReplyToCommand(client, "[PASS] Correctly rejected invalid waypoint index");
	} else {
		ReplyToCommand(client, "[FAIL] Should have rejected invalid waypoint index");
	}

	ReplyToCommand(client, "===== Waypoint Tests Complete =====\n");
	return Plugin_Handled;
}

public Action CMD_TestNavigation(int client, int args) {
	ReplyToCommand(client, "\n===== Bot Navigation Tests =====");

	int bot = FindRCBot();
	if (bot == -1) {
		bot = RCBot2_CreateBot("NavTestBot");
		if (bot == -1) {
			ReplyToCommand(client, "[FAIL] Could not create RCBot");
			return Plugin_Handled;
		}
		ReplyToCommand(client, "[PASS] Created RCBot #%d", bot);
	} else {
		ReplyToCommand(client, "[INFO] Using existing RCBot #%d", bot);
	}

	// Test RCBot2_HasPath
	bool hasPath = RCBot2_HasPath(bot);
	ReplyToCommand(client, "[INFO] Bot has path: %s", hasPath ? "Yes" : "No");

	if (hasPath) {
		// Test RCBot2_GetGoalOrigin
		float goalOrigin[3];
		if (RCBot2_GetGoalOrigin(bot, goalOrigin)) {
			ReplyToCommand(client, "[PASS] Bot goal origin: (%.1f, %.1f, %.1f)",
				goalOrigin[0], goalOrigin[1], goalOrigin[2]);
		} else {
			ReplyToCommand(client, "[INFO] Bot has no goal origin");
		}

		// Test RCBot2_GetCurrentWaypointID
		int currentWpt = RCBot2_GetCurrentWaypointID(bot);
		if (currentWpt != -1) {
			ReplyToCommand(client, "[PASS] Bot current waypoint: #%d", currentWpt);
		} else {
			ReplyToCommand(client, "[INFO] Bot has no current waypoint");
		}

		// Test RCBot2_GetGoalWaypointID
		int goalWpt = RCBot2_GetGoalWaypointID(bot);
		if (goalWpt != -1) {
			ReplyToCommand(client, "[PASS] Bot goal waypoint: #%d", goalWpt);
		} else {
			ReplyToCommand(client, "[INFO] Bot has no goal waypoint");
		}
	} else {
		ReplyToCommand(client, "[INFO] Bot has no active path to test");
	}

	// Test RCBot2_IsStuck
	bool isStuck = RCBot2_IsStuck(bot);
	ReplyToCommand(client, "[INFO] Bot stuck status: %s", isStuck ? "Stuck" : "Not stuck");

	ReplyToCommand(client, "===== Navigation Tests Complete =====\n");
	return Plugin_Handled;
}

public Action CMD_TestPathfinding(int client, int args) {
	ReplyToCommand(client, "\n===== Pathfinding Tests =====");

	int bot = FindRCBot();
	if (bot == -1) {
		ReplyToCommand(client, "[FAIL] No RCBot found");
		return Plugin_Handled;
	}

	if (!IsClientInGame(bot)) {
		ReplyToCommand(client, "[FAIL] Bot is not in game");
		return Plugin_Handled;
	}

	// Get bot's current state
	bool hadPath = RCBot2_HasPath(bot);
	ReplyToCommand(client, "[INFO] Bot initial path state: %s", hadPath ? "Has path" : "No path");

	// Test RCBot2_ClearPath
	if (RCBot2_ClearPath(bot)) {
		ReplyToCommand(client, "[PASS] Cleared bot's path");

		// Verify path was cleared
		if (!RCBot2_HasPath(bot)) {
			ReplyToCommand(client, "[PASS] Verified path cleared successfully");
		} else {
			ReplyToCommand(client, "[WARN] Bot still reports having a path");
		}
	} else {
		ReplyToCommand(client, "[INFO] Could not clear path");
	}

	// Test giving bot a destination
	float botOrigin[3];
	GetClientAbsOrigin(bot, botOrigin);

	int nearestWpt = RCBot2_GetNearestWaypoint(botOrigin, 500.0);
	if (nearestWpt != -1) {
		ReplyToCommand(client, "[INFO] Found waypoint #%d near bot", nearestWpt);

		// Try to navigate to a waypoint 5 indices away (simple test)
		int targetWpt = nearestWpt + 5;
		int waypointCount = RCBot2_GetWaypointCount();

		if (targetWpt < waypointCount) {
			float targetOrigin[3];
			if (RCBot2_GetWaypointOrigin(targetWpt, targetOrigin)) {
				ReplyToCommand(client, "[INFO] Target waypoint #%d at (%.1f, %.1f, %.1f)",
					targetWpt, targetOrigin[0], targetOrigin[1], targetOrigin[2]);

				// Command bot to move there
				if (RCBot2_GotoOrigin(bot, targetOrigin)) {
					ReplyToCommand(client, "[PASS] Commanded bot to navigate to waypoint #%d", targetWpt);

					// Give it a moment, then check if path was created
					CreateTimer(0.5, Timer_CheckPathCreated, GetClientUserId(bot));
				} else {
					ReplyToCommand(client, "[INFO] Could not command bot navigation");
				}
			}
		}
	}

	ReplyToCommand(client, "===== Pathfinding Tests Complete =====\n");
	return Plugin_Handled;
}

public Action Timer_CheckPathCreated(Handle timer, int userid) {
	int bot = GetClientOfUserId(userid);
	if (bot == 0 || !IsClientInGame(bot)) {
		return Plugin_Stop;
	}

	if (RCBot2_HasPath(bot)) {
		PrintToServer("[PASS] Bot successfully created navigation path");

		int currentWpt = RCBot2_GetCurrentWaypointID(bot);
		int goalWpt = RCBot2_GetGoalWaypointID(bot);

		PrintToServer("[INFO] Current waypoint: #%d, Goal waypoint: #%d", currentWpt, goalWpt);
	} else {
		PrintToServer("[INFO] Bot has not created a path yet");
	}

	return Plugin_Stop;
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

void PrintWaypointFlags(int client, int flags) {
	if (flags == 0) {
		ReplyToCommand(client, "  Flags: None");
		return;
	}

	char flagsStr[256];
	flagsStr[0] = '\0';

	// Common waypoint flags (from bot_waypoint.h)
	if (flags & (1 << 0)) StrCat(flagsStr, sizeof(flagsStr), "JUMP ");
	if (flags & (1 << 1)) StrCat(flagsStr, sizeof(flagsStr), "CROUCH ");
	if (flags & (1 << 2)) StrCat(flagsStr, sizeof(flagsStr), "UNREACHABLE ");
	if (flags & (1 << 3)) StrCat(flagsStr, sizeof(flagsStr), "LADDER ");
	if (flags & (1 << 4)) StrCat(flagsStr, sizeof(flagsStr), "FLAG/RESCUE ");
	if (flags & (1 << 5)) StrCat(flagsStr, sizeof(flagsStr), "CAPPOINT/GOAL ");
	if (flags & (1 << 8)) StrCat(flagsStr, sizeof(flagsStr), "HEALTH ");
	if (flags & (1 << 9)) StrCat(flagsStr, sizeof(flagsStr), "OPENS_LATER ");
	if (flags & (1 << 11)) StrCat(flagsStr, sizeof(flagsStr), "SNIPER ");
	if (flags & (1 << 12)) StrCat(flagsStr, sizeof(flagsStr), "AMMO ");
	if (flags & (1 << 14)) StrCat(flagsStr, sizeof(flagsStr), "SENTRY ");
	if (flags & (1 << 16)) StrCat(flagsStr, sizeof(flagsStr), "TELE_ENTRANCE ");
	if (flags & (1 << 17)) StrCat(flagsStr, sizeof(flagsStr), "TELE_EXIT ");
	if (flags & (1 << 18)) StrCat(flagsStr, sizeof(flagsStr), "DEFEND ");

	if (strlen(flagsStr) > 0) {
		ReplyToCommand(client, "  Flags: %s", flagsStr);
	} else {
		ReplyToCommand(client, "  Flags: 0x%X (unknown flags)", flags);
	}
}
