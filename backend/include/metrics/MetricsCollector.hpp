#pragma once

#include "routing/Router.hpp"
#include <string>
#include <vector>
#include <mutex>
#include <nlohmann/json.hpp>

namespace metrics {

struct RoutingMetrics {
    std::string lastAlgorithm{"none"};
    double lastRouteDistanceMeters{0.0};
    double lastRouteTimeSeconds{0.0};
    std::size_t nodesExplored{0};
    long long runtimeMicroseconds{0};
    
    long long dijkstraRuntime{0};
    std::size_t dijkstraNodes{0};
    double dijkstraDistance{0.0};
    
    long long astarRuntime{0};
    std::size_t astarNodes{0};
    double astarDistance{0.0};
};

struct SimulationMetrics {
    double simulationTimeSeconds{0.0};
    int vehicleCount{0};
    int waiting{0};
    int moving{0};
    int arrived{0};
    double averageTripTimeSeconds{0.0};
    double lastSimulationUpdateMicroseconds{0.0};
    double averageSimulationUpdateMicroseconds{0.0};
};

struct TrafficMetrics {
    int congestedRoadCount{0};
    double averageCongestionFactor{1.0};
    int totalReroutes{0};
};

struct MetricsSnapshot {
    std::string cityMode{"generated"};
    RoutingMetrics routing;
    SimulationMetrics simulation;
    TrafficMetrics traffic;

    nlohmann::json toJson() const;
};

class MetricsCollector {
public:
    static MetricsCollector& getInstance();

    void recordRouteResult(const std::string& algo, const routing::RouteResult& result);
    
    void recordSimulationUpdate(double simTime, int totalVehicles, int waiting, int moving, int arrived, double avgTripTime, double updateTimeMicro);
    void recordTrafficUpdate(int congestedRoads, double avgCongestion, int totalReroutes);
    
    void setCityMode(const std::string& mode);
    
    void resetSimulation();
    
    MetricsSnapshot getSnapshot() const;
    std::vector<MetricsSnapshot> getHistory() const;

private:
    MetricsCollector() = default;
    
    mutable std::mutex mtx;
    MetricsSnapshot currentSnapshot;
    std::vector<MetricsSnapshot> history;
    const size_t maxHistorySize = 100;
    
    double totalSimulationUpdateMicroseconds{0.0};
    long long simulationUpdateCount{0};
};

} // namespace metrics

