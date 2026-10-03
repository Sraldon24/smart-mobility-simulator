#pragma once

#include "model/RoadNetwork.hpp"

namespace api {

class HttpServer {
public:
    HttpServer(const model::RoadNetwork& network);
    void listen(const char* host, int port);

private:
    const model::RoadNetwork& network;
};

} // namespace api
