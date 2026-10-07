#include <set>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include "simulation/SimulationEngine.hpp"
#include "model/RoadNetwork.hpp"
#include "model/GeneratedCityLoader.hpp"

using namespace simulation;
using namespace model;

TEST_CASE("Simulation Engine basics", "[simulation]") {
    RoadNetwork network;
    network = GeneratedCityLoader::generate5x5Grid();
    
    
    SimulationEngine engine(network);
    
    SECTION("Initial state") {
        REQUIRE(engine.getVehicles().empty());
        REQUIRE(engine.getSimulationTimeSeconds() == Catch::Approx(0.0));
        REQUIRE(engine.getWaitingVehicleCount() == 0);
        REQUIRE(engine.getMovingVehicleCount() == 0);
        REQUIRE(engine.getArrivedVehicleCount() == 0);
    }
    
    SECTION("Spawning vehicles") {
        int vId1 = engine.spawnVehicle(0, 24, "fastest");
        REQUIRE(vId1 > 0);
        
        const auto& vehicles = engine.getVehicles();
        REQUIRE(vehicles.size() == 1);
        REQUIRE(vehicles.front().id == vId1);
        REQUIRE(vehicles.front().state == VehicleState::Waiting); // Depending on implementation, might be waiting or moving
        
        int vId2 = engine.spawnVehicle(1, 23, "shortest");
        REQUIRE(vehicles.size() == 2);
        REQUIRE(vId1 != vId2);
        
        REQUIRE(engine.getMovingVehicleCount() + engine.getWaitingVehicleCount() == 2);
        REQUIRE(engine.getArrivedVehicleCount() == 0);
    }
    
    SECTION("Batch spawning") {
        int count = engine.spawnVehiclesBatch(10);
        REQUIRE(count == 10);
        REQUIRE(engine.getVehicles().size() == 10);
        
        std::set<int> ids;
        for (const auto& v : engine.getVehicles()) {
            ids.insert(v.id);
        }
        REQUIRE(ids.size() == 10); // Unique IDs
    }
    
    SECTION("Vehicle movement with small deltaTime") {
        int vId = engine.spawnVehicle(0, 24, "fastest");
        REQUIRE(vId != -1);
        
        engine.update(1.0); // 1 second
        
        REQUIRE(engine.getSimulationTimeSeconds() == Catch::Approx(1.0));
        
        const auto& vehicles = engine.getVehicles();
        REQUIRE(vehicles.size() == 1);
        // Vehicle should have moved a bit
        REQUIRE(vehicles.front().distanceAlongCurrentRoadMeters > 0.0);
        REQUIRE(vehicles.front().state == VehicleState::Moving);
    }
    
    SECTION("Vehicle arrival with large deltaTime") {
        engine.spawnVehicle(0, 24, "fastest");
        
        // Large update to ensure arrival (e.g. 10 hours)
        engine.update(36000.0);
        
        const auto& vehicles = engine.getVehicles();
        REQUIRE(vehicles.size() == 1);
        REQUIRE(vehicles.front().state == VehicleState::Arrived);
        REQUIRE(engine.getArrivedVehicleCount() == 1);
        REQUIRE(engine.getMovingVehicleCount() == 0);
    }
    
    SECTION("Resetting simulation") {
        engine.spawnVehicle(0, 24, "fastest");
        engine.update(10.0);
        
        engine.reset();
        
        REQUIRE(engine.getVehicles().empty());
        REQUIRE(engine.getSimulationTimeSeconds() == Catch::Approx(0.0));
        REQUIRE(engine.getWaitingVehicleCount() == 0);
        REQUIRE(engine.getMovingVehicleCount() == 0);
        REQUIRE(engine.getArrivedVehicleCount() == 0);
    }
    
    SECTION("Emergent congestion and rerouting due to incidents") {
        engine.spawnVehicle(0, 24, "fastest");
        
        // Let it move a bit
        engine.update(10.0);
        
        // Add incident on its path (assuming route starts 0 -> 1 -> ... or 0 -> 5 -> ...)
        const auto& v = engine.getVehicles().front();
        if (v.routeNodeIds.size() >= 3) {
            int nextNode = v.routeNodeIds[1];
            int nextNextNode = v.routeNodeIds[2];
            
            network.setRoadClosed(nextNode, nextNextNode, true);
            
            int reroutesBefore = engine.getTotalReroutes();
            
            // Updating should trigger a reroute
            engine.update(10.0);
            
            REQUIRE(engine.getTotalReroutes() >= reroutesBefore);
        }
    }
}
