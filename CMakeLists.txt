cmake_minimum_required(VERSION 3.16)

project(GreenhouseClimate3D
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(greenhouse3d
    src/main.cpp
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

target_include_directories(greenhouse3d PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

enable_testing()
