#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
 
int server_listen(void) {
    int listenfd = 0,connfd = 0;

    struct sockaddr_in serv_addr;

    char sendBuff[1025];  
    char recvBuff[1025];
    int numrv;  
    int recvSize;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd > -1) {
        printf("socket retrieve success\n");
    }
    else {
        return -1;
    }

    memset(&serv_addr, '0', sizeof(serv_addr));
    memset(sendBuff, '0', sizeof(sendBuff));

    serv_addr.sin_family = AF_INET;    
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
    serv_addr.sin_port = htons(60118);    

    bind(listenfd, (struct sockaddr*)&serv_addr,sizeof(serv_addr));

    if(listen(listenfd, 10) == -1){
        printf("Failed to listen\n");
        return -1;
    }


    while(1) {
        connfd = accept(listenfd, (struct sockaddr*)NULL ,NULL); // accept awaiting request

        memset(recvBuff, '\0', sizeof(recvBuff));
        recvSize = read(connfd, recvBuff, 1024);
        if (recvSize < 0) {
            printf("ERROR reading from socket");
            return -1;
        }
        printf("Here is the message: %s\n",recvBuff);

        strcpy(sendBuff, "Message from server\n");
        write(connfd, sendBuff, strlen(sendBuff));
        printf("accept/write\n");

        close(connfd);    
        sleep(1);
    }


    return 0;
}
