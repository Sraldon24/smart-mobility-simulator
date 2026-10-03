#pragma once

#include "model/Node.hpp"
#include "model/Road.hpp"
#include <vector>

namespace model {

class RoadNetwork {
public:
    void addNode(const Node& node);
    void addRoad(const Road& road);

    const std::vector<Node>& getNodes() const;
    const std::vector<Road>& getRoads() const;

    const Node* getNodeById(int id) const;
    void setRoadClosed(int from, int to, bool closed);

private:
    std::vector<Node> nodes;
    std::vector<Road> roads;
};

} // namespace model

