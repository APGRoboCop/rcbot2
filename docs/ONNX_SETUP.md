# ONNX Runtime Setup for RCBot2

This guide explains how to integrate ONNX Runtime into RCBot2 for ML model inference.

---

## Quick Start (Without ONNX)

By default, RCBot2 can be built **without** ONNX Runtime support. The ONNX code is conditionally compiled and will gracefully handle the absence of ONNX Runtime by showing warning messages when ML model commands are used.

To build without ONNX:
```bash
python3 configure.py --enable-optimize
ambuild
```

The ML recording system will work fine, but model inference commands will not be available.

---

## Full Setup (With ONNX Runtime)

### Step 1: Download ONNX Runtime

Download the appropriate prebuilt ONNX Runtime library for your platform:

#### Linux (x64)
```bash
cd rcbot2
mkdir -p onnxruntime
cd onnxruntime

# Download ONNX Runtime 1.16.3 (or later)
wget https://github.com/microsoft/onnxruntime/releases/download/v1.16.3/onnxruntime-linux-x64-1.16.3.tgz

# Extract
tar -xzf onnxruntime-linux-x64-1.16.3.tgz

# Move contents to parent directory
mv onnxruntime-linux-x64-1.16.3/* .
rmdir onnxruntime-linux-x64-1.16.3
```

Your directory structure should look like:
```
rcbot2/
├── onnxruntime/
│   ├── include/
│   │   └── onnxruntime/
│   │       └── core/
│   │           └── session/
│   │               └── onnxruntime_cxx_api.h
│   └── lib/
│       └── libonnxruntime.so.1.16.3
```

#### Windows (x64)
```powershell
cd rcbot2
mkdir onnxruntime
cd onnxruntime

# Download from GitHub releases page
# https://github.com/microsoft/onnxruntime/releases/download/v1.16.3/onnxruntime-win-x64-1.16.3.zip

# Extract zip file
# Move contents to rcbot2/onnxruntime/
```

Your directory structure should look like:
```
rcbot2/
├── onnxruntime/
│   ├── include/
│   │   └── onnxruntime/
│   │       └── core/
│   │           └── session/
│   │               └── onnxruntime_cxx_api.h
│   └── lib/
│       └── onnxruntime.lib
```

### Step 2: Update AMBuildScript

Edit `AMBuildScript` to enable ONNX support. Add the following after line 100:

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
            '-Wl,-rpath,$ORIGIN'  # Use local libonnxruntime.so
        ]
    elif builder.target_platform == 'windows':
        cxx.linkflags += [
            os.path.join(onnx_path, 'lib', 'onnxruntime.lib')
        ]

    print('ONNX Runtime support: ENABLED')
else:
    print('ONNX Runtime support: DISABLED (onnxruntime/ folder not found)')
```

**Important**: The exact line number may vary. Look for the section where `cxx.cxxincludes` and `cxx.defines` are set, and add the ONNX configuration there.

### Step 3: Build RCBot2 with ONNX

```bash
python3 configure.py --enable-optimize
ambuild
```

You should see "ONNX Runtime support: ENABLED" during configuration.

### Step 4: Deploy ONNX Runtime Library

#### Linux
Copy the shared library to your game mod folder:
```bash
# Find your built plugin
cd objdir/package

# Copy ONNX Runtime library next to the plugin
cp ../../onnxruntime/lib/libonnxruntime.so.1.16.3 addons/rcbot2/bin/

# Create symlink
cd addons/rcbot2/bin/
ln -s libonnxruntime.so.1.16.3 libonnxruntime.so
```

#### Windows
Copy `onnxruntime.dll` to the same folder as `rcbot.dll`:
```
Half-Life 2 Deathmatch/
├── hl2mp/
│   └── addons/
│       └── rcbot2/
│           └── bin/
│               ├── rcbot.dll
│               └── onnxruntime.dll
```

---

## Verification

### Test 1: Create Test Model

```bash
cd rcbot2

# Install Python dependencies
pip install torch onnx onnxruntime numpy

# Generate test models
python3 tools/create_test_model.py
```

This creates:
- `models/test_model.onnx` - Standard test model (64 input → 10 output)
- `models/test_model_small.onnx` - Small model (16 input → 4 output)

### Test 2: Load Model in Game

Start HL2DM with RCBot2 and run:
```
rcbot ml_model_load test models/test_model.onnx
rcbot ml_model_list
rcbot ml_model_test test
rcbot ml_model_benchmark test 1000
```

Expected output:
```
[ML] Model loaded: models/test_model.onnx
[ML]   Input: 64 features
[ML]   Output: 10 values
[ML] Inference successful!
[ML]   Time: 0.123 ms
[ML] Benchmark complete!
[ML]   Average time: 0.125 ms
[ML]   Performance: EXCELLENT (HL2DM target met)
```

---

## Performance Targets

- **HL2DM**: <0.5ms inference time (64 input features)
- **TF2**: <1.0ms inference time (96 input features)
- **Acceptable**: <2.0ms

If your benchmark shows >2ms, consider:
1. Using a smaller model (fewer layers/neurons)
2. Enabling graph optimization in ONNX Runtime
3. Using INT8 quantization
4. Building ONNX Runtime with performance optimizations

---

## Troubleshooting

### "ONNX Runtime not available" error

**Cause**: ONNX Runtime libraries not found at runtime.

**Solution**:
- Ensure `libonnxruntime.so` (Linux) or `onnxruntime.dll` (Windows) is in the same directory as `rcbot.dll`
- Check `LD_LIBRARY_PATH` (Linux) or `PATH` (Windows)

### Build error: "onnxruntime_cxx_api.h: No such file"

**Cause**: ONNX Runtime headers not found during compilation.

**Solution**:
- Verify `onnxruntime/include/` exists
- Check AMBuildScript for correct include path

### Model fails to load

**Cause**: Invalid ONNX model or incompatible version.

**Solution**:
- Re-export model with `opset_version=11` or later
- Verify model with `onnx.checker.check_model()`
- Ensure model has single input and single output

### Inference too slow (>2ms)

**Cause**: Model too large or not optimized.

**Solution**:
- Reduce model size (fewer layers, smaller hidden dimensions)
- Use ONNX optimizer: `onnxoptimizer.optimize(model)`
- Quantize model to INT8
- Enable all ONNX Runtime graph optimizations

---

## Alternative: Build Without ONNX

If you don't need ML inference, you can skip ONNX Runtime entirely:

1. Don't download ONNX Runtime libraries
2. Build normally: `python3 configure.py && ambuild`
3. ML recording will work fine
4. Model inference commands will show "ONNX Runtime not available"

This is useful for:
- Data collection only
- Testing non-ML features
- Platforms without ONNX Runtime support

---

## Next Steps

Once ONNX Runtime is working:

1. **Collect training data**:
   ```
   rcbot ml_record_start
   # Play for 10+ minutes
   rcbot ml_record_stop
   rcbot ml_record_save session_001
   rcbot ml_record_export_json session_001
   ```

2. **Train a model** (see `roadmaps/IMPLEMENTATION_PLAN.md` for training pipeline)

3. **Deploy trained model**:
   ```
   rcbot ml_model_load behavior_clone models/my_trained_model.onnx
   rcbot ml_model_test behavior_clone
   rcbot ml_model_benchmark behavior_clone
   ```

4. **Integrate with bot AI** (Phase 0 Priority #3-4)

---

## Resources

- [ONNX Runtime Releases](https://github.com/microsoft/onnxruntime/releases)
- [ONNX Runtime C++ API](https://onnxruntime.ai/docs/api/c/)
- [PyTorch ONNX Export](https://pytorch.org/docs/stable/onnx.html)
- [RCBot2 ML Roadmap](../roadmaps/IMPLEMENTATION_PLAN.md)

---

**Last Updated**: 2025-11-22
