#include "simulation/SimulationEngine.hpp"
#include "routing/Router.hpp"
#include <stdexcept>

namespace simulation {

SimulationEngine::SimulationEngine(model::RoadNetwork& network)
    : network(network) {}

int SimulationEngine::spawnVehicle(int originNodeId, int destinationNodeId) {
    if (!network.getNodeById(originNodeId) || !network.getNodeById(destinationNodeId)) {
        return -1; // Invalid nodes
    }

    auto result = routing::Router::findRoute(
        network, originNodeId, destinationNodeId, 
        routing::RoutingAlgorithm::AStar, 
        routing::RoutingObjective::Fastest
    );

    if (!result.found) {
        return -1; // No route available
    }

    model::Vehicle v;
    v.id = nextVehicleId++;
    v.originNodeId = originNodeId;
    v.destinationNodeId = destinationNodeId;
    v.routeNodeIds = result.nodeIds;
    v.currentRouteIndex = 0;
    v.state = model::VehicleState::Waiting;
    v.timeSinceLastReroute = 0.0;
    
    const auto* originNode = network.getNodeById(originNodeId);
    if (originNode) {
        v.x = originNode->x;
        v.y = originNode->y;
    }
    
    v.currentNodeId = originNodeId;
    if (result.nodeIds.size() > 1) {
        v.nextNodeId = result.nodeIds[1];
    } else {
        v.nextNodeId = originNodeId;
        v.state = model::VehicleState::Arrived;
    }

    vehicles.push_back(v);
    return v.id;
}

const std::vector<model::Vehicle>& SimulationEngine::getVehicles() const {
    return vehicles;
}

void SimulationEngine::reset() {
    vehicles.clear();
    simulationTimeSeconds = 0.0;
    nextVehicleId = 1;
    totalReroutes = 0;
    for (auto& r : network.getRoadsMutable()) {
        r.currentVehicleCount = 0;
        r.dynamicCongestionFactor = 1.0;
    }
}

void SimulationEngine::update(double deltaTimeSeconds) {
    if (deltaTimeSeconds <= 0.0) return;
    
    // 1. inspect current vehicle positions (clear counts, then count vehicles per directed road)
    for (auto& r : network.getRoadsMutable()) {
        r.currentVehicleCount = 0;
    }
    
    for (const auto& v : vehicles) {
        if (v.state == model::VehicleState::Moving) {
            auto* road = network.getRoadMutable(v.currentNodeId, v.nextNodeId);
            if (road) {
                road->currentVehicleCount++;
            }
        }
    }
    
    // 3. update dynamic congestion factors
    for (auto& r : network.getRoadsMutable()) {
        if (r.currentVehicleCount == 0) r.dynamicCongestionFactor = 1.0;
        else if (r.currentVehicleCount <= 2) r.dynamicCongestionFactor = 1.0;
        else if (r.currentVehicleCount <= 5) r.dynamicCongestionFactor = 1.25;
        else if (r.currentVehicleCount <= 10) r.dynamicCongestionFactor = 1.5;
        else r.dynamicCongestionFactor = 2.0;
    }

    // 4. evaluate rerouting if needed
    for (auto& v : vehicles) {
        if (v.state == model::VehicleState::Moving) {
            v.timeSinceLastReroute += deltaTimeSeconds;
            if (v.timeSinceLastReroute >= 5.0) {
                if (v.nextNodeId != v.destinationNodeId) {
                    double timeOnCurrentEdge = 0.0;
                    auto* currentRoad = network.getRoadMutable(v.currentNodeId, v.nextNodeId);
                    if (currentRoad) {
                        double effectiveSpeedKph = currentRoad->speedLimitKph / currentRoad->getEffectiveTrafficFactor();
                        double effectiveSpeedMps = effectiveSpeedKph * (1000.0 / 3600.0);
                        if (effectiveSpeedMps > 0) {
                            timeOnCurrentEdge = (currentRoad->distanceMeters - v.distanceAlongCurrentRoadMeters) / effectiveSpeedMps;
                        } else {
                            timeOnCurrentEdge = 999999.0;
                        }
                    }
                    
                    double currentRemainingTime = timeOnCurrentEdge;
                    for (std::size_t i = v.currentRouteIndex + 1; i + 1 < v.routeNodeIds.size(); ++i) {
                        auto* r = network.getRoadMutable(v.routeNodeIds[i], v.routeNodeIds[i+1]);
                        if (r) {
                            currentRemainingTime += r->getTravelTimeSeconds();
                        } else {
                            currentRemainingTime += 999999.0;
                        }
                    }
                    
                    auto result = routing::Router::findRoute(
                        network, v.nextNodeId, v.destinationNodeId, 
                        routing::RoutingAlgorithm::AStar, 
                        routing::RoutingObjective::Fastest
                    );
                    
                    if (result.found) {
                        double newRemainingTime = timeOnCurrentEdge + result.estimatedTravelTimeSeconds;
                        if (newRemainingTime < currentRemainingTime * 0.8) {
                            std::vector<int> newRoute;
                            for (std::size_t i = 0; i <= v.currentRouteIndex; ++i) {
                                newRoute.push_back(v.routeNodeIds[i]);
                            }
                            for (std::size_t i = 1; i < result.nodeIds.size(); ++i) {
                                newRoute.push_back(result.nodeIds[i]);
                            }
                            v.routeNodeIds = newRoute;
                            totalReroutes++;
                        }
                    }
                }
                v.timeSinceLastReroute = 0.0; // Reset cooldown
            }
        }
    }

    // 5. move vehicles and 6. advance simulation time
    simulationTimeSeconds += deltaTimeSeconds;
    
    for (auto& v : vehicles) {
        if (v.state == model::VehicleState::Arrived) continue;
        
        if (v.state == model::VehicleState::Waiting) {
            v.state = model::VehicleState::Moving;
        }

        double remainingTime = deltaTimeSeconds;
        
        while (remainingTime > 0.0 && v.state == model::VehicleState::Moving) {
            if (v.currentRouteIndex >= v.routeNodeIds.size() - 1) {
                v.state = model::VehicleState::Arrived;
                break;
            }
            
            v.currentNodeId = v.routeNodeIds[v.currentRouteIndex];
            v.nextNodeId = v.routeNodeIds[v.currentRouteIndex + 1];
            
            const model::Road* currentRoad = network.getRoadMutable(v.currentNodeId, v.nextNodeId);
            
            if (!currentRoad) {
                v.state = model::VehicleState::Arrived;
                break;
            }
            
            double effectiveSpeedKph = currentRoad->speedLimitKph / currentRoad->getEffectiveTrafficFactor();
            double effectiveSpeedMps = effectiveSpeedKph * (1000.0 / 3600.0);
            
            if (effectiveSpeedMps <= 0.0) {
                break; // Stuck
            }
            
            double roadLength = currentRoad->distanceMeters;
            double distanceLeftOnRoad = roadLength - v.distanceAlongCurrentRoadMeters;
            double timeToFinishRoad = distanceLeftOnRoad / effectiveSpeedMps;
            
            if (remainingTime >= timeToFinishRoad) {
                remainingTime -= timeToFinishRoad;
                v.currentRouteIndex++;
                v.distanceAlongCurrentRoadMeters = 0.0;
                
                if (v.currentRouteIndex >= v.routeNodeIds.size() - 1) {
                    v.state = model::VehicleState::Arrived;
                    v.currentNodeId = v.routeNodeIds.back();
                    v.nextNodeId = v.currentNodeId;
                    
                    const auto* destNode = network.getNodeById(v.currentNodeId);
                    if (destNode) {
                        v.x = destNode->x;
                        v.y = destNode->y;
                    }
                    v.progressOnCurrentRoad = 1.0;
                    break;
                }
            } else {
                v.distanceAlongCurrentRoadMeters += remainingTime * effectiveSpeedMps;
                remainingTime = 0.0;
                
                v.progressOnCurrentRoad = v.distanceAlongCurrentRoadMeters / roadLength;
                
                const auto* n1 = network.getNodeById(v.currentNodeId);
                const auto* n2 = network.getNodeById(v.nextNodeId);
                
                if (n1 && n2) {
                    v.x = n1->x + v.progressOnCurrentRoad * (n2->x - n1->x);
                    v.y = n1->y + v.progressOnCurrentRoad * (n2->y - n1->y);
                }
            }
        }
    }
}

int SimulationEngine::spawnVehiclesBatch(int count) {
    int spawned = 0;
    std::size_t numNodes = network.getNodes().size();
    if (numNodes == 0) return 0;

    for (int i = 0; i < count; ++i) {
        int baseIndex = vehicles.size() + i;
        int origin = (baseIndex * 7) % numNodes;
        int dest = (baseIndex * 13 + 3) % numNodes;
        
        if (origin == dest) {
            dest = (dest + 1) % numNodes;
        }
        
        int result = spawnVehicle(origin, dest);
        if (result != -1) {
            spawned++;
        }
    }
    return spawned;
}

int SimulationEngine::getWaitingVehicleCount() const {
    int count = 0;
    for (const auto& v : vehicles) {
        if (v.state == model::VehicleState::Waiting) count++;
    }
    return count;
}

int SimulationEngine::getMovingVehicleCount() const {
    int count = 0;
    for (const auto& v : vehicles) {
        if (v.state == model::VehicleState::Moving) count++;
    }
    return count;
}

int SimulationEngine::getArrivedVehicleCount() const {
    int count = 0;
    for (const auto& v : vehicles) {
        if (v.state == model::VehicleState::Arrived) count++;
    }
    return count;
}

int SimulationEngine::getCongestedRoadCount() const {
    int count = 0;
    for (const auto& r : network.getRoads()) {
        if (r.dynamicCongestionFactor > 1.0) count++;
    }
    return count;
}

double SimulationEngine::getAverageCongestionFactor() const {
    double total = 0.0;
    const auto& roads = network.getRoads();
    if (roads.empty()) return 1.0;
    for (const auto& r : roads) {
        total += r.dynamicCongestionFactor;
    }
    return total / roads.size();
}

int SimulationEngine::getTotalReroutes() const {
    return totalReroutes;
}

double SimulationEngine::getSimulationTimeSeconds() const {
    return simulationTimeSeconds;
}

} // namespace simulation
