# cs3600ProducerConsumer

This was a class project designed to demonstrate the producer consumer problem on a multi-threaded server application.

The server was designed to use pthreads to dispatch a new handler thread for each connecting client. 
Each server thread then competes for resources in the producer buffer.

The client code is a simple Windows process that connects to the server and asks it to consume from the buffer.

There is also  Windows multi-threaded code that uses GTK to create a "monitor" that connects to the server and
asks for XML to describe the current number of clients, and the contents of the producer buffer. This then provides 
a visual representation of the buffer and clients, based on the XML response.

