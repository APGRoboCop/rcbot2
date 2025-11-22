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

#ifndef __BOT_RECORDER_H__
#define __BOT_RECORDER_H__

#include "bot_replay_format.h"
#include <vector>
#include <string>

class CBot;  // Forward declaration

/**
 * CBotRecorder - Records bot gameplay data for machine learning training
 *
 * This class implements a singleton pattern to manage recording of bot gameplay.
 * It captures state, observations, and actions at each frame, which can later
 * be used for supervised learning (behavior cloning) or reinforcement learning.
 *
 * Usage:
 *   CBotRecorder::GetInstance()->StartRecording();
 *   // ... gameplay happens, RecordFrame() called automatically ...
 *   CBotRecorder::GetInstance()->StopRecording();
 *   CBotRecorder::GetInstance()->SaveRecording("mydata.rcbr");
 */
class CBotRecorder {
public:
    // Singleton access
    static CBotRecorder* GetInstance();

    // Recording control
    void StartRecording();
    void StopRecording();
    void PauseRecording();
    void ResumeRecording();
    bool IsRecording() const { return m_bRecording && !m_bPaused; }
    bool IsPaused() const { return m_bPaused; }

    // Save/load recordings
    bool SaveRecording(const char* filename);
    bool LoadRecording(const char* filename);

    // Export to different formats for ML training
    bool ExportToJSON(const char* filename);
    bool ExportToCSV(const char* filename);
    bool ExportToNumPy(const char* filename);  // Future: export to .npy format

    // Record a frame of bot gameplay (called from CBot::think())
    void RecordFrame(CBot* pBot);

    // Statistics
    int GetFrameCount() const { return static_cast<int>(m_frames.size()); }
    float GetRecordingDuration() const;
    void ClearRecording();

    // Configuration
    void SetRecordingRate(float fps) { m_recordingRate = fps; }
    void SetMaxFrames(int maxFrames) { m_maxFrames = maxFrames; }

private:
    // Singleton - private constructor
    CBotRecorder();
    ~CBotRecorder();
    CBotRecorder(const CBotRecorder&) = delete;
    CBotRecorder& operator=(const CBotRecorder&) = delete;

    // Helper methods for extracting data from bot
    void ExtractBotState(CBot* pBot, BotReplayFrame& frame);
    void ExtractVisibleEntities(CBot* pBot, BotReplayFrame& frame);
    void ExtractActions(CBot* pBot, BotReplayFrame& frame);
    void ExtractOutcomes(CBot* pBot, BotReplayFrame& frame);
    void ExtractNavigationContext(CBot* pBot, BotReplayFrame& frame);

    // Internal state
    static CBotRecorder* s_instance;
    bool m_bRecording;
    bool m_bPaused;
    std::vector<BotReplayFrame> m_frames;
    BotReplayHeader m_header;

    // Recording parameters
    float m_recordingRate;        // Target FPS for recording
    float m_lastRecordTime;       // Last time we recorded a frame
    int m_frameCounter;           // Frame number counter
    int m_maxFrames;              // Maximum frames to store (circular buffer)

    // Circular buffer management
    void AddFrame(const BotReplayFrame& frame);
    void InitializeHeader();
};

#endif // __BOT_RECORDER_H__
