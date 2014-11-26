/*
 * File: consumer.h
 * Author : Trevor Simonton
 *
 * Define simple function stubs for the Consumer process
 * and include appropriate headers.
 */
#include <stdio.h>
#include <windows.h>

// Primary server function
int consumer_connect_and_consume();
int consumer_connection_shutdown();

// Should we print the consumer resources to console?
#define PRINT_CONSUMED 1

// should debug statements be printed to console?
struct {
	unsigned int print : 1;
} debug;