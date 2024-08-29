#pragma once

#include <stdint.h>

#include "defines.h"

typedef enum object_reference_flags
{
    OBJECT_REFERENCE_FLAGS_MIPMAP = 0x00000001,
    OBJECT_REFERENCE_FLAGS_SMOOTH = 0x00000002
} object_reference_flags_t;

typedef struct object_reference
{
    char label[MAX_CHAR_ARRAY_SIZE];
    char relative_model_path[MAX_CHAR_ARRAY_SIZE];
    char relative_texture_path[MAX_CHAR_ARRAY_SIZE];
    uint32_t flags;
} object_reference_t;
