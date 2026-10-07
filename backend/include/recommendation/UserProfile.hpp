#pragma once
#include <string>
#include <stdexcept>
#include <cmath>

namespace recommendation {

enum class UserPreference {
    Fastest,
    Shortest,
    LeastTraffic,
    Cheapest,
    Balanced
};

struct UserProfile {
    double timeWeight;
    double distanceWeight;
    double trafficWeight;
    double costWeight;

    bool isValid() const {
        double sum = timeWeight + distanceWeight + trafficWeight + costWeight;
        return std::abs(sum - 1.0) < 1e-6;
    }
};

UserProfile makeProfile(UserPreference preference);
std::string preferenceToString(UserPreference preference);
UserPreference stringToPreference(const std::string& str);

} // namespace recommendation

