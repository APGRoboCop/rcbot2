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

#include "bot.h"
#include "bot_ml_controller.h"
#include "bot_onnx.h"
#include "bot_features.h"
#include "bot_buttons.h"
#include <cstring>
#include <cmath>

#ifdef RCBOT_WITH_ONNX
#include <chrono>
#endif

// ============================================================================
// CMLBotController Implementation
// ============================================================================

CMLBotController::CMLBotController(CBot* pBot)
    : m_pBot(pBot)
    , m_bEnabled(false)
    , m_pModel(nullptr)
    , m_pExtractor(nullptr)
    , m_fLastInferenceTime(0.0f)
    , m_fAvgInferenceTime(0.0f)
    , m_iInferenceCount(0)
    , m_fActionSmoothing(0.3f)  // Default smoothing
{
    m_szModelName[0] = '\0';
    m_LastFeatures.reserve(64);   // Pre-allocate for typical feature count
    m_LastActions.reserve(16);    // Pre-allocate for action vector
}

CMLBotController::~CMLBotController()
{
    // Note: We don't delete m_pModel (managed by CONNXModelManager)
    if (m_pExtractor)
    {
        CFeatureExtractorFactory::DestroyExtractor(m_pExtractor);
        m_pExtractor = nullptr;
    }
}

bool CMLBotController::Enable(const char* model_name)
{
#ifdef RCBOT_WITH_ONNX
    if (!model_name || !*model_name)
    {
        Msg("[ML] Error: Invalid model name\n");
        return false;
    }

    // Get model from manager
    CONNXModelManager* pManager = CONNXModelManager::GetInstance();
    if (!pManager)
    {
        Msg("[ML] Error: ONNX model manager not initialized\n");
        return false;
    }

    m_pModel = pManager->GetModel(model_name);
    if (!m_pModel)
    {
        Msg("[ML] Error: Model '%s' not loaded. Use 'rcbot ml_model_load' first\n", model_name);
        return false;
    }

    // Create feature extractor if needed
    if (!m_pExtractor)
    {
        m_pExtractor = CFeatureExtractorFactory::CreateExtractor();
        if (!m_pExtractor)
        {
            Msg("[ML] Error: Failed to create feature extractor\n");
            return false;
        }
    }

    // Verify model input/output sizes
    size_t input_size = m_pModel->GetInputSize();
    size_t output_size = m_pModel->GetOutputSize();
    size_t expected_features = m_pExtractor->GetFeatureCount();

    if (input_size != expected_features)
    {
        Msg("[ML] Error: Model input size (%zu) doesn't match feature count (%zu)\n",
            input_size, expected_features);
        return false;
    }

    if (output_size != 9)
    {
        Msg("[ML] Warning: Model output size (%zu) is not 9. Unexpected behavior may occur\n",
            output_size);
    }

    // Enable ML control
    m_bEnabled = true;
    strncpy(m_szModelName, model_name, sizeof(m_szModelName) - 1);
    m_szModelName[sizeof(m_szModelName) - 1] = '\0';

    // Reset statistics
    m_fLastInferenceTime = 0.0f;
    m_fAvgInferenceTime = 0.0f;
    m_iInferenceCount = 0;

    Msg("[ML] Enabled ML control for bot '%s' using model '%s'\n",
        m_pBot->GetPlayerInfo() ? m_pBot->GetPlayerInfo()->GetName() : "unknown",
        model_name);

    return true;

#else
    Msg("[ML] Error: RCBot was compiled without ONNX support\n");
    return false;
#endif
}

void CMLBotController::Disable()
{
    if (m_bEnabled)
    {
        Msg("[ML] Disabled ML control for bot '%s'\n",
            m_pBot->GetPlayerInfo() ? m_pBot->GetPlayerInfo()->GetName() : "unknown");
    }

    m_bEnabled = false;
    m_pModel = nullptr;
    m_szModelName[0] = '\0';
}

bool CMLBotController::Update()
{
#ifdef RCBOT_WITH_ONNX
    if (!m_bEnabled || !m_pModel || !m_pExtractor)
        return false;

    if (!m_pBot || !m_pBot->InUse())
        return false;

    auto start_time = std::chrono::high_resolution_clock::now();

    // Extract features into reusable buffer
    if (!ExtractFeatures(m_FeaturesBuffer))
    {
        // Feature extraction failed - fallback to rule-based AI
        return false;
    }

    // Run inference into reusable buffer
    if (!RunInference(m_FeaturesBuffer, m_ActionsBuffer))
    {
        // Inference failed - fallback to rule-based AI
        return false;
    }

    // Apply action smoothing (in-place)
    SmoothActions(m_ActionsBuffer);

    // Apply actions to bot
    ApplyActions(m_ActionsBuffer);

    // Update performance statistics
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    m_fLastInferenceTime = duration.count() / 1000.0f;  // Convert to milliseconds

    if (m_iInferenceCount == 0)
        m_fAvgInferenceTime = m_fLastInferenceTime;
    else
        m_fAvgInferenceTime = (m_fAvgInferenceTime * m_iInferenceCount + m_fLastInferenceTime) / (m_iInferenceCount + 1);

    m_iInferenceCount++;

    // Swap buffers with last frame data (avoid copying)
    m_LastFeatures.swap(m_FeaturesBuffer);
    m_LastActions.swap(m_ActionsBuffer);

    return true;

#else
    return false;  // No ONNX support - always use rule-based AI
#endif
}

bool CMLBotController::ExtractFeatures(std::vector<float>& features)
{
    if (!m_pExtractor)
        return false;

    try
    {
        m_pExtractor->Extract(m_pBot, features);
        return features.size() == m_pExtractor->GetFeatureCount();
    }
    catch (...)
    {
        Msg("[ML] Exception during feature extraction\n");
        return false;
    }
}

bool CMLBotController::RunInference(const std::vector<float>& features, std::vector<float>& actions)
{
#ifdef RCBOT_WITH_ONNX
    if (!m_pModel)
        return false;

    try
    {
        return m_pModel->Inference(features, actions);
    }
    catch (...)
    {
        Msg("[ML] Exception during inference\n");
        return false;
    }
#else
    return false;
#endif
}

void CMLBotController::SmoothActions(std::vector<float>& actions)
{
    // Early exit if no smoothing or incompatible sizes
    if (m_fActionSmoothing <= 0.0f || m_LastActions.empty() || actions.size() != m_LastActions.size())
        return;

    // Cache smoothing factors to avoid repeated calculations
    const float alpha = m_fActionSmoothing;
    const float one_minus_alpha = 1.0f - alpha;
    const size_t count = actions.size();

    // Exponential moving average: smoothed = alpha * old + (1-alpha) * new
    for (size_t i = 0; i < count; ++i)
    {
        actions[i] = alpha * m_LastActions[i] + one_minus_alpha * actions[i];
    }
}

void CMLBotController::ApplyActions(const std::vector<float>& actions)
{
    if (actions.size() < 9)
    {
        // Invalid action vector
        return;
    }

    InterpretMovement(actions);
    InterpretAim(actions);
    InterpretButtons(actions);
}

void CMLBotController::InterpretMovement(const std::vector<float>& actions)
{
    // Actions[0-2]: forward/back, left/right, up/down
    // Range: -1 to 1
    // Scale to Source Engine movement speed (~450 units/sec max)

    const float MAX_MOVE_SPEED = 450.0f;

    float forward = actions[0] * MAX_MOVE_SPEED;
    float side = actions[1] * MAX_MOVE_SPEED;
    float up = actions[2] * MAX_MOVE_SPEED;

    // Set bot movement
    m_pBot->SetMovementVector(Vector(forward, side, up));
}

void CMLBotController::InterpretAim(const std::vector<float>& actions)
{
    // Actions[3-4]: yaw delta, pitch delta
    // Range: -1 to 1
    // Scale to reasonable turn speed (~90 degrees/frame max at 60 FPS = ~5400 deg/sec)

    const float MAX_TURN_SPEED = 90.0f;  // degrees per frame

    float yaw_delta = actions[3] * MAX_TURN_SPEED;
    float pitch_delta = actions[4] * MAX_TURN_SPEED;

    // Get current view angles
    QAngle current = m_pBot->GetViewAngles();

    // Apply deltas
    current.y += yaw_delta;    // Yaw
    current.x += pitch_delta;  // Pitch

    // Clamp pitch to valid range (-89 to 89 degrees)
    if (current.x > 89.0f)
        current.x = 89.0f;
    if (current.x < -89.0f)
        current.x = -89.0f;

    // Normalize yaw to 0-360
    while (current.y < 0.0f)
        current.y += 360.0f;
    while (current.y >= 360.0f)
        current.y -= 360.0f;

    // Set new view angles
    m_pBot->SetViewAngles(current);
}

void CMLBotController::InterpretButtons(const std::vector<float>& actions)
{
    // Actions[5-8]: attack, jump, crouch, reload
    // Range: 0 to 1 (treat as probabilities, threshold at 0.5)

    const float BUTTON_THRESHOLD = 0.5f;

    // Get current buttons
    CBotButtons* pButtons = m_pBot->GetButtons();
    if (!pButtons)
        return;

    // Clear previous frame buttons (ML model controls all buttons)
    pButtons->Reset();

    // Attack
    if (actions[5] > BUTTON_THRESHOLD)
    {
        pButtons->Tap(IN_ATTACK);
    }

    // Jump
    if (actions[6] > BUTTON_THRESHOLD)
    {
        pButtons->Tap(IN_JUMP);
    }

    // Crouch
    if (actions[7] > BUTTON_THRESHOLD)
    {
        pButtons->Hold(IN_DUCK);  // Crouch usually needs to be held
    }

    // Reload
    if (actions[8] > BUTTON_THRESHOLD)
    {
        pButtons->Tap(IN_RELOAD);
    }
}

// ============================================================================
// CMLBotManager Implementation
// ============================================================================

CMLBotManager* CMLBotManager::s_pInstance = nullptr;

CMLBotManager::CMLBotManager()
{
    m_Controllers.reserve(32);  // Pre-allocate for typical server size
}

CMLBotManager::~CMLBotManager()
{
    // Clean up all controllers
    for (size_t i = 0; i < m_Controllers.size(); ++i)
    {
        delete m_Controllers[i];
    }
    m_Controllers.clear();
}

CMLBotManager* CMLBotManager::GetInstance()
{
    if (!s_pInstance)
    {
        s_pInstance = new CMLBotManager();
    }
    return s_pInstance;
}

void CMLBotManager::Destroy()
{
    if (s_pInstance)
    {
        delete s_pInstance;
        s_pInstance = nullptr;
    }
}

bool CMLBotManager::EnableMLControl(CBot* pBot, const char* model_name)
{
    if (!pBot)
        return false;

    // Check if bot already has ML control
    int idx = FindControllerIndex(pBot);
    if (idx >= 0)
    {
        // Already has controller - just re-enable with new model
        return m_Controllers[idx]->Enable(model_name);
    }

    // Create new controller
    CMLBotController* pController = new CMLBotController(pBot);
    if (!pController->Enable(model_name))
    {
        delete pController;
        return false;
    }

    m_Controllers.push_back(pController);
    return true;
}

void CMLBotManager::DisableMLControl(CBot* pBot)
{
    if (!pBot)
        return;

    int idx = FindControllerIndex(pBot);
    if (idx >= 0)
    {
        m_Controllers[idx]->Disable();
        delete m_Controllers[idx];
        m_Controllers.erase(m_Controllers.begin() + idx);
    }
}

bool CMLBotManager::IsMLControlled(CBot* pBot)
{
    int idx = FindControllerIndex(pBot);
    if (idx >= 0)
    {
        return m_Controllers[idx]->IsEnabled();
    }
    return false;
}

CMLBotController* CMLBotManager::GetController(CBot* pBot)
{
    int idx = FindControllerIndex(pBot);
    if (idx >= 0)
    {
        return m_Controllers[idx];
    }
    return nullptr;
}

void CMLBotManager::UpdateAll()
{
    // Update all ML-controlled bots
    for (size_t i = 0; i < m_Controllers.size(); ++i)
    {
        if (m_Controllers[i]->IsEnabled())
        {
            // Update returns false if inference fails (falls back to rule-based AI)
            m_Controllers[i]->Update();
        }
    }
}

void CMLBotManager::GetStatistics(int& total_bots, int& ml_bots, float& avg_inference_time)
{
    total_bots = static_cast<int>(m_Controllers.size());
    ml_bots = 0;
    float total_time = 0.0f;

    for (size_t i = 0; i < m_Controllers.size(); ++i)
    {
        if (m_Controllers[i]->IsEnabled())
        {
            ml_bots++;
            total_time += m_Controllers[i]->GetAverageInferenceTime();
        }
    }

    avg_inference_time = (ml_bots > 0) ? (total_time / ml_bots) : 0.0f;
}

int CMLBotManager::FindControllerIndex(CBot* pBot)
{
    for (size_t i = 0; i < m_Controllers.size(); ++i)
    {
        // Compare bot pointers (assumes bot addresses are stable)
        // Alternative: compare by bot index if available
        if (m_Controllers[i] && m_Controllers[i]->GetBot() == pBot)
        {
            return static_cast<int>(i);
        }
    }
    return -1;
}
