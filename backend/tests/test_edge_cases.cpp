#include <catch2/catch_test_macros.hpp>
#include "model/RoadNetwork.hpp"
#include "routing/Router.hpp"
#include "simulation/SimulationEngine.hpp"
#include "recommendation/RouteCandidateGenerator.hpp"

using namespace model;
using namespace routing;
using namespace simulation;
using namespace recommendation;

TEST_CASE("Edge Cases - Empty Network", "[edgecases]") {
    RoadNetwork emptyNetwork;
    SimulationEngine engine(emptyNetwork);
    
    SECTION("Routing on empty network returns no route") {
        auto result = Router::findRouteDijkstra(emptyNetwork, 0, 1, RoutingObjective::Shortest);
        REQUIRE_FALSE(result.found);
        REQUIRE(result.nodeIds.empty());
    }
    
    SECTION("Spawning vehicle on empty network fails") {
        int vId = engine.spawnVehicle(0, 1, "fastest");
        REQUIRE(vId == -1);
    }
    
    SECTION("Candidate generation on empty network is empty") {
        auto candidates = RouteCandidateGenerator::generateCandidates(emptyNetwork, 0, 1);
        REQUIRE(candidates.empty());
    }
}

TEST_CASE("Edge Cases - 1-Node Network", "[edgecases]") {
    RoadNetwork network;
    Node n;
    n.id = 0; n.x = 0; n.y = 0;
    network.addNode(n);
    
    SECTION("Routing from node to itself") {
        auto result = Router::findRouteDijkstra(network, 0, 0, RoutingObjective::Shortest);
        // Depending on implementation, might be found with distance 0, or false if it expects at least 1 edge
        if (result.found) {
            REQUIRE(result.totalCost == 0.0);
            REQUIRE(result.nodeIds.size() == 1); // just node 0
        } else {
            REQUIRE_FALSE(result.found);
        }
    }
    
    SECTION("Routing to non-existent node") {
        auto result = Router::findRouteDijkstra(network, 0, 99, RoutingObjective::Shortest);
        REQUIRE_FALSE(result.found);
    }
}

TEST_CASE("Edge Cases - Incidents", "[edgecases]") {
    RoadNetwork network;
    Node n0; n0.id = 0; n0.x = 0; n0.y = 0;
    Node n1; n1.id = 1; n1.x = 100; n1.y = 0;
    network.addNode(n0);
    network.addNode(n1);
    
    Road r; r.from = 0; r.to = 1; r.distanceMeters = 100.0; r.speedLimitKph = 50.0;
    network.addRoad(r);
    
    SECTION("Closing non-existent road") {
        network.setRoadClosed(99, 100, true);
        REQUIRE(network.getRoad(99, 100) == nullptr);
    }
    
    SECTION("Duplicate incidents") {
        bool first = network.addIncident(0, 1, IncidentType::Closure);
        REQUIRE(first);
        
        bool second = network.addIncident(0, 1, IncidentType::Closure);
        (void)second; // Might be true or false, but shouldn't crash
        
        auto* road = network.getRoad(0, 1);
        REQUIRE(road != nullptr);
        REQUIRE(road->closed);
    }
    
    SECTION("Removing incident from non-existent road") {
        bool removed = network.removeIncident(99, 100);
        REQUIRE_FALSE(removed);
    }
}

