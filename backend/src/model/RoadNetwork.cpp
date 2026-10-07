#include "model/RoadNetwork.hpp"
#include <algorithm>

namespace model {

void RoadNetwork::addNode(const Node& node) {
    nodes.push_back(node);
}

void RoadNetwork::addRoad(const Road& road) {
    roads.push_back(road);
}

const std::vector<Node>& RoadNetwork::getNodes() const {
    return nodes;
}

const std::vector<Road>& RoadNetwork::getRoads() const {
    return roads;
}

std::vector<Road>& RoadNetwork::getRoadsMutable() {
    return roads;
}

const Node* RoadNetwork::getNodeById(int id) const {
    auto it = std::find_if(nodes.begin(), nodes.end(), [id](const Node& n) {
        return n.id == id;
    });

    if (it != nodes.end()) {
        return &(*it);
    }
    return nullptr;
}

Road* RoadNetwork::getRoadMutable(int from, int to) {
    for (auto& road : roads) {
        if (road.from == from && road.to == to) {
            return &road;
        }
    }
    return nullptr;
}

const Road* RoadNetwork::getRoad(int from, int to) const {
    for (const auto& road : roads) {
        if (road.from == from && road.to == to) {
            return &road;
        }
    }
    return nullptr;
}

void RoadNetwork::setRoadClosed(int from, int to, bool closed) {
    Road* road = getRoadMutable(from, to);
    if (road) road->closed = closed;
}

bool RoadNetwork::addIncident(int from, int to, IncidentType type) {
    int minId = std::min(from, to);
    int maxId = std::max(from, to);
    
    // Check if incident already exists
    for (const auto& inc : incidents) {
        if (inc.from == minId && inc.to == maxId) {
            return false; // Already has an incident
        }
    }

    Road* fwd = getRoadMutable(minId, maxId);
    Road* bwd = getRoadMutable(maxId, minId);

    if (!fwd && !bwd) return false;

    Incident inc;
    inc.from = minId;
    inc.to = maxId;
    inc.type = type;

    if (fwd) {
        inc.originalClosedFromTo = fwd->closed;
        if (type == IncidentType::Closure) fwd->closed = true;
        else if (type == IncidentType::Accident) fwd->incidentFactor = 3.0; // severe accident penalty
    }

    if (bwd) {
        inc.originalClosedToFrom = bwd->closed;
        if (type == IncidentType::Closure) bwd->closed = true;
        else if (type == IncidentType::Accident) bwd->incidentFactor = 3.0;
    }

    incidents.push_back(inc);
    return true;
}

bool RoadNetwork::removeIncident(int from, int to) {
    int minId = std::min(from, to);
    int maxId = std::max(from, to);

    auto it = std::find_if(incidents.begin(), incidents.end(), [minId, maxId](const Incident& inc) {
        return inc.from == minId && inc.to == maxId;
    });

    if (it != incidents.end()) {
        Road* fwd = getRoadMutable(minId, maxId);
        Road* bwd = getRoadMutable(maxId, minId);

        if (fwd) {
            fwd->incidentFactor = 1.0;
            fwd->closed = it->originalClosedFromTo;
        }

        if (bwd) {
            bwd->incidentFactor = 1.0;
            bwd->closed = it->originalClosedToFrom;
        }

        incidents.erase(it);
        return true;
    }
    return false;
}

const std::vector<Incident>& RoadNetwork::getIncidents() const {
    return incidents;
}

} // namespace model
