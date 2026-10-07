#pragma once
#include <vector>
#include <string>
#include "routing/Router.hpp"
#include "model/RoadNetwork.hpp"

namespace recommendation {

struct RouteCandidate {
    std::vector<int> nodeIds;
    double totalDistanceMeters;
    double travelTimeSeconds;
    double averageCongestion;
    double estimatedCost;
    std::string sourceObjective; // "shortest", "fastest", "least_traffic"
};

class RouteCandidateGenerator {
public:
    static std::vector<RouteCandidate> generateCandidates(
        const model::RoadNetwork& network, 
        int startNodeId, 
        int destNodeId
    );
private:
    static RouteCandidate buildCandidate(
        const model::RoadNetwork& network, 
        const routing::RouteResult& routeResult, 
        const std::string& sourceObj
    );
};

} // namespace recommendation

