/**
 * bot_ml_cvars.h
 *
 * Console variables and commands for ML system
 *
 * Part of Phase 0: Foundation and Infrastructure
 */

#ifndef __BOT_ML_CVARS_H__
#define __BOT_ML_CVARS_H__

/**
 * ML Console Variables (ConVars)
 *
 * These should be declared in bot_cvars.cpp with the RCBOT_CVAR macro
 */

// Core ML settings
// rcbot_ml_enable <0/1> - Enable/disable ML globally
// rcbot_ml_mode <mode> - ML mode: off, behavior_clone, dqn, ppo, hybrid, ensemble
// rcbot_ml_model_path <path> - Path to ONNX model file
// rcbot_ml_config_path <path> - Path to ML configuration JSON

// Feature extraction
// rcbot_ml_features_enable <0/1> - Enable feature extraction
// rcbot_ml_features_config <path> - Feature configuration file
// rcbot_ml_features_validate <0/1> - Validate features before inference

// Performance settings
// rcbot_ml_max_inference_ms <float> - Max inference time before fallback (default: 1.0)
// rcbot_ml_fallback_on_error <0/1> - Fallback to rules on inference error
// rcbot_ml_batch_inference <0/1> - Batch inference for multiple bots
// rcbot_ml_async_inference <0/1> - Run inference on separate thread

// Recording
// rcbot_record_enable <0/1> - Enable gameplay recording
// rcbot_record_max_frames <int> - Max frames to record (default: 100000)
// rcbot_record_fps <float> - Recording frames per second (default: 20)
// rcbot_record_format <format> - Recording format: binary, json, csv

// Debugging
// rcbot_ml_debug <0/1> - Enable ML debug output
// rcbot_ml_debug_features <0/1> - Print features every frame
// rcbot_ml_debug_predictions <0/1> - Print model predictions
// rcbot_ml_visualize_features <0/1> - Show feature visualization (HUD overlay)

// Performance monitoring
// rcbot_ml_stats_interval <seconds> - Stats logging interval (default: 60)
// rcbot_ml_stats_reset_on_map_change <0/1> - Reset stats on map change

/**
 * ML Console Commands (ConCommands)
 */

// Model management
// rcbot_ml_load_model <path> - Load ONNX model
// rcbot_ml_reload_model - Reload current model
// rcbot_ml_unload_model - Unload current model
// rcbot_ml_list_models - List available models from config

// Recording
// rcbot_record_start - Start recording gameplay
// rcbot_record_stop - Stop recording
// rcbot_record_save <filename> - Save recording to file
// rcbot_record_clear - Clear recorded frames
// rcbot_record_export_json <filename> - Export to JSON
// rcbot_record_export_csv <filename> - Export to CSV
// rcbot_record_export_numpy <filename> - Export to NumPy binary

// Feature debugging
// rcbot_ml_features_dump - Print current features for bot 0
// rcbot_ml_features_dump_bot <bot_index> - Print features for specific bot
// rcbot_ml_features_stats - Print feature statistics
// rcbot_ml_features_reset_stats - Reset feature statistics

// Performance monitoring
// rcbot_ml_stats - Print ML performance statistics
// rcbot_ml_stats_reset - Reset performance statistics
// rcbot_ml_stats_save <filename> - Save stats to file

// Testing
// rcbot_ml_test_inference - Run test inference with dummy data
// rcbot_ml_benchmark <iterations> - Benchmark model inference
// rcbot_ml_validate_model - Validate loaded model

// Configuration
// rcbot_ml_reload_config - Reload ML configuration
// rcbot_ml_save_config - Save current ML configuration
// rcbot_ml_print_config - Print current configuration

/**
 * Console Command Handlers
 *
 * These functions should be implemented in bot_ml_cvars.cpp
 * and registered as ConCommand callbacks
 */
class CMLConsoleCommands {
public:
    // Model commands
    static void CMD_LoadModel(const char* model_path);
    static void CMD_ReloadModel();
    static void CMD_UnloadModel();
    static void CMD_ListModels();

    // Recording commands
    static void CMD_RecordStart();
    static void CMD_RecordStop();
    static void CMD_RecordSave(const char* filename);
    static void CMD_RecordClear();
    static void CMD_RecordExportJSON(const char* filename);
    static void CMD_RecordExportCSV(const char* filename);
    static void CMD_RecordExportNumpy(const char* filename);

    // Feature commands
    static void CMD_FeaturesDump(int bot_index = 0);
    static void CMD_FeaturesStats();
    static void CMD_FeaturesResetStats();

    // Performance commands
    static void CMD_Stats();
    static void CMD_StatsReset();
    static void CMD_StatsSave(const char* filename);

    // Testing commands
    static void CMD_TestInference();
    static void CMD_Benchmark(int iterations);
    static void CMD_ValidateModel();

    // Configuration commands
    static void CMD_ReloadConfig();
    static void CMD_SaveConfig();
    static void CMD_PrintConfig();
};

/**
 * ConVar Declarations
 *
 * Actual ConVar instances should be declared in bot_cvars.cpp
 * Example:
 *
 * ConVar rcbot_ml_enable("rcbot_ml_enable", "0", FCVAR_NONE,
 *                        "Enable ML system");
 * ConVar rcbot_ml_mode("rcbot_ml_mode", "off", FCVAR_NONE,
 *                      "ML mode: off, behavior_clone, dqn, ppo, hybrid");
 */

// Helper functions for accessing ConVars
namespace MLCVars {
    bool IsMLEnabled();
    const char* GetMLMode();
    const char* GetModelPath();
    float GetMaxInferenceMs();
    bool GetFallbackOnError();
    bool GetDebugEnabled();
    int GetRecordMaxFrames();
}

#endif // __BOT_ML_CVARS_H__
