cmake_minimum_required(VERSION 3.10)

project(route_converter)

set(DMD_ROUTES_DIR ${PROJECT_SOURCE_DIR}/dmd_routes)
set(GLTF_ROUTE_DIR ${PROJECT_SOURCE_DIR}/gltf_route)

configure_file(config/cmake_defines.h.in cmake_defines.h)

add_executable(route_converter
    src/main.c
    src/application.c
    src/my_math.c
    src/gltf_types/scene.c
)

target_include_directories(route_converter PUBLIC ${PROJECT_BINARY_DIR} include)
