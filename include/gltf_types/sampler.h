#pragma once

typedef enum gltf_sampler_filter
{
    GLTF_SAMPLER_FILTER_NEAREST = 9728,
    GLTF_SAMPLER_FILTER_LINEAR = 9729,
    GLTF_SAMPLER_FILTER_NEAREST_MIPMAP_NEAREST = 9984,
    GLTF_SAMPLER_FILTER_LINEAR_MIPMAP_NEAREST = 9985,
    GLTF_SAMPLER_FILTER_NEAREST_MIPMAP_LINEAR = 9986,
    GLTF_SAMPLER_FILTER_LINEAR_MIPMAP_LINEAR = 9987
} gltf_sampler_filter_t;

typedef enum gltf_sampler_wrapping_mode
{
    GLTF_SAMPLER_WRAPPING_MODE_CLAMP_TO_EDGE = 33071,
    GLTF_SAMPLER_WRAPPING_MODE_MIRRORED_REPEAT = 33648,
    GLTF_SAMPLER_WRAPPING_MODE_REPEAT = 10497
} gltf_sampler_wrapping_mode_t;

typedef struct gltf_sampler
{
    gltf_sampler_filter_t magFilter;
    gltf_sampler_filter_t minFilter;
    gltf_sampler_wrapping_mode_t wrapS;
    gltf_sampler_wrapping_mode_t wrapT;
} gltf_sampler_t;
