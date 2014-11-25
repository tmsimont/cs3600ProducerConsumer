/**
 *
 */

#include "server.h"


ResourceBuffer *resource_buffer_new(int bufferSize) {
    ResourceBuffer *rb = malloc(sizeof(*rb));
    rb->count = 0;
    rb->size = bufferSize;
    return rb;
}

Resource *resource_new(int i) {
    Resource *r = malloc(sizeof(*r));
    r->produced_by = i;
    r->id = ridx++;
    r->next = NULL;
    return r;
}

int resource_buffer_enqueue(ResourceBuffer *rb, Resource *r) {
    if (rb->count == 0) {
        rb->head = r;
        rb->count++;
        if (debug.print) printf("enqueued r%d (count=%d)\n", r->id, rb->count);
        monitor_push_reports();
        return 0;
    }
    else if (rb->count < rb->size) {
        Resource *temp = rb->head;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = r;
        rb->count++;
        if (debug.print) printf("enqueued r%d (count=%d)\n", r->id, rb->count);
        monitor_push_reports();
        return 0;
    }
    else {
        if (debug.print) printf("refusing to enqueue r%d\n", r->id);
        return -1;
    }
}

int resource_buffer_dequeue(ResourceBuffer *rb, Resource **r) {
    if (rb->count == 0) {
        return -1;
    }
    else {
        if (debug.print) printf("setting *r to head\n");
        *r = rb->head;
        if (rb->count > 1) {
            rb->head = rb->head->next;
        }
        else {
            rb->head = NULL;
        }
    }
    rb->count--;

    if (debug.print) printf("about to dereference r in dequeue\n");
    if (debug.print) printf("r%d dequeued (count = %d).\n", (*r)->id, rb->count);
    
    monitor_push_reports();
    return 0;
}

void resource_buffer_print(ResourceBuffer *rb) {
    printf("----buffer (%2d)----\n", rb->count);
    if (rb->count == 0) {
        printf("empty\n");
    }
    else {
        Resource *temp = rb->head;
        while (temp != NULL) {
            printf("r%d in buffer\n", temp->id);
            temp = temp->next;
        }
    }
    printf("-------------------\n");
}

void resource_buffer_test(ResourceBuffer *rb) {
    Resource *r;

    resource_buffer_enqueue(rb, resource_new(0));
    resource_buffer_enqueue(rb, resource_new(1));
    resource_buffer_enqueue(rb, resource_new(2));
    resource_buffer_enqueue(rb, resource_new(10));
    if (debug.print) resource_buffer_print(rb);

    resource_buffer_dequeue(rb, &r);
    if (debug.print) resource_buffer_print(rb);

    resource_buffer_enqueue(rb, resource_new(3));
    resource_buffer_enqueue(rb, resource_new(11));
    if (debug.print) resource_buffer_print(rb);

    resource_buffer_dequeue(rb, &r);
    resource_buffer_dequeue(rb, &r);
    resource_buffer_dequeue(rb, &r);
    resource_buffer_dequeue(rb, &r);
    resource_buffer_dequeue(rb, &r);
    resource_buffer_dequeue(rb, &r);
    if (debug.print) resource_buffer_print(rb);

    resource_buffer_enqueue(rb, resource_new(4));
    resource_buffer_enqueue(rb, resource_new(5));
    resource_buffer_enqueue(rb, resource_new(6));
    resource_buffer_enqueue(rb, resource_new(12));
    resource_buffer_enqueue(rb, resource_new(13));
    if (debug.print) resource_buffer_print(rb);
    
}