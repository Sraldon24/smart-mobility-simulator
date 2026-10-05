#pragma once

#include "model/RoadNetwork.hpp"

namespace api {

class HttpServer {
public:
    HttpServer(model::RoadNetwork& network);
    void listen(const char* host, int port);

private:
    model::RoadNetwork& network;
};

} // namespace api
