#include "monitor.h"


STARTUPINFO si[5];
PROCESS_INFORMATION pi[5];
int procIndex = 0;
void monitor_new_process();
void monitor_shutdown();

/**
 *
 * @see: http://msdn.microsoft.com/en-us/library/windows/desktop/ms682512(v=vs.85).aspx
 */
int main(int argc, char *argv[]) {
	debug.print = 0;
	debug.console_report = 0;

	reportReady = 0;
	reportReadyMutex = CreateMutex( NULL, FALSE, NULL);

	gtk_init(&argc, &argv);
	start_ui();	
	monitor_shutdown();
	report_shutdown();
}

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

void monitor_shutdown() {
	while (procIndex >= 0){
		// Wait until child process exits.
		WaitForSingleObject(pi[procIndex].hProcess, INFINITE);

		// Close process and thread handles. 
		CloseHandle(pi[procIndex].hProcess);
		CloseHandle(pi[procIndex].hThread);
		procIndex--;
	}
}


void ErrorHandler(LPTSTR lpszFunction)
{
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


