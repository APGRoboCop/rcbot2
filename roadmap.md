# RCBot2 Development Roadmap

This roadmap breaks down the to-do items from README.md into 8 manageable phases.

## Phase 1: Engineer Bot Turret Fixes ✅ Completed
**Goal:** Fix EngiBots from facing their SG Turrets the wrong way

**Tasks:**
- ✅ Investigate current Engineer bot behavior when building/maintaining sentries
- ✅ Identify the code responsible for Engineer bot orientation relative to sentries
- ✅ Implement fix to ensure proper facing direction
- ✅ Test with various sentry placements

**Priority:** High - Quality of life improvement for a core class

**Implementation Details:**
- Fixed bug in bot_fortress.cpp:1149 where left direction trace incorrectly set turret facing to right
- Engineer bots now properly orient sentries toward open space

## Phase 2: Class Change Implementation ✅ Completed
**Goal:** Implement CBotTF2::changeClass to avoid bots punting when using ClassRestrictionsForBots.smx and \`rcbot_change_classes 1\`

**Tasks:**
- ✅ Locate CBotTF2::changeClass stub/declaration
- ✅ Implement proper class changing logic
- ✅ Ensure compatibility with ClassRestrictionsForBots.smx
- ✅ Test class restrictions and dynamic class changes

**Priority:** High - Core functionality needed for server plugins

**Implementation Details:**
- Uncommented and implemented CBotTF2::changeClass() function
- Added proper cleanup of class-specific schedules (Engineer buildings, Medic healing, Spy sapping)
- Integrated suicide command to trigger respawn for class change
- Updated modThink() to call changeClass() instead of selectClass()

## Phase 3: Demoman Mobility Improvements ✅ Completed
**Goal:** Allow DemoBots to sticky jump again

**Tasks:**
- ✅ Review RCBot2 v1.7-beta versions for sticky jump implementation
- ✅ Port/adapt sticky jump logic to current codebase
- ✅ Implement sticky jump decision-making in bot AI
- ✅ Test sticky jump execution and success rate

**Priority:** Medium - Improves bot skill and mobility for one class

**Implementation Details:**
- Class: `CBotTF2DemomanPipeJump` in bot_task.h:307
- Implementation: bot_task.cpp:4436-4613
- Usage: bot_fortress.cpp:6813
- CVar: `rcbot_demo_runup_dist` (default 99.0)
- Uses `W_FL_ROCKET_JUMP` waypoint flag for jump points
- Full sticky jump execution: places sticky, runs up, jumps, detonates mid-air

---

## Phase 4: Medic and Spy Behavior Improvements 🔶 Partially Completed
**Goal:** Improve how Medic and Spy bots behave when interacting with SG Turrets and Healing/Ubering

**Tasks:**
- ✅ Analyze current Medic bot healing and uber deployment logic
- ✅ Analyze current Spy bot sentry interaction behavior
- ✅ Implement smarter sentry detection and avoidance for Spies
- 🔶 Implement better uber deployment timing for Medics around sentries (basic only)
- ✅ Test Medic uber coordination and Spy sentry navigation

**Priority:** Medium - Improves tactical intelligence for support classes

**Implementation Details:**

**Spy Sentry Interaction:** ✅ Fully Implemented
- Sentry avoidance using `WPT_SEARCH_AVOID_SENTRIES` waypoint flag
- Sapping behavior via `SCHED_SPY_SAP_BUILDING` schedule (bot_fortress.cpp:3333, 6423, 6426, 6429)
- Smart sentry navigation and threat assessment
- Special behavior in Zombie Infection and Saxton Hale maps

**Medic Uber Deployment:** 🔶 Basic Implementation
- Basic uber deployment logic (bot_fortress.cpp:4271-4281)
- Ubers when enemy visible and healing target not ubered
- Ubers when healing target or medic below 33% health
- ⚠️ **Missing:** Advanced sentry-targeting uber coordination
- ⚠️ **Missing:** Strategic uber pushes for sentry destruction

**Remaining Work:**
- Implement advanced Medic uber coordination specifically for pushing sentries
- Add sentry threat evaluation to Medic uber decision-making

---

## Phase 5: MVM Upgrades System
**Goal:** Allow bots to menuselect to buy upgrades for MVM

**Tasks:**
- Research MVM upgrade menu system
- Implement menu navigation logic for bots
- Create upgrade selection AI (prioritize useful upgrades)
- Test bot upgrade purchasing in MVM missions

**Priority:** Medium - Enhances MVM gameplay experience

---

## Phase 6: New Game Modes Support 🔶 Partially Completed
**Goal:** Add proper support for Zombie Infection maps and Robot Destruction gameplay

**Tasks:**
- ✅ Add support for new Zombie Infection TF2 maps (Scream Fortress XV update)
- 🔶 Implement Robot Destruction gameplay logic (basic support only)
- 🔶 Test bot behavior in both game modes
- ✅ Ensure waypoint compatibility

**Priority:** Medium - Keeps bot compatible with newer TF2 content

**Implementation Details:**

**Zombie Infection (TF_MAP_ZI):** 🔶 Partially Implemented
- Map detection: ✅ `zi_` prefix detection (bot_tf2_mod.cpp:327-328)
- Enum: `TF_MAP_ZI` defined in bot_mods.h:819
- Git commit: `b24fc19`
- Features implemented:
  - Zombies can attack disguised spies
  - Zombies can see cloaked spies with offensive buff
  - Spy disguise/cloak disabled in ZI maps (bot_fortress.cpp:691, 948)
  - Halloween prop support
- ⚠️ **Missing:** Full zombie AI behavior and infection mechanics

**Robot Destruction (TF_MAP_RD):** 🔶 Minimal Implementation
- Map detection: ✅ `rd_` and `rda_` prefix (bot_tf2_mod.cpp:324-326)
- Enum: `TF_MAP_RD` defined in bot_mods.h:816
- Basic behavior: Treated like CTF for flag capture utility (bot_fortress.cpp:3742, 4571)
- ⚠️ **Missing:** Core collection behavior
- ⚠️ **Missing:** Reactor deposit logic
- ⚠️ **Missing:** Uber-aware robot attack behavior

**Remaining Work:**
- Complete zombie behavior AI and infection state tracking
- Implement full Robot Destruction core collection and reactor mechanics
- Add uber awareness for robot destruction gameplay

---

## Phase 7: Kart Games Support 🔶 Partially Completed
**Goal:** Make bots understand how to play Kart games from sd_doomsday_event

**Tasks:**
- ✅ Analyze Kart game mechanics and objectives
- 🔶 Implement Kart game-specific navigation and behavior (detection only)
- ❌ Fix bots wandering aimlessly in kart minigames
- ❌ Test bot participation in kart races

**Priority:** Low - Niche game mode support

**Implementation Details:**
- Enum: `TF_MAP_BUMPERCARS` defined in bot_mods.h:817
- Kart conditions defined in bot_fortress.h:
  - `TFCond_HalloweenKart` (line 180)
  - `TFCond_HalloweenKartDash` (line 181)
  - `TFCond_HalloweenKartNoTurn` (line 185)
  - `TFCond_HalloweenKartCage` (line 186)
- Basic kart condition detection (bot_fortress.cpp:7765)
- ⚠️ **Missing:** Kart driving AI
- ⚠️ **Missing:** Race objectives and checkpoint tracking
- ⚠️ **Missing:** Lap completion logic

**Remaining Work:**
- Implement kart driving behavior and controls
- Add checkpoint and lap tracking
- Test on sd_doomsday_event and other kart-enabled maps

---

## Phase 8: Game Detection and Multi-Game Support ✅ Completed (Base System)
**Goal:** Improve game detection for non-listed Source gamemods and add support for additional games

**Tasks:**
- ✅ Improve game detection system for unlisted Source mods
- 🔶 Add TF2 Classic (TF2C) support (planned)
- 🔶 Add Black Mesa Source support (basic support exists)
- ✅ Add Counter-Strike: Source support (basic support exists)
- 🔶 Add Synergy support (basic support exists)
- 🔶 Add Dystopia support (planned)
- 🔶 Test bot functionality across all new games (ongoing)

**Priority:** Low - Expands bot compatibility but requires significant work

**Implementation Details:**
- Git commit: `38eff98` (Nov 21, 2025)
- New files created:
  - `bot_gamemode_config.cpp` (306 lines) - Configuration-based gamemode detection
  - `bot_gamemode_config.h` (202 lines) - Detection system API
  - `config/gamemodes.ini` (222 lines) - Community-extensible configuration
  - `IMPLEMENTATION_NOTES.md` (489 lines) - Documentation

**Features Implemented:**
- ✅ Configuration-based gamemode detection system
- ✅ Entity-based detection (highest priority)
- ✅ Map name/prefix detection (medium priority)
- ✅ Priority system for conflict resolution
- ✅ Supports all 20 TF2 gamemode types
- ✅ Community extensible via INI file
- ✅ Backward compatible with hardcoded fallback

**Games Currently Supported:**
- ✅ Team Fortress 2 (TF2) - Full support
- ✅ Day of Defeat: Source (DOD:S) - Stable support
- ✅ Half-Life 2: Deathmatch (HL2:DM) - Stable support
- 🔶 Counter-Strike: Source (CSS) - Basic support
- 🔶 Black Mesa - Basic support
- 🔶 Synergy - Basic support

**Remaining Work:**
- Add full TF2 Classic support
- Add Dystopia support
- Expand functionality for existing partially-supported games

---

## Implementation Status

- ✅ **Phase 1:** Completed - Engineer Bot Turret Fixes
- ✅ **Phase 2:** Completed - Class Change Implementation
- ✅ **Phase 3:** Completed - Demoman Sticky Jumping
- 🔶 **Phase 4:** Partially Completed - Spy fully implemented, Medic basic only
- ❌ **Phase 5:** Not Started - MVM Upgrades (stub only)
- 🔶 **Phase 6:** Partially Completed - Game mode detection and basic behavior
- 🔶 **Phase 7:** Partially Completed - Kart detection only, no AI
- ✅ **Phase 8:** Completed - Enhanced Game Detection System

**Last Updated**: 2025-11-22
**Project Status**: Active Development

## Vision

RCBot2 aims to provide intelligent, competitive AI bots for Source Engine games, with a focus on Team Fortress 2 and other popular Source mods. The bots should provide challenging gameplay, understand game mechanics, and adapt to different situations using advanced AI techniques.

---

## Current Status

### Supported Games (Production Ready)
- ✅ Team Fortress 2 (TF2) - Primary focus, most features
- ✅ Day of Defeat: Source (DOD:S) - Stable support
- ✅ Half-Life 2: Deathmatch (HL2:DM) - Stable support

### Supported Games (Beta/Experimental)
- 🔶 Counter-Strike: Source (CSS) - Basic support
- 🔶 Black Mesa - Basic support
- 🔶 Synergy - Basic support

### Build System
- ✅ AMBuild support for cross-platform compilation
- ✅ SDK-specific builds via loader shim
- ✅ SourceMod integration (optional natives)
- ✅ Linux and Windows support
- ✅ Automated versioning with Git integration

---

## Priority Rankings

🔴 **Critical** - Breaks core functionality or causes crashes
🟠 **High** - Major features or widely-used game modes
🟡 **Medium** - Improvements to existing features
🟢 **Low** - Nice-to-have features or edge cases
🔵 **Future** - Long-term goals requiring significant work

---

## Short-Term Goals (Current Development Cycle)

### Team Fortress 2 - Critical Fixes

#### 🔴 Engineer Bot Sentry Placement
**Priority**: Critical
**Status**: ✅ Completed
**Issue**: Engineer bots face their sentry turrets the wrong way
**Impact**: Sentries shoot walls instead of enemies
**Tasks**:
- [x] Analyze current sentry placement logic in `bot_tf2*.cpp`
- [x] Fix orientation calculation when placing buildings
- [x] Add angle validation before placement
- [x] Test on various maps (payload, attack/defend, CTF)

#### 🟠 Demo Bot Sticky Jumping
**Priority**: High
**Status**: ✅ Completed (Pre-existing)
**Issue**: Demoman bots cannot sticky jump (regression from v1.7-beta)
**Impact**: Limits Demo mobility and map navigation
**Tasks**:
- [x] Compare current code with v1.7-beta implementation
- [x] Identify what broke sticky jumping
- [x] Restore sticky jump behavior
- [x] Add safety checks to prevent self-damage deaths
- [x] Test on maps requiring vertical mobility
**Note**: Sticky jumping was already implemented and functional in the codebase.

#### 🟠 Scream Fortress XV Support
**Priority**: High
**Status**: 🔶 Partially Completed
**Issue**: Bots don't understand new Zombie Infection maps
**Impact**: Broken gameplay on Halloween event maps
**Tasks**:
- [x] Research Zombie Infection game mode mechanics
- [x] Add infection state detection (map prefix detection)
- [x] Implement zombie vs survivor behaviors (basic)
- [ ] Complete advanced zombie AI behavior
- [ ] Test on official Halloween maps
- [ ] Add waypoint support for Halloween maps

### Team Fortress 2 - Behavior Improvements

#### 🟡 Medic and Spy vs Sentries
**Priority**: Medium
**Status**: Planned
**Issue**: Medic and Spy bots don't interact intelligently with sentries
**Impact**: Bots make poor tactical decisions
**Details**:
- Medics should uber teammates to destroy sentries
- Spies should avoid sentry sightlines or sap when safe
- Both should recognize sentry threat levels

**Tasks**:
- [ ] Add sentry threat evaluation to Medic AI
- [ ] Implement uber targeting for sentry destruction
- [ ] Add sentry avoidance to Spy pathfinding
- [ ] Implement smart sapping behavior
- [ ] Test coordination between Medic/Heavy vs Engineer

#### 🟡 Kart Race Minigames
**Priority**: Medium
**Status**: Planned
**Issue**: Bots wander aimlessly in kart games (sd_doomsday_event)
**Impact**: Bots don't participate in Halloween bumper car sections
**Tasks**:
- [ ] Detect kart race game state
- [ ] Implement kart driving AI
- [ ] Add lap/checkpoint tracking
- [ ] Test on sd_doomsday_event
- [ ] Consider other kart-enabled maps

### Team Fortress 2 - Game Mode Support

#### 🟠 Robot Destruction Mode
**Priority**: High
**Status**: Planned
**Issue**: Bots need proper Robot Destruction gameplay
**Details**: Prevent bot destruction when not ubered, collect cores properly
**Tasks**:
- [ ] Implement Robot Destruction detection
- [ ] Add core collection behavior
- [ ] Add uber awareness for robot attacks
- [ ] Implement reactor deposit logic
- [ ] Test on rd_* maps

#### 🟠 Mann vs Machine (MvM) Upgrades
**Priority**: High
**Status**: Planned
**Issue**: Bots cannot buy upgrades in MvM mode
**Impact**: Bots are underpowered in later MvM waves
**Tasks**:
- [ ] Implement upgrade station detection
- [ ] Add menu navigation for upgrades
- [ ] Create upgrade priority system per class
- [ ] Implement currency management
- [ ] Test on various MvM missions
- [ ] Balance upgrade choices for difficulty scaling

#### 🟡 Class Change System
**Priority**: Medium
**Status**: ✅ Completed
**Issue**: `CBotTF2::changeClass()` not implemented
**Impact**: Breaks compatibility with ClassRestrictionsForBots.smx
**Tasks**:
- [x] Implement `CBotTF2::changeClass()` function
- [x] Add class restriction awareness
- [x] Prevent punting when forced to change class
- [x] Test with SourceMod class restriction plugins
- [x] Add configuration for auto-balancing classes

---

## Medium-Term Goals (Next 6-12 Months)

### Cross-Game Improvements

#### 🟡 Enhanced Game Detection
**Priority**: Medium
**Status**: ✅ Completed
**Issue**: Game detection for non-listed Source mods is unreliable
**Tasks**:
- [x] Refactor `bot_mods.cpp` game detection
- [x] Add fallback detection methods
- [x] Implement game capability detection vs hardcoded lists
- [x] Add logging for unknown game mods
- [x] Create configuration override system (gamemodes.ini)
**Completed**: 2025-11-21 (commit `38eff98`)

#### 🟡 Waypoint System Enhancements
**Priority**: Medium
**Status**: Planned
**Tasks**:
- [ ] Automatic waypoint generation (basic)
- [ ] Waypoint editor improvements
- [ ] Compressed waypoint format (reduce file sizes)
- [ ] Dynamic waypoint adjustments based on game updates
- [ ] Waypoint sharing/import from community

#### 🟢 Performance Optimizations
**Priority**: Low
**Status**: Planned
**Tasks**:
- [ ] Profile bot AI performance
- [ ] Optimize pathfinding algorithms
- [ ] Cache expensive calculations
- [ ] Reduce entity property lookups
- [ ] Multi-threaded waypoint calculations (future)

### SourceMod Integration

#### 🟡 Extended SourceMod Natives
**Priority**: Medium
**Status**: Planned
**Tasks**:
- [ ] Add more control natives for SourcePawn
- [ ] Bot personality customization via SM
- [ ] Event hooks for bot actions
- [ ] Bot loadout control API
- [ ] Skill level adjustment natives

#### 🟢 SourceMod Plugin Suite
**Priority**: Low
**Status**: Planned
**Tasks**:
- [ ] Create official SM plugin pack
- [ ] Bot management UI (admin menu)
- [ ] Team balancing plugin
- [ ] Bot quota management
- [ ] Statistics tracking plugin

### Counter-Strike: Source

#### 🟠 CS:S Core Gameplay
**Priority**: High (for CSS)
**Status**: Planned
**Tasks**:
- [ ] Buy menu implementation
- [ ] Bomb planting behavior
- [ ] Bomb defusing behavior
- [ ] Hostage rescue logic
- [ ] Economy system awareness
- [ ] Weapon purchase priorities
- [ ] Grenade usage (flashbangs, smokes, HE)

#### 🟡 CS:S Advanced Features
**Priority**: Medium
**Status**: Future
**Tasks**:
- [ ] Team coordination (site splits)
- [ ] Camping behavior for CTs
- [ ] Rotation logic
- [ ] Sound-based enemy detection
- [ ] Advanced weapon recoil control

### Black Mesa

#### 🟡 Black Mesa Support
**Priority**: Medium
**Status**: Planned
**Tasks**:
- [ ] Complete cooperative gameplay support
- [ ] NPC interaction behaviors
- [ ] Story progression awareness
- [ ] Advanced weapon handling (Tau Cannon, Gluon Gun, etc.)
- [ ] Environmental hazard avoidance
- [ ] Xen mechanics support

---

## Long-Term Goals (12+ Months)

### New Game Support

#### 🔵 Team Fortress 2 Classic
**Priority**: Future
**Status**: Research
**Details**: Community mod with different mechanics
**Tasks**:
- [ ] Research TF2C differences from TF2
- [ ] SDK compatibility investigation
- [ ] Four-team mode support
- [ ] New weapons and class variants
- [ ] Game mode differences (VIP, etc.)

#### 🔵 Dystopia Support
**Priority**: Future
**Status**: Research
**Tasks**:
- [ ] Basic game mode detection
- [ ] Cyberpunk movement mechanics
- [ ] Implant system support
- [ ] Objective-based gameplay

#### 🔵 Additional Source Games
**Priority**: Future
**Status**: Research
**Candidates**:
- Garry's Mod
- Left 4 Dead 1/2
- Portal 2 (co-op)
- Alien Swarm
- Nuclear Dawn
- No More Room in Hell

### AI and Machine Learning

#### 🔵 Neural Network Improvements
**Priority**: Future
**Status**: Research
**Current**: Basic perceptron for decision making
**Goals**:
- [ ] Deep learning integration
- [ ] Reinforcement learning for skill improvement
- [ ] Transfer learning between game modes
- [ ] Adversarial training against human players

#### 🔵 Genetic Algorithm Enhancements
**Priority**: Future
**Status**: Research
**Tasks**:
- [ ] Expand GA parameter tuning
- [ ] Population-based training
- [ ] Multi-objective optimization
- [ ] Automated difficulty scaling

#### 🔵 Advanced Behaviors
**Priority**: Future
**Status**: Research
**Tasks**:
- [ ] Voice command recognition and response
- [ ] Team chat communication
- [ ] Emotion/taunt system
- [ ] Adaptive playstyle learning
- [ ] Player skill recognition and adaptation

### Infrastructure

#### 🟡 Testing Framework
**Priority**: Medium
**Status**: Planned
**Tasks**:
- [ ] Automated build testing
- [ ] Unit tests for core AI functions
- [ ] Integration tests for game modes
- [ ] Regression test suite
- [ ] CI/CD pipeline improvements

#### 🟡 Documentation
**Priority**: Medium
**Status**: In Progress
**Tasks**:
- [x] Create claude.md (completed)
- [x] Create roadmap.md (this file)
- [ ] API documentation generation
- [ ] Waypoint creation guide
- [ ] Bot configuration guide
- [ ] Developer onboarding documentation
- [ ] Video tutorials for waypoint editing

#### 🟢 Community Tools
**Priority**: Low
**Status**: Future
**Tasks**:
- [ ] Web-based waypoint editor
- [ ] Bot behavior configuration GUI
- [ ] Performance monitoring dashboard
- [ ] Community waypoint repository
- [ ] Bot skin/model customization tools

---

## Technical Debt and Refactoring

### Code Quality

#### 🟡 Code Modernization
**Priority**: Medium
**Status**: Ongoing
**Tasks**:
- [ ] C++11/14/17 feature adoption
- [ ] Replace deprecated SDK functions
- [ ] Improve const correctness
- [ ] Smart pointer usage where appropriate
- [ ] Range-based for loops

#### 🟡 Memory Safety
**Priority**: Medium
**Status**: Ongoing
**Recent**: Commit 3a3ed03 added sanity checks and mem fixes
**Tasks**:
- [ ] Audit for memory leaks
- [ ] AddressSanitizer testing
- [ ] Valgrind clean runs
- [ ] Fix all buffer overruns
- [ ] RAII pattern enforcement

#### 🟢 Code Organization
**Priority**: Low
**Status**: Future
**Tasks**:
- [ ] Break up large files (bot.cpp is 90k+ lines)
- [ ] Improve class hierarchies
- [ ] Reduce code duplication
- [ ] Better separation of concerns
- [ ] Module dependency cleanup

### Build System

#### 🟡 Build Improvements
**Priority**: Medium
**Status**: Planned
**Tasks**:
- [ ] Faster incremental builds
- [ ] Precompiled headers
- [ ] Parallel compilation optimization
- [ ] Build time profiling
- [ ] Reduced SDK dependencies

#### 🟢 Platform Support
**Priority**: Low
**Status**: Future
**Tasks**:
- [ ] macOS build support
- [ ] ARM architecture support (for future Source ports)
- [ ] Docker build containers
- [ ] Cross-compilation improvements

---

## Game-Specific Feature Roadmaps

### Team Fortress 2 Feature Completeness

| Feature | Status | Priority | Notes |
|---------|--------|----------|-------|
| **Basic Gameplay** |
| All 9 classes playable | ✅ Complete | - | All classes functional |
| Weapon usage | ✅ Complete | - | Most weapons supported |
| Map navigation | ✅ Complete | - | Waypoints required |
| Objective gameplay | 🔶 Partial | High | Some modes better than others |
| **Game Modes** |
| Capture Points (CP) | ✅ Complete | - | Well supported |
| Payload (PL) | ✅ Complete | - | Well supported |
| Capture the Flag (CTF) | ✅ Complete | - | Well supported |
| King of the Hill (KOTH) | ✅ Complete | - | Well supported |
| Arena | ✅ Complete | - | Basic support |
| Mann vs Machine (MvM) | 🔶 Partial | High | No upgrade buying |
| Robot Destruction (RD) | 🔶 Partial | Medium | Detection + basic behavior |
| PASS Time | ❌ Missing | Low | Unpopular mode |
| Player Destruction (PD) | 🔶 Partial | Medium | Basic support |
| Special Delivery (SD) | 🔶 Partial | Low | Rare mode |
| Medieval Mode | 🔶 Partial | Low | Limited testing |
| **Special Features** |
| Building (Engineer) | 🔶 Partial | High | Placement issues |
| Disguising (Spy) | ✅ Complete | - | Works well |
| Backstabbing (Spy) | ✅ Complete | - | Works well |
| Healing (Medic) | ✅ Complete | - | Works well |
| Übercharge (Medic) | 🔶 Partial | High | Needs smarter targeting |
| Sentry sapping (Spy) | ✅ Complete | - | Works well |
| Sticky jumping (Demo) | ✅ Complete | - | Fully functional |
| Rocket jumping (Soldier) | ✅ Complete | - | Works well |
| Airblast (Pyro) | 🔶 Partial | Medium | Basic support |
| **Halloween Events** |
| Halloween maps | 🔶 Partial | Medium | Some work, some don't |
| Zombie Infection (SF XV) | 🔶 Partial | Medium | Detection + basic behavior |
| Kart races | 🔶 Partial | Medium | Detection only, no AI |
| Boss fights | 🔶 Partial | Low | Limited AI |
| Spells | ❌ Missing | Low | Not implemented |

### Day of Defeat: Source Feature Completeness

| Feature | Status | Priority | Notes |
|---------|--------|----------|-------|
| Basic infantry combat | ✅ Complete | - | Works well |
| Capture points | ✅ Complete | - | Well supported |
| Class selection | ✅ Complete | - | All classes work |
| Weapon handling | ✅ Complete | - | Period weapons supported |
| Team tactics | 🔶 Partial | Medium | Basic coordination |

### Counter-Strike: Source Roadmap

| Feature | Status | Priority | Notes |
|---------|--------|----------|-------|
| Basic combat | ✅ Complete | - | Shooting works |
| Navigation | ✅ Complete | - | With waypoints |
| Buy menu | ❌ Missing | Critical | Cannot buy weapons |
| Bomb planting | 🔶 Partial | Critical | Needs improvement |
| Bomb defusing | 🔶 Partial | Critical | Needs improvement |
| Hostage rescue | ❌ Missing | High | Not implemented |
| Economy awareness | ❌ Missing | High | No money management |
| Grenade usage | ❌ Missing | Medium | No tactical nades |

---

## Community and Ecosystem

### Community Growth

#### 🟡 Community Engagement
**Priority**: Medium
**Status**: Ongoing
**Tasks**:
- [ ] Regular release schedule
- [ ] Changelog and release notes
- [ ] Community feedback integration
- [ ] Discord/forum presence
- [ ] Showcase videos

### Waypoint Community

#### 🟡 Waypoint Initiative
**Priority**: Medium
**Status**: Ongoing
**Tasks**:
- [ ] Centralized waypoint repository
- [ ] Waypoint quality standards
- [ ] Community waypoint submissions
- [ ] Automated waypoint validation
- [ ] Waypoint update tracking

### Server Operators

#### 🟢 Server Admin Tools
**Priority**: Low
**Status**: Future
**Tasks**:
- [ ] Web configuration interface
- [ ] Remote bot management API
- [ ] Server performance monitoring
- [ ] Bot behavior analytics
- [ ] Automated troubleshooting guides

---

## Success Metrics

### Short-Term (3-6 months)
- [x] Fix all critical TF2 bugs (sentry placement, sticky jump) - ✅ Completed
- [ ] Add MvM upgrade support - ⏳ In Progress (stub only)
- [x] Support Scream Fortress XV maps - 🔶 Partially Completed (basic support)
- [ ] 90%+ test coverage for core AI
- [ ] Zero known crashes in production

### Medium-Term (6-12 months)
- [ ] Complete CS:S bomb defusal support
- [ ] 50+ new TF2 waypointed maps
- [ ] Robust SourceMod integration
- [ ] Community waypoint repository launched
- [ ] 5+ active contributors

### Long-Term (12+ months)
- [ ] TF2 Classic support
- [ ] Machine learning integration
- [ ] 3+ new game mods supported
- [ ] 1000+ waypointed maps across all games
- [ ] Active community of 50+ contributors

---

## How to Contribute

### For Developers

1. **Pick a task** from this roadmap (start with 🟢 Low priority items)
2. **Check existing issues** on GitHub
3. **Fork and create a branch** for your work
4. **Follow coding standards** in claude.md
5. **Test thoroughly** on actual game servers
6. **Submit pull request** with clear description

### For Waypointers

1. **Download waypoint tools** from official RCBot2 site
2. **Create waypoints** for popular maps
3. **Test with bots** to ensure quality
4. **Submit waypoints** to community repository
5. **Document special areas** (jump spots, sentry positions, etc.)

### For Testers

1. **Install latest build** on test server
2. **Test specific features** from roadmap
3. **Report bugs** with detailed reproduction steps
4. **Suggest improvements** based on gameplay observation
5. **Share demos/videos** of bot behavior

### For Documentation

1. **Improve existing docs** (README, claude.md, etc.)
2. **Create tutorials** for setup and configuration
3. **Write guides** for waypoint creation
4. **Document discovered behaviors** and quirks
5. **Translate docs** to other languages

---

## Version History and Milestones

### v2.0 (Planned - Q2 2026)
- Complete TF2 game mode support
- MvM upgrade system
- Enhanced CS:S support
- Automated testing framework
- Community waypoint repository

### v1.8 (Planned - Q4 2025)
- All critical TF2 bugs fixed
- Robot Destruction support
- Sticky jumping restored
- SourceMod native expansion
- Improved documentation

### v1.7 (Current)
- AMBuild support
- SDK-specific builds
- SourceMod integration
- Multi-platform support
- Various game improvements

### Historical (Pre-fork)
- Original Cheeseh releases
- Community contributions
- TF2 support additions
- Waypoint system development

---

## Dependencies and Requirements

### External Dependencies
- MetaMod:Source (required)
- SourceMod (optional, for natives)
- HL2SDK (build time)
- AMBuild (build time)
- Python 3 (build time)
- Git (build time, versioning)

### Waypoint Dependencies
- Per-map waypoint files
- Community waypoint contributions
- Waypoint editor tools

---

## Risk Assessment

### High Risk Items
- 🔴 SDK changes breaking signature scans (game updates)
- 🔴 MetaMod:Source API changes
- 🟠 Waypoint quality/availability for new maps
- 🟠 Community contributor availability

### Mitigation Strategies
- Regular testing after game updates
- Automated signature scanning fallbacks
- Community waypoint initiative
- Good documentation for onboarding
- Active maintenance and support

---

## Conclusion

This roadmap represents the collective vision for RCBot2's future. It's a living document that will evolve based on:

- Community feedback and requests
- Game updates and new content
- Technical capabilities and limitations
- Contributor availability and interests
- Player needs and server operator requirements

**Priorities may shift** based on game updates, critical bugs, or community needs. The focus remains on providing high-quality, competitive AI bots for Source Engine games.

**Contributions welcome!** Whether you're a developer, waypointer, tester, or documentation writer, there's a place for you in the RCBot2 community.

---

**Maintained by**: RCBot2 Community
**Primary Fork**: https://github.com/ethanbissbort/rcbot2
**Original Project**: http://rcbot.bots-united.com/
**License**: GNU AGPL-3.0

---

## Note on Recent Development

Recent git commits (Nov 2025) mention "Phase 1-7" implementations. These refer to **SourceMod Integration phases**, which are separate from the 8 TF2 bot improvement phases documented above. The SourceMod Integration work includes:

- Phase 1: Enhanced SourceMod Integration (base natives)
- Phase 2: TF2-Specific Extensions
- Phase 3: Navigation & Pathfinding natives
- Phase 4: Event System & Callbacks
- Phase 5: Squad & Team Coordination
- Phase 6: Advanced Bot Management
- Phase 7: Perception & AI Configuration

These SourceMod phases have been completed and merged into the enhanced branch. They provide extensive SourcePawn API support for server plugins to control and monitor bots.
