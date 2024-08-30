#pragma once

#include <stdint.h>

#include "defines.h"

typedef struct mesh_reference
{
    char label[MAX_CHAR_ARRAY_SIZE];
    uint32_t geometry_index;
    uint32_t texture_index;
    uint32_t flags;
} mesh_reference_t;
