/**
 * bot_ml_features.h
 *
 * Feature extraction system for ML models
 * Converts game state into normalized feature vectors
 *
 * Part of Phase 0: Foundation and Infrastructure
 */

#ifndef __BOT_ML_FEATURES_H__
#define __BOT_ML_FEATURES_H__

#include <vector>
#include <string>

class CBot;
class Vector;
class QAngle;

/**
 * Feature Extractor Base Class
 *
 * Abstract interface for extracting features from game state.
 * Subclass this for game-specific feature extraction (TF2, DOD, etc.)
 */
class IFeatureExtractor {
public:
    virtual ~IFeatureExtractor() {}

    // Main extraction method
    virtual void Extract(CBot* pBot, std::vector<float>& features) = 0;

    // Metadata
    virtual size_t GetFeatureCount() const = 0;
    virtual void GetFeatureNames(std::vector<std::string>& names) const = 0;
    virtual void GetFeatureDescriptions(std::vector<std::string>& descriptions) const = 0;

    // Validation
    virtual bool ValidateFeatures(const std::vector<float>& features) const;

    // Feature group control (enable/disable specific feature groups)
    virtual void EnableFeatureGroup(const char* group_name) = 0;
    virtual void DisableFeatureGroup(const char* group_name) = 0;
    virtual bool IsFeatureGroupEnabled(const char* group_name) const = 0;
};

/**
 * TF2 Feature Extractor
 *
 * Extracts ~96 features for Team Fortress 2 gameplay:
 * - Self state (health, ammo, position, etc.)
 * - Visible threats (up to 5 enemies)
 * - Objectives (payload cart, control points, flags)
 * - Navigation (waypoints, cover)
 * - Recent history (damage, kills, deaths)
 */
class CTF2FeatureExtractor : public IFeatureExtractor {
public:
    CTF2FeatureExtractor();
    virtual ~CTF2FeatureExtractor() {}

    // IFeatureExtractor implementation
    void Extract(CBot* pBot, std::vector<float>& features) override;
    size_t GetFeatureCount() const override { return 96; }
    void GetFeatureNames(std::vector<std::string>& names) const override;
    void GetFeatureDescriptions(std::vector<std::string>& descriptions) const override;

    void EnableFeatureGroup(const char* group_name) override;
    void DisableFeatureGroup(const char* group_name) override;
    bool IsFeatureGroupEnabled(const char* group_name) const override;

private:
    // Feature extraction by category
    void ExtractSelfState(CBot* pBot, std::vector<float>& features);
    void ExtractVisibleThreats(CBot* pBot, std::vector<float>& features);
    void ExtractObjectives(CBot* pBot, std::vector<float>& features);
    void ExtractNavigation(CBot* pBot, std::vector<float>& features);
    void ExtractRecentHistory(CBot* pBot, std::vector<float>& features);

    // Normalization helpers
    float NormalizeHealth(float health, float max_health) const;
    float NormalizeDistance(float distance, float max_distance = 4096.0f) const;
    float NormalizeAngle(float angle) const;
    float NormalizePosition(float pos, float min_pos, float max_pos) const;
    void NormalizeVector(const Vector& vec, float& x, float& y, float& z) const;

    // Threat assessment
    float CalculateThreatLevel(CBot* pBot, edict_t* pEnemy) const;

    // Feature group flags
    struct FeatureGroups {
        bool self_state;
        bool visible_threats;
        bool objectives;
        bool navigation;
        bool recent_history;
    } m_enabledGroups;

    // Cached map bounds (for position normalization)
    Vector m_mapMin;
    Vector m_mapMax;
    bool m_bMapBoundsCalculated;
    void CalculateMapBounds();
};

/**
 * DOD:S Feature Extractor
 *
 * Extracts features for Day of Defeat: Source
 * Similar to TF2 but with DOD-specific objectives (capture points, flags)
 */
class CDODFeatureExtractor : public IFeatureExtractor {
public:
    CDODFeatureExtractor();
    virtual ~CDODFeatureExtractor() {}

    void Extract(CBot* pBot, std::vector<float>& features) override;
    size_t GetFeatureCount() const override { return 96; }
    void GetFeatureNames(std::vector<std::string>& names) const override;
    void GetFeatureDescriptions(std::vector<std::string>& descriptions) const override;

    void EnableFeatureGroup(const char* group_name) override;
    void DisableFeatureGroup(const char* group_name) override;
    bool IsFeatureGroupEnabled(const char* group_name) const override;

private:
    // DOD-specific extraction methods
    void ExtractSelfState(CBot* pBot, std::vector<float>& features);
    void ExtractVisibleThreats(CBot* pBot, std::vector<float>& features);
    void ExtractCapturePoints(CBot* pBot, std::vector<float>& features);
    void ExtractFlags(CBot* pBot, std::vector<float>& features);

    struct FeatureGroups {
        bool self_state;
        bool visible_threats;
        bool capture_points;
        bool flags;
        bool recent_history;
    } m_enabledGroups;
};

/**
 * Feature Extractor Factory
 *
 * Creates the appropriate feature extractor for the current game
 */
class CFeatureExtractorFactory {
public:
    static IFeatureExtractor* CreateExtractor();
    static IFeatureExtractor* CreateExtractorForGame(const char* game_name);
};

/**
 * Feature Statistics Tracker
 *
 * Tracks feature statistics for debugging and validation
 * Can compute running mean/std for normalization
 */
class CFeatureStatistics {
public:
    CFeatureStatistics(size_t feature_count);

    // Update statistics with new feature vector
    void Update(const std::vector<float>& features);

    // Get statistics
    void GetMean(std::vector<float>& mean) const;
    void GetStdDev(std::vector<float>& stddev) const;
    void GetMin(std::vector<float>& min) const;
    void GetMax(std::vector<float>& max) const;

    // Normalize features using statistics
    void Normalize(std::vector<float>& features) const;

    // Reset statistics
    void Reset();

    // Persistence
    bool SaveToFile(const char* filename) const;
    bool LoadFromFile(const char* filename);

private:
    size_t m_featureCount;
    size_t m_sampleCount;

    std::vector<float> m_sum;
    std::vector<float> m_sumSquared;
    std::vector<float> m_min;
    std::vector<float> m_max;
};

#endif // __BOT_ML_FEATURES_H__
