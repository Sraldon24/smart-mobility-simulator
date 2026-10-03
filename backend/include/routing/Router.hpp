#pragma once

#include "model/RoadNetwork.hpp"
#include <vector>

namespace routing {

struct RouteResult {
    bool found{false};
    std::vector<int> nodeIds;
    double totalCost{0.0};
    std::size_t nodesExplored{0};
    long long runtimeMicroseconds{0};
};

enum class RoutingAlgorithm {
    Dijkstra,
    AStar
};

class Router {
public:
    static RouteResult findRoute(const model::RoadNetwork& network, int startNodeId, int destNodeId, RoutingAlgorithm algorithm);
    
    static RouteResult findRouteDijkstra(const model::RoadNetwork& network, int startNodeId, int destNodeId);
    static RouteResult findRouteAStar(const model::RoadNetwork& network, int startNodeId, int destNodeId);
};

} // namespace routing
