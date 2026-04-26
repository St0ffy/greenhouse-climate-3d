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

add_executable(test_config
    tests/test_config.cpp
)

target_link_libraries(test_config PRIVATE greenhouse_core)

add_test(NAME config COMMAND test_config)

add_executable(test_weather
    tests/test_weather.cpp
)

target_link_libraries(test_weather PRIVATE greenhouse_core)

add_test(NAME weather COMMAND test_weather)

add_executable(test_material
    tests/test_material.cpp
)

target_link_libraries(test_material PRIVATE greenhouse_core)

add_test(NAME material COMMAND test_material)

add_executable(test_devices
    tests/test_devices.cpp
)

target_link_libraries(test_devices PRIVATE greenhouse_core)

add_test(NAME devices COMMAND test_devices)

add_executable(test_physics
    tests/test_physics.cpp
)

target_link_libraries(test_physics PRIVATE greenhouse_core)

add_test(NAME physics COMMAND test_physics)
