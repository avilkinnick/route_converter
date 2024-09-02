#include <stdio.h>
#include <string.h>

#include "application.h"

int main(int argc, char* argv[])
{
    application_t application;
    memset(&application, 0, sizeof(application_t));

    if (!application_parse_arguments(&application, argc, argv))
    {
        fprintf(stderr, "Failed to parse command-line arguments!\n");
        application_destroy(&application);
        return 1;
    }

    if (!application_load_objects_ref(&application))
    {
        fprintf(stderr, "Failed to load objects.ref!\n");
        application_destroy(&application);
        return 1;
    }

    if (!application_get_unique_relative_model_paths(&application))
    {
        fprintf(stderr, "Failed to get unique relative model paths!\n");
        application_destroy(&application);
        return 1;
    }

    application_load_models(&application);

    if (!application_get_unique_relative_texture_paths(&application))
    {
        fprintf(stderr, "Failed to get unique relative texture paths!\n");
        application_destroy(&application);
        return 1;
    }

    if (!application_check_texture_files_existence(&application))
    {
        fprintf(stderr, "Failed to check texture files existence!\n");
        application_destroy(&application);
        return 1;
    }

    if (!application_remove_redundant_object_references(&application))
    {
        fprintf(stderr, "Failed to remove redundant object references!\n");
        application_destroy(&application);
        return 1;
    }

    if (!application_load_route_map(&application))
    {
        fprintf(stderr, "Failed to load route1.map!\n");
        application_destroy(&application);
        return 1;
    }

    if (!application_get_node_references(&application))
    {
        fprintf(stderr, "Failed to get node references!\n");
        application_destroy(&application);
        return 1;
    }

    application_destroy(&application);

    return 0;
}
