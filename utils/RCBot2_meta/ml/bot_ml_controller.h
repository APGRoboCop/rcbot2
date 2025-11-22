/**
 * bot_ml_controller.h
 *
 * Main ML controller that coordinates model inference, feature extraction,
 * and action execution. This is the primary interface between bot AI and ML.
 *
 * Part of Phase 0: Foundation and Infrastructure
 */

#ifndef __BOT_ML_CONTROLLER_H__
#define __BOT_ML_CONTROLLER_H__

#include <vector>

class CBot;
class Vector;
class QAngle;
class CONNXModel;
class IFeatureExtractor;

/**
 * ML Mode Enumeration
 */
enum MLMode {
    ML_MODE_OFF = 0,           // Disabled, use rule-based AI only
    ML_MODE_BEHAVIOR_CLONE,    // Behavior cloning model
    ML_MODE_RL_DQN,           // Reinforcement learning (DQN)
    ML_MODE_RL_PPO,           // Reinforcement learning (PPO)
    ML_MODE_HYBRID,           // Hybrid: ML for low-level, rules for high-level
    ML_MODE_ENSEMBLE          // Ensemble of multiple models
};

/**
 * ML Action Output
 *
 * Decoded actions from ML model output
 */
struct MLAction {
    Vector movement;          // Movement vector (forward, side, up)
    QAngle aim_delta;        // Aim adjustment (yaw, pitch)
    int buttons;             // Button bitfield (IN_ATTACK, IN_JUMP, etc.)
    int weapon_slot;         // Desired weapon slot
    float confidence;        // Model confidence (0-1)
};

/**
 * ML Performance Metrics
 */
struct MLPerformanceMetrics {
    float avg_inference_time_ms;
    float max_inference_time_ms;
    float min_inference_time_ms;
    size_t total_inferences;
    size_t failed_inferences;
    float failure_rate;
};

/**
 * ML Controller
 *
 * Coordinates all ML operations for a single bot:
 * - Feature extraction
 * - Model inference
 * - Action decoding
 * - Performance monitoring
 */
class CMLController {
public:
    CMLController(CBot* pBot);
    ~CMLController();

    // Initialization
    bool Initialize();
    void Shutdown();
    bool IsInitialized() const { return m_bInitialized; }

    // Main ML decision method
    // Returns true if ML provided an action, false to fallback to rules
    bool GetMLAction(MLAction& action);

    // Mode control
    void SetMode(MLMode mode);
    MLMode GetMode() const { return m_mode; }
    bool IsModeActive() const { return m_mode != ML_MODE_OFF; }

    // Model management
    bool LoadModel(const char* model_path);
    bool SetModel(CONNXModel* pModel);
    CONNXModel* GetModel() { return m_pModel; }

    // Performance monitoring
    const MLPerformanceMetrics& GetMetrics() const { return m_metrics; }
    void ResetMetrics();
    void PrintMetrics() const;

    // Debugging
    void DumpCurrentFeatures() const;
    void DumpLastPrediction() const;

private:
    CBot* m_pBot;
    bool m_bInitialized;
    MLMode m_mode;

    // ML components
    CONNXModel* m_pModel;
    IFeatureExtractor* m_pFeatureExtractor;

    // Cached data
    std::vector<float> m_lastFeatures;
    std::vector<float> m_lastOutput;
    MLAction m_lastAction;

    // Performance metrics
    MLPerformanceMetrics m_metrics;

    // Internal methods
    bool ExtractFeatures(std::vector<float>& features);
    bool RunInference(const std::vector<float>& features, std::vector<float>& output);
    bool DecodeAction(const std::vector<float>& output, MLAction& action);

    // Action decoding helpers
    void DecodeMovement(const std::vector<float>& output, Vector& movement);
    void DecodeAiming(const std::vector<float>& output, QAngle& aim_delta);
    void DecodeButtons(const std::vector<float>& output, int& buttons);
    void DecodeWeapon(const std::vector<float>& output, int& weapon_slot);

    // Validation
    bool ValidateFeatures(const std::vector<float>& features) const;
    bool ValidateOutput(const std::vector<float>& output) const;
    void ClampAction(MLAction& action) const;

    // Fallback handling
    bool ShouldFallbackToRules() const;
    void LogFallback(const char* reason);
};

/**
 * ML Manager (Singleton)
 *
 * Manages ML controllers for all bots
 * Handles global ML configuration and model sharing
 */
class CMLManager {
public:
    static CMLManager* GetInstance();

    // Initialization
    bool Initialize();
    void Shutdown();

    // Per-bot controller management
    CMLController* GetController(CBot* pBot);
    CMLController* CreateController(CBot* pBot);
    void DestroyController(CBot* pBot);

    // Global ML settings
    void SetGlobalMode(MLMode mode);
    MLMode GetGlobalMode() const { return m_globalMode; }

    // Configuration
    bool LoadConfig(const char* config_path);
    void SaveConfig(const char* config_path);

    // Performance monitoring (all bots)
    void PrintGlobalMetrics() const;
    void ResetGlobalMetrics();

    // Console commands (called from ConCommand handlers)
    void CMD_LoadModel(const char* model_path);
    void CMD_SetMode(const char* mode_name);
    void CMD_DumpFeatures();
    void CMD_Stats();

private:
    CMLManager();
    ~CMLManager();

    std::map<CBot*, CMLController*> m_controllers;
    MLMode m_globalMode;

    // Shared models (to save memory)
    std::map<std::string, CONNXModel*> m_sharedModels;

    // Configuration
    std::string m_configPath;

    static CMLManager* s_pInstance;
};

/**
 * Hybrid AI System
 *
 * Combines ML and rule-based AI:
 * - ML handles low-level control (movement, aiming)
 * - Rules handle high-level strategy (objectives, team coordination)
 */
class CHybridAI {
public:
    CHybridAI(CBot* pBot);
    ~CHybridAI();

    // Main decision method
    void MakeDecision();

    // Configuration
    void SetUseMLForMovement(bool use) { m_bUseMLMovement = use; }
    void SetUseMLForAiming(bool use) { m_bUseMLAiming = use; }
    void SetUseMLForButtons(bool use) { m_bUseMLButtons = use; }

private:
    CBot* m_pBot;
    CMLController* m_pMLController;

    bool m_bUseMLMovement;
    bool m_bUseMLAiming;
    bool m_bUseMLButtons;

    // Blending methods
    void BlendMovement(const Vector& ml_move, const Vector& rule_move, Vector& final_move);
    void BlendAiming(const QAngle& ml_aim, const QAngle& rule_aim, QAngle& final_aim);
    void BlendButtons(int ml_buttons, int rule_buttons, int& final_buttons);
};

#endif // __BOT_ML_CONTROLLER_H__
