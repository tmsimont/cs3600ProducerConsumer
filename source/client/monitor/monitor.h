#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <gtk/gtk.h>

// Primary server function
int monitor_connect_and_monitor();
int monitor_connection_shutdown();

DWORD   reportThreadID;
HANDLE  reportThreadHandle;
int start_getreport_thread();
void getreport_shutdown();


int start_ui();

int message_buffer(int, char *);
int xml_write_message(int, char *);
int xml_parse_message(char *);


struct {
	unsigned int print : 1;
	unsigned int console_report : 1;
} debug;