#include "gltf_types/scene.h"

#include <stdlib.h>

void gltf_scene_destroy(gltf_scene_t* gltf_scene)
{
    free(gltf_scene->nodes);
    gltf_scene->nodes = NULL;
}
