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
