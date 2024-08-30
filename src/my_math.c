#include "my_math.h"

#include <stdlib.h>

void vec3_set(vec3_t* vec, float x, float y, float z)
{
    vec->x = x;
    vec->y = y;
    vec->z = z;
}

int vertex_equal(const vertex_t* vertex_1, const vertex_t* vertex_2)
{
    return vertex_1->pos.x == vertex_2->pos.x
        && vertex_1->pos.y == vertex_2->pos.y
        && vertex_1->pos.z == vertex_2->pos.z
        && vertex_1->tex_coord.x == vertex_2->tex_coord.x
        && vertex_1->tex_coord.x == vertex_2->tex_coord.y;
}

void geometry_destroy(geometry_t* geometry)
{
    free(geometry->indices);
    geometry->indices = NULL;

    free(geometry->vertices);
    geometry->vertices = NULL;
}
