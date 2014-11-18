/* 
 * File:   producer.c
 * Author: Trevor Simonton
 *
 * Created on November 17, 2014, 5:06 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include "server.h"

struct _Resource {
    int status;
};

Resource *resource_new() {
    Resource *r = malloc(sizeof(*r));
    return r;
}