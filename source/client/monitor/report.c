/**
 * @file
 * Author: Trevor Simonton
 *
 * This file handles XML parsing of messages received from the server.
 * Messages are parsed with the libxml2 library.
 * 
 * Report-related communication with the server is handled in socket.c 
 * by the thread created in this class.
 */

#include "monitor.h"
#include <libxml/parser.h>
#include <libxml/tree.h>

// producer states
char *producer_states[] = { "sleep", "producing", "export", "waiting" };

// consumer states
char *consumer_states[] = { "sleep", "hungry", "consuming" };


DWORD WINAPI reportThread(LPVOID lpParam);

// Character buffers used during XML parsing
char consumer_report[4096];
char buffer_report[4096];
char producer_report[4096];
char consumer_line[512];
char resource_line[512];
char producer_line[512];

/**
 * Initialize the XML report server communication thread
 */
int start_report_thread() {
	// initialize the XML library and check potential API mismatches
	LIBXML_TEST_VERSION

	// Create the thread to handle server socket use
	reportThreadHandle = CreateThread(NULL, 0, reportThread, NULL, 0, &reportThreadID);

	// If CreateThread fails, terminate execution. 
	// This will automatically clean up threads and memory. 
	if (reportThreadHandle == NULL)	{
		ErrorHandler(TEXT("CreateThread"));
		ExitProcess(3);
	}

	return 0;
}

/**
 * Shutdown the communication thread.
 */
void report_shutdown() {

	// Free the memory allocated by the parser.
	xmlCleanupParser();

	// Wait until all threads have terminated.
	WaitForSingleObject(reportThreadHandle, INFINITE);

	// Close all thread handles and free memory allocations.
	CloseHandle(reportThreadHandle);
}

/**
 * Thread handler, call out to the socket class.
 */
DWORD WINAPI reportThread(LPVOID lpParam) {
	monitor_connect_and_monitor();
	return 0;
}

/**
 * Write XML to the given socket
 */
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
	if (debug.print) printf("wrote to socket:\n%s", (char *)xmlbuff);

	/*
	* Free associated memory.
	*/
	xmlFree(xmlbuff);
	xmlFreeDoc(doc);

	return (0);

}

/**
 * The following consumer/resource/producer structs
 * are used to assist XML parsing when XML reports are
 * received from the Linux server.
 */
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

/**
 * Parse a <consumer> node into the consumer_line buffer
 */
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
		c->id, c->resources_consumed, consumer_states[atoi(c->status)]);
	xmlFree(c->id);
	xmlFree(c->resources_consumed);
	xmlFree(c->status);
	free(c);
}

/**
* Parse a <resource> node into the resource_line buffer
*/
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

/**
* Parse a <producer> node into the producer_line buffer
*/
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
		c->id, c->count, producer_states[atoi(c->status)]);
	xmlFree(c->id);
	xmlFree(c->count);
	xmlFree(c->status);
	free(c);
}

/**
* Parse a <consumers> node into the consumer_report buffer
*/
void monitor_xml_parse_consumers(xmlNode * a_node) {
	if (a_node == NULL) {
		memset(consumer_report, '\0', sizeof(consumer_report));
		strcat_s(consumer_report, sizeof(consumer_report), "No consumers.");
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
	}
}

/**
* Parse a <buffer> into the buffer_report buffer
*/
void monitor_xml_parse_buffer(xmlNode * a_node) {
	if (a_node == NULL) {
		memset(buffer_report, '\0', sizeof(buffer_report));
		strcat_s(buffer_report, sizeof(buffer_report), "No resources.");
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
	}

}

/**
* Parse a <producers> into the producer_report buffer
*/
void monitor_xml_parse_producers(xmlNode * a_node) {
	if (a_node == NULL) {
		memset(producer_report, '\0', sizeof(producer_report));
		strcat_s(producer_report, sizeof(producer_report), "No producers.");
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
	}
}

/**
 * Traverse through an XML report until the <report> node is found.
 * Then parse the children <consumers> <buffer> and <producers> node of 
 * the <report> node.
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

/**
 * Parse the report data received from the Linux server.
 * Expeted format:
 * <report>
 *  <consumers>
 *    <consumer />
 *  </consumers>
 *  <buffer>
 *    <resource />
 *  </buffer>
 *  <producers>
 *    <producer />
 *  </producers>
 * </report>
 */
int monitor_xml_parse_report(char *message) {
	xmlDoc *doc = NULL;
	xmlNode *root_element = NULL;

	// parse the file and get the DOM
	doc = xmlParseMemory(message, strlen(message));

	if (doc == NULL) {
		printf("error: could not parse file\n");
	}

	// get the root element node
	root_element = xmlDocGetRootElement(doc);

	// parse the XML into char arrays
	monitor_xml_parse_report_recursive(root_element);

	// update the view panes
	viewport_update_panes(consumer_report, buffer_report, producer_report);

	// free the document
	xmlFreeDoc(doc);

	return 0;
}