#!/usr/bin/env python3
"""
RCBot2 Behavior Cloning Training Pipeline for HL2DM

This script trains a neural network to imitate human gameplay behavior
by learning from recorded gameplay sessions.

Usage:
    # Train from single recording
    python3 train_hl2dm_behavior_clone.py --data data/ml/recording.json

    # Train from multiple recordings
    python3 train_hl2dm_behavior_clone.py --data data/ml/*.json

    # Resume training from checkpoint
    python3 train_hl2dm_behavior_clone.py --data data/ml/*.json --checkpoint models/checkpoint.pt

Requirements:
    pip install torch numpy onnx onnxruntime tqdm
"""

import argparse
import json
import glob
import os
import sys
from typing import List, Dict, Tuple
import numpy as np
from tqdm import tqdm

try:
    import torch
    import torch.nn as nn
    import torch.optim as optim
    from torch.utils.data import Dataset, DataLoader
    import onnx
    import onnxruntime as ort
except ImportError as e:
    print(f"Error: Missing required package: {e}")
    print("Install with: pip install torch numpy onnx onnxruntime tqdm")
    sys.exit(1)


# ============================================================================
# Dataset
# ============================================================================

class BehaviorCloneDataset(Dataset):
    """
    Dataset for behavior cloning from RCBot2 recordings

    Each sample is (feature_vector, action_vector) where:
    - feature_vector: 56 features (HL2DM game state)
    - action_vector: 9 outputs (movement + buttons)
    """

    def __init__(self, recording_files: List[str], sequence_length: int = 1):
        """
        Args:
            recording_files: List of JSON recording files
            sequence_length: Number of frames to use as input (1 = single frame)
        """
        self.sequence_length = sequence_length
        self.features = []
        self.actions = []

        print(f"Loading {len(recording_files)} recording files...")
        for filename in tqdm(recording_files):
            self._load_recording(filename)

        print(f"Loaded {len(self.features)} samples from {len(recording_files)} recordings")

    def _load_recording(self, filename: str):
        """Load a single recording file and extract features/actions"""
        try:
            with open(filename, 'r') as f:
                data = json.load(f)
        except Exception as e:
            print(f"Warning: Failed to load {filename}: {e}")
            return

        if 'frames' not in data:
            print(f"Warning: No frames in {filename}")
            return

        for frame in data['frames']:
            # Extract features (should match CHL2DMFeatureExtractor)
            features = self._extract_features_from_frame(frame)

            # Extract actions
            actions = self._extract_actions_from_frame(frame)

            if features is not None and actions is not None:
                self.features.append(features)
                self.actions.append(actions)

    def _extract_features_from_frame(self, frame: Dict) -> np.ndarray:
        """
        Extract 56 features from frame data
        Must match CHL2DMFeatureExtractor::Extract() output
        """
        try:
            features = []

            # [0-11] Self State (12 features)
            features.append(frame.get('health', 0) / 100.0)  # Normalized health
            features.append(frame.get('armor', 0) / 100.0)   # Normalized armor

            # Position (normalized to map bounds, roughly -4096 to 4096)
            pos = frame.get('position', {'x': 0, 'y': 0, 'z': 0})
            features.append(pos.get('x', 0) / 4096.0)
            features.append(pos.get('y', 0) / 4096.0)
            features.append(pos.get('z', 0) / 4096.0)

            # Velocity (normalized to ~600 units/sec max)
            vel = frame.get('velocity', {'x': 0, 'y': 0, 'z': 0})
            features.append(np.clip(vel.get('x', 0) / 600.0, -1, 1))
            features.append(np.clip(vel.get('y', 0) / 600.0, -1, 1))
            features.append(np.clip(vel.get('z', 0) / 600.0, -1, 1))

            # Weapon and ammo
            features.append(frame.get('current_weapon_id', 0) / 50.0)  # Weapon IDs usually < 50
            features.append(min(frame.get('primary_ammo', 0) / 100.0, 1.0))
            features.append(float(frame.get('on_ground', 1)))

            # [12-35] Enemies (24 features = 4 enemies × 6 features)
            visible_entities = frame.get('visible_entities', [])
            enemies = [e for e in visible_entities if e.get('is_enemy', False)][:4]

            for i in range(4):
                if i < len(enemies):
                    enemy = enemies[i]
                    # Distance (0-1, closer = higher value)
                    dist = enemy.get('distance', 4096.0)
                    features.append(1.0 - min(dist / 4096.0, 1.0))

                    # Horizontal angle (cos, sin)
                    h_angle = enemy.get('horizontal_angle', 0) * np.pi / 180.0
                    features.append(np.cos(h_angle))
                    features.append(np.sin(h_angle))

                    # Vertical angle (cos, sin)
                    v_angle = enemy.get('vertical_angle', 0) * np.pi / 180.0
                    features.append(np.cos(v_angle))
                    features.append(np.sin(v_angle))

                    # Enemy health estimate
                    features.append(min(enemy.get('health', 100) / 100.0, 1.0))
                else:
                    # No enemy in this slot - pad with zeros
                    features.extend([0.0] * 6)

            # [36-47] Navigation (12 features)
            # Simplified - these would need waypoint system integration
            features.extend([0.0] * 12)  # TODO: Implement navigation features

            # [48-55] Pickups (8 features)
            # Simplified - these would need entity scanning
            features.extend([0.0] * 8)  # TODO: Implement pickup features

            return np.array(features, dtype=np.float32)

        except Exception as e:
            print(f"Warning: Failed to extract features: {e}")
            return None

    def _extract_actions_from_frame(self, frame: Dict) -> np.ndarray:
        """
        Extract action outputs from frame

        Action vector (9 outputs):
        [0-2] Movement: forward/back, left/right, up/down (-1 to 1)
        [3-4] Aim delta: yaw, pitch (-1 to 1)
        [5-8] Buttons: attack, jump, crouch, reload (0 or 1)
        """
        try:
            actions = []

            # Movement (normalized -1 to 1)
            movement = frame.get('movement', {'x': 0, 'y': 0, 'z': 0})
            actions.append(np.clip(movement.get('x', 0) / 450.0, -1, 1))  # Max move speed ~450
            actions.append(np.clip(movement.get('y', 0) / 450.0, -1, 1))
            actions.append(np.clip(movement.get('z', 0) / 450.0, -1, 1))

            # Aim delta (normalized to reasonable turn speed)
            aim = frame.get('aim_delta', {'yaw': 0, 'pitch': 0})
            actions.append(np.clip(aim.get('yaw', 0) / 90.0, -1, 1))    # Max ~90 deg/frame
            actions.append(np.clip(aim.get('pitch', 0) / 90.0, -1, 1))

            # Buttons (binary)
            buttons = frame.get('buttons', 0)
            IN_ATTACK = 1 << 0
            IN_JUMP = 1 << 1
            IN_DUCK = 1 << 2
            IN_RELOAD = 1 << 13

            actions.append(float((buttons & IN_ATTACK) != 0))
            actions.append(float((buttons & IN_JUMP) != 0))
            actions.append(float((buttons & IN_DUCK) != 0))
            actions.append(float((buttons & IN_RELOAD) != 0))

            return np.array(actions, dtype=np.float32)

        except Exception as e:
            print(f"Warning: Failed to extract actions: {e}")
            return None

    def __len__(self):
        return len(self.features)

    def __getitem__(self, idx):
        features = torch.from_numpy(self.features[idx])
        actions = torch.from_numpy(self.actions[idx])
        return features, actions


# ============================================================================
# Model
# ============================================================================

class BehaviorCloneModel(nn.Module):
    """
    Neural network for behavior cloning

    Architecture:
    - Input: 56 features (HL2DM game state)
    - Hidden: 3 layers (128 -> 64 -> 32 neurons)
    - Output: 9 actions (3 continuous movement, 2 continuous aim, 4 binary buttons)

    Design choices:
    - ReLU activation for hidden layers
    - Tanh for continuous outputs (-1 to 1 range)
    - Sigmoid for binary outputs (0 to 1 range)
    - Dropout for regularization
    """

    def __init__(self, input_size: int = 56, hidden_sizes: List[int] = [128, 64, 32],
                 dropout: float = 0.2):
        super().__init__()

        self.input_size = input_size
        self.output_size = 9

        # Build network layers
        layers = []
        prev_size = input_size

        for hidden_size in hidden_sizes:
            layers.append(nn.Linear(prev_size, hidden_size))
            layers.append(nn.ReLU())
            layers.append(nn.Dropout(dropout))
            prev_size = hidden_size

        self.shared_layers = nn.Sequential(*layers)

        # Split heads for different action types
        # Continuous outputs: movement (3) + aim (2) = 5 outputs
        self.continuous_head = nn.Sequential(
            nn.Linear(prev_size, 5),
            nn.Tanh()  # Output range: -1 to 1
        )

        # Binary outputs: buttons = 4 outputs
        self.binary_head = nn.Sequential(
            nn.Linear(prev_size, 4),
            nn.Sigmoid()  # Output range: 0 to 1
        )

    def forward(self, x):
        shared = self.shared_layers(x)
        continuous = self.continuous_head(shared)
        binary = self.binary_head(shared)

        # Concatenate outputs
        return torch.cat([continuous, binary], dim=-1)


# ============================================================================
# Training
# ============================================================================

def train_model(model: nn.Module, train_loader: DataLoader, val_loader: DataLoader,
                epochs: int, learning_rate: float, device: str, checkpoint_dir: str):
    """Train the behavior cloning model"""

    model = model.to(device)

    # Loss functions
    # MSE for continuous outputs, BCE for binary outputs
    mse_loss = nn.MSELoss()
    bce_loss = nn.BCELoss()

    # Optimizer
    optimizer = optim.Adam(model.parameters(), lr=learning_rate)

    # Learning rate scheduler
    scheduler = optim.lr_scheduler.ReduceLROnPlateau(optimizer, mode='min',
                                                       factor=0.5, patience=5)

    best_val_loss = float('inf')

    print(f"\nTraining on {device}")
    print(f"Epochs: {epochs}, Learning rate: {learning_rate}")
    print(f"Train samples: {len(train_loader.dataset)}, Val samples: {len(val_loader.dataset)}")
    print("-" * 60)

    for epoch in range(epochs):
        # Training phase
        model.train()
        train_loss = 0.0

        for features, actions in tqdm(train_loader, desc=f"Epoch {epoch+1}/{epochs}"):
            features = features.to(device)
            actions = actions.to(device)

            # Forward pass
            predictions = model(features)

            # Compute loss (weighted by action type)
            continuous_loss = mse_loss(predictions[:, :5], actions[:, :5])
            binary_loss = bce_loss(predictions[:, 5:], actions[:, 5:])
            loss = continuous_loss + 0.5 * binary_loss  # Weight binary loss lower

            # Backward pass
            optimizer.zero_grad()
            loss.backward()
            optimizer.step()

            train_loss += loss.item()

        train_loss /= len(train_loader)

        # Validation phase
        model.eval()
        val_loss = 0.0

        with torch.no_grad():
            for features, actions in val_loader:
                features = features.to(device)
                actions = actions.to(device)

                predictions = model(features)

                continuous_loss = mse_loss(predictions[:, :5], actions[:, :5])
                binary_loss = bce_loss(predictions[:, 5:], actions[:, 5:])
                loss = continuous_loss + 0.5 * binary_loss

                val_loss += loss.item()

        val_loss /= len(val_loader)

        # Update learning rate
        scheduler.step(val_loss)

        print(f"Epoch {epoch+1}/{epochs} - Train Loss: {train_loss:.6f}, "
              f"Val Loss: {val_loss:.6f}, LR: {optimizer.param_groups[0]['lr']:.6f}")

        # Save checkpoint if best model
        if val_loss < best_val_loss:
            best_val_loss = val_loss
            checkpoint_path = os.path.join(checkpoint_dir, 'best_model.pt')
            torch.save({
                'epoch': epoch,
                'model_state_dict': model.state_dict(),
                'optimizer_state_dict': optimizer.state_dict(),
                'val_loss': val_loss,
            }, checkpoint_path)
            print(f"  → Saved best model (val_loss: {val_loss:.6f})")

        # Save periodic checkpoint
        if (epoch + 1) % 10 == 0:
            checkpoint_path = os.path.join(checkpoint_dir, f'checkpoint_epoch_{epoch+1}.pt')
            torch.save({
                'epoch': epoch,
                'model_state_dict': model.state_dict(),
                'optimizer_state_dict': optimizer.state_dict(),
                'val_loss': val_loss,
            }, checkpoint_path)

    print(f"\nTraining complete. Best validation loss: {best_val_loss:.6f}")
    return model


def export_to_onnx(model: nn.Module, output_path: str, input_size: int = 56):
    """Export trained model to ONNX format"""

    model.eval()
    model.to('cpu')

    # Create dummy input
    dummy_input = torch.randn(1, input_size)

    # Export
    torch.onnx.export(
        model,
        dummy_input,
        output_path,
        export_params=True,
        opset_version=11,
        do_constant_folding=True,
        input_names=['features'],
        output_names=['actions'],
        dynamic_axes={
            'features': {0: 'batch_size'},
            'actions': {0: 'batch_size'}
        }
    )

    print(f"Exported ONNX model to {output_path}")

    # Verify ONNX model
    onnx_model = onnx.load(output_path)
    onnx.checker.check_model(onnx_model)
    print("ONNX model verified successfully")

    # Test inference speed
    ort_session = ort.InferenceSession(output_path)
    test_input = np.random.randn(1, input_size).astype(np.float32)

    # Warmup
    for _ in range(10):
        ort_session.run(None, {'features': test_input})

    # Benchmark
    import time
    iterations = 1000
    start = time.time()
    for _ in range(iterations):
        ort_session.run(None, {'features': test_input})
    end = time.time()

    avg_time_ms = (end - start) / iterations * 1000
    print(f"ONNX inference speed: {avg_time_ms:.3f}ms (target: <0.5ms)")

    if avg_time_ms < 0.5:
        print("  ✓ EXCELLENT - Well within target")
    elif avg_time_ms < 1.0:
        print("  ✓ GOOD - Acceptable performance")
    elif avg_time_ms < 2.0:
        print("  ⚠ ACCEPTABLE - May cause slight lag")
    else:
        print("  ✗ TOO SLOW - Consider smaller model")


# ============================================================================
# Main
# ============================================================================

def main():
    parser = argparse.ArgumentParser(description='Train HL2DM behavior cloning model')
    parser.add_argument('--data', type=str, required=True,
                       help='Path to recording files (supports wildcards)')
    parser.add_argument('--epochs', type=int, default=50,
                       help='Number of training epochs (default: 50)')
    parser.add_argument('--batch-size', type=int, default=64,
                       help='Batch size (default: 64)')
    parser.add_argument('--learning-rate', type=float, default=0.001,
                       help='Learning rate (default: 0.001)')
    parser.add_argument('--hidden-sizes', type=int, nargs='+', default=[128, 64, 32],
                       help='Hidden layer sizes (default: 128 64 32)')
    parser.add_argument('--dropout', type=float, default=0.2,
                       help='Dropout rate (default: 0.2)')
    parser.add_argument('--val-split', type=float, default=0.2,
                       help='Validation split (default: 0.2)')
    parser.add_argument('--checkpoint', type=str, default=None,
                       help='Resume from checkpoint')
    parser.add_argument('--output-dir', type=str, default='models',
                       help='Output directory for models (default: models)')
    parser.add_argument('--device', type=str, default='auto',
                       help='Device to use: cpu, cuda, or auto (default: auto)')

    args = parser.parse_args()

    # Determine device
    if args.device == 'auto':
        device = 'cuda' if torch.cuda.is_available() else 'cpu'
    else:
        device = args.device

    # Create output directory
    os.makedirs(args.output_dir, exist_ok=True)

    # Find recording files
    if '*' in args.data:
        recording_files = glob.glob(args.data)
    else:
        recording_files = [args.data]

    if not recording_files:
        print(f"Error: No recording files found matching '{args.data}'")
        sys.exit(1)

    print(f"Found {len(recording_files)} recording files")

    # Load dataset
    dataset = BehaviorCloneDataset(recording_files)

    if len(dataset) == 0:
        print("Error: No valid samples in dataset")
        sys.exit(1)

    # Split into train/val
    val_size = int(len(dataset) * args.val_split)
    train_size = len(dataset) - val_size
    train_dataset, val_dataset = torch.utils.data.random_split(
        dataset, [train_size, val_size]
    )

    # Create data loaders
    train_loader = DataLoader(train_dataset, batch_size=args.batch_size,
                              shuffle=True, num_workers=4)
    val_loader = DataLoader(val_dataset, batch_size=args.batch_size,
                           shuffle=False, num_workers=4)

    # Create model
    model = BehaviorCloneModel(
        input_size=56,
        hidden_sizes=args.hidden_sizes,
        dropout=args.dropout
    )

    print(f"\nModel architecture:")
    print(model)
    print(f"\nTotal parameters: {sum(p.numel() for p in model.parameters()):,}")

    # Resume from checkpoint if specified
    start_epoch = 0
    if args.checkpoint:
        print(f"Loading checkpoint: {args.checkpoint}")
        checkpoint = torch.load(args.checkpoint)
        model.load_state_dict(checkpoint['model_state_dict'])
        start_epoch = checkpoint['epoch'] + 1
        print(f"Resuming from epoch {start_epoch}")

    # Train
    trained_model = train_model(
        model=model,
        train_loader=train_loader,
        val_loader=val_loader,
        epochs=args.epochs,
        learning_rate=args.learning_rate,
        device=device,
        checkpoint_dir=args.output_dir
    )

    # Export to ONNX
    onnx_path = os.path.join(args.output_dir, 'hl2dm_behavior_clone.onnx')
    export_to_onnx(trained_model, onnx_path)

    print(f"\n{'='*60}")
    print("Training complete!")
    print(f"Best model: {os.path.join(args.output_dir, 'best_model.pt')}")
    print(f"ONNX model: {onnx_path}")
    print(f"\nTo deploy in-game:")
    print(f"  1. Copy {onnx_path} to your game server")
    print(f"  2. In-game: rcbot ml_model_load hl2dm {onnx_path}")
    print(f"  3. In-game: rcbot ml_enable <bot_index>")
    print(f"{'='*60}")


if __name__ == '__main__':
    main()
