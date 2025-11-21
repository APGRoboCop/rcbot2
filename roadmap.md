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
