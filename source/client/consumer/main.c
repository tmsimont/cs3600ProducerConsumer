/**
 * @file
 * Author: Trevor Simonton
 *
 * This is the simple Consumer process main function. This will
 * start a connection with the Linux server and continually ask 
 * to consume resources.
 * 
 * Resources received from the server are printed to the console
 * window with printf()
 *
 */
#include "consumer.h"

int main() {
	debug.print = 0;
	consumer_connect_and_consume();
	consumer_connection_shutdown();
	return 0;
}