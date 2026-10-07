#pragma once

#include "model/RoadNetwork.hpp"
#include <vector>

namespace routing {

/*
 * RouteResult is a simple data struct returned by routing queries.
 * It contains the resolved path (if found) alongside performance metrics.
 */
struct RouteResult {
    bool found{false};
    std::vector<int> nodeIds;
    double totalCost{0.0};
    double estimatedTravelTimeSeconds{0.0};
    std::size_t nodesExplored{0};
    long long runtimeMicroseconds{0};
};

/*
 * Algorithm defines HOW we search the graph.
 */
enum class RoutingAlgorithm {
    Dijkstra,
    AStar
};

/*
 * Objective defines WHAT we optimize.
 * Separating HOW we search from WHAT we optimize allows 
 * a single algorithm implementation (e.g., Dijkstra) to 
 * find the Shortest, Fastest, or Least Traffic routes 
 * simply by passing a different cost function.
 */
enum class RoutingObjective {
    Shortest,
    Fastest,
    LeastTraffic
};

class Router {
public:
    static RouteResult findRoute(const model::RoadNetwork& network, int startNodeId, int destNodeId, RoutingAlgorithm algorithm, RoutingObjective objective);
    
    static RouteResult findRouteDijkstra(const model::RoadNetwork& network, int startNodeId, int destNodeId, RoutingObjective objective);
    static RouteResult findRouteAStar(const model::RoadNetwork& network, int startNodeId, int destNodeId, RoutingObjective objective);
};

} // namespace routing
