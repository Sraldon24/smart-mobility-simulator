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

class RoadNetwork {
public:
    RoadNetwork() = default;

    CoordinateSystem getCoordinateSystem() const { return coordSystem; }
    void setCoordinateSystem(CoordinateSystem sys) { coordSystem = sys; }

    void addNode(const Node& node);
    void addRoad(const Road& road);

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

private:
    CoordinateSystem coordSystem{CoordinateSystem::Cartesian};
    std::vector<Node> nodes;
    std::vector<Road> roads;
    std::vector<Incident> incidents;
};

} // namespace model
