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

add_executable(smart_mobility_backend
    src/main.cpp
    src/model/RoadNetwork.cpp
    src/model/GeneratedCityLoader.cpp
    src/routing/Router.cpp
    src/api/HttpServer.cpp
)
target_include_directories(smart_mobility_backend PRIVATE include)
target_link_libraries(smart_mobility_backend PRIVATE nlohmann_json::nlohmann_json httplib::httplib)
