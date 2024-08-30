#pragma once

#include <stdint.h>

#define GLTF_MATERIAL_ALPHA_MODE_OPAQUE "OPAQUE"
#define GLTF_MATERIAL_ALPHA_MODE_MASK "MASK"
#define GLTF_MATERIAL_ALPHA_MODE_BLEND "BLEND"

typedef struct gltf_material
{
    struct pbrMetallicRoughness
    {
        struct baseColorTexture
        {
            uint32_t index;
            uint32_t texCoord;
        } baseColorTexture;
    } pbrMetallicRoughness;
    const char* alphaMode;
} gltf_material_t;
