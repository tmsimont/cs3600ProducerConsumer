/* 
 * File:   producer.c
 * Author: Trevor Simonton
 *
 * Created on November 17, 2014, 5:06 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include "server.h"

struct _Producer {
    int status;
};


Producer *producer_new() {
    Producer *p = malloc(sizeof(*p));
    return p;
}

void producer_produce(Producer *p) {
    printf("produce\n");
}