# BSP Waypoint Generator for HL2DM - Design Document

## Executive Summary

This document describes the design of a standalone tool that parses Source Engine .bsp map files and automatically generates RCBot2 waypoints for **Half-Life 2: Deathmatch (HL2DM)** without requiring the game engine to be running. The tool leverages HL2DM's unique gameplay mechanics including weapon spawns, health/armor chargers, physics objects, and teleporters to create intelligent, game-aware waypoints.

**Primary Target Game: Half-Life 2: Deathmatch**

**Feasibility Assessment: HIGHLY VIABLE**

All necessary components exist and are well-documented:
- BSP file format is fully documented with multiple parsing libraries available
- Industry-standard navmesh generation algorithms exist (Recast/Detour)
- RCBot2's waypoint format and HL2DM entity mappings are well-defined in the codebase

---

## Table of Contents

1. [Background & Motivation](#background--motivation)
2. [HL2DM Game Mechanics Overview](#hl2dm-game-mechanics-overview)
3. [Technical Feasibility](#technical-feasibility)
4. [Architecture Overview](#architecture-overview)
5. [Component Design](#component-design)
6. [HL2DM Entity Mapping](#hl2dm-entity-mapping)
7. [HL2DM Waypoint Flags & Types](#hl2dm-waypoint-flags--types)
8. [Data Flow Pipeline](#data-flow-pipeline)
9. [Implementation Options](#implementation-options)
10. [Development Phases](#development-phases)
11. [Risk Assessment](#risk-assessment)
12. [References & Resources](#references--resources)

---

## Background & Motivation

### Current Waypoint Generation Methods

RCBot2 currently supports waypoint creation through:
1. **Manual placement** - Players use in-game commands to place waypoints
2. **Auto-waypoint mode** - Waypoints are placed as a player walks through the map
3. **In-engine auto-generation** - Uses trace rays and entity detection within the running game

### Limitations of Current Approaches

- Requires the Source Engine to be running
- Manual process is time-consuming and error-prone
- Cannot be automated in CI/CD pipelines
- Cannot batch-process multiple maps
- Requires a player to physically traverse the map
- Cannot run on headless servers without game files

### Proposed Solution

A standalone command-line tool that:
- Parses .bsp files directly without the Source Engine
- Extracts geometry and entity data
- Generates navigation meshes using proven algorithms
- Applies HL2DM-specific logic for weapons, items, and interactables
- Converts navigation data to RCBot2 waypoint format with proper flags
- Outputs valid .rcw waypoint files optimized for HL2DM bot behavior

---

## HL2DM Game Mechanics Overview

Understanding HL2DM's unique gameplay mechanics is essential for generating effective waypoints.

### Core Game Systems

| System | Description | Waypoint Implications |
|--------|-------------|----------------------|
| **Weapon Spawns** | 12 weapon types with respawn timers | High-priority waypoints with weapon metadata |
| **Health/Armor** | HEV suit system with separate health + armor | Health and armor waypoints with priority |
| **Charger Stations** | Reusable health/armor restoration points | USE flag waypoints, higher priority than pickups |
| **Ammo Pickups** | Individual ammo types per weapon | Ammo waypoints linked to weapon types |
| **Ammo Crates** | Bulk dispensers (by weapon type) | USE flag waypoints with crate type metadata |
| **Physics Objects** | Gravity gun manipulation | Not directly waypointed, but affects pathing |
| **Teleporters** | trigger_teleport entities | Entrance/exit waypoint pairs |
| **Ladders** | Vertical navigation | Ladder flag waypoints |
| **Breakables** | Destructible objects | Breakable flag for objects blocking paths |

### HL2DM Player Dimensions

These parameters are critical for navmesh generation:

| Parameter | Value (Hammer Units) | Description |
|-----------|---------------------|-------------|
| Standing Height | 72 | Full player height |
| Crouch Height | 36 | Crouched player height |
| Width/Radius | 32 (16 radius) | Player collision width |
| Step Height | 18 | Maximum automatic step-up |
| Jump Height | 56 | Maximum jump clearance |
| Walkable Slope | 45° | Maximum traversable incline |

---

## Technical Feasibility

### BSP File Format Analysis

The Source Engine BSP format (v19-21 for HL2/HL2DM) contains all necessary data:

| Lump | Content | Use for HL2DM Waypoints |
|------|---------|-------------------------|
| Lump 3 | **Vertices** | 3D vertex positions |
| Lump 1 | **Planes** | Surface plane equations |
| Lump 7 | **Faces** | Polygon face definitions |
| Lump 12 | **Edges** | Edge connectivity |
| Lump 13 | **Surfedges** | Surface edge references |
| Lump 18 | **Brushes** | CSG brush geometry |
| Lump 0 | **Entities** | Weapon spawns, chargers, teleporters |
| Lump 26 | **DispInfo** | Displacement surfaces (terrain) |

### Existing BSP Parsing Libraries

| Library | Language | License | HL2 BSP Support |
|---------|----------|---------|-----------------|
| **bsp_tool** | Python | MIT | Excellent (v19, v20) |
| **ValveBSP** | Python | MIT | Good |
| **Galaco/bsp** | Go | MIT | Excellent |
| **SourcePP** | C++20 | MIT | Excellent |

### Navigation Mesh Generation

**Recast/Detour** is the industry-standard solution:
- Used by Unity, Unreal, Godot, and major AAA games
- Open source (ZLib license)
- Configurable for HL2DM player dimensions
- Well-documented API

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────────────┐
│                HL2DM BSP Waypoint Generator                          │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐           │
│  │  BSP Parser  │───▶│   Geometry   │───▶│   Navmesh    │           │
│  │              │    │  Extractor   │    │  Generator   │           │
│  └──────────────┘    └──────────────┘    └──────────────┘           │
│         │                   │                   │                    │
│         ▼                   ▼                   ▼                    │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐           │
│  │   HL2DM      │    │   Triangle   │    │   Waypoint   │           │
│  │Entity Parser │    │    Mesh      │    │  Converter   │           │
│  └──────────────┘    └──────────────┘    └──────────────┘           │
│         │                                       │                    │
│         ▼                                       ▼                    │
│  ┌──────────────┐                        ┌──────────────┐           │
│  │   Weapon/    │───────────────────────▶│   .rcw       │           │
│  │ Item Analyzer│                        │   Writer     │           │
│  └──────────────┘                        └──────────────┘           │
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘
```

---

## Component Design

### 1. BSP Parser Module

**Purpose:** Read and parse HL2DM .bsp files (BSP version 19-20)

**Input:** .bsp file path
**Output:** Structured BSP data object

```python
class BSPParser:
    def load(self, filepath: str) -> BSPFile
    def get_vertices(self) -> List[Vector3]
    def get_faces(self) -> List[Face]
    def get_brushes(self) -> List[Brush]
    def get_entities(self) -> List[Entity]
    def get_displacements(self) -> List[Displacement]
```

**Implementation Notes:**
- Handle BSP versions 19, 20 (HL2/HL2DM)
- Entity lump parsing is critical for HL2DM gameplay elements
- Extract texture info for ladder/clip detection

### 2. Geometry Extractor Module

**Purpose:** Convert BSP geometry to triangle mesh format

```python
class GeometryExtractor:
    def extract_world_geometry(self, bsp: BSPFile) -> TriangleMesh
    def extract_brush_geometry(self, bsp: BSPFile) -> TriangleMesh
    def process_displacements(self, bsp: BSPFile) -> TriangleMesh
    def filter_non_solid(self, mesh: TriangleMesh) -> TriangleMesh
```

**Filtering Rules:**
- Exclude `CONTENTS_SKY`, `CONTENTS_WATER` brushes
- Exclude `tools/toolstrigger`, `tools/toolsclip` textures
- Include `CONTENTS_SOLID`, `CONTENTS_PLAYERCLIP`

### 3. Navmesh Generator Module

**Purpose:** Generate navigation mesh using HL2DM player dimensions

```python
class NavmeshGenerator:
    def configure(self,
                  agent_height: float = 72.0,      # HL2DM standing height
                  agent_radius: float = 16.0,      # HL2DM player radius
                  agent_climb: float = 18.0,       # HL2DM step height
                  walkable_slope: float = 45.0,    # Max slope angle
                  crouch_height: float = 36.0) -> None
    def generate(self, mesh: TriangleMesh) -> NavigationMesh
```

### 4. HL2DM Entity Analyzer Module

**Purpose:** Parse HL2DM-specific entities and classify them

```python
class HL2DMEntityAnalyzer:
    def parse_entities(self, entity_lump: str) -> List[Entity]
    def find_spawn_points(self) -> List[SpawnPoint]
    def find_weapons(self) -> List[WeaponSpawn]
    def find_health_items(self) -> List[HealthItem]
    def find_armor_items(self) -> List[ArmorItem]
    def find_chargers(self) -> List[Charger]
    def find_ammo(self) -> List[AmmoPickup]
    def find_ammo_crates(self) -> List[AmmoCrate]
    def find_teleporters(self) -> List[Teleporter]
    def find_ladders(self) -> List[Ladder]
    def find_breakables(self) -> List[Breakable]
    def find_buttons(self) -> List[Button]
    def find_doors(self) -> List[Door]
```

### 5. Waypoint Converter Module

**Purpose:** Convert navigation mesh to RCBot2 waypoints with HL2DM flags

```python
class HL2DMWaypointConverter:
    def convert(self, navmesh: NavigationMesh,
                entities: HL2DMEntityData) -> List[Waypoint]
    def place_waypoints_on_mesh(self) -> List[Vector3]
    def place_weapon_waypoints(self, weapons: List[WeaponSpawn]) -> None
    def place_charger_waypoints(self, chargers: List[Charger]) -> None
    def place_teleporter_waypoints(self, teleporters: List[Teleporter]) -> None
    def compute_connections(self) -> None
    def assign_hl2dm_flags(self) -> None
    def assign_weapon_metadata(self) -> None
```

### 6. RCW Writer Module

**Purpose:** Write waypoint data to .rcw file format

```python
class RCWWriter:
    def write(self, filepath: str,
              waypoints: List[Waypoint],
              map_name: str,
              author: str = "BSP-Waypoint-Generator-HL2DM") -> None
```

---

## HL2DM Entity Mapping

### Weapon Entities

| Entity Classname | Waypoint SubType | Priority | Respawn Time |
|------------------|------------------|----------|--------------|
| `weapon_rpg` | `WEAPON_RPG` | 95 | 30.0s |
| `weapon_crossbow` | `WEAPON_CROSSBOW` | 90 | 30.0s |
| `weapon_ar2` | `WEAPON_AR2` | 85 | 30.0s |
| `weapon_shotgun` | `WEAPON_SHOTGUN` | 80 | 30.0s |
| `weapon_357` | `WEAPON_357` | 75 | 30.0s |
| `weapon_smg1` | `WEAPON_SMG1` | 70 | 30.0s |
| `weapon_physcannon` | `WEAPON_PHYSCANNON` | 65 | 30.0s |
| `weapon_frag` | `WEAPON_GRENADE` | 60 | 30.0s |
| `weapon_slam` | `WEAPON_SLAM` | 55 | 30.0s |
| `weapon_pistol` | `WEAPON_PISTOL` | 40 | 30.0s |
| `weapon_stunstick` | `WEAPON_STUNSTICK` | 30 | 30.0s |
| `weapon_crowbar` | `WEAPON_CROWBAR` | 20 | 30.0s |

### Health & Armor Entities

| Entity Classname | Waypoint SubType | Flag | Priority |
|------------------|------------------|------|----------|
| `item_healthkit` | `ITEM_HEALTHKIT` | `W_FL_HEALTH` | Medium |
| `item_healthvial` | `ITEM_HEALTHVIAL` | `W_FL_HEALTH` | Low |
| `item_battery` | `ITEM_BATTERY` | `W_FL_ARMOR` | Medium |
| `item_suit` | `ITEM_SUIT` | `W_FL_ARMOR` | High |

### Charger Entities (Require USE)

| Entity Classname | Description | Flags |
|------------------|-------------|-------|
| `func_healthcharger` | Health recharge station | `W_FL_HEALTH \| W_FL_USE` |
| `func_recharge` | Armor recharge station | `W_FL_ARMOR \| W_FL_USE` |

### Ammunition Entities

| Entity Classname | Waypoint SubType | Associated Weapon |
|------------------|------------------|-------------------|
| `item_ammo_pistol` | `ITEM_AMMO_PISTOL` | Pistol |
| `item_ammo_smg1` | `ITEM_AMMO_SMG1` | SMG1 |
| `item_ammo_ar2` | `ITEM_AMMO_AR2` | AR2 |
| `item_ammo_357` | `ITEM_AMMO_357` | .357 Magnum |
| `item_ammo_crossbow` | `ITEM_AMMO_CROSSBOW` | Crossbow |
| `item_box_buckshot` | `ITEM_AMMO_SHOTGUN` | Shotgun |
| `item_rpg_round` | `ITEM_AMMO_RPG` | RPG |
| `item_ammo_smg1_grenade` | `ITEM_AMMO_GRENADE` | SMG1 Grenade |

### Ammo Crate Detection

Ammo crates use a single entity class `item_ammo_crate` with type determined by model:

| Model Substring | Crate Type | Associated Weapon |
|-----------------|------------|-------------------|
| `ammocrate_ar2` | AR2 Ammo | AR2 |
| `ammocrate_grenade` | Grenade Ammo | SMG1 Grenade |
| `ammocrate_rockets` | Rocket Ammo | RPG |
| `ammocrate_smg1` | SMG1 Ammo | SMG1 |

### Interactable Entities

| Entity Classname | Waypoint Type | Flags |
|------------------|---------------|-------|
| `func_button` | `BUTTON` | `W_FL_USE` |
| `func_rot_button` | `BUTTON` | `W_FL_USE` |
| `func_door` | `DOOR` | `W_FL_USE` (if requires use) |
| `func_door_rotating` | `DOOR` | `W_FL_USE` (if requires use) |
| `func_breakable` | `BREAKABLE` | `W_FL_BREAKABLE` |

### Teleporter Entities

| Entity Classname | Waypoint Type | Flags |
|------------------|---------------|-------|
| `trigger_teleport` (entrance) | `TELEPORT_SOURCE` | `W_FL_TELE_ENTRANCE` |
| `info_teleport_destination` | `TELEPORT_DEST` | `W_FL_TELE_EXIT` |

### Navigation Entities

| Entity Classname | Waypoint Flag | Detection Method |
|------------------|---------------|------------------|
| `func_ladder` | `W_FL_LADDER` | Entity bounds |
| Ladder texture | `W_FL_LADDER` | Texture name pattern |

---

## HL2DM Waypoint Flags & Types

### Standard Waypoint Flags (from bot_waypoint.h)

| Flag Constant | Bit Value | Purpose in HL2DM |
|---------------|-----------|------------------|
| `W_FL_NONE` | 0 | No special flags |
| `W_FL_JUMP` | 1 << 0 | Jump required to traverse |
| `W_FL_CROUCH` | 1 << 1 | Crouch required (low ceiling) |
| `W_FL_UNREACHABLE` | 1 << 2 | Pathfinding skip |
| `W_FL_LADDER` | 1 << 3 | Ladder climbing point |
| `W_FL_HEALTH` | 1 << 8 | Health item/charger location |
| `W_FL_SNIPER` | 1 << 11 | Good crossbow sniping position |
| `W_FL_AMMO` | 1 << 12 | Ammo pickup location |
| `W_FL_TELE_ENTRANCE` | 1 << 16 | Teleporter entrance |
| `W_FL_TELE_EXIT` | 1 << 17 | Teleporter exit |
| `W_FL_LIFT` | 1 << 23 | Elevator/lift platform |
| `W_FL_FALL` | 1 << 25 | Falling hazard ahead |
| `W_FL_BREAKABLE` | 1 << 26 | Breakable object blocking path |
| `W_FL_SPRINT` | 1 << 27 | Good area for sprinting |
| `W_FL_USE` | 1 << 30 | Requires USE key (buttons, chargers) |

### HL2DM Waypoint SubTypes (from bot_waypoint_hl2dm.h)

```cpp
enum EHL2DMWaypointSubType
{
    // Navigation
    SUBTYPE_NONE = 0,

    // Weapons (by priority)
    WEAPON_CROWBAR,        // Priority: 20
    WEAPON_STUNSTICK,      // Priority: 30
    WEAPON_PISTOL,         // Priority: 40
    WEAPON_SLAM,           // Priority: 55
    WEAPON_GRENADE,        // Priority: 60
    WEAPON_PHYSCANNON,     // Priority: 65
    WEAPON_SMG1,           // Priority: 70
    WEAPON_357,            // Priority: 75
    WEAPON_SHOTGUN,        // Priority: 80
    WEAPON_AR2,            // Priority: 85
    WEAPON_CROSSBOW,       // Priority: 90
    WEAPON_RPG,            // Priority: 95

    // Health & Armor
    ITEM_HEALTHKIT,
    ITEM_HEALTHVIAL,
    ITEM_BATTERY,
    ITEM_SUIT,

    // Ammunition
    ITEM_AMMO_PISTOL,
    ITEM_AMMO_SMG1,
    ITEM_AMMO_AR2,
    ITEM_AMMO_357,
    ITEM_AMMO_CROSSBOW,
    ITEM_AMMO_SHOTGUN,
    ITEM_AMMO_RPG,
    ITEM_AMMO_GRENADE,
    ITEM_AMMO_CRATE,

    // Interactables
    BUTTON,
    DOOR,
    BREAKABLE,

    // Teleportation
    TELEPORT_SOURCE,
    TELEPORT_DEST
};
```

### HL2DM Waypoint Metadata Structure

```cpp
struct HL2DMWaypointMetadata
{
    EHL2DMWaypointSubType subType;  // Weapon/item/interactable type
    int iWeaponPriority;            // Priority (0-100) for weapon selection
    float fRespawnTime;             // Item respawn time (-1 = doesn't respawn)
    bool bRequiresUse;              // Requires +USE input
    Vector vEntityOrigin;           // Original entity position
    Vector vUsePosition;            // Optimal approach position for USE
};
```

---

## Data Flow Pipeline

```
┌─────────────┐
│  .bsp file  │
│  (HL2DM)    │
└──────┬──────┘
       │
       ▼
┌──────────────────────────────────────────────────────────────┐
│ STAGE 1: BSP Parsing                                          │
│ ┌────────────┐  ┌────────────┐  ┌────────────┐               │
│ │  Vertices  │  │   Faces    │  │  Entities  │               │
│ │  (Lump 3)  │  │  (Lump 7)  │  │  (Lump 0)  │               │
│ └─────┬──────┘  └─────┬──────┘  └─────┬──────┘               │
└───────┼───────────────┼───────────────┼──────────────────────┘
        │               │               │
        ▼               ▼               │
┌──────────────────────────────────┐    │
│ STAGE 2: Geometry Extraction     │    │
│ ┌────────────────────────────┐   │    │
│ │     Triangle Mesh          │   │    │
│ │  - Solid brushes only      │   │    │
│ │  - No triggers/clips       │   │    │
│ └─────────────┬──────────────┘   │    │
└───────────────┼──────────────────┘    │
                │                       │
                ▼                       ▼
┌──────────────────────────┐  ┌─────────────────────────────────┐
│ STAGE 3: Navmesh Gen     │  │ STAGE 4: HL2DM Entity Analysis  │
│ ┌────────────────────┐   │  │ ┌─────────────────────────────┐ │
│ │  Recast/Detour     │   │  │ │  Weapons (12 types)         │ │
│ │  - Height: 72      │   │  │ │  Health/Armor items         │ │
│ │  - Radius: 16      │   │  │ │  Charger stations           │ │
│ │  - Step: 18        │   │  │ │  Ammo pickups & crates      │ │
│ └─────────┬──────────┘   │  │ │  Teleporters                │ │
└───────────┼──────────────┘  │ │  Buttons/Doors/Breakables   │ │
            │                 │ └────────────┬────────────────┘ │
            │                 └──────────────┼──────────────────┘
            │                                │
            ▼                                ▼
┌──────────────────────────────────────────────────────────────┐
│ STAGE 5: HL2DM Waypoint Conversion                            │
│ ┌────────────────────────────────────────────────────────┐   │
│ │  1. Place waypoints on navmesh polygons                │   │
│ │  2. Add waypoints at weapon spawn locations            │   │
│ │  3. Add waypoints at charger stations (with USE flag)  │   │
│ │  4. Add waypoints at health/armor/ammo pickups         │   │
│ │  5. Create teleporter entrance/exit pairs              │   │
│ │  6. Compute connections (line-of-sight + adjacency)    │   │
│ │  7. Assign HL2DM-specific flags and metadata           │   │
│ │  8. Detect sniper positions (crossbow-friendly)        │   │
│ │  9. Optimize count (max 2048 waypoints)                │   │
│ └────────────────────────────────────────────────────────┘   │
└───────────────────────────────┬──────────────────────────────┘
                                │
                                ▼
┌───────────────────────────────────────────────────────────────┐
│ STAGE 6: Output                                                │
│ ┌─────────────┐  ┌─────────────┐  ┌─────────────────────────┐ │
│ │  .rcw file  │  │  .rcv file  │  │  Debug visualization   │ │
│ │  (waypoints)│  │ (visibility)│  │  (optional .obj mesh)  │ │
│ └─────────────┘  └─────────────┘  └─────────────────────────┘ │
└───────────────────────────────────────────────────────────────┘
```

---

## Implementation Options

### Recommended: Python Implementation

**Rationale for HL2DM:**
1. Fastest path to working prototype
2. `bsp_tool` has excellent HL2 BSP support (version 19-20)
3. Entity parsing is string-based (Python excels at text processing)
4. Easy to prototype weapon priority logic
5. Can be extended to other games later

**Required Libraries:**
- `bsp_tool` - BSP parsing
- `PyRecast` or `recastnavigation-py` - Navmesh generation
- `numpy` - Geometry and vector math
- `struct` - Binary .rcw file I/O

### Alternative: C++ Implementation

**Advantages:**
- Could integrate directly into RCBot2
- Better performance for large maps
- Native Recast/Detour integration

**Libraries:**
- `SourcePP` - BSP parsing
- `recastnavigation` - Native navmesh
- RCBot2 existing codebase for format compatibility

---

## Development Phases

### Phase 1: Core BSP Parsing
- Set up Python project structure
- Integrate bsp_tool library for HL2 BSP (v19-20)
- Extract vertex and face data
- Parse entity lump with HL2DM entity patterns
- Output debug OBJ file for visualization

**Deliverable:** Tool that reads HL2DM .bsp and lists all entities

### Phase 2: Navmesh Generation
- Integrate Recast library
- Configure for HL2DM player dimensions (72h × 32w)
- Generate navmesh from BSP geometry
- Handle displacement surfaces
- Output navmesh visualization

**Deliverable:** Tool that generates navmesh from HL2DM .bsp

### Phase 3: Basic Waypoint Conversion
- Implement waypoint placement algorithm
- Compute waypoint connections
- Assign basic geometry flags (jump, crouch, ladder)
- Implement .rcw file writer
- Test with simple HL2DM maps

**Deliverable:** Tool that generates basic .rcw file

### Phase 4: HL2DM Entity Integration
- Parse all 12 weapon types with priorities
- Add health/armor item waypoints
- Add charger station waypoints with USE flag
- Add ammo pickup waypoints
- Link ammo types to weapons

**Deliverable:** Weapon and item-aware waypoints

### Phase 5: Advanced HL2DM Features
- Teleporter entrance/exit pairing
- Button and door USE waypoints
- Breakable detection
- Sniper position detection (for crossbow)
- Weapon priority metadata

**Deliverable:** Full HL2DM-aware waypoint generation

### Phase 6: Optimization & Testing
- Waypoint density optimization (max 2048)
- Connection validation
- Visibility table generation (.rcv)
- Test against existing HL2DM maps
- Compare with manually-created waypoints
- Bot behavior validation

**Deliverable:** Production-ready HL2DM waypoint generator

---

## Risk Assessment

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| Recast Python bindings insufficient | Medium | High | Fall back to C++ subprocess |
| Complex HL2DM maps exceed waypoint limit | Medium | Medium | Implement intelligent merging by priority |
| Charger USE positions incorrect | Medium | Low | Add offset calculation from entity facing |
| Teleporter pairing fails | Low | Medium | Use `target`/`targetname` entity linking |
| Weapon spawn detection misses items | Low | High | Test against known HL2DM maps, validate entity list |
| Ladder detection inaccurate | Medium | Low | Use both entity and texture detection |

---

## References & Resources

### BSP Format Documentation
- [Valve Developer Wiki - Source BSP File Format](https://developer.valvesoftware.com/wiki/Source_BSP_File_Format)
- HL2DM uses BSP version 19-20

### BSP Parsing Libraries
- [bsp_tool (Python)](https://github.com/snake-biscuits/bsp_tool) - Recommended
- [SourcePP (C++20)](https://sourcepp.org/)

### Navigation Mesh Generation
- [Recast/Detour (C++)](https://github.com/recastnavigation/recastnavigation)

### RCBot2 HL2DM Documentation
- `bot_waypoint_hl2dm.h` - HL2DM waypoint types and metadata
- `bot_hldm_bot.h` - HL2DM bot behaviors and utilities
- `bot_weapons.h` - HL2DM weapon definitions
- `bot_waypoint.h` - Waypoint flag constants

### HL2DM Entity Reference
- [Valve Developer Wiki - HL2DM Entities](https://developer.valvesoftware.com/wiki/Half-Life_2:_Deathmatch)
- [Valve Developer Wiki - Item Entities](https://developer.valvesoftware.com/wiki/Item)

---

## Appendix A: Command-Line Interface Design

```
hl2dm-waypoint-gen [OPTIONS] <input.bsp> [output.rcw]

OPTIONS:
  -a, --author <NAME>     Author name for waypoint file
  -d, --density <FLOAT>   Waypoint density (0.1-1.0, default: 0.5)
  -m, --max-waypoints <N> Maximum waypoints (default: 2048)

  --weapon-priority       Prioritize weapon spawn waypoints
  --include-chargers      Include charger station waypoints (default: on)
  --include-ammo          Include ammo pickup waypoints (default: on)
  --include-teleporters   Include teleporter waypoints (default: on)

  --agent-height <N>      Agent height in units (default: 72)
  --agent-radius <N>      Agent radius in units (default: 16)
  --step-height <N>       Max step height (default: 18)

  --no-entities           Skip entity parsing (geometry only)
  --debug-obj <FILE>      Output debug geometry as OBJ
  --debug-navmesh <FILE>  Output navmesh as OBJ

  -v, --verbose           Verbose output
  -q, --quiet             Quiet mode
  -h, --help              Show help
  --version               Show version

EXAMPLES:
  # Generate waypoints for HL2DM map
  hl2dm-waypoint-gen dm_lockdown.bsp

  # Generate with custom settings
  hl2dm-waypoint-gen -d 0.7 -m 1500 dm_overwatch.bsp custom.rcw

  # Batch process all HL2DM maps
  for f in maps/dm_*.bsp; do hl2dm-waypoint-gen "$f"; done
```

---

## Appendix B: HL2DM Waypoint Flag Detection Heuristics

| Flag | Detection Method |
|------|------------------|
| `W_FL_JUMP` | Navmesh edge height difference > 18 units (step height) |
| `W_FL_CROUCH` | Ceiling height < 72 units but > 36 units |
| `W_FL_LADDER` | Near `func_ladder` entity OR ladder texture detected |
| `W_FL_FALL` | Navmesh edge drops > 200 units |
| `W_FL_HEALTH` | Within 64 units of health item/charger |
| `W_FL_AMMO` | Within 64 units of ammo item/crate |
| `W_FL_SNIPER` | High ground (>256 units above floor) with sight lines >1400 units |
| `W_FL_USE` | Near charger, button, or useable door |
| `W_FL_TELE_ENTRANCE` | Inside `trigger_teleport` volume |
| `W_FL_TELE_EXIT` | At `info_teleport_destination` position |
| `W_FL_SPRINT` | Open area >512 units across, no obstacles |
| `W_FL_BREAKABLE` | Within 128 units of `func_breakable` |

---

## Appendix C: HL2DM Weapon Priority Logic

Bots select weapons based on priority when multiple are available:

```python
WEAPON_PRIORITIES = {
    "weapon_rpg": 95,          # Best damage, limited ammo
    "weapon_crossbow": 90,     # Long range, zoom
    "weapon_ar2": 85,          # Versatile, alt-fire orb
    "weapon_shotgun": 80,      # High close-range damage
    "weapon_357": 75,          # High damage per shot
    "weapon_smg1": 70,         # Good all-around
    "weapon_physcannon": 65,   # Situational but powerful
    "weapon_frag": 60,         # Grenades
    "weapon_slam": 55,         # Mines/thrown
    "weapon_pistol": 40,       # Backup weapon
    "weapon_stunstick": 30,    # Melee
    "weapon_crowbar": 20,      # Last resort melee
}
```

Waypoints for high-priority weapons (>80) should have higher navigation weight to encourage bots to visit them.

---

*Document Version: 2.0*
*Target Game: Half-Life 2: Deathmatch*
*Last Updated: 2025-12-29*
*Author: BSP Waypoint Generator Design Team*
