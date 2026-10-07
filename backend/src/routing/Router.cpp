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
            double weight = road->distanceMeters;
            if (objective == RoutingObjective::Fastest) {
                weight = road->getTravelTimeSeconds();
            } else if (objective == RoutingObjective::LeastTraffic) {
                weight = road->distanceMeters * road->getEffectiveTrafficFactor();
            }

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
        
        double dist = 0.0;
        if (network.getCoordinateSystem() == model::CoordinateSystem::Geographic) {
            constexpr double R = 6371000.0;
            constexpr double PI = 3.14159265358979323846;
            double lat1 = n->y, lon1 = n->x, lat2 = destNode->y, lon2 = destNode->x;
            double dLat = (lat2 - lat1) * PI / 180.0;
            double dLon = (lon2 - lon1) * PI / 180.0;
            lat1 = lat1 * PI / 180.0;
            lat2 = lat2 * PI / 180.0;
            double a = std::sin(dLat / 2) * std::sin(dLat / 2) +
                       std::sin(dLon / 2) * std::sin(dLon / 2) * std::cos(lat1) * std::cos(lat2);
            double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));
            dist = R * c;
        } else {
            double dx = n->x - destNode->x;
            double dy = n->y - destNode->y;
            dist = std::sqrt(dx*dx + dy*dy);
        }

        if (objective == RoutingObjective::Fastest) {
            if (network.getCoordinateSystem() == model::CoordinateSystem::Geographic) {
                // Safe max speed for Montreal is 120 km/h
                return dist / (120.0 / 3.6);
            } else {
                // Generated city uses 50 km/h max
                return dist / (50.0 / 3.6);
            }
        } else if (objective == RoutingObjective::LeastTraffic) {
            // effective traffic factor is >= 1.0, so distance * 1.0 is admissible
            return dist;
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
            double weight = road->distanceMeters;
            if (objective == RoutingObjective::Fastest) {
                weight = road->getTravelTimeSeconds();
            } else if (objective == RoutingObjective::LeastTraffic) {
                weight = road->distanceMeters * road->getEffectiveTrafficFactor();
            }
            
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
