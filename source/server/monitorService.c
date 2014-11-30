/**
 * @file
 * Author: Trevor Simonton
 * 
 * The MonitorService is a struct that tracks data that is used by 
 * individual Monitor-handling threads. Communication with Monitors
 * is handled with XML, generated and parsed with libxml2.
 */

#include "server.h"
#include <libxml/parser.h>


int monitor_service_await_and_handle_message(MonitorService*);
void *monitor_service_connection_handler(void *);
void monitor_service_write_report(MonitorService *);
void monitor_mark_no_longer_queued_send(MonitorService *);

/**
 * Create a new MonitorService struct, and begin the corresponding thread.
 * This new struct is added to the global linked list of MonitorService
 * structs.  This is a doubly-linked list with a tail. The list helps us
 * track any existing monitor connections.
 */
int monitor_service_new(Environment *env, int client_sock) {
    MonitorService *t = malloc(sizeof(*t));
    t->client_sock = client_sock;
    t->env = env;
    t->ready = 0;
    t->deleted = 0;
    t->waiting = 0;
    t->has_queued_send = 0;
    pthread_mutex_init(&(t->hasQueuedSendMutex), NULL);
    t->id = monitorList->idx++;
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

/**
 * Remove a MonitorService from the global linked list of MonitorService
 * structs. This should be called when a Monitor disconnects from the server.
 * The process of removing the monitor is delicate. Access to the list must be
 * locked, and so must access to the individual MonitorService.
 */
int monitor_service_remove(MonitorService *ms) {

    // acquire individual MonitorService ready flag mutex
    pthread_mutex_lock(&(ms->monitorReadyMutex));
        ms->deleted = 1;
        // another thread is waiting for this ms to be ready, release that
        // thread and don't delete this yet.
        // @see: monitor_push_reports_handler_for_ms()
        if (ms->waiting == 1) {
            // unblock any waiting push report calls
            if (debug.print) printf("MS waiting cant delete.");
            pthread_cond_signal(&(ms->monitorNowReady));
            ms->ready = 1;
            pthread_mutex_unlock(&(ms->monitorReadyMutex));
            return -1;
        }
    // release MonitorService ready flag mutex
    pthread_mutex_unlock(&(ms->monitorReadyMutex));


    // acquire list mutex
    pthread_mutex_lock(&monitorListMutex);


    // CRITICAL SECTION-------------------------------------------

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


    // END CRITICAL SECTION---------------------------------------


    // release list mutex
    pthread_mutex_unlock(&monitorListMutex);

}

/**
 * Thread handler for individual monitorService push report call.
 * 
 * There may be numerous MonitorService instances. These are tracked
 * in a linked list. That linked list may be altered my several threads.
 * So may the individual MonitorService instances. We need to protect both 
 * the linked list as a whole, and each individual service with mutex locks.
 *
 * In order to prevent deadlock, one thread acquires the lock on the list,
 * and as it traverses the list it will dispatch a new thread for each item
 * in the least. These child threads will then acquire the lock on the individual
 * item, and might have to wait for the signal that the individual item is 
 * ready. If this were handled by the parent, which is traversing the list, then
 * the mutex lock on the list would not be released in a timely fashion.
 *
 * Therefore, this function is used to prevent the monitor_push_reports_handler
 * caller thread from blocking the monitorListMutex while traversing 
 * the linked list of active MonitorServices.
 *
 * @see monitor_push_reports_handler()
 */
void *monitor_push_reports_handler_for_ms(void *msp) {
    MonitorService *ms = (MonitorService *)msp;

    // acquire individual MonitorService ready flag mutex
    pthread_mutex_lock(&(ms->monitorReadyMutex));
    if (ms->ready == 0 && ms->deleted == 0) {
        // wait until the individual monitor is ready
        if (debug.print) printf("wait for MS-%d\n",ms->id);
        ms->waiting = 1;
        pthread_cond_wait(&(ms->monitorNowReady), &(ms->monitorReadyMutex));
    }

    // this thread is not waiting for the monitorNowReady signal
    ms->waiting = 0;

    // mark the monitor as not ready, and send report data
    ms->ready = 0;
    if (ms->deleted == 0) {
        if (debug.print) printf("MS-%d not deleted.\n",ms->id);
        monitor_service_write_report(ms);
    }
    else {
        // this was flagged for delete, but was not deleted
        // because this thread was waiting for this MontiorService.
        // now it's gone, so queue the removal
        // @see: monitor_service_remove()
        if (debug.print) printf("MS-%d flag for delete.\n", ms->id);
        pthread_mutex_unlock(&(ms->monitorReadyMutex));
        monitor_service_remove(ms);
        pthread_exit(NULL);
    }
    // release MonitorService ready flag mutex
    pthread_mutex_unlock(&(ms->monitorReadyMutex));

    pthread_exit(NULL);
}

/**
 * This will push "report data" out to all connected Monitor clients and 
 * immediately exit the calling thread. This is called by threads created
 * in monitor_push_reports(), which allows "report data" to be pushed
 * in separate thread from the thread that calls monitor_push_reports().
 * This avoids deadlock when Producers or ConsumerServices call to push
 * "report data."
 */
void *monitor_push_reports_handler(void *tp) {
    if (debug.print) printf("push reports\n");

    // acquire list mutex
    pthread_mutex_lock(&monitorListMutex);

    if (monitorList->count == 0) {
        // no monitors
        if (debug.print) printf("no monitors\n");
    }
    else {
        // iterate over all MonitorService instances
        MonitorService *ms = monitorList->head;
        while (ms != NULL) {

            // before spawning another thread to send a report when the 
            // monitor service is ready, make sure there isn't already 
            // a thread waiting to do exactly this (no need to have numerous
            // threads waiting to do the same thing...)

            // acquire individual "has queued mutex"
            pthread_mutex_lock(&(ms->hasQueuedSendMutex));
            // prevent creation of erroneous threads
            if (ms->has_queued_send) {
                pthread_mutex_unlock(&(ms->hasQueuedSendMutex));
                ms = ms->next;
                continue;
            }
            else {
                ms->has_queued_send = 1;
            }
            pthread_mutex_unlock(&(ms->hasQueuedSendMutex));

            // use yet another thread so we can quickly release the monitorListMutex
            pthread_t thread_id;
            if( pthread_create(&thread_id, NULL, monitor_push_reports_handler_for_ms, (void*)ms) < 0) {
                if (debug.print) printf("could not create push report thread ms\n");
                return;
            }
            if (debug.print) printf("pushing report for MS-%d\n",ms->id);

            ms = ms->next;
        }
    }

    // release list mutex
    pthread_mutex_unlock(&monitorListMutex);

    pthread_exit(NULL);
}

/**
 * Push report XML data out to listening Monitor processes.
 * This is done with a new thread so calling Producer or ConsumerService
 * threads are not stopped.
 */
void monitor_push_reports() {
    pthread_t thread_id;
    if( pthread_create(&thread_id, NULL, monitor_push_reports_handler, NULL) < 0) {
        if (debug.print) printf("could not create push report thread\n");
        return;
    }
    if (debug.print) puts("pushing report with thread");
}

/**
 * This will notify the client that the connection is established 
 * between the client process and the MonitorService thread. Note that 
 * access to the global monitorList is protected by mutex in this function.
 */
void *monitor_service_connection_handler(void *tp) {
    char *message;
    MonitorService *t = (MonitorService *)tp;

    // acquire monitorListMutex
    pthread_mutex_lock(&monitorListMutex);

    // CRITICAL SECTION-------------------------------------------

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

    // END CRITICAL SECTION---------------------------------------

    // release monitorListMutex
    pthread_mutex_unlock(&monitorListMutex);

     
    // Notify client that a thread has taken the connection
    if (debug.print) printf("Write to sock %d\n",t->client_sock);
    message = "handshake:monitor";
    write(t->client_sock , message , strlen(message));

    // push reports now we have a new Monitor
    monitor_push_reports();

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
        // TODO: why do some messages come through duplicated? (need message framing...)
        strncpy(recvBuff, recvBuff, 5);
        recvBuff[6] = '\0';
        if (debug.print) printf("Message from client clean: %s\n",recvBuff);

        // report message from client
        if( strcmp(recvBuff,"report") == 0 ) {
            // message indicates that the monitor is ready to receive report
            if (debug.print) printf("lock ready mutex\n");

            // acquire this MonitorService ready flag mutex
            pthread_mutex_lock(&(t->monitorReadyMutex));

            // CRITICAL SECTION-------------------------------------------
            if (t->ready == 0) {
                // signal other threads this MonitorService is ready to send data
                pthread_cond_signal(&(t->monitorNowReady));
            }

            // lock has hasQueuedSendMutex, set to 0, unlock it
            monitor_mark_no_longer_queued_send(t);

            t->ready = 1;
            // END CRITICAL SECTION---------------------------------------

            // release this MonitorService ready flag mutex
            pthread_mutex_unlock(&(t->monitorReadyMutex));
            if (debug.print) printf("unlock, now ready\n");

            // regular interval push reports call
            //monitor_push_reports();
        }
        else {
            if (debug.print) printf("unrecognized client command.\n");
        }

        // sets interval for regular push/pull call
        //sleep(1);
    }
    return 0;
}

/**
 * Set the has_queued_send flag to "false" in thread-safe manner
 */
void monitor_mark_no_longer_queued_send(MonitorService *ms) {
    pthread_mutex_lock(&(ms->hasQueuedSendMutex));
    ms->has_queued_send = 0;
    pthread_mutex_unlock(&(ms->hasQueuedSendMutex));
}

/**
 * Send report Data in XML to the client socket that is stored
 * in the given MonitorService object.
 */
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