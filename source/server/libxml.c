/* 
 * Author: Trevor
 *
 * Created on November 24, 2014, 8:22 AM
 */

#include "server.h"
#include <libxml/parser.h>

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
    write(socket, xmlbuff, buffersize);
    if (debug.print) printf("wrote to socket:\n%s", (char *) xmlbuff);

    /*
     * Free associated memory.
     */
    xmlFree(xmlbuff);
    xmlFreeDoc(doc);

    return (0);

}