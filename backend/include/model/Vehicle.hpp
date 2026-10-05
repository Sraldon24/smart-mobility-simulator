#pragma once

#include <vector>
#include <cstddef>
#include <string>

namespace model {

enum class VehicleState {
    Waiting,
    Moving,
    Arrived
};

inline std::string vehicleStateToString(VehicleState state) {
    switch (state) {
        case VehicleState::Waiting: return "waiting";
        case VehicleState::Moving: return "moving";
        case VehicleState::Arrived: return "arrived";
        default: return "unknown";
    }
}

struct Vehicle {
    int id;
    int originNodeId;
    int destinationNodeId;
    std::vector<int> routeNodeIds;
    std::size_t currentRouteIndex{0};
    VehicleState state{VehicleState::Waiting};

    double distanceAlongCurrentRoadMeters{0.0};
    int currentNodeId{-1};
    int nextNodeId{-1};
    double progressOnCurrentRoad{0.0};
    double x{0.0};
    double y{0.0};
    
    double timeSinceLastReroute{0.0};
};

} // namespace model
