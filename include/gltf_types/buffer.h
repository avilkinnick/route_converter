#pragma once

#include <stdint.h>

#include "defines.h"

typedef struct gltf_buffer
{
    char uri[MAX_CHAR_ARRAY_SIZE];
    uint32_t byteLength;
} gltf_buffer_t;
