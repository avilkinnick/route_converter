#include "application.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "cmake_defines.h"

void application_destroy(application_t* application)
{
    free(application->gltf_nodes);
    application->gltf_nodes = NULL;

    free(application->gltf_meshes);
    application->gltf_meshes = NULL;

    free(application->gltf_materials);
    application->gltf_materials = NULL;

    free(application->gltf_textures);
    application->gltf_textures = NULL;

    free(application->gltf_images);
    application->gltf_images = NULL;

    free(application->gltf_accessors);
    application->gltf_accessors = NULL;

    free(application->unique_relative_model_paths);
    application->unique_relative_model_paths = NULL;

    free(application->object_references);
    application->object_references = NULL;
}

// TODO: command-line arguments

int application_parse_arguments(application_t* application, int argc, char* argv[])
{
    // if (argc != 2)
    // {
    //     fprintf(stderr, "Wrong number of arguments (%d / 2)! Expected route name as second argument!\n", argc);
    //     return 0;
    // }

    application->route_name = "agryz-krugloe_pole";
    sprintf(application->full_route_path, DMD_ROUTES_DIR "/%s", application->route_name);

    return 1;
}

int application_load_objects_ref(application_t* application)
{
    char full_file_path[MAX_CHAR_ARRAY_SIZE];
    sprintf(full_file_path, "%s/objects.ref", application->full_route_path);

    FILE* file = fopen(full_file_path, "r");
    if (file == NULL)
    {
        fprintf(stderr, "Failed to open file \"%s\" for reading!\n", full_file_path);
        return 0;
    }

    fclose(file);
    file = NULL;

    return 1;
}

int application_get_unique_relative_model_paths(application_t* application)
{
    application->unique_relative_model_paths = malloc(application->object_references_size * MAX_CHAR_ARRAY_SIZE * sizeof(char));
    if (application->unique_relative_model_paths == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for unique_relative_model_paths!\n");
        return 0;
    }

    return 1;
}
