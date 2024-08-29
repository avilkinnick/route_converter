#pragma once

#include <stdint.h>

#include "defines.h"

typedef struct gltf_node
{
    float matrix[16];
    uint32_t mesh;
    char name[MAX_CHAR_ARRAY_SIZE];
} gltf_node_t;
