#include <iostream>
#include <cassert>
#include "model/GeneratedCityLoader.hpp"
#include "model/MontrealOSMLoader.hpp"
#include "routing/Router.hpp"
#include "api/HttpServer.hpp"
#include "simulation/SimulationEngine.hpp"
#include <chrono>
#include <unordered_set>
#include <unordered_map>
#include <fstream>
#include "recommendation/RouteCandidateGenerator.hpp"

void printRouteResult(const routing::RouteResult& result, int start, int dest, const std::string& algo, const std::string& obj) {
    if (!result.found) {
        std::cout << algo << " (" << obj << ") Route " << start << " -> " << dest << ": No route found\n";
        return;
    }
    
    std::cout << algo << " (" << obj << ") Route found " << start << " -> " << dest << "\n";
    for (size_t i = 0; i < result.nodeIds.size(); ++i) {
        std::cout << result.nodeIds[i];
        if (i + 1 < result.nodeIds.size()) {
            std::cout << " -> ";
        }
    }
    std::cout << "\nTotal distance: " << result.totalCost << " m\n";
    std::cout << "Travel time: " << result.estimatedTravelTimeSeconds << " s\n";
    std::cout << "Nodes explored: " << result.nodesExplored << "\n";
    std::cout << "Runtime: " << result.runtimeMicroseconds << " us\n\n";
}

int main(int argc, char** argv) {
    if (argc >= 3 && std::string(argv[1]) == "--import-osm") {
        std::string pbfFile = argv[2];
        smart_mobility::model::MontrealOSMLoader loader;
        model::RoadNetwork network = loader.load(pbfFile);
        const auto& stats = loader.getStats();

        double avgDistance = 0.0;
        if (stats.internalDirectedRoads > 0) {
            double totalDist = 0;
            for(const auto& r : network.getRoads()) totalDist += r.distanceMeters;
            avgDistance = totalDist / stats.internalDirectedRoads;
        }

        std::cout << "Montreal RoadNetwork Import\n";
        std::cout << "---------------------------\n";
        std::cout << "Nodes: " << stats.internalNodes << "\n";
        std::cout << "Directed roads: " << stats.internalDirectedRoads << "\n";
        std::cout << "One-way segments: " << stats.internalOneWaySegments << "\n";
        std::cout << "Two-way segments: " << stats.internalTwoWaySegments << "\n";
        std::cout << "Average segment distance: " << avgDistance << " m\n";
        std::cout << "Import time: " << stats.importScanTimeMs << " ms\n\n";

        // Print a sample road from network
        int printed = 0;
        for (const auto& road : network.getRoads()) {
            if (printed >= 2) break;
            std::cout << "Road:\n";
            std::cout << "internal " << road.from << " -> " << road.to << "\n";
            std::cout << "distance: " << road.distanceMeters << " m\n";
            std::cout << "speed: " << road.speedLimitKph << " km/h\n";
            std::cout << "oneway: " << (network.getRoad(road.to, road.from) == nullptr ? "true" : "false") << "\n\n";
            printed++;
        }

        // Validation
        bool valid = true;
        std::unordered_set<int> validNodeIds;
        for (const auto& node : network.getNodes()) {
            validNodeIds.insert(node.id);
        }

        for (const auto& road : network.getRoads()) {
            if (validNodeIds.find(road.from) == validNodeIds.end() || validNodeIds.find(road.to) == validNodeIds.end()) valid = false;
            if (road.distanceMeters < 0) valid = false;
            if (road.speedLimitKph <= 0) valid = false;
            if (road.from == road.to) valid = false;
        }
        std::cout << "Validation pass: " << (valid ? "true" : "false") << "\n";

        // Haversine sanity test
        double testDist = smart_mobility::model::MontrealOSMLoader::calculateHaversineDistance(45.5017, -73.5673, 45.5018, -73.5673);
        std::cout << "Haversine test (0.0001 lat diff): " << testDist << " m\n";

        // Find a connected pair using BFS
        int startId = -1;
        int endId = -1;
        
        std::unordered_map<int, std::vector<int>> adj;
        for (const auto& road : network.getRoads()) {
            adj[road.from].push_back(road.to);
        }

        for (const auto& node : network.getNodes()) {
            if (adj[node.id].empty()) continue;
            
            std::vector<int> q;
            std::unordered_set<int> visited;
            q.push_back(node.id);
            visited.insert(node.id);
            size_t head = 0;
            
            while(head < q.size() && q.size() < 1000) {
                int curr = q[head++];
                for (int nxt : adj[curr]) {
                    if (visited.insert(nxt).second) {
                        q.push_back(nxt);
                    }
                }
            }
            if (q.size() > 50) { // Found a decent component
                startId = node.id;
                endId = q.back();
                break;
            }
        }

        if (startId != -1 && endId != -1) {
            std::cout << "\n--- Dijkstra vs A* Sanity Check ---\n";
            std::cout << "Testing route from " << startId << " to " << endId << "\n\n";

            auto resShortestDij = routing::Router::findRouteDijkstra(network, startId, endId, routing::RoutingObjective::Shortest);
            printRouteResult(resShortestDij, startId, endId, "Dijkstra", "Shortest");

            auto resShortestAStar = routing::Router::findRouteAStar(network, startId, endId, routing::RoutingObjective::Shortest);
            printRouteResult(resShortestAStar, startId, endId, "AStar", "Shortest");

            auto resFastestDij = routing::Router::findRouteDijkstra(network, startId, endId, routing::RoutingObjective::Fastest);
            printRouteResult(resFastestDij, startId, endId, "Dijkstra", "Fastest");

            auto resFastestAStar = routing::Router::findRouteAStar(network, startId, endId, routing::RoutingObjective::Fastest);
            printRouteResult(resFastestAStar, startId, endId, "AStar", "Fastest");
            
            std::cout << "Match verification:\n";
            std::cout << "Shortest cost match: " << (std::abs(resShortestDij.totalCost - resShortestAStar.totalCost) < 0.1 ? "yes" : "no") << "\n";
            std::cout << "Fastest time match: " << (std::abs(resFastestDij.estimatedTravelTimeSeconds - resFastestAStar.estimatedTravelTimeSeconds) < 0.1 ? "yes" : "no") << "\n";
        } else {
            std::cout << "Could not find a connected component large enough for testing.\n";
        }

        return 0;
    }

    std::string cityMode = "generated";
    std::string pbfFile = "../data/montreal/montreal.osm.pbf";
    // Check if running from build/ or backend/
    std::ifstream f(pbfFile.c_str());
    if (!f.good()) {
        pbfFile = "../../data/montreal/montreal.osm.pbf";
    }
    
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--city" && i + 1 < argc) {
            cityMode = argv[++i];
        }
    }

    model::RoadNetwork network;
    if (cityMode == "montreal") {
        std::cout << "Loading Montreal OSM data...\n";
        smart_mobility::model::MontrealOSMLoader loader;
        network = loader.load(pbfFile);
        if (network.getNodes().empty()) {
            std::cerr << "Failed to load Montreal OSM graph. Returning.\n";
            return 1;
        }
    } else {
        network = model::GeneratedCityLoader::generate5x5Grid();
    }

    simulation::SimulationEngine engine(network);

    if (cityMode != "montreal") {
        std::cout << "Smart Mobility Simulator - Deterministic Rerouting Tests\n";
        std::cout << "========================================================\n\n";

        std::cout << "--- 1. Baseline Route 0 -> 24 (Fastest) ---\n";
        auto baseFastest = routing::Router::findRouteDijkstra(network, 0, 24, routing::RoutingObjective::Fastest);
        printRouteResult(baseFastest, 0, 24, "Dijkstra", "Fastest");
        
        std::cout << "--- Baseline Route 0 -> 24 (Shortest) ---\n";
        auto baseShortest = routing::Router::findRouteDijkstra(network, 0, 24, routing::RoutingObjective::Shortest);
        printRouteResult(baseShortest, 0, 24, "Dijkstra", "Shortest");

        std::cout << "--- 2. Add Closure on active route (1 <-> 6) ---\n";
        network.addIncident(1, 6, model::IncidentType::Closure);
        auto afterClosureFastest = routing::Router::findRouteDijkstra(network, 0, 24, routing::RoutingObjective::Fastest);
        printRouteResult(afterClosureFastest, 0, 24, "Dijkstra", "Fastest");
        network.removeIncident(1, 6);

        std::cout << "--- 3. Add Accident on fastest route (1 <-> 6) ---\n";
        network.addIncident(1, 6, model::IncidentType::Accident);
        auto afterAccidentFastest = routing::Router::findRouteDijkstra(network, 0, 24, routing::RoutingObjective::Fastest);
        printRouteResult(afterAccidentFastest, 0, 24, "Dijkstra", "Fastest");
        
        std::cout << "--- 4. Add Accident on shortest route (1 <-> 6) ---\n";
        auto afterAccidentShortest = routing::Router::findRouteDijkstra(network, 0, 24, routing::RoutingObjective::Shortest);
        printRouteResult(afterAccidentShortest, 0, 24, "Dijkstra", "Shortest");
        network.removeIncident(1, 6);

        std::cout << "--- 5. Add Incident on unused road (20 <-> 21) ---\n";
        network.addIncident(20, 21, model::IncidentType::Accident);
        auto afterUnusedIncidentFastest = routing::Router::findRouteDijkstra(network, 0, 24, routing::RoutingObjective::Fastest);
        printRouteResult(afterUnusedIncidentFastest, 0, 24, "Dijkstra", "Fastest");
    }

    std::cout << "\n--- Simulation Engine Batch & Timing Test ---\n";
    engine.reset();
    
    auto runBatchTest = [&](int count) {
        engine.reset();
        int spawned = engine.spawnVehiclesBatch(count);
        std::cout << "Spawned " << spawned << " / " << count << " vehicles.\n";
        
        auto start = std::chrono::high_resolution_clock::now();
        engine.update(1.0);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        
        std::cout << "Update timing for " << count << " vehicles: " << duration << " us\n";
        std::cout << "State -> Waiting: " << engine.getWaitingVehicleCount() 
                  << ", Moving: " << engine.getMovingVehicleCount() 
                  << ", Arrived: " << engine.getArrivedVehicleCount() << "\n\n";
    };
    
    runBatchTest(10);

    if (cityMode != "montreal") {
        runBatchTest(50);
        runBatchTest(100);
        
        std::cout << "\n--- Candidate Generation Test ---\n";
        auto startCand = std::chrono::high_resolution_clock::now();
        auto candidates = recommendation::RouteCandidateGenerator::generateCandidates(network, 0, 24);
        auto endCand = std::chrono::high_resolution_clock::now();
        std::cout << "Generated " << candidates.size() << " candidates in " 
                  << std::chrono::duration_cast<std::chrono::microseconds>(endCand - startCand).count() << " us\n";
    }

    std::cout << "Starting API server in " << cityMode << " mode...\n";
    api::HttpServer server(network, engine, pbfFile);
    server.listen("127.0.0.1", 8400);

    return 0;
}
