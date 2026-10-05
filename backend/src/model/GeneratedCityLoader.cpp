#include "model/GeneratedCityLoader.hpp"

namespace model {

RoadNetwork GeneratedCityLoader::generate5x5Grid() {
    RoadNetwork network;
    const int gridSize = 5;
    const double distance = 100.0;
    const double speedLimit = 50.0;

    for (int row = 0; row < gridSize; ++row) {
        for (int col = 0; col < gridSize; ++col) {
            int id = row * gridSize + col;
            double x = col * distance;
            double y = row * distance;
            network.addNode(Node{id, x, y});
        }
    }

    for (int row = 0; row < gridSize; ++row) {
        for (int col = 0; col < gridSize; ++col) {
            int id = row * gridSize + col;

            auto getTrafficFactor = [&](int r, int c, int dir) {
                // To test shortest != fastest:
                // Create massive traffic on the direct route between 1 and 2
                if (r == 0 && c == 1 && dir == 0) return 10.0; // edge 1->2
                
                // Add some moderate traffic elsewhere for general testing
                if (r == 2 && dir == 0) return 2.0; // center horizontal
                if (c == 2 && dir == 1) return 2.0; // center vertical
                
                return 1.0;
            };

            if (col < gridSize - 1) {
                int rightId = id + 1;
                double tf = getTrafficFactor(row, col, 0);
                network.addRoad(Road{id, rightId, distance, speedLimit, tf, 1.0, 1.0, false, 0});
                network.addRoad(Road{rightId, id, distance, speedLimit, tf, 1.0, 1.0, false, 0}); // Symmetric traffic for simplicity
            }

            if (row < gridSize - 1) {
                int bottomId = id + gridSize;
                double tf = getTrafficFactor(row, col, 1);
                network.addRoad(Road{id, bottomId, distance, speedLimit, tf, 1.0, 1.0, false, 0});
                network.addRoad(Road{bottomId, id, distance, speedLimit, tf, 1.0, 1.0, false, 0});
            }
        }
    }

    return network;
}

} // namespace model
