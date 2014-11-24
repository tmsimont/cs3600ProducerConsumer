/*
* Author: Trevor
*
* Created on November 24, 2014, 8:22 AM
*/

#include "monitor.h"
#include <libxml/parser.h>
#include <libxml/tree.h>

int xml_write_message(int socket, char *message) {
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

/**
* print_element_names:
* @a_node: the initial xml node to consider.
*
* Prints the names of the all the xml elements
* that are siblings or children of a given xml node.
*/
static void print_element_names(xmlNode * a_node)
{
	xmlNode *cur_node = NULL;
	xmlChar *content = xmlNodeGetContent(cur_node);

	for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
		if (cur_node->type == XML_ELEMENT_NODE) {
			printf("node type: Element --\n name: %s\n  content: %s\n", cur_node->name, content);
		}

		print_element_names(cur_node->children);
	}
	xmlFree(content);
}


int xml_parse_message(char *message) {
	xmlDoc *doc = NULL;
	xmlNode *root_element = NULL;
	
	/*
	* this initialize the library and check potential ABI mismatches
	* between the version it was compiled for and the actual shared
	* library used.
	*/
	LIBXML_TEST_VERSION

	/*parse the file and get the DOM */
	doc = xmlParseMemory(message, strlen(message));

	if (doc == NULL) {
		printf("error: could not parse file\n");
	}

	/*Get the root element node */
	root_element = xmlDocGetRootElement(doc);

	print_element_names(root_element);

	/*free the document */
	xmlFreeDoc(doc);

	/*
	*Free the global variables that may
	*have been allocated by the parser.
	*/
	xmlCleanupParser();

	return 0;
}