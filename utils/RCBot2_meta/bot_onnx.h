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

#ifndef __BOT_ONNX_H__
#define __BOT_ONNX_H__

#include <vector>
#include <string>
#include <memory>

// Forward declarations for ONNX Runtime
// This allows compilation without ONNX Runtime installed
#ifdef RCBOT_WITH_ONNX
namespace Ort {
    class Env;
    class Session;
    class SessionOptions;
    class MemoryInfo;
    class Value;
}
#endif

/**
 * CONNXModel - Wrapper class for ONNX Runtime inference
 *
 * This class provides a clean interface for loading and running ONNX models
 * in RCBot2. It handles model loading, input/output tensor management, and
 * inference execution.
 *
 * Usage:
 *   CONNXModel model;
 *   if (model.LoadModel("models/behavior_clone.onnx")) {
 *       std::vector<float> input(96, 0.5f);
 *       std::vector<float> output;
 *       if (model.Inference(input, output)) {
 *           // Use output predictions
 *       }
 *   }
 *
 * Performance:
 *   - Target: <0.5ms inference time for HL2DM (48-64 input features)
 *   - Target: <1.0ms inference time for TF2 (96 input features)
 *   - Single-threaded execution (game thread)
 *   - Optimized for CPU inference
 */
class CONNXModel {
public:
    CONNXModel();
    ~CONNXModel();

    // Disable copy (ONNX runtime objects are not copyable)
    CONNXModel(const CONNXModel&) = delete;
    CONNXModel& operator=(const CONNXModel&) = delete;

    // Load model from file
    // Returns true on success, false on failure
    bool LoadModel(const char* model_path);

    // Unload current model
    void UnloadModel();

    // Run inference on input features
    // input: flat array of input features (must match model input size)
    // output: will be resized and filled with predictions
    // Returns true on success, false on failure
    bool Inference(const std::vector<float>& input, std::vector<float>& output);

    // Benchmark inference performance
    // Runs inference N times and returns average time in microseconds
    float BenchmarkInference(const std::vector<float>& input, int iterations = 1000);

    // Get model metadata
    size_t GetInputSize() const { return m_inputSize; }
    size_t GetOutputSize() const { return m_outputSize; }
    bool IsLoaded() const { return m_isLoaded; }
    const char* GetModelPath() const { return m_modelPath.c_str(); }

    // Get model info as string (for debugging)
    std::string GetModelInfo() const;

    // Performance statistics
    float GetLastInferenceTimeMs() const { return m_lastInferenceTimeMs; }
    int GetTotalInferences() const { return m_totalInferences; }
    float GetAverageInferenceTimeMs() const;

private:
    // ONNX Runtime objects (only allocated if ONNX is enabled)
#ifdef RCBOT_WITH_ONNX
    std::unique_ptr<Ort::Env> m_env;
    std::unique_ptr<Ort::Session> m_session;
    std::unique_ptr<Ort::SessionOptions> m_sessionOptions;
    std::unique_ptr<Ort::MemoryInfo> m_memoryInfo;  // Cached memory info for tensors
#endif

    // Model metadata
    size_t m_inputSize;
    size_t m_outputSize;
    bool m_isLoaded;
    std::string m_modelPath;

    // Input/Output names (ONNX requires const char* arrays)
    std::vector<std::string> m_inputNameStrings;
    std::vector<std::string> m_outputNameStrings;
    std::vector<const char*> m_inputNames;
    std::vector<const char*> m_outputNames;

    // Performance tracking
    float m_lastInferenceTimeMs;
    int m_totalInferences;
    float m_totalInferenceTimeMs;

    // Cached tensors and buffers for inference (reduce allocations)
#ifdef RCBOT_WITH_ONNX
    std::vector<int64_t> m_inputShape;   // Cached input shape
    std::vector<int64_t> m_outputShape;  // Cached output shape
#endif

    // Helper methods
    bool InitializeONNXRuntime();
    void CleanupONNXRuntime();
};

/**
 * Singleton accessor for a global test model
 * This is useful for quick testing without managing model lifecycle
 */
class CONNXModelManager {
public:
    static CONNXModelManager* GetInstance();

    // Load a model into a named slot
    bool LoadModel(const char* name, const char* path);

    // Get a model by name
    CONNXModel* GetModel(const char* name);

    // Unload a model
    void UnloadModel(const char* name);

    // Unload all models
    void UnloadAllModels();

    // List loaded models
    void ListModels();

private:
    CONNXModelManager() = default;
    ~CONNXModelManager() = default;
    CONNXModelManager(const CONNXModelManager&) = delete;
    CONNXModelManager& operator=(const CONNXModelManager&) = delete;

    static CONNXModelManager* s_instance;
    std::vector<std::pair<std::string, std::unique_ptr<CONNXModel>>> m_models;
};

#endif // __BOT_ONNX_H__
