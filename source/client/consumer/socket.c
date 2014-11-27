/**
 * @file
 * Author: Trevor Simonton
 * 
 * This socket.c file handles the Winsock connection for the Consumer process
 * with the Linux server.
 *
 * This code was derived from examples posted on microsoft.com
 * @see: http://msdn.microsoft.com/en-us/library/windows/desktop/bb530750(v=vs.85).aspx
 */
#include <winsock2.h>
#include <ws2tcpip.h>
#include "consumer.h"

// Winsock
#pragma comment(lib, "Ws2_32.lib")
WSADATA wsaData;
SOCKET ConnectSocket = INVALID_SOCKET;

// default length for communication char buffers
#define DEFAULT_BUFLEN 512

// stubs for communication funcions
int consumer_connection_send_string(char *);
int consumer_connection_consume();

/**
 * Initialize the connection and begin making "consume" calls
 * Once the connection is made, this sends
 * the "handshake:consumer" signal that initializes the server
 * thread
 */
int consumer_connect_and_consume() {
	int iResult;
	struct addrinfo
		*result = NULL,
		*ptr = NULL,
		hints;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		if (debug.print) printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}
	else {
		if (debug.print) printf("winsock initialized\n");
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	iResult = getaddrinfo("192.168.0.111", "60118", &hints, &result);
	if (iResult != 0) {
		if (debug.print) printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return 1;
	}
	else {
		if (debug.print) printf("got addrinfo\n");
	}

	// Attempt to connect to the first address returned by
	// the call to getaddrinfo
	ptr = result;

	// Create a SOCKET for connecting to server
	ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

	if (ConnectSocket == INVALID_SOCKET) {
		if (debug.print) printf("Error at socket(): %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}
	else {
		if (debug.print) printf("connect socket\n");
	}

	// Connect to server.
	iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		closesocket(ConnectSocket);
		ConnectSocket = INVALID_SOCKET;
	}
	else {
		if (debug.print) printf("iresult connect OK\n");
	}

	// @todo: see microsoft comment below
	// Should really try the next address returned by getaddrinfo
	// if the connect call failed

	// addr info no longer needed
	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET) {
		if (debug.print) printf("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}
	else {
		if (debug.print) printf("connect socket valid\n");
	}


	// Send and receive the connection handshake messages
	if (debug.print) printf("Shaking hands...\n");

	// send
	consumer_connection_send_string("handshake:consumer");

	// receive
	int recvbuflen = DEFAULT_BUFLEN;
	char recvbuf[DEFAULT_BUFLEN];
	iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
	if (iResult > 0) {
		if (debug.print) printf("Bytes received: %d\n", iResult);
		recvbuf[iResult] = '\0';
		if (strcmp(recvbuf, "handshake:consumer") == 0) {
			if (debug.print) printf("HANDSHAKE SUCCESS:\nrecvbuf: %s\n", recvbuf);
			if (debug.print) puts("consuming...");

			// begin conumer loop with established connection
			while(consumer_connection_consume() > 0)
				;
		}
		else {
			if (debug.print) printf("HANDSHAKE FAIL:\nrecvbuf: %s\n", recvbuf);
		}
	}
	else if (iResult == 0) {
		if (debug.print) printf("Connection closed\n");
	}
	else {
		if (debug.print) printf("recv failed: %d\n", WSAGetLastError());
	}

	return 0;
}

/**
 * Send a string out to the server
 */
int consumer_connection_send_string(char *sendbuf) {
	if (debug.print) printf("Sending: %s...\n", sendbuf);
	int iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
	if (iResult == SOCKET_ERROR) {
		if (debug.print) printf("send failed: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		return 1;
	}
	else {
		if (debug.print) printf("send ok\n");
	}
	if (debug.print) printf("Bytes Sent: %ld\n", iResult);
	return 0;
}

/**
 * Send "consume" command to the server, and wait for the response.
 * The response should be a detail about the resource that was consumed.
 * This detail is printed with printf() to console.
 */
int consumer_connection_consume() {
	int iResult;

	int recvbuflen = DEFAULT_BUFLEN;
	char recvbuf[DEFAULT_BUFLEN];

	char *sendbuf = "consume";
	consumer_connection_send_string(sendbuf);

	if (debug.print) printf("Receiving...\n");
	// Receive a message
	iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
	if (iResult > 0) {
		if (debug.print) printf("Bytes received: %d\n", iResult);
		recvbuf[iResult] = '\0';
		if (debug.print) printf("recvbuf: %s", recvbuf);
		if (PRINT_CONSUMED) printf("%s\n", recvbuf);
	}
	else if (iResult == 0) {
		if (debug.print) printf("Connection closed\n");
	}
	else {
		if (debug.print) printf("recv failed: %d\n", WSAGetLastError());
	}
	
	return iResult;
}

/**
 * Disconnect and clean up memory
 */
int consumer_connection_shutdown() {
	int iResult;

	// shutdown the connection for sending since no more data will be sent
	// the client can still use the ConnectSocket for receiving data
	iResult = shutdown(ConnectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		if (debug.print) printf("shutdown failed: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		return 1;
	}
	else {
		if (debug.print) printf("shutdown ok\n");
	}

	// cleanup
	closesocket(ConnectSocket);
	WSACleanup();

	return 0;
}