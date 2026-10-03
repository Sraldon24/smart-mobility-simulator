#include "routing/Router.hpp"
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <limits>
#include <algorithm>
#include <cmath>
#include <chrono>

namespace routing {

RouteResult Router::findRouteDijkstra(const model::RoadNetwork& network, int startNodeId, int destNodeId) {
    auto startTime = std::chrono::steady_clock::now();
    std::size_t nodesExplored = 0;

    // Basic validations
    if (!network.getNodeById(startNodeId) || !network.getNodeById(destNodeId)) {
        auto endTime = std::chrono::steady_clock::now();
        auto runtime = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
        return RouteResult{false, {}, 0.0, 0, runtime};
    }

    if (startNodeId == destNodeId) {
        auto endTime = std::chrono::steady_clock::now();
        auto runtime = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
        return RouteResult{true, {startNodeId}, 0.0, 0, runtime};
    }

    // Build adjacency list for fast lookup
    std::unordered_map<int, std::vector<const model::Road*>> adj;
    for (const auto& road : network.getRoads()) {
        if (!road.closed) {
            adj[road.from].push_back(&road);
        }
    }

    std::unordered_map<int, double> dist;
    std::unordered_map<int, int> prev;
    
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

        // If we found a shorter path to u before popping this, ignore
        if (currentCost > dist[u]) continue;

        nodesExplored++;

        if (u == destNodeId) {
            found = true;
            break;
        }

        // Traverse neighbors
        for (const model::Road* road : adj[u]) {
            int v = road->to;
            double weight = road->distanceMeters;

            if (dist[u] + weight < dist[v]) {
                dist[v] = dist[u] + weight;
                prev[v] = u;
                pq.push({dist[v], v});
            }
        }
    }

    if (!found) {
        auto endTime = std::chrono::steady_clock::now();
        auto runtime = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
        return RouteResult{false, {}, 0.0, nodesExplored, runtime};
    }

    // Reconstruct path
    std::vector<int> path;
    int curr = destNodeId;
    while (curr != startNodeId) {
        path.push_back(curr);
        curr = prev[curr];
    }
    path.push_back(startNodeId);
    std::reverse(path.begin(), path.end());

    auto endTime = std::chrono::steady_clock::now();
    auto runtime = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
    
    return RouteResult{true, path, dist[destNodeId], nodesExplored, runtime};
}

RouteResult Router::findRouteAStar(const model::RoadNetwork& network, int startNodeId, int destNodeId) {
    auto startTime = std::chrono::steady_clock::now();
    std::size_t nodesExplored = 0;

    const model::Node* startNode = network.getNodeById(startNodeId);
    const model::Node* destNode = network.getNodeById(destNodeId);

    if (!startNode || !destNode) {
        auto endTime = std::chrono::steady_clock::now();
        auto runtime = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
        return RouteResult{false, {}, 0.0, 0, runtime};
    }

    if (startNodeId == destNodeId) {
        auto endTime = std::chrono::steady_clock::now();
        auto runtime = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
        return RouteResult{true, {startNodeId}, 0.0, 0, runtime};
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
        return std::sqrt(dx*dx + dy*dy);
    };

    std::unordered_map<int, double> gScore;
    std::unordered_map<int, int> prev;
    
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
            double weight = road->distanceMeters;
            
            double tentative_gScore = gScore[u] + weight;
            if (tentative_gScore < gScore[v]) {
                prev[v] = u;
                gScore[v] = tentative_gScore;
                double fScore = tentative_gScore + heuristic(v);
                pq.push({fScore, v});
            }
        }
    }

    if (!found) {
        auto endTime = std::chrono::steady_clock::now();
        auto runtime = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
        return RouteResult{false, {}, 0.0, nodesExplored, runtime};
    }

    std::vector<int> path;
    int curr = destNodeId;
    while (curr != startNodeId) {
        path.push_back(curr);
        curr = prev[curr];
    }
    path.push_back(startNodeId);
    std::reverse(path.begin(), path.end());

    auto endTime = std::chrono::steady_clock::now();
    auto runtime = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();

    return RouteResult{true, path, gScore[destNodeId], nodesExplored, runtime};
}

RouteResult Router::findRoute(const model::RoadNetwork& network, int startNodeId, int destNodeId, RoutingAlgorithm algorithm) {
    if (algorithm == RoutingAlgorithm::Dijkstra) {
        return findRouteDijkstra(network, startNodeId, destNodeId);
    } else if (algorithm == RoutingAlgorithm::AStar) {
        return findRouteAStar(network, startNodeId, destNodeId);
    }
    return RouteResult{false, {}, 0.0, 0, 0};
}

} // namespace routing
