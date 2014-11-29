/**
 * @file
 * Author: Trevor Simonton
 * 
 * The socket.c file provides Winsock2 integration for the 
 * Monitor process. This connects the Monitor to the Linux server
 * and handles communication to and from the server.
 *
 * This code was derived from examples posted on microsoft.com
 * @see: http://msdn.microsoft.com/en-us/library/windows/desktop/bb530750(v=vs.85).aspx
 */

#include <winsock2.h>
#include <ws2tcpip.h>
#include "monitor.h"

// Winsock
#pragma comment(lib, "Ws2_32.lib")
WSADATA wsaData;
SOCKET ConnectSocket = INVALID_SOCKET;

// stubs for communication functions
int monitor_connection_monitor();

/**
 * This is the main function of this "class."
 * This initializes Winsock, and establishes the connection with 
 * the Linux server. Once the connection is made, this sends
 * the "handshake:monitor" signal that initializes the server
 * thread
 */
int monitor_connect_and_monitor() {
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

	// prepare the address info
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

	// Send and receive the connection handshake message
	if (debug.print) printf("Shaking hands...\n");
	monitor_connection_send_string("handshake:monitor");
	iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
	if (iResult > 0) {
		if (debug.print) printf("Bytes received: %d\n", iResult);
		recvbuf[iResult] = '\0';

		// validate handshake
		if (strcmp(recvbuf, "handshake:monitor") == 0) {
			// successful handshake with server
			if (debug.print) printf("HANDSHAKE SUCCESS:\nrecvbuf: %s\n", recvbuf);
			if (debug.print) puts("monitoring...");

			// start receive data loop
			monitor_connection_monitor();
		}
		else {
			// failure to establish thread handshake...
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
 * Send a string to the ConnectSocket
 */
int monitor_connection_send_string(char *sendbuf) {
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
 * Repeatedly send the message "report" to the Linux server to
 * indicate that this Monitor process is ready to receive the next
 * "report data" package from the server.
 */
int monitor_connection_monitor() {
	int iResult;
	DWORD waitResult;
	guiUpdateEvent = CreateEvent(NULL, TRUE, FALSE, TEXT("guiUpdateEvent"));

	do {
		// reset receive buffer
		memset(recvbuf, '\0', sizeof(recvbuf));

		// send "report" signal to the ConnectSocket
		char *sendbuf = "report";
		monitor_connection_send_string(sendbuf);

		// Receive the "report data" and parse it
		if (debug.print) printf("Receiving...\n");
		iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0) {
			if (debug.print) printf("Bytes received: %d\n", iResult);

			// null terminate the buffer
			recvbuf[iResult] = '\0';
			if (debug.print) printf("recvbuf: %s", recvbuf);
			if (debug.console_report) printf("%s\n", recvbuf);

			// add an endline character
			recvbuf[iResult + 1] = '\n';

			printf("parse report\n");

			// parse the report
			monitor_xml_parse_report(recvbuf);

			// wait for the GUI to be updated (update occurs in separate thread)
			// @see viewport.c::display_status_textbuffer()
			printf("waiting for signal...\n");
			// at most wait 1 second...
			waitResult = WaitForSingleObject(guiUpdateEvent, 1000);
			switch (waitResult) {
				case WAIT_OBJECT_0:
					// reset the event and continue the loop
					printf("reset signal.\n");
					ResetEvent(guiUpdateEvent);
					break;
				default:
					printf("Wait error (%d)\n", GetLastError());
					// error.. just wait it out and try to continue
					Sleep(1000);
					ResetEvent(guiUpdateEvent);
			}

		}
		else if (iResult == 0) {
			if (debug.print) printf("Connection closed\n");
		}
		else {
			if (debug.print) printf("recv failed: %d\n", WSAGetLastError());
		}
	} while (iResult > 0);

	return iResult;
}

/**
 * Shutdown the connection by closing the ConnectSocket and 
 * cleaning up Winsock elements.
 */
int monitor_connection_shutdown() {
	int iResult;

	CloseHandle(guiUpdateEvent);

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