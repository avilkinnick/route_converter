#pragma once

#include <stdint.h>

typedef enum gltf_bufferView_target
{
    GLTF_BUFFER_VIEW_TARGET_ARRAY_BUFFER = 34962,
    GLTF_BUFFER_VIEW_TARGET_ELEMENT_ARRAY_BUFFER = 34963
} gltf_bufferView_target_t;

typedef struct gltf_bufferView
{
    uint32_t buffer;
    uint32_t byteOffset;
    uint32_t byteLength;
    uint32_t byteStride;
    gltf_bufferView_target_t target;
} gltf_bufferView_t;
