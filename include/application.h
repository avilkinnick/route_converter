#pragma once

#include <stdint.h>

#include "defines.h"

#include "geometry_reference.h"
#include "mesh_reference.h"
#include "node_reference.h"
#include "object_reference.h"
#include "transformation_reference.h"

#include "gltf_types/accessor.h"
#include "gltf_types/asset.h"
#include "gltf_types/buffer.h"
#include "gltf_types/bufferView.h"
#include "gltf_types/image.h"
#include "gltf_types/material.h"
#include "gltf_types/mesh.h"
#include "gltf_types/node.h"
#include "gltf_types/sampler.h"
#include "gltf_types/scene.h"
#include "gltf_types/texture.h"

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
    mesh_reference_t* mesh_references;
    uint32_t mesh_references_size;

    transformation_reference_t* transformation_references;
    uint32_t transformation_references_size;
    node_reference_t* node_references;
    uint32_t node_references_size;

    gltf_asset_t gltf_asset;
    gltf_buffer_t gltf_buffer;
    gltf_bufferView_t gltf_bufferViews[3];
    gltf_accessor_t* gltf_accessors;
    gltf_image_t* gltf_images;
    gltf_sampler_t gltf_sampler;
    gltf_texture_t* gltf_textures;
    gltf_material_t* gltf_materials;
    gltf_mesh_t* gltf_meshes;
    gltf_node_t* gltf_nodes;
} application_t;

void application_destroy(application_t* application);

int application_parse_arguments(application_t* application, int argc, char* argv[]);

int application_load_objects_ref(application_t* application);

int application_get_unique_relative_model_paths(application_t* application);

int application_load_models(application_t* application);

int application_get_unique_relative_texture_paths(application_t* application);

int application_check_texture_files_existence(application_t* application);

int application_remove_redundant_object_references(application_t* application);

int application_load_route_map(application_t* application);

int application_get_node_references(application_t* application);

int application_fill_gltf_data(application_t* application);

int application_generate_gltf_files(application_t* application);
