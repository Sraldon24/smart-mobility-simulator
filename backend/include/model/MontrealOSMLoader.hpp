#pragma once

#include <string>
#include <iostream>
#include <cstddef>
#include "model/RoadNetwork.hpp"

namespace smart_mobility {
namespace model {

struct OSMImportStats {
    size_t totalWaysExamined = 0;
    size_t drivableWays = 0;
    size_t referencedNodes = 0;
    size_t oneWayRoads = 0;
    size_t namedRoads = 0;
    long long importScanTimeMs = 0;
    
    // New stats for Step 2
    size_t internalNodes = 0;
    size_t internalDirectedRoads = 0;
    size_t internalOneWaySegments = 0;
    size_t internalTwoWaySegments = 0;
    size_t internalRoadsUsingMaxspeed = 0;
    size_t internalRoadsUsingFallbackSpeed = 0;
};

class MontrealOSMLoader {
public:
    MontrealOSMLoader();
    ~MontrealOSMLoader();

    ::model::RoadNetwork load(const std::string& pbfFilePath);
    
    const OSMImportStats& getStats() const { return stats; }

    static double calculateHaversineDistance(double lat1, double lon1, double lat2, double lon2);

private:
    OSMImportStats stats;
};

} // namespace model
} // namespace smart_mobility

