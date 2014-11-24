/* 
 * Author: Trevor
 *
 * Created on November 24, 2014, 8:22 AM
 */

#include "server.h"
#include <libxml/parser.h>

int xml_write_message(int socket, char *message) {

    xmlNodePtr root_node;
    xmlDocPtr doc;
    xmlChar *xmlbuff;
    int buffersize;

    /*
     * Create the document.
     */
    doc = xmlNewDoc(BAD_CAST "1.0");
    root_node = xmlNewNode(NULL, BAD_CAST "message");
    xmlDocSetRootElement(doc, root_node);

    /* 
     * xmlNewChild() creates a new node, which is "attached" as child node
     * of root_node node. 
     */
    xmlNewChild(root_node, NULL, BAD_CAST "node1",
                BAD_CAST "content of node 1");
    /* 
     * The same as above, but the new child node doesn't have a content 
     */
    xmlNewChild(root_node, NULL, BAD_CAST "node2", NULL);


    /*
     * Dump the document to a buffer and print it
     * for demonstration purposes.
     */
    xmlDocDumpFormatMemory(doc, &xmlbuff, &buffersize, 1);
    write(socket, xmlbuff, buffersize);
    if (debug.print) printf("wrote to socket:\n%s", (char *) xmlbuff);

    /*
     * Free associated memory.
     */
    xmlFree(xmlbuff);
    xmlFreeDoc(doc);

    return (0);

}