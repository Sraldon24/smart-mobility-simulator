#include <iostream>
#include <cassert>
#include "model/GeneratedCityLoader.hpp"
#include "routing/Router.hpp"
#include "api/HttpServer.hpp"
#include "simulation/SimulationEngine.hpp"
#include <chrono>

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

int main() {
    std::cout << "Smart Mobility Simulator - Deterministic Rerouting Tests\n";
    std::cout << "========================================================\n\n";

    model::RoadNetwork network = model::GeneratedCityLoader::generate5x5Grid();
    simulation::SimulationEngine engine(network);

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

    std::cout << "\n--- MVP 4 Step 3: Simulation Engine Batch & Timing Test ---\n";
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
    runBatchTest(50);
    runBatchTest(100);

    // Leave 100 vehicles running for API
    std::cout << "Leaving 100 vehicles spawned for API.\n";
    std::cout << "Starting API server...\n";
    api::HttpServer server(network, engine);
    server.listen("127.0.0.1", 8400);

    return 0;
}
