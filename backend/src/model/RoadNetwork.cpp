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

const Node* RoadNetwork::getNodeById(int id) const {
    auto it = std::find_if(nodes.begin(), nodes.end(), [id](const Node& n) {
        return n.id == id;
    });

    if (it != nodes.end()) {
        return &(*it);
    }
    return nullptr;
}

void RoadNetwork::setRoadClosed(int from, int to, bool closed) {
    for (auto& road : roads) {
        if (road.from == from && road.to == to) {
            road.closed = closed;
        }
    }
}

} // namespace model
