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

#include "bot_onnx.h"
#include "eiface.h"

#include <chrono>
#include <cstring>

// Only include ONNX Runtime headers if enabled
#ifdef RCBOT_WITH_ONNX
#include <onnxruntime/core/session/onnxruntime_cxx_api.h>
#endif

extern IVEngineServer* engine;

// ============================================================================
// CONNXModel Implementation
// ============================================================================

CONNXModel::CONNXModel()
    : m_inputSize(0)
    , m_outputSize(0)
    , m_isLoaded(false)
    , m_lastInferenceTimeMs(0.0f)
    , m_totalInferences(0)
    , m_totalInferenceTimeMs(0.0f)
{
#ifdef RCBOT_WITH_ONNX
    InitializeONNXRuntime();
#endif
}

CONNXModel::~CONNXModel()
{
    UnloadModel();
#ifdef RCBOT_WITH_ONNX
    CleanupONNXRuntime();
#endif
}

bool CONNXModel::InitializeONNXRuntime()
{
#ifdef RCBOT_WITH_ONNX
    try {
        // Initialize ONNX Runtime environment
        m_env = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "RCBot2ML");

        // Create session options
        m_sessionOptions = std::make_unique<Ort::SessionOptions>();

        // Configure for game thread (single-threaded, low latency)
        m_sessionOptions->SetIntraOpNumThreads(1);  // Single thread
        m_sessionOptions->SetGraphOptimizationLevel(
            GraphOptimizationLevel::ORT_ENABLE_ALL);  // Maximum optimization

        // Disable telemetry
        m_sessionOptions->DisableProfiling();

        // Create cached memory info object (reused for all tensor creations)
        m_memoryInfo = std::make_unique<Ort::MemoryInfo>(
            Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault)
        );

        Msg("[RCBot2 ML] ONNX Runtime initialized\n");
        return true;
    } catch (const Ort::Exception& e) {
        Warning("[RCBot2 ML] Failed to initialize ONNX Runtime: %s\n", e.what());
        return false;
    }
#else
    Warning("[RCBot2 ML] ONNX Runtime support not compiled in. Rebuild with RCBOT_WITH_ONNX defined.\n");
    return false;
#endif
}

void CONNXModel::CleanupONNXRuntime()
{
#ifdef RCBOT_WITH_ONNX
    m_session.reset();
    m_sessionOptions.reset();
    m_memoryInfo.reset();
    m_env.reset();
#endif
}

bool CONNXModel::LoadModel(const char* model_path)
{
#ifndef RCBOT_WITH_ONNX
    Warning("[RCBot2 ML] ONNX Runtime not available. Cannot load model: %s\n", model_path);
    return false;
#else
    if (!model_path || !*model_path) {
        Warning("[RCBot2 ML] Invalid model path\n");
        return false;
    }

    // Unload any existing model
    UnloadModel();

    try {
        // Load ONNX model
        m_session = std::make_unique<Ort::Session>(*m_env, model_path, *m_sessionOptions);

        Ort::AllocatorWithDefaultOptions allocator;

        // Get input metadata
        size_t num_input_nodes = m_session->GetInputCount();
        if (num_input_nodes != 1) {
            Warning("[RCBot2 ML] Model has %zu inputs, expected 1\n", num_input_nodes);
            UnloadModel();
            return false;
        }

        // Get input name
        {
            auto input_name = m_session->GetInputNameAllocated(0, allocator);
            m_inputNameStrings.push_back(std::string(input_name.get()));
            m_inputNames.push_back(m_inputNameStrings.back().c_str());
        }

        // Get input dimensions
        Ort::TypeInfo input_type_info = m_session->GetInputTypeInfo(0);
        auto input_tensor_info = input_type_info.GetTensorTypeAndShapeInfo();
        auto input_dims = input_tensor_info.GetShape();

        // Assume [batch_size, features] or [features]
        if (input_dims.size() == 2) {
            m_inputSize = static_cast<size_t>(input_dims[1]);
            // Cache shape for inference (batch size = 1)
            m_inputShape = {1, static_cast<int64_t>(m_inputSize)};
        } else if (input_dims.size() == 1) {
            m_inputSize = static_cast<size_t>(input_dims[0]);
            m_inputShape = {static_cast<int64_t>(m_inputSize)};
        } else {
            Warning("[RCBot2 ML] Unexpected input dimensions: %zu\n", input_dims.size());
            UnloadModel();
            return false;
        }

        // Get output metadata
        size_t num_output_nodes = m_session->GetOutputCount();
        if (num_output_nodes != 1) {
            Warning("[RCBot2 ML] Model has %zu outputs, expected 1\n", num_output_nodes);
            UnloadModel();
            return false;
        }

        // Get output name
        {
            auto output_name = m_session->GetOutputNameAllocated(0, allocator);
            m_outputNameStrings.push_back(std::string(output_name.get()));
            m_outputNames.push_back(m_outputNameStrings.back().c_str());
        }

        // Get output dimensions
        Ort::TypeInfo output_type_info = m_session->GetOutputTypeInfo(0);
        auto output_tensor_info = output_type_info.GetTensorTypeAndShapeInfo();
        auto output_dims = output_tensor_info.GetShape();

        // Assume [batch_size, outputs] or [outputs]
        if (output_dims.size() == 2) {
            m_outputSize = static_cast<size_t>(output_dims[1]);
            // Cache shape for inference (batch size = 1)
            m_outputShape = {1, static_cast<int64_t>(m_outputSize)};
        } else if (output_dims.size() == 1) {
            m_outputSize = static_cast<size_t>(output_dims[0]);
            m_outputShape = {static_cast<int64_t>(m_outputSize)};
        } else {
            Warning("[RCBot2 ML] Unexpected output dimensions: %zu\n", output_dims.size());
            UnloadModel();
            return false;
        }

        m_modelPath = model_path;
        m_isLoaded = true;

        Msg("[RCBot2 ML] Model loaded: %s\n", model_path);
        Msg("[RCBot2 ML]   Input: %zu features\n", m_inputSize);
        Msg("[RCBot2 ML]   Output: %zu values\n", m_outputSize);

        return true;

    } catch (const Ort::Exception& e) {
        Warning("[RCBot2 ML] Failed to load ONNX model: %s\n", e.what());
        UnloadModel();
        return false;
    }
#endif
}

void CONNXModel::UnloadModel()
{
#ifdef RCBOT_WITH_ONNX
    m_session.reset();
    m_inputShape.clear();
    m_outputShape.clear();
#endif
    m_inputSize = 0;
    m_outputSize = 0;
    m_isLoaded = false;
    m_modelPath.clear();
    m_inputNameStrings.clear();
    m_outputNameStrings.clear();
    m_inputNames.clear();
    m_outputNames.clear();
    m_lastInferenceTimeMs = 0.0f;
    m_totalInferences = 0;
    m_totalInferenceTimeMs = 0.0f;
}

bool CONNXModel::Inference(const std::vector<float>& input, std::vector<float>& output)
{
#ifndef RCBOT_WITH_ONNX
    Warning("[RCBot2 ML] ONNX Runtime not available\n");
    return false;
#else
    if (!m_isLoaded || !m_session) {
        Warning("[RCBot2 ML] No model loaded\n");
        return false;
    }

    if (input.size() != m_inputSize) {
        Warning("[RCBot2 ML] Input size mismatch: got %zu, expected %zu\n",
                input.size(), m_inputSize);
        return false;
    }

    auto start = std::chrono::high_resolution_clock::now();

    try {
        // Create input tensor using cached memory info and shape
        Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
            *m_memoryInfo,
            const_cast<float*>(input.data()),
            input.size(),
            m_inputShape.data(),
            m_inputShape.size()
        );

        // Run inference
        auto output_tensors = m_session->Run(
            Ort::RunOptions{nullptr},
            m_inputNames.data(),
            &input_tensor,
            1,
            m_outputNames.data(),
            1
        );

        // Extract output - pre-allocate if needed
        if (output.size() != m_outputSize) {
            output.resize(m_outputSize);
        }
        float* output_data = output_tensors[0].GetTensorMutableData<float>();
        std::memcpy(output.data(), output_data, m_outputSize * sizeof(float));

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        m_lastInferenceTimeMs = duration.count() / 1000.0f;
        m_totalInferences++;
        m_totalInferenceTimeMs += m_lastInferenceTimeMs;

        return true;

    } catch (const Ort::Exception& e) {
        Warning("[RCBot2 ML] Inference failed: %s\n", e.what());
        return false;
    }
#endif
}

float CONNXModel::BenchmarkInference(const std::vector<float>& input, int iterations)
{
#ifndef RCBOT_WITH_ONNX
    return -1.0f;
#else
    if (!m_isLoaded || input.size() != m_inputSize) {
        return -1.0f;
    }

    std::vector<float> output;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; i++) {
        if (!Inference(input, output)) {
            return -1.0f;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto total_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    return (total_duration.count() / static_cast<float>(iterations)) / 1000.0f;  // Return ms
#endif
}

std::string CONNXModel::GetModelInfo() const
{
    char buffer[512];
    snprintf(buffer, sizeof(buffer),
        "Model: %s\n"
        "  Loaded: %s\n"
        "  Input size: %zu\n"
        "  Output size: %zu\n"
        "  Total inferences: %d\n"
        "  Last inference: %.3f ms\n"
        "  Average inference: %.3f ms\n",
        m_modelPath.c_str(),
        m_isLoaded ? "YES" : "NO",
        m_inputSize,
        m_outputSize,
        m_totalInferences,
        m_lastInferenceTimeMs,
        GetAverageInferenceTimeMs()
    );
    return std::string(buffer);
}

float CONNXModel::GetAverageInferenceTimeMs() const
{
    if (m_totalInferences == 0) {
        return 0.0f;
    }
    return m_totalInferenceTimeMs / static_cast<float>(m_totalInferences);
}

// ============================================================================
// CONNXModelManager Implementation
// ============================================================================

CONNXModelManager* CONNXModelManager::s_instance = nullptr;

CONNXModelManager* CONNXModelManager::GetInstance()
{
    if (!s_instance) {
        s_instance = new CONNXModelManager();
    }
    return s_instance;
}

bool CONNXModelManager::LoadModel(const char* name, const char* path)
{
    if (!name || !*name || !path || !*path) {
        Warning("[RCBot2 ML] Invalid model name or path\n");
        return false;
    }

    // Check if model already exists
    for (auto& pair : m_models) {
        if (pair.first == name) {
            Warning("[RCBot2 ML] Model '%s' already loaded. Unload it first.\n", name);
            return false;
        }
    }

    // Create and load new model
    auto model = std::make_unique<CONNXModel>();
    if (!model->LoadModel(path)) {
        return false;
    }

    m_models.emplace_back(std::string(name), std::move(model));
    Msg("[RCBot2 ML] Model '%s' loaded into manager\n", name);
    return true;
}

CONNXModel* CONNXModelManager::GetModel(const char* name)
{
    if (!name || !*name) {
        return nullptr;
    }

    for (auto& pair : m_models) {
        if (pair.first == name) {
            return pair.second.get();
        }
    }

    return nullptr;
}

void CONNXModelManager::UnloadModel(const char* name)
{
    if (!name || !*name) {
        return;
    }

    for (auto it = m_models.begin(); it != m_models.end(); ++it) {
        if (it->first == name) {
            Msg("[RCBot2 ML] Unloading model '%s'\n", name);
            m_models.erase(it);
            return;
        }
    }

    Warning("[RCBot2 ML] Model '%s' not found\n", name);
}

void CONNXModelManager::UnloadAllModels()
{
    Msg("[RCBot2 ML] Unloading all models (%zu total)\n", m_models.size());
    m_models.clear();
}

void CONNXModelManager::ListModels()
{
    if (m_models.empty()) {
        Msg("[RCBot2 ML] No models loaded\n");
        return;
    }

    Msg("[RCBot2 ML] Loaded models (%zu):\n", m_models.size());
    for (const auto& pair : m_models) {
        const CONNXModel* model = pair.second.get();
        Msg("  [%s] %s (in=%zu, out=%zu, avg=%.3fms)\n",
            pair.first.c_str(),
            model->GetModelPath(),
            model->GetInputSize(),
            model->GetOutputSize(),
            model->GetAverageInferenceTimeMs());
    }
}
