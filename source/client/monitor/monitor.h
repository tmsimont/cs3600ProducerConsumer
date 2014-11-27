/**
 * @file
 * Author: Trevor Simonton
 *
 * Header for the Monitor process.
 * This includes thread, socket and string libraries, and
 * declares shared functions and resources.
 */
#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <gtk/gtk.h>

// Primary server interactions
int monitor_connect_and_monitor();
int monitor_connection_shutdown();
void monitor_new_process();
int monitor_connection_send_string(char *);

// Report thread
DWORD   reportThreadID;
HANDLE  reportThreadHandle;
int start_report_thread();
void report_shutdown();

// Event to signal GUI update (which occurs in a separate thread)
HANDLE guiUpdateEvent;

// GUI 
int start_ui();
int viewport_update_panes(char *, char *, char *);

// XML parser
int monitor_xml_write_message(int, char *);
int monitor_xml_parse_report(char *);

// Microsoft error handler
void ErrorHandler(LPTSTR);

// Debugging flags
struct {
	unsigned int print : 1;
	unsigned int console_report : 1;
} debug;