#pragma once

namespace model {

struct Road {
    int from;
    int to;
    double distanceMeters{0.0};
    double speedLimitKph{50.0};
    double baseTrafficFactor{1.0};
    double dynamicCongestionFactor{1.0};
    double incidentFactor{1.0};
    bool closed{false};
    int currentVehicleCount{0};

    double getEffectiveTrafficFactor() const {
        return baseTrafficFactor * dynamicCongestionFactor * incidentFactor;
    }

    double getTravelTimeSeconds() const {
        double speedMetersPerSecond = speedLimitKph / 3.6;
        return (distanceMeters / speedMetersPerSecond) * getEffectiveTrafficFactor();
    }
};

} // namespace model

