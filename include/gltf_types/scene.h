#pragma once

#include <stdint.h>

#include "defines.h"

typedef struct gltf_scene
{
    uint32_t* nodes;
    uint32_t nodes_size;
    char name[MAX_CHAR_ARRAY_SIZE];
} gltf_scene_t;
