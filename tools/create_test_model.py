#!/usr/bin/env python3
"""
RCBot2 ML - Create Test ONNX Model

This script creates a simple test ONNX model for verifying the ONNX Runtime
integration in RCBot2. The model is intentionally simple to ensure quick
inference times and easy verification.

Usage:
    python3 tools/create_test_model.py

Output:
    models/test_model.onnx - Simple HL2DM test model (64 input, 10 output)
    models/test_model_small.onnx - Minimal test model (16 input, 4 output)

Requirements:
    pip install torch onnx onnxruntime numpy
"""

import os
import torch
import torch.nn as nn
import numpy as np

class TestModel(nn.Module):
    """
    Simple feedforward neural network for testing ONNX integration
    """
    def __init__(self, input_size=64, hidden_size=32, output_size=10):
        super(TestModel, self).__init__()
        self.net = nn.Sequential(
            nn.Linear(input_size, hidden_size),
            nn.ReLU(),
            nn.Linear(hidden_size, output_size),
            nn.Tanh()  # Output in range [-1, 1]
        )

    def forward(self, x):
        return self.net(x)


class SmallTestModel(nn.Module):
    """
    Minimal model for quick testing
    """
    def __init__(self):
        super(SmallTestModel, self).__init__()
        self.net = nn.Sequential(
            nn.Linear(16, 8),
            nn.ReLU(),
            nn.Linear(8, 4),
            nn.Tanh()
        )

    def forward(self, x):
        return self.net(x)


def create_test_model(output_path, input_size=64, hidden_size=32, output_size=10):
    """
    Create and export a test ONNX model
    """
    print(f"Creating test model: {input_size} -> {hidden_size} -> {output_size}")

    model = TestModel(input_size, hidden_size, output_size)
    model.eval()

    # Create dummy input
    dummy_input = torch.randn(1, input_size)

    # Test forward pass
    with torch.no_grad():
        output = model(dummy_input)
        print(f"  Test forward pass: input shape {dummy_input.shape} -> output shape {output.shape}")

    # Export to ONNX
    torch.onnx.export(
        model,
        dummy_input,
        output_path,
        input_names=["input"],
        output_names=["output"],
        dynamic_axes={
            "input": {0: "batch_size"},
            "output": {0: "batch_size"}
        },
        opset_version=11
    )

    print(f"  Exported to: {output_path}")

    # Verify the exported model
    verify_onnx_model(output_path, dummy_input.numpy())


def verify_onnx_model(model_path, test_input):
    """
    Verify that the exported ONNX model works correctly
    """
    try:
        import onnxruntime as ort
    except ImportError:
        print("  Warning: onnxruntime not installed, skipping verification")
        return

    print(f"  Verifying ONNX model...")

    session = ort.InferenceSession(model_path)

    # Get model metadata
    input_name = session.get_inputs()[0].name
    input_shape = session.get_inputs()[0].shape
    output_name = session.get_outputs()[0].name
    output_shape = session.get_outputs()[0].shape

    print(f"    Input: {input_name} {input_shape}")
    print(f"    Output: {output_name} {output_shape}")

    # Run inference
    result = session.run([output_name], {input_name: test_input})[0]
    print(f"    Inference output shape: {result.shape}")
    print(f"    Sample outputs: {result[0][:5]}")

    # Benchmark
    import time
    num_iterations = 1000
    start = time.time()
    for _ in range(num_iterations):
        session.run([output_name], {input_name: test_input})
    end = time.time()

    avg_time_ms = (end - start) / num_iterations * 1000
    print(f"    Benchmark ({num_iterations} iterations): {avg_time_ms:.3f} ms average")

    if avg_time_ms < 0.5:
        print(f"    Performance: EXCELLENT (HL2DM target met)")
    elif avg_time_ms < 1.0:
        print(f"    Performance: GOOD (TF2 target met)")
    else:
        print(f"    Performance: Acceptable for testing")

    print("  ✓ Model verification complete!")


def main():
    # Create models directory if it doesn't exist
    os.makedirs("models", exist_ok=True)

    print("=" * 60)
    print("RCBot2 ML - Test Model Generator")
    print("=" * 60)
    print()

    # Create standard HL2DM test model
    print("[1/2] Creating standard test model (HL2DM simulation)")
    create_test_model(
        output_path="models/test_model.onnx",
        input_size=64,    # Simulates HL2DM feature vector
        hidden_size=32,   # Small hidden layer for fast inference
        output_size=10    # Bot actions (movement, aim, buttons)
    )
    print()

    # Create small test model
    print("[2/2] Creating minimal test model")
    model = SmallTestModel()
    model.eval()
    dummy_input = torch.randn(1, 16)

    torch.onnx.export(
        model,
        dummy_input,
        "models/test_model_small.onnx",
        input_names=["input"],
        output_names=["output"],
        dynamic_axes={
            "input": {0: "batch_size"},
            "output": {0: "batch_size"}
        },
        opset_version=11
    )

    print(f"  Exported to: models/test_model_small.onnx")
    verify_onnx_model("models/test_model_small.onnx", dummy_input.numpy())
    print()

    print("=" * 60)
    print("✓ All models created successfully!")
    print("=" * 60)
    print()
    print("Test the models in RCBot2:")
    print("  1. rcbot ml_model_load test models/test_model.onnx")
    print("  2. rcbot ml_model_test test")
    print("  3. rcbot ml_model_benchmark test 1000")
    print()


if __name__ == "__main__":
    main()
