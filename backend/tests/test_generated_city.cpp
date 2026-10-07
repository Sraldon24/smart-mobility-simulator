#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include "model/GeneratedCityLoader.hpp"

using namespace model;

TEST_CASE("Generated City properties", "[generated_city]") {
    auto network = GeneratedCityLoader::generate5x5Grid();

    SECTION("Size constraints") {
        REQUIRE(network.getNodes().size() == 25);
        // 5x5 grid has 4 segments per row (20) + 4 per column (20) = 40 edges.
        // Directed roads = 40 * 2 = 80.
        REQUIRE(network.getRoads().size() == 80);
    }

    SECTION("Node coordinates and IDs") {
        auto n0 = network.getNodeById(0);
        REQUIRE(n0 != nullptr);
        REQUIRE(n0->x == Catch::Approx(0.0));
        REQUIRE(n0->y == Catch::Approx(0.0));

        auto n24 = network.getNodeById(24);
        REQUIRE(n24 != nullptr);
        // 4 intervals of 100m = 400m
        REQUIRE(n24->x == Catch::Approx(400.0));
        REQUIRE(n24->y == Catch::Approx(400.0));
    }

    SECTION("Node neighbors") {
        // Corner node (0,0) -> neighbors should be 1 and 5
        REQUIRE(network.getRoad(0, 1) != nullptr);
        REQUIRE(network.getRoad(0, 5) != nullptr);
        REQUIRE(network.getRoad(0, 2) == nullptr);
        
        // Edge node (1,0) -> ID 1 -> neighbors 0, 2, 6
        REQUIRE(network.getRoad(1, 0) != nullptr);
        REQUIRE(network.getRoad(1, 2) != nullptr);
        REQUIRE(network.getRoad(1, 6) != nullptr);
        REQUIRE(network.getRoad(1, 7) == nullptr);

        // Center node (2,2) -> ID 12 -> neighbors 7, 11, 13, 17
        REQUIRE(network.getRoad(12, 7) != nullptr);
        REQUIRE(network.getRoad(12, 11) != nullptr);
        REQUIRE(network.getRoad(12, 13) != nullptr);
        REQUIRE(network.getRoad(12, 17) != nullptr);
    }
    
    SECTION("Deterministic output") {
        auto network2 = GeneratedCityLoader::generate5x5Grid();
        REQUIRE(network.getNodes().size() == network2.getNodes().size());
        REQUIRE(network.getRoads().size() == network2.getRoads().size());
        for(size_t i = 0; i < network.getNodes().size(); ++i) {
            REQUIRE(network.getNodes()[i].id == network2.getNodes()[i].id);
        }
    }
}
