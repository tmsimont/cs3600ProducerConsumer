// @see: http://msdn.microsoft.com/en-us/library/windows/desktop/ms682516%28v=vs.85%29.aspx

#include "monitor.h"

#define BUF_SIZE 255

DWORD WINAPI getReportThread(LPVOID lpParam);
void ErrorHandler(LPTSTR lpszFunction);

// Sample custom data structure for threads to use.
// This is passed by void pointer so it can be any data type
// that can be passed using a single void pointer (LPVOID).
typedef struct MyData {
	int val1;
	int val2;
} MYDATA, *PMYDATA;

PMYDATA pData;

int start_getreport_thread()
{

	// Allocate memory for thread data.
	pData = (PMYDATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MYDATA));

	if (pData == NULL) {
		// If the array allocation fails, the system is out of memory
		// so there is no point in trying to print an error message.
		// Just terminate execution.
		ExitProcess(2);
	}

	// Generate unique data for each thread to work with.
	pData->val1 = 1;
	pData->val2 = 2;

	// Create the thread to begin execution on its own.

	reportThreadHandle = CreateThread(
		NULL,                   // default security attributes
		0,                      // use default stack size  
		getReportThread,       // thread function name
		pData,          // argument to thread function 
		0,                      // use default creation flags 
		&reportThreadID);   // returns the thread identifier 


	// Check the return value for success.
	// If CreateThread fails, terminate execution. 
	// This will automatically clean up threads and memory. 

	if (reportThreadHandle == NULL)
	{
		ErrorHandler(TEXT("CreateThread"));
		ExitProcess(3);
	}

	return 0;
}

void getreport_shutdown() {

	// Wait until all threads have terminated.
	WaitForSingleObject(reportThreadHandle, INFINITE);

	// Close all thread handles and free memory allocations.
	CloseHandle(reportThreadHandle);
	if (pData != NULL) {
		HeapFree(GetProcessHeap(), 0, pData);
		pData = NULL;    // Ensure address is not reused.
	}

}


DWORD WINAPI getReportThread(LPVOID lpParam)
{
	PMYDATA pDataArray;

	// Cast the parameter to the correct data type.
	// The pointer is known to be valid because 
	// it was checked for NULL before the thread was created.
	pDataArray = (PMYDATA)lpParam;

	monitor_connect_and_monitor();

	return 0;
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
