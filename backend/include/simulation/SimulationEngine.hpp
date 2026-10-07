#pragma once

#include "model/RoadNetwork.hpp"
#include "model/Vehicle.hpp"
#include <vector>

namespace simulation {

class SimulationEngine {
public:
    explicit SimulationEngine(model::RoadNetwork& network);
    
    int spawnVehicle(int originNodeId, int destinationNodeId, const std::string& preferenceStr = "balanced");
    int spawnVehiclesBatch(int count);
    
    const std::vector<model::Vehicle>& getVehicles() const;
    void reset();
    
    void update(double deltaTimeSeconds);
    
    double getSimulationTimeSeconds() const;
    
    int getWaitingVehicleCount() const;
    int getMovingVehicleCount() const;
    int getArrivedVehicleCount() const;

    int getCongestedRoadCount() const;
    double getAverageCongestionFactor() const;
    int getTotalReroutes() const;

private:
    model::RoadNetwork& network;
    std::vector<model::Vehicle> vehicles;
    std::vector<model::Road*> activeRoads;
    double simulationTimeSeconds{0.0};
    int nextVehicleId{1};
    int totalReroutes{0};
};

} // namespace simulation
