#include "server.h"
#include <libxml/parser.h>


int monitor_service_await_and_handle_message(MonitorService*);
void *monitor_service_connection_handler(void *);
void monitor_service_write_report(MonitorService *);

/**
 * Create a new ConsumerService struct, and begin the corresponding thread.
 */
int monitor_service_new(Environment *env, int client_sock) {
    MonitorService *t = malloc(sizeof(*t));
    t->client_sock = client_sock;
    t->env = env;
    t->ready = 0;
    pthread_mutex_init(&(t->monitorReadyMutex), NULL);
    pthread_cond_init(&(t->monitorNowReady), NULL);
    if (debug.print) printf("monitor service struct ready\n");

    if( pthread_create(&(t->thread), NULL, monitor_service_connection_handler, (void*)t) < 0) {
        if (debug.print) printf("could not create monitor service thread\n");
        return -1;
    }

    if (debug.print) puts("monitor service handler assigned");
    return 0;
}

int monitor_service_remove(MonitorService *ms) {

    pthread_mutex_lock(&monitorListMutex);

    // remove from linked list
    if (monitorList->count == 0) {
        if (debug.print) printf("unable to remove MS-%d from empty list (%d)\n", ms->id, monitorList->count);
    }
    else if (monitorList->count == 1) {
        monitorList->head = NULL;
        monitorList->tail = NULL;
        monitorList->count = 0;
        if (debug.print) printf("MS-%d removed from singleton list (%d)\n", ms->id, monitorList->count);
    }
    else {
        if (ms->id == monitorList->head->id) {
            monitorList->head = monitorList->head->next;
            monitorList->head->prev = NULL;
            monitorList->count--;
            if (debug.print) printf("MS-%d removed from head of large list (%d)\n", ms->id, monitorList->count);
        }
        else {
            MonitorService *temp;
            temp = monitorList->head;
            while (temp->next->next != NULL && temp->next->id != ms->id) {
                temp = temp->next;
            }
            if (temp->next->id == ms->id) {
                if (temp->next->next != NULL) {
                    temp->next->next->prev = temp;
                    temp->next = temp->next->next;
                    monitorList->count--;
                    if (debug.print) printf("MS-%d removed large list (%d)\n", ms->id, monitorList->count);
                }
                else {
                    monitorList->tail = temp;
                    temp->next = NULL;
                    monitorList->count--;
                    if (debug.print) printf("MS-%d removed from end of large list (%d)\n", ms->id, monitorList->count);
                }
            }
            else {
                if (debug.print) printf("unable to remove MS-%d from non-empty list (%d)\n", ms->id, monitorList->count);
                // service not in list
            }
        }
    }

    free(ms);
    if (debug.print) printf("monitor struct freed from memory\n");

    pthread_mutex_unlock(&monitorListMutex);

}

void *monitor_push_reports_handler(void *tp) {
    if (debug.print) printf("push reports\n");
    if (monitorList->count == 0) {
        // no monitors
        if (debug.print) printf("no monitors\n");
    }
    else {
        MonitorService *ms = monitorList->head;
        while (ms != NULL) {
            if (debug.print) printf("lock for push\n");
            pthread_mutex_lock(&(ms->monitorReadyMutex));
            if (ms->ready == 1) {
            } else {
                // wait until the monitor is ready
                if (debug.print) printf("wait for monitor\n");
                pthread_cond_wait(&(ms->monitorNowReady), &(ms->monitorReadyMutex));
            }
            ms->ready = 0;
            monitor_service_write_report(ms);
            pthread_mutex_unlock(&(ms->monitorReadyMutex));
            ms = ms->next;
        }
    }
    pthread_exit(NULL);
}

void monitor_push_reports() {
    pthread_t thread_id;
    if( pthread_create(&thread_id, NULL, monitor_push_reports_handler, NULL) < 0) {
        if (debug.print) printf("could not create push report thread\n");
        return;
    }
    if (debug.print) puts("pushing report with thread");
    return;
}

/**
 * This will notify the client that the connection is established 
 * between the client process and the server thread.
 */
void *monitor_service_connection_handler(void *tp) {
    char *message;
    MonitorService *t = (MonitorService *)tp;
    pthread_mutex_lock(&monitorListMutex);

    // add to linked list
    if (monitorList->count == 0) {
        monitorList->head = t;
        monitorList->tail = t;
        monitorList->count++;
        if (debug.print) printf("MS-%d added to list at head (%d)\n", t->id, monitorList->count);
    }
    else {
        monitorList->tail->next = t;
        t->prev = monitorList->tail;
        monitorList->tail = t;
        monitorList->count++;
        if (debug.print) printf("MS-%d added to list (%d)\n", t->id, monitorList->count);
    }

    pthread_mutex_unlock(&monitorListMutex);

     
    // Notify client that a thread has taken the connection
    if (debug.print) printf("Write to sock %d\n",t->client_sock);
    message = "handshake:monitor";
    write(t->client_sock , message , strlen(message));

    // wait for client messages
    while(monitor_service_await_and_handle_message(t) == 0) {
        ;
    }

    // remove monitor service from push list when it disconnects or fails
    monitor_service_remove(t);

    pthread_exit(NULL);

    return NULL;
}

/**
 * This handles incoming communications from a client socket for 
 * an individual client thread.
 */
int monitor_service_await_and_handle_message(MonitorService *t) {
    char *message;
    int recvSize;
    char recvBuff[1025];

    // clear recvBuff
    memset(recvBuff, '\0', sizeof(recvBuff));

    // read a message from the client
    if (debug.print) printf("Attempt to read sock %d\n",t->client_sock);
    recvSize = read(t->client_sock, recvBuff, 1024);
    if (recvSize == 0) {
        // Client has disconnected
        if (debug.print) printf("Client disconnect\n");
        fflush(stdout);
        return -1;
    }
    else if (recvSize < 0) {
        // Error reading message
        if (debug.print) printf("ERROR reading from socket\n");
        return -1;
    }
    else {
        // Valid message from client
        if (debug.print) printf("Message from client: %s\n",recvBuff);
        // limit recvBuff size to 6, to eliminate duplicate "reportreport" commands
        strncpy(recvBuff, recvBuff, 5);
        recvBuff[6] = '\0';
        if (debug.print) printf("Message from client clean: %s\n",recvBuff);
        if( strcmp(recvBuff,"report") == 0 ) {
            message = "report data";
            if (debug.print) printf("lock ready mutex\n");
            // message indicates that the monitor is ready to receive
            pthread_mutex_lock(&(t->monitorReadyMutex));
            if (t->ready == 0) {
                pthread_cond_signal(&(t->monitorNowReady));
            }
            t->ready = 1;
            pthread_mutex_unlock(&(t->monitorReadyMutex));                
            if (debug.print) printf("unlock, now ready\n");
            monitor_push_reports();
            sleep(monitorPullDelay);
        }
        else {
            message = "unrecognized client command.";
        }
        if (debug.print) printf("Message back from monitor client:\n%s\n", message);
        // monitor_service_write_report(t);
        sleep(monitorPullDelay);
    }
    return 0;
}

void monitor_service_write_report(MonitorService *ms) {
    xmlNodePtr root_node, consumers_node, producers_node;
    xmlNodePtr buffer_node, events_node;
    xmlDocPtr doc;
    xmlChar *xmlbuff;
    int buffersize;

    doc = xmlNewDoc(BAD_CAST "1.0");
    root_node = xmlNewNode(NULL, BAD_CAST "report");
    xmlDocSetRootElement(doc, root_node);

    // print consumers as XML
    consumers_node = xmlNewChild(root_node, NULL, BAD_CAST "consumers", NULL);
    if (consumerList->count == 0) {
        // no consumers
    }
    else {
        ConsumerService *cs = consumerList->head;
        while (cs != NULL) {
            xmlNodePtr consumer;
            char consumer_data[1024];

            consumer = xmlNewChild(consumers_node, NULL, BAD_CAST "consumer", NULL);

            sprintf(consumer_data, "%d", cs->id);
            xmlNewChild(consumer, NULL, BAD_CAST "id", 
                BAD_CAST consumer_data);
            
            sprintf(consumer_data, "%d", cs->resources_consumed);
            xmlNewChild(consumer, NULL, BAD_CAST "resources_consumed", 
                BAD_CAST consumer_data);
            
            sprintf(consumer_data, "%d", cs->status);
            xmlNewChild(consumer, NULL, BAD_CAST "status", 
                BAD_CAST consumer_data);

            cs = cs->next;
        }
    }
    

    // print producers as XML
    producers_node = xmlNewChild(root_node, NULL, BAD_CAST "producers", NULL);
    int p;
    for (p = 0; p < pidx; p++) {
        xmlNodePtr producer_node;
        char producer_data[1024];
        
        producer_node = xmlNewChild(producers_node, NULL, BAD_CAST "producer", NULL);
        
        sprintf(producer_data, "%d", producers[p]->id);
        xmlNewChild(producer_node, NULL, BAD_CAST "id", 
            BAD_CAST producer_data);

        sprintf(producer_data, "%d", producers[p]->status);
        xmlNewChild(producer_node, NULL, BAD_CAST "status", 
            BAD_CAST producer_data);

        sprintf(producer_data, "%d", producers[p]->resources_produced);
        xmlNewChild(producer_node, NULL, BAD_CAST "count", 
            BAD_CAST producer_data);
    }

    buffer_node = xmlNewChild(root_node, NULL, BAD_CAST "buffer", NULL);


    // print buffer as XML
    ResourceBuffer *rb;
    rb = ms->env->bufferp;
    if (rb->count == 0) {
        // empty buffer
    }
    else {
        Resource *temp = rb->head;
        while (temp != NULL) {
            xmlNodePtr buffer_resource;
            char resource_data[1024];

            buffer_resource = xmlNewChild(buffer_node, NULL, BAD_CAST "resource", NULL);

            sprintf(resource_data, "%d", temp->id);
            xmlNewChild(buffer_resource, NULL, BAD_CAST "id", 
                BAD_CAST resource_data);
            
            sprintf(resource_data, "%d", temp->produced_by);
            xmlNewChild(buffer_resource, NULL, BAD_CAST "producer", 
                BAD_CAST resource_data);

            temp = temp->next;
        }
    }

    events_node = xmlNewChild(root_node, NULL, BAD_CAST "events", NULL);


    /*
     * Dump the document to a buffer and print it
     * for demonstration purposes.
     */
    xmlDocDumpFormatMemory(doc, &xmlbuff, &buffersize, 1);
    write(ms->client_sock, xmlbuff, buffersize);
    // if (debug.print) printf("wrote to socket:\n%s", (char *) xmlbuff);

    /*
     * Free associated memory.
     */
    xmlFree(xmlbuff);
    xmlFreeDoc(doc);

}