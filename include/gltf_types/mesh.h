#pragma once

#include <stdint.h>

typedef enum gltf_mesh_primitive_mode
{
    GLTF_MESH_PRIMITIVE_MODE_POINTS = 0,
    GLTF_MESH_PRIMITIVE_MODE_LINES = 1,
    GLTF_MESH_PRIMITIVE_MODE_LINE_LOOP = 2,
    GLTF_MESH_PRIMITIVE_MODE_LINE_STRIP = 3,
    GLTF_MESH_PRIMITIVE_MODE_TRIANGLES = 4,
    GLTF_MESH_PRIMITIVE_MODE_TRIANGLE_STRIP = 5,
    GLTF_MESH_PRIMITIVE_MODE_TRIANGLE_FAN = 6
} gltf_mesh_primitive_mode_t;

typedef struct gltf_mesh_primitive_attributes
{
    uint32_t POSITION;
    uint32_t TEXCOORD_0;
} gltf_mesh_primitive_attributes_t;

typedef struct gltf_mesh_primitive
{
    gltf_mesh_primitive_attributes_t attributes;
    uint32_t indices;
    uint32_t material;
    gltf_mesh_primitive_mode_t mode;
} gltf_mesh_primitive_t;

typedef struct gltf_mesh
{
    gltf_mesh_primitive_t primitive;
} gltf_mesh_t;
