#include <stdio.h>
#include <windows.h>

// Primary server function
int consumer_connect_and_consume();
int consumer_connection_shutdown();

struct {
	unsigned int print : 1;
} debug;