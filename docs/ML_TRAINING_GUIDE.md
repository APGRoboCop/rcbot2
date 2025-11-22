# RCBot2 ML Training and Deployment Guide

**Last Updated**: 2025-11-22
**Target Game**: Half-Life 2: Deathmatch (HL2DM)
**Phase**: Behavior Cloning (Phase 0, Priority #4)

---

## Table of Contents

1. [Overview](#overview)
2. [Prerequisites](#prerequisites)
3. [Data Collection](#data-collection)
4. [Training Pipeline](#training-pipeline)
5. [Model Deployment](#model-deployment)
6. [Testing and Evaluation](#testing-and-evaluation)
7. [Troubleshooting](#troubleshooting)
8. [Advanced Topics](#advanced-topics)

---

## Overview

This guide walks through the complete process of training and deploying ML-powered bots in RCBot2:

1. **Collect Data**: Record human gameplay sessions
2. **Train Model**: Use behavior cloning to learn from recordings
3. **Export to ONNX**: Convert PyTorch model to ONNX format
4. **Deploy In-Game**: Load model and enable ML control
5. **Evaluate**: Compare ML bot vs rule-based bot performance

### What is Behavior Cloning?

Behavior cloning is supervised learning where the bot learns to imitate human actions:
- **Input**: Game state (56 features: health, enemies, navigation, etc.)
- **Output**: Actions (movement, aim, buttons)
- **Goal**: Minimize difference between bot actions and human actions

---

## Prerequisites

### Software Requirements

1. **RCBot2** built with ML support:
   ```bash
   # See docs/ONNX_SETUP.md for ONNX Runtime setup
   python3 configure.py --enable-optimize
   ambuild
   ```

2. **Python 3.8+** with ML packages:
   ```bash
   pip install torch numpy onnx onnxruntime tqdm
   ```

3. **HL2DM Server** (for data collection and testing)

### Knowledge Requirements

- Basic understanding of neural networks
- Familiarity with Python and PyTorch
- Experience running Source engine servers

---

## Data Collection

### Step 1: Start Recording

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

### Step 2: Collect Multiple Sessions

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

### Step 3: Verify Data Quality

```bash
# Check recording statistics
rcbot ml_record_status

# Verify JSON files
cat rcbot2/data/ml/combat_01.json | python3 -m json.tool | head -100
```

**Good Data Indicators**:
- Frame rate: 30 FPS (1800 frames/min)
- Varied actions (not just standing still)
- Multiple enemy encounters
- Health/ammo collection events

---

## Training Pipeline

### Step 1: Prepare Training Script

The training script is located at `tools/train_hl2dm_behavior_clone.py`.

**View help**:
```bash
python3 tools/train_hl2dm_behavior_clone.py --help
```

### Step 2: Train Model

**Basic training** (single recording):
```bash
python3 tools/train_hl2dm_behavior_clone.py \
    --data data/ml/combat_01.json \
    --epochs 50 \
    --output-dir models/hl2dm_v1
```

**Advanced training** (multiple recordings):
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
- `--epochs`: Number of training iterations (default: 50)
  - More epochs = better learning (but risk overfitting)
  - Watch validation loss - stop when it stops improving
- `--batch-size`: Samples per update (default: 64)
  - Larger = faster training, more memory
  - Smaller = more stable, less memory
- `--learning-rate`: Learning speed (default: 0.001)
  - Too high = unstable training
  - Too low = very slow learning
- `--hidden-sizes`: Network architecture (default: 128 64 32)
  - Larger = more capacity, slower inference
  - Smaller = faster inference, less capacity

### Step 3: Monitor Training

**Expected output**:
```
Loading 3 recording files...
Loaded 54,000 samples from 3 recordings

Model architecture:
BehaviorCloneModel(
  (shared_layers): Sequential(...)
  (continuous_head): Sequential(...)
  (binary_head): Sequential(...)
)

Total parameters: 12,345

Training on cuda
Epochs: 50, Learning rate: 0.001
Train samples: 43,200, Val samples: 10,800
------------------------------------------------------------
Epoch 1/50 - Train Loss: 0.342156, Val Loss: 0.298743, LR: 0.001000
  → Saved best model (val_loss: 0.298743)
Epoch 2/50 - Train Loss: 0.287654, Val Loss: 0.265432, LR: 0.001000
  → Saved best model (val_loss: 0.265432)
...
Epoch 50/50 - Train Loss: 0.123456, Val Loss: 0.134567, LR: 0.000100

Training complete. Best validation loss: 0.134567
```

**What to look for**:
- ✅ Train loss and val loss both decreasing
- ✅ Val loss eventually plateaus (convergence)
- ❌ Val loss increasing while train loss decreases (overfitting)
  - **Fix**: Reduce epochs, increase dropout, add more data

### Step 4: ONNX Export

The script automatically exports to ONNX at the end:

```
Exported ONNX model to models/hl2dm_v1/hl2dm_behavior_clone.onnx
ONNX model verified successfully
ONNX inference speed: 0.342ms (target: <0.5ms)
  ✓ EXCELLENT - Well within target
```

**Performance targets**:
- **<0.5ms**: Excellent (recommended)
- **<1.0ms**: Good (acceptable)
- **<2.0ms**: Acceptable (may cause slight lag)
- **>2.0ms**: Too slow (use smaller model)

---

## Model Deployment

### Step 1: Copy Model to Server

```bash
# Copy ONNX model to server
scp models/hl2dm_v1/hl2dm_behavior_clone.onnx user@server:/path/to/srcds/
```

### Step 2: Load Model In-Game

```bash
# Start HL2DM server with RCBot2

# Load ONNX model
rcbot ml_model_load hl2dm_v1 hl2dm_behavior_clone.onnx

# Verify model loaded
rcbot ml_model_list
# Output:
# [ML] Loaded ONNX models:
# [ML]   hl2dm_v1 (input: 56, output: 9, avg: 0.000ms, count: 0)

# Test inference
rcbot ml_model_test hl2dm_v1
# Output:
# [ML] Test inference successful
# [ML]   Inference time: 0.342ms

# Benchmark performance
rcbot ml_model_benchmark hl2dm_v1 1000
# Output:
# [ML] Benchmark results (1000 iterations):
# [ML]   Average: 0.342ms
# [ML]   Min: 0.298ms
# [ML]   Max: 0.487ms
```

### Step 3: Enable ML Control

**Single bot**:
```bash
rcbot addbot        # Add a bot (returns bot index, e.g., 0)
rcbot ml_enable 0 hl2dm_v1  # Enable ML control for bot 0
```

**All bots**:
```bash
rcbot addbot
rcbot addbot
rcbot addbot
rcbot ml_enable_all hl2dm_v1  # Enable ML for all bots
```

**Check status**:
```bash
rcbot ml_status
# Output:
# [ML] ML Control Status:
# [ML]   Total bots with controllers: 3
# [ML]   ML-controlled bots: 3
# [ML]   Avg inference time: 0.345 ms
# [ML]   Bot 0 (MLBot): ML (model: hl2dm_v1)
# [ML]   Bot 1 (MLBot): ML (model: hl2dm_v1)
# [ML]   Bot 2 (MLBot): ML (model: hl2dm_v1)
```

### Step 4: Observe Bot Behavior

Join the server and watch the ML-controlled bots:
- Do they move naturally?
- Do they engage enemies?
- Do they collect health/ammo?
- Do they use weapons effectively?

---

## Testing and Evaluation

### A/B Testing: ML vs Rule-Based

**Setup**:
```bash
# Add 2 bots: one ML, one rule-based
rcbot addbot  # Bot 0
rcbot addbot  # Bot 1

rcbot ml_enable 0 hl2dm_v1  # ML bot
# Bot 1 uses default rule-based AI

# Play a match and compare scores
```

**Metrics to compare**:
1. **K/D Ratio** (kills/deaths)
2. **Score per minute**
3. **Accuracy** (hits/shots)
4. **Movement quality** (subjective)
5. **Decision-making** (subjective)

### Performance Monitoring

```bash
# Check detailed status for specific bot
rcbot ml_status 0
# Output:
# [ML] Bot 0: ML control ENABLED
# [ML]   Model: hl2dm_v1
# [ML]   Inferences: 5432
# [ML]   Avg time: 0.345 ms
# [ML]   Last time: 0.298 ms
```

**Red flags**:
- Avg time > 2.0ms (performance issues)
- Bot getting stuck (feature extraction bugs)
- Bot not shooting (button threshold too high)
- Bot spinning (aim delta too large)

### Disable ML Control

```bash
# Disable single bot
rcbot ml_disable 0

# Disable all bots
rcbot ml_disable_all
```

---

## Troubleshooting

### Problem: Model won't load

**Symptoms**:
```
[ML] Error: Failed to load ONNX model
```

**Solutions**:
1. Check ONNX Runtime is installed (see `docs/ONNX_SETUP.md`)
2. Verify file path is correct
3. Check file permissions
4. Verify ONNX model is valid:
   ```bash
   python3 -c "import onnx; onnx.checker.check_model(onnx.load('model.onnx'))"
   ```

### Problem: Inference too slow

**Symptoms**:
```
[ML] ONNX inference speed: 3.456ms (target: <0.5ms)
  ✗ TOO SLOW - Consider smaller model
```

**Solutions**:
1. Use smaller model:
   ```bash
   python3 tools/train_hl2dm_behavior_clone.py \
       --hidden-sizes 64 32 \
       --data "data/ml/*.json"
   ```

2. Enable ONNX optimizations in AMBuildScript:
   ```python
   # Add compiler flags
   cxx.cflags += ['-O3', '-march=native']
   ```

3. Use CPU with fewer threads:
   ```cpp
   // In bot_onnx.cpp
   sessionOptions.SetIntraOpNumThreads(1);
   ```

### Problem: Bot behavior is erratic

**Symptoms**:
- Bot spinning in circles
- Bot shooting at nothing
- Bot walking into walls

**Solutions**:

1. **Check action smoothing**:
   ```cpp
   // Increase smoothing in bot_ml_controller.cpp
   m_fActionSmoothing = 0.5f;  // Default: 0.3
   ```

2. **Adjust button threshold**:
   ```cpp
   // In InterpretButtons(), increase threshold
   const float BUTTON_THRESHOLD = 0.7f;  // Default: 0.5
   ```

3. **Reduce aim sensitivity**:
   ```cpp
   // In InterpretAim(), reduce max turn speed
   const float MAX_TURN_SPEED = 45.0f;  // Default: 90.0
   ```

4. **Collect better training data**:
   - Avoid AFK periods in recordings
   - Include diverse gameplay scenarios
   - Remove bugged sessions (stuck in walls, etc.)

### Problem: Low training accuracy

**Symptoms**:
```
Epoch 50/50 - Train Loss: 0.543210, Val Loss: 0.567890
```

**Solutions**:

1. **Increase model capacity**:
   ```bash
   --hidden-sizes 256 128 64
   ```

2. **Train longer**:
   ```bash
   --epochs 100
   ```

3. **Reduce learning rate**:
   ```bash
   --learning-rate 0.0001
   ```

4. **Add more data**: Collect 20+ hours instead of 10

5. **Check data quality**: Verify recordings contain actual gameplay

---

## Advanced Topics

### Custom Model Architectures

Edit `tools/train_hl2dm_behavior_clone.py`:

```python
class CustomBehaviorCloneModel(nn.Module):
    def __init__(self):
        super().__init__()
        # Larger network with skip connections
        self.layer1 = nn.Linear(56, 256)
        self.layer2 = nn.Linear(256, 256)
        self.layer3 = nn.Linear(256, 128)
        # ... custom architecture ...
```

### Transfer Learning

Use a pre-trained model as starting point:

```bash
python3 tools/train_hl2dm_behavior_clone.py \
    --data "data/ml/new_session_*.json" \
    --checkpoint models/hl2dm_v1/best_model.pt \
    --epochs 20
```

### Multi-Game Support

To train for TF2/DOD/CS:S:

1. Implement game-specific feature extractor:
   ```cpp
   // In bot_features.cpp
   class CTF2FeatureExtractor : public CFeatureExtractor {
       // 96 features for TF2 (includes class-specific info)
   };
   ```

2. Update training script with new feature count:
   ```python
   model = BehaviorCloneModel(input_size=96)  # TF2
   ```

3. Train and deploy as usual

### Reinforcement Learning (Future)

Behavior cloning is just the first step. Future phases:

1. **Phase 1**: Behavior cloning (✓ current implementation)
2. **Phase 2**: Imitation learning with DAgger
3. **Phase 3**: Reinforcement learning (PPO/DQN)
4. **Phase 4**: Multi-agent RL for teamwork

See `roadmaps/IMPLEMENTATION_PLAN.md` for full roadmap.

---

## Summary: Complete Workflow

**Quick reference** for the entire process:

```bash
# 1. Collect data
rcbot ml_record_start
# ... play for 30 min ...
rcbot ml_record_stop
rcbot ml_record_save session_01
rcbot ml_record_export_json session_01

# 2. Train model
python3 tools/train_hl2dm_behavior_clone.py \
    --data "data/ml/*.json" \
    --epochs 50 \
    --output-dir models/hl2dm_v1

# 3. Deploy model
rcbot ml_model_load hl2dm_v1 models/hl2dm_v1/hl2dm_behavior_clone.onnx
rcbot ml_model_test hl2dm_v1

# 4. Enable ML control
rcbot addbot
rcbot ml_enable 0 hl2dm_v1

# 5. Monitor and iterate
rcbot ml_status 0
# If performance is good, deploy to all bots
rcbot ml_enable_all hl2dm_v1
```

---

## Getting Help

- **Documentation**: `roadmaps/ML_IMPLEMENTATION_PROGRESS.md`
- **ONNX Setup**: `docs/ONNX_SETUP.md`
- **Implementation Plan**: `roadmaps/IMPLEMENTATION_PLAN.md`
- **GitHub Issues**: https://github.com/ethanbissbort/rcbot2/issues

---

**Good luck training your ML-powered bots!** 🤖
