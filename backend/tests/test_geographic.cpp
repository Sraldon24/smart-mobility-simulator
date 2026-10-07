#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "model/MontrealOSMLoader.hpp"
#include "routing/Router.hpp"

using namespace smart_mobility::model;

TEST_CASE("Geographic Operations", "[geographic]") {
    SECTION("Haversine distance") {
        // Montreal coordinates to approx 100m away
        double dist = MontrealOSMLoader::calculateHaversineDistance(45.5017, -73.5673, 45.5018, -73.5673);
        REQUIRE(dist > 10.0);
        REQUIRE(dist < 20.0); // 0.0001 deg lat is approx 11 meters
    }
    
    SECTION("Synthetic Geographic Network Routing") {
        model::RoadNetwork geoNetwork;
        geoNetwork.setCoordinateSystem(model::CoordinateSystem::Geographic);
        
        // Node 1: 45.5017, -73.5673
        // Node 2: 45.5018, -73.5673 (approx 11.1m North)
        // Node 3: 45.5018, -73.5672 (approx 7.8m East)
        geoNetwork.addNode(model::Node{1, -73.5673, 45.5017});
        geoNetwork.addNode(model::Node{2, -73.5673, 45.5018});
        geoNetwork.addNode(model::Node{3, -73.5672, 45.5018});
        
        double d12 = MontrealOSMLoader::calculateHaversineDistance(45.5017, -73.5673, 45.5018, -73.5673);
        double d23 = MontrealOSMLoader::calculateHaversineDistance(45.5018, -73.5673, 45.5018, -73.5672);
        
        // One-way 1->2
        geoNetwork.addRoad(model::Road{1, 2, d12, 50.0});
        // One-way 2->3
        geoNetwork.addRoad(model::Road{2, 3, d23, 50.0});
        
        geoNetwork.buildIndex();
        
        // Test routing
        auto res = routing::Router::findRouteAStar(geoNetwork, 1, 3, routing::RoutingObjective::Shortest);
        REQUIRE(res.found);
        REQUIRE(res.nodeIds == std::vector<int>{1, 2, 3});
        REQUIRE_THAT(res.totalCost, Catch::Matchers::WithinAbs(d12 + d23, 0.001));
        
        // Test one-way constraint
        auto resBack = routing::Router::findRouteDijkstra(geoNetwork, 3, 1, routing::RoutingObjective::Shortest);
        REQUIRE_FALSE(resBack.found);
    }
}
