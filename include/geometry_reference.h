#pragma once

#include "defines.h"

#include "my_math.h"

typedef struct geometry_reference
{
    char relative_model_path[MAX_CHAR_ARRAY_SIZE];
    geometry_t* geometry;
} geometry_reference_t;
