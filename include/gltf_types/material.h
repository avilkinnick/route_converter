#pragma once

#include <stdint.h>

#define GLTF_MATERIAL_ALPHA_MODE_OPAQUE "OPAQUE"
#define GLTF_MATERIAL_ALPHA_MODE_MASK "MASK"
#define GLTF_MATERIAL_ALPHA_MODE_BLEND "BLEND"

typedef struct gltf_material_pbrMetallicRoughness_baseColorTexture
{
    uint32_t index;
    uint32_t texCoord;
} gltf_material_pbrMetallicRoughness_baseColorTexture_t;

typedef struct gltf_material_pbrMetallicRoughness
{
    gltf_material_pbrMetallicRoughness_baseColorTexture_t baseColorTexture;
} gltf_material_pbrMetallicRoughness_t;

typedef struct gltf_material
{
    gltf_material_pbrMetallicRoughness_t pbrMetallicRoughness;
    const char* alphaMode;
} gltf_material_t;
