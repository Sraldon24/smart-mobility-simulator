#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "routing/Router.hpp"
#include "model/GeneratedCityLoader.hpp"

using namespace routing;

TEST_CASE("Routing basics", "[routing]") {
    auto network = model::GeneratedCityLoader::generate5x5Grid();

    SECTION("Dijkstra standard routes") {
        auto res1 = Router::findRouteDijkstra(network, 0, 1, RoutingObjective::Shortest);
        REQUIRE(res1.found);
        REQUIRE(res1.nodeIds == std::vector<int>{0, 1});
        REQUIRE_THAT(res1.totalCost, Catch::Matchers::WithinAbs(100.0, 0.001));

        auto res4 = Router::findRouteDijkstra(network, 0, 4, RoutingObjective::Shortest);
        REQUIRE(res4.found);
        REQUIRE(res4.nodeIds.front() == 0);
        REQUIRE(res4.nodeIds.back() == 4);
        REQUIRE_THAT(res4.totalCost, Catch::Matchers::WithinAbs(400.0, 0.001));

        auto res24 = Router::findRouteDijkstra(network, 0, 24, RoutingObjective::Shortest);
        REQUIRE(res24.found);
        REQUIRE(res24.nodeIds.front() == 0);
        REQUIRE(res24.nodeIds.back() == 24);
        REQUIRE_THAT(res24.totalCost, Catch::Matchers::WithinAbs(800.0, 0.001));

        auto res12 = Router::findRouteDijkstra(network, 12, 12, RoutingObjective::Shortest);
        REQUIRE(res12.found);
        REQUIRE(res12.nodeIds == std::vector<int>{12});
        REQUIRE_THAT(res12.totalCost, Catch::Matchers::WithinAbs(0.0, 0.001));
    }

    SECTION("A* standard routes") {
        auto res1 = Router::findRouteAStar(network, 0, 1, RoutingObjective::Shortest);
        REQUIRE(res1.found);
        REQUIRE_THAT(res1.totalCost, Catch::Matchers::WithinAbs(100.0, 0.001));

        auto res24 = Router::findRouteAStar(network, 0, 24, RoutingObjective::Shortest);
        REQUIRE(res24.found);
        REQUIRE_THAT(res24.totalCost, Catch::Matchers::WithinAbs(800.0, 0.001));
    }

    SECTION("Dijkstra vs A* Equivalence") {
        auto dRes = Router::findRouteDijkstra(network, 0, 24, RoutingObjective::Shortest);
        auto aRes = Router::findRouteAStar(network, 0, 24, RoutingObjective::Shortest);
        
        REQUIRE(dRes.found == true);
        REQUIRE(aRes.found == true);
        REQUIRE_THAT(dRes.totalCost, Catch::Matchers::WithinAbs(aRes.totalCost, 0.001));
    }

    SECTION("Invalid inputs") {
        auto resBadStart = Router::findRoute(network, 999, 24, RoutingAlgorithm::Dijkstra, RoutingObjective::Shortest);
        REQUIRE_FALSE(resBadStart.found);

        auto resBadDest = Router::findRoute(network, 0, 999, RoutingAlgorithm::AStar, RoutingObjective::Shortest);
        REQUIRE_FALSE(resBadDest.found);
    }
}
