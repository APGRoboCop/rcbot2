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

## Phase 3: Demoman Mobility Improvements
**Goal:** Allow DemoBots to sticky jump again

**Tasks:**
- Review RCBot2 v1.7-beta versions for sticky jump implementation
- Port/adapt sticky jump logic to current codebase
- Implement sticky jump decision-making in bot AI
- Test sticky jump execution and success rate

**Priority:** Medium - Improves bot skill and mobility for one class

---

## Phase 4: Medic and Spy Behavior Improvements
**Goal:** Improve how Medic and Spy bots behave when interacting with SG Turrets and Healing/Ubering

**Tasks:**
- Analyze current Medic bot healing and uber deployment logic
- Analyze current Spy bot sentry interaction behavior
- Implement smarter sentry detection and avoidance for Spies
- Implement better uber deployment timing for Medics around sentries
- Test Medic uber coordination and Spy sentry navigation

**Priority:** Medium - Improves tactical intelligence for support classes

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

## Phase 6: New Game Modes Support
**Goal:** Add proper support for Zombie Infection maps and Robot Destruction gameplay

**Tasks:**
- Add support for new Zombie Infection TF2 maps (Scream Fortress XV update)
- Implement Robot Destruction gameplay logic (destroying bots when not ubered)
- Test bot behavior in both game modes
- Ensure waypoint compatibility

**Priority:** Medium - Keeps bot compatible with newer TF2 content

---

## Phase 7: Kart Games Support
**Goal:** Make bots understand how to play Kart games from sd_doomsday_event

**Tasks:**
- Analyze Kart game mechanics and objectives
- Implement Kart game-specific navigation and behavior
- Fix bots wandering aimlessly in kart minigames
- Test bot participation in kart races

**Priority:** Low - Niche game mode support

---

## Phase 8: Game Detection and Multi-Game Support
**Goal:** Improve game detection for non-listed Source gamemods and add support for additional games

**Tasks:**
- Improve game detection system for unlisted Source mods
- Add TF2 Classic (TF2C) support
- Add Black Mesa Source support
- Add Counter-Strike: Source support
- Add Synergy support
- Add Dystopia support
- Test bot functionality across all new games

**Priority:** Low - Expands bot compatibility but requires significant work

---

## Implementation Status

- ✅ **Phase 1:** Completed
- ✅ **Phase 2:** Completed
- ⏳ **Phase 3:** Not Started
- ⏳ **Phase 4:** Not Started
- ⏳ **Phase 5:** Not Started
- ⏳ **Phase 6:** Not Started
- ⏳ **Phase 7:** Not Started
- ⏳ **Phase 8:** Not Started
**Last Updated**: 2025-11-21
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
**Status**: In Progress
**Issue**: Engineer bots face their sentry turrets the wrong way
**Impact**: Sentries shoot walls instead of enemies
**Tasks**:
- [ ] Analyze current sentry placement logic in `bot_tf2*.cpp`
- [ ] Fix orientation calculation when placing buildings
- [ ] Add angle validation before placement
- [ ] Test on various maps (payload, attack/defend, CTF)

#### 🟠 Demo Bot Sticky Jumping
**Priority**: High
**Status**: Planned
**Issue**: Demoman bots cannot sticky jump (regression from v1.7-beta)
**Impact**: Limits Demo mobility and map navigation
**Tasks**:
- [ ] Compare current code with v1.7-beta implementation
- [ ] Identify what broke sticky jumping
- [ ] Restore sticky jump behavior
- [ ] Add safety checks to prevent self-damage deaths
- [ ] Test on maps requiring vertical mobility

#### 🟠 Scream Fortress XV Support
**Priority**: High
**Status**: Planned
**Issue**: Bots don't understand new Zombie Infection maps
**Impact**: Broken gameplay on Halloween event maps
**Tasks**:
- [ ] Research Zombie Infection game mode mechanics
- [ ] Add infection state detection
- [ ] Implement zombie vs survivor behaviors
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
**Status**: Planned
**Issue**: `CBotTF2::changeClass()` not implemented
**Impact**: Breaks compatibility with ClassRestrictionsForBots.smx
**Tasks**:
- [ ] Implement `CBotTF2::changeClass()` function
- [ ] Add class restriction awareness
- [ ] Prevent punting when forced to change class
- [ ] Test with SourceMod class restriction plugins
- [ ] Add configuration for auto-balancing classes

---

## Medium-Term Goals (Next 6-12 Months)

### Cross-Game Improvements

#### 🟡 Enhanced Game Detection
**Priority**: Medium
**Status**: Planned
**Issue**: Game detection for non-listed Source mods is unreliable
**Tasks**:
- [ ] Refactor `bot_mods.cpp` game detection
- [ ] Add fallback detection methods
- [ ] Implement game capability detection vs hardcoded lists
- [ ] Add logging for unknown game mods
- [ ] Create configuration override system

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
| Robot Destruction (RD) | ❌ Missing | High | Needs implementation |
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
| Sticky jumping (Demo) | ❌ Broken | High | Regression from v1.7 |
| Rocket jumping (Soldier) | ✅ Complete | - | Works well |
| Airblast (Pyro) | 🔶 Partial | Medium | Basic support |
| **Halloween Events** |
| Halloween maps | 🔶 Partial | Medium | Some work, some don't |
| Zombie Infection (SF XV) | ❌ Missing | High | New mode |
| Kart races | ❌ Missing | Medium | Bots wander |
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
- [ ] Fix all critical TF2 bugs (sentry placement, sticky jump)
- [ ] Add MvM upgrade support
- [ ] Support Scream Fortress XV maps
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
