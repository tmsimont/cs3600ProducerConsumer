#include <stdio.h>
#include <windows.h>

// Primary server function
int consumer_connect_and_consume();
int consumer_connection_shutdown();

#define PRINT_CONSUMED 1

struct {
	unsigned int print : 1;
} debug;