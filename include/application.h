#pragma once

#include <stdint.h>

#include "defines.h"

#include "geometry_reference.h"
#include "object_reference.h"

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

// TODO: fix destroying of objects
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

    gltf_asset_t gltf_asset;
    gltf_buffer_t gltf_buffer;
    gltf_bufferView_t gltf_bufferViews[3];
    gltf_accessor_t* gltf_accessors;
    uint32_t gltf_accessors_size;
    gltf_image_t* gltf_images;
    uint32_t gltf_images_size;
    gltf_sampler_t gltf_sampler;
    gltf_texture_t* gltf_textures;
    gltf_material_t* gltf_materials;
    gltf_mesh_t* gltf_meshes;
    uint32_t gltf_meshes_size;
    gltf_node_t* gltf_nodes;
    uint32_t gltf_nodes_size;
    gltf_scene_t gltf_scene;
} application_t;

void application_destroy(application_t* application);

int application_parse_arguments(application_t* application, int argc, char* argv[]);

int application_load_objects_ref(application_t* application);

int application_get_unique_relative_model_paths(application_t* application);
