/*
 *    This file is part of RCBot.
 */

#ifndef __BOT_FEATURES_H__
#define __BOT_FEATURES_H__

// Stub header - feature extraction temporarily disabled
// TODO: Fix API mismatches

#include <vector>

class CBot;

// Stub feature extractor
class CFeatureExtractor
{
public:
    virtual ~CFeatureExtractor() = default;
    virtual void Extract(CBot* pBot, std::vector<float>& features) { (void)pBot; (void)features; }
    virtual size_t GetFeatureCount() const { return 0; }
};

// Stub factory
class CFeatureExtractorFactory
{
public:
    static CFeatureExtractor* CreateExtractor() { return nullptr; }
    static void DestroyExtractor(CFeatureExtractor* pExtractor) { (void)pExtractor; }
};

#endif
