#include "consumer.h"

int main() {
	debug.print = 0;
	consumer_connect_and_consume();
	consumer_connection_shutdown();
	return 0;
}