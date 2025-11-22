// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
/*
 *    This file is part of RCBot.
 *
 *    Machine Learning commands for RCBot2
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

#include "bot_commands.h"
#include "bot_recorder.h"
#include "bot_onnx.h"
#include "bot_features.h"
#include "bot_globals.h"
#include "bot.h"

#include <sstream>
#include <iomanip>

// ML Recording Commands

CBotCommandInline MLRecordStartCommand("ml_record_start", CMD_ACCESS_BOT | CMD_ACCESS_DEDICATED,
    [](const CClient* pClient, const BotCommandArgs& args)
    {
        edict_t* pEntity = nullptr;
        if (pClient)
            pEntity = pClient->getPlayer();

        CBotRecorder::GetInstance()->StartRecording();
        CBotGlobals::botMessage(pEntity, 0, "[ML] Recording started");
        return COMMAND_ACCESSED;
    });

CBotCommandInline MLRecordStopCommand("ml_record_stop", CMD_ACCESS_BOT | CMD_ACCESS_DEDICATED,
    [](const CClient* pClient, const BotCommandArgs& args)
    {
        edict_t* pEntity = nullptr;
        if (pClient)
            pEntity = pClient->getPlayer();

        CBotRecorder::GetInstance()->StopRecording();
        int frames = CBotRecorder::GetInstance()->GetFrameCount();
        float duration = CBotRecorder::GetInstance()->GetRecordingDuration();

        CBotGlobals::botMessage(pEntity, 0, "[ML] Recording stopped. Frames: %d, Duration: %.1fs",
            frames, duration);
        return COMMAND_ACCESSED;
    });

CBotCommandInline MLRecordPauseCommand("ml_record_pause", CMD_ACCESS_BOT | CMD_ACCESS_DEDICATED,
    [](const CClient* pClient, const BotCommandArgs& args)
    {
        edict_t* pEntity = nullptr;
        if (pClient)
            pEntity = pClient->getPlayer();

        CBotRecorder::GetInstance()->PauseRecording();
        CBotGlobals::botMessage(pEntity, 0, "[ML] Recording paused");
        return COMMAND_ACCESSED;
    });

CBotCommandInline MLRecordResumeCommand("ml_record_resume", CMD_ACCESS_BOT | CMD_ACCESS_DEDICATED,
    [](const CClient* pClient, const BotCommandArgs& args)
    {
        edict_t* pEntity = nullptr;
        if (pClient)
            pEntity = pClient->getPlayer();

        CBotRecorder::GetInstance()->ResumeRecording();
        CBotGlobals::botMessage(pEntity, 0, "[ML] Recording resumed");
        return COMMAND_ACCESSED;
    });

CBotCommandInline MLRecordSaveCommand("ml_record_save", CMD_ACCESS_BOT | CMD_ACCESS_DEDICATED,
    [](const CClient* pClient, const BotCommandArgs& args)
    {
        edict_t* pEntity = nullptr;
        if (pClient)
            pEntity = pClient->getPlayer();

        if (!args[0] || !*args[0]) {
            CBotGlobals::botMessage(pEntity, 0, "[ML] Usage: rcbot ml_record_save <filename>");
            return COMMAND_ERROR;
        }

        char filepath[512];
        snprintf(filepath, sizeof(filepath), "rcbot2/data/ml/%s.rcbr", args[0]);

        if (CBotRecorder::GetInstance()->SaveRecording(filepath)) {
            CBotGlobals::botMessage(pEntity, 0, "[ML] Recording saved to %s", filepath);
            return COMMAND_ACCESSED;
        } else {
            CBotGlobals::botMessage(pEntity, 0, "[ML] Failed to save recording");
            return COMMAND_ERROR;
        }
    });

CBotCommandInline MLRecordExportJSONCommand("ml_record_export_json", CMD_ACCESS_BOT | CMD_ACCESS_DEDICATED,
    [](const CClient* pClient, const BotCommandArgs& args)
    {
        edict_t* pEntity = nullptr;
        if (pClient)
            pEntity = pClient->getPlayer();

        if (!args[0] || !*args[0]) {
            CBotGlobals::botMessage(pEntity, 0, "[ML] Usage: rcbot ml_record_export_json <filename>");
            return COMMAND_ERROR;
        }

        char filepath[512];
        snprintf(filepath, sizeof(filepath), "rcbot2/data/ml/%s.json", args[0]);

        if (CBotRecorder::GetInstance()->ExportToJSON(filepath)) {
            CBotGlobals::botMessage(pEntity, 0, "[ML] Recording exported to JSON: %s", filepath);
            return COMMAND_ACCESSED;
        } else {
            CBotGlobals::botMessage(pEntity, 0, "[ML] Failed to export recording");
            return COMMAND_ERROR;
        }
    });

CBotCommandInline MLRecordExportCSVCommand("ml_record_export_csv", CMD_ACCESS_BOT | CMD_ACCESS_DEDICATED,
    [](const CClient* pClient, const BotCommandArgs& args)
    {
        edict_t* pEntity = nullptr;
        if (pClient)
            pEntity = pClient->getPlayer();

        if (!args[0] || !*args[0]) {
            CBotGlobals::botMessage(pEntity, 0, "[ML] Usage: rcbot ml_record_export_csv <filename>");
            return COMMAND_ERROR;
        }

        char filepath[512];
        snprintf(filepath, sizeof(filepath), "rcbot2/data/ml/%s.csv", args[0]);

        if (CBotRecorder::GetInstance()->ExportToCSV(filepath)) {
            CBotGlobals::botMessage(pEntity, 0, "[ML] Recording exported to CSV: %s", filepath);
            return COMMAND_ACCESSED;
        } else {
            CBotGlobals::botMessage(pEntity, 0, "[ML] Failed to export recording");
            return COMMAND_ERROR;
        }
    });

CBotCommandInline MLRecordStatusCommand("ml_record_status", CMD_ACCESS_BOT | CMD_ACCESS_DEDICATED,
    [](const CClient* pClient, const BotCommandArgs& args)
    {
        edict_t* pEntity = nullptr;
        if (pClient)
            pEntity = pClient->getPlayer();

        CBotRecorder* recorder = CBotRecorder::GetInstance();

        if (recorder->IsRecording()) {
            CBotGlobals::botMessage(pEntity, 0, "[ML] Recording: YES (frames: %d, duration: %.1fs)",
                recorder->GetFrameCount(),
                recorder->GetRecordingDuration());
        } else if (recorder->IsPaused()) {
            CBotGlobals::botMessage(pEntity, 0, "[ML] Recording: PAUSED (frames: %d)",
                recorder->GetFrameCount());
        } else {
            CBotGlobals::botMessage(pEntity, 0, "[ML] Recording: NO (frames in buffer: %d)",
                recorder->GetFrameCount());
        }

        return COMMAND_ACCESSED;
    });

CBotCommandInline MLRecordClearCommand("ml_record_clear", CMD_ACCESS_BOT | CMD_ACCESS_DEDICATED,
    [](const CClient* pClient, const BotCommandArgs& args)
    {
        edict_t* pEntity = nullptr;
        if (pClient)
            pEntity = pClient->getPlayer();

        CBotRecorder::GetInstance()->ClearRecording();
        CBotGlobals::botMessage(pEntity, 0, "[ML] Recording buffer cleared");
        return COMMAND_ACCESSED;
    });

// ML Model Management Commands

CBotCommandInline MLModelLoadCommand("ml_model_load", CMD_ACCESS_BOT | CMD_ACCESS_DEDICATED,
    [](const CClient* pClient, const BotCommandArgs& args)
    {
        edict_t* pEntity = nullptr;
        if (pClient)
            pEntity = pClient->getPlayer();

        if (!args[0] || !*args[0] || !args[1] || !*args[1]) {
            CBotGlobals::botMessage(pEntity, 0, "[ML] Usage: rcbot ml_model_load <name> <filepath>");
            CBotGlobals::botMessage(pEntity, 0, "[ML] Example: rcbot ml_model_load test models/test_model.onnx");
            return COMMAND_ERROR;
        }

        const char* name = args[0];
        const char* path = args[1];

        if (CONNXModelManager::GetInstance()->LoadModel(name, path)) {
            CONNXModel* model = CONNXModelManager::GetInstance()->GetModel(name);
            if (model) {
                CBotGlobals::botMessage(pEntity, 0, "[ML] Model '%s' loaded successfully", name);
                CBotGlobals::botMessage(pEntity, 0, "[ML]   Input: %zu features", model->GetInputSize());
                CBotGlobals::botMessage(pEntity, 0, "[ML]   Output: %zu values", model->GetOutputSize());
            }
            return COMMAND_ACCESSED;
        } else {
            CBotGlobals::botMessage(pEntity, 0, "[ML] Failed to load model '%s'", name);
            return COMMAND_ERROR;
        }
    });

CBotCommandInline MLModelUnloadCommand("ml_model_unload", CMD_ACCESS_BOT | CMD_ACCESS_DEDICATED,
    [](const CClient* pClient, const BotCommandArgs& args)
    {
        edict_t* pEntity = nullptr;
        if (pClient)
            pEntity = pClient->getPlayer();

        if (!args[0] || !*args[0]) {
            CBotGlobals::botMessage(pEntity, 0, "[ML] Usage: rcbot ml_model_unload <name>");
            return COMMAND_ERROR;
        }

        CONNXModelManager::GetInstance()->UnloadModel(args[0]);
        CBotGlobals::botMessage(pEntity, 0, "[ML] Model '%s' unloaded", args[0]);
        return COMMAND_ACCESSED;
    });

CBotCommandInline MLModelListCommand("ml_model_list", CMD_ACCESS_BOT | CMD_ACCESS_DEDICATED,
    [](const CClient* pClient, const BotCommandArgs& args)
    {
        edict_t* pEntity = nullptr;
        if (pClient)
            pEntity = pClient->getPlayer();

        CONNXModelManager::GetInstance()->ListModels();
        return COMMAND_ACCESSED;
    });

CBotCommandInline MLModelTestCommand("ml_model_test", CMD_ACCESS_BOT | CMD_ACCESS_DEDICATED,
    [](const CClient* pClient, const BotCommandArgs& args)
    {
        edict_t* pEntity = nullptr;
        if (pClient)
            pEntity = pClient->getPlayer();

        if (!args[0] || !*args[0]) {
            CBotGlobals::botMessage(pEntity, 0, "[ML] Usage: rcbot ml_model_test <name>");
            return COMMAND_ERROR;
        }

        CONNXModel* model = CONNXModelManager::GetInstance()->GetModel(args[0]);
        if (!model) {
            CBotGlobals::botMessage(pEntity, 0, "[ML] Model '%s' not found", args[0]);
            return COMMAND_ERROR;
        }

        // Create dummy input
        std::vector<float> input(model->GetInputSize(), 0.5f);
        std::vector<float> output;

        // Run inference
        if (model->Inference(input, output)) {
            CBotGlobals::botMessage(pEntity, 0, "[ML] Inference successful!");
            CBotGlobals::botMessage(pEntity, 0, "[ML]   Time: %.3f ms", model->GetLastInferenceTimeMs());
            CBotGlobals::botMessage(pEntity, 0, "[ML]   First 5 outputs: %.3f, %.3f, %.3f, %.3f, %.3f",
                output.size() > 0 ? output[0] : 0.0f,
                output.size() > 1 ? output[1] : 0.0f,
                output.size() > 2 ? output[2] : 0.0f,
                output.size() > 3 ? output[3] : 0.0f,
                output.size() > 4 ? output[4] : 0.0f);
            return COMMAND_ACCESSED;
        } else {
            CBotGlobals::botMessage(pEntity, 0, "[ML] Inference failed");
            return COMMAND_ERROR;
        }
    });

CBotCommandInline MLModelBenchmarkCommand("ml_model_benchmark", CMD_ACCESS_BOT | CMD_ACCESS_DEDICATED,
    [](const CClient* pClient, const BotCommandArgs& args)
    {
        edict_t* pEntity = nullptr;
        if (pClient)
            pEntity = pClient->getPlayer();

        if (!args[0] || !*args[0]) {
            CBotGlobals::botMessage(pEntity, 0, "[ML] Usage: rcbot ml_model_benchmark <name> [iterations]");
            return COMMAND_ERROR;
        }

        CONNXModel* model = CONNXModelManager::GetInstance()->GetModel(args[0]);
        if (!model) {
            CBotGlobals::botMessage(pEntity, 0, "[ML] Model '%s' not found", args[0]);
            return COMMAND_ERROR;
        }

        int iterations = 1000;
        if (args[1] && *args[1]) {
            iterations = atoi(args[1]);
            if (iterations <= 0 || iterations > 10000) {
                CBotGlobals::botMessage(pEntity, 0, "[ML] Invalid iterations (1-10000)");
                return COMMAND_ERROR;
            }
        }

        CBotGlobals::botMessage(pEntity, 0, "[ML] Benchmarking model '%s' (%d iterations)...", args[0], iterations);

        // Create dummy input
        std::vector<float> input(model->GetInputSize(), 0.5f);

        // Run benchmark
        float avg_time = model->BenchmarkInference(input, iterations);

        if (avg_time >= 0.0f) {
            CBotGlobals::botMessage(pEntity, 0, "[ML] Benchmark complete!");
            CBotGlobals::botMessage(pEntity, 0, "[ML]   Average time: %.3f ms", avg_time);
            CBotGlobals::botMessage(pEntity, 0, "[ML]   FPS limit: %.1f", 1000.0f / avg_time);

            // Performance assessment
            if (avg_time < 0.5f) {
                CBotGlobals::botMessage(pEntity, 0, "[ML]   Performance: EXCELLENT (HL2DM target met)");
            } else if (avg_time < 1.0f) {
                CBotGlobals::botMessage(pEntity, 0, "[ML]   Performance: GOOD (TF2 target met)");
            } else if (avg_time < 2.0f) {
                CBotGlobals::botMessage(pEntity, 0, "[ML]   Performance: ACCEPTABLE");
            } else {
                CBotGlobals::botMessage(pEntity, 0, "[ML]   Performance: TOO SLOW (optimize model)");
            }

            return COMMAND_ACCESSED;
        } else {
            CBotGlobals::botMessage(pEntity, 0, "[ML] Benchmark failed");
            return COMMAND_ERROR;
        }
    });

CBotCommandInline MLModelInfoCommand("ml_model_info", CMD_ACCESS_BOT | CMD_ACCESS_DEDICATED,
    [](const CClient* pClient, const BotCommandArgs& args)
    {
        edict_t* pEntity = nullptr;
        if (pClient)
            pEntity = pClient->getPlayer();

        if (!args[0] || !*args[0]) {
            CBotGlobals::botMessage(pEntity, 0, "[ML] Usage: rcbot ml_model_info <name>");
            return COMMAND_ERROR;
        }

        CONNXModel* model = CONNXModelManager::GetInstance()->GetModel(args[0]);
        if (!model) {
            CBotGlobals::botMessage(pEntity, 0, "[ML] Model '%s' not found", args[0]);
            return COMMAND_ERROR;
        }

        std::string info = model->GetModelInfo();
        std::istringstream iss(info);
        std::string line;
        while (std::getline(iss, line)) {
            CBotGlobals::botMessage(pEntity, 0, "[ML] %s", line.c_str());
        }

        return COMMAND_ACCESSED;
    });

// ML Feature Extraction Commands

CBotCommandInline MLFeaturesDumpCommand("ml_features_dump", CMD_ACCESS_BOT | CMD_ACCESS_DEDICATED,
    [](const CClient* pClient, const BotCommandArgs& args)
    {
        edict_t* pEntity = nullptr;
        if (pClient)
            pEntity = pClient->getPlayer();

        // Get target bot index (default: first bot)
        int botIndex = 0;
        if (args[0] && *args[0]) {
            botIndex = atoi(args[0]);
        }

        CBot* pBot = CBots::GetBotPointer(botIndex);
        if (!pBot || !pBot->inUse()) {
            CBotGlobals::botMessage(pEntity, 0, "[ML] Bot %d not found or not in use", botIndex);
            return COMMAND_ERROR;
        }

        // Create feature extractor
        CFeatureExtractor* pExtractor = CFeatureExtractorFactory::CreateExtractor();
        if (!pExtractor) {
            CBotGlobals::botMessage(pEntity, 0, "[ML] Failed to create feature extractor");
            return COMMAND_ERROR;
        }

        // Extract features
        std::vector<float> features;
        pExtractor->Extract(pBot, features);

        // Get feature names
        std::vector<std::string> names;
        pExtractor->GetFeatureNames(names);

        // Display header
        CBotGlobals::botMessage(pEntity, 0, "[ML] ========================================");
        CBotGlobals::botMessage(pEntity, 0, "[ML] Feature Dump for Bot #%d: %s",
            botIndex, pBot->getName());
        CBotGlobals::botMessage(pEntity, 0, "[ML] Extractor: %s",
            pExtractor->GetDescription());
        CBotGlobals::botMessage(pEntity, 0, "[ML] Features: %zu", features.size());
        CBotGlobals::botMessage(pEntity, 0, "[ML] ========================================");

        // Display features in groups
        CBotGlobals::botMessage(pEntity, 0, "[ML] Self State (0-11):");
        for (size_t i = 0; i < 12 && i < features.size(); i++) {
            CBotGlobals::botMessage(pEntity, 0, "[ML]   [%2zu] %-20s = %.4f",
                i, names[i].c_str(), features[i]);
        }

        CBotGlobals::botMessage(pEntity, 0, "[ML] Enemies (12-35):");
        for (size_t i = 12; i < 36 && i < features.size(); i++) {
            CBotGlobals::botMessage(pEntity, 0, "[ML]   [%2zu] %-20s = %.4f",
                i, names[i].c_str(), features[i]);
        }

        CBotGlobals::botMessage(pEntity, 0, "[ML] Navigation (36-47):");
        for (size_t i = 36; i < 48 && i < features.size(); i++) {
            CBotGlobals::botMessage(pEntity, 0, "[ML]   [%2zu] %-20s = %.4f",
                i, names[i].c_str(), features[i]);
        }

        CBotGlobals::botMessage(pEntity, 0, "[ML] Pickups (48-55):");
        for (size_t i = 48; i < 56 && i < features.size(); i++) {
            CBotGlobals::botMessage(pEntity, 0, "[ML]   [%2zu] %-20s = %.4f",
                i, names[i].c_str(), features[i]);
        }

        CBotGlobals::botMessage(pEntity, 0, "[ML] ========================================");

        CFeatureExtractorFactory::DestroyExtractor(pExtractor);
        return COMMAND_ACCESSED;
    });

CBotCommandInline MLFeaturesTestCommand("ml_features_test", CMD_ACCESS_BOT | CMD_ACCESS_DEDICATED,
    [](const CClient* pClient, const BotCommandArgs& args)
    {
        edict_t* pEntity = nullptr;
        if (pClient)
            pEntity = pClient->getPlayer();

        // Get target bot
        int botIndex = 0;
        if (args[0] && *args[0]) {
            botIndex = atoi(args[0]);
        }

        CBot* pBot = CBots::GetBotPointer(botIndex);
        if (!pBot || !pBot->inUse()) {
            CBotGlobals::botMessage(pEntity, 0, "[ML] Bot %d not found", botIndex);
            return COMMAND_ERROR;
        }

        // Create feature extractor
        CFeatureExtractor* pExtractor = CFeatureExtractorFactory::CreateExtractor();
        if (!pExtractor) {
            CBotGlobals::botMessage(pEntity, 0, "[ML] Failed to create extractor");
            return COMMAND_ERROR;
        }

        // Benchmark feature extraction
        constexpr int iterations = 1000;
        std::vector<float> features;

        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iterations; i++) {
            pExtractor->Extract(pBot, features);
        }
        auto end = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        float avgTimeMs = (duration.count() / static_cast<float>(iterations)) / 1000.0f;

        CBotGlobals::botMessage(pEntity, 0, "[ML] Feature Extraction Benchmark:");
        CBotGlobals::botMessage(pEntity, 0, "[ML]   Bot: %s (#%d)", pBot->getName(), botIndex);
        CBotGlobals::botMessage(pEntity, 0, "[ML]   Features: %zu", features.size());
        CBotGlobals::botMessage(pEntity, 0, "[ML]   Iterations: %d", iterations);
        CBotGlobals::botMessage(pEntity, 0, "[ML]   Average time: %.4f ms", avgTimeMs);

        if (avgTimeMs < 0.05f) {
            CBotGlobals::botMessage(pEntity, 0, "[ML]   Performance: EXCELLENT");
        } else if (avgTimeMs < 0.1f) {
            CBotGlobals::botMessage(pEntity, 0, "[ML]   Performance: GOOD");
        } else if (avgTimeMs < 0.2f) {
            CBotGlobals::botMessage(pEntity, 0, "[ML]   Performance: ACCEPTABLE");
        } else {
            CBotGlobals::botMessage(pEntity, 0, "[ML]   Performance: TOO SLOW (optimize!)");
        }

        CFeatureExtractorFactory::DestroyExtractor(pExtractor);
        return COMMAND_ACCESSED;
    });

CBotCommandInline MLFeaturesInfoCommand("ml_features_info", CMD_ACCESS_BOT | CMD_ACCESS_DEDICATED,
    [](const CClient* pClient, const BotCommandArgs& args)
    {
        edict_t* pEntity = nullptr;
        if (pClient)
            pEntity = pClient->getPlayer();

        CFeatureExtractor* pExtractor = CFeatureExtractorFactory::CreateExtractor();
        if (!pExtractor) {
            CBotGlobals::botMessage(pEntity, 0, "[ML] Failed to create extractor");
            return COMMAND_ERROR;
        }

        std::vector<std::string> names;
        pExtractor->GetFeatureNames(names);

        CBotGlobals::botMessage(pEntity, 0, "[ML] Feature Extractor Information:");
        CBotGlobals::botMessage(pEntity, 0, "[ML]   %s", pExtractor->GetDescription());
        CBotGlobals::botMessage(pEntity, 0, "[ML]   Feature count: %zu", pExtractor->GetFeatureCount());
        CBotGlobals::botMessage(pEntity, 0, "[ML]   All features:");

        for (size_t i = 0; i < names.size(); i++) {
            CBotGlobals::botMessage(pEntity, 0, "[ML]     [%2zu] %s", i, names[i].c_str());
        }

        CFeatureExtractorFactory::DestroyExtractor(pExtractor);
        return COMMAND_ACCESSED;
    });
