// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
/*
 *    This file is part of RCBot.
 *
 *    RCBot by Paul Murphy adapted from Botman's HPB Bot 2 template.
 *
 *    RCBot is free software; you can redistribute it and/or modify it
 *    under the terms of the GNU General Public License as published by the
 *    Free Software Foundation; either version 2 of the License, or (at
 *    your option) any later version.
 *
 *    RCBot is distributed in the hope that it will be useful, but
 *    WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with RCBot; if not, write to the Free Software Foundation,
 *    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "bot_features.h"
#include "bot.h"
#include "bot_globals.h"
#include "bot_navigator.h"
#include "bot_weapons.h"
#include "bot_visibles.h"
#include "bot_waypoint.h"

#include <cmath>
#include <algorithm>

// ============================================================================
// CFeatureExtractor Base Class Implementation
// ============================================================================

float CFeatureExtractor::NormalizeHealth(float health, float max_health)
{
    if (max_health <= 0.0f) return 0.0f;
    return std::max(0.0f, std::min(1.0f, health / max_health));
}

float CFeatureExtractor::NormalizeDistance(float distance, float max_distance)
{
    if (max_distance <= 0.0f) return 0.0f;
    // Invert: 0 = far, 1 = close
    return std::max(0.0f, std::min(1.0f, 1.0f - (distance / max_distance)));
}

float CFeatureExtractor::NormalizeAngle(float angle)
{
    // Convert angle in degrees to normalized range [-1, 1]
    // Wrap angle to [-180, 180]
    while (angle > 180.0f) angle -= 360.0f;
    while (angle < -180.0f) angle += 360.0f;
    return angle / 180.0f;
}

float CFeatureExtractor::NormalizeVelocity(float velocity, float max_velocity)
{
    if (max_velocity <= 0.0f) return 0.0f;
    return std::max(-1.0f, std::min(1.0f, velocity / max_velocity));
}

void CFeatureExtractor::NormalizePosition(const Vector& pos, Vector& normalized)
{
    // Normalize world coordinates to approximately [-1, 1]
    // Typical Source map bounds are around -4096 to 4096
    constexpr float MAP_BOUND = 4096.0f;
    normalized.x = std::max(-1.0f, std::min(1.0f, pos.x / MAP_BOUND));
    normalized.y = std::max(-1.0f, std::min(1.0f, pos.y / MAP_BOUND));
    normalized.z = std::max(-1.0f, std::min(1.0f, pos.z / MAP_BOUND));
}

// ============================================================================
// CHL2DMFeatureExtractor Implementation
// ============================================================================

void CHL2DMFeatureExtractor::Extract(CBot* pBot, std::vector<float>& features)
{
    if (!pBot) {
        features.assign(FEATURE_COUNT, 0.0f);
        return;
    }

    features.resize(FEATURE_COUNT);
    std::fill(features.begin(), features.end(), 0.0f);

    // Extract each feature category
    ExtractSelfState(pBot, features, 0);         // [0-11]
    ExtractEnemies(pBot, features, 12);          // [12-35]
    ExtractNavigation(pBot, features, 36);       // [36-47]
    ExtractPickups(pBot, features, 48);          // [48-55]
}

void CHL2DMFeatureExtractor::ExtractSelfState(CBot* pBot, std::vector<float>& features, size_t offset)
{
    // Health (normalized)
    const float maxHealth = static_cast<float>(pBot->getMaxHealth());
    features[offset + 0] = NormalizeHealth(static_cast<float>(pBot->getHealth()), maxHealth);

    // Armor (normalized) - HL2DM has armor
    features[offset + 1] = pBot->getHealthPercent();  // Proxy for armor

    // Position (normalized to [-1, 1])
    Vector pos = pBot->getOrigin();
    Vector normPos;
    NormalizePosition(pos, normPos);
    features[offset + 2] = normPos.x;
    features[offset + 3] = normPos.y;
    features[offset + 4] = normPos.z;

    // Velocity (normalized to [-1, 1])
    Vector vel = pBot->getVelocity();
    features[offset + 5] = NormalizeVelocity(vel.x, 600.0f);
    features[offset + 6] = NormalizeVelocity(vel.y, 600.0f);
    features[offset + 7] = NormalizeVelocity(vel.z, 600.0f);

    // Current weapon (normalized weapon ID)
    CBotWeapon* pWeapon = pBot->getCurrentWeapon();
    if (pWeapon) {
        // Normalize weapon ID to 0-1 range (assuming max 20 weapons in HL2DM)
        features[offset + 8] = static_cast<float>(pWeapon->getID()) / 20.0f;

        // Primary ammo (normalized)
        int maxAmmo = 100;  // Approximate max for most HL2DM weapons
        features[offset + 9] = std::min(1.0f, static_cast<float>(pWeapon->getAmmo(pBot)) / maxAmmo);
    } else {
        features[offset + 8] = 0.0f;
        features[offset + 9] = 0.0f;
    }

    // On ground flag
    features[offset + 10] = (pBot->onGround()) ? 1.0f : 0.0f;

    // Reserved for future use
    features[offset + 11] = 0.0f;
}

void CHL2DMFeatureExtractor::GetNearestEnemies(CBot* pBot, std::vector<EnemyInfo>& enemies, size_t max_count)
{
    enemies.clear();

    CBotVisibles* pVisibles = pBot->getVisibles();
    if (!pVisibles) return;

    const Vector botPos = pBot->getOrigin();

    // Collect all visible enemies with their info
    for (int i = 0; i < pVisibles->numVisible(); i++) {
        edict_t* pEntity = pVisibles->getVisible(i);
        if (!pEntity || !CBotGlobals::entityIsValid(pEntity)) continue;

        // Check if enemy
        if (!pBot->isEnemy(pEntity)) continue;

        EnemyInfo info;
        info.pEntity = pEntity;

        Vector enemyPos = CBotGlobals::entityOrigin(pEntity);
        Vector delta = enemyPos - botPos;
        info.distance = delta.Length();
        info.direction = delta.Normalized();

        // Estimate health (we may not know exact value)
        info.health = 0.5f;  // Default to half health if unknown

        enemies.push_back(info);
    }

    // Sort by distance (closest first)
    std::sort(enemies.begin(), enemies.end(),
        [](const EnemyInfo& a, const EnemyInfo& b) {
            return a.distance < b.distance;
        });

    // Keep only the closest max_count enemies
    if (enemies.size() > max_count) {
        enemies.resize(max_count);
    }
}

void CHL2DMFeatureExtractor::ExtractEnemies(CBot* pBot, std::vector<float>& features, size_t offset)
{
    std::vector<EnemyInfo> enemies;
    GetNearestEnemies(pBot, enemies, MAX_ENEMIES);

    const Vector botPos = pBot->getOrigin();
    const QAngle botAngles = CBotGlobals::playerAngles(pBot->getEdict());

    for (size_t i = 0; i < MAX_ENEMIES; i++) {
        size_t baseIdx = offset + (i * 6);

        if (i < enemies.size()) {
            const EnemyInfo& enemy = enemies[i];

            // Distance (normalized, inverted: 1=close, 0=far)
            features[baseIdx + 0] = NormalizeDistance(enemy.distance, 2048.0f);

            // Horizontal angle to enemy (cos, sin)
            Vector toEnemy = CBotGlobals::entityOrigin(enemy.pEntity) - botPos;
            QAngle angleToEnemy;
            VectorAngles(toEnemy, angleToEnemy);

            float horizAngleDiff = angleToEnemy.y - botAngles.y;
            // Normalize to [-180, 180]
            while (horizAngleDiff > 180.0f) horizAngleDiff -= 360.0f;
            while (horizAngleDiff < -180.0f) horizAngleDiff += 360.0f;

            // Convert to radians and get cos/sin
            float horizRad = horizAngleDiff * (M_PI / 180.0f);
            features[baseIdx + 1] = cosf(horizRad);
            features[baseIdx + 2] = sinf(horizRad);

            // Vertical angle (cos, sin)
            float vertAngleDiff = angleToEnemy.x - botAngles.x;
            while (vertAngleDiff > 180.0f) vertAngleDiff -= 360.0f;
            while (vertAngleDiff < -180.0f) vertAngleDiff += 360.0f;

            float vertRad = vertAngleDiff * (M_PI / 180.0f);
            features[baseIdx + 3] = cosf(vertRad);
            features[baseIdx + 4] = sinf(vertRad);

            // Enemy health estimate
            features[baseIdx + 5] = enemy.health;

        } else {
            // No enemy in this slot - fill with zeros
            for (size_t j = 0; j < 6; j++) {
                features[baseIdx + j] = 0.0f;
            }
        }
    }
}

void CHL2DMFeatureExtractor::ExtractNavigation(CBot* pBot, std::vector<float>& features, size_t offset)
{
    CBotNavigator* pNav = pBot->getNavigator();
    const Vector botPos = pBot->getOrigin();

    if (pNav && pNav->hasNextPoint()) {
        // Distance to next waypoint
        Vector nextPos = pNav->getNextPoint();
        float dist = (nextPos - botPos).Length();
        features[offset + 0] = NormalizeDistance(dist, 1024.0f);

        // Direction to waypoint (cos, sin of horizontal angle)
        Vector toWaypoint = nextPos - botPos;
        QAngle angleToWpt;
        VectorAngles(toWaypoint, angleToWpt);

        float horizRad = angleToWpt.y * (M_PI / 180.0f);
        features[offset + 1] = cosf(horizRad);
        features[offset + 2] = sinf(horizRad);

        // Path length to goal (if available)
        // This is approximate - we don't have exact path length
        features[offset + 3] = 0.5f;  // Placeholder

        // Has path flag
        features[offset + 10] = 1.0f;
    } else {
        features[offset + 0] = 0.0f;
        features[offset + 1] = 0.0f;
        features[offset + 2] = 0.0f;
        features[offset + 3] = 0.0f;
        features[offset + 10] = 0.0f;
    }

    // Nearest cover position
    float coverDist = 0.0f;
    Vector cover = GetNearestCover(pBot, coverDist);
    if (coverDist > 0.0f) {
        Vector normCover;
        NormalizePosition(cover, normCover);
        features[offset + 4] = normCover.x;
        features[offset + 5] = normCover.y;
        features[offset + 6] = normCover.z;
        features[offset + 7] = NormalizeDistance(coverDist, 512.0f);
        features[offset + 8] = 0.0f;  // In cover flag (TODO: implement)
    } else {
        features[offset + 4] = 0.0f;
        features[offset + 5] = 0.0f;
        features[offset + 6] = 0.0f;
        features[offset + 7] = 0.0f;
        features[offset + 8] = 0.0f;
    }

    // Stuck indicator
    features[offset + 11] = pBot->isStuck() ? 1.0f : 0.0f;
}

Vector CHL2DMFeatureExtractor::GetNearestCover(CBot* pBot, float& distance)
{
    // TODO: Implement actual cover detection using waypoints
    // For now, return zero vector
    distance = 0.0f;
    return Vector(0, 0, 0);
}

Vector CHL2DMFeatureExtractor::GetNearestHealthPack(CBot* pBot, float& distance)
{
    // TODO: Implement health pack detection
    // This would search for item_healthkit entities
    distance = 0.0f;
    return Vector(0, 0, 0);
}

Vector CHL2DMFeatureExtractor::GetNearestAmmoPack(CBot* pBot, float& distance)
{
    // TODO: Implement ammo pack detection
    // This would search for item_ammo entities
    distance = 0.0f;
    return Vector(0, 0, 0);
}

void CHL2DMFeatureExtractor::ExtractPickups(CBot* pBot, std::vector<float>& features, size_t offset)
{
    const Vector botPos = pBot->getOrigin();

    // Nearest health pack
    float healthDist = 0.0f;
    Vector healthPos = GetNearestHealthPack(pBot, healthDist);
    if (healthDist > 0.0f) {
        features[offset + 0] = NormalizeDistance(healthDist, 1024.0f);

        Vector toHealth = healthPos - botPos;
        QAngle angleToHealth;
        VectorAngles(toHealth, angleToHealth);
        float horizRad = angleToHealth.y * (M_PI / 180.0f);
        features[offset + 1] = cosf(horizRad);
        features[offset + 2] = sinf(horizRad);
    } else {
        features[offset + 0] = 0.0f;
        features[offset + 1] = 0.0f;
        features[offset + 2] = 0.0f;
    }

    // Nearest ammo pack
    float ammoDist = 0.0f;
    Vector ammoPos = GetNearestAmmoPack(pBot, ammoDist);
    if (ammoDist > 0.0f) {
        features[offset + 3] = NormalizeDistance(ammoDist, 1024.0f);

        Vector toAmmo = ammoPos - botPos;
        QAngle angleToAmmo;
        VectorAngles(toAmmo, angleToAmmo);
        float horizRad = angleToAmmo.y * (M_PI / 180.0f);
        features[offset + 4] = cosf(horizRad);
        features[offset + 5] = sinf(horizRad);
    } else {
        features[offset + 3] = 0.0f;
        features[offset + 4] = 0.0f;
        features[offset + 5] = 0.0f;
    }

    // Nearest weapon (TODO: implement)
    features[offset + 6] = 0.0f;

    // Need health flag
    features[offset + 7] = (pBot->getHealthPercent() < 0.5f) ? 1.0f : 0.0f;
}

void CHL2DMFeatureExtractor::GetFeatureNames(std::vector<std::string>& names) const
{
    names.clear();
    names.reserve(FEATURE_COUNT);

    // Self state [0-11]
    names.push_back("self_health");
    names.push_back("self_armor");
    names.push_back("self_pos_x");
    names.push_back("self_pos_y");
    names.push_back("self_pos_z");
    names.push_back("self_vel_x");
    names.push_back("self_vel_y");
    names.push_back("self_vel_z");
    names.push_back("self_weapon_id");
    names.push_back("self_ammo");
    names.push_back("self_on_ground");
    names.push_back("self_reserved");

    // Enemies [12-35] (4 enemies × 6 features)
    for (size_t i = 0; i < MAX_ENEMIES; i++) {
        std::string prefix = "enemy" + std::to_string(i) + "_";
        names.push_back(prefix + "distance");
        names.push_back(prefix + "horiz_cos");
        names.push_back(prefix + "horiz_sin");
        names.push_back(prefix + "vert_cos");
        names.push_back(prefix + "vert_sin");
        names.push_back(prefix + "health");
    }

    // Navigation [36-47]
    names.push_back("nav_waypoint_dist");
    names.push_back("nav_waypoint_cos");
    names.push_back("nav_waypoint_sin");
    names.push_back("nav_path_length");
    names.push_back("nav_cover_x");
    names.push_back("nav_cover_y");
    names.push_back("nav_cover_z");
    names.push_back("nav_cover_dist");
    names.push_back("nav_in_cover");
    names.push_back("nav_reserved1");
    names.push_back("nav_has_path");
    names.push_back("nav_stuck");

    // Pickups [48-55]
    names.push_back("pickup_health_dist");
    names.push_back("pickup_health_cos");
    names.push_back("pickup_health_sin");
    names.push_back("pickup_ammo_dist");
    names.push_back("pickup_ammo_cos");
    names.push_back("pickup_ammo_sin");
    names.push_back("pickup_weapon_dist");
    names.push_back("pickup_need_health");
}

// ============================================================================
// CFeatureExtractorFactory Implementation
// ============================================================================

CFeatureExtractor* CFeatureExtractorFactory::CreateExtractor()
{
    // For now, only HL2DM is implemented
    // TODO: Add TF2, DOD, CS:S extractors when needed
    if (CBotGlobals::isHL2DM()) {
        return new CHL2DMFeatureExtractor();
    }

    // Default to HL2DM extractor
    return new CHL2DMFeatureExtractor();
}

void CFeatureExtractorFactory::DestroyExtractor(CFeatureExtractor* pExtractor)
{
    delete pExtractor;
}
