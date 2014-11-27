/**
 * @file
 * Author: Trevor Simonton
 *
 * This head contains struct and function declarations
 * for the Linux server process.
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#define APPLICATION_PORT 60118
#define MAX_PRODUCERS 128

// behavioral settings
int consumeDelay;
int consumerRest;
int produceDelay;
int producerRest;
int bufferSize;
int numProducers;

int reset();


// Resource data
typedef struct _Resource Resource;
struct _Resource {
    int id;
    int produced_by;
    int consumed_by;
    Resource *next;
};
Resource *resource_new(int);
int ridx;


// ResourceBuffer linked list structure
typedef struct _ResourceBuffer ResourceBuffer;
struct _ResourceBuffer {
    int size;
    int count;
    Resource *head;
};
ResourceBuffer *resource_buffer_new(int);
ResourceBuffer *globalResourceBuffer;
void resource_buffer_test(ResourceBuffer*);
void resource_buffer_print(ResourceBuffer*);
int initialize_producers(ResourceBuffer*, int);
pthread_mutex_t bufferMutex;
pthread_cond_t bufferHasRoom;
pthread_cond_t bufferNotEmpty;


// Producer
typedef struct _Producer Producer;
struct _Producer {
    int id;
    int resources_produced;
    pthread_t thread;
    ResourceBuffer *bufferp;
    int status;
};
/**
 * producers helps us track all of our Producer instances
 */ 
Producer *producers[MAX_PRODUCERS];
int pidx;
Producer *producer_new(ResourceBuffer*);


// Environmental variables for various thread arguments
typedef struct _environment Environment;
struct _environment {
    int socket_desc;
    ResourceBuffer *bufferp;
};
Environment *env;


// ConsumerService thread data
typedef struct _ConsumerService ConsumerService;
struct _ConsumerService {
    int id;
    Environment* env;
    int client_sock;
    pthread_t thread;
    int resources_consumed;
    int status;
    ConsumerService *next;
    ConsumerService *prev;
};
// ConsumerService linked list
typedef struct _ConsumerServiceList ConsumerServiceList;
struct _ConsumerServiceList {
    ConsumerService *head;
    ConsumerService *tail;
    int count;
    int idx;
};
/**
 * consumerList helps us track any live consumer connections.
 * This is accessed from multiples threads, and is protected by mutex.
 */ 
ConsumerServiceList *consumerList;
int consumer_service_new(Environment *, int client_socket);
int consumer_service_remove(ConsumerService *);
int consumer_service_get_resource(Environment *, Resource **);
pthread_mutex_t consumerListMutex;


// MonitorService thread data
typedef struct _MonitorService MonitorService;
struct _MonitorService {
    Environment* env;
    int client_sock;
    int ready;
    int id;
    int deleted;
    int waiting;
    pthread_mutex_t monitorReadyMutex;
    pthread_cond_t monitorNowReady;
    pthread_t thread;
    MonitorService *next;
    MonitorService *prev;
};
// MonitorService linked list
typedef struct _MonitorServiceList MonitorServiceList;
struct _MonitorServiceList {
    MonitorService *head;
    MonitorService *tail;
    int count;
    int idx;
};
int monitor_service_new(Environment *, int);
int monitor_service_remove(MonitorService *);
/**
 * monitorList helps us track any live monitor connections.
 * This is accessed from multiples threads, and is protected by mutex.
 */ 
MonitorServiceList *monitorList;
void monitor_push_reports();
pthread_mutex_t monitorListMutex;


// global debugger flag
struct {
    unsigned int print : 1;
} debug;
