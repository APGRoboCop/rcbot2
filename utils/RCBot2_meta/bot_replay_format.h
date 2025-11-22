/*
 *    This file is part of RCBot.
 *
 *    RCBot by Paul Murphy adapted from Botman's HPB Bot 2 template.
 *
 *    RCBot is free software; you can redistribute it and/or modify it
 *    under the terms of the GNU General Public License as published by the
 *    Free Software Foundation; either version 2 of the License, or (at
 *    your option) any later version.
 *
 *    RCBot is distributed in the hope that it will be useful, but
 *    WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with RCBot; if not, write to the Free Software Foundation,
 *    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __BOT_REPLAY_FORMAT_H__
#define __BOT_REPLAY_FORMAT_H__

#include "vector.h"
#include "QAngle.h"

// Replay data format for ML training
// Designed to capture bot state, observations, and actions for supervised learning

struct VisibleEntityData {
    int entity_index;        // Entity index in game
    int entity_class;        // Player=0, NPC=1, Weapon=2, Item=3, Building=4, etc.
    Vector position;         // 3D world position
    float distance;          // Distance from bot (meters)
    float threat_level;      // 0-1 score (0=no threat, 1=immediate danger)
    float health_estimate;   // 0-1 estimated health (if visible)
    int team;                // Entity team (-1=unknown, 0=spectator, 2=red, 3=blue, etc.)
};

struct BotReplayFrame {
    // Metadata
    float timestamp;         // Game time when frame was recorded
    int bot_index;           // Which bot (for multi-bot recording)
    int frame_number;        // Sequential frame number

    // === Bot State (what the bot knows about itself) ===
    Vector position;         // Bot 3D position
    QAngle viewangle;        // Bot aim direction (pitch, yaw, roll)
    Vector velocity;         // Current velocity vector
    float health;            // Health (0-1 normalized)
    float armor;             // Armor (0-1 normalized)
    int ammo_primary;        // Primary weapon ammo
    int ammo_secondary;      // Secondary weapon ammo
    int weapon_id;           // Current active weapon ID
    int team;                // Bot's team

    // === Observations (what the bot sees) ===
    static constexpr int MAX_VISIBLE = 32;
    VisibleEntityData visible[MAX_VISIBLE];
    int num_visible;         // Number of visible entities (0-32)

    // Nearest pickups (if visible)
    Vector nearest_health_pack;    // Position of nearest health pack (-1,-1,-1 if none)
    Vector nearest_ammo_pack;      // Position of nearest ammo pack
    float dist_health_pack;        // Distance to health pack
    float dist_ammo_pack;          // Distance to ammo pack

    // === Actions Taken (labels for supervised learning) ===
    Vector movement;         // Movement input (forward/side/up, -450 to 450)
    QAngle aim_delta;        // Change in aim this frame (degrees)
    int buttons;             // Button state bitfield (IN_ATTACK, IN_JUMP, etc.)
    int weapon_switch_to;    // Weapon ID if switching (-1 if no switch)

    // === Outcomes (for reward calculation in RL) ===
    float damage_dealt;      // Damage dealt this frame
    float damage_taken;      // Damage taken this frame
    int kills;               // Cumulative kills this life
    int deaths;              // Total deaths this session
    float objective_score;   // Game mode specific score

    // === Navigation context ===
    int current_waypoint;    // Current waypoint index (-1 if none)
    int next_waypoint;       // Next waypoint in path (-1 if none)
    float dist_to_waypoint;  // Distance to next waypoint
};

struct BotReplayHeader {
    char magic[4];           // "RCBR" (RCBot Replay)
    int version;             // Format version (currently 1)
    char game_mode[32];      // e.g., "hl2dm", "tf2_ctf", "dod_conquest"
    char map_name[64];       // Map name (e.g., "dm_lockdown")
    int frame_count;         // Total number of frames in recording
    float duration;          // Recording duration in seconds
    int bot_count;           // Number of different bots recorded
    float recording_rate;    // Frames per second (typically 30-60)

    // Recording date/time
    int year;
    int month;
    int day;
    int hour;
    int minute;
};

// Estimates:
// - Size per frame: ~2.5 KB
// - 60 FPS recording: ~150 KB/sec = ~9 MB/minute
// - 10 minute session: ~90 MB
// - 1 hour session: ~540 MB

#endif // __BOT_REPLAY_FORMAT_H__
