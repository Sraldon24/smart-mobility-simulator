#include "api/HttpServer.hpp"
#include "routing/Router.hpp"
#include "recommendation/UserProfile.hpp"
#include "recommendation/RecommendationEngine.hpp"
#include "recommendation/RouteCandidateGenerator.hpp"
#include "model/GeneratedCityLoader.hpp"
#include "model/MontrealOSMLoader.hpp"
#include "metrics/MetricsCollector.hpp"
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <mutex>

using json = nlohmann::json;

namespace api {

// EDUCATIONAL NOTE: Thin API Layer Architecture
// WHY: We want the core C++ backend to be completely decoupled from HTTP concerns.
// HOW: HttpServer acts as a simple translation layer. It parses JSON, calls native C++
// functions on the injected model::RoadNetwork or simulation::SimulationEngine, and
// serializes the C++ structs back into JSON. There is NO business logic in this file.
HttpServer::HttpServer(model::RoadNetwork& network, simulation::SimulationEngine& engine, const std::string& pbfPath) : network(network), engine(engine), pbfPath(pbfPath) {}

void HttpServer::listen(const char* host, int port) {
    httplib::Server svr;
    // Requests share one mutable city and simulation; serialize access to both.
    std::mutex stateMutex;

    auto set_cors_headers = [](httplib::Response& res) {
        const char* origin_env = std::getenv("FRONTEND_ORIGIN");
        std::string origin = origin_env ? std::string(origin_env) : "http://localhost:5173";
        res.set_header("Access-Control-Allow-Origin", origin);
        res.set_header("Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
    };

    svr.Options(R"(.*)", [&](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(stateMutex);
        set_cors_headers(res);
        res.status = 200;
    });

    svr.Get("/health", [&](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(stateMutex);
        set_cors_headers(res);
        json j = {{"status", "ok"}};
        res.set_content(j.dump(), "application/json");
    });

    svr.Get("/city", [&](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(stateMutex);
        set_cors_headers(res);
        json j;
        j["coordinateSystem"] = (network.getCoordinateSystem() == model::CoordinateSystem::Geographic) ? "geographic" : "cartesian";
        j["nodes"] = json::array();
        for (const auto& node : network.getNodes()) {
            json n = {
                {"id", node.id},
                {"x", node.x},
                {"y", node.y}
            };
            if (network.getCoordinateSystem() == model::CoordinateSystem::Geographic) {
                n["lon"] = node.x;
                n["lat"] = node.y;
            }
            j["nodes"].push_back(n);
        }
        
        j["roads"] = json::array();
        for (const auto& road : network.getRoads()) {
            j["roads"].push_back({
                {"from", road.from},
                {"to", road.to},
                {"distanceMeters", road.distanceMeters},
                {"speedLimitKph", road.speedLimitKph},
                {"trafficFactor", road.getEffectiveTrafficFactor()},
                {"dynamicCongestionFactor", road.dynamicCongestionFactor},
                {"currentVehicleCount", road.currentVehicleCount},
                {"closed", road.closed}
            });
        }
        res.set_content(j.dump(), "application/json");
    });

    svr.Post("/mode", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(stateMutex);
        set_cors_headers(res);
        try {
            auto body = json::parse(req.body);
            if (!body.contains("mode")) {
                res.status = 400;
                res.set_content(json{{"error", {{"code", "BAD_REQUEST"}, {"message", "Missing mode parameter"}}}}.dump(), "application/json");
                return;
            }
            std::string mode = body["mode"];
            if (mode == "montreal") {
                smart_mobility::model::MontrealOSMLoader loader;
                auto loadedNetwork = loader.load(pbfPath);
                if (loadedNetwork.getNodes().empty()) {
                    res.status = 503;
                    res.set_content(json{{"error", {{"code", "MAP_UNAVAILABLE"}, {"message", "Montreal map data is unavailable. Your current map has been preserved."}}}}.dump(), "application/json");
                    return;
                }
                engine.reset(); // Release road pointers before replacing their network.
                network = std::move(loadedNetwork);
            } else if (mode == "generated") {
                engine.reset();
                network = model::GeneratedCityLoader::generate5x5Grid();
            } else {
                res.status = 400;
                res.set_content(json{{"error", {{"code", "BAD_REQUEST"}, {"message", "Invalid mode"}}}}.dump(), "application/json");
                return;
            }
            metrics::MetricsCollector::getInstance().setCityMode(mode);
            res.status = 200;
            res.set_content(json{{"status", "success"}}.dump(), "application/json");
        } catch (...) {
            res.status = 400;
            res.set_content(json{{"error", {{"code", "BAD_REQUEST"}, {"message", "Malformed request"}}}}.dump(), "application/json");
        }
    });

    svr.Get("/route", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(stateMutex);
        set_cors_headers(res);
        
        if (!req.has_param("start") || !req.has_param("end")) {
            res.status = 400;
            json err = {{"error", {{"code", "BAD_REQUEST"}, {"message", "Missing 'start' or 'end' parameter"}}}};
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

            std::string objStr = "shortest";
            if (req.has_param("objective")) {
                objStr = req.get_param_value("objective");
            }

            routing::RoutingAlgorithm algo;
            if (algoStr == "dijkstra") {
                algo = routing::RoutingAlgorithm::Dijkstra;
            } else if (algoStr == "astar") {
                algo = routing::RoutingAlgorithm::AStar;
            } else {
                res.status = 400;
                json err = {{"error", {{"code", "BAD_REQUEST"}, {"message", "invalid routing algorithm"}}}};
                res.set_content(err.dump(), "application/json");
                return;
            }

            routing::RoutingObjective obj;
            if (objStr == "shortest") {
                obj = routing::RoutingObjective::Shortest;
            } else if (objStr == "fastest") {
                obj = routing::RoutingObjective::Fastest;
            } else if (objStr == "least_traffic") {
                obj = routing::RoutingObjective::LeastTraffic;
            } else {
                res.status = 400;
                json err = {{"error", {{"code", "BAD_REQUEST"}, {"message", "invalid routing objective"}}}};
                res.set_content(err.dump(), "application/json");
                return;
            }

            auto result = routing::Router::findRoute(network, start, end, algo, obj);
            
            if (result.found) {
                metrics::MetricsCollector::getInstance().recordRouteResult(algoStr, result);
            }
            
            json j;
            j["found"] = result.found;
            j["algorithm"] = algoStr;
            j["objective"] = objStr;
            j["start"] = start;
            j["end"] = end;
            j["nodesExplored"] = result.nodesExplored;
            j["runtimeMicroseconds"] = result.runtimeMicroseconds;
            
            if (result.found) {
                j["nodeIds"] = result.nodeIds;
                j["totalDistanceMeters"] = result.totalCost;
                j["estimatedTravelTimeSeconds"] = result.estimatedTravelTimeSeconds;
            } else {
                j["error"] = "Route not found or invalid node ID";
                res.status = 404;
            }

            res.set_content(j.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            json err = {{"error", {{"code", "BAD_REQUEST"}, {"message", "Invalid parameter format"}}}};
            res.set_content(err.dump(), "application/json");
        }
    });

    svr.Get("/metrics", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(stateMutex);
        set_cors_headers(res);
        auto snapshot = metrics::MetricsCollector::getInstance().getSnapshot();
        json j = snapshot.toJson();
        
        if (req.has_param("history") && req.get_param_value("history") == "true") {
            auto history = metrics::MetricsCollector::getInstance().getHistory();
            j["history"] = json::array();
            for (const auto& snap : history) {
                j["history"].push_back(snap.toJson());
            }
        }
        
        res.set_content(j.dump(), "application/json");
    });

    svr.Get("/profiles", [&](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(stateMutex);
        set_cors_headers(res);
        json j = {
            {"profiles", {"fastest", "shortest", "least_traffic", "cheapest", "balanced"}}
        };
        res.set_content(j.dump(), "application/json");
    });

    svr.Get(R"(/profiles/([a-zA-Z_]+))", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(stateMutex);
        set_cors_headers(res);
        std::string name = req.matches[1];
        auto pref = recommendation::stringToPreference(name);
        auto profile = recommendation::makeProfile(pref);
        
        json j = {
            {"name", name},
            {"timeWeight", profile.timeWeight},
            {"distanceWeight", profile.distanceWeight},
            {"trafficWeight", profile.trafficWeight},
            {"costWeight", profile.costWeight}
        };
        res.set_content(j.dump(), "application/json");
    });

    svr.Get("/recommend-route", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(stateMutex);
        set_cors_headers(res);
        
        if (!req.has_param("start") || !req.has_param("end")) {
            res.status = 400;
            json err = {{"error", {{"code", "BAD_REQUEST"}, {"message", "Missing 'start' or 'end' parameter"}}}};
            res.set_content(err.dump(), "application/json");
            return;
        }

        try {
            int start = std::stoi(req.get_param_value("start"));
            int end = std::stoi(req.get_param_value("end"));
            
            std::string profileStr = "balanced";
            if (req.has_param("profile")) {
                profileStr = req.get_param_value("profile");
            }

            auto candidates = recommendation::RouteCandidateGenerator::generateCandidates(network, start, end);
            
            if (candidates.empty()) {
                res.status = 404;
                res.set_content(json{{"error", {{"code", "NOT_FOUND"}, {"message", "No route found"}}}}.dump(), "application/json");
                return;
            }

            auto profile = recommendation::makeProfile(recommendation::stringToPreference(profileStr));
            auto recommendationRes = recommendation::RecommendationEngine::recommend(candidates, profile);

            auto serializeCandidate = [](const recommendation::ScoredCandidate& sc) {
                return json{
                    {"nodeIds", sc.candidate.nodeIds},
                    {"distanceMeters", sc.candidate.totalDistanceMeters},
                    {"travelTimeSeconds", sc.candidate.travelTimeSeconds},
                    {"averageCongestion", sc.candidate.averageCongestion},
                    {"estimatedCost", sc.candidate.estimatedCost},
                    {"sourceObjective", sc.candidate.sourceObjective},
                    {"score", sc.totalScore},
                    {"normalizedValues", {
                        {"time", sc.normalizedTime},
                        {"distance", sc.normalizedDistance},
                        {"traffic", sc.normalizedTraffic},
                        {"cost", sc.normalizedCost}
                    }},
                    {"scoreBreakdown", {
                        {"time", sc.timeContribution},
                        {"distance", sc.distanceContribution},
                        {"traffic", sc.trafficContribution},
                        {"cost", sc.costContribution}
                    }}
                };
            };

            json j = {
                {"profile", profileStr},
                {"recommendedRoute", serializeCandidate(recommendationRes.recommendedRoute)},
                {"explanation", recommendationRes.explanation},
                {"candidates", json::array()}
            };
            
            for (const auto& c : recommendationRes.allCandidates) {
                j["candidates"].push_back(serializeCandidate(c));
            }

            res.set_content(j.dump(), "application/json");

        } catch (...) {
            res.status = 400;
            res.set_content(json{{"error", {{"code", "BAD_REQUEST"}, {"message", "Invalid parameter format"}}}}.dump(), "application/json");
        }
    });

    svr.Get("/recommend-route/compare", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(stateMutex);
        set_cors_headers(res);
        
        if (!req.has_param("start") || !req.has_param("end")) {
            res.status = 400;
            res.set_content(json{{"error", {{"code", "BAD_REQUEST"}, {"message", "Missing 'start' or 'end' parameter"}}}}.dump(), "application/json");
            return;
        }

        try {
            int start = std::stoi(req.get_param_value("start"));
            int end = std::stoi(req.get_param_value("end"));
            
            auto candidates = recommendation::RouteCandidateGenerator::generateCandidates(network, start, end);
            
            if (candidates.empty()) {
                res.status = 404;
                res.set_content(json{{"error", {{"code", "NOT_FOUND"}, {"message", "No route found"}}}}.dump(), "application/json");
                return;
            }

            std::vector<std::string> profileNames = {"fastest", "shortest", "least_traffic", "cheapest", "balanced"};
            json j = json::array();

            for (const auto& pName : profileNames) {
                auto profile = recommendation::makeProfile(recommendation::stringToPreference(pName));
                auto recommendationRes = recommendation::RecommendationEngine::recommend(candidates, profile);
                
                j.push_back({
                    {"profile", pName},
                    {"nodeIds", recommendationRes.recommendedRoute.candidate.nodeIds},
                    {"distanceMeters", recommendationRes.recommendedRoute.candidate.totalDistanceMeters},
                    {"travelTimeSeconds", recommendationRes.recommendedRoute.candidate.travelTimeSeconds},
                    {"averageCongestion", recommendationRes.recommendedRoute.candidate.averageCongestion},
                    {"estimatedCost", recommendationRes.recommendedRoute.candidate.estimatedCost},
                    {"score", recommendationRes.recommendedRoute.totalScore}
                });
            }

            res.set_content(j.dump(), "application/json");

        } catch (...) {
            res.status = 400;
            res.set_content(json{{"error", {{"code", "BAD_REQUEST"}, {"message", "Invalid parameter format"}}}}.dump(), "application/json");
        }
    });

    svr.Get("/incidents", [&](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(stateMutex);
        set_cors_headers(res);
        json j;
        j["incidents"] = json::array();
        for (const auto& inc : network.getIncidents()) {
            std::string typeStr = (inc.type == model::IncidentType::Closure) ? "closure" : "accident";
            j["incidents"].push_back({
                {"from", inc.from},
                {"to", inc.to},
                {"type", typeStr}
            });
        }
        res.set_content(j.dump(), "application/json");
    });

    auto processReroute = [&](const json& body, int incidentFrom, int incidentTo, json& response, bool isClear) {
        response["routeAffected"] = false;
        response["rerouted"] = false;

        if (body.contains("activeRoute") && body["activeRoute"].is_object()) {
            auto activeRoute = body["activeRoute"];
            
            bool affected = false;
            
            // If it's an incident ADD, it only affects the route if the incident edge is ON the route
            if (!isClear) {
                if (activeRoute.contains("nodeIds") && activeRoute["nodeIds"].is_array()) {
                    auto nodes = activeRoute["nodeIds"].get<std::vector<int>>();
                    for (size_t i = 0; i < nodes.size() - 1; ++i) {
                        if ((nodes[i] == incidentFrom && nodes[i+1] == incidentTo) ||
                            (nodes[i] == incidentTo && nodes[i+1] == incidentFrom)) {
                            affected = true;
                            break;
                        }
                    }
                }
            } else {
                // If it's a CLEAR, we just consider it affected to trigger a recalculation check
                affected = true; 
            }

            if (affected && activeRoute.contains("start") && activeRoute.contains("end") && 
                activeRoute.contains("algorithm") && activeRoute.contains("objective")) {
                
                int start = activeRoute["start"];
                int end = activeRoute["end"];
                std::string algoStr = activeRoute["algorithm"];
                std::string objStr = activeRoute["objective"];
                
                routing::RoutingAlgorithm algo = (algoStr == "astar") ? routing::RoutingAlgorithm::AStar : routing::RoutingAlgorithm::Dijkstra;
                routing::RoutingObjective obj = (objStr == "fastest") ? routing::RoutingObjective::Fastest : routing::RoutingObjective::Shortest;

                auto newResult = routing::Router::findRoute(network, start, end, algo, obj);

                bool rerouted = false;
                auto oldNodes = activeRoute["nodeIds"].get<std::vector<int>>();
                
                if (newResult.found) {
                    if (newResult.nodeIds != oldNodes) {
                        rerouted = true;
                        response["previousRoute"] = oldNodes;
                        response["newRoute"] = newResult.nodeIds;
                        response["previousDistanceMeters"] = activeRoute["totalDistanceMeters"];
                        response["newDistanceMeters"] = newResult.totalCost;
                        response["previousTravelTimeSeconds"] = activeRoute["estimatedTravelTimeSeconds"];
                        response["newTravelTimeSeconds"] = newResult.estimatedTravelTimeSeconds;
                    }
                } else {
                    rerouted = true; // No route available is a significant route change
                }

                // For CLEAR, only say routeAffected=true if it ACTUALLY changed the route.
                // For ADD, say it if it was on the route.
                if (isClear && !rerouted) {
                    response["routeAffected"] = false;
                } else {
                    response["routeAffected"] = true;
                    response["rerouted"] = rerouted;

                    response["newRouteResult"] = {
                        {"found", newResult.found},
                        {"algorithm", algoStr},
                        {"objective", objStr},
                        {"start", start},
                        {"end", end},
                        {"nodeIds", newResult.nodeIds},
                        {"totalDistanceMeters", newResult.totalCost},
                        {"estimatedTravelTimeSeconds", newResult.estimatedTravelTimeSeconds},
                        {"nodesExplored", newResult.nodesExplored},
                        {"runtimeMicroseconds", newResult.runtimeMicroseconds}
                    };
                }
            }
        }
    };

    svr.Post("/incident", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(stateMutex);
        set_cors_headers(res);
        try {
            auto body = json::parse(req.body);
            if (!body.contains("from") || !body.contains("to") || !body.contains("type")) {
                res.status = 400;
                res.set_content(json{{"error", {{"code", "BAD_REQUEST"}, {"message", "Missing fields"}}}}.dump(), "application/json");
                return;
            }
            
            int from = body["from"];
            int to = body["to"];
            std::string typeStr = body["type"];
            
            model::IncidentType type;
            if (typeStr == "closure") type = model::IncidentType::Closure;
            else if (typeStr == "accident") type = model::IncidentType::Accident;
            else {
                res.status = 400;
                res.set_content(json{{"error", {{"code", "BAD_REQUEST"}, {"message", "Invalid incident type"}}}}.dump(), "application/json");
                return;
            }

            if (!network.getNodeById(from) || !network.getNodeById(to)) {
                res.status = 400;
                res.set_content(json{{"error", {{"code", "BAD_REQUEST"}, {"message", "Invalid node ID"}}}}.dump(), "application/json");
                return;
            }

            if (network.addIncident(from, to, type)) {
                json response = {
                    {"status", "success"},
                    {"incidentAdded", true}
                };
                processReroute(body, from, to, response, false);
                res.status = 200;
                res.set_content(response.dump(), "application/json");
            } else {
                res.status = 400;
                res.set_content(json{{"error", {{"code", "NOT_FOUND"}, {"message", "Incident already exists or road not found"}}}}.dump(), "application/json");
            }
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", {{"code", "BAD_REQUEST"}, {"message", "Malformed request"}}}}.dump(), "application/json");
        }
    });

    svr.Delete("/incident", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(stateMutex);
        set_cors_headers(res);
        try {
            auto body = json::parse(req.body);
            if (!body.contains("from") || !body.contains("to")) {
                res.status = 400;
                res.set_content(json{{"error", {{"code", "BAD_REQUEST"}, {"message", "Missing fields"}}}}.dump(), "application/json");
                return;
            }
            
            int from = body["from"];
            int to = body["to"];
            
            if (network.removeIncident(from, to)) {
                json response = {
                    {"status", "success"}
                };
                processReroute(body, from, to, response, true);
                res.status = 200;
                res.set_content(response.dump(), "application/json");
            } else {
                res.status = 404;
                res.set_content(json{{"error", {{"code", "NOT_FOUND"}, {"message", "Incident not found"}}}}.dump(), "application/json");
            }
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", {{"code", "BAD_REQUEST"}, {"message", "Malformed request"}}}}.dump(), "application/json");
        }
    });

    svr.Post("/simulation/vehicle", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(stateMutex);
        set_cors_headers(res);
        try {
            auto body = json::parse(req.body);
            if (!body.contains("origin") || !body.contains("destination")) {
                res.status = 400;
                res.set_content(json{{"error", {{"code", "BAD_REQUEST"}, {"message", "Missing origin or destination"}}}}.dump(), "application/json");
                return;
            }
            int origin = body["origin"];
            int dest = body["destination"];
            std::string prefStr = "balanced";
            if (body.contains("preference")) {
                prefStr = body["preference"];
            }
            
            int id = engine.spawnVehicle(origin, dest, prefStr);
            if (id == -1) {
                res.status = 400;
                res.set_content(json{{"error", {{"code", "BAD_REQUEST"}, {"message", "Could not spawn vehicle. Invalid nodes or no route."}}}}.dump(), "application/json");
                return;
            }
            
            const auto& vehicles = engine.getVehicles();
            for (const auto& v : vehicles) {
                if (v.id == id) {
                    json j = {
                        {"id", v.id},
                        {"preference", recommendation::preferenceToString(v.preference)},
                        {"origin", v.originNodeId},
                        {"destination", v.destinationNodeId},
                        {"state", model::vehicleStateToString(v.state)},
                        {"currentNodeId", v.currentNodeId},
                        {"nextNodeId", v.nextNodeId},
                        {"currentRouteIndex", v.currentRouteIndex},
                        {"progress", v.progressOnCurrentRoad},
                        {"x", v.x},
                        {"y", v.y},
                        {"routeNodeIds", v.routeNodeIds}
                    };
                    res.status = 200;
                    res.set_content(j.dump(), "application/json");
                    return;
                }
            }
        } catch (...) {
            res.status = 400;
            res.set_content(json{{"error", {{"code", "BAD_REQUEST"}, {"message", "Malformed request"}}}}.dump(), "application/json");
        }
    });

    svr.Get("/vehicles", [&](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(stateMutex);
        set_cors_headers(res);
        json j = json::array();
        for (const auto& v : engine.getVehicles()) {
            j.push_back({
                {"id", v.id},
                {"preference", recommendation::preferenceToString(v.preference)},
                {"origin", v.originNodeId},
                {"destination", v.destinationNodeId},
                {"state", model::vehicleStateToString(v.state)},
                {"currentNodeId", v.currentNodeId},
                {"nextNodeId", v.nextNodeId},
                {"currentRouteIndex", v.currentRouteIndex},
                {"progress", v.progressOnCurrentRoad},
                {"x", v.x},
                {"y", v.y},
                {"routeNodeIds", v.routeNodeIds}
            });
        }
        res.set_content(j.dump(), "application/json");
    });

    svr.Post("/simulation/step", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(stateMutex);
        set_cors_headers(res);
        try {
            double dt = 1.0;
            if (!req.body.empty()) {
                auto body = json::parse(req.body);
                if (body.contains("deltaTimeSeconds")) {
                    dt = body["deltaTimeSeconds"];
                }
            }
            engine.update(dt);
            
            json j = {
                {"simulationTimeSeconds", engine.getSimulationTimeSeconds()},
                {"vehicles", json::array()}
            };
            for (const auto& v : engine.getVehicles()) {
                j["vehicles"].push_back({
                    {"id", v.id},
                    {"preference", recommendation::preferenceToString(v.preference)},
                    {"origin", v.originNodeId},
                    {"destination", v.destinationNodeId},
                    {"state", model::vehicleStateToString(v.state)},
                    {"currentNodeId", v.currentNodeId},
                    {"nextNodeId", v.nextNodeId},
                    {"currentRouteIndex", v.currentRouteIndex},
                    {"progress", v.progressOnCurrentRoad},
                    {"x", v.x},
                    {"y", v.y},
                    {"routeNodeIds", v.routeNodeIds}
                });
            }
            res.set_content(j.dump(), "application/json");
        } catch (...) {
            res.status = 400;
            res.set_content(json{{"error", {{"code", "BAD_REQUEST"}, {"message", "Malformed request"}}}}.dump(), "application/json");
        }
    });

    svr.Post("/simulation/reset", [&](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(stateMutex);
        set_cors_headers(res);
        engine.reset();
        res.set_content(json{{"status", "success"}}.dump(), "application/json");
    });

    svr.Get("/simulation", [&](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(stateMutex);
        set_cors_headers(res);
        std::map<std::string, int> prefCounts;
        for (const auto& v : engine.getVehicles()) {
            prefCounts[recommendation::preferenceToString(v.preference)]++;
        }

        json j = {
            {"simulationTimeSeconds", engine.getSimulationTimeSeconds()},
            {"vehicleCount", engine.getVehicles().size()},
            {"waiting", engine.getWaitingVehicleCount()},
            {"moving", engine.getMovingVehicleCount()},
            {"arrived", engine.getArrivedVehicleCount()},
            {"congestedRoadCount", engine.getCongestedRoadCount()},
            {"averageCongestionFactor", engine.getAverageCongestionFactor()},
            {"totalReroutes", engine.getTotalReroutes()},
            {"preferences", prefCounts}
        };
        res.set_content(j.dump(), "application/json");
    });

    svr.Post("/simulation/vehicles/batch", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(stateMutex);
        set_cors_headers(res);
        try {
            int count = 10;
            if (!req.body.empty()) {
                auto body = json::parse(req.body);
                if (body.contains("count")) {
                    count = body["count"];
                }
            }
            int spawned = engine.spawnVehiclesBatch(count);
            
            json j = {
                {"spawned", spawned},
                {"vehicleCount", engine.getVehicles().size()}
            };
            res.set_content(j.dump(), "application/json");
        } catch (...) {
            res.status = 400;
            res.set_content(json{{"error", {{"code", "BAD_REQUEST"}, {"message", "Malformed request"}}}}.dump(), "application/json");
        }
    });

    std::cout << "Starting backend API server on http://" << host << ":" << port << std::endl;
    bool success = svr.listen(host, port);
    if (!success) {
        std::cerr << "Failed to start server on port " << port << std::endl;
    }
}

} // namespace api
