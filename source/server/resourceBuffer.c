/**
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "server.h"

struct _ResourceBuffer {
    int status;
};


Resource *resource_buffer_new() {
    ResourceBuffer *r = malloc(sizeof(*r));
    return r;
}