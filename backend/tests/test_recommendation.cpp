#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "recommendation/RecommendationEngine.hpp"
#include "recommendation/RouteCandidateGenerator.hpp"
#include "model/RoadNetwork.hpp"
#include "model/GeneratedCityLoader.hpp"

using namespace recommendation;
using namespace model;
using namespace Catch::Matchers;

TEST_CASE("Recommendation Profiles", "[recommendation]") {
    SECTION("Profile Weights") {
        UserProfile fastest = makeProfile(UserPreference::Fastest);
        REQUIRE(fastest.timeWeight > fastest.distanceWeight);
        REQUIRE(fastest.timeWeight > fastest.trafficWeight);
        REQUIRE(fastest.isValid());

        UserProfile shortest = makeProfile(UserPreference::Shortest);
        REQUIRE(shortest.distanceWeight > shortest.timeWeight);
        REQUIRE(shortest.isValid());

        UserProfile leastTraffic = makeProfile(UserPreference::LeastTraffic);
        REQUIRE(leastTraffic.trafficWeight > leastTraffic.timeWeight);
        REQUIRE(leastTraffic.isValid());
        
        UserProfile cheapest = makeProfile(UserPreference::Cheapest);
        REQUIRE(cheapest.costWeight > cheapest.timeWeight);
        REQUIRE(cheapest.isValid());
        
        UserProfile balanced = makeProfile(UserPreference::Balanced);
        REQUIRE(balanced.isValid());
    }
}

TEST_CASE("Route Candidate Generation", "[recommendation]") {
    RoadNetwork network;
    network = GeneratedCityLoader::generate5x5Grid();
    
    
    SECTION("Valid candidates") {
        auto candidates = RouteCandidateGenerator::generateCandidates(network, 0, 24);
        
        REQUIRE(!candidates.empty());
        for (const auto& candidate : candidates) {
            REQUIRE(!candidate.nodeIds.empty());
            REQUIRE(candidate.nodeIds.front() == 0);
            REQUIRE(candidate.nodeIds.back() == 24);
            REQUIRE(candidate.totalDistanceMeters > 0);
            REQUIRE(candidate.travelTimeSeconds > 0);
        }
    }
    
    SECTION("No route scenarios") {
        auto candidates = RouteCandidateGenerator::generateCandidates(network, 0, 9999);
        REQUIRE(candidates.empty());
    }
}

TEST_CASE("Recommendation Engine Scoring", "[recommendation]") {
    std::vector<RouteCandidate> candidates = {
        {{0, 1, 2}, 1000.0, 300.0, 1.0, 5.0, "shortest"},
        {{0, 3, 2}, 1500.0, 200.0, 1.2, 8.0, "fastest"},
        {{0, 4, 2}, 2000.0, 400.0, 0.8, 2.0, "least_traffic"}
    };
    
    SECTION("Fastest Profile prefers fastest route") {
        auto result = RecommendationEngine::recommend(candidates, makeProfile(UserPreference::Fastest));
        REQUIRE(result.recommendedRoute.candidate.travelTimeSeconds == Catch::Approx(200.0));
        REQUIRE(result.recommendedRoute.candidate.sourceObjective == "fastest");
    }
    
    SECTION("Shortest Profile prefers shortest route") {
        auto result = RecommendationEngine::recommend(candidates, makeProfile(UserPreference::Shortest));
        REQUIRE(result.recommendedRoute.candidate.totalDistanceMeters == Catch::Approx(1000.0));
        REQUIRE(result.recommendedRoute.candidate.sourceObjective == "shortest");
    }
    
    SECTION("Least Traffic Profile prefers least congested route") {
        auto result = RecommendationEngine::recommend(candidates, makeProfile(UserPreference::LeastTraffic));
        REQUIRE(result.recommendedRoute.candidate.averageCongestion == Catch::Approx(0.8));
    }
    
    SECTION("Cheapest Profile prefers cheapest route") {
        auto result = RecommendationEngine::recommend(candidates, makeProfile(UserPreference::Cheapest));
        REQUIRE(result.recommendedRoute.candidate.estimatedCost == Catch::Approx(2.0));
    }
    
    SECTION("Single candidate") {
        std::vector<RouteCandidate> single = {candidates[0]};
        auto result = RecommendationEngine::recommend(single, makeProfile(UserPreference::Balanced));
        REQUIRE(result.recommendedRoute.candidate.sourceObjective == "shortest");
        // Check normalization for single element doesn't divide by zero
        REQUIRE_THAT(result.recommendedRoute.normalizedTime, WithinAbs(0.0, 1e-6));
    }
    
    SECTION("Empty candidates throws or returns empty") {
        std::vector<RouteCandidate> empty;
        auto result = RecommendationEngine::recommend(empty, makeProfile(UserPreference::Balanced));
        REQUIRE(result.allCandidates.empty());
    }
}
