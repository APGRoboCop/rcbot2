# BSP Waypoint Generator - Design Document

## Executive Summary

This document describes the design of a standalone tool that can parse Source Engine .bsp map files and automatically generate RCBot2 waypoints without requiring the game engine to be running. This approach enables batch processing of maps, CI/CD integration, and waypoint generation on systems without the Source Engine installed.

**Feasibility Assessment: HIGHLY VIABLE**

The research confirms that all necessary components exist and are well-documented:
- BSP file format is fully documented with multiple parsing libraries available
- Industry-standard navmesh generation algorithms exist (Recast/Detour)
- RCBot2's waypoint format is straightforward and well-defined

---

## Table of Contents

1. [Background & Motivation](#background--motivation)
2. [Technical Feasibility](#technical-feasibility)
3. [Architecture Overview](#architecture-overview)
4. [Component Design](#component-design)
5. [Data Flow Pipeline](#data-flow-pipeline)
6. [Implementation Options](#implementation-options)
7. [Recommended Approach](#recommended-approach)
8. [Development Phases](#development-phases)
9. [Risk Assessment](#risk-assessment)
10. [References & Resources](#references--resources)

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
- Converts navigation data to RCBot2 waypoint format
- Outputs valid .rcw waypoint files

---

## Technical Feasibility

### BSP File Format Analysis

The Source Engine BSP format is well-documented and contains all necessary data:

| Lump | Content | Use for Waypoints |
|------|---------|-------------------|
| Lump 3 | **Vertices** | 3D vertex positions |
| Lump 1 | **Planes** | Surface plane equations |
| Lump 7 | **Faces** | Polygon face definitions |
| Lump 12 | **Edges** | Edge connectivity |
| Lump 13 | **Surfedges** | Surface edge references |
| Lump 18 | **Brushes** | CSG brush geometry |
| Lump 19 | **Brushsides** | Brush face definitions |
| Lump 5 | **Nodes** | BSP tree structure |
| Lump 10 | **Leafs** | BSP leaf nodes |
| Lump 14 | **Models** | Brush model bounds |
| Lump 0 | **Entities** | Entity definitions (spawns, objectives) |
| Lump 26 | **DispInfo** | Displacement surfaces |
| Lump 33 | **DispVerts** | Displacement vertices |

### Existing BSP Parsing Libraries

| Library | Language | License | Maturity | Source Engine Support |
|---------|----------|---------|----------|----------------------|
| **bsp_tool** | Python | MIT | Stable (v0.6.0) | Excellent (v20, v21) |
| **ValveBSP** | Python | MIT | Stable | Good |
| **Galaco/bsp** | Go | MIT | Stable | Excellent (v20, v21) |
| **valve-bsp-parser** | C++ | MIT | Stable | Good |
| **SourcePP** | C++20 | MIT | Active | Excellent |

### Navigation Mesh Generation

**Recast/Detour** is the industry-standard solution:
- Used by Unity, Unreal, Godot, and major AAA games
- Open source (ZLib license)
- Takes triangle mesh input
- Outputs convex polygon navigation mesh
- Highly configurable for agent sizes
- Well-documented API

**Algorithm Pipeline:**
1. Voxelization of input geometry
2. Filtering of non-walkable voxels
3. Region detection and segmentation
4. Polygon generation via triangulation
5. Navigation mesh output

### RCBot2 Waypoint Format

The .rcw format is straightforward binary:

```
Header (88 bytes):
├── szFileType[16]     "RCBot2\0"
├── szMapName[64]      Map name
├── iVersion           5 (current version)
├── iNumWaypoints      Waypoint count
└── iFlags             File flags

Author Info (64 bytes, v4+):
├── szAuthor[32]       Original author
└── szModifiedBy[32]   Last modifier

Per Waypoint:
├── m_vOrigin          Vector3 (12 bytes)
├── m_iAimYaw          int (4 bytes)
├── m_iFlags           int (4 bytes) - bitfield
├── m_bUsed            bool (1 byte)
├── iPaths             int (4 bytes) - connection count
├── PathIndices[]      int array - connected waypoint indices
├── m_iArea            int (4 bytes)
└── m_fRadius          float (4 bytes)
```

**Key Waypoint Flags:**
- `W_FL_JUMP` (1<<0) - Jump required
- `W_FL_CROUCH` (1<<1) - Crouch required
- `W_FL_LADDER` (1<<3) - Ladder navigation
- `W_FL_SNIPER` (1<<11) - Sniper position
- `W_FL_SENTRY` (1<<14) - Sentry placement
- `W_FL_HEALTH` (1<<8) - Health pickup
- `W_FL_AMMO` (1<<12) - Ammo pickup
- `W_FL_CAPPOINT` (1<<5) - Capture point
- `W_FL_FLAG` (1<<4) - Flag location

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                    BSP Waypoint Generator                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐       │
│  │  BSP Parser  │───▶│   Geometry   │───▶│   Navmesh    │       │
│  │              │    │  Extractor   │    │  Generator   │       │
│  └──────────────┘    └──────────────┘    └──────────────┘       │
│         │                   │                   │                │
│         ▼                   ▼                   ▼                │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐       │
│  │   Entity     │    │   Triangle   │    │   Waypoint   │       │
│  │   Parser     │    │    Mesh      │    │  Converter   │       │
│  └──────────────┘    └──────────────┘    └──────────────┘       │
│         │                                       │                │
│         ▼                                       ▼                │
│  ┌──────────────┐                        ┌──────────────┐       │
│  │   Entity     │───────────────────────▶│   .rcw       │       │
│  │   Analyzer   │                        │   Writer     │       │
│  └──────────────┘                        └──────────────┘       │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## Component Design

### 1. BSP Parser Module

**Purpose:** Read and parse Source Engine .bsp files

**Input:** .bsp file path
**Output:** Structured BSP data object

**Key Functions:**
```python
class BSPParser:
    def load(self, filepath: str) -> BSPFile
    def get_vertices(self) -> List[Vector3]
    def get_faces(self) -> List[Face]
    def get_edges(self) -> List[Edge]
    def get_brushes(self) -> List[Brush]
    def get_entities(self) -> List[Entity]
    def get_displacements(self) -> List[Displacement]
```

**Implementation Notes:**
- Use existing library (bsp_tool or Galaco/bsp) rather than writing from scratch
- Handle BSP versions 19, 20, 21 (covers most Source games)
- Extract entity lump for spawn points and objectives

### 2. Geometry Extractor Module

**Purpose:** Convert BSP geometry to triangle mesh format

**Input:** Parsed BSP data
**Output:** Triangle mesh (vertices + indices)

**Key Functions:**
```python
class GeometryExtractor:
    def extract_world_geometry(self, bsp: BSPFile) -> TriangleMesh
    def extract_brush_geometry(self, bsp: BSPFile) -> TriangleMesh
    def process_displacements(self, bsp: BSPFile) -> TriangleMesh
    def merge_meshes(self, meshes: List[TriangleMesh]) -> TriangleMesh
    def filter_by_texture(self, mesh: TriangleMesh,
                          exclude: List[str]) -> TriangleMesh
```

**Implementation Notes:**
- Convert face data to triangles (fan triangulation for convex faces)
- Handle displacement surfaces (terrain)
- Filter out sky, clip, and trigger brushes
- Merge all geometry into single mesh

### 3. Navmesh Generator Module

**Purpose:** Generate navigation mesh from triangle geometry

**Input:** Triangle mesh
**Output:** Navigation mesh (convex polygons)

**Key Functions:**
```python
class NavmeshGenerator:
    def configure(self,
                  agent_height: float = 72.0,
                  agent_radius: float = 18.0,
                  agent_climb: float = 18.0,
                  walkable_slope: float = 45.0) -> None
    def generate(self, mesh: TriangleMesh) -> NavigationMesh
    def get_walkable_polygons(self) -> List[ConvexPolygon]
    def get_edges(self) -> List[NavEdge]
```

**Implementation Notes:**
- Use Recast library for generation
- Configure for Source Engine player dimensions
- TF2 player: 72 units tall, ~36 units wide
- Step height: 18 units
- Walkable slope: ~45 degrees

### 4. Entity Analyzer Module

**Purpose:** Parse entity data for waypoint flags

**Input:** Entity lump data
**Output:** Classified entity list with positions

**Key Functions:**
```python
class EntityAnalyzer:
    def parse_entities(self, entity_lump: str) -> List[Entity]
    def find_spawn_points(self) -> List[SpawnPoint]
    def find_health_packs(self) -> List[Vector3]
    def find_ammo_packs(self) -> List[Vector3]
    def find_capture_points(self) -> List[CapturePoint]
    def find_flags(self) -> List[Vector3]
    def find_ladders(self) -> List[Ladder]
    def find_teleporters(self) -> List[Teleporter]
    def find_resupply_cabinets(self) -> List[Vector3]
```

**Entity Class Mappings:**

| Entity Class | Waypoint Flag | Game |
|--------------|---------------|------|
| `info_player_teamspawn` | - | TF2 |
| `item_healthkit_*` | W_FL_HEALTH | TF2 |
| `item_ammopack_*` | W_FL_AMMO | TF2 |
| `func_regenerate` | W_FL_RESUPPLY | TF2 |
| `item_teamflag` | W_FL_FLAG | TF2 |
| `team_control_point` | W_FL_CAPPOINT | TF2 |
| `func_ladder` | W_FL_LADDER | All |
| `hostage_entity` | - | CS:S |
| `func_hostage_rescue` | W_FL_RESCUEZONE | CS:S |

### 5. Waypoint Converter Module

**Purpose:** Convert navigation mesh to RCBot2 waypoints

**Input:** Navigation mesh + Entity data
**Output:** Waypoint list with connections

**Key Functions:**
```python
class WaypointConverter:
    def convert(self, navmesh: NavigationMesh,
                entities: EntityData) -> List[Waypoint]
    def place_waypoints_on_mesh(self, navmesh: NavigationMesh) -> List[Vector3]
    def compute_connections(self, waypoints: List[Waypoint]) -> None
    def assign_flags(self, waypoints: List[Waypoint],
                     entities: EntityData) -> None
    def assign_areas(self, waypoints: List[Waypoint],
                     control_points: List[CapturePoint]) -> None
    def optimize_waypoint_count(self, waypoints: List[Waypoint],
                                 max_count: int = 2048) -> List[Waypoint]
```

**Waypoint Placement Strategy:**
1. Place waypoint at centroid of each navmesh polygon
2. Place additional waypoints at polygon edges for connectivity
3. Place waypoints at entity locations (health, ammo, etc.)
4. Place waypoints at spawn points
5. Merge waypoints that are too close together
6. Ensure maximum of 2048 waypoints (RCBot2 limit)

### 6. RCW Writer Module

**Purpose:** Write waypoint data to .rcw file format

**Input:** Waypoint list
**Output:** Binary .rcw file

**Key Functions:**
```python
class RCWWriter:
    def write(self, filepath: str,
              waypoints: List[Waypoint],
              map_name: str,
              author: str = "BSP-Waypoint-Generator") -> None
    def write_header(self, file, map_name: str,
                     num_waypoints: int) -> None
    def write_author_info(self, file, author: str) -> None
    def write_waypoint(self, file, waypoint: Waypoint) -> None
```

**Binary Format Details:**
```python
# Header
struct.pack('<16s64siii',
    b'RCBot2\0...',      # File type (16 bytes)
    map_name.encode(),    # Map name (64 bytes)
    5,                    # Version
    num_waypoints,        # Count
    0)                    # Flags

# Author info
struct.pack('<32s32s',
    author.encode(),      # Author (32 bytes)
    b'')                  # Modified by (32 bytes)

# Per waypoint
struct.pack('<fff i i ? i',
    origin.x, origin.y, origin.z,  # Position
    aim_yaw,                        # Aim direction
    flags,                          # Waypoint flags
    True,                           # Used flag
    num_paths)                      # Connection count
# Followed by path indices (ints)
struct.pack('<i', area)             # Area ID
struct.pack('<f', radius)           # Radius
```

---

## Data Flow Pipeline

```
┌─────────────┐
│  .bsp file  │
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
│ │  - World brushes           │   │    │
│ │  - Displacement terrain    │   │    │
│ │  - Filtered (no sky/clip)  │   │    │
│ └─────────────┬──────────────┘   │    │
└───────────────┼──────────────────┘    │
                │                       │
                ▼                       ▼
┌──────────────────────────┐  ┌─────────────────────────────┐
│ STAGE 3: Navmesh Gen     │  │ STAGE 4: Entity Analysis    │
│ ┌────────────────────┐   │  │ ┌─────────────────────────┐ │
│ │  Recast/Detour     │   │  │ │  Spawn points           │ │
│ │  - Voxelization    │   │  │ │  Health/ammo packs      │ │
│ │  - Region detect   │   │  │ │  Capture points         │ │
│ │  - Polygon gen     │   │  │ │  Ladders                │ │
│ └─────────┬──────────┘   │  │ │  Objectives             │ │
└───────────┼──────────────┘  │ └────────────┬────────────┘ │
            │                 └──────────────┼──────────────┘
            │                                │
            ▼                                ▼
┌──────────────────────────────────────────────────────────────┐
│ STAGE 5: Waypoint Conversion                                  │
│ ┌────────────────────────────────────────────────────────┐   │
│ │  1. Place waypoints on navmesh polygons                │   │
│ │  2. Add waypoints at entity locations                  │   │
│ │  3. Compute connections (line-of-sight + adjacency)    │   │
│ │  4. Assign flags based on nearby entities              │   │
│ │  5. Assign areas based on control point regions        │   │
│ │  6. Optimize count (merge close waypoints)             │   │
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

### Option A: Python Implementation

**Pros:**
- Fastest development time
- Excellent BSP parsing libraries available (bsp_tool, ValveBSP)
- Easy to prototype and iterate
- Good for scripting and batch processing
- Recast bindings available (PyRecast)

**Cons:**
- Slower execution than native code
- Recast Python bindings less mature than C++ API
- May struggle with very large maps

**Recommended Libraries:**
- `bsp_tool` - BSP parsing
- `PyRecast` or `recastnavigation-py` - Navmesh generation
- `numpy` - Geometry processing
- `struct` - Binary file I/O

**Estimated Development Time:** 2-3 weeks for MVP

### Option B: C++ Implementation

**Pros:**
- Best performance
- Native Recast/Detour integration
- Could potentially be integrated into RCBot2 codebase
- C++ BSP parsers available (valve-bsp-parser, SourcePP)

**Cons:**
- Longer development time
- More complex build system
- Cross-platform considerations

**Recommended Libraries:**
- `SourcePP` or `valve-bsp-parser` - BSP parsing
- `recastnavigation` - Navmesh generation (native)
- Standard library for file I/O

**Estimated Development Time:** 4-6 weeks for MVP

### Option C: Go Implementation

**Pros:**
- Good balance of development speed and performance
- Excellent BSP library (Galaco/bsp)
- Easy cross-compilation
- Single binary distribution

**Cons:**
- Recast bindings less mature
- Smaller ecosystem for game development

**Recommended Libraries:**
- `Galaco/bsp` - BSP parsing
- `go-detour` - Navmesh (partial implementation)
- Standard library for file I/O

**Estimated Development Time:** 3-4 weeks for MVP

### Option D: Hybrid Approach (Python + C++)

**Pros:**
- Use Python for BSP parsing and orchestration
- Use C++ Recast via bindings for navmesh generation
- Best of both worlds

**Cons:**
- More complex dependency management
- Potential binding issues

---

## Recommended Approach

**Recommendation: Option A (Python) for initial implementation**

**Rationale:**
1. Fastest path to working prototype
2. `bsp_tool` is mature and well-maintained
3. Easy to integrate with existing RCBot2 testing infrastructure
4. Can be rewritten in C++ later if performance is critical
5. Python is accessible to more contributors

**Future Consideration:**
Once the Python implementation is proven, consider a C++ port that could be integrated directly into RCBot2 as an offline tool or even runtime generation.

---

## Development Phases

### Phase 1: Core BSP Parsing (Week 1)
- [ ] Set up Python project structure
- [ ] Integrate bsp_tool library
- [ ] Extract vertex and face data
- [ ] Parse entity lump
- [ ] Output debug OBJ file for visualization

**Deliverable:** Tool that can read .bsp and output geometry as .obj

### Phase 2: Navmesh Generation (Week 2)
- [ ] Integrate Recast library (via bindings)
- [ ] Configure for Source Engine player dimensions
- [ ] Generate navmesh from BSP geometry
- [ ] Handle displacement surfaces
- [ ] Output navmesh visualization

**Deliverable:** Tool that generates navmesh from .bsp

### Phase 3: Waypoint Conversion (Week 3)
- [ ] Implement waypoint placement algorithm
- [ ] Compute waypoint connections
- [ ] Assign flags based on geometry (jump, crouch, ladder)
- [ ] Parse entities and assign objective flags
- [ ] Implement .rcw file writer

**Deliverable:** Tool that generates .rcw file from .bsp

### Phase 4: Entity Integration (Week 4)
- [ ] Full entity parsing for all supported games
- [ ] TF2-specific entity handling
- [ ] DOD:S entity handling
- [ ] CS:S entity handling
- [ ] Area assignment for control point maps

**Deliverable:** Game-aware waypoint generation

### Phase 5: Optimization & Polish (Week 5)
- [ ] Waypoint density optimization
- [ ] Connection validation
- [ ] Visibility table generation (.rcv)
- [ ] Command-line interface
- [ ] Batch processing support
- [ ] Documentation

**Deliverable:** Production-ready tool

### Phase 6: Integration & Testing (Week 6)
- [ ] Integration with RCBot2 testing framework
- [ ] CI/CD pipeline integration
- [ ] Comparison with manually-created waypoints
- [ ] Performance benchmarking
- [ ] Bug fixes and refinement

**Deliverable:** Fully integrated and tested tool

---

## Risk Assessment

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| Recast Python bindings insufficient | Medium | High | Fall back to subprocess call to C++ binary |
| BSP format variations break parsing | Low | Medium | Test against multiple games, use bsp_tool's multi-game support |
| Generated waypoints unusable | Medium | High | Implement quality metrics, compare against manual waypoints |
| Performance too slow for large maps | Low | Medium | Use chunked processing, consider C++ rewrite |
| Complex geometry causes navmesh failures | Medium | Medium | Add geometry simplification step |
| Ladder/elevator detection inaccurate | High | Low | Use entity data for ladders, document limitations |

---

## References & Resources

### BSP Format Documentation
- [Valve Developer Wiki - Source BSP File Format](https://developer.valvesoftware.com/wiki/Source_BSP_File_Format)
- [BSP Format - bagthorpe.org](http://www.bagthorpe.org/bob/cofrdrbob/bspformat.html)

### BSP Parsing Libraries
- [bsp_tool (Python)](https://github.com/snake-biscuits/bsp_tool)
- [ValveBSP (Python)](https://github.com/pySourceSDK/ValveBSP)
- [Galaco/bsp (Go)](https://github.com/Galaco/bsp)
- [valve-bsp-parser (C++)](https://github.com/ReactiioN1337/valve-bsp-parser)
- [SourcePP (C++20)](https://sourcepp.org/)

### Navigation Mesh Generation
- [Recast/Detour (C++)](https://github.com/recastnavigation/recastnavigation)
- [recast4j (Java)](https://github.com/ppiastucki/recast4j)
- [go-detour (Go)](https://github.com/arl/go-detour)
- [Navigation Mesh - Wikipedia](https://en.wikipedia.org/wiki/Navigation_mesh)
- [Automatic Navigation Mesh Generation](https://www.accu.org/journals/overload/21/117/golodetz_1838/)

### NAV File Format
- [NAV File Format - Valve Developer Wiki](https://developer.valvesoftware.com/wiki/NAV_(file_format))
- [gonav (Go NAV parser)](https://github.com/mrazza/gonav)

### RCBot2 Documentation
- Waypoint format defined in `bot_waypoint.h`
- File I/O in `bot_waypoint.cpp`
- Constants in `bot_const.h`

---

## Appendix A: Command-Line Interface Design

```
bsp-waypoint-gen [OPTIONS] <input.bsp> [output.rcw]

OPTIONS:
  -g, --game <GAME>       Target game (tf2, dods, css, hl2dm, synergy)
  -a, --author <NAME>     Author name for waypoint file
  -d, --density <FLOAT>   Waypoint density (0.1-1.0, default: 0.5)
  -m, --max-waypoints <N> Maximum waypoints (default: 2048)

  --agent-height <N>      Agent height in units (default: 72)
  --agent-radius <N>      Agent radius in units (default: 18)
  --step-height <N>       Max step height (default: 18)

  --no-entities           Skip entity parsing
  --no-flags              Don't auto-assign waypoint flags
  --debug-obj <FILE>      Output debug geometry as OBJ
  --debug-navmesh <FILE>  Output navmesh as OBJ

  -v, --verbose           Verbose output
  -q, --quiet             Quiet mode
  -h, --help              Show help
  --version               Show version

EXAMPLES:
  # Generate waypoints for TF2 map
  bsp-waypoint-gen -g tf2 cp_dustbowl.bsp

  # Generate with custom settings
  bsp-waypoint-gen -g tf2 -d 0.7 -m 1500 pl_upward.bsp custom.rcw

  # Batch process all maps
  for f in maps/*.bsp; do bsp-waypoint-gen -g tf2 "$f"; done
```

---

## Appendix B: Waypoint Flag Detection Heuristics

| Flag | Detection Method |
|------|------------------|
| `W_FL_JUMP` | Navmesh edge height difference > step_height |
| `W_FL_CROUCH` | Ceiling height < standing height |
| `W_FL_LADDER` | Near `func_ladder` entity or ladder texture |
| `W_FL_FALL` | Navmesh edge drops > 200 units |
| `W_FL_HEALTH` | Near health entity |
| `W_FL_AMMO` | Near ammo entity |
| `W_FL_SNIPER` | High ground with long sight lines |
| `W_FL_SENTRY` | Near sentry hint entity or good coverage |
| `W_FL_CAPPOINT` | Near control point entity |
| `W_FL_FLAG` | Near flag entity |
| `W_FL_RESUPPLY` | Near resupply cabinet entity |
| `W_FL_NOBLU/NORED` | In team-restricted spawn area |

---

## Appendix C: Entity Class Reference

### Team Fortress 2
```
info_player_teamspawn     - Spawn points
item_healthkit_small      - Small health pack
item_healthkit_medium     - Medium health pack
item_healthkit_full       - Full health pack
item_ammopack_small       - Small ammo pack
item_ammopack_medium      - Medium ammo pack
item_ammopack_full        - Full ammo pack
func_regenerate           - Resupply cabinet
item_teamflag             - Intelligence flag
team_control_point        - Control point
trigger_capture_area      - Capture zone
obj_sentrygun            - Sentry gun (hint)
```

### Counter-Strike: Source
```
info_player_terrorist     - T spawn
info_player_counterterrorist - CT spawn
hostage_entity            - Hostage
func_hostage_rescue       - Rescue zone
func_bomb_target          - Bomb site
weapon_* entities         - Weapon spawns
```

### Day of Defeat: Source
```
info_player_axis          - Axis spawn
info_player_allies        - Allies spawn
dod_capture_area          - Capture zone
dod_bomb_target           - Bomb target
```

---

*Document Version: 1.0*
*Last Updated: 2025-12-29*
*Author: BSP Waypoint Generator Design Team*
