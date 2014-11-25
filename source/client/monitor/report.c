// @see: http://msdn.microsoft.com/en-us/library/windows/desktop/ms682516%28v=vs.85%29.aspx

#include "monitor.h"
#include <libxml/parser.h>
#include <libxml/tree.h>

DWORD WINAPI reportThread(LPVOID lpParam);

char consumer_report[4096];
char buffer_report[4096];
char producer_report[4096];

char consumer_line[512];
char resource_line[512];
char producer_line[512];

int start_report_thread() {
	/*
	* this initialize the library and check potential ABI mismatches
	* between the version it was compiled for and the actual shared
	* library used.
	*/
	LIBXML_TEST_VERSION

	// Allocate memory for thread data.
	pReportData = (PReportData)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ReportData));

	if (pReportData == NULL) {
		// If the array allocation fails, the system is out of memory
		// so there is no point in trying to print an error message.
		// Just terminate execution.
		ExitProcess(2);
	}

	// Create the thread to begin execution on its own.
	reportThreadHandle = CreateThread(
		NULL,                   // default security attributes
		0,                      // use default stack size  
		reportThread,           // thread function name
		pReportData,            // argument to thread function 
		0,                      // use default creation flags 
		&reportThreadID);       // returns the thread identifier 


	// Check the return value for success.
	// If CreateThread fails, terminate execution. 
	// This will automatically clean up threads and memory. 
	if (reportThreadHandle == NULL)	{
		ErrorHandler(TEXT("CreateThread"));
		ExitProcess(3);
	}

	return 0;
}

void report_shutdown() {

	// Wait until all threads have terminated.
	WaitForSingleObject(reportThreadHandle, INFINITE);

	// Close all thread handles and free memory allocations.
	CloseHandle(reportThreadHandle);
	if (pReportData != NULL) {
		HeapFree(GetProcessHeap(), 0, pReportData);
		pReportData = NULL;    // Ensure address is not reused.
	}

}


DWORD WINAPI reportThread(LPVOID lpParam) {
	PReportData pReportDataArray;

	// Cast the parameter to the correct data type.
	// The pointer is known to be valid because 
	// it was checked for NULL before the thread was created.
	pReportDataArray = (PReportData)lpParam;

	monitor_connect_and_monitor();

	return 0;
}



int monitor_xml_write_message(int socket, char *message) {
	xmlNodePtr n;
	xmlDocPtr doc;
	xmlChar *xmlbuff;
	int buffersize;

	/*
	* Create the document.
	*/
	doc = xmlNewDoc(BAD_CAST "1.0");
	n = xmlNewNode(NULL, BAD_CAST "message");
	xmlNodeSetContent(n, BAD_CAST message);
	xmlDocSetRootElement(doc, n);

	/*
	* Dump the document to a buffer and print it
	* for demonstration purposes.
	*/
	xmlDocDumpFormatMemory(doc, &xmlbuff, &buffersize, 1);
	printf("wrote to socket:\n%s", (char *)xmlbuff);

	/*
	* Free associated memory.
	*/
	xmlFree(xmlbuff);
	xmlFreeDoc(doc);

	return (0);

}

struct consumer {
	xmlChar *id;
	xmlChar *status;
	xmlChar *resources_consumed;
};

struct resource {
	xmlChar *id;
	xmlChar *producer;
};

struct producer {
	xmlChar *id;
	xmlChar *status;
	xmlChar *count;
};


void monitor_xml_parse_consumer(xmlNode * a_node) {
	int count = 0;
	xmlNode *cur_node = NULL;
	struct consumer *c = malloc(sizeof(*c));
	memset(consumer_line, '\0', sizeof(consumer_line));
	for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
		if (cur_node->type == XML_ELEMENT_NODE) {
			if (strcmp(cur_node->name, "id") == 0) {
				c->id = xmlNodeGetContent(cur_node);
				count++;
			}
			if (strcmp(cur_node->name, "status") == 0) {
				c->status = xmlNodeGetContent(cur_node);
				count++;
			}
			if (strcmp(cur_node->name, "resources_consumed") == 0) {
				c->resources_consumed = xmlNodeGetContent(cur_node);
				count++;
			}
		}
	}
	if (count % 3 != 0) {
		strcpy_s(consumer_line, sizeof(consumer_line), "Invalid consumer node\n");
		return;
	}
	if (count == 0) {
		return;
	}
	sprintf_s(consumer_line, sizeof(consumer_line), "consumer %s:\n   resources consumed:%s\n   status: %s\n-----------\n",
		c->id, c->resources_consumed, c->status);
	xmlFree(c->id);
	xmlFree(c->resources_consumed);
	xmlFree(c->status);
	free(c);
}

void monitor_xml_parse_resource(xmlNode * a_node) {
	int count = 0;
	xmlNode *cur_node = NULL;
	struct resource *c = malloc(sizeof(*c));
	memset(resource_line, '\0', sizeof(resource_line));
	for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
		if (cur_node->type == XML_ELEMENT_NODE) {
			if (strcmp(cur_node->name, "id") == 0) {
				c->id = xmlNodeGetContent(cur_node);
				count++;
			}
			if (strcmp(cur_node->name, "producer") == 0) {
				c->producer = xmlNodeGetContent(cur_node);
				count++;
			}
		}
	}
	if (count % 2 != 0) {
		strcpy_s(resource_line, sizeof(resource_line), "Invalid buffer node\n");
		return;
	}
	if (count == 0) {
		return;
	}
	sprintf_s(resource_line, sizeof(resource_line), "resource %s:\n   produced by:%s\n-----------\n",
		c->id, c->producer);
	xmlFree(c->id);
	xmlFree(c->producer);
	free(c);
}

void monitor_xml_parse_producer(xmlNode * a_node) {
	xmlNode *cur_node = NULL;
	int count = 0;
	struct producer *c = malloc(sizeof(*c));
	memset(producer_line, '\0', sizeof(producer_line));
	for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
		if (cur_node->type == XML_ELEMENT_NODE) {
			if (strcmp(cur_node->name, "id") == 0) {
				c->id = xmlNodeGetContent(cur_node);
				count++;
			}
			if (strcmp(cur_node->name, "status") == 0) {
				c->status = xmlNodeGetContent(cur_node);
				count++;
			}
			if (strcmp(cur_node->name, "count") == 0) {
				c->count = xmlNodeGetContent(cur_node);
				count++;
			}
		}
	}
	if (count % 3 != 0) {
		strcpy_s(producer_line, sizeof(producer_line), "Invalid producer node\n");
	}
	if (count == 0) {
		return;
	}
	sprintf_s(producer_line, sizeof(producer_line), "producer %s:\n   resources produced:%s\n   status: %s\n-----------\n",
		c->id, c->count, c->status);
	xmlFree(c->id);
	xmlFree(c->count);
	xmlFree(c->status);
	free(c);
}

void monitor_xml_parse_consumers(xmlNode * a_node) {
	if (a_node == NULL) {
		message_buffer_set(0, "No consumers.");
	}
	else {
		memset(consumer_report, '\0', sizeof(consumer_report));
		xmlNode *cur_node = NULL;
		for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
			if (cur_node->type == XML_ELEMENT_NODE) {
				monitor_xml_parse_consumer(cur_node->children);
				strcat_s(consumer_report, sizeof(consumer_report), consumer_line);
			}
		}
		message_buffer_set(0, consumer_report);
	}
}
void monitor_xml_parse_buffer(xmlNode * a_node) {
	if (a_node == NULL) {
		message_buffer_set(1, "No resources.");
	}
	else {
		memset(buffer_report, '\0', sizeof(buffer_report));
		xmlNode *cur_node = NULL;
		for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
			if (cur_node->type == XML_ELEMENT_NODE) {
				monitor_xml_parse_resource(cur_node->children);
				strcat_s(buffer_report, sizeof(buffer_report), resource_line);
			}
		}
		message_buffer_set(1, buffer_report);

	}

}
void monitor_xml_parse_producers(xmlNode * a_node) {
	if (a_node == NULL) {
		message_buffer_set(2, "No producers.");
	}
	else {
		memset(producer_report, '\0', sizeof(producer_report));
		xmlNode *cur_node = NULL;
		for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
			if (cur_node->type == XML_ELEMENT_NODE) {
				monitor_xml_parse_producer(cur_node->children);
				strcat_s(producer_report, sizeof(producer_report), producer_line);
			}
		}
		message_buffer_set(2, producer_report);

	}
}

/**
* print_element_names:
* @a_node: the initial xml node to consider.
*
* Prints the names of the all the xml elements
* that are siblings or children of a given xml node.
*/
void monitor_xml_parse_report_recursive(xmlNode * a_node)
{
	xmlNode *cur_node = NULL;

	for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
		if (cur_node->type == XML_ELEMENT_NODE) {
			if (strcmp(cur_node->name, "report") == 0) {
				monitor_xml_parse_report_recursive(cur_node->children);
			}
			if (strcmp(cur_node->name, "consumers") == 0) {
				monitor_xml_parse_consumers(cur_node->children);
			}
			if (strcmp(cur_node->name, "buffer") == 0) {
				monitor_xml_parse_buffer(cur_node->children);
			}
			if (strcmp(cur_node->name, "producers") == 0) {
				monitor_xml_parse_producers(cur_node->children);
			}
		}
		else {
			monitor_xml_parse_report_recursive(cur_node->children);
		}
	}
}


int monitor_xml_parse_report(char *message) {
	xmlDoc *doc = NULL;
	xmlNode *root_element = NULL;

	/*parse the file and get the DOM */
	doc = xmlParseMemory(message, strlen(message));

	if (doc == NULL) {
		printf("error: could not parse file\n");
	}

	/*Get the root element node */
	root_element = xmlDocGetRootElement(doc);

	monitor_xml_parse_report_recursive(root_element);

	/*free the document */
	xmlFreeDoc(doc);

	/*
	*Free the global variables that may
	*have been allocated by the parser.
	*/
	xmlCleanupParser();

	return 0;
}