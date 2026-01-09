#!/bin/bash
# Manual 32-bit build script for RCBot2
# This bypasses AMBuild's compiler execution test which fails in gVisor

set -e

RCBOT_ROOT="$(cd "$(dirname "$0")" && pwd)"
SDK_ROOT="${RCBOT_ROOT}/alliedmodders"
MMS_ROOT="${SDK_ROOT}/metamod-source"
SM_ROOT="${SDK_ROOT}/sourcemod"
BUILD_DIR="${RCBOT_ROOT}/build-manual-32"

# Compiler settings for 32-bit
CXX="g++ -m32"
CXXFLAGS="-fPIC -fno-strict-aliasing -fvisibility=hidden -Wall -Wno-unknown-pragmas"
CXXFLAGS+=" -Wno-deprecated -Wno-sign-compare -Wno-unused-variable -Wno-unused-function"
CXXFLAGS+=" -Wno-parentheses -Wno-reorder"
CXXFLAGS+=" -std=c++17 -fno-rtti -O2"
CXXFLAGS+=" -march=pentium4 -msse2 -mfpmath=sse"

# Platform defines
DEFINES="-DPOSIX -DLINUX -D_LINUX -DGNUC -D__linux__"
DEFINES+=" -DCOMPILER_GCC -DNO_MALLOC_OVERRIDE -DNDEBUG"
DEFINES+=" -DNO_HOOK_MALLOC"
DEFINES+=" -DSM_EXT -DENABLE_SOURCEMOD_INTEGRATION"
DEFINES+=" -DGAME_DLL"
DEFINES+=" -DRCBOT_MAXPLAYERS=101"
# SDK engine codes (from hl2sdk-manifests)
DEFINES+=" -DSE_EPISODEONE=1 -DSE_DARKMESSIAH=2 -DSE_ORANGEBOX=3"
DEFINES+=" -DSE_BLOODYGOODTIME=4 -DSE_EYE=5 -DSE_CSS=6"
DEFINES+=" -DSE_HL2DM=7 -DSE_DODS=8 -DSE_SDK2013=9"
DEFINES+=" -DSE_PVKII=10 -DSE_BMS=11 -DSE_TF2=12"
DEFINES+=" -DSE_LEFT4DEAD=13 -DSE_NUCLEARDAWN=14 -DSE_CONTAGION=15"
DEFINES+=" -DSE_LEFT4DEAD2=16 -DSE_ALIENSWARM=17 -DSE_PORTAL2=18"
DEFINES+=" -DSE_BLADE=19 -DSE_INSURGENCY=20 -DSE_DOI=21"
DEFINES+=" -DSE_MCV=22 -DSE_CSGO=23 -DSE_DOTA=24"

# Common include paths
COMMON_INCLUDES=(
    "-I${RCBOT_ROOT}/utils/RCBot2_meta"
    "-I${RCBOT_ROOT}/versioning"
    "-I${RCBOT_ROOT}/rcbot"
    "-I${RCBOT_ROOT}"
    "-I${RCBOT_ROOT}/sourcehook"
    "-I${RCBOT_ROOT}/loader"
    "-I${RCBOT_ROOT}/build-release/includes"
    "-I${MMS_ROOT}/core"
    "-I${MMS_ROOT}/core/sourcehook"
    "-I${SM_ROOT}/sourcepawn/include"
    "-I${SM_ROOT}/public/amtl"
    "-I${SM_ROOT}/public/amtl/amtl"
    "-I${SM_ROOT}/public/extensions"
    "-I${SM_ROOT}/public"
    "-I${RCBOT_ROOT}/sm_ext"
)

# Source files
COMMON_SOURCES=(
    "utils/RCBot2_meta/bot.cpp"
    "utils/RCBot2_meta/bot_accessclient.cpp"
    "utils/RCBot2_meta/bot_buttons.cpp"
    "utils/RCBot2_meta/bot_client.cpp"
    "utils/RCBot2_meta/bot_commands.cpp"
    "utils/RCBot2_meta/bot_configfile.cpp"
    "utils/RCBot2_meta/bot_coop.cpp"
    "utils/RCBot2_meta/bot_css_bot.cpp"
    "utils/RCBot2_meta/bot_css_buying.cpp"
    "utils/RCBot2_meta/bot_css_mod.cpp"
    "utils/RCBot2_meta/bot_dod_bot.cpp"
    "utils/RCBot2_meta/bot_dod_mod.cpp"
    "utils/RCBot2_meta/bot_events.cpp"
    "utils/RCBot2_meta/bot_fortress.cpp"
    "utils/RCBot2_meta/bot_ga.cpp"
    "utils/RCBot2_meta/bot_ga_ind.cpp"
    "utils/RCBot2_meta/bot_getprop.cpp"
    "utils/RCBot2_meta/bot_globals.cpp"
    "utils/RCBot2_meta/bot_hl1dmsrc.cpp"
    "utils/RCBot2_meta/bot_hldm_bot.cpp"
    "utils/RCBot2_meta/bot_kv.cpp"
    "utils/RCBot2_meta/bot_menu.cpp"
    "utils/RCBot2_meta/bot_mods.cpp"
    "utils/RCBot2_meta/bot_mtrand.cpp"
    "utils/RCBot2_meta/bot_perceptron.cpp"
    "utils/RCBot2_meta/bot_recorder.cpp"
    "utils/RCBot2_meta/bot_onnx.cpp"
    "utils/RCBot2_meta/bot_features.cpp"
    "utils/RCBot2_meta/bot_ml_controller.cpp"
    "utils/RCBot2_meta/bot_profile.cpp"
    "utils/RCBot2_meta/bot_profiling.cpp"
    "utils/RCBot2_meta/bot_schedule.cpp"
    "utils/RCBot2_meta/bot_tf2_points.cpp"
    "utils/RCBot2_meta/bot_som.cpp"
    "utils/RCBot2_meta/bot_squads.cpp"
    "utils/RCBot2_meta/bot_strings.cpp"
    "utils/RCBot2_meta/bot_synergy.cpp"
    "utils/RCBot2_meta/bot_synergy_mod.cpp"
    "utils/RCBot2_meta/bot_task.cpp"
    "utils/RCBot2_meta/bot_tf2_mod.cpp"
    "utils/RCBot2_meta/bot_utility.cpp"
    "utils/RCBot2_meta/bot_visibles.cpp"
    "utils/RCBot2_meta/bot_waypoint.cpp"
    "utils/RCBot2_meta/bot_waypoint_locations.cpp"
    "utils/RCBot2_meta/bot_waypoint_visibility.cpp"
    "utils/RCBot2_meta/bot_waypoint_autorefine.cpp"
    "utils/RCBot2_meta/bot_weapons.cpp"
    "utils/RCBot2_meta/bot_wpt_dist.cpp"
    "utils/RCBot2_meta/bot_zombie.cpp"
    "utils/RCBot2_meta/bot_sigscan.cpp"
    "utils/RCBot2_meta/bot_cvars.cpp"
    "utils/RCBot2_meta/bot_plugin_meta.cpp"
    "utils/RCBot2_meta/bot_waypoint_auto.cpp"
    "utils/RCBot2_meta/bot_waypoint_hl2dm.cpp"
    "utils/RCBot2_meta/bot_npc_combat.cpp"
    "utils/RCBot2_meta/bot_gamemode_config.cpp"
    "utils/RCBot2_meta/bot_navtest.cpp"
    "utils/RCBot2_meta/bot_door.cpp"
    "utils/RCBot2_meta/bot_tactical.cpp"
    "utils/RCBot2_meta/bot_gravity.cpp"
    "utils/RCBot2_meta/bot_teleport.cpp"
    "rcbot/logging.cpp"
    "rcbot/helper.cpp"
    "rcbot/entprops.cpp"
    "rcbot/propvar.cpp"
    "rcbot/math_fix.cpp"
    "rcbot/utils.cpp"
    "rcbot/tf2/conditions.cpp"
    "versioning/build_info.cpp"
    "sm_ext/smsdk_config.cpp"
    "sm_ext/bot_sm_ext.cpp"
    "sm_ext/bot_sm_natives.cpp"
    "sm_ext/bot_sm_natives_tf2.cpp"
    "sm_ext/bot_sm_natives_dod.cpp"
    "sm_ext/bot_sm_natives_hldm.cpp"
    "sm_ext/bot_sm_forwards.cpp"
    "sm_ext/bot_sm_events.cpp"
)

build_sdk() {
    local SDK_NAME=$1
    local SDK_PATH=$2
    local OUTPUT_NAME=$3
    local ENGINE_CODE=$4

    echo "Building ${OUTPUT_NAME} (32-bit)..."
    echo "  SDK: ${SDK_NAME}"
    echo "  Path: ${SDK_PATH}"

    # SDK include paths from manifest
    local SDK_INCLUDES=(
        "-I${SDK_PATH}/public"
        "-I${SDK_PATH}/public/engine"
        "-I${SDK_PATH}/public/mathlib"
        "-I${SDK_PATH}/public/vstdlib"
        "-I${SDK_PATH}/public/tier0"
        "-I${SDK_PATH}/public/tier1"
        "-I${SDK_PATH}/public/toolframework"
        "-I${SDK_PATH}/public/game/server"
        "-I${SDK_PATH}/game/shared"
        "-I${SDK_PATH}/game/server"
        "-I${SDK_PATH}/common"
    )

    # SDK-specific defines
    local SDK_DEFINES="-DSOURCE_ENGINE=${ENGINE_CODE}"

    local OBJ_DIR="${BUILD_DIR}/${SDK_NAME}"
    mkdir -p "${OBJ_DIR}"

    # Compile each source file
    local OBJECTS=()
    local FAILED=0
    for src in "${COMMON_SOURCES[@]}"; do
        local obj="${OBJ_DIR}/$(basename ${src%.cpp}.o)"
        echo -n "  Compiling $(basename ${src})..."
        if ${CXX} ${CXXFLAGS} ${DEFINES} ${SDK_DEFINES} \
            ${COMMON_INCLUDES[@]} ${SDK_INCLUDES[@]} \
            -c "${RCBOT_ROOT}/${src}" -o "${obj}" 2>/tmp/compile_error_$$.txt; then
            echo " OK"
            OBJECTS+=("${obj}")
        else
            echo " FAILED"
            cat /tmp/compile_error_$$.txt | head -20
            FAILED=1
            break
        fi
    done

    if [ ${FAILED} -eq 1 ]; then
        echo "ERROR: Compilation failed for ${OUTPUT_NAME}"
        return 1
    fi

    # Static libraries to link
    local STATIC_LIBS=(
        "${SDK_PATH}/lib/public/linux/mathlib_i486.a"
        "${SDK_PATH}/lib/public/linux/tier1_i486.a"
    )

    # Verify static libs exist
    for lib in "${STATIC_LIBS[@]}"; do
        if [ ! -f "${lib}" ]; then
            echo "WARNING: Static library not found: ${lib}"
        fi
    done

    # Link
    echo "  Linking ${OUTPUT_NAME}..."
    ${CXX} -m32 -shared -o "${BUILD_DIR}/${OUTPUT_NAME}" \
        ${OBJECTS[@]} \
        ${STATIC_LIBS[@]} \
        -lm -ldl \
        -static-libgcc -static-libstdc++ 2>&1 || {
        echo "ERROR: Failed to link ${OUTPUT_NAME}"
        return 1
    }

    echo "  Successfully built ${OUTPUT_NAME}"
    file "${BUILD_DIR}/${OUTPUT_NAME}"
}

# Create build directory
rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"

echo "=== RCBot2 32-bit Manual Build ==="
echo ""
echo "Checking SDK availability..."

# Check SDK paths
HL2DM_SDK="${SDK_ROOT}/hl2sdk-hl2dm"
TF2_SDK="${SDK_ROOT}/hl2sdk-tf2"

if [ ! -d "${HL2DM_SDK}/public" ]; then
    echo "ERROR: HL2DM SDK not initialized at ${HL2DM_SDK}"
    echo "Run: git submodule update --init alliedmodders/hl2sdk-hl2dm"
    exit 1
fi

if [ ! -d "${TF2_SDK}/public" ]; then
    echo "ERROR: TF2 SDK not initialized at ${TF2_SDK}"
    echo "Run: git submodule update --init alliedmodders/hl2sdk-tf2"
    exit 1
fi

echo ""

# Build for HL2DM (SDK engine code 7)
build_sdk "hl2dm" "${HL2DM_SDK}" "rcbot-2-hl2dm.so" "7" || true

echo ""

# Build for TF2 (SDK engine code 12)
build_sdk "tf2" "${TF2_SDK}" "rcbot-2-tf2.so" "12" || true

echo ""
echo "=== Build Complete ==="
echo "Output files in: ${BUILD_DIR}/"
ls -la "${BUILD_DIR}"/*.so 2>/dev/null || echo "No .so files produced"
