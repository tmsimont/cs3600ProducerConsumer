/**
 * @file
 * Author: Trevor Simonton
 *
 * This is the main Monitor process. This starts a GUI for the user to interact with, 
 * and waits to spawn Consumer processes
 */

#include "monitor.h"

#define MAX_CHILD_PROCCESS 128

STARTUPINFO si[MAX_CHILD_PROCCESS];
PROCESS_INFORMATION pi[MAX_CHILD_PROCCESS];

int procIndex = 0;
void monitor_new_process();
void monitor_shutdown();

int main(int argc, char *argv[]) {
	debug.print = 0;
	debug.console_report = 0;

	gtk_init(&argc, &argv);
	start_ui();	

	monitor_connection_shutdown();
	monitor_shutdown();
	report_shutdown();
}

/**
 * Spawn a new Consumer process. Note that "consumer.exe" must be in the same
 * folder as the running Monitor process.
 *
 * This code was built from an example on microsoft.com:
 * @see: http://msdn.microsoft.com/en-us/library/windows/desktop/ms682512(v=vs.85).aspx
 */
void monitor_new_process() {
	ZeroMemory(&si[procIndex], sizeof(si[procIndex]));
	si[procIndex].cb = sizeof(si[procIndex]);
	ZeroMemory(&pi[procIndex], sizeof(pi[procIndex]));

	// Start the child process. 
	if (!CreateProcess(NULL,   // No module name (use command line)
		"consumer",        // Command line
		NULL,           // Process handle not inheritable
		NULL,           // Thread handle not inheritable
		FALSE,          // Set handle inheritance to FALSE
		0,              // No creation flags
		NULL,           // Use parent's environment block
		NULL,           // Use parent's starting directory 
		&si[procIndex],            // Pointer to STARTUPINFO structure
		&pi[procIndex])           // Pointer to PROCESS_INFORMATION structure
		)
	{
		printf("CreateProcess failed (%d).\n", GetLastError());
		return;
	}

	procIndex++;
}

/**
 * Wait for spawned Consumer to stop and then close the handles.
 */
void monitor_shutdown() {
	while (procIndex >= 0){
		// Wait until child process exits.
		WaitForSingleObject(pi[procIndex].hProcess, 1000);

		// Close process and thread handles. 
		CloseHandle(pi[procIndex].hProcess);
		CloseHandle(pi[procIndex].hThread);
		procIndex--;
	}
}

/**
 * This is an error handler provided in the above microsoft example.
 */
void ErrorHandler(LPTSTR lpszFunction) {
	// Retrieve the system error message for the last-error code.

	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	// Display the error message.

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
		(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
	StringCchPrintf((LPTSTR)lpDisplayBuf,
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"),
		lpszFunction, dw, lpMsgBuf);
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

	// Free error-handling buffer allocations.

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
}


