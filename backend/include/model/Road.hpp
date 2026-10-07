#pragma once

namespace model {

/*
 * Represents a single directed edge in the graph.
 * Note: A physical two-way street is stored as two separate Road structs,
 * one for each direction, because traffic or incidents could affect one
 * direction but not the other.
 */
struct Road {
    int from;
    int to;
    double distanceMeters{0.0};
    double speedLimitKph{50.0};
    
    // Traffic model components. Base is standard speed, 
    // dynamic rises when vehicles occupy the road, and 
    // incident acts as a severe multiplier for accidents.
    double baseTrafficFactor{1.0};
    double dynamicCongestionFactor{1.0};
    double incidentFactor{1.0};
    bool closed{false};
    int currentVehicleCount{0};

    // Centralized cost calculation. 
    // By keeping this logic here, the Router algorithms remain 
    // independent of HOW traffic effects are actually combined.
    double getEffectiveTrafficFactor() const {
        return baseTrafficFactor * dynamicCongestionFactor * incidentFactor;
    }

    // trailing 'const' promises this method won't modify the Road object.
    double getTravelTimeSeconds() const {
        double speedMetersPerSecond = speedLimitKph / 3.6;
        return (distanceMeters / speedMetersPerSecond) * getEffectiveTrafficFactor();
    }
};

} // namespace model

