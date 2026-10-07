#include "recommendation/UserProfile.hpp"

namespace recommendation {

UserProfile makeProfile(UserPreference preference) {
    switch (preference) {
        case UserPreference::Fastest:
            return {0.7, 0.1, 0.1, 0.1};
        case UserPreference::Shortest:
            return {0.1, 0.7, 0.1, 0.1};
        case UserPreference::LeastTraffic:
            return {0.1, 0.1, 0.7, 0.1};
        case UserPreference::Cheapest:
            return {0.1, 0.1, 0.1, 0.7};
        case UserPreference::Balanced:
            return {0.25, 0.25, 0.25, 0.25};
    }
    return {0.25, 0.25, 0.25, 0.25};
}

std::string preferenceToString(UserPreference preference) {
    switch (preference) {
        case UserPreference::Fastest: return "fastest";
        case UserPreference::Shortest: return "shortest";
        case UserPreference::LeastTraffic: return "least_traffic";
        case UserPreference::Cheapest: return "cheapest";
        case UserPreference::Balanced: return "balanced";
    }
    return "balanced";
}

UserPreference stringToPreference(const std::string& str) {
    if (str == "fastest") return UserPreference::Fastest;
    if (str == "shortest") return UserPreference::Shortest;
    if (str == "least_traffic") return UserPreference::LeastTraffic;
    if (str == "cheapest") return UserPreference::Cheapest;
    return UserPreference::Balanced;
}

} // namespace recommendation

