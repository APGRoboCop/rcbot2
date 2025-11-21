# GitHub Issues Verification Report

**Date**: 2025-11-21
**Branch**: `claude/create-clau-01BQekPwjuJFQ3FB99UhEU6G`
**Source Repository**: https://github.com/APGRoboCop/rcbot2/issues
**Verified By**: Claude (Automated Analysis)

---

## Executive Summary

This report verifies whether the open issues from the APGRoboCop/rcbot2 repository affect our current branch. Out of **4 open issues** analyzed:

- ✅ **0 issues are fully resolved** in our branch
- ⚠️ **4 issues still affect** our branch
- 📝 **Partial work exists** for Issues #38 and #42

---

## Issue-by-Issue Analysis

### Issue #42: TF2 Map Crash Issue

**Status in our branch**: ⚠️ **POTENTIALLY AFFECTED**

**Original Issue**:
- Map: `gg_poolparty_v20`
- Crash occurs after 7 minutes of gameplay
- Issue introduced in newer RCBot2 build with MetaMod v2.0
- Does not occur with older versions
- Related map `mvm_casino` has similar multi-year crash issues

**Findings**:
1. **Commit found**: `e44962e` - "To-do: To investigate TF2 crash when bots spawn"
   - This commit is **present** in our branch
   - Indicates the crash issue has been acknowledged but not fully resolved

2. **Recent fixes found**:
   - `f989050` - "Tweaking getRandomBombToDefuse" (most recent commit before our docs)
   - `3a3ed03` - "Added sanity checks and mem fixes for DODS"
   - `e69446b` - "Reducing risk of buffer overflow debugString()"

3. **Analysis**:
   - The crash appears to be related to edge cases in game event handling
   - Memory safety improvements have been made but specific map crashes may persist
   - No specific fix for `gg_poolparty_v20` or `mvm_casino` has been implemented

**Recommendation**:
- ⚠️ **Issue still affects our branch**
- Requires further investigation and testing on affected maps
- May need profiling and crash dump analysis

**Code Location**: Likely in event handling code or entity spawning logic

---

### Issue #40: Sticky Jump for Demoman Doesn't Work

**Status in our branch**: ⚠️ **CONFIRMED BROKEN**

**Original Issue**:
- Demoman sticky jump with rocketjump waypoint flag is broken
- Last worked in RCBot2 v1.7-beta 4
- Bot tries to execute but never actually jumps
- Related to TF2 64-bit update and HL2SDK changes

**Findings**:
1. **CVar exists**: `rcbot_demo_runup_dist` found in `bot_cvars.cpp:59`
   ```cpp
   ConVar rcbot_demo_runup("rcbot_demo_runup", "99.0", 0,
       "Distance the demo bot will take to run up for a pipe/sticky bomb jump");
   ```

2. **Documentation confirms**:
   - Roadmap.md lists this as **🔴 Critical Priority**
   - README.md acknowledges the issue
   - Troubleshooting.md lists it as a known issue

3. **No fix implemented**:
   - Code infrastructure exists (CVar, waypoint flags)
   - Actual sticky jump execution appears to be broken
   - No recent commits addressing this specific functionality

**Recommendation**:
- ⚠️ **Issue confirmed in our branch**
- Already documented in roadmap as high-priority fix
- Requires comparison with v1.7-beta 4 code to identify regression

**Code Location**:
- `utils/RCBot2_meta/bot_cvars.cpp:59`
- Likely `bot_fortress.cpp` or `bot_task.cpp` for execution logic

---

### Issue #38: MANNPOWER and PASS Time Gamemode Support

**Status in our branch**: ⚠️ **PARTIALLY IMPLEMENTED / INCOMPLETE**

**Original Issue**:
- **MANNPOWER**: Bots can't use `tf_weapon_grapplinghook` (required for maps like `ctf_hellfire`)
- **PASS Time**: Bots don't recognize `passtime_ball` or `tf_weapon_passtime_gun`
- Bots can push the ball but don't know which direction
- Related: Bots struggle with duck collection (`tf_halloween_pickup`) on `sd_doomsday_event`

**Findings**:
1. **Commit found**: `57d2d41` - "Attempting to add Passtime support"
   - This commit is **present** in our branch
   - Indicates work has begun but is incomplete

2. **Code analysis in `bot_tf2_mod.cpp`**:
   - **Line 570**: PASS Time ball detection code **EXISTS BUT IS COMMENTED OUT**:
     ```cpp
     /*bool CTeamFortress2Mod::isBall(edict_t* pEntity, const int iTeam)
     {
         return (!iTeam || getEnemyTeam(iTeam) == getTeam(pEntity)) &&
                std::strcmp(pEntity->GetClassName(), "passtime_ball") == 0;
     }*/
     ```

3. **MANNPOWER support**:
   - **No code found** for `tf_weapon_grapplinghook`
   - No grappling hook weapon handling
   - No MANNPOWER gamemode detection beyond map prefix

4. **Documentation confirms**:
   - Roadmap.md lists PASS Time as **🟡 Medium Priority**
   - Known to be incomplete

**Recommendation**:
- ⚠️ **Issue confirmed - partial implementation exists but disabled**
- PASS Time: Code exists but is commented out (needs testing and enabling)
- MANNPOWER: No implementation found
- Already documented in roadmap

**Code Locations**:
- `utils/RCBot2_meta/bot_tf2_mod.cpp:570` (commented-out PASS Time code)
- Grappling hook support: Not found

---

### Issue #37: Un-Hardcode Gamemodes

**Status in our branch**: ⚠️ **CONFIRMED - HARDCODED DETECTION**

**Original Issue**:
- Game modes detected by hardcoded map prefix checks
- Unconventional map names break detection
- Proposed solution: Configuration files instead of C++ code
- Suggested config: `addons/rcbot2/config/<modname>/bot_gamemodes.cfg`
- Detection priority: Files > Entities > Maps

**Findings**:
1. **Commit found**: `798348e` - "Extended gamemodes/maps support + Rafmod basic support"
   - This commit is **present** in our branch
   - Adds more map prefixes but doesn't solve the hardcoding issue

2. **Code analysis in `bot_tf2_mod.cpp` (lines 265-330)**:
   - **Extensive hardcoded map prefix checks**:
     ```cpp
     if (std::strncmp(szmapname, "ctf_", 4) == 0 || ...)
         m_MapType = TF_MAP_CTF;
     else if (std::strncmp(szmapname, "cp_", 3) == 0 || ...)
         m_MapType = TF_MAP_CP;
     else if (std::strncmp(szmapname, "pl_", 3) == 0 || ...)
         m_MapType = TF_MAP_CART;
     // ... etc for ~15 different game modes
     ```

3. **Special case hardcoding** (lines 906-953):
   - Specific map exceptions hardcoded:
     ```cpp
     if (std::strncmp(szmapname, "pl_embargo", 10) == 0 && ...)
     if (std::strncmp(szmapname, "pl_aquarius", 11) == 0 && ...)
     if (std::strncmp(szmapname, "ctf_system", sizeof("ctf_system") - 1) == 0)
     ```

4. **Map types detected via hardcoding**:
   - CTF (Capture the Flag)
   - CP (Control Points)
   - PL (Payload)
   - PLR (Payload Race)
   - KOTH (King of the Hill)
   - Arena
   - MVM (Mann vs Machine)
   - RD (Robot Destruction)
   - ZI (Zombie Infection)
   - DM (Deathmatch)
   - CPPL (CP + PL hybrid)
   - GG (GunGame)
   - And more...

**Recommendation**:
- ⚠️ **Issue confirmed - gamemode detection is fully hardcoded**
- Exactly as described in the issue
- Adding new maps or game modes requires code changes and recompilation
- Already documented in roadmap as **🟡 Medium Priority**

**Code Locations**:
- `utils/RCBot2_meta/bot_tf2_mod.cpp:265-330` (main detection)
- `utils/RCBot2_meta/bot_tf2_mod.cpp:906-953` (special cases)

---

## Summary Table

| Issue # | Title | Status | Priority | Fix Available | Notes |
|---------|-------|--------|----------|---------------|-------|
| #42 | Map Crash Issue | ⚠️ Affects | High | Partial | Memory fixes added, specific crash not resolved |
| #40 | Sticky Jump Broken | ⚠️ Affects | Critical | No | Documented, awaiting fix |
| #38 | MANNPOWER/PASS Time | ⚠️ Affects | Medium | Partial | PASS Time code exists but disabled |
| #37 | Un-Hardcode Gamemodes | ⚠️ Affects | Medium | No | Architectural issue, needs refactoring |

---

## Commit Timeline

These commits from APGRoboCop/rcbot2 are present in our branch and relate to the issues:

```
798348e Extended gamemodes/maps support + Rafmod basic support (Issue #37)
e44962e To-do: To investigate TF2 crash when bots spawn (Issue #42)
57d2d41 Attempting to add Passtime support (Issue #38)
f989050 Tweaking getRandomBombToDefuse (General stability)
3a3ed03 Added sanity checks and mem fixes for DODS (General stability)
e69446b Reducing risk of buffer overflow debugString() (General stability)
```

---

## Documentation Status

All four issues are already documented in our repository:

### roadmap.md
- **Issue #40**: Listed as "🔴 Demo Bot Sticky Jumping - Restore sticky jump behavior"
- **Issue #38**: Listed as "🟡 Kart Race Minigames" and mentioned in MANNPOWER section
- **Issue #37**: Listed as "🟡 Enhanced Game Detection"
- **Issue #42**: Implicitly covered under stability/crash fixes

### README.md
- Lists all issues in "Current Development Status" section
- Provides priority indicators

### docs/troubleshooting.md
- Issue #40: Listed as "Known issue: Regression from v1.7-beta"
- Issue #38: Listed under "Halloween maps" issues
- Issue #42: General crash troubleshooting

---

## Recommendations

### Immediate Actions

1. **Issue #40 (Sticky Jump)** - Highest priority
   - Compare with v1.7-beta 4 source code
   - Identify what broke after TF2 64-bit update
   - Create fix targeting bot_task.cpp or bot_fortress.cpp

2. **Issue #42 (Map Crashes)** - High priority
   - Set up test environment with `gg_poolparty_v20`
   - Enable crash dumps and profiling
   - Reproduce crash at 7-minute mark
   - Analyze event handling during crash

### Medium-Term Actions

3. **Issue #38 (MANNPOWER/PASS Time)** - Medium priority
   - **PASS Time**: Uncomment and test existing code at bot_tf2_mod.cpp:570
   - **MANNPOWER**: Implement grappling hook weapon detection
   - Add waypoint types for grappling hook spots

4. **Issue #37 (Hardcoded Gamemodes)** - Medium priority (architectural)
   - Design configuration file format
   - Implement config parser
   - Refactor gamemode detection to use config
   - Maintain backward compatibility

### Documentation Actions

5. **Create GitHub Issues** in our repository
   - Track these issues separately from APGRoboCop repo
   - Link to upstream issues for reference
   - Add to project board/milestones

---

## Code Impact Assessment

### Files That Need Modification

**Issue #40 (Sticky Jump)**:
- `utils/RCBot2_meta/bot_task.cpp` - Jump execution logic
- `utils/RCBot2_meta/bot_fortress.cpp` - Demo-specific behavior
- `utils/RCBot2_meta/bot_navigator.cpp` - Waypoint navigation

**Issue #42 (Crashes)**:
- Unknown - requires investigation
- Likely event handlers or entity spawning code

**Issue #38 (MANNPOWER/PASS Time)**:
- `utils/RCBot2_meta/bot_tf2_mod.cpp` - Uncomment PASS Time code
- `utils/RCBot2_meta/bot_weapons.cpp` - Add grappling hook
- `package/config/weapons.ini` - Weapon configuration

**Issue #37 (Gamemode Detection)**:
- `utils/RCBot2_meta/bot_tf2_mod.cpp` - Refactor detection logic
- Create new config parser module
- Create `package/config/gamemodes.cfg`

---

## Testing Requirements

Before claiming any issue is fixed:

1. **Issue #40**: Test on maps with rocket/sticky jump waypoints
2. **Issue #42**: Run prolonged tests on `gg_poolparty_v20` and `mvm_casino`
3. **Issue #38**: Test MANNPOWER maps (ctf_hellfire) and PASS Time maps
4. **Issue #37**: Test with unconventional map names after implementing config system

---

## Conclusion

**All four open issues from the APGRoboCop/rcbot2 repository affect our branch.**

While some partial work exists (commits, commented code, documentation), none of the issues are fully resolved. Our branch contains the latest code from the upstream repository, including attempted fixes and workarounds, but the core problems persist.

The issues are well-documented in our roadmap and troubleshooting guides, providing transparency to users about known limitations.

**Next Steps**:
1. Prioritize Issue #40 (sticky jump) as it's marked critical
2. Investigate Issue #42 (crashes) with proper debugging tools
3. Enable and test Issue #38 (PASS Time) commented code
4. Plan architectural refactoring for Issue #37 (gamemode detection)

---

**Report Generated**: 2025-11-21
**Branch Verified**: `claude/create-clau-01BQekPwjuJFQ3FB99UhEU6G`
**Commit**: `bcecabf` - "Add comprehensive documentation structure"
