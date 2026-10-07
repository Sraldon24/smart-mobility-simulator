include(FetchContent)

# Fetch nlohmann/json
FetchContent_Declare(json
    URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
)
FetchContent_MakeAvailable(json)

# Fetch cpp-httplib
FetchContent_Declare(httplib
    URL https://github.com/yhirose/cpp-httplib/archive/refs/tags/v0.14.1.tar.gz
)
FetchContent_MakeAvailable(httplib)

find_package(Threads REQUIRED)
find_package(ZLIB REQUIRED)
find_package(EXPAT REQUIRED)
find_package(BZip2 REQUIRED)
# lz4 does not usually have a built-in find_package, so we use pkg-config or target_link_libraries directly.

add_executable(smart_mobility_backend
    src/main.cpp
    src/model/RoadNetwork.cpp
    src/model/GeneratedCityLoader.cpp
    src/model/MontrealOSMLoader.cpp
    src/routing/Router.cpp
    src/api/HttpServer.cpp
    src/simulation/SimulationEngine.cpp
    src/recommendation/UserProfile.cpp
    src/recommendation/RecommendationEngine.cpp
    src/recommendation/RouteCandidateGenerator.cpp
)
target_include_directories(smart_mobility_backend PRIVATE include)
target_link_libraries(smart_mobility_backend PRIVATE nlohmann_json::nlohmann_json httplib::httplib Threads::Threads ZLIB::ZLIB EXPAT::EXPAT BZip2::BZip2 lz4)

add_executable(test_recommendation
    src/test_recommendation.cpp
    src/model/RoadNetwork.cpp
    src/routing/Router.cpp
    src/recommendation/UserProfile.cpp
    src/recommendation/RecommendationEngine.cpp
    src/recommendation/RouteCandidateGenerator.cpp
)
target_include_directories(test_recommendation PRIVATE include)
