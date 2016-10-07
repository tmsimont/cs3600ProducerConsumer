# cs3600ProducerConsumer

This was a class project designed to demonstrate the producer consumer problem on a multi-threaded server application.

The server was designed to use pthreads to dispatch a new handler thread for each connecting client. 
Each server thread then competes for resources in the producer server's buffer.

The client consumer code is a simple Windows process that connects to the server on a network socket and asks for items in the buffer.

There is also a Windows "monitor" process that provides a GUI for the application. This application connects to the server, and spawns consumer processes. It asks the server for XML to describe the current number of client threads, and the contents of the producer buffer. The application then displays a visual representation of the buffer and clients, based on the XML response. This makes it possible to visualize the producer/consumer interactions.

