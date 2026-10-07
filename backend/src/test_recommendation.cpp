#include "recommendation/RecommendationEngine.hpp"
#include <iostream>
#include <cassert>
#include <vector>

using namespace recommendation;

void verifyProfileWeights() {
    std::vector<UserPreference> prefs = {
        UserPreference::Fastest,
        UserPreference::Shortest,
        UserPreference::LeastTraffic,
        UserPreference::Cheapest,
        UserPreference::Balanced
    };

    for (auto pref : prefs) {
        auto profile = makeProfile(pref);
        assert(profile.isValid() && "Profile weights must sum to 1.0");
    }
    std::cout << "Profile weights validated successfully.\n";
}

void verifyScoring() {
    // Artificial candidate routes
    RouteCandidate routeA = { {0}, 5000.0, 700.0, 3.0, 5000.0 * 0.05, "a" }; 
    RouteCandidate routeB = { {1}, 8000.0, 500.0, 1.5, 8000.0 * 0.05, "b" };
    RouteCandidate routeC = { {2}, 9000.0, 800.0, 1.0, 9000.0 * 0.05, "c" }; 
    RouteCandidate routeD = { {3}, 4000.0, 900.0, 2.0, 4000.0 * 0.01, "d" }; 

    std::vector<RouteCandidate> candidates = {routeA, routeB, routeC, routeD};

    auto res_fastest = RecommendationEngine::recommend(candidates, makeProfile(UserPreference::Fastest));
    assert(res_fastest.recommendedRoute.candidate.sourceObjective == "b" && "Fastest should favor route B");

    auto res_shortest = RecommendationEngine::recommend(candidates, makeProfile(UserPreference::Shortest));
    assert(res_shortest.recommendedRoute.candidate.sourceObjective == "d" && "Shortest should favor route D"); // wait, A is 5000, D is 4000, so D is shortest.

    auto res_traffic = RecommendationEngine::recommend(candidates, makeProfile(UserPreference::LeastTraffic));
    assert(res_traffic.recommendedRoute.candidate.sourceObjective == "c" && "LeastTraffic should favor route C");

    auto res_cheapest = RecommendationEngine::recommend(candidates, makeProfile(UserPreference::Cheapest));
    assert(res_cheapest.recommendedRoute.candidate.sourceObjective == "d" && "Cheapest should favor route D");

    std::cout << "Scoring behavior validated successfully.\n";
    
    std::cout << "\nExample Balanced Recommendation:\n";
    auto res_balanced = RecommendationEngine::recommend(candidates, makeProfile(UserPreference::Balanced));
    std::cout << "Recommended route source: " << res_balanced.recommendedRoute.candidate.sourceObjective << "\n";
    std::cout << "Total: " << res_balanced.recommendedRoute.totalScore << "\n";
    std::cout << "Explanation: " << res_balanced.explanation << "\n";
}

int main() {
    verifyProfileWeights();
    verifyScoring();
    return 0;
}

