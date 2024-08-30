#include "geometry_reference.h"

#include <stdlib.h>

void geometry_reference_destroy(geometry_reference_t* geometry_reference)
{
    if (geometry_reference->geometry != NULL)
    {
        geometry_destroy(geometry_reference->geometry);
        free(geometry_reference->geometry);
        geometry_reference->geometry = NULL;
    }
}
