#include "routing/Router.hpp"
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <limits>
#include <algorithm>
#include <cmath>
#include <chrono>

namespace routing {

RouteResult Router::findRouteDijkstra(const model::RoadNetwork& network, int startNodeId, int destNodeId, RoutingObjective objective) {
    auto startTime = std::chrono::steady_clock::now();
    std::size_t nodesExplored = 0;

    if (!network.getNodeById(startNodeId) || !network.getNodeById(destNodeId)) {
        auto endTime = std::chrono::steady_clock::now();
        auto runtime = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
        return RouteResult{false, {}, 0.0, 0.0, 0, runtime};
    }

    if (startNodeId == destNodeId) {
        auto endTime = std::chrono::steady_clock::now();
        auto runtime = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
        return RouteResult{true, {startNodeId}, 0.0, 0.0, 0, runtime};
    }

    std::unordered_map<int, std::vector<const model::Road*>> adj;
    for (const auto& road : network.getRoads()) {
        if (!road.closed) {
            adj[road.from].push_back(&road);
        }
    }

    std::unordered_map<int, double> dist;
    std::unordered_map<int, const model::Road*> prevRoad;
    
    using PQueueItem = std::pair<double, int>;
    std::priority_queue<PQueueItem, std::vector<PQueueItem>, std::greater<PQueueItem>> pq;

    for (const auto& node : network.getNodes()) {
        dist[node.id] = std::numeric_limits<double>::infinity();
    }

    dist[startNodeId] = 0.0;
    pq.push({0.0, startNodeId});

    bool found = false;

    while (!pq.empty()) {
        auto [currentCost, u] = pq.top();
        pq.pop();

        if (currentCost > dist[u]) continue;

        nodesExplored++;

        if (u == destNodeId) {
            found = true;
            break;
        }

        for (const model::Road* road : adj[u]) {
            int v = road->to;
            double weight = (objective == RoutingObjective::Fastest) ? road->getTravelTimeSeconds() : road->distanceMeters;

            if (dist[u] + weight < dist[v]) {
                dist[v] = dist[u] + weight;
                prevRoad[v] = road;
                pq.push({dist[v], v});
            }
        }
    }

    if (!found) {
        auto endTime = std::chrono::steady_clock::now();
        auto runtime = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
        return RouteResult{false, {}, 0.0, 0.0, nodesExplored, runtime};
    }

    std::vector<int> path;
    double totalDist = 0.0;
    double totalTime = 0.0;
    int curr = destNodeId;
    
    while (curr != startNodeId) {
        path.push_back(curr);
        const model::Road* road = prevRoad[curr];
        totalDist += road->distanceMeters;
        totalTime += road->getTravelTimeSeconds();
        curr = road->from;
    }
    path.push_back(startNodeId);
    std::reverse(path.begin(), path.end());

    auto endTime = std::chrono::steady_clock::now();
    auto runtime = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
    
    return RouteResult{true, path, totalDist, totalTime, nodesExplored, runtime};
}

RouteResult Router::findRouteAStar(const model::RoadNetwork& network, int startNodeId, int destNodeId, RoutingObjective objective) {
    auto startTime = std::chrono::steady_clock::now();
    std::size_t nodesExplored = 0;

    const model::Node* startNode = network.getNodeById(startNodeId);
    const model::Node* destNode = network.getNodeById(destNodeId);

    if (!startNode || !destNode) {
        auto endTime = std::chrono::steady_clock::now();
        auto runtime = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
        return RouteResult{false, {}, 0.0, 0.0, 0, runtime};
    }

    if (startNodeId == destNodeId) {
        auto endTime = std::chrono::steady_clock::now();
        auto runtime = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
        return RouteResult{true, {startNodeId}, 0.0, 0.0, 0, runtime};
    }

    std::unordered_map<int, std::vector<const model::Road*>> adj;
    for (const auto& road : network.getRoads()) {
        if (!road.closed) {
            adj[road.from].push_back(&road);
        }
    }

    auto heuristic = [&](int u) {
        const model::Node* n = network.getNodeById(u);
        if (!n) return 0.0;
        double dx = n->x - destNode->x;
        double dy = n->y - destNode->y;
        double dist = std::sqrt(dx*dx + dy*dy);
        if (objective == RoutingObjective::Fastest) {
            // Assume max speed limit of 50 km/h and free traffic (1.0) for admissible heuristic
            return dist / (50.0 / 3.6);
        }
        return dist;
    };

    std::unordered_map<int, double> gScore;
    std::unordered_map<int, const model::Road*> prevRoad;
    
    using PQueueItem = std::pair<double, int>;
    std::priority_queue<PQueueItem, std::vector<PQueueItem>, std::greater<PQueueItem>> pq;
    std::unordered_set<int> closedSet;

    for (const auto& node : network.getNodes()) {
        gScore[node.id] = std::numeric_limits<double>::infinity();
    }

    gScore[startNodeId] = 0.0;
    pq.push({heuristic(startNodeId), startNodeId});

    bool found = false;

    while (!pq.empty()) {
        auto [currentFScore, u] = pq.top();
        pq.pop();

        if (closedSet.count(u)) continue;
        closedSet.insert(u);
        
        nodesExplored++;

        if (u == destNodeId) {
            found = true;
            break;
        }

        for (const model::Road* road : adj[u]) {
            int v = road->to;
            double weight = (objective == RoutingObjective::Fastest) ? road->getTravelTimeSeconds() : road->distanceMeters;
            
            double tentative_gScore = gScore[u] + weight;
            if (tentative_gScore < gScore[v]) {
                prevRoad[v] = road;
                gScore[v] = tentative_gScore;
                double fScore = tentative_gScore + heuristic(v);
                pq.push({fScore, v});
            }
        }
    }

    if (!found) {
        auto endTime = std::chrono::steady_clock::now();
        auto runtime = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
        return RouteResult{false, {}, 0.0, 0.0, nodesExplored, runtime};
    }

    std::vector<int> path;
    double totalDist = 0.0;
    double totalTime = 0.0;
    int curr = destNodeId;
    
    while (curr != startNodeId) {
        path.push_back(curr);
        const model::Road* road = prevRoad[curr];
        totalDist += road->distanceMeters;
        totalTime += road->getTravelTimeSeconds();
        curr = road->from;
    }
    path.push_back(startNodeId);
    std::reverse(path.begin(), path.end());

    auto endTime = std::chrono::steady_clock::now();
    auto runtime = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();

    return RouteResult{true, path, totalDist, totalTime, nodesExplored, runtime};
}

RouteResult Router::findRoute(const model::RoadNetwork& network, int startNodeId, int destNodeId, RoutingAlgorithm algorithm, RoutingObjective objective) {
    if (algorithm == RoutingAlgorithm::Dijkstra) {
        return findRouteDijkstra(network, startNodeId, destNodeId, objective);
    } else if (algorithm == RoutingAlgorithm::AStar) {
        return findRouteAStar(network, startNodeId, destNodeId, objective);
    }
    return RouteResult{false, {}, 0.0, 0.0, 0, 0};
}

} // namespace routing
