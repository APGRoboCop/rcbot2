# RCBot2 Machine Learning Module (HL2DM First)

This directory contains the ML infrastructure for RCBot2, implementing Phase 0 of the AI/ML roadmap.

**Target Game**: **HL2DM (Half-Life 2: Deathmatch)** - Start simple, then expand to TF2/DOD/CS:S

## Overview

The ML module provides:
- **ONNX Runtime Integration**: Load and run ONNX models for bot decision making
- **Feature Extraction**: Convert game state to normalized feature vectors
- **Gameplay Recording**: Record bot/human gameplay for training data
- **ML Controller**: Coordinate inference and action execution
- **Hybrid AI**: Blend ML predictions with rule-based AI

## Why HL2DM First?

Phase 0 targets HL2DM exclusively:
- ✅ Simplest game (pure deathmatch, no classes, no objectives)
- ✅ Only 64 features (vs TF2's 96 features)
- ✅ Smaller codebase (`bot_hldm_bot.cpp`: 884 lines vs TF2's 8,485)
- ✅ Faster iteration and debugging
- ✅ Once working, ~70-80% code reuse for TF2/DOD/CS:S expansion

## Architecture

```
┌─────────────────────────────────────────────────────┐
│                    CBot (Main Bot)                   │
└─────────────────────┬───────────────────────────────┘
                      │
                      ▼
          ┌───────────────────────┐
          │   CMLController       │  ◄─── Main ML interface
          └───────┬───────────────┘
                  │
        ┌─────────┼─────────┐
        ▼         ▼         ▼
   ┌────────┐ ┌──────┐ ┌─────────┐
   │Feature │ │ONNX  │ │Action   │
   │Extract │ │Model │ │Decoder  │
   └────────┘ └──────┘ └─────────┘
        │         │         │
        └─────────┴─────────┘
                  │
                  ▼
            ┌───────────┐
            │Bot Actions│  ◄─── Movement, aim, buttons
            └───────────┘
```

## File Structure

### Header Files

- **bot_ml_onnx.h**: ONNX Runtime wrapper
  - `CONNXModel`: Load and run ONNX models
  - `CModelManager`: Manage multiple models

- **bot_ml_features.h**: Feature extraction
  - `IFeatureExtractor`: Base interface
  - **`CHL2DMFeatureExtractor`**: HL2DM-specific features (Phase 0 - PRIMARY)
  - `CTF2FeatureExtractor`: TF2-specific features (Phase 0.5)
  - `CDODFeatureExtractor`: DOD:S-specific features (Phase 0.5)
  - `CFeatureStatistics`: Track feature statistics

- **bot_ml_recorder.h**: Gameplay recording
  - `CBotRecorder`: Record gameplay data
  - `CDemoParser`: Parse SourceTV demos
  - `CDataAugmenter`: Augment training data

- **bot_ml_controller.h**: Main ML controller
  - `CMLController`: Per-bot ML coordination
  - `CMLManager`: Global ML management
  - `CHybridAI`: Hybrid ML/rule-based AI

- **bot_ml_cvars.h**: Console commands and variables
  - ConVar declarations
  - Console command handlers

### Implementation Files (To Be Created)

**Phase 0 (HL2DM Only)**:
- `bot_ml_onnx.cpp`: ONNX wrapper implementation
- `bot_ml_features.cpp`: Base feature extraction
- **`bot_ml_features_hl2dm.cpp`**: HL2DM-specific features (64 features)
- `bot_ml_recorder.cpp`: Recording system
- `bot_ml_controller.cpp`: ML controller
- `bot_ml_cvars.cpp`: Console commands

**Phase 0.5 (Game Expansion)**:
- `bot_ml_features_tf2.cpp`: TF2-specific features (96 features)
- `bot_ml_features_dod.cpp`: DOD:S-specific features (~64 features)
- `bot_ml_features_css.cpp`: CS:S-specific features (~80 features)

## Usage

### Basic Usage (Behavior Cloning - HL2DM)

```cpp
// In bot initialization (bot_hldm_bot.cpp)
CMLController* pMLController = CMLManager::GetInstance()->CreateController(pBot);
pMLController->LoadModel("models/behavior_clone_hl2dm.onnx");  // HL2DM model!
pMLController->SetMode(ML_MODE_BEHAVIOR_CLONE);

// In bot Think() loop
if (pMLController->IsModeActive()) {
    MLAction action;
    if (pMLController->GetMLAction(action)) {
        // Apply ML action
        SetMovement(action.movement);  // Forward/back/strafe
        SetAim(action.aim_delta);      // Aim adjustments
        SetButtons(action.buttons);    // Jump, crouch, fire, etc.
        return; // Skip rule-based AI
    }
}

// Fallback to rule-based AI
RunRuleBasedAI();
```

### Recording Gameplay

```cpp
// In TF2 console
rcbot_record_start
// Play for 10 minutes
rcbot_record_stop
rcbot_record_save "recordings/session_001.rcbr"
rcbot_record_export_json "recordings/session_001.json"
```

### Hybrid Mode

```cpp
CHybridAI* pHybridAI = new CHybridAI(pBot);
pHybridAI->SetUseMLForMovement(true);
pHybridAI->SetUseMLForAiming(true);
pHybridAI->SetUseMLForButtons(false); // Use rules for shooting decisions

// In Think() loop
pHybridAI->MakeDecision(); // Automatically blends ML and rules
```

## Console Commands

### Model Management
```
rcbot_ml_load_model <path>      - Load ONNX model
rcbot_ml_reload_model            - Reload current model
rcbot_ml_list_models             - List available models
```

### Recording
```
rcbot_record_start               - Start recording
rcbot_record_stop                - Stop recording
rcbot_record_save <file>         - Save recording
rcbot_record_export_json <file>  - Export to JSON
```

### Debugging
```
rcbot_ml_features_dump           - Print current features
rcbot_ml_stats                   - Print performance stats
rcbot_ml_test_inference          - Test inference
```

## Configuration

ML configuration is loaded from `config/ml_models.json` and `config/ml_features.json`.

Example model configuration:
```json
{
  "models": {
    "behavior_clone_tf2": {
      "enabled": true,
      "path": "models/behavior_clone_tf2.onnx",
      "type": "behavior_cloning",
      "input_size": 96,
      "output_size": 10,
      "game": "tf2"
    }
  }
}
```

## Feature Vector

### TF2 Features (96 total)

1. **Self State** (10 features)
   - Health, armor, ammo, position, velocity

2. **Visible Threats** (40 features = 5 enemies × 8)
   - Distance, angle, health, class, threat level

3. **Objectives** (20 features)
   - Objective distance/status, team score, pickups

4. **Navigation** (16 features)
   - Waypoints, path length, cover positions

5. **Recent History** (10 features)
   - Damage dealt/taken, kills, deaths, time since spawn

All features are normalized to [0, 1] or [-1, 1] ranges.

## Action Output

### Model Output Vector (10 values)

1. **Movement** (3 values)
   - Forward/backward: [-1, 1] → scaled to ±450 units/sec
   - Left/right: [-1, 1] → scaled to ±450 units/sec
   - Up/down: [-1, 1] → scaled to ±200 units/sec

2. **Aiming** (2 values)
   - Yaw delta: [-1, 1] → scaled to ±30 degrees/frame
   - Pitch delta: [-1, 1] → scaled to ±30 degrees/frame

3. **Buttons** (5 values, binary at 0.5 threshold)
   - Attack, Jump, Crouch, Reload, Secondary Attack

## Performance Targets

- **Inference Time**: <1ms per bot per frame
- **Memory Usage**: <10MB per bot
- **FPS Impact**: <5% with 32 bots
- **Accuracy**: >85% for behavior cloning

## Development Workflow

### 1. Data Collection
```bash
# Record gameplay
rcbot_record_start
# ... play for 10+ minutes ...
rcbot_record_stop
rcbot_record_save "recordings/session_001.rcbr"
rcbot_record_export_json "data/session_001.json"
```

### 2. Training (Python)
```bash
# Train behavior cloning model
python tools/train_behavior_clone.py \
    --data data/session_*.json \
    --output models/behavior_clone_tf2.onnx \
    --epochs 50
```

### 3. Deployment
```bash
# In TF2 console
rcbot_ml_load_model "models/behavior_clone_tf2.onnx"
rcbot_ml_mode behavior_clone
rcbot_bot_add 5
```

### 4. Evaluation
```bash
# Monitor performance
rcbot_ml_stats
# Compare with rule-based AI
rcbot_ml_mode off  # Disable ML
# ... observe behavior ...
rcbot_ml_mode behavior_clone  # Re-enable ML
```

## Dependencies

### C++ Dependencies
- ONNX Runtime 1.16+ (included in onnxruntime/ folder)
- Source Engine SDK
- Standard C++11 library

### Python Dependencies (for training)
```bash
pip install torch>=2.0 onnx>=1.14 onnxruntime>=1.16 numpy pandas
```

## Testing

### Unit Tests (To Be Implemented)
- `test_onnx.cpp`: Test ONNX loading and inference
- `test_features.cpp`: Test feature extraction
- `test_recorder.cpp`: Test recording system

### Integration Tests
1. Load test model → verify <1ms inference
2. Record 10 min gameplay → export to JSON
3. Train model → export to ONNX → load in-game
4. Run 32 bots with ML → verify <5% FPS impact

## Troubleshooting

### Model Not Loading
- Check model path is correct
- Verify ONNX Runtime library is in path
- Check model input/output dimensions match config

### Inference Too Slow
- Reduce model size (fewer layers/neurons)
- Enable quantization (INT8)
- Check inference running on single thread
- Profile with `rcbot_ml_stats`

### Bot Behavior Weird
- Validate features: `rcbot_ml_features_dump`
- Check model predictions: `rcbot_ml_debug_predictions 1`
- Try hybrid mode to blend with rules
- Collect more/better training data

## Future Work (Phase 1+)

- [ ] Reinforcement learning (DQN, PPO)
- [ ] Transfer learning between games
- [ ] Online learning during gameplay
- [ ] Ensemble models
- [ ] Adversarial training
- [ ] Player skill recognition

## References

- **ONNX Runtime**: https://onnxruntime.ai/
- **PyTorch ONNX**: https://pytorch.org/docs/stable/onnx.html
- **RCBot2 Roadmap**: `../../roadmap-intelligence.md`
- **Implementation Plan**: `../../IMPLEMENTATION_PLAN.md`

---

**Last Updated**: 2025-11-22
**Status**: Architecture defined, implementation pending
**Phase**: 0 - Foundation and Infrastructure
