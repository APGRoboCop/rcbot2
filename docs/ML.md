# RCBot2 Machine Learning Documentation

**Status: NOT YET IMPLEMENTED**

This document describes planned ML features for RCBot2, including behavior cloning, ONNX model inference, and SourceMod integration for ML-controlled bots.

---

## Table of Contents

- [Overview](#overview)
- [ML Training and Deployment](#ml-training-and-deployment)
- [ONNX Runtime Integration](#onnx-runtime-integration)
- [SourceMod Integration (Phase 1)](#sourcemod-integration-phase-1)
- [Implementation Roadmap](#implementation-roadmap)

---

## Overview

RCBot2's ML system aims to create bots that learn from human gameplay through behavior cloning. The pipeline consists of:

1. **Data Collection** - Record human gameplay sessions
2. **Model Training** - Train neural networks using behavior cloning
3. **ONNX Export** - Convert models to ONNX format for inference
4. **In-Game Deployment** - Load and run models in real-time
5. **Hybrid Control** - Combine ML with rule-based behavior

### Implementation Status

| Feature | Status |
|---------|--------|
| Recording system | Planned |
| Training pipeline | Planned |
| ONNX inference | Planned |
| ML bot control | Planned |
| SourceMod natives | Planned |

---

## ML Training and Deployment

### Prerequisites

#### Software Requirements

1. **RCBot2** built with ML support:
   ```bash
   python3 configure.py --enable-optimize
   ambuild
   ```

2. **Python 3.8+** with ML packages:
   ```bash
   pip install torch numpy onnx onnxruntime tqdm
   ```

3. **HL2DM Server** (for data collection and testing)

### Data Collection

#### Recording Gameplay

Launch your HL2DM server with RCBot2 and start recording:

```bash
# In server console
rcbot addbot          # Add bots for opponents
rcbot ml_record_start # Start recording

# Play for 10-30 minutes
# Focus on diverse gameplay: combat, movement, item collection

rcbot ml_record_stop  # Stop recording
rcbot ml_record_save session_001
rcbot ml_record_export_json session_001
```

**Output Files**:
- `rcbot2/data/ml/session_001.rcbr` (binary format, ~9 MB/min)
- `rcbot2/data/ml/session_001.json` (JSON format, ~15 MB/min)

#### Multiple Sessions

**Recommended**: 10-20 hours of gameplay data

```bash
# Session 1: Basic combat
rcbot ml_record_start
# ... play for 30 minutes ...
rcbot ml_record_save combat_01
rcbot ml_record_export_json combat_01

# Session 2: Movement-focused
rcbot ml_record_start
# ... practice rocket jumping, strafe jumping ...
rcbot ml_record_save movement_01
rcbot ml_record_export_json movement_01

# Session 3: Item collection
rcbot ml_record_start
# ... focus on health/ammo pickups ...
rcbot ml_record_save items_01
rcbot ml_record_export_json items_01
```

#### Data Quality

**Good Data Indicators**:
- Frame rate: 30 FPS (1800 frames/min)
- Varied actions (not just standing still)
- Multiple enemy encounters
- Health/ammo collection events

### Training Pipeline

#### Basic Training

```bash
python3 tools/train_hl2dm_behavior_clone.py \
    --data data/ml/combat_01.json \
    --epochs 50 \
    --output-dir models/hl2dm_v1
```

#### Advanced Training

```bash
python3 tools/train_hl2dm_behavior_clone.py \
    --data "data/ml/*.json" \
    --epochs 100 \
    --batch-size 128 \
    --learning-rate 0.001 \
    --hidden-sizes 128 64 32 \
    --dropout 0.2 \
    --output-dir models/hl2dm_v2
```

**Training Parameters**:

| Parameter | Default | Description |
|-----------|---------|-------------|
| `--epochs` | 50 | Training iterations |
| `--batch-size` | 64 | Samples per update |
| `--learning-rate` | 0.001 | Learning speed |
| `--hidden-sizes` | 128 64 32 | Network architecture |
| `--dropout` | 0.2 | Regularization |

### Model Deployment

#### Load Model In-Game

```bash
# Start HL2DM server with RCBot2

# Load ONNX model
rcbot ml_model_load hl2dm_v1 hl2dm_behavior_clone.onnx

# Verify model loaded
rcbot ml_model_list

# Test inference
rcbot ml_model_test hl2dm_v1

# Benchmark performance
rcbot ml_model_benchmark hl2dm_v1 1000
```

#### Enable ML Control

```bash
# Single bot
rcbot addbot        # Add a bot (returns bot index)
rcbot ml_enable 0 hl2dm_v1  # Enable ML control for bot 0

# All bots
rcbot ml_enable_all hl2dm_v1

# Check status
rcbot ml_status

# Disable ML control
rcbot ml_disable 0
rcbot ml_disable_all
```

### Performance Targets

- **<0.5ms**: Excellent (recommended)
- **<1.0ms**: Good (acceptable)
- **<2.0ms**: Acceptable (may cause slight lag)
- **>2.0ms**: Too slow (use smaller model)

---

## ONNX Runtime Integration

### Quick Start (Without ONNX)

By default, RCBot2 can be built without ONNX Runtime support. The ML recording system will work, but model inference will not be available.

```bash
python3 configure.py --enable-optimize
ambuild
```

### Full Setup (With ONNX Runtime)

#### Step 1: Download ONNX Runtime

**Linux (x64)**:
```bash
cd rcbot2
mkdir -p onnxruntime
cd onnxruntime

wget https://github.com/microsoft/onnxruntime/releases/download/v1.16.3/onnxruntime-linux-x64-1.16.3.tgz
tar -xzf onnxruntime-linux-x64-1.16.3.tgz
mv onnxruntime-linux-x64-1.16.3/* .
rmdir onnxruntime-linux-x64-1.16.3
```

**Windows (x64)**:
```powershell
cd rcbot2
mkdir onnxruntime
cd onnxruntime

# Download from GitHub releases:
# https://github.com/microsoft/onnxruntime/releases/download/v1.16.3/onnxruntime-win-x64-1.16.3.zip
# Extract to rcbot2/onnxruntime/
```

**Directory Structure**:
```
rcbot2/
├── onnxruntime/
│   ├── include/
│   │   └── onnxruntime/
│   │       └── core/
│   │           └── session/
│   │               └── onnxruntime_cxx_api.h
│   └── lib/
│       └── libonnxruntime.so.1.16.3  (Linux)
│       └── onnxruntime.lib           (Windows)
```

#### Step 2: Update AMBuildScript

Add ONNX support configuration to `AMBuildScript`:

```python
# ONNX Runtime support (optional)
onnx_path = os.path.join(builder.currentSourcePath, 'onnxruntime')
if os.path.isdir(onnx_path):
    cxx.cxxincludes += [os.path.join(onnx_path, 'include')]
    cxx.defines += ['RCBOT_WITH_ONNX']

    if builder.target_platform == 'linux':
        cxx.linkflags += [
            '-L' + os.path.join(onnx_path, 'lib'),
            '-lonnxruntime',
            '-Wl,-rpath,$ORIGIN'
        ]
    elif builder.target_platform == 'windows':
        cxx.linkflags += [
            os.path.join(onnx_path, 'lib', 'onnxruntime.lib')
        ]

    print('ONNX Runtime support: ENABLED')
else:
    print('ONNX Runtime support: DISABLED')
```

#### Step 3: Build with ONNX

```bash
python3 configure.py --enable-optimize
ambuild
```

#### Step 4: Deploy Runtime Library

**Linux**:
```bash
cp onnxruntime/lib/libonnxruntime.so.1.16.3 addons/rcbot2/bin/
ln -s libonnxruntime.so.1.16.3 libonnxruntime.so
```

**Windows**:
Copy `onnxruntime.dll` to the same folder as `rcbot.dll`.

### Verification

```bash
# Create test model
python3 tools/create_test_model.py

# Load in game
rcbot ml_model_load test models/test_model.onnx
rcbot ml_model_test test
rcbot ml_model_benchmark test 1000
```

### Troubleshooting

#### "ONNX Runtime not available"

ONNX libraries not found at runtime.
- Ensure library is in same directory as plugin
- Check `LD_LIBRARY_PATH` (Linux) or `PATH` (Windows)

#### "onnxruntime_cxx_api.h: No such file"

Headers not found during compilation.
- Verify `onnxruntime/include/` exists
- Check AMBuildScript include path

#### Inference too slow (>2ms)

- Use smaller model architecture
- Enable ONNX graph optimizations
- Consider INT8 quantization

---

## SourceMod Integration (Phase 1)

Phase 1 focuses on exposing bot control, weapon management, and task system queries to SourcePawn plugins.

### New Enumerations

```sourcepawn
enum RCBotObjective {
    RCBotObjective_None = 0,
    RCBotObjective_Attack,
    RCBotObjective_Defend,
    RCBotObjective_Goto,
    RCBotObjective_Retreat,
    RCBotObjective_Follow
};

enum RCBotAction {
    RCBotAction_Jump = 0,
    RCBotAction_Crouch,
    RCBotAction_Scope,
    RCBotAction_Reload,
    RCBotAction_UseItem
};

enum RCBotTask {
    RCBotTask_None = 0,
    RCBotTask_FindPath,
    RCBotTask_GetAmmo,
    RCBotTask_GetHealth,
    RCBotTask_Attack,
    RCBotTask_Defend,
    RCBotTask_Build,
    RCBotTask_UseDispenser,
    RCBotTask_UseTeleporter,
    RCBotTask_FindEnemy,
    RCBotTask_AvoidDanger,
    RCBotTask_FollowLeader
};
```

### Bot Command & Control Natives

```sourcepawn
native bool RCBot2_SetBotObjective(int client, RCBotObjective objective, const float origin[3] = {0.0, 0.0, 0.0});
native bool RCBot2_SetBotEnemy(int client, int target);
native bool RCBot2_ClearBotEnemy(int client);
native bool RCBot2_ForceBotAction(int client, RCBotAction action);
native int RCBot2_GetBotTeam(int client);
native int RCBot2_GetBotHealth(int client);
native int RCBot2_GetBotEnemy(int client);
native bool RCBot2_GetBotOrigin(int client, float origin[3]);
native bool RCBot2_GetBotEyeAngles(int client, float angles[3]);
```

### Weapon Management Natives

```sourcepawn
native bool RCBot2_SelectWeapon(int client, const char[] weapon);
native bool RCBot2_GetCurrentWeapon(int client, char[] weapon, int maxlength);
native bool RCBot2_ForceAttack(int client, bool primary = true);
native bool RCBot2_ForceReload(int client);
```

### Task System Natives

```sourcepawn
native RCBotTask RCBot2_GetCurrentTask(int client);
native bool RCBot2_ClearSchedule(int client);
native float RCBot2_GetTaskProgress(int client);
```

### Example Plugin

```sourcepawn
#include <sourcemod>
#include <rcbot2>

public void OnPluginStart() {
    RegAdminCmd("sm_rcbot_test", CMD_Test, ADMFLAG_ROOT);
}

public Action CMD_Test(int client, int args) {
    int bot = -1;
    for (int i = 1; i <= MaxClients; i++) {
        if (IsClientInGame(i) && IsRCBot2Client(i)) {
            bot = i;
            break;
        }
    }

    if (bot == -1) {
        bot = RCBot2_CreateBot("TestBot");
    }

    int team = RCBot2_GetBotTeam(bot);
    int health = RCBot2_GetBotHealth(bot);

    float origin[3];
    RCBot2_GetBotOrigin(bot, origin);

    ReplyToCommand(client, "Bot %d: Team=%d, Health=%d%%, Pos=(%.0f, %.0f, %.0f)",
        bot, team, health, origin[0], origin[1], origin[2]);

    RCBotTask task = RCBot2_GetCurrentTask(bot);
    ReplyToCommand(client, "Current task: %d", task);

    return Plugin_Handled;
}
```

---

## Implementation Roadmap

### Phase 0: Core ML Infrastructure

| Priority | Feature | Status |
|----------|---------|--------|
| 1 | ML recording system | Planned |
| 2 | ONNX model loading | Planned |
| 3 | Feature extraction | Planned |
| 4 | Behavior cloning training | Planned |

### Phase 1: Essential Bot Control (SourceMod)

| Feature | Natives | Status |
|---------|---------|--------|
| Bot Command & Control | 9 natives | Planned |
| Weapon Management | 4 natives | Planned |
| Task System Queries | 3 natives | Planned |

### Phase 2: Game-Specific Extensions

- TF2 class-specific natives
- CS:S bomb/hostage natives
- DOD:S objective natives

### Phase 3: Advanced Features

- Real-time ML inference
- Hybrid ML/rule-based control
- Custom model architectures
- Multi-agent coordination

### Phase 4: Future Research

- Reinforcement learning (PPO/DQN)
- Imitation learning with DAgger
- Multi-agent RL for teamwork
- Curriculum learning

---

## Architecture

### Feature Extraction

The ML system extracts 56 features from game state (HL2DM):

| Category | Features |
|----------|----------|
| Player State | Health, armor, ammo, position, velocity |
| Enemy Info | Distance, angle, visible, last seen |
| Navigation | Waypoint distance, path length, stuck time |
| Environment | Nearby pickups, cover positions |
| Weapons | Current weapon, available weapons |

### Model Architecture

Default behavior cloning model:
```
Input (56 features)
    → Linear(128) → ReLU → Dropout(0.2)
    → Linear(64) → ReLU → Dropout(0.2)
    → Linear(32) → ReLU
    ↓
    ├─→ Continuous Output (4): movement, aim
    └─→ Binary Output (5): attack, reload, jump, etc.
```

### Inference Pipeline

1. Game state → Feature extraction (C++)
2. Features → ONNX model inference
3. Model output → Action interpretation
4. Actions → Bot command execution

---

## Resources

- [ONNX Runtime Releases](https://github.com/microsoft/onnxruntime/releases)
- [ONNX Runtime C++ API](https://onnxruntime.ai/docs/api/c/)
- [PyTorch ONNX Export](https://pytorch.org/docs/stable/onnx.html)
- [SourceMod API](https://sm.alliedmods.net/new-api/)

---

**Status**: NOT YET IMPLEMENTED
**Last Updated**: 2025-12-27
