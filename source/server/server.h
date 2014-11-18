/* 
 * File:   server.h
 * Author: Trevor Simonton
 *
 * Created on November 17, 2014, 5:04 PM
 */

// Producer
typedef struct _Producer Producer;
Producer *producers[5];
Producer *producer_new(void);

// Resource
typedef struct _Resource Resource;

// ResourceBuffer
typedef struct _ResourceBuffer ResourceBuffer;

// Primary server function
int initialize_producers(void);
int execute_producers(void);
int report_status(void);
int server_listen(void);
int server_close_connection(void);
