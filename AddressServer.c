#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <math.h>
#include <stddef.h>

void DieWithError(char *err)
{
    perror(err);
    exit(1);
}

// Structure to register addresses
typedef struct
{
    enum
    {
        login,
        logout,
        list,
        chat
    } message_type;
    unsigned int user_id;
    int port;
    int port2;
} addServer;

// structure to store logged in client addresses
typedef struct
{
    unsigned int user_id;
    int port;
    int port2;
} clients;

int main(int argc, char *argv[])
{
    int sock; /* Socket */
    int sock2;
    struct sockaddr_in servAddr; /* Local address */
    struct sockaddr_in pkAddr;
    struct sockaddr_in recv;
    unsigned int recvLen;
    struct sockaddr_in clntAddr; /* Client address */
    unsigned int cliAddrLen;     /* Length of incoming message */
    unsigned short servPort;     /* Server port */
    unsigned short pkPort = 27000;
    int recvMsgSize; /* Size of received message */
    clients logged[10];
    int count = 0;

    for (int i = 0; i < 10; i++)
    {
        logged[i].user_id = 0;
        logged[i].port = 0;
        logged[i].port2 = 0;
    }

    servPort = 27001;

    /* Create socket for sending/receiving datagrams */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("socket() failed");

    /* Construct local address structure */
    memset(&servAddr, 0, sizeof(servAddr));       /* Zero out structure */
    servAddr.sin_family = AF_INET;                /* Internet address family */
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    servAddr.sin_port = htons(servPort);          /* Local port */

    /* Bind to the local address */
    if (bind(sock, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
        DieWithError("bind() failed");

    char *servIP;
    servIP = "127.0.0.1";
    /* Construct the server address structure */
    memset(&pkAddr, 0, sizeof(pkAddr));         /* Zero out structure */
    pkAddr.sin_family = AF_INET;                /* Internet addr family */
    pkAddr.sin_addr.s_addr = inet_addr(servIP); /* Server IP address */
    pkAddr.sin_port = htons(pkPort);            /* Server port */

    /* Set the size of the in-out parameter */
    cliAddrLen = sizeof(clntAddr);

    addServer mesg;

    for (;;)
    {
        /* Block until receive message from  a client */
        if ((recvMsgSize = recvfrom(sock, &mesg, sizeof(mesg), 0,
                                    (struct sockaddr *)&clntAddr, &cliAddrLen)) < 0)
            DieWithError("recvfrom() failed");

        // Register client addresses
        if (mesg.message_type == login)
        {
            printf("Login request received from user %d\n", mesg.user_id);
            int temp = 0;
            temp = sendto(sock, &mesg, sizeof(mesg), 0, (struct sockaddr *)&clntAddr, sizeof(clntAddr));
            printf("Acknowledgement sent\n");

            logged[count].user_id = mesg.user_id;
            logged[count].port = mesg.port;
            logged[count].port2 = mesg.port2;

            count += 1;
            printf("%d user(s) logged in\n", count);

            for (int x = 0; x < count; x++)
            {
                if (logged[x].port != 0)
                {
                    /* Create a reliable, stream socket using TCP */
                    if ((sock2 = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
                        DieWithError("socket() failed");

                    struct sockaddr_in echoServAddr; /* Echo server address */
                    // unsigned short echoServPort = clntPort.port;

                    /* Construct the server address structure */
                    memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Zero out structure */
                    echoServAddr.sin_family = AF_INET;                /* Internet address family */
                    echoServAddr.sin_addr.s_addr = inet_addr(servIP); /* Server IP address */
                    echoServAddr.sin_port = htons(logged[x].port2);   /* Server port */

                    /* Establish the connection to the echo server */
                    if (connect(sock2, (struct sockaddr *)&echoServAddr, sizeof(echoServAddr)) < 0)
                        DieWithError("connect() failed");

                    sleep(1);
                    send(sock2, logged, sizeof(clients), 0);
                    close(sock2);
                }
            }
        }

        // send list of all logged in users
        else if (mesg.message_type == list)
        {
            int temp = 0;
            temp = sendto(sock, &logged, sizeof(logged), 0, (struct sockaddr *)&clntAddr, sizeof(clntAddr));
            printf("List of logged in users sent\n");
        }
        // send requested addresses
        else if (mesg.message_type == chat)
        {
            int y = 0;
            for (int x = 0; x < 10; x++)
            {
                if (logged[x].user_id == mesg.user_id)
                {
                    mesg.port = logged[x].port;
                    int temp = 0;
                    temp = sendto(sock, &mesg, sizeof(mesg), 0, (struct sockaddr *)&clntAddr, sizeof(clntAddr));
                    printf("Client address sent\n");
                    y = 1;
                }
            }
            if (y == 0)
            {
                printf("Client not found\n");
            }
        }
    }
}