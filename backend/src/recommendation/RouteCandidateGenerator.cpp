#include "recommendation/RouteCandidateGenerator.hpp"
#include <unordered_set>

namespace recommendation {

RouteCandidate RouteCandidateGenerator::buildCandidate(
    const model::RoadNetwork& network, 
    const routing::RouteResult& routeResult, 
    const std::string& sourceObj
) {
    RouteCandidate candidate;
    candidate.nodeIds = routeResult.nodeIds;
    candidate.sourceObjective = sourceObj;

    double totalDistance = 0.0;
    double totalTime = 0.0;
    double sumTrafficFactorWeighted = 0.0;

    for (size_t i = 0; i < routeResult.nodeIds.size() - 1; ++i) {
        int u = routeResult.nodeIds[i];
        int v = routeResult.nodeIds[i+1];
        const model::Road* road = network.getRoad(u, v);
        if (road) {
            totalDistance += road->distanceMeters;
            totalTime += road->getTravelTimeSeconds();
            sumTrafficFactorWeighted += road->distanceMeters * road->getEffectiveTrafficFactor();
        }
    }
    
    candidate.totalDistanceMeters = totalDistance;
    candidate.travelTimeSeconds = totalTime;
    candidate.averageCongestion = totalDistance > 0 ? (sumTrafficFactorWeighted / totalDistance) : 1.0;
    candidate.estimatedCost = totalDistance * 0.05; // MVP placeholder

    return candidate;
}

std::vector<RouteCandidate> RouteCandidateGenerator::generateCandidates(
    const model::RoadNetwork& network, 
    int startNodeId, 
    int destNodeId
) {
    std::vector<RouteCandidate> candidates;

    if (!network.getNodeById(startNodeId) || !network.getNodeById(destNodeId)) {
        return candidates;
    }

    auto rShortest = routing::Router::findRoute(network, startNodeId, destNodeId, routing::RoutingAlgorithm::AStar, routing::RoutingObjective::Shortest);
    auto rFastest = routing::Router::findRoute(network, startNodeId, destNodeId, routing::RoutingAlgorithm::AStar, routing::RoutingObjective::Fastest);
    auto rLeastTraffic = routing::Router::findRoute(network, startNodeId, destNodeId, routing::RoutingAlgorithm::AStar, routing::RoutingObjective::LeastTraffic);

    auto addIfValidAndUnique = [&](const routing::RouteResult& res, const std::string& objStr) {
        if (!res.found) return;
        
        for (const auto& c : candidates) {
            if (c.nodeIds == res.nodeIds) {
                // duplicate
                return;
            }
        }
        candidates.push_back(buildCandidate(network, res, objStr));
    };

    addIfValidAndUnique(rShortest, "shortest");
    addIfValidAndUnique(rFastest, "fastest");
    addIfValidAndUnique(rLeastTraffic, "least_traffic");

    return candidates;
}

} // namespace recommendation

