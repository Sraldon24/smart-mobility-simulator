#include <iostream>
#include <cassert>
#include "model/GeneratedCityLoader.hpp"
#include "routing/Router.hpp"
#include "api/HttpServer.hpp"

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

    std::cout << "\nStarting API server...\n";
    api::HttpServer server(network);
    server.listen("0.0.0.0", 8080);

    return 0;
}
