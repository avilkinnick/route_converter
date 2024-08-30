#pragma once

#include <stdint.h>

typedef struct vec2
{
    float x;
    float y;
} vec2_t;

typedef struct vec3
{
    float x;
    float y;
    float z;
} vec3_t;

void vec3_set(vec3_t* vec, float x, float y, float z);

typedef float mat4_t[4][4];

void mat4_create_identity_matrix(mat4_t out);

void mat4_create_translation_matrix(float x, float y, float z, mat4_t out);

void mat4_create_rotation_x_matrix(float rad, mat4_t out);

void mat4_create_rotation_y_matrix(float rad, mat4_t out);

void mat4_create_rotation_z_matrix(float rad, mat4_t out);

void mat4_multiplicate(mat4_t left, mat4_t right, mat4_t out);

typedef struct vertex
{
    vec3_t pos;
    vec2_t tex_coord;
} vertex_t;

int vertex_equal(const vertex_t* vertex_1, const vertex_t* vertex_2);

typedef struct geometry
{
    vertex_t* vertices;
    uint32_t vertices_size;
    uint32_t* indices;
    uint32_t indices_size;
} geometry_t;

void geometry_destroy(geometry_t* geometry);
