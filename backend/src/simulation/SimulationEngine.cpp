#include "simulation/SimulationEngine.hpp"
#include "routing/Router.hpp"
#include <stdexcept>

#include "recommendation/RouteCandidateGenerator.hpp"
#include "recommendation/RecommendationEngine.hpp"
#include "metrics/MetricsCollector.hpp"
#include <chrono>
#include "utils/Timer.hpp"
#include "utils/Logger.hpp"

namespace simulation {

SimulationEngine::SimulationEngine(model::RoadNetwork& network)
    : network(network) {}

int SimulationEngine::spawnVehicle(int originNodeId, int destinationNodeId, const std::string& preferenceStr) {
    if (!network.getNodeById(originNodeId) || !network.getNodeById(destinationNodeId)) {
        return -1; // Invalid nodes
    }

    auto candidates = recommendation::RouteCandidateGenerator::generateCandidates(network, originNodeId, destinationNodeId);
    if (candidates.empty()) {
        return -1; // No route available
    }

    auto prefEnum = recommendation::stringToPreference(preferenceStr);
    auto profile = recommendation::makeProfile(prefEnum);
    auto recommendationRes = recommendation::RecommendationEngine::recommend(candidates, profile);

    model::Vehicle v;
    v.id = nextVehicleId++;
    v.preference = prefEnum;
    v.originNodeId = originNodeId;
    v.destinationNodeId = destinationNodeId;
    v.routeNodeIds = recommendationRes.recommendedRoute.candidate.nodeIds;
    v.currentRouteIndex = 0;
    v.state = model::VehicleState::Waiting;
    v.timeSinceLastReroute = 0.0;
    v.activeTimeSeconds = 0.0;
    v.totalTripTimeSeconds = 0.0;
    
    const auto* originNode = network.getNodeById(originNodeId);
    if (originNode) {
        v.x = originNode->x;
        v.y = originNode->y;
    }
    
    v.currentNodeId = originNodeId;
    if (v.routeNodeIds.size() > 1) {
        v.nextNodeId = v.routeNodeIds[1];
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
    for (auto* r : activeRoads) {
        r->currentVehicleCount = 0;
        r->dynamicCongestionFactor = 1.0;
    }
    activeRoads.clear();
    metrics::MetricsCollector::getInstance().resetSimulation();
}

/*
 * Simulation Update Loop:
 * 
 * Order is critical to determinism and emergent behavior:
 * 1. Compute occupancy (where are vehicles right now?)
 * 2. Update dynamic congestion (feedback loop from occupancy)
 * 3. Evaluate rerouting (vehicles react to new congestion)
 * 4. Move vehicles (advance them according to deltaTime and current speed limits)
 * 
 * Multi-Agent Determinism:
 * Vehicles maintain independent state but are updated in this single-threaded loop.
 * This guarantees reproducible behavior for tests and avoids complex race conditions.
 */
void SimulationEngine::update(double deltaTimeSeconds) {
    if (deltaTimeSeconds <= 0.0) return;
    utils::ScopedTimer timer("SimulationEngine", "update");
    
    auto startTimer = std::chrono::high_resolution_clock::now();
    
    // Performance Optimization:
    // Previously, clearing and updating counts scanned ALL roads O(R). 
    // On the Montreal graph (1.4M edges), this took ~1.5ms even with 0 vehicles.
    // By tracking `activeRoads`, we reduce the complexity to O(V).
    
    // 1. clear previous active roads vehicle counts and congestion factor
    for (auto* r : activeRoads) {
        r->currentVehicleCount = 0;
        r->dynamicCongestionFactor = 1.0;
    }
    activeRoads.clear();
    
    // 2. update active roads and vehicle counts based on current occupancy
    for (const auto& v : vehicles) {
        if (v.state == model::VehicleState::Moving) {
            auto* road = network.getRoadMutable(v.currentNodeId, v.nextNodeId);
            if (road) {
                if (road->currentVehicleCount == 0) {
                    activeRoads.push_back(road);
                }
                road->currentVehicleCount++;
            }
        }
    }
    
    // 3. update dynamic congestion factors ONLY for newly active roads.
    // Emergent Congestion: More vehicles -> higher factor -> slower travel times.
    for (auto* r : activeRoads) {
        if (r->currentVehicleCount <= 2) r->dynamicCongestionFactor = 1.0;
        else if (r->currentVehicleCount <= 5) r->dynamicCongestionFactor = 1.25;
        else if (r->currentVehicleCount <= 10) r->dynamicCongestionFactor = 1.5;
        else r->dynamicCongestionFactor = 2.0;
    }

    // 4. evaluate rerouting if needed
    // Vehicles don't recompute routes every frame (too expensive, causes oscillation).
    // They have a cooldown, and only reroute if the new path is significantly better (e.g. 20% faster).
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
                    
                    auto candidates = recommendation::RouteCandidateGenerator::generateCandidates(network, v.nextNodeId, v.destinationNodeId);
                    if (!candidates.empty()) {
                        auto profile = recommendation::makeProfile(v.preference);
                        auto recommendationRes = recommendation::RecommendationEngine::recommend(candidates, profile);
                        
                        double newRemainingTime = timeOnCurrentEdge + recommendationRes.recommendedRoute.candidate.travelTimeSeconds;
                        if (newRemainingTime < currentRemainingTime * 0.8) {
                            std::vector<int> newRoute;
                            for (std::size_t i = 0; i <= v.currentRouteIndex; ++i) {
                                newRoute.push_back(v.routeNodeIds[i]);
                            }
                            for (std::size_t i = 1; i < recommendationRes.recommendedRoute.candidate.nodeIds.size(); ++i) {
                                newRoute.push_back(recommendationRes.recommendedRoute.candidate.nodeIds[i]);
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
        
        v.activeTimeSeconds += deltaTimeSeconds;
        
        if (v.state == model::VehicleState::Waiting) {
            v.state = model::VehicleState::Moving;
        }

        // We use deltaTime to calculate physical movement: distance = speed * time.
        // It's critical not to discard leftover time when a vehicle reaches 
        // the end of a segment. It should seamlessly continue onto the next segment.
        double remainingTime = deltaTimeSeconds;
        
        while (remainingTime > 0.0 && v.state == model::VehicleState::Moving) {
            if (v.currentRouteIndex >= v.routeNodeIds.size() - 1) {
                v.state = model::VehicleState::Arrived;
                v.totalTripTimeSeconds = v.activeTimeSeconds;
                break;
            }
            
            v.currentNodeId = v.routeNodeIds[v.currentRouteIndex];
            v.nextNodeId = v.routeNodeIds[v.currentRouteIndex + 1];
            
            const model::Road* currentRoad = network.getRoadMutable(v.currentNodeId, v.nextNodeId);
            
            if (!currentRoad) {
                v.state = model::VehicleState::Arrived;
                v.totalTripTimeSeconds = v.activeTimeSeconds;
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
                // The vehicle finishes this road segment within the current tick.
                // Subtract the time spent, and loop again to process the next segment.
                remainingTime -= timeToFinishRoad;
                v.currentRouteIndex++;
                v.distanceAlongCurrentRoadMeters = 0.0;
                
                if (v.currentRouteIndex >= v.routeNodeIds.size() - 1) {
                    v.state = model::VehicleState::Arrived;
                    v.totalTripTimeSeconds = v.activeTimeSeconds;
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
                // The vehicle moves partially along this segment.
                v.distanceAlongCurrentRoadMeters += remainingTime * effectiveSpeedMps;
                remainingTime = 0.0; // Tick complete for this vehicle
                
                v.progressOnCurrentRoad = v.distanceAlongCurrentRoadMeters / roadLength;
                
                // Interpolate visual coordinates. 
                // Note: Simulation logic runs entirely on graph edge distances;
                // this x/y interpolation is only for the frontend UI.
                const auto* n1 = network.getNodeById(v.currentNodeId);
                const auto* n2 = network.getNodeById(v.nextNodeId);
                
                if (n1 && n2) {
                    v.x = n1->x + v.progressOnCurrentRoad * (n2->x - n1->x);
                    v.y = n1->y + v.progressOnCurrentRoad * (n2->y - n1->y);
                }
            }
        }
    }
    
    auto endTimer = std::chrono::high_resolution_clock::now();
    double updateMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(endTimer - startTimer).count();

    int total = vehicles.size();
    int waiting = getWaitingVehicleCount();
    int moving = getMovingVehicleCount();
    int arrived = getArrivedVehicleCount();

    double totalTripTime = 0.0;
    int arrivedWithTime = 0;
    for (const auto& v : vehicles) {
        if (v.state == model::VehicleState::Arrived && v.activeTimeSeconds > 0) {
            totalTripTime += v.activeTimeSeconds;
            arrivedWithTime++;
        }
    }
    double avgTripTime = arrivedWithTime > 0 ? totalTripTime / arrivedWithTime : 0.0;

    metrics::MetricsCollector::getInstance().recordSimulationUpdate(
        simulationTimeSeconds, total, waiting, moving, arrived, avgTripTime, updateMicroseconds);

    metrics::MetricsCollector::getInstance().recordTrafficUpdate(
        getCongestedRoadCount(), getAverageCongestionFactor(), getTotalReroutes());
}

int SimulationEngine::spawnVehiclesBatch(int count) {
    int spawned = 0;
    std::size_t numNodes = network.getNodes().size();
    if (numNodes == 0) return 0;

    const std::vector<std::string> prefs = {
        "fastest", "shortest", "least_traffic", "balanced", "cheapest"
    };

    for (int i = 0; i < count; ++i) {
        int baseIndex = vehicles.size() + i;
        int origin = (baseIndex * 7) % numNodes;
        int dest = (baseIndex * 13 + 3) % numNodes;
        
        if (origin == dest) {
            dest = (dest + 1) % numNodes;
        }
        
        std::string pref = prefs[baseIndex % prefs.size()];
        
        int result = spawnVehicle(origin, dest, pref);
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
    for (const auto* r : activeRoads) {
        if (r->dynamicCongestionFactor > 1.0) count++;
    }
    return count;
}

double SimulationEngine::getAverageCongestionFactor() const {
    const auto& roads = network.getRoads();
    if (roads.empty()) return 1.0;
    
    double total = roads.size(); // baseline where all factors are 1.0
    for (const auto* r : activeRoads) {
        total += (r->dynamicCongestionFactor - 1.0);
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
