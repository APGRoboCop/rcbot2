// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
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

#include "bot_recorder.h"
#include "bot.h"
#include "bot_globals.h"
#include "bot_navigator.h"
#include "bot_weapons.h"
#include "bot_visibles.h"
#include "eiface.h"

#include <ctime>
#include <cstdio>
#include <cstring>
#include <fstream>

// Singleton instance
CBotRecorder* CBotRecorder::s_instance = nullptr;

CBotRecorder::CBotRecorder()
    : m_bRecording(false)
    , m_bPaused(false)
    , m_recordingRate(30.0f)  // Default 30 FPS
    , m_lastRecordTime(0.0f)
    , m_frameCounter(0)
    , m_maxFrames(100000)  // ~55 minutes at 30 FPS
{
    m_frames.reserve(10000);  // Pre-allocate for better performance
    InitializeHeader();
}

CBotRecorder::~CBotRecorder()
{
    StopRecording();
    ClearRecording();
}

CBotRecorder* CBotRecorder::GetInstance()
{
    if (s_instance == nullptr) {
        s_instance = new CBotRecorder();
    }
    return s_instance;
}

void CBotRecorder::InitializeHeader()
{
    memset(&m_header, 0, sizeof(BotReplayHeader));
    m_header.magic[0] = 'R';
    m_header.magic[1] = 'C';
    m_header.magic[2] = 'B';
    m_header.magic[3] = 'R';
    m_header.version = 1;

    // Get current time
    time_t now = time(nullptr);
    tm* timeinfo = localtime(&now);
    if (timeinfo) {
        m_header.year = timeinfo->tm_year + 1900;
        m_header.month = timeinfo->tm_mon + 1;
        m_header.day = timeinfo->tm_mday;
        m_header.hour = timeinfo->tm_hour;
        m_header.minute = timeinfo->tm_min;
    }

    m_header.recording_rate = m_recordingRate;
}

void CBotRecorder::StartRecording()
{
    if (m_bRecording) {
        Msg("[RCBot2 ML] Already recording!\n");
        return;
    }

    ClearRecording();
    InitializeHeader();
    m_bRecording = true;
    m_bPaused = false;
    m_frameCounter = 0;
    m_lastRecordTime = engine->Time();

    // Get map name
    const char* mapName = CBotGlobals::getMapName();
    if (mapName) {
        strncpy(m_header.map_name, mapName, sizeof(m_header.map_name) - 1);
    }

    // Set game mode based on current game
    const char* gameMode = "unknown";
    if (CBotGlobals::isHL2DM()) {
        gameMode = "hl2dm";
    } else if (CBotGlobals::isTF2()) {
        gameMode = "tf2";
    } else if (CBotGlobals::isDOD()) {
        gameMode = "dod";
    }
    strncpy(m_header.game_mode, gameMode, sizeof(m_header.game_mode) - 1);

    Msg("[RCBot2 ML] Recording started on map %s (mode: %s)\n",
        m_header.map_name, m_header.game_mode);
}

void CBotRecorder::StopRecording()
{
    if (!m_bRecording) {
        return;
    }

    m_bRecording = false;
    m_bPaused = false;
    m_header.frame_count = static_cast<int>(m_frames.size());
    m_header.duration = GetRecordingDuration();

    Msg("[RCBot2 ML] Recording stopped. Frames: %d, Duration: %.1fs\n",
        m_header.frame_count, m_header.duration);
}

void CBotRecorder::PauseRecording()
{
    if (m_bRecording) {
        m_bPaused = true;
        Msg("[RCBot2 ML] Recording paused\n");
    }
}

void CBotRecorder::ResumeRecording()
{
    if (m_bRecording && m_bPaused) {
        m_bPaused = false;
        m_lastRecordTime = engine->Time();
        Msg("[RCBot2 ML] Recording resumed\n");
    }
}

void CBotRecorder::ClearRecording()
{
    m_frames.clear();
    m_frameCounter = 0;
    m_lastRecordTime = 0.0f;
}

float CBotRecorder::GetRecordingDuration() const
{
    if (m_frames.empty()) {
        return 0.0f;
    }
    return m_frames.back().timestamp - m_frames.front().timestamp;
}

void CBotRecorder::AddFrame(const BotReplayFrame& frame)
{
    if (static_cast<int>(m_frames.size()) >= m_maxFrames) {
        // Circular buffer: remove oldest frame
        m_frames.erase(m_frames.begin());
        Msg("[RCBot2 ML] Warning: Max frames reached, discarding oldest frame\n");
    }
    m_frames.push_back(frame);
}

void CBotRecorder::RecordFrame(CBot* pBot)
{
    if (!IsRecording() || !pBot || !pBot->inUse() || !pBot->isAlive()) {
        return;
    }

    // Rate limiting: only record at target FPS
    const float currentTime = engine->Time();
    const float timeSinceLastRecord = currentTime - m_lastRecordTime;
    const float targetInterval = 1.0f / m_recordingRate;

    if (timeSinceLastRecord < targetInterval) {
        return;  // Too soon, skip this frame
    }

    m_lastRecordTime = currentTime;

    // Create new frame
    BotReplayFrame frame;
    memset(&frame, 0, sizeof(BotReplayFrame));

    frame.timestamp = currentTime;
    frame.bot_index = pBot->getIndex();
    frame.frame_number = m_frameCounter++;

    // Extract all data from bot
    ExtractBotState(pBot, frame);
    ExtractVisibleEntities(pBot, frame);
    ExtractActions(pBot, frame);
    ExtractOutcomes(pBot, frame);
    ExtractNavigationContext(pBot, frame);

    // Add to recording
    AddFrame(frame);
}

void CBotRecorder::ExtractBotState(CBot* pBot, BotReplayFrame& frame)
{
    // Position and orientation
    frame.position = pBot->getOrigin();
    frame.viewangle = CBotGlobals::playerAngles(pBot->getEdict());
    frame.velocity = pBot->getVelocity();

    // Health and armor (normalized 0-1)
    const float maxHealth = static_cast<float>(pBot->getMaxHealth());
    frame.health = (maxHealth > 0) ? (pBot->getHealth() / maxHealth) : 0.0f;

    // Armor depends on game mode
    if (pBot->isHLDM()) {
        // HL2DM has armor
        frame.armor = pBot->getHealthPercent();  // Using health percent as proxy for now
    } else {
        frame.armor = 0.0f;
    }

    // Ammo
    CBotWeapon* pWeapon = pBot->getCurrentWeapon();
    if (pWeapon) {
        frame.ammo_primary = pWeapon->getAmmo(pBot);
        frame.weapon_id = pWeapon->getID();
    } else {
        frame.ammo_primary = 0;
        frame.weapon_id = -1;
    }
    frame.ammo_secondary = 0;  // TODO: Get secondary ammo

    // Team
    frame.team = pBot->getTeam();
}

void CBotRecorder::ExtractVisibleEntities(CBot* pBot, BotReplayFrame& frame)
{
    frame.num_visible = 0;

    // Get visible entities from bot's vision system
    CBotVisibles* pVisibles = pBot->getVisibles();
    if (!pVisibles) {
        return;
    }

    // Initialize nearest pickup tracking
    frame.nearest_health_pack = Vector(-9999, -9999, -9999);
    frame.nearest_ammo_pack = Vector(-9999, -9999, -9999);
    frame.dist_health_pack = 999999.0f;
    frame.dist_ammo_pack = 999999.0f;

    // Iterate through visible entities (up to MAX_VISIBLE)
    for (int i = 0; i < pVisibles->numVisible() && frame.num_visible < BotReplayFrame::MAX_VISIBLE; i++) {
        edict_t* pEntity = pVisibles->getVisible(i);
        if (!pEntity || !CBotGlobals::entityIsValid(pEntity)) {
            continue;
        }

        VisibleEntityData& visData = frame.visible[frame.num_visible];

        visData.entity_index = ENTINDEX(pEntity);
        visData.position = CBotGlobals::entityOrigin(pEntity);
        visData.distance = (visData.position - frame.position).Length();

        // Classify entity
        if (pEntity->GetUnknown()) {
            IServerNetworkable* pNet = pEntity->GetNetworkable();
            if (pNet) {
                const char* classname = pNet->GetClassName();
                if (classname) {
                    // Simple classification
                    if (strstr(classname, "player")) {
                        visData.entity_class = 0;  // Player
                    } else if (strstr(classname, "weapon")) {
                        visData.entity_class = 2;  // Weapon
                    } else if (strstr(classname, "item_health") || strstr(classname, "healthkit")) {
                        visData.entity_class = 3;  // Health item
                        if (visData.distance < frame.dist_health_pack) {
                            frame.nearest_health_pack = visData.position;
                            frame.dist_health_pack = visData.distance;
                        }
                    } else if (strstr(classname, "item_ammo") || strstr(classname, "ammopack")) {
                        visData.entity_class = 3;  // Ammo item
                        if (visData.distance < frame.dist_ammo_pack) {
                            frame.nearest_ammo_pack = visData.position;
                            frame.dist_ammo_pack = visData.distance;
                        }
                    } else {
                        visData.entity_class = 5;  // Other
                    }
                }
            }
        }

        // Team
        visData.team = pBot->getTeamViaEdict(pEntity);

        // Threat level (0-1)
        if (pBot->isEnemy(pEntity)) {
            visData.threat_level = 1.0f / (1.0f + visData.distance / 1000.0f);  // Closer = more threatening
        } else {
            visData.threat_level = 0.0f;
        }

        // Health estimate
        visData.health_estimate = 0.5f;  // TODO: Implement health estimation

        frame.num_visible++;
    }
}

void CBotRecorder::ExtractActions(CBot* pBot, BotReplayFrame& frame)
{
    // Movement (forward/side/up)
    frame.movement = pBot->getMoveTo();  // This might need adjustment

    // Aim delta (change in viewangle)
    // Note: This should ideally compare with previous frame
    // For now, we'll set it to zero and update in next iteration
    frame.aim_delta = QAngle(0, 0, 0);

    // Buttons
    frame.buttons = pBot->getButtons();

    // Weapon switch
    frame.weapon_switch_to = -1;  // TODO: Track weapon switching
}

void CBotRecorder::ExtractOutcomes(CBot* pBot, BotReplayFrame& frame)
{
    // These would ideally be tracked across frames
    // For now, initialize to zero
    frame.damage_dealt = 0.0f;
    frame.damage_taken = 0.0f;
    frame.kills = 0;  // TODO: Track from game events
    frame.deaths = 0;
    frame.objective_score = 0.0f;
}

void CBotRecorder::ExtractNavigationContext(CBot* pBot, BotReplayFrame& frame)
{
    CBotNavigator* pNav = pBot->getNavigator();
    if (pNav) {
        frame.current_waypoint = pNav->getCurrentWaypointID();
        frame.next_waypoint = pNav->getNextWaypointID();

        if (pNav->hasNextPoint()) {
            Vector nextPos = pNav->getNextPoint();
            frame.dist_to_waypoint = (nextPos - frame.position).Length();
        } else {
            frame.dist_to_waypoint = -1.0f;
        }
    } else {
        frame.current_waypoint = -1;
        frame.next_waypoint = -1;
        frame.dist_to_waypoint = -1.0f;
    }
}

bool CBotRecorder::SaveRecording(const char* filename)
{
    if (m_frames.empty()) {
        Warning("[RCBot2 ML] No frames to save\n");
        return false;
    }

    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        Warning("[RCBot2 ML] Failed to open file for writing: %s\n", filename);
        return false;
    }

    // Update header
    m_header.frame_count = static_cast<int>(m_frames.size());
    m_header.duration = GetRecordingDuration();

    // Write header
    file.write(reinterpret_cast<const char*>(&m_header), sizeof(BotReplayHeader));

    // Write all frames
    for (const auto& frame : m_frames) {
        file.write(reinterpret_cast<const char*>(&frame), sizeof(BotReplayFrame));
    }

    file.close();

    Msg("[RCBot2 ML] Saved %d frames to %s (%.2f MB)\n",
        m_header.frame_count, filename,
        (sizeof(BotReplayHeader) + m_frames.size() * sizeof(BotReplayFrame)) / (1024.0f * 1024.0f));

    return true;
}

bool CBotRecorder::LoadRecording(const char* filename)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        Warning("[RCBot2 ML] Failed to open file for reading: %s\n", filename);
        return false;
    }

    // Read header
    file.read(reinterpret_cast<char*>(&m_header), sizeof(BotReplayHeader));

    // Verify magic number
    if (m_header.magic[0] != 'R' || m_header.magic[1] != 'C' ||
        m_header.magic[2] != 'B' || m_header.magic[3] != 'R') {
        Warning("[RCBot2 ML] Invalid replay file format\n");
        file.close();
        return false;
    }

    // Read frames
    ClearRecording();
    m_frames.reserve(m_header.frame_count);

    for (int i = 0; i < m_header.frame_count; i++) {
        BotReplayFrame frame;
        file.read(reinterpret_cast<char*>(&frame), sizeof(BotReplayFrame));
        m_frames.push_back(frame);
    }

    file.close();

    Msg("[RCBot2 ML] Loaded %d frames from %s\n", m_header.frame_count, filename);
    return true;
}

bool CBotRecorder::ExportToJSON(const char* filename)
{
    if (m_frames.empty()) {
        Warning("[RCBot2 ML] No frames to export\n");
        return false;
    }

    std::ofstream file(filename);
    if (!file.is_open()) {
        Warning("[RCBot2 ML] Failed to open JSON file for writing: %s\n", filename);
        return false;
    }

    file << "{\n";
    file << "  \"header\": {\n";
    file << "    \"version\": " << m_header.version << ",\n";
    file << "    \"game_mode\": \"" << m_header.game_mode << "\",\n";
    file << "    \"map_name\": \"" << m_header.map_name << "\",\n";
    file << "    \"frame_count\": " << m_header.frame_count << ",\n";
    file << "    \"duration\": " << m_header.duration << ",\n";
    file << "    \"recording_rate\": " << m_header.recording_rate << "\n";
    file << "  },\n";
    file << "  \"frames\": [\n";

    for (size_t i = 0; i < m_frames.size(); i++) {
        const BotReplayFrame& f = m_frames[i];

        file << "    {\n";
        file << "      \"timestamp\": " << f.timestamp << ",\n";
        file << "      \"bot_index\": " << f.bot_index << ",\n";
        file << "      \"position\": [" << f.position.x << ", " << f.position.y << ", " << f.position.z << "],\n";
        file << "      \"health\": " << f.health << ",\n";
        file << "      \"armor\": " << f.armor << ",\n";
        file << "      \"weapon_id\": " << f.weapon_id << ",\n";
        file << "      \"num_visible\": " << f.num_visible << ",\n";
        file << "      \"buttons\": " << f.buttons << "\n";
        file << "    }";

        if (i < m_frames.size() - 1) {
            file << ",";
        }
        file << "\n";
    }

    file << "  ]\n";
    file << "}\n";

    file.close();

    Msg("[RCBot2 ML] Exported %d frames to JSON: %s\n", m_header.frame_count, filename);
    return true;
}

bool CBotRecorder::ExportToCSV(const char* filename)
{
    if (m_frames.empty()) {
        Warning("[RCBot2 ML] No frames to export\n");
        return false;
    }

    std::ofstream file(filename);
    if (!file.is_open()) {
        Warning("[RCBot2 ML] Failed to open CSV file for writing: %s\n", filename);
        return false;
    }

    // CSV Header
    file << "timestamp,bot_index,pos_x,pos_y,pos_z,health,armor,weapon_id,num_visible,buttons\n";

    // Write frames
    for (const auto& f : m_frames) {
        file << f.timestamp << ","
             << f.bot_index << ","
             << f.position.x << "," << f.position.y << "," << f.position.z << ","
             << f.health << "," << f.armor << ","
             << f.weapon_id << ","
             << f.num_visible << ","
             << f.buttons << "\n";
    }

    file.close();

    Msg("[RCBot2 ML] Exported %d frames to CSV: %s\n", m_header.frame_count, filename);
    return true;
}

bool CBotRecorder::ExportToNumPy(const char* filename)
{
    // TODO: Implement .npy export for direct Python/NumPy loading
    Warning("[RCBot2 ML] NumPy export not yet implemented\n");
    return false;
}
