#include "recommendation/RecommendationEngine.hpp"
#include <sstream>

namespace recommendation {

ScoredCandidate RecommendationEngine::scoreCandidate(
    const RouteCandidate& candidate, 
    const CandidateSetMetrics& setMetrics, 
    const UserProfile& profile
) {
    // EDUCATIONAL NOTE: Min-Max Normalization
    // WHY: We need to combine different units (seconds, meters, congestion factor, dollars).
    // HOW: We map every value to a [0.0, 1.0] scale relative to the other candidates in the set.
    // A value of 0.0 means it is the best (lowest) in the set, 1.0 means it is the worst.
    auto normalize = [](double val, double minVal, double maxVal) {
        if (maxVal - minVal <= 1e-6) return 0.0;
        return (val - minVal) / (maxVal - minVal);
    };

    double normalizedTime = normalize(candidate.travelTimeSeconds, setMetrics.minTravelTime, setMetrics.maxTravelTime);
    double normalizedDist = normalize(candidate.totalDistanceMeters, setMetrics.minDistance, setMetrics.maxDistance);
    double normalizedCong = normalize(candidate.averageCongestion, setMetrics.minCongestion, setMetrics.maxCongestion);
    double normalizedCost = normalize(candidate.estimatedCost, setMetrics.minCost, setMetrics.maxCost);

    ScoredCandidate scored;
    scored.candidate = candidate;
    scored.normalizedTime = normalizedTime;
    scored.normalizedDistance = normalizedDist;
    scored.normalizedTraffic = normalizedCong;
    scored.normalizedCost = normalizedCost;

    // EDUCATIONAL NOTE: Multi-Objective Weighted Scoring
    // User preferences (profile weights) are applied to the normalized values.
    // Lower score is better. If a user sets a weight to 0.0, that factor is ignored.
    scored.timeContribution = profile.timeWeight * normalizedTime;
    scored.distanceContribution = profile.distanceWeight * normalizedDist;
    scored.trafficContribution = profile.trafficWeight * normalizedCong;
    scored.costContribution = profile.costWeight * normalizedCost;
    
    scored.totalScore = scored.timeContribution + scored.distanceContribution + scored.trafficContribution + scored.costContribution;
    
    return scored;
}

RecommendationResult RecommendationEngine::recommend(
    const std::vector<RouteCandidate>& candidates, 
    const UserProfile& profile
) {
    RecommendationResult result;
    if (candidates.empty()) return result;

    CandidateSetMetrics setMetrics;
    setMetrics.computeFrom(candidates);

    for (const auto& c : candidates) {
        result.allCandidates.push_back(scoreCandidate(c, setMetrics, profile));
    }

    // Sort by total score
    std::sort(result.allCandidates.begin(), result.allCandidates.end(), 
        [](const ScoredCandidate& a, const ScoredCandidate& b) {
            return a.totalScore < b.totalScore;
        });

    result.recommendedRoute = result.allCandidates.front();

    // Generate simple deterministic explanation
    std::stringstream ss;
    if (candidates.size() == 1) {
        ss << "This is the only viable route available.";
    } else {
        bool lowestTime = std::abs(result.recommendedRoute.candidate.travelTimeSeconds - setMetrics.minTravelTime) < 1e-3;
        bool lowestDist = std::abs(result.recommendedRoute.candidate.totalDistanceMeters - setMetrics.minDistance) < 1e-3;
        bool lowestCong = std::abs(result.recommendedRoute.candidate.averageCongestion - setMetrics.minCongestion) < 1e-3;
        bool lowestCost = std::abs(result.recommendedRoute.candidate.estimatedCost - setMetrics.minCost) < 1e-3;

        std::vector<std::string> benefits;
        if (lowestTime) benefits.push_back("lowest estimated travel time");
        if (lowestDist) benefits.push_back("lowest total distance");
        if (lowestCong) benefits.push_back("lowest congestion");
        if (lowestCost) benefits.push_back("lowest estimated cost");

        if (!benefits.empty()) {
            ss << "Recommended because it has the ";
            for (size_t i = 0; i < benefits.size(); ++i) {
                if (i > 0 && i == benefits.size() - 1) ss << " and ";
                else if (i > 0) ss << ", ";
                ss << benefits[i];
            }
            ss << ".";
        } else {
            ss << "Recommended because it balances your priorities better than alternatives, avoiding worst-case extremes.";
        }
    }
    result.explanation = ss.str();

    return result;
}

} // namespace recommendation

