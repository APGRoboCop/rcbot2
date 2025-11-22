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

#ifndef __BOT_ML_CONTROLLER_H__
#define __BOT_ML_CONTROLLER_H__

#include <vector>
#include <string>

class CBot;  // Forward declaration
class CONNXModel;
class CFeatureExtractor;

/**
 * CMLBotController - Controls a bot using ML model inference
 *
 * This class bridges the gap between the ML model and bot behavior:
 * 1. Extracts features from current game state
 * 2. Runs ONNX model inference
 * 3. Interprets model outputs as bot actions
 * 4. Applies actions to bot
 *
 * Design principles:
 * - Per-bot controller (each bot can use different model or rule-based AI)
 * - Fallback to rule-based AI on inference failure
 * - Performance monitoring (inference time tracking)
 * - Optional action smoothing to prevent jitter
 */
class CMLBotController {
public:
    CMLBotController(CBot* pBot);
    ~CMLBotController();

    /**
     * Update bot using ML model
     * Called from bot's think loop when ML mode is enabled
     * @return true if ML control succeeded, false to fallback to rule-based AI
     */
    bool Update();

    /**
     * Enable ML control for this bot
     * @param model_name Name of loaded ONNX model to use
     * @return true if successfully enabled
     */
    bool Enable(const char* model_name);

    /**
     * Disable ML control (revert to rule-based AI)
     */
    void Disable();

    /**
     * Check if ML control is active
     */
    bool IsEnabled() const { return m_bEnabled; }

    /**
     * Get the bot this controller manages
     */
    CBot* GetBot() const { return m_pBot; }

    /**
     * Get name of current model
     */
    const char* GetModelName() const { return m_szModelName; }

    /**
     * Get performance statistics
     */
    float GetAverageInferenceTime() const { return m_fAvgInferenceTime; }
    float GetLastInferenceTime() const { return m_fLastInferenceTime; }
    int GetInferenceCount() const { return m_iInferenceCount; }

    /**
     * Set action smoothing (helps prevent jittery movement)
     * @param alpha Smoothing factor (0 = no smoothing, 1 = max smoothing)
     */
    void SetActionSmoothing(float alpha) { m_fActionSmoothing = alpha; }

private:
    CBot* m_pBot;                      // Bot being controlled
    bool m_bEnabled;                    // ML control enabled
    char m_szModelName[64];             // Name of current model
    CONNXModel* m_pModel;               // ONNX model instance
    CFeatureExtractor* m_pExtractor;    // Feature extractor

    // Performance tracking
    float m_fLastInferenceTime;         // Last inference time (ms)
    float m_fAvgInferenceTime;          // Running average inference time (ms)
    int m_iInferenceCount;              // Total inferences performed

    // Action state
    std::vector<float> m_LastFeatures;  // Previous frame features
    std::vector<float> m_LastActions;   // Previous frame actions
    float m_fActionSmoothing;           // Action smoothing factor (0-1)

    // Helper methods
    bool ExtractFeatures(std::vector<float>& features);
    bool RunInference(const std::vector<float>& features, std::vector<float>& actions);
    void ApplyActions(const std::vector<float>& actions);
    void SmoothActions(std::vector<float>& actions);

    /**
     * Interpret model outputs as bot actions
     *
     * Action vector (9 outputs from model):
     * [0-2] Movement: forward/back, left/right, up/down (-1 to 1)
     * [3-4] Aim delta: yaw, pitch (-1 to 1)
     * [5-8] Buttons: attack, jump, crouch, reload (0 or 1)
     */
    void InterpretMovement(const std::vector<float>& actions);
    void InterpretAim(const std::vector<float>& actions);
    void InterpretButtons(const std::vector<float>& actions);
};

/**
 * CMLBotManager - Singleton manager for all ML-controlled bots
 *
 * Provides global access to ML bot control functionality
 */
class CMLBotManager {
public:
    static CMLBotManager* GetInstance();
    static void Destroy();

    /**
     * Enable ML control for a bot
     * @param pBot Bot to control
     * @param model_name Name of ONNX model to use
     * @return true if successfully enabled
     */
    bool EnableMLControl(CBot* pBot, const char* model_name);

    /**
     * Disable ML control for a bot
     */
    void DisableMLControl(CBot* pBot);

    /**
     * Check if bot has ML control enabled
     */
    bool IsMLControlled(CBot* pBot);

    /**
     * Get controller for a bot (returns nullptr if not ML-controlled)
     */
    CMLBotController* GetController(CBot* pBot);

    /**
     * Update all ML-controlled bots
     * Can be called from game frame or per-bot think
     */
    void UpdateAll();

    /**
     * Get statistics for all ML-controlled bots
     */
    void GetStatistics(int& total_bots, int& ml_bots, float& avg_inference_time);

private:
    CMLBotManager();
    ~CMLBotManager();

    static CMLBotManager* s_pInstance;
    std::vector<CMLBotController*> m_Controllers;

    // Helper to find controller by bot
    int FindControllerIndex(CBot* pBot);
};

#endif // __BOT_ML_CONTROLLER_H__
