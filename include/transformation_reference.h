#pragma once

#include "defines.h"

#include "my_math.h"

typedef struct transformation_reference
{
    char label[MAX_CHAR_ARRAY_SIZE];
    vec3_t translation;
    vec3_t rotation;
} transformation_reference_t;
