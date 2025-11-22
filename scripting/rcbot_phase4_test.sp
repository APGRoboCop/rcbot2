/**
 * Phase 4 Event System Test Plugin
 * Tests RCBot2 event forwards and callbacks
 */
#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>
#include <rcbot2>

public Plugin myinfo = {
	name = "RCBot2 Phase 4 Test",
	author = "RCBot2 Development Team",
	description = "Test plugin for Phase 4 event system and callbacks",
	version = "4.0.0",
	url = "https://github.com/APGRoboCop/rcbot2"
};

// Event counters for testing
int g_iSpawnCount = 0;
int g_iDeathCount = 0;
int g_iKillCount = 0;
int g_iThinkCount = 0;
int g_iEnemyFoundCount = 0;
int g_iEnemyLostCount = 0;
int g_iDamagedCount = 0;
int g_iTaskChangeCount = 0;

// Tracking arrays
int g_iLastEnemy[MAXPLAYERS + 1] = {0, ...};
int g_iLastAttacker[MAXPLAYERS + 1] = {0, ...};
RCBotSchedule g_iCurrentTask[MAXPLAYERS + 1] = {RCBotSchedule_None, ...};

public void OnPluginStart() {
	RegAdminCmd("sm_rcbot_test_phase4", CMD_TestPhase4, ADMFLAG_ROOT, "Test Phase 4 event system");
	RegAdminCmd("sm_rcbot_test_events_stats", CMD_EventStats, ADMFLAG_ROOT, "Show event statistics");
	RegAdminCmd("sm_rcbot_test_events_reset", CMD_ResetStats, ADMFLAG_ROOT, "Reset event statistics");
}

public Action CMD_TestPhase4(int client, int args) {
	ReplyToCommand(client, "\n===== Phase 4 Event System Test =====");
	ReplyToCommand(client, "Event forwards are now active and listening.");
	ReplyToCommand(client, "Create or interact with RCBots to trigger events.");
	ReplyToCommand(client, "Use sm_rcbot_test_events_stats to view statistics.");

	// Create a test bot if none exist
	int bot = FindRCBot();
	if (bot == -1) {
		bot = RCBot2_CreateBot("EventTestBot");
		if (bot != -1) {
			ReplyToCommand(client, "[INFO] Created test bot #%d - Events will be logged", bot);
		} else {
			ReplyToCommand(client, "[WARN] Could not create test bot");
		}
	} else {
		ReplyToCommand(client, "[INFO] Using existing bot #%d for event testing", bot);
	}

	ReplyToCommand(client, "===== Event System Ready =====\n");
	return Plugin_Handled;
}

public Action CMD_EventStats(int client, int args) {
	ReplyToCommand(client, "\n===== Event Statistics =====");
	ReplyToCommand(client, "Bot Spawns:        %d", g_iSpawnCount);
	ReplyToCommand(client, "Bot Deaths:        %d", g_iDeathCount);
	ReplyToCommand(client, "Bot Kills:         %d", g_iKillCount);
	ReplyToCommand(client, "Bot Thinks:        %d", g_iThinkCount);
	ReplyToCommand(client, "Enemies Found:     %d", g_iEnemyFoundCount);
	ReplyToCommand(client, "Enemies Lost:      %d", g_iEnemyLostCount);
	ReplyToCommand(client, "Damage Events:     %d", g_iDamagedCount);
	ReplyToCommand(client, "Task Changes:      %d", g_iTaskChangeCount);
	ReplyToCommand(client, "=========================\n");

	// Show per-bot info
	for (int i = 1; i <= MaxClients; i++) {
		if (IsClientInGame(i) && IsRCBot2Client(i)) {
			RCBotSchedule currentTask = g_iCurrentTask[i];
			ReplyToCommand(client, "Bot #%d - Current Task: %s", i, GetScheduleName(currentTask));
			if (g_iLastEnemy[i] != 0) {
				ReplyToCommand(client, "  Last Enemy: #%d", g_iLastEnemy[i]);
			}
		}
	}

	return Plugin_Handled;
}

public Action CMD_ResetStats(int client, int args) {
	g_iSpawnCount = 0;
	g_iDeathCount = 0;
	g_iKillCount = 0;
	g_iThinkCount = 0;
	g_iEnemyFoundCount = 0;
	g_iEnemyLostCount = 0;
	g_iDamagedCount = 0;
	g_iTaskChangeCount = 0;

	ReplyToCommand(client, "[Event Stats] Statistics reset");
	return Plugin_Handled;
}

//=============================================================================
// Event Forward Handlers
//=============================================================================

public void RCBot2_OnBotSpawn(int client) {
	g_iSpawnCount++;
	PrintToServer("[Event] Bot #%d spawned (Total spawns: %d)", client, g_iSpawnCount);

	// Reset bot tracking
	g_iLastEnemy[client] = 0;
	g_iLastAttacker[client] = 0;
	g_iCurrentTask[client] = RCBotSchedule_None;
}

public void RCBot2_OnBotDeath(int client, int attacker) {
	g_iDeathCount++;

	if (attacker > 0 && attacker <= MaxClients && IsClientInGame(attacker)) {
		char attackerName[64];
		GetClientName(attacker, attackerName, sizeof(attackerName));
		PrintToServer("[Event] Bot #%d died - Killed by #%d (%s)", client, attacker, attackerName);
	} else {
		PrintToServer("[Event] Bot #%d died - World/Suicide", client);
	}

	g_iLastAttacker[client] = attacker;
}

public void RCBot2_OnBotKill(int client, int victim) {
	g_iKillCount++;

	if (victim > 0 && victim <= MaxClients && IsClientInGame(victim)) {
		char victimName[64];
		GetClientName(victim, victimName, sizeof(victimName));
		PrintToServer("[Event] Bot #%d killed #%d (%s)", client, victim, victimName);
	}
}

public void RCBot2_OnBotThink(int client) {
	g_iThinkCount++;

	// Only log every 100 thinks to avoid spam
	if (g_iThinkCount % 100 == 0) {
		PrintToServer("[Event] Bot think cycle: %d total thinks processed", g_iThinkCount);
	}
}

public void RCBot2_OnBotEnemyFound(int client, int enemy) {
	g_iEnemyFoundCount++;
	g_iLastEnemy[client] = enemy;

	if (enemy > 0 && enemy <= MaxClients && IsClientInGame(enemy)) {
		char enemyName[64];
		GetClientName(enemy, enemyName, sizeof(enemyName));
		PrintToServer("[Event] Bot #%d found enemy #%d (%s)", client, enemy, enemyName);
	}
}

public void RCBot2_OnBotEnemyLost(int client, int enemy) {
	g_iEnemyLostCount++;

	if (g_iLastEnemy[client] == enemy) {
		g_iLastEnemy[client] = 0;
	}

	if (enemy > 0 && enemy <= MaxClients && IsClientInGame(enemy)) {
		char enemyName[64];
		GetClientName(enemy, enemyName, sizeof(enemyName));
		PrintToServer("[Event] Bot #%d lost enemy #%d (%s)", client, enemy, enemyName);
	}
}

public void RCBot2_OnBotDamaged(int client, int attacker, int damage) {
	g_iDamagedCount++;

	if (attacker > 0 && attacker <= MaxClients && IsClientInGame(attacker)) {
		char attackerName[64];
		GetClientName(attacker, attackerName, sizeof(attackerName));
		PrintToServer("[Event] Bot #%d took %d damage from #%d (%s)", client, damage, attacker, attackerName);
	} else {
		PrintToServer("[Event] Bot #%d took %d damage from world", client, damage);
	}
}

public void RCBot2_OnBotTaskChange(int client, int oldTask, int newTask) {
	g_iTaskChangeCount++;
	g_iCurrentTask[client] = view_as<RCBotSchedule>(newTask);

	char oldTaskName[64], newTaskName[64];
	strcopy(oldTaskName, sizeof(oldTaskName), GetScheduleName(view_as<RCBotSchedule>(oldTask)));
	strcopy(newTaskName, sizeof(newTaskName), GetScheduleName(view_as<RCBotSchedule>(newTask)));

	PrintToServer("[Event] Bot #%d task changed: %s -> %s", client, oldTaskName, newTaskName);
}

//=============================================================================
// Helper Functions
//=============================================================================

int FindRCBot() {
	for (int i = 1; i <= MaxClients; i++) {
		if (IsClientInGame(i) && IsRCBot2Client(i)) {
			return i;
		}
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
		default: return "Other";
	}
}
