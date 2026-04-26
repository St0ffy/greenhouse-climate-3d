cmake_minimum_required(VERSION 3.16)

project(GreenhouseClimate3D
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(greenhouse_core
    src/Config.cpp
    src/Geometry.cpp
    src/Weather.cpp
    src/Material.cpp
    src/Devices.cpp
    src/Physics.cpp
    src/Simulator.cpp
    src/Optimizer.cpp
    src/Exporter.cpp
    src/Report.cpp
)

target_include_directories(greenhouse_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(greenhouse3d
    src/main.cpp
)

target_link_libraries(greenhouse3d PRIVATE greenhouse_core)

enable_testing()

add_executable(test_geometry
    tests/test_geometry.cpp
)

target_link_libraries(test_geometry PRIVATE greenhouse_core)

add_test(NAME geometry COMMAND test_geometry)
