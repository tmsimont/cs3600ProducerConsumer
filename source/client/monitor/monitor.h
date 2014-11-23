#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include <strsafe.h>

// Primary server function
int monitor_connect_and_monitor();
int monitor_connection_shutdown();

DWORD   reportThreadID;
HANDLE  reportThreadHandle;
int start_getreport_thread();
void getreport_shutdown();

struct {
	unsigned int print : 1;
	unsigned int console_report : 1;
} debug;