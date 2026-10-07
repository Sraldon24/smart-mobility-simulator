import re

with open('backend/src/model/RoadNetwork.cpp', 'r') as f:
    content = f.read()

# Replace getNodeById
get_node_old = r'''const Node\* RoadNetwork::getNodeById\(int id\) const \{
    auto it = std::find_if\(nodes\.begin\(\), nodes\.end\(\), \[id\]\(const Node& n\) \{
        return n\.id == id;
    \}\);

    if \(it != nodes\.end\(\)\) \{
        return &\(\*it\);
    \}
    return nullptr;
\}'''

get_node_new = '''const Node* RoadNetwork::getNodeById(int id) const {
    if (id >= 0 && static_cast<size_t>(id) < nodes.size() && nodes[id].id == id) {
        return &nodes[id];
    }
    auto it = std::find_if(nodes.begin(), nodes.end(), [id](const Node& n) {
        return n.id == id;
    });

    if (it != nodes.end()) {
        return &(*it);
    }
    return nullptr;
}'''

content = re.sub(get_node_old, get_node_new, content)

# Replace getRoad and getRoadMutable
# Actually, if adj is built, we can just use adj!
get_road_mut_old = r'''Road\* RoadNetwork::getRoadMutable\(int from, int to\) \{
    for \(auto& road : roads\) \{
        if \(road\.from == from && road\.to == to\) \{
            return &road;
        \}
    \}
    return nullptr;
\}'''

get_road_mut_new = '''Road* RoadNetwork::getRoadMutable(int from, int to) {
    if (from >= 0 && static_cast<size_t>(from) < adj.size()) {
        for (const Road* r : adj[from]) {
            if (r->to == to) {
                // Return mutable pointer by casting away const, since adj stores const Road* 
                // but we own the roads. Or calculate pointer offset.
                return const_cast<Road*>(r);
            }
        }
    }
    // Fallback if adj not built
    for (auto& road : roads) {
        if (road.from == from && road.to == to) {
            return &road;
        }
    }
    return nullptr;
}'''

content = re.sub(get_road_mut_old, get_road_mut_new, content)

get_road_old = r'''const Road\* RoadNetwork::getRoad\(int from, int to\) const \{
    for \(const auto& road : roads\) \{
        if \(road\.from == from && road\.to == to\) \{
            return &road;
        \}
    \}
    return nullptr;
\}'''

get_road_new = '''const Road* RoadNetwork::getRoad(int from, int to) const {
    if (from >= 0 && static_cast<size_t>(from) < adj.size()) {
        for (const Road* r : adj[from]) {
            if (r->to == to) return r;
        }
    }
    for (const auto& road : roads) {
        if (road.from == from && road.to == to) {
            return &road;
        }
    }
    return nullptr;
}'''

content = re.sub(get_road_old, get_road_new, content)

# Append buildIndex and getOutgoingRoads
append_code = '''

void RoadNetwork::buildIndex() {
    int maxNodeId = -1;
    for (const auto& node : nodes) {
        maxNodeId = std::max(maxNodeId, node.id);
    }
    
    if (maxNodeId >= 0) {
        adj.assign(maxNodeId + 1, std::vector<const Road*>());
    } else {
        adj.clear();
    }
    
    for (const auto& road : roads) {
        if (road.from >= 0 && road.from <= maxNodeId) {
            adj[road.from].push_back(&road);
        }
    }
}

const std::vector<const Road*>& RoadNetwork::getOutgoingRoads(int nodeId) const {
    if (nodeId >= 0 && static_cast<size_t>(nodeId) < adj.size()) {
        return adj[nodeId];
    }
    return emptyRoads;
}
'''

content += append_code

with open('backend/src/model/RoadNetwork.cpp', 'w') as f:
    f.write(content)
