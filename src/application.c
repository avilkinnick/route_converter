#include "application.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmake_defines.h"

#include "mesh_reference.h"
#include "my_math.h"
#include "object_reference.h"

static int first_char_is_slash(const char* string)
{
    return (string[0] == '/') || (string[0] == '\\');
}

static int string_is_float(const char* string)
{
    for (uint32_t i = 0; i < strlen(string); ++i)
    {
        char ch = string[i];
        if ((ch < '0' || ch > '9') && ch != '.' && ch != '-' && ch != 'e')
        {
            return 0;
        }
    }

    return 1;
}

void application_destroy(application_t* application)
{
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
