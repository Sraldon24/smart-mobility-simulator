#pragma once

#include "model/RoadNetwork.hpp"
#include "simulation/SimulationEngine.hpp"

#include <string>

namespace api {

class HttpServer {
public:
    HttpServer(model::RoadNetwork& network, simulation::SimulationEngine& engine, const std::string& pbfPath);
    void listen(const char* host, int port);

private:
    model::RoadNetwork& network;
    simulation::SimulationEngine& engine;
    std::string pbfPath;
};

} // namespace api
