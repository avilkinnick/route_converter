#include "application.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "cmake_defines.h"

#include "gltf_types/accessor.h"
#include "gltf_types/bufferView.h"
#include "gltf_types/image.h"
#include "gltf_types/material.h"
#include "gltf_types/mesh.h"
#include "gltf_types/node.h"
#include "gltf_types/sampler.h"
#include "gltf_types/scene.h"
#include "gltf_types/texture.h"
#include "mesh_reference.h"
#include "my_math.h"
#include "transformation_reference.h"

static int first_char_is_slash(const char* string)
{
    return (string[0] == '/') || (string[0] == '\\');
}

static int string_is_float(const char* string)
{
    for (uint32_t i = 0; i < strlen(string); ++i)
    {
        char ch = string[i];
        if ((ch < '0' || ch > '9') && ch != '.' && ch != '-' && ch != 'e' && ch != 'E')
        {
            return 0;
        }
    }

    return 1;
}

static uint32_t find_char_index(const char* string, uint32_t offset, char ch)
{
    for (uint32_t i = offset; i < strlen(string); ++i)
    {
        if (string[i] == ch)
        {
            return i;
        }
    }

    return 0;
}

static void replace_slashes_with_underscores(char string[])
{
    for (uint32_t i = 0; i < strlen(string); ++i)
    {
        if (string[i] == '/' || string[i] == '\\')
        {
            string[i] = '_';
        }
    }
}

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

    free(application->node_references);
    application->node_references = NULL;

    free(application->transformation_references);
    application->transformation_references = NULL;

    free(application->mesh_references);
    application->mesh_references = NULL;

    free(application->valid_relative_texture_paths);
    application->valid_relative_texture_paths = NULL;

    free(application->unique_relative_texture_paths);
    application->unique_relative_texture_paths = NULL;

    for (uint32_t i = 0; i < application->geometry_references_size; ++i)
    {
        geometry_reference_destroy(&application->geometry_references[i]);
    }
    free(application->geometry_references);
    application->geometry_references = NULL;

    free(application->unique_relative_model_paths);
    application->unique_relative_model_paths = NULL;

    free(application->object_references);
    application->object_references = NULL;
}

// TODO: return command-line arguments
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

static void parse_objects_ref(application_t* application, FILE* file)
{
    char buffers[3][MAX_CHAR_ARRAY_SIZE];
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
                if (application->object_references != NULL)
                {
                    object_reference_t* object_reference = &application->object_references[application->object_references_size];
                    strcpy(object_reference->label, buffers[0]);
                    strcpy(object_reference->relative_model_path, buffers[1]);
                    strcpy(object_reference->relative_texture_path, buffers[2]);
                    object_reference->flags = flags;
                }

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
    
    parse_objects_ref(application, file);

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

    parse_objects_ref(application, file);

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

    if (application->unique_relative_model_paths_size != application->object_references_size)
    {
        char (*new_memory_block)[MAX_CHAR_ARRAY_SIZE] = realloc(application->unique_relative_model_paths, application->unique_relative_model_paths_size * MAX_CHAR_ARRAY_SIZE * sizeof(char));
        if (new_memory_block == NULL)
        {
            fprintf(stderr, "Failed to reallocate memory for unique_relative_model_paths!\n");
            return 0;
        }

        application->unique_relative_model_paths = new_memory_block;
    }

    return 1;
}

// TODO: split on several functions
// TODO: replace face count by index count?
static int load_model(application_t* application, const char* relative_model_path)
{
    char full_file_path[MAX_CHAR_ARRAY_SIZE];
    sprintf(full_file_path, "%s%s", application->full_route_path, relative_model_path);
    
    FILE* file = fopen(full_file_path, "r");
    if (file == NULL)
    {
        fprintf(stderr, "Failed to open file \"%s\" for reading!\n", full_file_path);
        return 0;
    }

    char buffer[MAX_CHAR_ARRAY_SIZE] = "";

    while (strcmp(buffer, "TriMesh()") != 0)
    {
        if (fscanf(file, "%s", buffer) == EOF)
        {
            fprintf(stderr, "Failed to find string \"TriMesh()\" in model file!\n");
            fclose(file);
            file = NULL;
            return 0;
        }
    }

    fscanf(file, "%*s%*s");

    uint32_t temp_vertices_size, temp_vertex_faces_size;
    if (fscanf(file, "%u%u", &temp_vertices_size, &temp_vertex_faces_size) != 2)
    {
        fprintf(stderr, "Failed to read temp_vertices_size and temp_vertex_faces_size from model file!\n");
        fclose(file);
        file = NULL;
        return 0;
    }

    fscanf(file, "%*s%*s");

    vec3_t* temp_vertices = malloc(temp_vertices_size * sizeof(vec3_t));
    if (temp_vertices == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for temp_vertices!\n");
        fclose(file);
        file = NULL;
        return 0;
    }

    for (uint32_t i = 0; i < temp_vertices_size; ++i)
    {
        fscanf(file, "%s", buffer);
        if (!string_is_float(buffer))
        {
            fprintf(stderr, "Found non-float value in vertices of model!\n");
            free(temp_vertices);
            temp_vertices = NULL;
            fclose(file);
            file = NULL;
            return 0;
        }
        temp_vertices[i].x = atof(buffer);

        fscanf(file, "%s", buffer);
        if (!string_is_float(buffer))
        {
            fprintf(stderr, "Found non-float value in vertices of model!\n");
            free(temp_vertices);
            temp_vertices = NULL;
            fclose(file);
            file = NULL;
            return 0;
        }
        temp_vertices[i].y = atof(buffer);

        fscanf(file, "%s", buffer);
        if (!string_is_float(buffer))
        {
            fprintf(stderr, "Found non-float value in vertices of model!\n");
            free(temp_vertices);
            temp_vertices = NULL;
            fclose(file);
            file = NULL;
            return 0;
        }
        temp_vertices[i].z = atof(buffer);
    }

    fscanf(file, "%*s%*s%*s%*s");

    uint32_t* temp_vertex_indices = malloc(temp_vertex_faces_size * 3 * sizeof(uint32_t));
    if (temp_vertex_indices == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for temp_vertex_indices!\n");
        free(temp_vertices);
        temp_vertices = NULL;
        fclose(file);
        file = NULL;
        return 0;
    }

    for (uint32_t i = 0; i < temp_vertex_faces_size * 3; ++i)
    {
        fscanf(file, "%u", &temp_vertex_indices[i]);
        --temp_vertex_indices[i];
    }

    while (strcmp(buffer, "Texture:") != 0)
    {
        if (fscanf(file, "%s", buffer) == EOF)
        {
            fprintf(stderr, "Failed to find string \"Texture:\" in model file!\n");
            free(temp_vertex_indices);
            temp_vertex_indices = NULL;
            free(temp_vertices);
            temp_vertices = NULL;
            fclose(file);
            file = NULL;
            return 0;
        }
    }

    fscanf(file, "%*s%*s");

    uint32_t temp_tex_coords_size, temp_tex_coord_faces_size;
    if (fscanf(file, "%u%u", &temp_tex_coords_size, &temp_tex_coord_faces_size) != 2)
    {
        fprintf(stderr, "Failed to read temp_tex_coords_size and temp_tex_coord_faces_size from model file!\n");
        free(temp_vertex_indices);
        temp_vertex_indices = NULL;
        free(temp_vertices);
        temp_vertices = NULL;
        fclose(file);
        file = NULL;
        return 0;
    }

    if (temp_vertex_faces_size != temp_tex_coord_faces_size)
    {
        fprintf(stderr, "Error! temp_vertex_faces_size must be equal to temp_tex_coord_faces_size!\n");
        free(temp_vertex_indices);
        temp_vertex_indices = NULL;
        free(temp_vertices);
        temp_vertices = NULL;
        fclose(file);
        file = NULL;
        return 0;
    }

    uint32_t temp_faces_size = temp_vertex_faces_size;

    fscanf(file, "%*s%*s");

    vec2_t* temp_tex_coords = malloc(temp_tex_coords_size * sizeof(vec2_t));
    if (temp_tex_coords == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for temp_tex_coords!\n");
        free(temp_vertex_indices);
        temp_vertex_indices = NULL;
        free(temp_vertices);
        temp_vertices = NULL;
        fclose(file);
        file = NULL;
        return 0;
    }

    for (uint32_t i = 0; i < temp_tex_coords_size; ++i)
    {
        fscanf(file, "%s", buffer);
        if (!string_is_float(buffer))
        {
            fprintf(stderr, "Found non-float value in texture coordinates of model!\n");
            free(temp_tex_coords);
            temp_tex_coords = NULL;
            free(temp_vertex_indices);
            temp_vertex_indices = NULL;
            free(temp_vertices);
            temp_vertices = NULL;
            fclose(file);
            file = NULL;
            return 0;
        }
        temp_tex_coords[i].x = atof(buffer);

        fscanf(file, "%s", buffer);
        if (!string_is_float(buffer))
        {
            fprintf(stderr, "Found non-float value in texture coordinates of model!\n");
            free(temp_tex_coords);
            temp_tex_coords = NULL;
            free(temp_vertex_indices);
            temp_vertex_indices = NULL;
            free(temp_vertices);
            temp_vertices = NULL;
            fclose(file);
            file = NULL;
            return 0;
        }
        temp_tex_coords[i].y = atof(buffer);

        fscanf(file, "%*s");
    }

    fscanf(file, "%*s%*s%*s%*s%*s");

    uint32_t* temp_tex_coord_indices = malloc(temp_faces_size * 3 * sizeof(uint32_t));
    if (temp_tex_coord_indices == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for temp_tex_coord_indices!\n");
        free(temp_tex_coords);
        temp_tex_coords = NULL;
        free(temp_vertex_indices);
        temp_vertex_indices = NULL;
        free(temp_vertices);
        temp_vertices = NULL;
        fclose(file);
        file = NULL;
        return 0;
    }

    for (uint32_t i = 0; i < temp_faces_size * 3; ++i)
    {
        fscanf(file, "%u", &temp_tex_coord_indices[i]);
        --temp_tex_coord_indices[i];
    }

    fclose(file);
    file = NULL;

    vertex_t* vertices = malloc(temp_faces_size * 3 * sizeof(vertex_t));
    if (vertices == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for vertices!\n");
        free(temp_tex_coord_indices);
        temp_tex_coord_indices = NULL;
        free(temp_tex_coords);
        temp_tex_coords = NULL;
        free(temp_vertex_indices);
        temp_vertex_indices = NULL;
        free(temp_vertices);
        temp_vertices = NULL;
        return 0;
    }

    for (uint32_t i = 0; i < temp_faces_size * 3; ++i)
    {
        vertices[i].pos = temp_vertices[temp_vertex_indices[i]];
        vertices[i].tex_coord = temp_tex_coords[temp_tex_coord_indices[i]];
    }

    free(temp_tex_coord_indices);
    temp_tex_coord_indices = NULL;

    free(temp_tex_coords);
    temp_tex_coords = NULL;

    free(temp_vertex_indices);
    temp_vertex_indices = NULL;

    free(temp_vertices);
    temp_vertices = NULL;

    uint32_t* indices = malloc(temp_faces_size * 3 * sizeof(uint32_t));
    if (indices == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for indices!\n");
        free(vertices);
        vertices = NULL;
        return 0;
    }

    for (uint32_t i = 0; i < temp_faces_size * 3; ++i)
    {
        indices[i] = i;
    }

    vertex_t* reduced_vertices = malloc(temp_faces_size * 3 * sizeof(vertex_t));
    if (reduced_vertices == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for reduced_vertices!\n");
        free(indices);
        indices = NULL;
        free(vertices);
        vertices = NULL;
        return 0;
    }

    uint32_t reduced_vertices_size = 0;

    for (uint32_t i = 0; i < temp_faces_size * 3; ++i)
    {
        const vertex_t* vertex_1 = &vertices[i];

        int found = 0;
        uint32_t found_index;
        for (uint32_t j = 0; j < reduced_vertices_size; ++j)
        {
            const vertex_t* vertex_2 = &reduced_vertices[j];

            if (vertex_equal(vertex_1, vertex_2))
            {
                found = 1;
                found_index = j;
                break;
            }
        }

        if (found)
        {
            indices[i] = found_index;
        }
        else
        {
            reduced_vertices[reduced_vertices_size] = *vertex_1;
            indices[i] = reduced_vertices_size;
            ++reduced_vertices_size;
        }
    }

    free(vertices);
    vertices = NULL;

    if (reduced_vertices_size != temp_faces_size * 3)
    {
        vertex_t* new_memory_block = realloc(reduced_vertices, reduced_vertices_size * sizeof(vertex_t));
        if (new_memory_block == NULL)
        {
            fprintf(stderr, "Failed to reallocate memory for reduced_vertices!\n");
            free(reduced_vertices);
            reduced_vertices = NULL;
            free(indices);
            indices = NULL;
            return 0;
        }

        reduced_vertices = new_memory_block;
    }

    uint32_t* reduced_indices = malloc(temp_faces_size * 3 * sizeof(uint32_t));
    if (reduced_indices == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for reduced_indices!\n");
        free(reduced_vertices);
        reduced_vertices = NULL;
        free(indices);
        indices = NULL;
        return 0;
    }

    uint32_t reduced_indices_size = 0;

    for (uint32_t i = 0; i < temp_faces_size * 3; i += 3)
    {
        uint32_t index_1 = indices[i];
        uint32_t index_2 = indices[i + 1];
        uint32_t index_3 = indices[i + 2];

        if (index_1 == index_2 || index_1 == index_3 || index_2 == index_3)
        {
            continue;
        }

        int found = 0;
        for (uint32_t j = 0; j < reduced_vertices_size; j += 3)
        {
            uint32_t index_4 = reduced_indices[j];
            uint32_t index_5 = reduced_indices[j + 1];
            uint32_t index_6 = reduced_indices[j + 2];

            if (index_1 == index_4 && index_2 == index_5 && index_3 == index_6)
            {
                found = 1;
                break;
            }
        }

        if (!found)
        {
            reduced_indices[reduced_indices_size] = index_1;
            reduced_indices[reduced_indices_size + 1] = index_2;
            reduced_indices[reduced_indices_size + 2] = index_3;
            reduced_indices_size += 3;
        }
    }

    if (reduced_indices_size != temp_faces_size * 3)
    {
        uint32_t* new_memory_block = realloc(reduced_indices, reduced_indices_size * sizeof(uint32_t));
        if (new_memory_block == NULL)
        {
            fprintf(stderr, "Failed to reallocate memory for reduced_indices!\n");
            free(reduced_indices);
            reduced_indices = NULL;
            free(reduced_vertices);
            reduced_vertices = NULL;
            free(indices);
            indices = NULL;
            return 0;
        }

        reduced_indices = new_memory_block;
    }

    free(indices);
    indices = NULL;

    geometry_t* geometry = malloc(sizeof(geometry_t));
    if (geometry == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for geometry!\n");
        free(reduced_indices);
        reduced_indices = NULL;
        free(reduced_vertices);
        reduced_vertices = NULL;
        return 0;
    }

    geometry->vertices = reduced_vertices;
    geometry->vertices_size = reduced_vertices_size;
    geometry->indices = reduced_indices;
    geometry->indices_size = reduced_indices_size;

    geometry_reference_t* geometry_reference = &application->geometry_references[application->geometry_references_size];
    strcpy(geometry_reference->relative_model_path, relative_model_path);
    geometry_reference->geometry = geometry;
    ++application->geometry_references_size;

    return 1;
}

int application_load_models(application_t* application)
{
    application->geometry_references = malloc(application->unique_relative_model_paths_size * sizeof(geometry_reference_t));
    if (application->geometry_references == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for geometry_references!\n");
        return 0;
    }

    for (uint32_t i = 0; i < application->unique_relative_model_paths_size; ++i)
    {
        if (!load_model(application, application->unique_relative_model_paths[i]))
        {
            fprintf(stderr, "Failed to load model file \"%s\"!\n", application->unique_relative_model_paths[i]);
        }
    }

    free(application->unique_relative_model_paths);
    application->unique_relative_model_paths = NULL;

    if (application->geometry_references_size != application->unique_relative_model_paths_size)
    {
        geometry_reference_t* new_memory_block = realloc(application->geometry_references, application->geometry_references_size * sizeof(geometry_reference_t));
        if (new_memory_block == NULL)
        {
            fprintf(stderr, "Failed to reallocate memory for geometry_references!\n");
            return 0;
        }

        application->geometry_references = new_memory_block;
    }

    return 1;
}

int application_get_unique_relative_texture_paths(application_t* application)
{
    application->unique_relative_texture_paths = malloc(application->object_references_size * MAX_CHAR_ARRAY_SIZE * sizeof(char));
    if (application->unique_relative_texture_paths == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for unique_relative_texture_paths!\n");
        return 0;
    }

    for (uint32_t i = 0; i < application->object_references_size; ++i)
    {
        const char* object_reference_relative_texture_path = application->object_references[i].relative_texture_path;

        int found = 0;
        for (uint32_t j = 0; j < application->unique_relative_texture_paths_size; ++j)
        {
            const char* unique_relative_texture_path = application->unique_relative_texture_paths[j];

            if (strcmp(object_reference_relative_texture_path, unique_relative_texture_path) == 0)
            {
                found = 1;
                break;
            }
        }

        if (!found)
        {
            strcpy(application->unique_relative_texture_paths[application->unique_relative_texture_paths_size], object_reference_relative_texture_path);
            ++application->unique_relative_texture_paths_size;
        }
    }

    if (application->unique_relative_texture_paths_size != application->object_references_size)
    {
        char (*new_memory_block)[MAX_CHAR_ARRAY_SIZE] = realloc(application->unique_relative_texture_paths, application->unique_relative_texture_paths_size * MAX_CHAR_ARRAY_SIZE * sizeof(char));
        if (new_memory_block == NULL)
        {
            fprintf(stderr, "Failed to reallocate memory for unique_relative_texture_paths!\n");
            return 0;
        }

        application->unique_relative_texture_paths = new_memory_block;
    }

    return 1;
}

int application_check_texture_files_existence(application_t* application)
{
    application->valid_relative_texture_paths = malloc(application->unique_relative_texture_paths_size * MAX_CHAR_ARRAY_SIZE * sizeof(char));
    if (application->valid_relative_texture_paths == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for valid_relative_texture_paths!\n");
        return 0;
    }

    for (uint32_t i = 0; i < application->unique_relative_texture_paths_size; ++i)
    {
        char full_file_path[MAX_CHAR_ARRAY_SIZE];
        sprintf(full_file_path, "%s%s", application->full_route_path, application->unique_relative_texture_paths[i]);

        FILE* file = fopen(full_file_path, "r");
        if (file != NULL)
        {
            fclose(file);
            file = NULL;

            strcpy(application->valid_relative_texture_paths[application->valid_relative_texture_paths_size], application->unique_relative_texture_paths[i]);
            ++application->valid_relative_texture_paths_size;
        }
    }

    free(application->unique_relative_texture_paths);
    application->unique_relative_texture_paths = NULL;

    if (application->valid_relative_texture_paths_size != application->unique_relative_texture_paths_size)
    {
        char (*new_memory_block)[MAX_CHAR_ARRAY_SIZE] = realloc(application->valid_relative_texture_paths, application->valid_relative_texture_paths_size * MAX_CHAR_ARRAY_SIZE * sizeof(char));
        if (new_memory_block == NULL)
        {
            fprintf(stderr, "Failed to reallocate memory for valid_relative_texture_paths!\n");
            return 0;
        }

        application->valid_relative_texture_paths = new_memory_block;
    }

    return 1;
}

static int find_geometry_index(application_t* application, const char* relative_model_path, uint32_t* geometry_index)
{
    for (uint32_t i = 0; i < application->geometry_references_size; ++i)
    {
        if (strcmp(relative_model_path, application->geometry_references[i].relative_model_path) == 0)
        {
            *geometry_index = i;
            return 1;
        }
    }

    return 0;
}

static int find_texture_index(application_t* application, const char* relative_texture_path, uint32_t* texture_index)
{
    for (uint32_t i = 0; i < application->valid_relative_texture_paths_size; ++i)
    {
        if (strcmp(relative_texture_path, application->valid_relative_texture_paths[i]) == 0)
        {
            *texture_index = i;
            return 1;
        }
    }

    return 0;
}

int application_remove_redundant_object_references(application_t* application)
{
    application->mesh_references = malloc(application->object_references_size * sizeof(mesh_reference_t));
    if (application->mesh_references == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for mesh_references!\n");
        return 0;
    }

    for (uint32_t i = 0; i < application->object_references_size; ++i)
    {
        const object_reference_t* object_reference = &application->object_references[i];

        uint32_t geometry_index;
        if (!find_geometry_index(application, object_reference->relative_model_path, &geometry_index))
        {
            continue;
        }

        uint32_t texture_index;
        if (!find_texture_index(application, object_reference->relative_texture_path, &texture_index))
        {
            continue;
        }

        int found = 0;
        for (uint32_t j = 0; j < application->mesh_references_size; ++j)
        {
            if (strcmp(object_reference->label, application->mesh_references[j].label) == 0)
            {
                found = 1;
                break;
            }
        }

        if (!found)
        {
            mesh_reference_t* mesh_reference = &application->mesh_references[application->mesh_references_size];
            strcpy(mesh_reference->label, object_reference->label);
            mesh_reference->geometry_index = geometry_index;
            mesh_reference->texture_index = texture_index;
            mesh_reference->flags = object_reference->flags;
            ++application->mesh_references_size;
        }
    }

    free(application->object_references);
    application->object_references = NULL;

    if (application->mesh_references_size != application->object_references_size)
    {
        mesh_reference_t* new_memory_block = realloc(application->mesh_references, application->mesh_references_size * sizeof(mesh_reference_t));
        if (new_memory_block == NULL)
        {
            fprintf(stderr, "Failed to reallocate memory for mesh_references!\n");
            return 0;
        }

        application->mesh_references = new_memory_block;
    }

    return 1;
}

// TODO: split on several functions
// TODO: remove duplication
static void parse_route_map(application_t* application, FILE* file)
{
    char buffer[MAX_CHAR_ARRAY_SIZE];

    while (fscanf(file, "%s", buffer) != EOF)
    {
        if (buffer[0] == ',' || buffer[strlen(buffer) - 1] != ';')
        {
            continue;
        }

        buffer[strlen(buffer) - 1] = '\0';

        uint32_t comma_1_index = find_char_index(buffer, 0, ',');
        uint32_t comma_2_index = find_char_index(buffer, comma_1_index + 1, ',');
        uint32_t comma_3_index = find_char_index(buffer, comma_2_index + 1, ',');
        uint32_t comma_4_index = find_char_index(buffer, comma_3_index + 1, ',');
        uint32_t comma_5_index = find_char_index(buffer, comma_4_index + 1, ',');
        uint32_t comma_6_index = find_char_index(buffer, comma_5_index + 1, ',');

        if (comma_1_index == 0 || comma_2_index == 0 || comma_3_index == 0 || comma_4_index == 0 || comma_5_index == 0 || comma_6_index == 0)
        {
            continue;
        }

        char label[MAX_CHAR_ARRAY_SIZE];
        strncpy(label, buffer, comma_1_index);
        label[comma_1_index] = '\0';

        char translation_x_string[MAX_CHAR_ARRAY_SIZE];
        strncpy(translation_x_string, buffer + comma_1_index + 1, comma_2_index - comma_1_index - 1);
        translation_x_string[comma_2_index - comma_1_index - 1] = '\0';

        float translation_x;
        if (!string_is_float(translation_x_string))
        {
            fprintf(stderr, "Found non-float value in translations!\n");
            continue;
        }
        translation_x = atof(translation_x_string);

        char translation_y_string[MAX_CHAR_ARRAY_SIZE];
        strncpy(translation_y_string, buffer + comma_2_index + 1, comma_3_index - comma_2_index - 1);
        translation_y_string[comma_3_index - comma_2_index - 1] = '\0';

        float translation_y;
        if (!string_is_float(translation_y_string))
        {
            fprintf(stderr, "Found non-float value in translations!\n");
            continue;
        }
        translation_y = atof(translation_y_string);

        char translation_z_string[MAX_CHAR_ARRAY_SIZE];
        strncpy(translation_z_string, buffer + comma_3_index + 1, comma_4_index - comma_3_index - 1);
        translation_z_string[comma_4_index - comma_3_index - 1] = '\0';

        float translation_z;
        if (!string_is_float(translation_z_string))
        {
            fprintf(stderr, "Found non-float value in translations!\n");
            continue;
        }
        translation_z = atof(translation_z_string);

        char rotation_x_string[MAX_CHAR_ARRAY_SIZE];
        strncpy(rotation_x_string, buffer + comma_4_index + 1, comma_5_index - comma_4_index - 1);
        rotation_x_string[comma_5_index - comma_4_index - 1] = '\0';

        float rotation_x;
        if (!string_is_float(rotation_x_string))
        {
            fprintf(stderr, "Found non-float value in translations!\n");
            continue;
        }
        rotation_x = atof(rotation_x_string);

        char rotation_y_string[MAX_CHAR_ARRAY_SIZE];
        strncpy(rotation_y_string, buffer + comma_5_index + 1, comma_6_index - comma_5_index - 1);
        rotation_y_string[comma_6_index - comma_5_index - 1] = '\0';

        float rotation_y;
        if (!string_is_float(rotation_y_string))
        {
            fprintf(stderr, "Found non-float value in translations!\n");
            continue;
        }
        rotation_y = atof(rotation_y_string);

        char rotation_z_string[MAX_CHAR_ARRAY_SIZE];
        strncpy(rotation_z_string, buffer + comma_6_index + 1, strlen(buffer) - comma_6_index - 1);
        rotation_z_string[strlen(buffer) - comma_6_index - 1] = '\0';

        float rotation_z;
        if (!string_is_float(rotation_z_string))
        {
            fprintf(stderr, "Found non-float value in translations!\n");
            continue;
        }
        rotation_z = atof(rotation_z_string);

        if (application->transformation_references != NULL)
        {
            transformation_reference_t* transformation_reference = &application->transformation_references[application->transformation_references_size];
            strcpy(transformation_reference->label, label);
            vec3_set(&transformation_reference->translation, translation_x, translation_y, translation_z);
            vec3_set(&transformation_reference->rotation, rotation_x, rotation_y, rotation_z);
        }

        ++application->transformation_references_size;
    }
}

int application_load_route_map(application_t* application)
{
    char full_file_path[MAX_CHAR_ARRAY_SIZE];
    sprintf(full_file_path, "%s/route1.map", application->full_route_path);

    FILE* file = fopen(full_file_path, "r");
    if (file == NULL)
    {
        fprintf(stderr, "Failed to open file \"%s\" for reading!\n", full_file_path);
        return 0;
    }

    parse_route_map(application, file);

    application->transformation_references = malloc(application->transformation_references_size * sizeof(transformation_reference_t));
    if (application->transformation_references == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for transformation_references!\n");
        fclose(file);
        file = NULL;
        return 0;
    }

    application->transformation_references_size = 0;
    fseek(file, 0, SEEK_SET);

    parse_route_map(application, file);

    fclose(file);
    file = NULL;

    return 1;
}

// TODO: finish matrix
int application_get_node_references(application_t* application)
{
    application->node_references = malloc(application->transformation_references_size * sizeof(node_reference_t));
    if (application->node_references == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for node_references!\n");
        return 0;
    }

    for (uint32_t i = 0; i < application->transformation_references_size; ++i)
    {
        const transformation_reference_t* transformation_reference = &application->transformation_references[i];
        const char* transformation_reference_label = transformation_reference->label;

        int found_mesh = 0;
        uint32_t found_mesh_index;
        for (uint32_t j = 0; j < application->mesh_references_size; ++j)
        {
            if (strcmp(transformation_reference_label, application->mesh_references[j].label) == 0)
            {
                found_mesh = 1;
                found_mesh_index = j;
                break;
            }
        }

        if (!found_mesh)
        {
            continue;
        }

        int found_node = 0;
        for (uint32_t j = 0; j < application->node_references_size; ++j)
        {
            if (strcmp(transformation_reference_label, application->node_references[j].label) == 0)
            {
                found_node = 1;
                break;
            }
        }

        if (!found_node)
        {
            node_reference_t* node_reference = &application->node_references[application->node_references_size];
            strcpy(node_reference->label, transformation_reference_label);
            node_reference->mesh_index = found_mesh_index;

            const vec3_t* translation = &transformation_reference->translation;
            const vec3_t* rotation = &transformation_reference->rotation;

            mat4_t rotation_x_matrix;
            mat4_create_identity_matrix(rotation_x_matrix);
            mat4_create_rotation_x_matrix(rotation->y, rotation_x_matrix);

            mat4_t rotation_y_matrix;
            mat4_create_identity_matrix(rotation_y_matrix);
            mat4_create_rotation_y_matrix(rotation->y, rotation_y_matrix);

            mat4_t rotation_z_matrix;
            mat4_create_identity_matrix(rotation_z_matrix);
            mat4_create_rotation_y_matrix(rotation->z, rotation_z_matrix);

            mat4_t rotation_xy_matrix;
            mat4_multiplicate(rotation_x_matrix, rotation_y_matrix, rotation_xy_matrix);

            mat4_t rotation_matrix;
            mat4_multiplicate(rotation_xy_matrix, rotation_z_matrix, rotation_matrix);

            mat4_t translation_matrix;
            mat4_create_identity_matrix(translation_matrix);
            mat4_create_translation_matrix(translation->x, translation->y, translation->z, translation_matrix);

            mat4_multiplicate(translation_matrix, rotation_matrix, node_reference->matrix);

            ++application->node_references_size;
        }
    }

    free(application->transformation_references);
    application->transformation_references = NULL;

    return 1;
}

int application_fill_gltf_data(application_t* application)
{
    application->gltf_asset.version = "2.0";

    sprintf(application->gltf_buffer.uri, "%s.bin", application->route_name);

    uint32_t total_vertices_size = 0;
    uint32_t total_indices_size = 0;
    uint32_t total_byte_length = 0;

    for (uint32_t i = 0; i < application->geometry_references_size; ++i)
    {
        const geometry_t* geometry = application->geometry_references[i].geometry;

        total_vertices_size += geometry->vertices_size;
        total_indices_size += geometry->indices_size;
        total_byte_length += geometry->vertices_size * (sizeof(vec3_t) + sizeof(vec2_t));
        total_byte_length += geometry->indices_size * sizeof(uint32_t);
    }
    application->gltf_buffer.byteLength = total_byte_length;

    application->gltf_bufferViews[0].buffer = 0;
    application->gltf_bufferViews[0].byteOffset = 0;
    application->gltf_bufferViews[0].byteLength = total_vertices_size * sizeof(vec3_t);
    application->gltf_bufferViews[0].byteStride = sizeof(vec3_t) + sizeof(vec2_t);
    application->gltf_bufferViews[0].target = GLTF_BUFFER_VIEW_TARGET_ARRAY_BUFFER;

    application->gltf_bufferViews[1].buffer = 0;
    application->gltf_bufferViews[1].byteOffset = sizeof(vec3_t);
    application->gltf_bufferViews[1].byteLength = total_vertices_size * sizeof(vec2_t);
    application->gltf_bufferViews[1].byteStride = sizeof(vec3_t) + sizeof(vec2_t);
    application->gltf_bufferViews[1].target = GLTF_BUFFER_VIEW_TARGET_ARRAY_BUFFER;

    application->gltf_bufferViews[2].buffer = 0;
    application->gltf_bufferViews[2].byteOffset = total_vertices_size * (sizeof(vec3_t) + sizeof(vec2_t));
    application->gltf_bufferViews[2].byteLength = total_indices_size * sizeof(uint32_t);
    application->gltf_bufferViews[2].byteStride = 0;
    application->gltf_bufferViews[2].target = GLTF_BUFFER_VIEW_TARGET_ELEMENT_ARRAY_BUFFER;

    application->gltf_accessors = malloc(application->geometry_references_size * 3 * sizeof(gltf_accessor_t));
    if (application->gltf_accessors == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for gltf_accessors!\n");
        return 0;
    }

    total_vertices_size = 0;
    total_indices_size = 0;

    for (uint32_t i = 0; i < application->geometry_references_size; ++i)
    {
        const geometry_t* geometry = application->geometry_references[i].geometry;
        
        gltf_accessor_t* position_accessor = &application->gltf_accessors[i * 3];
        gltf_accessor_t* tex_coords_accessor = &application->gltf_accessors[i * 3 + 1];
        gltf_accessor_t* indices_accessor = &application->gltf_accessors[i * 3 + 2];

        vec3_t max_pos_values = geometry->vertices[0].pos;
        vec3_t min_pos_values = geometry->vertices[0].pos;
        vec2_t max_tex_coord_values = geometry->vertices[0].tex_coord;
        vec2_t min_tex_coord_values = geometry->vertices[0].tex_coord;

        for (uint32_t j = 1; j < geometry->vertices_size; ++j)
        {
            if (geometry->vertices[j].pos.x > max_pos_values.x)
            {
                max_pos_values.x = geometry->vertices[j].pos.x;
            }

            if (geometry->vertices[j].pos.y > max_pos_values.y)
            {
                max_pos_values.y = geometry->vertices[j].pos.y;
            }

            if (geometry->vertices[j].pos.z > max_pos_values.z)
            {
                max_pos_values.z = geometry->vertices[j].pos.z;
            }

            if (geometry->vertices[j].pos.x < min_pos_values.x)
            {
                min_pos_values.x = geometry->vertices[j].pos.x;
            }

            if (geometry->vertices[j].pos.y < max_pos_values.y)
            {
                min_pos_values.y = geometry->vertices[j].pos.y;
            }

            if (geometry->vertices[j].pos.z < max_pos_values.z)
            {
                min_pos_values.z = geometry->vertices[j].pos.z;
            }

            if (geometry->vertices[j].tex_coord.x > max_tex_coord_values.x)
            {
                max_tex_coord_values.x = geometry->vertices[j].tex_coord.x;
            }

            if (geometry->vertices[j].tex_coord.y > max_tex_coord_values.y)
            {
                max_tex_coord_values.y = geometry->vertices[j].tex_coord.y;
            }

            if (geometry->vertices[j].tex_coord.x < min_tex_coord_values.x)
            {
                min_tex_coord_values.x = geometry->vertices[j].tex_coord.x;
            }

            if (geometry->vertices[j].tex_coord.y < min_tex_coord_values.y)
            {
                min_tex_coord_values.y = geometry->vertices[j].tex_coord.y;
            }
        }

        position_accessor->bufferView = 0;
        position_accessor->byteOffset = total_vertices_size * sizeof(vec3_t);
        position_accessor->componentType = GLTF_ACCESSOR_COMPONENT_TYPE_FLOAT;
        position_accessor->count = geometry->vertices_size;
        position_accessor->type = GLTF_ACCESSOR_TYPE_VEC3;
        position_accessor->max_floats[0] = max_pos_values.x;
        position_accessor->max_floats[1] = max_pos_values.y;
        position_accessor->max_floats[2] = max_pos_values.z;
        position_accessor->min_floats[0] = min_pos_values.x;
        position_accessor->min_floats[1] = min_pos_values.y;
        position_accessor->min_floats[2] = min_pos_values.z;

        tex_coords_accessor->bufferView = 1;
        tex_coords_accessor->byteOffset = total_vertices_size * sizeof(vec2_t);
        tex_coords_accessor->componentType = GLTF_ACCESSOR_COMPONENT_TYPE_FLOAT;
        tex_coords_accessor->count = geometry->vertices_size;
        tex_coords_accessor->type = GLTF_ACCESSOR_TYPE_VEC2;
        tex_coords_accessor->max_floats[0] = max_tex_coord_values.x;
        tex_coords_accessor->max_floats[1] = max_tex_coord_values.y;
        tex_coords_accessor->min_floats[0] = min_tex_coord_values.x;
        tex_coords_accessor->min_floats[1] = min_tex_coord_values.y;

        indices_accessor->bufferView = 2;
        indices_accessor->byteOffset = total_indices_size * sizeof(uint32_t);
        indices_accessor->componentType = GLTF_ACCESSOR_COMPONENT_TYPE_UNSIGNED_INT;
        indices_accessor->count = geometry->indices_size;
        indices_accessor->type = GLTF_ACCESSOR_TYPE_SCALAR;
        indices_accessor->max_integer = geometry->indices_size - 1;
        indices_accessor->min_integer = 0;

        total_vertices_size += geometry->vertices_size;
        total_indices_size += geometry->indices_size;
    }

    application->gltf_images = malloc(application->valid_relative_texture_paths_size * sizeof(gltf_image_t));
    if (application->gltf_images == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for gltf_images!\n");
        return 0;
    }

    for (uint32_t i = 0; i < application->valid_relative_texture_paths_size; ++i)
    {
        strncpy(application->gltf_images[i].uri, application->valid_relative_texture_paths[i] + 1, strlen(application->valid_relative_texture_paths[i]) - 1);
        application->gltf_images[i].uri[strlen(application->valid_relative_texture_paths[i]) - 1] = '\0';
        replace_slashes_with_underscores(application->gltf_images[i].uri);
    }

    application->gltf_sampler.magFilter = GLTF_SAMPLER_FILTER_LINEAR;
    application->gltf_sampler.minFilter = GLTF_SAMPLER_FILTER_LINEAR_MIPMAP_LINEAR;
    application->gltf_sampler.wrapS = GLTF_SAMPLER_WRAPPING_MODE_REPEAT;
    application->gltf_sampler.wrapT = GLTF_SAMPLER_WRAPPING_MODE_REPEAT;

    application->gltf_textures = malloc(application->valid_relative_texture_paths_size * sizeof(gltf_texture_t));
    if (application->gltf_textures == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for gltf_textures!\n");
        return 0;
    }

    for (uint32_t i = 0; i < application->valid_relative_texture_paths_size; ++i)
    {
        application->gltf_textures[i].sampler = 0;
        application->gltf_textures[i].source = i;
    }

    application->gltf_materials = malloc(application->valid_relative_texture_paths_size * sizeof(gltf_material_t));
    if (application->gltf_materials == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for gltf_materials!\n");
        return 0;
    }

    for (uint32_t i = 0; i < application->valid_relative_texture_paths_size; ++i)
    {
        application->gltf_materials[i].pbrMetallicRoughness.baseColorTexture.index = i;
        application->gltf_materials[i].pbrMetallicRoughness.baseColorTexture.texCoord = 0;
        application->gltf_materials[i].alphaMode = GLTF_MATERIAL_ALPHA_MODE_MASK;
    }

    application->gltf_meshes = malloc(application->mesh_references_size * sizeof(gltf_mesh_t));
    if (application->gltf_meshes == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for gltf_meshes!\n");
        return 0;
    }

    for (uint32_t i = 0; i < application->mesh_references_size; ++i)
    {
        application->gltf_meshes[i].primitive.attributes.POSITION = application->mesh_references[i].geometry_index * 3;
        application->gltf_meshes[i].primitive.attributes.TEXCOORD_0 = application->mesh_references[i].geometry_index * 3 + 1;
        application->gltf_meshes[i].primitive.indices = application->mesh_references[i].geometry_index * 3 + 2;
        application->gltf_meshes[i].primitive.material = application->mesh_references[i].texture_index;
        application->gltf_meshes[i].primitive.mode = GLTF_MESH_PRIMITIVE_MODE_TRIANGLES;
    }

    application->gltf_nodes = malloc(application->node_references_size * sizeof(gltf_node_t));
    if (application->gltf_nodes == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for gltf_nodes!\n");
        return 0;
    }

    for (uint32_t i = 0; i < application->node_references_size; ++i)
    {
        for (uint32_t j = 0; j < 4; ++j)
        {
            for (uint32_t k = 0; k < 4; ++k)
            {
                application->gltf_nodes[i].matrix[k * 4 + j] = application->node_references[i].matrix[j][k];
            }
        }
        application->gltf_nodes[i].mesh = application->node_references[i].mesh_index;
        strcpy(application->gltf_nodes[i].name, application->node_references[i].label);
    }

    return 1;
}

int application_generate_gltf_files(application_t* application)
{
    const char* full_gltf_route_path = GLTF_ROUTE_DIR;

    char full_gltf_file_path[MAX_CHAR_ARRAY_SIZE];
    sprintf(full_gltf_file_path, "%s/%s.gltf", full_gltf_route_path, application->route_name);

    char full_bin_file_path[MAX_CHAR_ARRAY_SIZE];
    sprintf(full_bin_file_path, "%s/%s.bin", full_gltf_route_path, application->route_name);

    FILE* bin_file = fopen(full_bin_file_path, "wb");
    if (bin_file == NULL)
    {
        fprintf(stderr, "Failed to open .bin file for writing!\n");
        return 0;
    }

    for (uint32_t i = 0; i < application->geometry_references_size; ++i)
    {
        const geometry_t* geometry = application->geometry_references[i].geometry;

        fwrite(geometry->vertices, sizeof(vertex_t), geometry->vertices_size, bin_file);
    }

    for (uint32_t i = 0; i < application->geometry_references_size; ++i)
    {
        const geometry_t* geometry = application->geometry_references[i].geometry;

        fwrite(geometry->indices, sizeof(uint32_t), geometry->indices_size, bin_file);
    }

    fclose(bin_file);
    bin_file = NULL;

    // for (uint32_t i = 0; i < application->valid_relative_texture_paths_size; ++i)
    // {
    //     char full_dmd_texture_path[MAX_CHAR_ARRAY_SIZE];
    //     sprintf(full_dmd_texture_path, "%s%s", application->full_route_path, application->valid_relative_texture_paths[i]);

    //     char relative_texture_path[MAX_CHAR_ARRAY_SIZE];
    //     strcpy(relative_texture_path, application->valid_relative_texture_paths[i]);
    //     replace_slashes_with_underscores(relative_texture_path);
    //     relative_texture_path[0] = '/';

    //     char full_gltf_texture_path[MAX_CHAR_ARRAY_SIZE];
    //     sprintf(full_gltf_texture_path, GLTF_ROUTE_DIR "%s", relative_texture_path);

    //     FILE* dmd_texture_file = fopen(full_dmd_texture_path, "rb");
    //     if (dmd_texture_file == NULL)
    //     {
    //         fprintf(stderr, "Failed to open file \"%s\" for reading!\n", full_dmd_texture_path);
    //         return 0;
    //     }

    //     fseek(dmd_texture_file, 0, SEEK_END);
    //     uint32_t byte_length = ftell(dmd_texture_file);

    //     fseek(dmd_texture_file, 0, SEEK_SET);

    //     FILE* gltf_texture_file = fopen(full_gltf_texture_path, "wb");
    //     if (gltf_texture_file == NULL)
    //     {
    //         fprintf(stderr, "Failed to open file \"%s\" for writing!\n", full_gltf_texture_path);
    //         fclose(dmd_texture_file);
    //         dmd_texture_file = NULL;
    //         return 0;
    //     }

    //     char* buffer = malloc(byte_length);
    //     if (buffer == NULL)
    //     {
    //         fprintf(stderr, "Failed to allocate memory for buffer!\n");
    //         return 0;
    //     }

    //     fread(buffer, 1, byte_length, dmd_texture_file);
    //     fwrite(buffer, 1, byte_length, gltf_texture_file);

    //     fclose(dmd_texture_file);
    //     dmd_texture_file = NULL;

    //     fclose(gltf_texture_file);
    //     gltf_texture_file = NULL;
    // }

    FILE* gltf_file = fopen(full_gltf_file_path, "w");
    if (gltf_file == NULL)
    {
        fprintf(stderr, "Failed to open .gltf file for writing!\n");
        return 0;
    }

    fprintf(
        gltf_file,
        "{\n"
        "    \"asset\": {\n"
        "        \"version\": \"%s\"\n"
        "    },\n",
        application->gltf_asset.version
    );

    fprintf(
        gltf_file,
        "    \"buffers\": [\n"
        "        {\n"
        "            \"byteLength\": %u,\n"
        "            \"uri\": \"%s\"\n"
        "        }\n"
        "    ],\n",
        application->gltf_buffer.byteLength,
        application->gltf_buffer.uri
    );

    fprintf(
        gltf_file,
        "    \"bufferViews\": [\n"
        "        {\n"
        "            \"buffer\": %u,\n"
        "            \"byteLength\": %u,\n"
        "            \"byteOffset\": %u,\n"
        "            \"byteStride\": %u,\n"
        "            \"target\": %u\n"
        "        },\n"
        "        {\n"
        "            \"buffer\": %u,\n"
        "            \"byteLength\": %u,\n"
        "            \"byteOffset\": %u,\n"
        "            \"byteStride\": %u,\n"
        "            \"target\": %u\n"
        "        },\n"
        "        {\n"
        "            \"buffer\": %u,\n"
        "            \"byteLength\": %u,\n"
        "            \"byteOffset\": %u,\n"
        "            \"target\": %u\n"
        "        }\n"
        "    ],\n",
        application->gltf_bufferViews[0].buffer,
        application->gltf_bufferViews[0].byteLength,
        application->gltf_bufferViews[0].byteOffset,
        application->gltf_bufferViews[0].byteStride,
        application->gltf_bufferViews[0].target,
        application->gltf_bufferViews[1].buffer,
        application->gltf_bufferViews[1].byteLength,
        application->gltf_bufferViews[1].byteOffset,
        application->gltf_bufferViews[1].byteStride,
        application->gltf_bufferViews[1].target,
        application->gltf_bufferViews[2].buffer,
        application->gltf_bufferViews[2].byteLength,
        application->gltf_bufferViews[2].byteOffset,
        application->gltf_bufferViews[2].target
    );

    fprintf(
        gltf_file,
        "    \"accessors\": [\n"
    );

    for (uint32_t i = 0; i < application->geometry_references_size; ++i)
    {
        fprintf(
            gltf_file,
            "        {\n"
            "            \"bufferView\": %u,\n"
            "            \"byteOffset\": %u,\n"
            "            \"componentType\": %u,\n"
            "            \"count\": %u,\n"
            "            \"max\": [\n"
            "                %f,\n"
            "                %f,\n"
            "                %f\n"
            "            ],\n"
            "            \"min\": [\n"
            "                %f,\n"
            "                %f,\n"
            "                %f\n"
            "            ],\n"
            "            \"type\": \"%s\"\n"
            "        },\n"
            "        {\n"
            "            \"bufferView\": %u,\n"
            "            \"byteOffset\": %u,\n"
            "            \"componentType\": %u,\n"
            "            \"count\": %u,\n"
            "            \"max\": [\n"
            "                %f,\n"
            "                %f\n"
            "            ],\n"
            "            \"min\": [\n"
            "                %f,\n"
            "                %f\n"
            "            ],\n"
            "            \"type\": \"%s\"\n"
            "        },\n"
            "        {\n"
            "            \"bufferView\": %u,\n"
            "            \"byteOffset\": %u,\n"
            "            \"componentType\": %u,\n"
            "            \"count\": %u,\n"
            "            \"max\": [\n"
            "                %u\n"
            "            ],\n"
            "            \"min\": [\n"
            "                %u\n"
            "            ],\n"
            "            \"type\": \"%s\"\n"
            "        }%s",
            application->gltf_accessors[i * 3].bufferView,
            application->gltf_accessors[i * 3].byteOffset,
            application->gltf_accessors[i * 3].componentType,
            application->gltf_accessors[i * 3].count,
            application->gltf_accessors[i * 3].max_floats[0],
            application->gltf_accessors[i * 3].max_floats[1],
            application->gltf_accessors[i * 3].max_floats[2],
            application->gltf_accessors[i * 3].min_floats[0],
            application->gltf_accessors[i * 3].min_floats[1],
            application->gltf_accessors[i * 3].min_floats[2],
            application->gltf_accessors[i * 3].type,
            application->gltf_accessors[i * 3 + 1].bufferView,
            application->gltf_accessors[i * 3 + 1].byteOffset,
            application->gltf_accessors[i * 3 + 1].componentType,
            application->gltf_accessors[i * 3 + 1].count,
            application->gltf_accessors[i * 3 + 1].max_floats[0],
            application->gltf_accessors[i * 3 + 1].max_floats[1],
            application->gltf_accessors[i * 3 + 1].min_floats[0],
            application->gltf_accessors[i * 3 + 1].min_floats[1],
            application->gltf_accessors[i * 3 + 1].type,
            application->gltf_accessors[i * 3 + 2].bufferView,
            application->gltf_accessors[i * 3 + 2].byteOffset,
            application->gltf_accessors[i * 3 + 2].componentType,
            application->gltf_accessors[i * 3 + 2].count,
            application->gltf_accessors[i * 3 + 2].max_integer,
            application->gltf_accessors[i * 3 + 2].min_integer,
            application->gltf_accessors[i * 3 + 2].type,
            (i == application->geometry_references_size - 1) ? "\n" : ",\n"
        );
    }

    fprintf(
        gltf_file,
        "    ],\n"
        "    \"images\": [\n"
    );

    for (uint32_t i = 0; i < application->valid_relative_texture_paths_size; ++i)
    {
        fprintf(
            gltf_file,
            "        {\n"
            "            \"uri\": \"%s\"\n"
            "        }%s",
            application->gltf_images[i].uri,
            (i == application->valid_relative_texture_paths_size - 1) ? "\n" : ",\n"
        );
    }

    fprintf(
        gltf_file,
        "    ],\n"
        "    \"samplers\": [\n"
        "        {\n"
        "            \"magFilter\": %u,\n"
        "            \"minFilter\": %u,\n"
        "            \"wrapS\": %u,\n"
        "            \"wrapT\": %u\n"
        "        }\n"
        "    ],\n",
        application->gltf_sampler.magFilter,
        application->gltf_sampler.minFilter,
        application->gltf_sampler.wrapS,
        application->gltf_sampler.wrapT
    );

    fprintf(
        gltf_file,
        "    \"textures\": [\n"
    );

    for (uint32_t i = 0; i < application->valid_relative_texture_paths_size; ++i)
    {
        fprintf(
            gltf_file,
            "        {\n"
            "            \"sampler\": 0,\n"
            "            \"source\": %u\n"
            "        }%s",
            i,
            (i == application->valid_relative_texture_paths_size - 1) ? "\n" : ",\n"
        );
    }

    fprintf(
        gltf_file,
        "    ],\n"
        "    \"materials\": [\n"
    );

    for (uint32_t i = 0; i < application->valid_relative_texture_paths_size; ++i)
    {
        fprintf(
            gltf_file,
            "        {\n"
            "            \"pbrMetallicRoughness\": {\n"
            "                \"baseColorTexture\": {\n"
            "                    \"index\": %u,\n"
            "                    \"texCoord\": 0\n"
            "                }\n"
            "            }\n"
            "        }%s",
            i,
            (i == application->valid_relative_texture_paths_size - 1) ? "\n" : ",\n"
        );
    }

    fprintf(
        gltf_file,
        "    ],\n"
        "    \"meshes\": [\n"
    );

    for (uint32_t i = 0; i < application->mesh_references_size; ++i)
    {
        fprintf(
            gltf_file,
            "        {\n"
            "            \"primitives\": [\n"
            "                {\n"
            "                    \"attributes\": {\n"
            "                        \"POSITION\": %u,\n"
            "                        \"TEXCOORD_0\": %u\n"
            "                    },\n"
            "                    \"indices\": %u,\n"
            "                    \"material\": %u,\n"
            "                    \"mode\": %u\n"
            "                }\n"
            "            ]\n"
            "        }%s",
            application->gltf_meshes[i].primitive.attributes.POSITION,
            application->gltf_meshes[i].primitive.attributes.TEXCOORD_0,
            application->gltf_meshes[i].primitive.indices,
            application->gltf_meshes[i].primitive.material,
            application->gltf_meshes[i].primitive.mode,
            (i == application->mesh_references_size - 1) ? "\n" : ",\n"
        );
    }

    fprintf(
        gltf_file,
        "    ],\n"
        "    \"nodes\": [\n"
    );

    for (uint32_t i = 0; i < application->node_references_size; ++i)
    {
        fprintf(
            gltf_file,
            "        {\n"
            "            \"name\": \"%s\",\n"
            "            \"mesh\": %u,\n"
            "            \"matrix\": [\n"
            "                %f,\n"
            "                %f,\n"
            "                %f,\n"
            "                %f,\n"
            "                %f,\n"
            "                %f,\n"
            "                %f,\n"
            "                %f,\n"
            "                %f,\n"
            "                %f,\n"
            "                %f,\n"
            "                %f,\n"
            "                %f,\n"
            "                %f,\n"
            "                %f,\n"
            "                %f\n"
            "            ]\n"
            "        }%s",
            application->gltf_nodes[i].name,
            application->gltf_nodes[i].mesh,
            application->gltf_nodes[i].matrix[0],
            application->gltf_nodes[i].matrix[1],
            application->gltf_nodes[i].matrix[2],
            application->gltf_nodes[i].matrix[3],
            application->gltf_nodes[i].matrix[4],
            application->gltf_nodes[i].matrix[5],
            application->gltf_nodes[i].matrix[6],
            application->gltf_nodes[i].matrix[7],
            application->gltf_nodes[i].matrix[8],
            application->gltf_nodes[i].matrix[9],
            application->gltf_nodes[i].matrix[10],
            application->gltf_nodes[i].matrix[11],
            application->gltf_nodes[i].matrix[12],
            application->gltf_nodes[i].matrix[13],
            application->gltf_nodes[i].matrix[14],
            application->gltf_nodes[i].matrix[15],
            (i == application->node_references_size - 1) ? "\n" : ",\n"
        );
    }

    fprintf(
        gltf_file,
        "    ],\n"
        "    \"scenes\": [\n"
        "        {\n"
        "            \"name\": \"scene\",\n"
        "            \"nodes\": [\n"
    );

    for (uint32_t i = 0; i < application->node_references_size; ++i)
    {
        fprintf(gltf_file, "                %u%s", i, (i == application->node_references_size - 1) ? "\n" : ",\n");
    }

    fprintf(
        gltf_file,
        "            ]\n"
        "        }\n"
        "    ],\n"
        "    \"scene\": 0\n"
        "}\n"
    );

    return 1;
}
