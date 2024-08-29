#include "application.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmake_defines.h"

static int first_char_is_slash(const char* string)
{
    return (string[0] == '/') || (string[0] == '\\');
}

void application_destroy(application_t* application)
{
    gltf_scene_destroy(&application->gltf_scene);

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

    free(application->geometry_references);
    application->geometry_references = NULL;

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

static int current_buffer_is_property(const char* current_buffer, uint32_t* flags)
{
    if (strcmp(current_buffer, "[mipmap]") == 0)
    {
        *flags |= OBJECT_REFERENCE_FLAGS_MIPMAP;
        return 1;
    }

    if (strcmp(current_buffer, "[not_mipmap]") == 0)
    {
        *flags &= ~OBJECT_REFERENCE_FLAGS_MIPMAP;
        return 1;
    }

    if (strcmp(current_buffer, "[smooth]") == 0)
    {
        *flags |= OBJECT_REFERENCE_FLAGS_SMOOTH;
        return 1;
    }

    if (strcmp(current_buffer, "[not_smooth]") == 0)
    {
        *flags &= ~OBJECT_REFERENCE_FLAGS_SMOOTH;
        return 1;
    }

    return 0;
}

// TODO: remove duplication
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

    char buffers[3][256];
    uint32_t current_buffer_index = 0;

    uint32_t flags = 0;

    while (fscanf(file, "%s", buffers[current_buffer_index]) != EOF)
    {
        const char* current_buffer = buffers[current_buffer_index];
        
        if (current_buffer_is_property(current_buffer, &flags))
        {
            continue;
        }
        
        switch (current_buffer_index)
        {
        case 0:
        {
            if (!first_char_is_slash(current_buffer))
            {
                ++current_buffer_index;
            }

            break;
        }
        case 1:
        {
            if (first_char_is_slash(current_buffer))
            {
                ++current_buffer_index;
            }
            else
            {
                strcpy(buffers[0], current_buffer);
            }

            break;
        }
        case 2:
        {
            if (first_char_is_slash(current_buffer))
            {
                ++application->object_references_size;
                current_buffer_index = 0;
            }
            else
            {
                strcpy(buffers[0], current_buffer);
                current_buffer_index = 1;
            }

            break;
        }
        default:
        {
            break;
        }
        }
    }

    if (application->object_references_size == 0)
    {
        fprintf(stderr, "Failed to read any object reference from objects.ref!\n");
        fclose(file);
        file = NULL;
        return 0;
    }

    application->object_references = malloc(application->object_references_size * sizeof(object_reference_t));
    if (application->object_references == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for object_references!\n");
        fclose(file);
        file = NULL;
        return 0;
    }

    application->object_references_size = 0;
    fseek(file, 0, SEEK_SET);
    current_buffer_index = 0;
    flags = 0;

    while (fscanf(file, "%s", buffers[current_buffer_index]) != EOF)
    {
        const char* current_buffer = buffers[current_buffer_index];
        
        if (current_buffer_is_property(current_buffer, &flags))
        {
            continue;
        }
        
        switch (current_buffer_index)
        {
        case 0:
        {
            if (!first_char_is_slash(current_buffer))
            {
                ++current_buffer_index;
            }

            break;
        }
        case 1:
        {
            if (first_char_is_slash(current_buffer))
            {
                ++current_buffer_index;
            }
            else
            {
                strcpy(buffers[0], current_buffer);
            }

            break;
        }
        case 2:
        {
            if (first_char_is_slash(current_buffer))
            {
                object_reference_t object_reference;
                strcpy(object_reference.label, buffers[0]);
                strcpy(object_reference.relative_model_path, buffers[1]);
                strcpy(object_reference.relative_texture_path, buffers[2]);
                object_reference.flags = flags;

                application->object_references[application->object_references_size] = object_reference;
                ++application->object_references_size;
                
                current_buffer_index = 0;
            }
            else
            {
                strcpy(buffers[0], current_buffer);
                current_buffer_index = 1;
            }

            break;
        }
        default:
        {
            break;
        }
        }
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

    for (uint32_t i = 0; i < application->object_references_size; ++i)
    {
        const char* object_reference_relative_model_path = application->object_references[i].relative_model_path;

        int found = 0;
        for (uint32_t j = 0; j < application->unique_relative_model_paths_size; ++j)
        {
            const char* unique_relative_model_path = application->unique_relative_model_paths[j];

            if (strcmp(object_reference_relative_model_path, unique_relative_model_path) == 0)
            {
                found = 1;
                break;
            }
        }

        if (!found)
        {
            strcpy(application->unique_relative_model_paths[application->unique_relative_model_paths_size], object_reference_relative_model_path);
            ++application->unique_relative_model_paths_size;
        }
    }

    char (*new_memory_block)[MAX_CHAR_ARRAY_SIZE] = realloc(application->unique_relative_model_paths, application->unique_relative_model_paths_size * MAX_CHAR_ARRAY_SIZE * sizeof(char));
    if (new_memory_block == NULL)
    {
        fprintf(stderr, "Failed to reallocate memory for unique_relative_model_paths!\n");
        return 0;
    }

    application->unique_relative_model_paths = new_memory_block;

    return 1;
}
