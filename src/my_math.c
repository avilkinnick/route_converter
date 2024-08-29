#include "my_math.h"

#include <stdlib.h>

void geometry_destroy(geometry_t* geometry)
{
    free(geometry->indices);
    geometry->indices = NULL;

    free(geometry->vertices);
    geometry->vertices = NULL;
}
