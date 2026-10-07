#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "routing/Router.hpp"
#include "model/GeneratedCityLoader.hpp"

using namespace routing;
using namespace model;

TEST_CASE("Traffic and Incidents", "[traffic]") {
    auto network = GeneratedCityLoader::generate5x5Grid();

    SECTION("Traffic factors affect travel time") {
        auto* r = network.getRoadMutable(0, 1);
        REQUIRE(r != nullptr);
        
        double baseTime = r->getTravelTimeSeconds();
        
        r->dynamicCongestionFactor = 2.0;
        double congestedTime = r->getTravelTimeSeconds();
        
        REQUIRE_THAT(congestedTime, Catch::Matchers::WithinAbs(baseTime * 2.0, 0.001));
        
        r->incidentFactor = 5.0;
        double incidentTime = r->getTravelTimeSeconds();
        
        REQUIRE_THAT(incidentTime, Catch::Matchers::WithinAbs(baseTime * 10.0, 0.001));
    }

    SECTION("Incidents affect routing") {
        // Find baseline shortest and fastest
        auto baseShortest = Router::findRouteDijkstra(network, 0, 24, RoutingObjective::Shortest);
        auto baseFastest = Router::findRouteDijkstra(network, 0, 24, RoutingObjective::Fastest);
        
        REQUIRE(baseShortest.found);
        REQUIRE(baseFastest.found);

        // Add closure on route
        int nodeOnRoute = baseShortest.nodeIds[1];
        network.addIncident(0, nodeOnRoute, IncidentType::Closure);
        
        auto closedShortest = Router::findRouteDijkstra(network, 0, 24, RoutingObjective::Shortest);
        REQUIRE(closedShortest.found);
        // It should avoid the closed road
        REQUIRE(closedShortest.nodeIds[1] != nodeOnRoute);
        // Cost should be higher (or equal if another optimal route exists)
        REQUIRE(closedShortest.totalCost >= baseShortest.totalCost);

        // Remove incident
        network.removeIncident(0, nodeOnRoute);
        auto restoredShortest = Router::findRouteDijkstra(network, 0, 24, RoutingObjective::Shortest);
        REQUIRE_THAT(restoredShortest.totalCost, Catch::Matchers::WithinAbs(baseShortest.totalCost, 0.001));
        
        // Add Accident
        network.addIncident(0, nodeOnRoute, IncidentType::Accident);
        auto accidentFastest = Router::findRouteDijkstra(network, 0, 24, RoutingObjective::Fastest);
        REQUIRE(accidentFastest.found);
        // Fastest route should avoid the accident if alternative is faster
        if (accidentFastest.nodeIds[1] == nodeOnRoute) {
            REQUIRE(accidentFastest.estimatedTravelTimeSeconds > baseFastest.estimatedTravelTimeSeconds);
        } else {
            // It rerouted
            REQUIRE(accidentFastest.estimatedTravelTimeSeconds >= baseFastest.estimatedTravelTimeSeconds);
        }
    }
}
