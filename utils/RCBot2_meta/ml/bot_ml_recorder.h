/**
 * bot_ml_recorder.h
 *
 * Gameplay recording system for ML training data collection
 * Records state-action pairs for behavior cloning and RL training
 *
 * Part of Phase 0: Foundation and Infrastructure
 */

#ifndef __BOT_ML_RECORDER_H__
#define __BOT_ML_RECORDER_H__

#include <vector>
#include <string>
#include <fstream>

class CBot;
class Vector;
class QAngle;

/**
 * Replay Data Structures
 */

// Single frame of recorded gameplay
struct BotReplayFrame {
    float timestamp;          // Game time in seconds
    int bot_index;           // Bot entity index

    // State features (96 floats - matches feature extractor)
    float features[96];

    // Actions taken (what the bot did this frame)
    struct {
        float move_forward;   // -1 to 1
        float move_side;      // -1 to 1
        float move_up;        // -1 to 1
        float aim_yaw_delta;  // Degrees
        float aim_pitch_delta; // Degrees
        int buttons;          // Bitfield of buttons pressed
        int weapon_slot;      // Active weapon slot
    } action;

    // Outcome/reward (for RL training)
    struct {
        float damage_dealt;
        float damage_taken;
        float health_delta;
        int kills;            // Cumulative
        int deaths;           // Cumulative
        float objective_score; // Game mode specific
    } outcome;
};

// Header for replay file
struct BotReplayHeader {
    char magic[4];           // "RCBR" (RCBot Replay)
    int version;            // Format version (currently 1)
    char game[32];          // "tf2", "dod", etc.
    char game_mode[32];     // "payload", "ctf", etc.
    char map_name[64];      // Map name
    int frame_count;        // Total frames in recording
    float duration;         // Total duration in seconds
    float recording_fps;    // Frames per second
    int feature_count;      // Number of features per frame
};

/**
 * Bot Recorder (Singleton)
 *
 * Records gameplay for training ML models.
 * Can record bot AI actions or human player actions.
 */
class CBotRecorder {
public:
    static CBotRecorder* GetInstance();

    // Recording control
    void StartRecording();
    void StopRecording();
    bool IsRecording() const { return m_bRecording; }

    // Save/load recordings
    bool SaveRecording(const char* filename);
    bool LoadRecording(const char* filename);

    // Export to training formats
    bool ExportToJSON(const char* filename) const;
    bool ExportToCSV(const char* filename) const;
    bool ExportToNumpy(const char* filename) const; // Binary format for Python

    // Record a frame for a bot
    void RecordFrame(CBot* pBot);

    // Access recorded data
    size_t GetFrameCount() const { return m_frames.size(); }
    const BotReplayFrame& GetFrame(size_t index) const { return m_frames[index]; }
    const BotReplayHeader& GetHeader() const { return m_header; }

    // Memory management
    void Clear();
    void SetMaxFrames(size_t max_frames) { m_maxFrames = max_frames; }
    size_t GetMaxFrames() const { return m_maxFrames; }

    // Statistics
    void PrintStatistics() const;

private:
    CBotRecorder();
    ~CBotRecorder();

    bool m_bRecording;
    std::vector<BotReplayFrame> m_frames;
    BotReplayHeader m_header;

    // Circular buffer for memory management
    size_t m_maxFrames;
    size_t m_frameWriteIndex;

    // Feature extraction (use same extractor as inference)
    class IFeatureExtractor* m_pFeatureExtractor;

    // Helper methods
    void ExtractFeatures(CBot* pBot, float* features);
    void ExtractAction(CBot* pBot, BotReplayFrame::action& action);
    void ExtractOutcome(CBot* pBot, BotReplayFrame::outcome& outcome);

    // Serialization helpers
    bool WriteBinaryFrame(std::ofstream& file, const BotReplayFrame& frame) const;
    bool ReadBinaryFrame(std::ifstream& file, BotReplayFrame& frame);

    static CBotRecorder* s_pInstance;
};

/**
 * Demo Parser
 *
 * Parses SourceTV demo files to extract human player gameplay
 * for behavior cloning training data.
 */
class CDemoParser {
public:
    CDemoParser();
    ~CDemoParser();

    // Parse demo file
    bool ParseDemo(const char* demo_path);

    // Extract player data
    bool ExtractPlayerData(int player_index, std::vector<BotReplayFrame>& frames);

    // Get demo information
    int GetPlayerCount() const { return m_playerCount; }
    float GetDuration() const { return m_duration; }
    const char* GetMapName() const { return m_mapName.c_str(); }

private:
    int m_playerCount;
    float m_duration;
    std::string m_mapName;

    // Demo parsing is complex - may use external library
    // or implement basic functionality for specific data extraction
};

/**
 * Data Augmentation
 *
 * Augments recorded gameplay data for training
 * (horizontal flip, noise injection, etc.)
 */
class CDataAugmenter {
public:
    // Augmentation operations
    static void HorizontalFlip(BotReplayFrame& frame);
    static void AddNoise(BotReplayFrame& frame, float noise_scale);
    static void FeatureDropout(BotReplayFrame& frame, float dropout_rate);

    // Batch augmentation
    static void AugmentDataset(std::vector<BotReplayFrame>& frames,
                               bool horizontal_flip,
                               float noise_scale,
                               float dropout_rate);
};

#endif // __BOT_ML_RECORDER_H__
