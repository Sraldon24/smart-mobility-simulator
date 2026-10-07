#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include "model/RoadNetwork.hpp"

using namespace model;

TEST_CASE("RoadNetwork basic operations", "[roadnetwork]") {
    RoadNetwork network;
    
    SECTION("Empty network") {
        REQUIRE(network.getNodes().empty());
        REQUIRE(network.getRoads().empty());
        REQUIRE(network.getNodeById(1) == nullptr);
    }
    
    SECTION("Adding nodes") {
        network.addNode(Node{1, 10.0, 20.0});
        network.addNode(Node{2, 15.0, 25.0});
        
        REQUIRE(network.getNodes().size() == 2);
        
        auto n1 = network.getNodeById(1);
        REQUIRE(n1 != nullptr);
        REQUIRE(n1->id == 1);
        REQUIRE(n1->x == 10.0);
        REQUIRE(n1->y == 20.0);
        
        auto n2 = network.getNodeById(2);
        REQUIRE(n2 != nullptr);
        
        // Invalid node
        REQUIRE(network.getNodeById(3) == nullptr);
    }
    
    SECTION("Adding roads") {
        network.addNode(Node{1, 0.0, 0.0});
        network.addNode(Node{2, 10.0, 0.0});
        
        Road r1;
        r1.from = 1;
        r1.to = 2;
        r1.distanceMeters = 10.0;
        r1.speedLimitKph = 50.0;
        network.addRoad(r1);
        
        REQUIRE(network.getRoads().size() == 1);
        
        auto r = network.getRoad(1, 2);
        REQUIRE(r != nullptr);
        REQUIRE(r->from == 1);
        REQUIRE(r->to == 2);
        REQUIRE(r->distanceMeters == 10.0);
        REQUIRE(r->speedLimitKph == 50.0);
        
        // Directional test
        REQUIRE(network.getRoad(2, 1) == nullptr);
        
        // Two-way by adding reverse
        Road r2;
        r2.from = 2;
        r2.to = 1;
        r2.distanceMeters = 10.0;
        network.addRoad(r2);
        REQUIRE(network.getRoads().size() == 2);
        REQUIRE(network.getRoad(2, 1) != nullptr);
    }

    SECTION("Road state") {
        network.addNode(Node{1, 0.0, 0.0});
        network.addNode(Node{2, 10.0, 0.0});
        network.addRoad(Road{1, 2, 10.0, 50.0});
        
        network.setRoadClosed(1, 2, true);
        REQUIRE(network.getRoad(1, 2)->closed == true);
        
        network.setRoadClosed(1, 2, false);
        REQUIRE(network.getRoad(1, 2)->closed == false);
    }
}
