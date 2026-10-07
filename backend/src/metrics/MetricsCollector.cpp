#include "metrics/MetricsCollector.hpp"

namespace metrics {

nlohmann::json MetricsSnapshot::toJson() const {
    nlohmann::json j;
    j["cityMode"] = cityMode;
    
    j["routing"] = {
        {"lastAlgorithm", routing.lastAlgorithm},
        {"lastRouteDistanceMeters", routing.lastRouteDistanceMeters},
        {"lastRouteTimeSeconds", routing.lastRouteTimeSeconds},
        {"nodesExplored", routing.nodesExplored},
        {"runtimeMicroseconds", routing.runtimeMicroseconds},
        {"dijkstraRuntime", routing.dijkstraRuntime},
        {"dijkstraNodes", routing.dijkstraNodes},
        {"dijkstraDistance", routing.dijkstraDistance},
        {"astarRuntime", routing.astarRuntime},
        {"astarNodes", routing.astarNodes},
        {"astarDistance", routing.astarDistance}
    };
    
    j["simulation"] = {
        {"simulationTimeSeconds", simulation.simulationTimeSeconds},
        {"vehicleCount", simulation.vehicleCount},
        {"waiting", simulation.waiting},
        {"moving", simulation.moving},
        {"arrived", simulation.arrived},
        {"averageTripTimeSeconds", simulation.averageTripTimeSeconds},
        {"lastSimulationUpdateMicroseconds", simulation.lastSimulationUpdateMicroseconds},
        {"averageSimulationUpdateMicroseconds", simulation.averageSimulationUpdateMicroseconds}
    };
    
    j["traffic"] = {
        {"congestedRoadCount", traffic.congestedRoadCount},
        {"averageCongestionFactor", traffic.averageCongestionFactor},
        {"totalReroutes", traffic.totalReroutes}
    };
    
    return j;
}

MetricsCollector& MetricsCollector::getInstance() {
    static MetricsCollector instance;
    return instance;
}

void MetricsCollector::recordRouteResult(const std::string& algo, const routing::RouteResult& result) {
    std::lock_guard<std::mutex> lock(mtx);
    if (!result.found) return;

    currentSnapshot.routing.lastAlgorithm = algo;
    currentSnapshot.routing.lastRouteDistanceMeters = result.totalCost;
    currentSnapshot.routing.lastRouteTimeSeconds = result.estimatedTravelTimeSeconds;
    currentSnapshot.routing.nodesExplored = result.nodesExplored;
    currentSnapshot.routing.runtimeMicroseconds = result.runtimeMicroseconds;
    
    if (algo == "dijkstra" || algo == "Dijkstra") {
        currentSnapshot.routing.dijkstraRuntime = result.runtimeMicroseconds;
        currentSnapshot.routing.dijkstraNodes = result.nodesExplored;
        currentSnapshot.routing.dijkstraDistance = result.totalCost;
    } else if (algo == "astar" || algo == "AStar") {
        currentSnapshot.routing.astarRuntime = result.runtimeMicroseconds;
        currentSnapshot.routing.astarNodes = result.nodesExplored;
        currentSnapshot.routing.astarDistance = result.totalCost;
    }
}

void MetricsCollector::recordSimulationUpdate(double simTime, int totalVehicles, int waiting, int moving, int arrived, double avgTripTime, double updateTimeMicro) {
    std::lock_guard<std::mutex> lock(mtx);
    currentSnapshot.simulation.simulationTimeSeconds = simTime;
    currentSnapshot.simulation.vehicleCount = totalVehicles;
    currentSnapshot.simulation.waiting = waiting;
    currentSnapshot.simulation.moving = moving;
    currentSnapshot.simulation.arrived = arrived;
    currentSnapshot.simulation.averageTripTimeSeconds = avgTripTime;
    currentSnapshot.simulation.lastSimulationUpdateMicroseconds = updateTimeMicro;
    
    totalSimulationUpdateMicroseconds += updateTimeMicro;
    simulationUpdateCount++;
    currentSnapshot.simulation.averageSimulationUpdateMicroseconds = totalSimulationUpdateMicroseconds / simulationUpdateCount;
    
    // Add to history
    history.push_back(currentSnapshot);
    if (history.size() > maxHistorySize) {
        history.erase(history.begin());
    }
}

void MetricsCollector::recordTrafficUpdate(int congestedRoads, double avgCongestion, int totalReroutes) {
    std::lock_guard<std::mutex> lock(mtx);
    currentSnapshot.traffic.congestedRoadCount = congestedRoads;
    currentSnapshot.traffic.averageCongestionFactor = avgCongestion;
    currentSnapshot.traffic.totalReroutes = totalReroutes;
}

void MetricsCollector::setCityMode(const std::string& mode) {
    std::lock_guard<std::mutex> lock(mtx);
    currentSnapshot.cityMode = mode;
}

void MetricsCollector::resetSimulation() {
    std::lock_guard<std::mutex> lock(mtx);
    currentSnapshot.simulation = SimulationMetrics{};
    currentSnapshot.traffic = TrafficMetrics{};
    totalSimulationUpdateMicroseconds = 0.0;
    simulationUpdateCount = 0;
    history.clear();
}

MetricsSnapshot MetricsCollector::getSnapshot() const {
    std::lock_guard<std::mutex> lock(mtx);
    return currentSnapshot;
}

std::vector<MetricsSnapshot> MetricsCollector::getHistory() const {
    std::lock_guard<std::mutex> lock(mtx);
    return history;
}

} // namespace metrics

