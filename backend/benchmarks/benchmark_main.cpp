#include <iostream>
#include <chrono>
#include <vector>
#include <functional>
#include <algorithm>
#include <numeric>
#include <iomanip>

#include "model/GeneratedCityLoader.hpp"
#include "model/MontrealOSMLoader.hpp"
#include "routing/Router.hpp"
#include "simulation/SimulationEngine.hpp"
#include "recommendation/RouteCandidateGenerator.hpp"
#include "recommendation/RecommendationEngine.hpp"

// using namespace smart_mobility;

struct BenchmarkResult {
    double averageMs{0};
    double medianMs{0};
    double minMs{0};
    double maxMs{0};
};

BenchmarkResult runBenchmark(int warmupRuns, int measuredRuns, const std::function<void()>& fn) {
    for (int i = 0; i < warmupRuns; ++i) {
        fn();
    }
    
    std::vector<double> times;
    times.reserve(measuredRuns);
    
    for (int i = 0; i < measuredRuns; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        fn();
        auto end = std::chrono::high_resolution_clock::now();
        times.push_back(std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0);
    }
    
    std::sort(times.begin(), times.end());
    double sum = std::accumulate(times.begin(), times.end(), 0.0);
    
    BenchmarkResult res;
    res.minMs = times.front();
    res.maxMs = times.back();
    res.averageMs = sum / times.size();
    res.medianMs = times[times.size() / 2];
    if (times.size() % 2 == 0) {
        res.medianMs = (times[times.size() / 2 - 1] + times[times.size() / 2]) / 2.0;
    }
    
    return res;
}

void printResult(const std::string& name, const BenchmarkResult& res) {
    std::cout << std::left << std::setw(35) << name 
              << " Avg: " << std::setw(8) << res.averageMs << " ms  "
              << " Med: " << std::setw(8) << res.medianMs << " ms  "
              << " Min: " << std::setw(8) << res.minMs << " ms  "
              << " Max: " << std::setw(8) << res.maxMs << " ms\n";
}

int main(int argc, char** argv) {
    std::string pbfFile = "../../data/montreal/montreal.osm.pbf";
    if (argc > 1) {
        pbfFile = argv[1];
    }
    
    std::cout << "=========================================\n";
    std::cout << "       SMART MOBILITY BENCHMARKS         \n";
    std::cout << "=========================================\n\n";

    // 1. GENERATED CITY
    std::cout << "--- GENERATED CITY ---\n";
    model::RoadNetwork genNetwork = model::GeneratedCityLoader::generate5x5Grid();
    
    printResult("GenCity Dijkstra Fastest", runBenchmark(10, 1000, [&]() {
        auto r = routing::Router::findRouteDijkstra(genNetwork, 0, 24, routing::RoutingObjective::Fastest);
    }));
    
    printResult("GenCity A* Fastest", runBenchmark(10, 1000, [&]() {
        auto r = routing::Router::findRouteAStar(genNetwork, 0, 24, routing::RoutingObjective::Fastest);
    }));
    
    printResult("GenCity Dijkstra Shortest", runBenchmark(10, 1000, [&]() {
        auto r = routing::Router::findRouteDijkstra(genNetwork, 0, 24, routing::RoutingObjective::Shortest);
    }));

    // 2. MONTREAL (OSM IMPORT)
    std::cout << "\n--- MONTREAL (" << pbfFile << ") ---\n";
    model::RoadNetwork mtlNetwork;
    printResult("OSM Import Full", runBenchmark(0, 1, [&]() {
        smart_mobility::model::MontrealOSMLoader loader;
        mtlNetwork = loader.load(pbfFile);
    }));
    
    if (mtlNetwork.getNodes().empty()) {
        std::cerr << "Failed to load Montreal network for benchmarks.\n";
        return 1;
    }

    // Find valid nodes for Montreal routing
    int mtlShortStart = mtlNetwork.getNodes().front().id;
    int mtlShortEnd = mtlShortStart;
    int mtlLongEnd = mtlNetwork.getNodes().back().id;

    // A hacky way to find a valid route target
    std::cout << "Finding initial route from " << mtlShortStart << " to " << mtlLongEnd << "...\n";
    auto shortRoute = routing::Router::findRouteAStar(mtlNetwork, mtlShortStart, mtlLongEnd, routing::RoutingObjective::Fastest);
    std::cout << "Initial route found: " << shortRoute.found << " nodes explored: " << shortRoute.nodesExplored << "\n";
    if (shortRoute.found && shortRoute.nodeIds.size() > 5) {
        mtlShortEnd = shortRoute.nodeIds[4]; // ~4 hops
        mtlLongEnd = shortRoute.nodeIds.back(); // full
    } else {
        // Fallback or skip
        std::cout << "No valid long route found! Skipping Montreal routing benchmarks.\n";
    }

    printResult("Mtl Dijkstra Short (4 hops)", runBenchmark(10, 100, [&]() {
        auto r = routing::Router::findRouteDijkstra(mtlNetwork, mtlShortStart, mtlShortEnd, routing::RoutingObjective::Fastest);
    }));
    
    printResult("Mtl A* Short (4 hops)", runBenchmark(10, 100, [&]() {
        auto r = routing::Router::findRouteAStar(mtlNetwork, mtlShortStart, mtlShortEnd, routing::RoutingObjective::Fastest);
    }));
    
    printResult("Mtl Dijkstra Long", runBenchmark(5, 50, [&]() {
        auto r = routing::Router::findRouteDijkstra(mtlNetwork, mtlShortStart, mtlLongEnd, routing::RoutingObjective::Fastest);
    }));
    
    printResult("Mtl A* Long", runBenchmark(5, 50, [&]() {
        auto r = routing::Router::findRouteAStar(mtlNetwork, mtlShortStart, mtlLongEnd, routing::RoutingObjective::Fastest);
    }));

    // 3. SIMULATION
    std::cout << "\n--- SIMULATION (Generated City) ---\n";
    auto simTest = [&](int vehicles) {
        simulation::SimulationEngine engine(genNetwork);
        engine.spawnVehiclesBatch(vehicles);
        printResult("Sim Update " + std::to_string(vehicles) + " veh", runBenchmark(5, 100, [&]() {
            engine.update(1.0);
        }));
    };
    simTest(10);
    simTest(50);
    simTest(100);
    simTest(500);

    // 4. RECOMMENDATIONS
    std::cout << "\n--- RECOMMENDATIONS ---\n";
    std::vector<recommendation::RouteCandidate> candidates;
    printResult("Candidate Gen (0->24)", runBenchmark(10, 1000, [&]() {
        candidates = recommendation::RouteCandidateGenerator::generateCandidates(genNetwork, 0, 24);
    }));
    
    recommendation::UserProfile profile = recommendation::makeProfile(recommendation::UserPreference::Balanced);
    printResult("Candidate Scoring", runBenchmark(10, 10000, [&]() {
        auto rec = recommendation::RecommendationEngine::recommend(candidates, profile);
    }));

    std::cout << "\n=========================================\n";
    return 0;
}
