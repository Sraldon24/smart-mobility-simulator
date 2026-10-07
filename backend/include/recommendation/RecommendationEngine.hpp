#pragma once
#include "recommendation/UserProfile.hpp"
#include "recommendation/RouteCandidateGenerator.hpp"
#include <algorithm>
#include <vector>
#include <string>

namespace recommendation {

struct CandidateSetMetrics {
    double minTravelTime{0.0}, maxTravelTime{0.0};
    double minDistance{0.0}, maxDistance{0.0};
    double minCongestion{0.0}, maxCongestion{0.0};
    double minCost{0.0}, maxCost{0.0};

    void computeFrom(const std::vector<RouteCandidate>& candidates) {
        if (candidates.empty()) return;
        
        minTravelTime = maxTravelTime = candidates[0].travelTimeSeconds;
        minDistance = maxDistance = candidates[0].totalDistanceMeters;
        minCongestion = maxCongestion = candidates[0].averageCongestion;
        minCost = maxCost = candidates[0].estimatedCost;

        for (const auto& c : candidates) {
            minTravelTime = std::min(minTravelTime, c.travelTimeSeconds);
            maxTravelTime = std::max(maxTravelTime, c.travelTimeSeconds);
            
            minDistance = std::min(minDistance, c.totalDistanceMeters);
            maxDistance = std::max(maxDistance, c.totalDistanceMeters);
            
            minCongestion = std::min(minCongestion, c.averageCongestion);
            maxCongestion = std::max(maxCongestion, c.averageCongestion);
            
            minCost = std::min(minCost, c.estimatedCost);
            maxCost = std::max(maxCost, c.estimatedCost);
        }
    }
};

struct ScoredCandidate {
    RouteCandidate candidate;
    double totalScore;
    
    double normalizedTime;
    double normalizedDistance;
    double normalizedTraffic;
    double normalizedCost;

    double timeContribution;
    double distanceContribution;
    double trafficContribution;
    double costContribution;
};

struct RecommendationResult {
    ScoredCandidate recommendedRoute;
    std::vector<ScoredCandidate> allCandidates;
    std::string explanation;
};

class RecommendationEngine {
public:
    static RecommendationResult recommend(
        const std::vector<RouteCandidate>& candidates, 
        const UserProfile& profile
    );
private:
    static ScoredCandidate scoreCandidate(
        const RouteCandidate& candidate, 
        const CandidateSetMetrics& setMetrics, 
        const UserProfile& profile
    );
};

} // namespace recommendation

