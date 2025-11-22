# RCBot2 AI/ML Implementation Plan (HL2DM First)

**Last Updated**: 2025-11-22
**Status**: Phase 0 - Foundation (Not Started)
**Target Game**: **HL2DM ONLY** (expand to TF2/DOD/CS:S later)
**Next Milestone**: Deploy first ML feature (behavior cloning on HL2DM)

---

## 🎯 Why HL2DM First?

**Start with the simplest game, then expand:**
- HL2DM is pure deathmatch - no classes, no complex objectives
- `bot_hldm_bot.cpp` is only 884 lines (vs TF2's 8,485 lines)
- Faster iteration, easier debugging, clearer metrics
- Once HL2DM works, expansion to TF2/DOD/CS:S is straightforward (~70-80% code reuse)

**Expansion Path**: HL2DM (Phase 0) → TF2 (Phase 0.5) → DOD:S/CS:S (Phase 0.5) → Advanced ML (Phase 1+)

---

## Quick Start: What to Do Right Now

If you're ready to start implementing ML features, follow this priority order:

### Week 1-2: Environment Setup
1. ✅ Read this document and `roadmap-intelligence.md` Phase 0
2. ✅ Set up development environment (see `docs/building.md`)
3. ✅ Build RCBot2 successfully on your platform
4. ✅ **Run HL2DM with bots** to understand current behavior
5. ✅ Review existing code: `bot.cpp`, **`bot_hldm_bot.cpp`** (only 884 lines!), `bot_perceptron.cpp`

### Week 3-4: Data Collection (PRIORITY #1)
6. ✅ Implement replay recording format (see Section 2.1 below)
7. ✅ Hook recording into `CBot::Think()` loop
8. ✅ Add console commands for recording control
9. ✅ **Record 5+ hours of HL2DM gameplay data**
10. ✅ Test data export to JSON/CSV

### Week 5-6: ONNX Integration (PRIORITY #2)
11. ✅ Add ONNX Runtime to build system
12. ✅ Create `CONNXModel` wrapper class
13. ✅ Create dummy test model and verify inference works
14. ✅ Benchmark inference performance (<0.5ms target for HL2DM's simpler features)

### Week 7-8: Feature Extraction (PRIORITY #3) - HL2DM Focused
15. ✅ Design **HL2DM feature vector (48-64 floats)** - simpler than TF2's 96
16. ✅ Implement `CFeatureExtractor` base class
17. ✅ **Implement `CHL2DMFeatureExtractor`** (self state, enemies, pickups, nav)
18. ✅ Add debugging console commands
19. ✅ Validate features are sensible on HL2DM maps

### Week 9-11: Behavior Cloning (PRIORITY #4) - HL2DM Only
20. ✅ **Record 10+ hours of human HL2DM player demos**
21. ✅ Create Python training pipeline (`tools/train_hl2dm_behavior_clone.py`)
22. ✅ Train behavior cloning model (64 input → 128 → 64 → 32 → 10 output)
23. ✅ Export to ONNX and deploy in **HL2DM** game
24. ✅ A/B test vs rule-based HL2DM bot
25. ✅ Test on multiple HL2DM maps (dm_lockdown, dm_overwatch, etc.)

**After Week 11**: You should have a working **HL2DM** ML bot that imitates human behavior!

### Week 12: Prepare for Game Expansion
26. ✅ Document HL2DM implementation
27. ✅ Identify TF2 expansion requirements
28. ✅ **Begin Phase 0.5: TF2 Expansion**

---

## Section 1: Development Prerequisites

### 1.1 Required Skills

**Minimum Required**:
- C++ programming (C++11 or later)
- Understanding of Source Engine basics
- Basic ML concepts (neural networks, training/inference)
- Git version control

**Nice to Have**:
- Python for training scripts (PyTorch/TensorFlow)
- Experience with ONNX or other ML inference libraries
- Game AI programming experience
- HL2DM gameplay knowledge (or TF2 for later expansion)

### 1.2 Required Tools

**C++ Development**:
- GCC 9+ (Linux) or Visual Studio 2019+ (Windows)
- CMake or AMBuild (build system)
- Git for version control

**ML Development**:
- Python 3.8+
- PyTorch 2.0+ or TensorFlow 2.12+
- ONNX Runtime 1.16+
- NumPy, Pandas for data processing

**Testing**:
- **Half-Life 2: Deathmatch** (primary testing game for Phase 0)
- Team Fortress 2 (for Phase 0.5 expansion)
- MetaMod:Source and SourceMod (optional)

### 1.3 Development Environment Setup

```bash
# Clone the repository
git clone https://github.com/your-username/rcbot2.git
cd rcbot2

# Create branch for ML work
git checkout -b ml-foundation

# Build the project (see docs/building.md for details)
# Linux:
python3 configure.py --enable-optimize
ambuild

# Windows:
# Use Visual Studio solution or AMBuild

# Install Python dependencies for training
pip install torch torchvision onnx onnxruntime numpy pandas matplotlib
```

---

## Section 2: Phase 0 Implementation Details

### 2.1 Data Collection System (Weeks 3-4)

#### 2.1.1 Replay Format Design

Create `utils/RCBot2_meta/bot_replay_format.h`:

```cpp
// Replay data format for ML training
struct BotReplayFrame {
    float timestamp;          // Game time
    int bot_index;           // Which bot (for multi-bot recording)

    // State features (what the bot sees/knows)
    Vector position;         // Bot 3D position
    QAngle viewangle;       // Bot aim direction
    float health;           // 0-1 normalized
    float armor;            // 0-1 normalized
    int ammo_primary;       // Current ammo count
    int ammo_secondary;
    int weapon_id;          // Current weapon

    // Visible entities (up to 32)
    struct VisibleEntity {
        int entity_index;
        int entity_class;    // Player, building, etc.
        Vector position;
        float distance;
        float threat_level;  // 0-1 score
    } visible[32];
    int num_visible;

    // Actions taken (labels for supervised learning)
    Vector movement;         // Forward/side/up movement
    QAngle aim_delta;       // Change in aim this frame
    int buttons;            // Button state (bitfield)

    // Outcomes (for reward calculation)
    float damage_dealt;     // This frame
    float damage_taken;
    int kills;              // Cumulative
    int deaths;
    float objective_score;  // Game mode specific
};

struct BotReplayHeader {
    char magic[4];          // "RCBR"
    int version;            // Format version
    char game_mode[32];     // e.g., "tf2_payload"
    char map_name[64];
    int frame_count;
    float duration;
};
```

**Why this format?**
- Compact: ~2KB per frame, ~120KB per second of recording
- Contains state (features) and actions (labels)
- Extensible: Add more fields without breaking old data
- Binary format for efficiency

#### 2.1.2 Recording System Implementation

Create `utils/RCBot2_meta/bot_recorder.h`:

```cpp
class CBotRecorder {
public:
    // Singleton pattern
    static CBotRecorder* GetInstance();

    // Recording control
    void StartRecording();
    void StopRecording();
    void SaveRecording(const char* filename);
    bool IsRecording() const { return m_bRecording; }

    // Record a frame of bot gameplay
    void RecordFrame(CBot* pBot);

    // Export to different formats
    bool ExportToJSON(const char* filename);
    bool ExportToCSV(const char* filename);

private:
    bool m_bRecording;
    std::vector<BotReplayFrame> m_frames;
    BotReplayHeader m_header;

    // Circular buffer (limit memory usage)
    static const int MAX_FRAMES = 100000; // ~2 hours at 60 FPS

    // Helper methods
    void ExtractFeatures(CBot* pBot, BotReplayFrame& frame);
    void ExtractActions(CBot* pBot, BotReplayFrame& frame);
};
```

**Integration into bot code** (`bot.cpp`):

```cpp
void CBot::Think() {
    // Existing code...

    // Record frame if enabled
    if (CBotRecorder::GetInstance()->IsRecording()) {
        CBotRecorder::GetInstance()->RecordFrame(this);
    }

    // Rest of Think() logic...
}
```

**Console commands** (add to `bot_cvars.cpp`):

```cpp
CON_COMMAND(rcbot_record_start, "Start recording bot gameplay") {
    CBotRecorder::GetInstance()->StartRecording();
    Msg("Recording started\n");
}

CON_COMMAND(rcbot_record_stop, "Stop recording") {
    CBotRecorder::GetInstance()->StopRecording();
    Msg("Recording stopped (%d frames)\n", frame_count);
}

CON_COMMAND(rcbot_record_save, "Save recording to file") {
    if (args.ArgC() < 2) {
        Msg("Usage: rcbot_record_save <filename>\n");
        return;
    }
    CBotRecorder::GetInstance()->SaveRecording(args.Arg(1));
}

CON_COMMAND(rcbot_record_export_json, "Export recording to JSON") {
    if (args.ArgC() < 2) {
        Msg("Usage: rcbot_record_export_json <filename>\n");
        return;
    }
    CBotRecorder::GetInstance()->ExportToJSON(args.Arg(1));
}
```

#### 2.1.3 Testing the Recording System

```bash
# In TF2 console:
sv_cheats 1
rcbot_bot_add 5  # Add 5 bots
rcbot_record_start
# Play for 5-10 minutes
rcbot_record_stop
rcbot_record_save recordings/test_session_001.rcbr
rcbot_record_export_json recordings/test_session_001.json

# Verify the JSON file is readable
cat recordings/test_session_001.json | jq '.header'
```

---

### 2.2 ONNX Runtime Integration (Weeks 5-6)

#### 2.2.1 Add ONNX to Build System

Update `utils/RCBot2_meta/AMBuild`:

```python
# Add ONNX Runtime include path
binary.compiler.includes += [
    os.path.join(builder.sourcePath, 'onnxruntime', 'include'),
]

# Add ONNX Runtime library
if builder.target_platform == 'linux':
    binary.compiler.linkflags += [
        '-L' + os.path.join(builder.sourcePath, 'onnxruntime', 'lib'),
        '-lonnxruntime',
    ]
elif builder.target_platform == 'windows':
    binary.compiler.linkflags += [
        os.path.join(builder.sourcePath, 'onnxruntime', 'lib', 'onnxruntime.lib'),
    ]
```

**Download ONNX Runtime**:
```bash
# Linux x64
wget https://github.com/microsoft/onnxruntime/releases/download/v1.16.3/onnxruntime-linux-x64-1.16.3.tgz
tar -xzf onnxruntime-linux-x64-1.16.3.tgz -C onnxruntime/

# Windows x64
# Download from https://github.com/microsoft/onnxruntime/releases
# Extract to onnxruntime/ folder
```

#### 2.2.2 Create ONNX Wrapper Classes

Create `utils/RCBot2_meta/bot_onnx.h`:

```cpp
#include <onnxruntime/core/session/onnxruntime_cxx_api.h>
#include <vector>
#include <memory>

class CONNXModel {
public:
    CONNXModel();
    ~CONNXModel();

    // Load model from file
    bool LoadModel(const char* model_path);

    // Run inference
    // input: flat array of features
    // output: flat array of predictions
    bool Inference(const std::vector<float>& input,
                   std::vector<float>& output);

    // Get model metadata
    size_t GetInputSize() const { return m_input_size; }
    size_t GetOutputSize() const { return m_output_size; }
    bool IsLoaded() const { return m_session != nullptr; }

private:
    std::unique_ptr<Ort::Env> m_env;
    std::unique_ptr<Ort::Session> m_session;
    std::unique_ptr<Ort::SessionOptions> m_session_options;

    size_t m_input_size;
    size_t m_output_size;

    std::vector<const char*> m_input_names;
    std::vector<const char*> m_output_names;
};
```

Implementation `utils/RCBot2_meta/bot_onnx.cpp`:

```cpp
CONNXModel::CONNXModel()
    : m_input_size(0), m_output_size(0) {
    // Initialize ONNX Runtime environment
    m_env = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "RCBot2ML");
    m_session_options = std::make_unique<Ort::SessionOptions>();
    m_session_options->SetIntraOpNumThreads(1); // Single thread for game
    m_session_options->SetGraphOptimizationLevel(
        GraphOptimizationLevel::ORT_ENABLE_ALL);
}

bool CONNXModel::LoadModel(const char* model_path) {
    try {
        // Load ONNX model
        m_session = std::make_unique<Ort::Session>(
            *m_env, model_path, *m_session_options);

        // Get input/output dimensions
        Ort::AllocatorWithDefaultOptions allocator;

        // Input info
        size_t num_input_nodes = m_session->GetInputCount();
        if (num_input_nodes != 1) {
            Warning("Model has %zu inputs, expected 1\n", num_input_nodes);
            return false;
        }

        Ort::TypeInfo input_type_info = m_session->GetInputTypeInfo(0);
        auto input_tensor_info = input_type_info.GetTensorTypeAndShapeInfo();
        m_input_size = input_tensor_info.GetShape()[1]; // Assume [batch, features]

        // Output info
        size_t num_output_nodes = m_session->GetOutputCount();
        if (num_output_nodes != 1) {
            Warning("Model has %zu outputs, expected 1\n", num_output_nodes);
            return false;
        }

        Ort::TypeInfo output_type_info = m_session->GetOutputTypeInfo(0);
        auto output_tensor_info = output_type_info.GetTensorTypeAndShapeInfo();
        m_output_size = output_tensor_info.GetShape()[1];

        Msg("ONNX model loaded: %zu inputs, %zu outputs\n",
            m_input_size, m_output_size);

        return true;
    } catch (const Ort::Exception& e) {
        Warning("Failed to load ONNX model: %s\n", e.what());
        return false;
    }
}

bool CONNXModel::Inference(const std::vector<float>& input,
                           std::vector<float>& output) {
    if (!m_session) return false;
    if (input.size() != m_input_size) return false;

    try {
        // Create input tensor
        std::vector<int64_t> input_shape = {1, (int64_t)m_input_size};
        Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(
            OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);

        Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
            memory_info, const_cast<float*>(input.data()), input.size(),
            input_shape.data(), input_shape.size());

        // Run inference
        const char* input_names[] = {"input"};
        const char* output_names[] = {"output"};

        auto output_tensors = m_session->Run(
            Ort::RunOptions{nullptr}, input_names, &input_tensor, 1,
            output_names, 1);

        // Extract output
        float* output_data = output_tensors.front().GetTensorMutableData<float>();
        output.assign(output_data, output_data + m_output_size);

        return true;
    } catch (const Ort::Exception& e) {
        Warning("ONNX inference failed: %s\n", e.what());
        return false;
    }
}
```

#### 2.2.3 Create Test Model (Python)

Create `tools/create_test_model.py`:

```python
import torch
import torch.nn as nn

class DummyModel(nn.Module):
    """Simple model for testing ONNX integration"""
    def __init__(self, input_size=128, output_size=10):
        super().__init__()
        self.net = nn.Sequential(
            nn.Linear(input_size, 64),
            nn.ReLU(),
            nn.Linear(64, output_size)
        )

    def forward(self, x):
        return self.net(x)

# Create and export model
model = DummyModel(input_size=128, output_size=10)
dummy_input = torch.randn(1, 128)

torch.onnx.export(
    model,
    dummy_input,
    "models/test_model.onnx",
    input_names=["input"],
    output_names=["output"],
    dynamic_axes={"input": {0: "batch"}, "output": {0: "batch"}}
)

print("Test model exported to models/test_model.onnx")
```

#### 2.2.4 Test ONNX Integration

Add to `bot.cpp` (temporary test code):

```cpp
void TestONNX() {
    CONNXModel model;
    if (!model.LoadModel("models/test_model.onnx")) {
        Warning("Failed to load test model\n");
        return;
    }

    // Test inference
    std::vector<float> input(128, 0.5f); // Dummy input
    std::vector<float> output;

    auto start = std::chrono::high_resolution_clock::now();
    bool success = model.Inference(input, output);
    auto end = std::chrono::high_resolution_clock::now();

    if (success) {
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            end - start).count();
        Msg("ONNX inference successful! Latency: %lld µs\n", duration);
        Msg("Output size: %zu\n", output.size());
    } else {
        Warning("ONNX inference failed\n");
    }
}
```

---

### 2.3 Feature Extraction (Weeks 7-9)

#### 2.3.1 Feature Design

**Feature Categories** (Total: ~96 features):

1. **Self State** (10 features):
   - Health (normalized 0-1)
   - Armor (normalized 0-1)
   - Primary ammo (normalized by max)
   - Secondary ammo (normalized)
   - Current weapon ID (one-hot encoded, 5 values)
   - Position X, Y, Z (normalized by map bounds)

2. **Nearby Threats** (40 features = 5 enemies × 8 features each):
   - Enemy distance (0-1, 0=far, 1=close)
   - Enemy relative angle (cos, sin)
   - Enemy health (0-1 estimate)
   - Enemy class (one-hot, 9 classes for TF2)
   - Enemy threat level (computed score)

3. **Objectives** (20 features):
   - Distance to nearest objective (0-1)
   - Objective status (0=enemy, 0.5=contested, 1=friendly)
   - Team score difference (-1 to 1)
   - Time remaining (0-1 normalized)
   - Nearest health pack distance
   - Nearest ammo pack distance

4. **Navigation** (16 features):
   - Current waypoint type
   - Distance to next waypoint
   - Path length to objective
   - Nearby cover positions (4× direction + distance)

5. **Recent History** (10 features):
   - Damage dealt (last 5 seconds)
   - Damage taken (last 5 seconds)
   - Kills (last 60 seconds)
   - Deaths (last 60 seconds)
   - Time since last spawn

#### 2.3.2 Feature Extractor Implementation

Create `utils/RCBot2_meta/bot_features.h`:

```cpp
class CFeatureExtractor {
public:
    virtual ~CFeatureExtractor() {}

    // Extract features for a bot
    virtual void Extract(CBot* pBot, std::vector<float>& features) = 0;

    // Get expected feature count
    virtual size_t GetFeatureCount() const = 0;

    // Get feature names (for debugging)
    virtual void GetFeatureNames(std::vector<std::string>& names) = 0;
};

class CTF2FeatureExtractor : public CFeatureExtractor {
public:
    void Extract(CBot* pBot, std::vector<float>& features) override;
    size_t GetFeatureCount() const override { return 96; }
    void GetFeatureNames(std::vector<std::string>& names) override;

private:
    void ExtractSelfState(CBot* pBot, std::vector<float>& features);
    void ExtractThreats(CBot* pBot, std::vector<float>& features);
    void ExtractObjectives(CBot* pBot, std::vector<float>& features);
    void ExtractNavigation(CBot* pBot, std::vector<float>& features);
    void ExtractHistory(CBot* pBot, std::vector<float>& features);

    // Normalization helpers
    float NormalizeHealth(float health, float max_health);
    float NormalizeDistance(float distance, float max_distance = 4096.0f);
    float NormalizeAngle(float angle);
};
```

**Console command for debugging**:

```cpp
CON_COMMAND(rcbot_ml_features_dump, "Print current ML features for a bot") {
    CBot* pBot = CBots::GetBotPointer(0); // First bot
    if (!pBot) {
        Msg("No bots found\n");
        return;
    }

    CTF2FeatureExtractor extractor;
    std::vector<float> features;
    extractor.Extract(pBot, features);

    std::vector<std::string> names;
    extractor.GetFeatureNames(names);

    Msg("=== Bot Features (%zu) ===\n", features.size());
    for (size_t i = 0; i < features.size(); i++) {
        Msg("[%3zu] %s = %.3f\n", i, names[i].c_str(), features[i]);
    }
}
```

---

### 2.4 Behavior Cloning (Weeks 10-12)

#### 2.4.1 Collect Human Demonstrations

**Option 1: Record from SourceTV demos**

Create `tools/parse_demo.py`:

```python
"""
Parse SourceTV demo files to extract human player gameplay.
Requires demoparse library: pip install demoparse
"""

import demoparse
import numpy as np
import json

def parse_demo(demo_path, output_path):
    parser = demoparse.DemoParser(demo_path)

    # Extract player data
    # ... implementation specific to demo format

    # Convert to same format as bot recordings
    # Save to output_path
    pass

# Usage:
# python tools/parse_demo.py demos/pro_match.dem recordings/human_demo_001.json
```

**Option 2: Record human player controlling bot**

Use existing `rcbot_record_start` system, but track human input instead of bot AI.

#### 2.4.2 Training Pipeline

Create `tools/train_behavior_clone.py`:

```python
import torch
import torch.nn as nn
import torch.optim as optim
from torch.utils.data import Dataset, DataLoader
import json
import numpy as np

class BehaviorCloneDataset(Dataset):
    """Load recorded gameplay data for training"""
    def __init__(self, json_files):
        self.states = []
        self.actions = []

        for json_file in json_files:
            with open(json_file, 'r') as f:
                data = json.load(f)
                for frame in data['frames']:
                    self.states.append(frame['features'])
                    self.actions.append(frame['actions'])

        self.states = np.array(self.states, dtype=np.float32)
        self.actions = np.array(self.actions, dtype=np.float32)

    def __len__(self):
        return len(self.states)

    def __getitem__(self, idx):
        return (torch.tensor(self.states[idx]),
                torch.tensor(self.actions[idx]))

class BehaviorCloneModel(nn.Module):
    """Neural network for behavior cloning"""
    def __init__(self, input_size=96, output_size=10):
        super().__init__()
        self.net = nn.Sequential(
            nn.Linear(input_size, 256),
            nn.ReLU(),
            nn.Dropout(0.2),
            nn.Linear(256, 128),
            nn.ReLU(),
            nn.Dropout(0.2),
            nn.Linear(128, 64),
            nn.ReLU(),
            nn.Linear(64, output_size)
        )

    def forward(self, x):
        return self.net(x)

def train(model, dataloader, epochs=50):
    optimizer = optim.Adam(model.parameters(), lr=0.001)
    criterion = nn.MSELoss()

    for epoch in range(epochs):
        total_loss = 0
        for states, actions in dataloader:
            optimizer.zero_grad()
            predictions = model(states)
            loss = criterion(predictions, actions)
            loss.backward()
            optimizer.step()
            total_loss += loss.item()

        print(f"Epoch {epoch+1}/{epochs}, Loss: {total_loss/len(dataloader):.4f}")

def export_to_onnx(model, output_path, input_size=96):
    model.eval()
    dummy_input = torch.randn(1, input_size)
    torch.onnx.export(
        model, dummy_input, output_path,
        input_names=["input"], output_names=["output"],
        dynamic_axes={"input": {0: "batch"}, "output": {0: "batch"}}
    )
    print(f"Model exported to {output_path}")

# Main training script
if __name__ == "__main__":
    # Load data
    dataset = BehaviorCloneDataset([
        "recordings/human_demo_001.json",
        "recordings/human_demo_002.json",
        # Add more recordings...
    ])

    dataloader = DataLoader(dataset, batch_size=32, shuffle=True)

    # Train model
    model = BehaviorCloneModel(input_size=96, output_size=10)
    train(model, dataloader, epochs=50)

    # Export to ONNX
    export_to_onnx(model, "models/behavior_clone.onnx", input_size=96)
```

#### 2.4.3 Deploy in Game

Add to `bot_fortress.cpp`:

```cpp
class CML_BehaviorClone {
public:
    bool Init(const char* model_path);
    void GetAction(CBot* pBot, Vector& move, QAngle& aim_delta, int& buttons);

private:
    CONNXModel m_model;
    CTF2FeatureExtractor m_feature_extractor;
};

bool CML_BehaviorClone::Init(const char* model_path) {
    return m_model.LoadModel(model_path);
}

void CML_BehaviorClone::GetAction(CBot* pBot, Vector& move,
                                   QAngle& aim_delta, int& buttons) {
    // Extract features
    std::vector<float> features;
    m_feature_extractor.Extract(pBot, features);

    // Run inference
    std::vector<float> output;
    if (!m_model.Inference(features, output)) {
        // Fallback to rule-based AI
        return;
    }

    // Decode output to actions
    // output[0-2]: movement (forward, side, up)
    // output[3-4]: aim delta (yaw, pitch)
    // output[5-9]: buttons (fire, jump, crouch, etc.)

    move.x = output[0] * 450.0f; // Max forward speed
    move.y = output[1] * 450.0f; // Max side speed
    move.z = output[2] * 200.0f; // Max up speed

    aim_delta.y = output[3] * 30.0f; // Max yaw change per frame
    aim_delta.x = output[4] * 30.0f; // Max pitch change

    buttons = 0;
    if (output[5] > 0.5f) buttons |= IN_ATTACK;
    if (output[6] > 0.5f) buttons |= IN_JUMP;
    if (output[7] > 0.5f) buttons |= IN_DUCK;
    // etc...
}
```

**Integration**:

```cpp
// In bot Think() loop
if (rcbot_ml_mode.GetInt() == 1) { // Behavior clone mode
    static CML_BehaviorClone g_MLBot;
    if (!g_MLBot.IsInit()) {
        g_MLBot.Init("models/behavior_clone.onnx");
    }

    Vector move;
    QAngle aim_delta;
    int buttons;
    g_MLBot.GetAction(this, move, aim_delta, buttons);

    // Apply actions
    SetMovement(move);
    AdjustAim(aim_delta);
    SetButtons(buttons);
} else {
    // Use regular rule-based AI
    // ... existing code ...
}
```

---

## Section 3: Configuration Files

### 3.1 ML Features Configuration

Create `config/ml_features.json`:

```json
{
  "version": "1.0",
  "feature_groups": {
    "self_state": {
      "enabled": true,
      "features": [
        {"name": "health", "type": "float", "min": 0, "max": 1},
        {"name": "armor", "type": "float", "min": 0, "max": 1},
        {"name": "ammo_primary", "type": "float", "min": 0, "max": 1},
        {"name": "ammo_secondary", "type": "float", "min": 0, "max": 1}
      ]
    },
    "threats": {
      "enabled": true,
      "max_enemies": 5,
      "features_per_enemy": 8
    },
    "objectives": {
      "enabled": true,
      "game_mode_specific": true
    },
    "navigation": {
      "enabled": true,
      "include_waypoint_data": true
    }
  },
  "normalization": {
    "method": "min_max",
    "running_stats": false
  }
}
```

### 3.2 ML Model Configuration

Create `config/ml_models.json`:

```json
{
  "models": {
    "behavior_clone_tf2": {
      "path": "models/behavior_clone_tf2.onnx",
      "input_size": 96,
      "output_size": 10,
      "game": "tf2",
      "description": "Behavior cloning from human TF2 demos"
    },
    "dqn_tf2_soldier": {
      "path": "models/dqn_tf2_soldier.onnx",
      "input_size": 96,
      "output_size": 20,
      "game": "tf2",
      "class": "soldier",
      "description": "DQN trained for TF2 Soldier class"
    }
  },
  "performance": {
    "max_inference_time_ms": 1.0,
    "batch_inference": false,
    "num_threads": 1,
    "optimization_level": "all"
  }
}
```

---

## Section 4: Testing and Validation

### 4.1 Unit Tests

Create basic unit tests for each component:

```cpp
// test_onnx.cpp
void TestONNXLoading() {
    CONNXModel model;
    assert(model.LoadModel("models/test_model.onnx"));
    assert(model.GetInputSize() == 128);
    assert(model.GetOutputSize() == 10);
}

void TestONNXInference() {
    CONNXModel model;
    model.LoadModel("models/test_model.onnx");

    std::vector<float> input(128, 0.5f);
    std::vector<float> output;

    assert(model.Inference(input, output));
    assert(output.size() == 10);
}
```

### 4.2 Integration Tests

Test end-to-end pipeline:

1. Record 10 minutes of gameplay
2. Export to JSON
3. Train model on the data
4. Load model in-game
5. Run inference 1000 times, verify <1ms average latency
6. Play match with ML bot, verify no crashes

### 4.3 Performance Benchmarks

Create `tools/benchmark_ml.py`:

```python
"""Benchmark ML inference performance"""

import onnxruntime as ort
import numpy as np
import time

def benchmark_model(model_path, num_iterations=1000):
    session = ort.InferenceSession(model_path)
    input_name = session.get_inputs()[0].name
    input_shape = session.get_inputs()[0].shape

    # Random input
    input_data = np.random.randn(*input_shape).astype(np.float32)

    # Warmup
    for _ in range(10):
        session.run(None, {input_name: input_data})

    # Benchmark
    start = time.time()
    for _ in range(num_iterations):
        session.run(None, {input_name: input_data})
    end = time.time()

    avg_time_ms = (end - start) / num_iterations * 1000
    print(f"Average inference time: {avg_time_ms:.3f} ms")

    if avg_time_ms > 1.0:
        print("WARNING: Inference time exceeds 1ms target!")
    else:
        print("PASS: Inference time within target")

if __name__ == "__main__":
    benchmark_model("models/behavior_clone.onnx")
```

---

## Section 5: Common Issues and Solutions

### 5.1 ONNX Runtime Build Issues

**Problem**: Linker errors when building with ONNX Runtime

**Solution**:
- Ensure ONNX Runtime version matches your compiler
- Use static linking if possible
- Check that library path is in `LD_LIBRARY_PATH` (Linux) or `PATH` (Windows)

### 5.2 Inference Too Slow

**Problem**: Model inference takes >1ms

**Solutions**:
1. Reduce model size (fewer layers, smaller hidden dimensions)
2. Use INT8 quantization
3. Enable all ONNX Runtime optimizations
4. Profile to find bottlenecks
5. Consider moving inference to separate thread

### 5.3 Model Not Improving Bot Behavior

**Problem**: Behavior cloning model doesn't perform better than rule-based AI

**Solutions**:
1. Collect more training data (need >10 hours)
2. Ensure training data is from skilled players
3. Check feature extraction is correct (debug with `rcbot_ml_features_dump`)
4. Validate model predictions make sense
5. Try hybrid approach: ML for low-level control, rules for strategy

---

## Section 6: Next Steps After Phase 0

Once Phase 0 is complete, you'll have:
- ✅ Data collection infrastructure
- ✅ ML inference pipeline
- ✅ Working behavior cloning model
- ✅ Validation that ML can improve bot behavior

**Move to Phase 1 (Reinforcement Learning)**:
1. Design reward functions for TF2
2. Implement DQN or PPO algorithm
3. Set up offline training pipeline
4. Deploy RL-trained bot
5. Compare RL bot vs behavior cloning

**Or expand Phase 0 achievements**:
1. Add more game modes (DOD:S, CS:S)
2. Train class-specific models
3. Implement model ensembling
4. Add online learning (update model during gameplay)

---

## Section 7: Resources and References

### Documentation
- ONNX Runtime C++ API: https://onnxruntime.ai/docs/api/c/
- PyTorch ONNX Export: https://pytorch.org/docs/stable/onnx.html
- Source Engine SDK: https://developer.valvesoftware.com/

### Papers
- "A Reduction of Imitation Learning and Structured Prediction to No-Regret Online Learning" (DAgger algorithm)
- "Mastering the game of Go without human knowledge" (AlphaGo Zero - self-play)
- "Human-level control through deep reinforcement learning" (DQN)

### Code Examples
- `bot_perceptron.cpp` - Existing neural network code
- `bot_ga.cpp` - Genetic algorithm implementation
- `bot_fortress.cpp` - TF2 bot behavior (8,485 lines of reference)

---

**Last Updated**: 2025-11-22
**Status**: Phase 0 ready to begin
**Questions?** Review `roadmap-intelligence.md` or open an issue on GitHub
