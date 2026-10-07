with open('backend/src/model/RoadNetwork.cpp', 'r') as f:
    content = f.read()

# Remove the incorrectly placed code at the very end
content = content.replace('''
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
''', '')

# Insert it before the closing brace of the namespace
insert_pos = content.rfind('} // namespace model')
content = content[:insert_pos] + '''
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

''' + content[insert_pos:]

with open('backend/src/model/RoadNetwork.cpp', 'w') as f:
    f.write(content)
