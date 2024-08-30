#include "my_math.h"

#include <math.h>
#include <stdlib.h>

void vec3_set(vec3_t* vec, float x, float y, float z)
{
    vec->x = x;
    vec->y = y;
    vec->z = z;
}

void mat4_create_identity_matrix(mat4_t out)
{
    out[0][0] = 1;    out[0][1] = 0;    out[0][2] = 0;    out[0][3] = 0;
    out[1][0] = 0;    out[1][1] = 1;    out[1][2] = 0;    out[1][3] = 0;
    out[2][0] = 0;    out[2][1] = 0;    out[2][2] = 1;    out[2][3] = 0;
    out[3][0] = 0;    out[3][1] = 0;    out[3][2] = 0;    out[3][3] = 1;
}

void mat4_create_translation_matrix(float x, float y, float z, mat4_t out)
{
    mat4_create_identity_matrix(out);
    out[0][3] = x;
    out[1][3] = y;
    out[2][3] = z;
}

void mat4_create_rotation_x_matrix(float rad, mat4_t out)
{
    mat4_create_identity_matrix(out);
    out[1][1] = cosf(rad);
    out[1][2] = -sinf(rad);
    out[2][1] = sinf(rad);
    out[2][2] = cosf(rad);
}

void mat4_create_rotation_y_matrix(float rad, mat4_t out)
{
    mat4_create_identity_matrix(out);
    out[0][0] = cosf(rad);
    out[0][2] = sinf(rad);
    out[2][0] = -sinf(rad);
    out[2][2] = cosf(rad);
}

void mat4_create_rotation_z_matrix(float rad, mat4_t out)
{
    mat4_create_identity_matrix(out);
    out[0][0] = cosf(rad);
    out[0][1] = -sinf(rad);
    out[1][0] = sinf(rad);
    out[1][1] = cosf(rad);
}

void mat4_multiplicate(mat4_t left, mat4_t right, mat4_t out)
{
    out[0][0] = left[0][0] * right[0][0] + left[0][1] * right[1][0] + left[0][2] * right[2][0] + left[0][3] * right[3][0];
    out[0][1] = left[0][0] * right[0][1] + left[0][1] * right[1][1] + left[0][2] * right[2][1] + left[0][3] * right[3][1];
    out[0][2] = left[0][0] * right[0][2] + left[0][1] * right[1][2] + left[0][2] * right[2][2] + left[0][3] * right[3][2];
    out[0][3] = left[0][0] * right[0][3] + left[0][1] * right[1][3] + left[0][2] * right[2][3] + left[0][3] * right[3][3];
    out[1][0] = left[1][0] * right[0][0] + left[1][1] * right[1][0] + left[1][2] * right[2][0] + left[1][3] * right[3][0];
    out[1][1] = left[1][0] * right[0][1] + left[1][1] * right[1][1] + left[1][2] * right[2][1] + left[1][3] * right[3][1];
    out[1][2] = left[1][0] * right[0][2] + left[1][1] * right[1][2] + left[1][2] * right[2][2] + left[1][3] * right[3][2];
    out[1][3] = left[1][0] * right[0][3] + left[1][1] * right[1][3] + left[1][2] * right[2][3] + left[1][3] * right[3][3];
    out[2][0] = left[2][0] * right[0][0] + left[2][1] * right[1][0] + left[2][2] * right[2][0] + left[2][3] * right[3][0];
    out[2][1] = left[2][0] * right[0][1] + left[2][1] * right[1][1] + left[2][2] * right[2][1] + left[2][3] * right[3][1];
    out[2][2] = left[2][0] * right[0][2] + left[2][1] * right[1][2] + left[2][2] * right[2][2] + left[2][3] * right[3][2];
    out[2][3] = left[2][0] * right[0][3] + left[2][1] * right[1][3] + left[2][2] * right[2][3] + left[2][3] * right[3][3];
    out[3][0] = left[3][0] * right[0][0] + left[3][1] * right[1][0] + left[3][2] * right[2][0] + left[3][3] * right[3][0];
    out[3][1] = left[3][0] * right[0][1] + left[3][1] * right[1][1] + left[3][2] * right[2][1] + left[3][3] * right[3][1];
    out[3][2] = left[3][0] * right[0][2] + left[3][1] * right[1][2] + left[3][2] * right[2][2] + left[3][3] * right[3][2];
    out[3][3] = left[3][0] * right[0][3] + left[3][1] * right[1][3] + left[3][2] * right[2][3] + left[3][3] * right[3][3];
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
