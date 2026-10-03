#pragma once

#include "model/RoadNetwork.hpp"

namespace model {

class GeneratedCityLoader {
public:
    static RoadNetwork generate5x5Grid();
};

} // namespace model
