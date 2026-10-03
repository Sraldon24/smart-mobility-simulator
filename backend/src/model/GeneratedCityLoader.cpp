#include "model/GeneratedCityLoader.hpp"

namespace model {

RoadNetwork GeneratedCityLoader::generate5x5Grid() {
    RoadNetwork network;
    const int gridSize = 5;
    const double distance = 100.0;
    const double speedLimit = 50.0;

    // Create 25 nodes
    for (int row = 0; row < gridSize; ++row) {
        for (int col = 0; col < gridSize; ++col) {
            int id = row * gridSize + col;
            double x = col * distance;
            double y = row * distance;
            network.addNode(Node{id, x, y});
        }
    }

    // Connect nodes with two-way roads
    for (int row = 0; row < gridSize; ++row) {
        for (int col = 0; col < gridSize; ++col) {
            int id = row * gridSize + col;

            // Connect to the right neighbor
            if (col < gridSize - 1) {
                int rightId = id + 1;
                network.addRoad(Road{id, rightId, distance, speedLimit, 1.0, false});
                network.addRoad(Road{rightId, id, distance, speedLimit, 1.0, false});
            }

            // Connect to the bottom neighbor
            if (row < gridSize - 1) {
                int bottomId = id + gridSize;
                network.addRoad(Road{id, bottomId, distance, speedLimit, 1.0, false});
                network.addRoad(Road{bottomId, id, distance, speedLimit, 1.0, false});
            }
        }
    }

    return network;
}

} // namespace model
