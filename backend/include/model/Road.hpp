#pragma once

namespace model {

struct Road {
    int from;
    int to;
    double distanceMeters{0.0};
    double speedLimitKph{50.0};
    double trafficFactor{1.0};
    bool closed{false};
};

} // namespace model

