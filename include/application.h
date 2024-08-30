#pragma once

#include <stdint.h>

#include "defines.h"

#include "geometry_reference.h"
#include "object_reference.h"

typedef struct application
{
    const char* route_name;
    char full_route_path[MAX_CHAR_ARRAY_SIZE];

    object_reference_t* object_references;
    uint32_t object_references_size;
    char (*unique_relative_model_paths)[MAX_CHAR_ARRAY_SIZE];
    uint32_t unique_relative_model_paths_size;
    geometry_reference_t* geometry_references;
    uint32_t geometry_references_size;
    char (*unique_relative_texture_paths)[MAX_CHAR_ARRAY_SIZE];
    uint32_t unique_relative_texture_paths_size;
    char (*valid_relative_texture_paths)[MAX_CHAR_ARRAY_SIZE];
    uint32_t valid_relative_texture_paths_size;
} application_t;

void application_destroy(application_t* application);

int application_parse_arguments(application_t* application, int argc, char* argv[]);

int application_load_objects_ref(application_t* application);

int application_get_unique_relative_model_paths(application_t* application);

int application_load_models(application_t* application);

int application_get_unique_relative_texture_paths(application_t* application);

int application_check_texture_files_existence(application_t* application);
