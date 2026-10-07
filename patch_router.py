import re

with open('backend/src/routing/Router.cpp', 'r') as f:
    content = f.read()

# Replace findRouteDijkstra
dij_old = r'''RouteResult Router::findRouteDijkstra\(const model::RoadNetwork& network, int startNodeId, int destNodeId, RoutingObjective objective\) \{.*?return RouteResult\{true, path, totalDist, totalTime, nodesExplored, runtime\};\n\}'''

dij_new = '''RouteResult Router::findRouteDijkstra(const model::RoadNetwork& network, int startNodeId, int destNodeId, RoutingObjective objective) {
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

    size_t nodeCount = network.getNodes().size();
    int maxId = 0;
    if (nodeCount > 0) maxId = network.getNodes().back().id;
    size_t vecSize = std::max(nodeCount, static_cast<size_t>(maxId + 1));

    std::vector<double> dist(vecSize, std::numeric_limits<double>::infinity());
    std::vector<const model::Road*> prevRoad(vecSize, nullptr);
    
    using PQueueItem = std::pair<double, int>;
    std::priority_queue<PQueueItem, std::vector<PQueueItem>, std::greater<PQueueItem>> pq;

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

        for (const model::Road* road : network.getOutgoingRoads(u)) {
            if (road->closed) continue;
            
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
}'''

content = re.sub(dij_old, dij_new, content, flags=re.DOTALL)

# Replace findRouteAStar
astar_old = r'''RouteResult Router::findRouteAStar\(const model::RoadNetwork& network, int startNodeId, int destNodeId, RoutingObjective objective\) \{.*?return RouteResult\{true, path, totalDist, totalTime, nodesExplored, runtime\};\n\}'''

astar_new = '''RouteResult Router::findRouteAStar(const model::RoadNetwork& network, int startNodeId, int destNodeId, RoutingObjective objective) {
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

    size_t nodeCount = network.getNodes().size();
    int maxId = 0;
    if (nodeCount > 0) maxId = network.getNodes().back().id;
    size_t vecSize = std::max(nodeCount, static_cast<size_t>(maxId + 1));

    // Heuristic pre-calculation for node coordinates if Cartesian, or geographic
    // To avoid `getNodeById` inside heuristic:
    // If we rely on node array being dense:
    const auto& nodes = network.getNodes();

    auto heuristic = [&](int u) {
        const model::Node* n = nullptr;
        if (u >= 0 && static_cast<size_t>(u) < nodes.size() && nodes[u].id == u) {
            n = &nodes[u];
        } else {
            n = network.getNodeById(u);
        }
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
                return dist / (120.0 / 3.6);
            } else {
                return dist / (50.0 / 3.6);
            }
        } else if (objective == RoutingObjective::LeastTraffic) {
            return dist;
        }
        return dist;
    };

    std::vector<double> gScore(vecSize, std::numeric_limits<double>::infinity());
    std::vector<const model::Road*> prevRoad(vecSize, nullptr);
    std::vector<bool> closedSet(vecSize, false);
    
    using PQueueItem = std::pair<double, int>;
    std::priority_queue<PQueueItem, std::vector<PQueueItem>, std::greater<PQueueItem>> pq;

    gScore[startNodeId] = 0.0;
    pq.push({heuristic(startNodeId), startNodeId});

    bool found = false;

    while (!pq.empty()) {
        auto [currentFScore, u] = pq.top();
        pq.pop();

        if (closedSet[u]) continue;
        closedSet[u] = true;
        
        nodesExplored++;

        if (u == destNodeId) {
            found = true;
            break;
        }

        for (const model::Road* road : network.getOutgoingRoads(u)) {
            if (road->closed) continue;
            
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
}'''

content = re.sub(astar_old, astar_new, content, flags=re.DOTALL)

with open('backend/src/routing/Router.cpp', 'w') as f:
    f.write(content)
