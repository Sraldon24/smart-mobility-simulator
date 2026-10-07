#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>

#include "metrics/MetricsCollector.hpp"
#include "api/HttpServer.hpp"
#include "model/RoadNetwork.hpp"
#include "simulation/SimulationEngine.hpp"

using namespace metrics;
using json = nlohmann::json;

TEST_CASE("MetricsCollector state updates", "[metrics]") {
    auto& collector = MetricsCollector::getInstance();
    collector.resetSimulation();
    
    SECTION("Initial state") {
        auto snapshot = collector.getSnapshot();
        REQUIRE(snapshot.simulation.vehicleCount == 0);
        REQUIRE(snapshot.traffic.totalReroutes == 0);
    }
    
    SECTION("Recording route result") {
        routing::RouteResult rr;
        rr.found = true;
        rr.totalCost = 1500.0;
        rr.estimatedTravelTimeSeconds = 300.0;
        rr.nodesExplored = 42;
        rr.runtimeMicroseconds = 120;
        
        collector.recordRouteResult("dijkstra", rr);
        
        auto snapshot = collector.getSnapshot();
        REQUIRE(snapshot.routing.lastAlgorithm == "dijkstra");
        REQUIRE(snapshot.routing.dijkstraNodes == 42);
        REQUIRE(snapshot.routing.dijkstraRuntime == 120);
        REQUIRE(snapshot.routing.dijkstraDistance == Catch::Approx(1500.0));
    }
    
    SECTION("Recording simulation update") {
        collector.recordSimulationUpdate(10.5, 100, 20, 70, 10, 45.0, 250.0);
        
        auto snapshot = collector.getSnapshot();
        REQUIRE(snapshot.simulation.simulationTimeSeconds == Catch::Approx(10.5));
        REQUIRE(snapshot.simulation.vehicleCount == 100);
        REQUIRE(snapshot.simulation.moving == 70);
    }
    
    SECTION("Recording traffic update") {
        collector.recordTrafficUpdate(5, 1.25, 15);
        
        auto snapshot = collector.getSnapshot();
        REQUIRE(snapshot.traffic.congestedRoadCount == 5);
        REQUIRE(snapshot.traffic.averageCongestionFactor == Catch::Approx(1.25));
        REQUIRE(snapshot.traffic.totalReroutes == 15);
    }
}

TEST_CASE("HTTP API valid and invalid inputs", "[api]") {
    model::RoadNetwork network;
    simulation::SimulationEngine engine(network);
    
    // Setup a detached background thread for the server
    std::thread([&]() {
        api::HttpServer server(network, engine, "");
        server.listen("127.0.0.1", 8089);
    }).detach();
    
    // Give the server a moment to start
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    httplib::Client cli("127.0.0.1", 8089);
    
    SECTION("GET /health") {
        if (auto res = cli.Get("/health")) {
            REQUIRE(res->status == 200);
            auto body = json::parse(res->body);
            REQUIRE(body["status"] == "ok");
        } else {
            FAIL("Could not connect to test server");
        }
    }
    
    SECTION("GET /metrics") {
        if (auto res = cli.Get("/metrics")) {
            REQUIRE(res->status == 200);
            auto body = json::parse(res->body);
            REQUIRE(body.contains("routing"));
            REQUIRE(body.contains("simulation"));
            REQUIRE(body.contains("traffic"));
        } else {
            FAIL("Could not connect to test server");
        }
    }
    
    SECTION("POST /mode invalid") {
        if (auto res = cli.Post("/mode", "{\"wrong\":\"json\"}", "application/json")) {
            REQUIRE(res->status == 400); // Missing mode
        } else {
            FAIL("Could not connect to test server");
        }
    }
}
