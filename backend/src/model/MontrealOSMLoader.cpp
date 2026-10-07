#include "model/MontrealOSMLoader.hpp"
#include <osmium/io/pbf_input.hpp>
#include <osmium/handler.hpp>
#include <osmium/visitor.hpp>
#include <osmium/osm/way.hpp>
#include <osmium/osm/node.hpp>
#include <osmium/index/map/sparse_mem_array.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <chrono>
#include <iostream>
#include <unordered_set>
#include <unordered_map>
#include <cmath>
#include <string>

namespace smart_mobility {
namespace model {

double MontrealOSMLoader::calculateHaversineDistance(double lat1, double lon1, double lat2, double lon2) {
    constexpr double R = 6371000.0; // Earth's radius in meters
    constexpr double PI = 3.14159265358979323846;
    
    double dLat = (lat2 - lat1) * PI / 180.0;
    double dLon = (lon2 - lon1) * PI / 180.0;
    
    lat1 = lat1 * PI / 180.0;
    lat2 = lat2 * PI / 180.0;
    
    double a = std::sin(dLat / 2) * std::sin(dLat / 2) +
               std::sin(dLon / 2) * std::sin(dLon / 2) * std::cos(lat1) * std::cos(lat2);
    double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));
    
    return R * c;
}

static double getFallbackSpeedKph(const std::string& highway) {
    if (highway == "motorway" || highway == "motorway_link") return 100.0;
    if (highway == "trunk" || highway == "trunk_link") return 90.0;
    if (highway == "primary" || highway == "primary_link") return 70.0;
    if (highway == "secondary" || highway == "secondary_link") return 60.0;
    if (highway == "tertiary" || highway == "tertiary_link") return 50.0;
    if (highway == "residential") return 40.0;
    if (highway == "service") return 30.0;
    return 50.0;
}

class OSMWayHandler : public osmium::handler::Handler {
public:
    OSMImportStats stats;
    int samplesPrinted = 0;
    ::model::RoadNetwork network;
    
    std::unordered_map<osmium::object_id_type, int> osmNodeToInternalId;
    int nextInternalNodeId = 0;

    int getOrCreateNode(osmium::object_id_type osm_id, double lon, double lat) {
        auto it = osmNodeToInternalId.find(osm_id);
        if (it != osmNodeToInternalId.end()) {
            return it->second;
        }
        int internal_id = nextInternalNodeId++;
        osmNodeToInternalId[osm_id] = internal_id;
        
        ::model::Node new_node;
        new_node.id = internal_id;
        new_node.x = lon;
        new_node.y = lat;
        network.addNode(new_node);
        stats.internalNodes++;
        return internal_id;
    }

    void way(const osmium::Way& way) {
        stats.totalWaysExamined++;

        const char* highway = way.tags().get_value_by_key("highway");
        if (!highway) return;

        std::string h(highway);
        if (h == "motorway" || h == "trunk" || h == "primary" || 
            h == "secondary" || h == "tertiary" || h == "residential" || 
            h == "unclassified" || h == "service" ||
            h == "motorway_link" || h == "trunk_link" || h == "primary_link" ||
            h == "secondary_link" || h == "tertiary_link") {
            
            stats.drivableWays++;
            stats.referencedNodes += way.nodes().size();

            const char* oneway = way.tags().get_value_by_key("oneway");
            bool isOneway = false;
            bool reverseDirection = false;
            if (oneway) {
                std::string ow(oneway);
                if (ow == "yes" || ow == "1" || ow == "true") {
                    isOneway = true;
                    stats.oneWayRoads++;
                } else if (ow == "-1") {
                    isOneway = true;
                    reverseDirection = true;
                    stats.oneWayRoads++;
                }
            }

            double speedLimit = -1.0;
            const char* maxspeed = way.tags().get_value_by_key("maxspeed");
            if (maxspeed) {
                try {
                    std::string ms(maxspeed);
                    size_t kmh_pos = ms.find(" km/h");
                    if (kmh_pos != std::string::npos) ms = ms.substr(0, kmh_pos);
                    speedLimit = std::stod(ms);
                    stats.internalRoadsUsingMaxspeed++;
                } catch (...) {
                    speedLimit = getFallbackSpeedKph(h);
                    stats.internalRoadsUsingFallbackSpeed++;
                }
            } else {
                speedLimit = getFallbackSpeedKph(h);
                stats.internalRoadsUsingFallbackSpeed++;
            }

            const char* name = way.tags().get_value_by_key("name");
            if (name) stats.namedRoads++;

            if (samplesPrinted < 5 && name) {
                std::cout << "Road:\n";
                std::cout << "name: " << name << "\n";
                std::cout << "speed: " << speedLimit << " km/h\n";
                std::cout << "oneway: " << (isOneway ? "true" : "false") << "\n\n";
                samplesPrinted++;
            }

            // Create segments
            const auto& nodes = way.nodes();
            if (nodes.size() < 2) return;

            for (size_t i = 0; i < nodes.size() - 1; ++i) {
                try {
                    const auto& n1 = nodes[i];
                    const auto& n2 = nodes[i+1];
                    
                    if (!n1.location().valid() || !n2.location().valid()) {
                        continue; // Location missing
                    }
                    
                    int id1 = getOrCreateNode(n1.ref(), n1.lon(), n1.lat());
                    int id2 = getOrCreateNode(n2.ref(), n2.lon(), n2.lat());
                    
                    if (id1 == id2) continue; // Skip zero-length loop

                    double dist = MontrealOSMLoader::calculateHaversineDistance(n1.lat(), n1.lon(), n2.lat(), n2.lon());
                    if (dist < 0.0) dist = 0.0;
                    
                    int from = reverseDirection ? id2 : id1;
                    int to = reverseDirection ? id1 : id2;
                    
                    ::model::Road r1;
                    r1.from = from;
                    r1.to = to;
                    r1.distanceMeters = dist;
                    r1.speedLimitKph = speedLimit;
                    r1.baseTrafficFactor = 1.0;
                    r1.closed = false;
                    
                    network.addRoad(r1);
                    stats.internalDirectedRoads++;
                    
                    if (isOneway) {
                        stats.internalOneWaySegments++;
                    } else {
                        ::model::Road r2;
                        r2.from = to;
                        r2.to = from;
                        r2.distanceMeters = dist;
                        r2.speedLimitKph = speedLimit;
                        r2.baseTrafficFactor = 1.0;
                        r2.closed = false;
                        
                        network.addRoad(r2);
                        stats.internalDirectedRoads++;
                        stats.internalTwoWaySegments++;
                    }
                } catch (...) {
                    // Ignore missing nodes location if not available
                }
            }
        }
    }
};

MontrealOSMLoader::MontrealOSMLoader() {}

MontrealOSMLoader::~MontrealOSMLoader() {}

::model::RoadNetwork MontrealOSMLoader::load(const std::string& pbfFilePath) {
    auto start = std::chrono::high_resolution_clock::now();
    ::model::RoadNetwork network;
    network.setCoordinateSystem(::model::CoordinateSystem::Geographic);

    try {
        osmium::io::File input_file{pbfFilePath};
        osmium::io::Reader reader{input_file, osmium::osm_entity_bits::node | osmium::osm_entity_bits::way};

        using index_type = osmium::index::map::SparseMemArray<osmium::unsigned_object_id_type, osmium::Location>;
        using location_handler_type = osmium::handler::NodeLocationsForWays<index_type>;

        index_type index;
        location_handler_type location_handler{index};
        OSMWayHandler handler;

        osmium::apply(reader, location_handler, handler);
        reader.close();

        this->stats = handler.stats;
        network = std::move(handler.network);

    } catch (const std::exception& e) {
        std::cerr << "Failed to read OSM file: " << e.what() << "\n";
        return network;
    }

    auto end = std::chrono::high_resolution_clock::now();
    stats.importScanTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    return network;
}

} // namespace model
} // namespace smart_mobility
