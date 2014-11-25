/* 
 * File:   server.h
 * Author: Trevor Simonton
 *
 * Created on November 17, 2014, 5:04 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#define MAX_PRODUCERS 128

int consumeDelay;
int produceDelay;
int monitorPullDelay;

// Resource
typedef struct _Resource Resource;
struct _Resource {
    int id;
    int produced_by;
    int consumed_by;
    Resource *next;
};
Resource *resource_new(int);
int ridx;

// ResourceBuffer
typedef struct _ResourceBuffer ResourceBuffer;
struct _ResourceBuffer {
    int size;
    int count;
    Resource *head;
};
ResourceBuffer *resource_buffer_new(int);
void resource_buffer_test(ResourceBuffer*);
void resource_buffer_print(ResourceBuffer*);
int initialize_producers(ResourceBuffer*, int);

// bufferMutex
pthread_mutex_t bufferMutex;
pthread_cond_t bufferHasRoom;

// Producer
typedef struct _Producer Producer;
struct _Producer {
    int id;
    int resources_produced;
    pthread_t thread;
    ResourceBuffer *bufferp;
    int status;
};
Producer *producers[MAX_PRODUCERS];
int pidx;
Producer *producer_new(ResourceBuffer*);

// Primary server function
typedef struct _environment Environment;
struct _environment {
    int socket_desc;
    ResourceBuffer *bufferp;
};

int consumer_service_new(Environment *, int);

int report_status(void);
int server_listen(Environment*);
int server_close_connection(void);


struct {
    unsigned int print : 1;
} debug;


// helper struct for individual consumer service threads
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
typedef struct _ConsumerServiceList ConsumerServiceList;
struct _ConsumerServiceList {
    ConsumerService *head;
    ConsumerService *tail;
    int count;
    int idx;
};
ConsumerServiceList *consumerList;
int consumer_service_new(Environment *, int client_socket);
int consumer_service_remove(ConsumerService *);
int consumer_service_get_resource(Environment *, Resource **);

// consumerListMutext
pthread_mutex_t consumerListMutex;


// helper struct for individual consumer service threads
typedef struct _MonitorService MonitorService;
struct _MonitorService {
    Environment* env;
    int client_sock;
    int ready;
    int id;
    pthread_mutex_t monitorReadyMutex;
    pthread_cond_t monitorNowReady;
    pthread_t thread;
    MonitorService *next;
    MonitorService *prev;
};
typedef struct _MonitorServiceList MonitorServiceList;
struct _MonitorServiceList {
    MonitorService *head;
    MonitorService *tail;
    int count;
    int idx;
};
int monitor_service_new(Environment *, int);
int monitor_service_remove(MonitorService *);
MonitorServiceList *monitorList;
void monitor_push_reports();

// monitorListMutex
pthread_mutex_t monitorListMutex;


int xml_write_message(int, char *);