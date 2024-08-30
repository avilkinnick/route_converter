#pragma once

#include <stdint.h>

#include "defines.h"

#include "my_math.h"

typedef struct node_reference
{
    char label[MAX_CHAR_ARRAY_SIZE];
    uint32_t mesh_index;
    mat4_t matrix;
} node_reference_t;
