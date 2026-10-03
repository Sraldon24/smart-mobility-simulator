#include "api/HttpServer.hpp"
#include "routing/Router.hpp"
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;

namespace api {

HttpServer::HttpServer(const model::RoadNetwork& network) : network(network) {}

void HttpServer::listen(const char* host, int port) {
    httplib::Server svr;

    // Helper to add CORS headers
    auto set_cors_headers = [](httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
    };

    svr.Options(R"(.*)", [&](const httplib::Request&, httplib::Response& res) {
        set_cors_headers(res);
        res.status = 200;
    });

    svr.Get("/health", [&](const httplib::Request&, httplib::Response& res) {
        set_cors_headers(res);
        json j = {{"status", "ok"}};
        res.set_content(j.dump(), "application/json");
    });

    svr.Get("/city", [&](const httplib::Request&, httplib::Response& res) {
        set_cors_headers(res);
        json j;
        j["nodes"] = json::array();
        for (const auto& node : network.getNodes()) {
            j["nodes"].push_back({
                {"id", node.id},
                {"x", node.x},
                {"y", node.y}
            });
        }
        
        j["roads"] = json::array();
        for (const auto& road : network.getRoads()) {
            j["roads"].push_back({
                {"from", road.from},
                {"to", road.to},
                {"distanceMeters", road.distanceMeters},
                {"speedLimitKph", road.speedLimitKph},
                {"trafficFactor", road.trafficFactor},
                {"closed", road.closed}
            });
        }
        res.set_content(j.dump(), "application/json");
    });

    svr.Get("/route", [&](const httplib::Request& req, httplib::Response& res) {
        set_cors_headers(res);
        
        if (!req.has_param("start") || !req.has_param("end")) {
            res.status = 400;
            json err = {{"error", "Missing 'start' or 'end' parameter"}};
            res.set_content(err.dump(), "application/json");
            return;
        }

        try {
            int start = std::stoi(req.get_param_value("start"));
            int end = std::stoi(req.get_param_value("end"));

            std::string algoStr = "dijkstra";
            if (req.has_param("algorithm")) {
                algoStr = req.get_param_value("algorithm");
            }

            routing::RoutingAlgorithm algo;
            if (algoStr == "dijkstra") {
                algo = routing::RoutingAlgorithm::Dijkstra;
            } else if (algoStr == "astar") {
                algo = routing::RoutingAlgorithm::AStar;
            } else {
                res.status = 400;
                json err = {{"error", "invalid routing algorithm"}};
                res.set_content(err.dump(), "application/json");
                return;
            }

            auto result = routing::Router::findRoute(network, start, end, algo);
            
            json j;
            j["found"] = result.found;
            j["algorithm"] = algoStr;
            j["start"] = start;
            j["end"] = end;
            j["nodesExplored"] = result.nodesExplored;
            j["runtimeMicroseconds"] = result.runtimeMicroseconds;
            
            if (result.found) {
                j["nodeIds"] = result.nodeIds;
                j["totalDistanceMeters"] = result.totalCost;
            } else {
                j["error"] = "Route not found or invalid node ID";
                res.status = 404;
            }

            res.set_content(j.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            json err = {{"error", "Invalid parameter format. Start and end must be integers."}};
            res.set_content(err.dump(), "application/json");
        }
    });

    std::cout << "Starting backend API server on http://" << host << ":" << port << std::endl;
    svr.listen(host, port);
}

} // namespace api
