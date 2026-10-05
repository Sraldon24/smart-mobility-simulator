#pragma once

#include "model/RoadNetwork.hpp"
#include "simulation/SimulationEngine.hpp"

namespace api {

class HttpServer {
public:
    HttpServer(model::RoadNetwork& network, simulation::SimulationEngine& engine);
    void listen(const char* host, int port);

private:
    model::RoadNetwork& network;
    simulation::SimulationEngine& engine;
};

} // namespace api
