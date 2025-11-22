/**
 * RCBot2 HL2DM Features Demonstration Plugin
 *
 * Demonstrates the comprehensive HL2DM SourceMod integration including:
 * - Gravity Gun mechanics and physics manipulation
 * - Sprint and suit power management
 * - Equipment interaction (chargers, batteries, health kits)
 * - Weapon and ammo tracking
 * - Grenade throwing
 * - HL2DM-specific event forwards
 */

#include <sourcemod>
#include <rcbot2>
#include <rcbot2_hldm>

#pragma semicolon 1
#pragma newdecls required

// Plugin info
public Plugin myinfo = {
	name = "RCBot2 HL2DM Features Demo",
	author = "RCBot2 Team",
	description = "Demonstrates HL2DM-specific bot features and event forwards",
	version = "1.0.0",
	url = "https://github.com/APGRoboCop/rcbot2"
};

// Statistics tracking
int g_iGravGunPickups[MAXPLAYERS + 1];
int g_iGravGunLaunches[MAXPLAYERS + 1];
int g_iWeaponPickups[MAXPLAYERS + 1];
int g_iChargerUses[MAXPLAYERS + 1];

public void OnPluginStart() {
	// Register admin commands
	RegAdminCmd("sm_rcbot_hldm_stats", Cmd_HLDMStats, ADMFLAG_ROOT, "Show HL2DM bot statistics");
	RegAdminCmd("sm_rcbot_hldm_gravity", Cmd_TestGravityGun, ADMFLAG_ROOT, "Test Gravity Gun features");
	RegAdminCmd("sm_rcbot_hldm_sprint", Cmd_TestSprint, ADMFLAG_ROOT, "Test sprint features");
	RegAdminCmd("sm_rcbot_hldm_info", Cmd_BotInfo, ADMFLAG_ROOT, "Show detailed bot information");

	PrintToServer("[RCBot2 HLDM] Plugin loaded - Enhanced HL2DM bot features active");
}

//=============================================================================
// Event Handlers - Core Bot Events
//=============================================================================

public void RCBot2_OnBotSpawn(int client) {
	if (!IsClientInGame(client) || !IsRCBot2Client(client)) return;

	// Reset statistics for new life
	g_iGravGunPickups[client] = 0;
	g_iGravGunLaunches[client] = 0;

	// Check if bot has Gravity Gun
	if (RCBot2_HLDM_HasGravityGun(client)) {
		PrintToChatAll("[RCBot2 HLDM] Bot %N spawned with Gravity Gun!", client);
	}
}

public void RCBot2_OnBotDeath(int client, int attacker) {
	if (!IsClientInGame(client) || !IsRCBot2Client(client)) return;

	// Report statistics for this life
	if (g_iGravGunPickups[client] > 0 || g_iGravGunLaunches[client] > 0) {
		PrintToChatAll("[RCBot2 HLDM] %N stats - Grav Gun pickups: %d, launches: %d",
			client, g_iGravGunPickups[client], g_iGravGunLaunches[client]);
	}
}

//=============================================================================
// Event Handlers - HL2DM Events
//=============================================================================

public void RCBot2_HLDM_OnBotWeaponPickup(int client, int weapon) {
	g_iWeaponPickups[client]++;

	char weaponClass[64];
	if (weapon > 0 && IsValidEntity(weapon)) {
		GetEntityClassname(weapon, weaponClass, sizeof(weaponClass));
		PrintToChatAll("[RCBot2 HLDM] Bot %N picked up weapon: %s", client, weaponClass);
	}
}

public void RCBot2_HLDM_OnBotGravityGunPickup(int client, int object) {
	g_iGravGunPickups[client]++;

	char objectClass[64];
	if (object > 0 && IsValidEntity(object)) {
		GetEntityClassname(object, objectClass, sizeof(objectClass));
		PrintToChatAll("[RCBot2 HLDM] Bot %N picked up object with Gravity Gun: %s (total: %d)",
			client, objectClass, g_iGravGunPickups[client]);
	}
}

public void RCBot2_HLDM_OnBotGravityGunLaunch(int client, int object) {
	g_iGravGunLaunches[client]++;

	char objectClass[64];
	if (object > 0 && IsValidEntity(object)) {
		GetEntityClassname(object, objectClass, sizeof(objectClass));
		PrintToChatAll("[RCBot2 HLDM] Bot %N launched object: %s (total launches: %d)",
			client, objectClass, g_iGravGunLaunches[client]);
	}
}

public void RCBot2_HLDM_OnBotGravityGunDrop(int client, int object) {
	char objectClass[64];
	if (object > 0 && IsValidEntity(object)) {
		GetEntityClassname(object, objectClass, sizeof(objectClass));
		PrintToChatAll("[RCBot2 HLDM] Bot %N dropped object: %s", client, objectClass);
	}
}

public void RCBot2_HLDM_OnBotSuitChargeUsed(int client, int chargerType, int chargerEntity) {
	g_iChargerUses[client]++;

	char chargerName[32];
	if (chargerType == 0) {
		chargerName = "Health Charger";
	} else {
		chargerName = "Suit Charger";
	}

	PrintToChatAll("[RCBot2 HLDM] Bot %N used %s (total uses: %d)",
		client, chargerName, g_iChargerUses[client]);
}

//=============================================================================
// Commands
//=============================================================================

public Action Cmd_HLDMStats(int client, int args) {
	ReplyToCommand(client, "[RCBot2 HLDM] === Bot Statistics ===");

	for (int i = 1; i <= MaxClients; i++) {
		if (IsClientInGame(i) && IsRCBot2Client(i)) {
			float armor = RCBot2_HLDM_GetArmorPercent(i);
			bool sprinting = RCBot2_HLDM_IsSprinting(i);
			bool carrying = RCBot2_HLDM_IsCarryingObject(i);
			bool canGrenade = RCBot2_HLDM_CanThrowGrenade(i);

			ReplyToCommand(client, "%N: Armor: %.0f%% | Sprint: %s | Carrying: %s | Has Grenades: %s",
				i, armor * 100.0, sprinting ? "Yes" : "No",
				carrying ? "Yes" : "No", canGrenade ? "Yes" : "No");

			ReplyToCommand(client, "  Stats - Pickups: %d | Launches: %d | Weapons: %d | Chargers: %d",
				g_iGravGunPickups[i], g_iGravGunLaunches[i],
				g_iWeaponPickups[i], g_iChargerUses[i]);
		}
	}

	return Plugin_Handled;
}

public Action Cmd_TestGravityGun(int client, int args) {
	if (args < 1) {
		ReplyToCommand(client, "Usage: sm_rcbot_hldm_gravity <bot_name>");
		return Plugin_Handled;
	}

	char targetName[64];
	GetCmdArg(1, targetName, sizeof(targetName));

	int target = FindBotByName(targetName);
	if (target == -1) {
		ReplyToCommand(client, "[RCBot2 HLDM] Bot not found: %s", targetName);
		return Plugin_Handled;
	}

	if (!RCBot2_HLDM_HasGravityGun(target)) {
		ReplyToCommand(client, "[RCBot2 HLDM] Bot %N does not have a Gravity Gun", target);
		return Plugin_Handled;
	}

	// Check current state
	if (RCBot2_HLDM_IsCarryingObject(target)) {
		int object = RCBot2_HLDM_GetCarriedObject(target);
		ReplyToCommand(client, "[RCBot2 HLDM] Bot is carrying object: %d", object);

		// Launch it
		if (RCBot2_HLDM_LaunchObject(target)) {
			ReplyToCommand(client, "[RCBot2 HLDM] Commanded bot to launch object");
		}
	} else {
		// Try to pick up nearest physics object
		int nearestObj = RCBot2_HLDM_GetNearestPhysicsObject(target);
		if (nearestObj > 0) {
			if (RCBot2_HLDM_PickupObject(target, nearestObj)) {
				ReplyToCommand(client, "[RCBot2 HLDM] Commanded bot to pick up object: %d", nearestObj);
			}
		} else {
			ReplyToCommand(client, "[RCBot2 HLDM] No physics objects nearby");
		}
	}

	return Plugin_Handled;
}

public Action Cmd_TestSprint(int client, int args) {
	if (args < 1) {
		ReplyToCommand(client, "Usage: sm_rcbot_hldm_sprint <bot_name> <0|1>");
		return Plugin_Handled;
	}

	char targetName[64], enableStr[4];
	GetCmdArg(1, targetName, sizeof(targetName));
	GetCmdArg(2, enableStr, sizeof(enableStr));

	int target = FindBotByName(targetName);
	if (target == -1) {
		ReplyToCommand(client, "[RCBot2 HLDM] Bot not found: %s", targetName);
		return Plugin_Handled;
	}

	bool enable = StringToInt(enableStr) != 0;

	if (RCBot2_HLDM_UseSprint(target, enable)) {
		float sprintTime = RCBot2_HLDM_GetSprintTime(target);
		ReplyToCommand(client, "[RCBot2 HLDM] Sprint %s for %N (cooldown: %.1fs)",
			enable ? "enabled" : "disabled", target, sprintTime);
	}

	return Plugin_Handled;
}

public Action Cmd_BotInfo(int client, int args) {
	if (args < 1) {
		ReplyToCommand(client, "Usage: sm_rcbot_hldm_info <bot_name>");
		return Plugin_Handled;
	}

	char targetName[64];
	GetCmdArg(1, targetName, sizeof(targetName));

	int target = FindBotByName(targetName);
	if (target == -1) {
		ReplyToCommand(client, "[RCBot2 HLDM] Bot not found: %s", targetName);
		return Plugin_Handled;
	}

	ReplyToCommand(client, "[RCBot2 HLDM] === Bot Information for %N ===", target);

	// Sprint info
	bool sprinting = RCBot2_HLDM_IsSprinting(target);
	float sprintTime = RCBot2_HLDM_GetSprintTime(target);
	ReplyToCommand(client, "Sprint: %s (cooldown: %.1fs)", sprinting ? "Active" : "Inactive", sprintTime);

	// Armor info
	float armor = RCBot2_HLDM_GetArmorPercent(target);
	ReplyToCommand(client, "Armor: %.0f%%", armor * 100.0);

	// Weapon info
	int currentWeapon = RCBot2_HLDM_GetCurrentWeapon(target);
	int clip1 = RCBot2_HLDM_GetWeaponClip1(target);
	int clip2 = RCBot2_HLDM_GetWeaponClip2(target);
	ReplyToCommand(client, "Weapon: %d | Clip1: %d | Clip2: %d", currentWeapon, clip1, clip2);

	// Gravity Gun info
	bool hasGravGun = RCBot2_HLDM_HasGravityGun(target);
	bool carrying = RCBot2_HLDM_IsCarryingObject(target);
	int carriedObj = RCBot2_HLDM_GetCarriedObject(target);
	ReplyToCommand(client, "Gravity Gun: %s | Carrying: %s (object: %d)",
		hasGravGun ? "Yes" : "No", carrying ? "Yes" : "No", carriedObj);

	// Nearby items
	int nearestWeapon = RCBot2_HLDM_GetNearestWeapon(target);
	int nearestHealth = RCBot2_HLDM_GetNearestHealthKit(target);
	int nearestBattery = RCBot2_HLDM_GetNearestBattery(target);
	int nearestAmmo = RCBot2_HLDM_GetNearestAmmoCrate(target);
	ReplyToCommand(client, "Nearby - Weapon: %d | Health: %d | Battery: %d | Ammo: %d",
		nearestWeapon, nearestHealth, nearestBattery, nearestAmmo);

	// Physics objects
	int nearestPhys = RCBot2_HLDM_GetNearestPhysicsObject(target);
	int nearestBreak = RCBot2_HLDM_GetNearestBreakable(target);
	int failedObj = RCBot2_HLDM_GetFailedObject(target);
	ReplyToCommand(client, "Physics - Nearest: %d | Breakable: %d | Failed: %d",
		nearestPhys, nearestBreak, failedObj);

	// Combat
	bool canGrenade = RCBot2_HLDM_CanThrowGrenade(target);
	ReplyToCommand(client, "Can throw grenade: %s", canGrenade ? "Yes" : "No");

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
