#pragma once

#include <stdint.h>

#define GLTF_ACCESSOR_TYPE_SCALAR "SCALAR"
#define GLTF_ACCESSOR_TYPE_VEC2 "VEC2"
#define GLTF_ACCESSOR_TYPE_VEC3 "VEC3"
#define GLTF_ACCESSOR_TYPE_VEC4 "VEC4"
#define GLTF_ACCESSOR_TYPE_MAT2 "MAT2"
#define GLTF_ACCESSOR_TYPE_MAT3 "MAT3"
#define GLTF_ACCESSOR_TYPE_MAT4 "MAT4"

typedef enum gltf_accessor_componentType
{
    GLTF_ACCESSOR_COMPONENT_TYPE_BYTE = 5120,
    GLTF_ACCESSOR_COMPONENT_TYPE_UNSIGNED_BYTE = 5121,
    GLTF_ACCESSOR_COMPONENT_TYPE_SHORT = 5122,
    GLTF_ACCESSOR_COMPONENT_TYPE_UNSIGNED_SHORT = 5123,
    GLTF_ACCESSOR_COMPONENT_TYPE_UNSIGNED_INT = 5125,
    GLTF_ACCESSOR_COMPONENT_TYPE_FLOAT = 5126
} gltf_accessor_componentType_t;

typedef struct gltf_accessor
{
    uint32_t bufferView;
    uint32_t byteOffset;
    gltf_accessor_componentType_t componentType;
    uint32_t count;
    const char* type;
    int32_t max_integers[16];
    float max_floats[16];
    int32_t min_integers[16];
    float min_floats[16];
} gltf_accessor_t;
