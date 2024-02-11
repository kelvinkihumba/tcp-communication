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

typedef struct
{
    enum
    {
        register_key,
        request_public_key
    } message_type;
    unsigned int user_id;
    unsigned int public_key;
} keyServer;

typedef struct
{
    unsigned int user_id;
    unsigned int public_key;
} clients;

int main(int argc, char *argv[])
{
    int sock;                    /* Socket */
    struct sockaddr_in servAddr; /* Local address */
    struct sockaddr_in clntAddr; /* Client address */
    struct sockaddr_in adAddr;
    unsigned int cliAddrLen; /* Length of incoming message */
    unsigned short servPort; /* Server port */
    int recvMsgSize;         /* Size of received message */
    clients client[10];
    int count = 0;

    servPort = 27000;

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

    /* Set the size of the in-out parameter */
    cliAddrLen = sizeof(clntAddr);

    keyServer pk;

    for (;;)
    {
        /* Block until receive message from  a client */
        if ((recvMsgSize = recvfrom(sock, &pk, sizeof(pk), 0,
                                    (struct sockaddr *)&clntAddr, &cliAddrLen)) < 0)
            DieWithError("recvfrom() failed");

        if (pk.message_type == register_key) // register public key
        {
            printf("Public key received from user %d\n", pk.user_id);
            client[count].user_id = pk.user_id;
            client[count].public_key = pk.public_key;
            count += 1;

            int temp = 0;
            temp = sendto(sock, &pk, sizeof(pk), 0, (struct sockaddr *)&clntAddr, sizeof(clntAddr));

            printf("Public key registered!\n");
        }
        else if (pk.message_type == request_public_key) // send requested public key
        {
            printf("Public key requested\n");

            for (int x = 0; x < 10; x++)
            {
                if (client[x].user_id == pk.user_id)
                {
                    // send acknowledgement
                    keyServer pk = {request_public_key, client[x].user_id, client[x].public_key};
                    int temp = 0;
                    temp = sendto(sock, &pk, sizeof(pk), 0, (struct sockaddr *)&clntAddr, sizeof(clntAddr));

                    printf("Public key sent\n");
                }
            }
        }
    }
}
