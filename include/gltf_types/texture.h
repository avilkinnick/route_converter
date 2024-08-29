#pragma once

#include <stdint.h>

typedef struct gltf_texture
{
    uint32_t sampler;
    uint32_t source;
} gltf_texture_t;
