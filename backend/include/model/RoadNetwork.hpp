#pragma once

#include "model/Node.hpp"
#include "model/Road.hpp"
#include <vector>
#include <string>

namespace model {

enum class IncidentType {
    Closure,
    Accident
};

struct Incident {
    int from;
    int to;
    IncidentType type;
    double originalTrafficFactorFromTo{1.0};
    double originalTrafficFactorToFrom{1.0};
    bool originalClosedFromTo{false};
    bool originalClosedToFrom{false};
};

enum class CoordinateSystem {
    Cartesian,
    Geographic
};

/*
 * RoadNetwork is the shared graph representation used by both
 * GeneratedCityLoader and MontrealOSMLoader.
 * 
 * Nodes represent intersections.
 * Roads represent directed road segments (edges).
 * 
 * Routing algorithms depend only on RoadNetwork, so they do not
 * need to know whether the graph came from a generated city or OpenStreetMap.
 */
class RoadNetwork {
public:
    RoadNetwork() = default;

    CoordinateSystem getCoordinateSystem() const { return coordSystem; }
    void setCoordinateSystem(CoordinateSystem sys) { coordSystem = sys; }

    void addNode(const Node& node);
    void addRoad(const Road& road);

    // Returning references (&) instead of copies avoids duplicating 
    // potentially huge graphs (e.g., 1.4 million edges for Montreal).
    const std::vector<Node>& getNodes() const;
    const std::vector<Road>& getRoads() const;
    std::vector<Road>& getRoadsMutable();

    const Node* getNodeById(int id) const;
    Road* getRoadMutable(int from, int to);
    const Road* getRoad(int from, int to) const;
    
    void setRoadClosed(int from, int to, bool closed);

    // Incidents
    bool addIncident(int from, int to, IncidentType type);
    bool removeIncident(int from, int to);
    const std::vector<Incident>& getIncidents() const;
    
    void buildIndex();
    const std::vector<const Road*>& getOutgoingRoads(int nodeId) const;

private:
    CoordinateSystem coordSystem{CoordinateSystem::Cartesian};
    std::vector<Node> nodes;
    std::vector<Road> roads;
    std::vector<Incident> incidents;
    
    // We store outgoing road pointers per node so routing does not
    // need to scan every road in the graph for each expansion.
    // This transforms neighbor lookup from an O(R) full graph scan 
    // to an O(out-degree) direct adjacency access.
    std::vector<std::vector<const Road*>> adj;
    std::vector<const Road*> emptyRoads; // For returning empty list
};

} // namespace model
