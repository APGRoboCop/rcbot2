/**
 * bot_ml_onnx.h
 *
 * ONNX Runtime integration for RCBot2
 * Provides C++ wrapper for loading and running ONNX models
 *
 * Part of Phase 0: Foundation and Infrastructure
 */

#ifndef __BOT_ML_ONNX_H__
#define __BOT_ML_ONNX_H__

#include <vector>
#include <string>
#include <memory>

// Forward declarations (actual ONNX headers included in .cpp)
namespace Ort {
    class Env;
    class Session;
    class SessionOptions;
}

/**
 * ONNX Model Wrapper
 *
 * Handles loading ONNX models and running inference.
 * Thread-safe for single bot, but each bot should have its own instance
 * if running multiple bots with different models.
 */
class CONNXModel {
public:
    CONNXModel();
    ~CONNXModel();

    // Model loading
    bool LoadModel(const char* model_path);
    bool IsLoaded() const { return m_bLoaded; }
    void UnloadModel();

    // Model information
    size_t GetInputSize() const { return m_inputSize; }
    size_t GetOutputSize() const { return m_outputSize; }
    const char* GetModelPath() const { return m_modelPath.c_str(); }

    // Inference
    bool Inference(const std::vector<float>& input, std::vector<float>& output);

    // Performance metrics
    float GetLastInferenceTimeMs() const { return m_lastInferenceMs; }
    float GetAverageInferenceTimeMs() const;
    size_t GetInferenceCount() const { return m_inferenceCount; }

    // Error handling
    const char* GetLastError() const { return m_lastError.c_str(); }

private:
    bool m_bLoaded;
    std::string m_modelPath;
    std::string m_lastError;

    // ONNX Runtime objects (opaque pointers to avoid header dependencies)
    std::unique_ptr<Ort::Env> m_env;
    std::unique_ptr<Ort::Session> m_session;
    std::unique_ptr<Ort::SessionOptions> m_sessionOptions;

    // Model dimensions
    size_t m_inputSize;
    size_t m_outputSize;

    // Performance tracking
    float m_lastInferenceMs;
    size_t m_inferenceCount;
    float m_totalInferenceMs;

    // Validation
    bool ValidateInput(const std::vector<float>& input) const;
    bool ValidateOutput(const std::vector<float>& output) const;
};

/**
 * Model Manager (Singleton)
 *
 * Manages multiple ONNX models and handles model selection
 * based on configuration (game, class, game mode, etc.)
 */
class CModelManager {
public:
    static CModelManager* GetInstance();

    // Model management
    bool LoadModelsFromConfig(const char* config_path);
    CONNXModel* GetModel(const char* model_name);
    CONNXModel* GetActiveModel() { return m_pActiveModel; }
    bool SetActiveModel(const char* model_name);

    // Auto model selection
    CONNXModel* SelectModelForBot(class CBot* pBot);

    // Performance monitoring
    void PrintPerformanceStats();
    void ResetPerformanceStats();

private:
    CModelManager();
    ~CModelManager();

    std::map<std::string, CONNXModel*> m_models;
    CONNXModel* m_pActiveModel;

    static CModelManager* s_pInstance;
};

#endif // __BOT_ML_ONNX_H__
