#include <iostream>
#include <cassert>
#include "model/GeneratedCityLoader.hpp"
#include "routing/Router.hpp"
#include "api/HttpServer.hpp"

void printRouteResult(const routing::RouteResult& result, int start, int dest, const std::string& algo) {
    if (!result.found) {
        std::cout << algo << " Route " << start << " -> " << dest << ": No route found\n";
        return;
    }
    
    std::cout << algo << " Route found " << start << " -> " << dest << "\n";
    for (size_t i = 0; i < result.nodeIds.size(); ++i) {
        std::cout << result.nodeIds[i];
        if (i + 1 < result.nodeIds.size()) {
            std::cout << " -> ";
        }
    }
    std::cout << "\nTotal distance: " << result.totalCost << " m\n";
    std::cout << "Nodes explored: " << result.nodesExplored << "\n";
    std::cout << "Runtime: " << result.runtimeMicroseconds << " us\n\n";
}

void verifyRoutes(const model::RoadNetwork& network, int start, int dest, double expectedDistance) {
    auto resDijkstra = routing::Router::findRouteDijkstra(network, start, dest);
    auto resAStar = routing::Router::findRouteAStar(network, start, dest);

    printRouteResult(resDijkstra, start, dest, "Dijkstra");
    printRouteResult(resAStar, start, dest, "A*");

    if (resDijkstra.found && resAStar.found) {
        // Use a small epsilon for floating point comparison
        if (std::abs(resDijkstra.totalCost - resAStar.totalCost) > 1e-5) {
            std::cerr << "ERROR: Cost mismatch! Dijkstra: " << resDijkstra.totalCost 
                      << " A*: " << resAStar.totalCost << "\n";
        }
        if (expectedDistance >= 0.0 && std::abs(resDijkstra.totalCost - expectedDistance) > 1e-5) {
            std::cerr << "ERROR: Expected distance " << expectedDistance << " but got " << resDijkstra.totalCost << "\n";
        }
    } else if (resDijkstra.found != resAStar.found) {
        std::cerr << "ERROR: One algorithm found a route while the other didn't.\n";
    }
}

void runBenchmark(const model::RoadNetwork& network, int start, int dest, int runs = 1000) {
    long long totalDijkstra = 0;
    long long totalAStar = 0;

    for (int i = 0; i < runs; ++i) {
        totalDijkstra += routing::Router::findRouteDijkstra(network, start, dest).runtimeMicroseconds;
        totalAStar += routing::Router::findRouteAStar(network, start, dest).runtimeMicroseconds;
    }

    std::cout << "Benchmark over " << runs << " runs for " << start << " -> " << dest << ":\n";
    std::cout << "  Dijkstra avg: " << (totalDijkstra / runs) << " us\n";
    std::cout << "  A* avg:       " << (totalAStar / runs) << " us\n\n";
}

int main() {
    std::cout << "Smart Mobility Simulator\n";
    std::cout << "========================\n\n";

    model::RoadNetwork network = model::GeneratedCityLoader::generate5x5Grid();

    std::cout << "Testing Routing Algorithms...\n\n";

    // Test case 1: 0 -> 24
    verifyRoutes(network, 0, 24, 800.0);

    // Test case 2: 0 -> 1 (100 m)
    verifyRoutes(network, 0, 1, 100.0);

    // Test case 3: 0 -> 4 (400 m)
    verifyRoutes(network, 0, 4, 400.0);

    // Test case 4: 0 -> 20 (400 m)
    verifyRoutes(network, 0, 20, 400.0);

    // Test case 5: 12 -> 12 (0 m)
    verifyRoutes(network, 12, 12, 0.0);

    std::cout << "--- Running Benchmarks ---\n";
    runBenchmark(network, 0, 24);

    // Test case 6: closed road
    network.setRoadClosed(1, 2, true);
    std::cout << "--- Closing road 1 -> 2 ---\n";
    verifyRoutes(network, 0, 2, -1.0);

    // Test case 7: Invalid ID
    verifyRoutes(network, 0, 999, -1.0);

    std::cout << "\nStarting API server...\n";
    api::HttpServer server(network);
    server.listen("0.0.0.0", 8080);

    return 0;
}
