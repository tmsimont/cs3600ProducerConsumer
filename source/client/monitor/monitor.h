#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <gtk/gtk.h>

// Primary server function
int monitor_connect_and_monitor();
int monitor_connection_shutdown();
void monitor_new_process();

// Report thread
DWORD   reportThreadID;
HANDLE  reportThreadHandle;
int start_report_thread();
void report_shutdown();
typedef struct _reportData ReportData, *PReportData;
struct _reportData {
	int foo;
};
PReportData pReportData;


HANDLE    reportReadyMutex;
int reportReady;


int start_ui();
int viewport_update_panes(char *, char *, char *);
int monitor_xml_write_message(int, char *);
int monitor_xml_parse_report(char *);

void ErrorHandler(LPTSTR);

struct {
	unsigned int print : 1;
	unsigned int console_report : 1;
} debug;