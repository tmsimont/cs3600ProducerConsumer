#include <stdio.h>
#include <windows.h>

// Producer
typedef struct _Consumer Consumer;
Consumer *consumers[5];
Consumer *consumer_new(void);
int consumer_consumer(Consumer);
int consumer_sleep(Consumer);
int initialize_consumers(void);
int execute_consumers(void);

// Resource
typedef struct _Resource Resource;

// ResourceBuffer
typedef struct _ResourceBuffer ResourceBuffer;

// Primary server function
int consumer_test_message();