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
#include <sys/mman.h>

void DieWithError(char *err)
{
    perror(err);
    exit(1);
}

// Define a sample struct
struct SampleStruct
{
    int userId;
    int port;
    int mess[32];
};

// Function to send a struct over a TCP socket
void sendStruct(int socket, struct SampleStruct *data)
{
    send(socket, data, sizeof(struct SampleStruct), 0);
}

// Function to receive a struct over a TCP socket
void receiveStruct(int socket, struct SampleStruct *data)
{
    recv(socket, data, sizeof(struct SampleStruct), 0);
}

// structure to register public key
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

// structure to register address
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

typedef struct
{
    unsigned int user_id;
    int port;
    int port2;
} clients;

// to encrypt the given number
int encrypt(double message, int key)
{
    int e = key;
    long long int encrpyted_text = 1;
    while (e--)
    {
        encrpyted_text *= message;
        encrpyted_text %= 221;
    }
    return encrpyted_text;
}
// to decrypt the given number
int decrypt(int encrpyted_text, int key)
{
    int d = key;
    long long int decrypted = 1;
    while (d--)
    {
        decrypted *= encrpyted_text;
        decrypted %= 221;
    }
    return decrypted;
}

int main(int argc, char *argv[])
{
    int sock; /* Socket descriptor */
    int sock2;
    struct sockaddr_in servAddr; /*server address */
    struct sockaddr_in recev;
    unsigned short servPort; /*server port */
    int userId;
    char *servIP; /* IP address of server */

    servIP = "127.0.0.1";
    servPort = 27000;

    /* Create a datagram/UDP socket */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("socket() failed");

    /* Construct the server address structure */
    memset(&servAddr, 0, sizeof(servAddr));       /* Zero out structure */
    servAddr.sin_family = AF_INET;                /* Internet addr family */
    servAddr.sin_addr.s_addr = inet_addr(servIP); /* Server IP address */
    servAddr.sin_port = htons(servPort);          /* Server port */

    int publicKey;
    int privateKey;
    int cKey;

    printf("Enter your user ID: ");
    scanf("%d", &userId);

    printf("Enter your private key: ");
    scanf("%d", &privateKey);

    printf("Enter your public key: ");
    scanf("%d", &publicKey);

    printf("Registering public key with the public key server...\n");

    keyServer pk = {register_key, userId, publicKey};

    int temp = 0;
    /* Send the string to the server */
    temp = sendto(sock, &pk, sizeof(pk), 0, (struct sockaddr *)&servAddr, sizeof(servAddr));

    keyServer fromPk;
    int recevSize;
    unsigned int fromSize = sizeof(recev);
    /* Block until receive message from  a client */
    if ((recevSize = recvfrom(sock, &fromPk, sizeof(fromPk), 0,
                              (struct sockaddr *)&recev, &fromSize) < 0))
        DieWithError("recvfrom() failed");

    printf("Acknowledgement received\n");

    pid_t child1, child2; // initialize child processes variables

    // create shared variables
    int *ptr;
    ptr = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    int *clntKey;
    clntKey = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    child1 = fork();

    // first child
    if (child1 == 0)
    {
        int mySock;                  /* Socket descriptor for server */
        int clntSock;                /* Socket descriptor for client */
        struct sockaddr_in myAddr;   /* Local address */
        struct sockaddr_in clntAddr; /* Client address */
        unsigned short myPort;       /* Server port */
        unsigned int clntLen;        /* Length of client address data structure */
        myPort = 27002;

        /* Create socket for incoming connections */
        if ((mySock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
            DieWithError("socket() failed");

        /* Construct local address structure */
        memset(&myAddr, 0, sizeof(myAddr));         /* Zero out structure */
        myAddr.sin_family = AF_INET;                /* Internet address family */
        myAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
        myAddr.sin_port = htons(myPort);            /* Local port */

        /* Bind to the local address */
        while (bind(mySock, (struct sockaddr *)&myAddr, sizeof(myAddr)) < 0)
        {
            myPort += 1;
            /* Construct local address structure */
            memset(&myAddr, 0, sizeof(myAddr));         /* Zero out structure */
            myAddr.sin_family = AF_INET;                /* Internet address family */
            myAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
            myAddr.sin_port = htons(myPort);            /* Local port */
        }

        *ptr = myPort;

        /* Mark the socket so it will listen for incoming connections */
        if (listen(mySock, 5) < 0)
            DieWithError("listen() failed");

        /* Set the size of the in-out parameter */
        clntLen = sizeof(clntAddr);

        for (;;)
        {
            /* Wait for a client to connect */
            if ((clntSock = accept(mySock, (struct sockaddr *)&clntAddr,
                                   &clntLen)) < 0)
                DieWithError("accept() failed");

            struct SampleStruct receivedData;
            receiveStruct(clntSock, &receivedData);
            if (receivedData.port == myPort)
            {
                close(clntSock);
                if ((clntSock = accept(mySock, (struct sockaddr *)&clntAddr,
                                       &clntLen)) < 0)
                    DieWithError("accept() failed");
            }
            else
            {
                printf("Connection request received from user %d\n", receivedData.userId);
                *clntKey = receivedData.mess[0];
                printf("Enter 8 to accept or 9 to decline\n");
                *ptr = receivedData.port;
            }

            while (1)
            {
                if (*ptr == 0)
                {
                    break;
                }
                receiveStruct(clntSock, &receivedData);
                printf("Message received\n");

                printf("1. Send message\n2. Close connection\n");
                if (receivedData.userId == 11)
                {
                    printf("Connection closed!\nEnter any number to exit\n");
                    exit(1);
                    break;
                }
                else if (receivedData.userId == 1000)
                {
                    printf("Connection declined!\n");
                    close(clntSock);
                    goto end;
                }
                else
                {
                    int myMessage[32];
                    for (int x = 0; x < receivedData.port; x++)
                    {
                        myMessage[x] = decrypt(receivedData.mess[x], privateKey);
                        printf("%d -> %d\n", receivedData.mess[x], myMessage[x]);
                    }
                    printf("Message: ");
                    for (int x = 0; x < receivedData.port; x++)
                    {
                        printf("%c", myMessage[x]);
                    }
                }
                printf("\n");
                printf("1. Send message\n2. Close connection\n");
            }
            printf("Connection closed!\n");
            close(clntSock);
            close(mySock);
            exit(1);
        end:
        }
        exit(EXIT_SUCCESS);
    }

    int *ptr2;
    ptr2 = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    child2 = fork();

    // child 2
    if (child2 == 0)
    {
        int mySock;                  /* Socket descriptor for server */
        int clntSock;                /* Socket descriptor for client */
        struct sockaddr_in myAddr;   /* Local address */
        struct sockaddr_in clntAddr; /* Client address */
        unsigned short myPort;       /* Server port */
        unsigned int clntLen;        /* Length of client address data structure */
        myPort = 27003;

        /* Create socket for incoming connections */
        if ((mySock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
            DieWithError("socket() failed");

        /* Construct local address structure */
        memset(&myAddr, 0, sizeof(myAddr));         /* Zero out structure */
        myAddr.sin_family = AF_INET;                /* Internet address family */
        myAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
        myAddr.sin_port = htons(myPort);            /* Local port */

        /* Bind to the local address */
        while (bind(mySock, (struct sockaddr *)&myAddr, sizeof(myAddr)) < 0)
        {
            myPort += 1;
            /* Construct local address structure */
            memset(&myAddr, 0, sizeof(myAddr));         /* Zero out structure */
            myAddr.sin_family = AF_INET;                /* Internet address family */
            myAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
            myAddr.sin_port = htons(myPort);            /* Local port */
        }

        *ptr2 = myPort;
        /* Mark the socket so it will listen for incoming connections */
        if (listen(mySock, 5) < 0)
            DieWithError("listen() failed");

        /* Set the size of the in-out parameter */
        clntLen = sizeof(clntAddr);

        for (;;)
        {
            /* Wait for a client to connect */
            if ((clntSock = accept(mySock, (struct sockaddr *)&clntAddr,
                                   &clntLen)) < 0)
                DieWithError("accept() failed");

            clients who[10];
            for (int i = 0; i < 10; i++)
            {
                who[i].user_id = 0;
                who[i].port = 0;
                who[i].port2 = 0;
            }

            recv(clntSock, &who, sizeof(clients), 0);
            printf("New user logged in\n");
            close(clntSock);
        }

        exit(EXIT_SUCCESS);
    }

    printf("Logging in...\n");
    sleep(1);
    servPort = 27001;

    /* Create a datagram/UDP socket */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("socket() failed");

    /* Construct the server address structure */
    memset(&servAddr, 0, sizeof(servAddr));       /* Zero out structure */
    servAddr.sin_family = AF_INET;                /* Internet addr family */
    servAddr.sin_addr.s_addr = inet_addr(servIP); /* Server IP address */
    servAddr.sin_port = htons(servPort);          /* Server port */

    addServer logn = {login, userId, *ptr, *ptr2};

    /* Send the string to the server */
    temp = sendto(sock, &logn, sizeof(logn), 0, (struct sockaddr *)&servAddr, sizeof(servAddr));

    addServer ack;
    /* Block until receive message from  a client */
    if ((recevSize = recvfrom(sock, &ack, sizeof(ack), 0,
                              (struct sockaddr *)&recev, &fromSize) < 0))
        DieWithError("recvfrom() failed");

    printf("Log in successful!\n");
    int acpt = 0;
    struct SampleStruct receivedData;
    struct SampleStruct sendData;

    int receiver;
    for (;;)
    {
        if (acpt == 1) // if a connection has been established
        {
            int y;
            printf("1. Send message\n2. Close connection\n");
            for (;;)
            {
                scanf("%d", &y);
                if (*ptr == 0)
                {
                    break;
                }
                if (y == 1)
                {
                    // get message from keyboard
                    char str[32];
                    printf("Enter message: ");
                    fgetc(stdin);
                    fgets(str, sizeof(str), stdin);

                    if (receiver == 1)
                    {
                        cKey = *clntKey;
                    }

                    int asciiValues[32];
                    int counter;
                    for (int i = 0; str[i] != '\0'; i++)
                    {
                        // Store ASCII values in the array
                        asciiValues[i] = str[i];

                        // Print the character and its corresponding ASCII value
                        receivedData.mess[i] = encrypt(asciiValues[i], cKey);
                        printf("'%c' -> %d -> %d\n", str[i], asciiValues[i], receivedData.mess[i]);
                        counter += 1;
                    }

                    receivedData.userId = 100;
                    receivedData.port = counter;
                    sendStruct(sock, &receivedData); // send the message
                    printf("Message sent!\n");
                }
                else if (y == 2) // if you wish to quit
                {
                    receivedData.userId = 11;
                    receivedData.port = 0;
                    sendStruct(sock, &receivedData);
                    *ptr = 0;
                    exit(1);
                    break;
                }

                else
                {
                    printf("Invalid input\n");
                }
            }
        }
        // if chat request has been declined
        else if (acpt == 0)
        {
            receivedData.userId = 1000;
            receivedData.port = 1000;
            sendStruct(sock, &receivedData);
        }
        printf("1. See logged in users\n2. Initiate chat session\n3. Quit\n"); // menu options
        int input;
        scanf("%d", &input);
        if (input == 1)
        {
            addServer logn = {list, userId, *ptr, *ptr2};
            /* Send the string to the server */
            temp = sendto(sock, &logn, sizeof(logn), 0, (struct sockaddr *)&servAddr, sizeof(servAddr));

            clients lst[10];
            for (int i = 0; i < 10; i++)
            {
                lst[i].user_id = 0;
                lst[i].port = 0;
                lst[i].port2 = 0;
            }
            /* Block until receive message from  a client */
            if ((recevSize = recvfrom(sock, &lst, sizeof(lst), 0,
                                      (struct sockaddr *)&recev, &fromSize) < 0))
                DieWithError("recvfrom() failed");

            int count = 0;
            // list logged in users
            for (int i = 0; i < 10; i++)
            {
                if (lst[i].user_id < 11 && lst[i].user_id > 0)
                {
                    count += 1;
                }
            }
            printf("Logged in clients: %d\n", count);
            for (int x = 0; x < count; x++)
            {
                printf("Id: %d\n", lst[x].user_id);
            }
        }
        else if (input == 2) // to establish a connection
        {
            int clientId;
            printf("Enter client you want to chat with: ");
            scanf("%d", &clientId);

            servPort = 27001;

            /* Create a datagram/UDP socket */
            if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
                DieWithError("socket() failed");

            /* Construct the server address structure */
            memset(&servAddr, 0, sizeof(servAddr));       /* Zero out structure */
            servAddr.sin_family = AF_INET;                /* Internet addr family */
            servAddr.sin_addr.s_addr = inet_addr(servIP); /* Server IP address */
            servAddr.sin_port = htons(servPort);          /* Server port */

            addServer clntPort = {chat, clientId, 0};
            /* Send the string to the server */
            temp = sendto(sock, &clntPort, sizeof(clntPort), 0, (struct sockaddr *)&servAddr, sizeof(servAddr));

            /* Block until receive message from  a client */
            if ((recevSize = recvfrom(sock, &clntPort, sizeof(clntPort), 0,
                                      (struct sockaddr *)&recev, &fromSize) < 0))
                DieWithError("recvfrom() failed");

            printf("Client address received:\n");

            if (clntPort.port != 0)
            {
                servPort = 27000;

                /* Create a datagram/UDP socket */
                if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
                    DieWithError("socket() failed");

                /* Construct the server address structure */
                memset(&servAddr, 0, sizeof(servAddr));       /* Zero out structure */
                servAddr.sin_family = AF_INET;                /* Internet addr family */
                servAddr.sin_addr.s_addr = inet_addr(servIP); /* Server IP address */
                servAddr.sin_port = htons(servPort);          /* Server port */

                keyServer clntKey = {request_public_key, clientId, 0};
                /* Send the string to the server */
                temp = sendto(sock, &clntKey, sizeof(clntKey), 0, (struct sockaddr *)&servAddr, sizeof(servAddr));

                /* Block until receive message from  a client */
                if ((recevSize = recvfrom(sock, &clntKey, sizeof(clntKey), 0,
                                          (struct sockaddr *)&recev, &fromSize) < 0))
                    DieWithError("recvfrom() failed");

                cKey = clntKey.public_key;
                printf("Client public key received: %d\n", clntKey.public_key);

                /* Create a reliable, stream socket using TCP */
                if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
                    DieWithError("socket() failed");

                struct sockaddr_in echoServAddr; /* Echo server address */
                // unsigned short echoServPort = clntPort.port;

                /* Construct the server address structure */
                memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Zero out structure */
                echoServAddr.sin_family = AF_INET;                /* Internet address family */
                echoServAddr.sin_addr.s_addr = inet_addr(servIP); /* Server IP address */
                echoServAddr.sin_port = htons(*ptr);              /* Server port */

                /* Establish the connection to the echo server */
                if (connect(sock, (struct sockaddr *)&echoServAddr, sizeof(echoServAddr)) < 0)
                    DieWithError("connect() failed");

                sendData.userId = userId;
                sendData.port = *ptr;
                sendData.mess[0] = publicKey;

                sendStruct(sock, &sendData);

                /* Create a reliable, stream socket using TCP */
                if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
                    DieWithError("socket() failed");

                // struct sockaddr_in echoServAddr; /* Echo server address */
                unsigned short echoServPort = clntPort.port;

                /* Construct the server address structure */
                memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Zero out structure */
                echoServAddr.sin_family = AF_INET;                /* Internet address family */
                echoServAddr.sin_addr.s_addr = inet_addr(servIP); /* Server IP address */
                echoServAddr.sin_port = htons(echoServPort);      /* Server port */

                /* Establish the connection to the echo server */
                if (connect(sock, (struct sockaddr *)&echoServAddr, sizeof(echoServAddr)) < 0)
                    DieWithError("connect() failed");

                sendData.userId = userId;
                sendData.port = *ptr;

                sendStruct(sock, &sendData);
                printf("Chat request sent\n");
                acpt = 1;
            }
            else
            {
                printf("Client not found!\n"); // if address not found
            }
        }

        else if (input == 3)
        {
            servPort = 27001;

            /* Create a datagram/UDP socket */
            if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
                DieWithError("socket() failed");

            /* Construct the server address structure */
            memset(&servAddr, 0, sizeof(servAddr));       /* Zero out structure */
            servAddr.sin_family = AF_INET;                /* Internet addr family */
            servAddr.sin_addr.s_addr = inet_addr(servIP); /* Server IP address */
            servAddr.sin_port = htons(servPort);          /* Server port */

            addServer logn = {logout, userId, *ptr};

            /* Send the string to the server */
            temp = sendto(sock, &logn, sizeof(logn), 0, (struct sockaddr *)&servAddr, sizeof(servAddr));
            printf("Program terminated!\n");
            exit(1);
        }
        else if (input == 8)
        {

            /* Create a reliable, stream socket using TCP */
            if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
                DieWithError("socket() failed");

            struct sockaddr_in echoServAddr; /* Echo server address */

            unsigned short echoServPort = *ptr;

            /* Construct the server address structure */
            memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Zero out structure */
            echoServAddr.sin_family = AF_INET;                /* Internet address family */
            echoServAddr.sin_addr.s_addr = inet_addr(servIP); /* Server IP address */
            echoServAddr.sin_port = htons(echoServPort);      /* Server port */

            /* Establish the connection to the echo server */
            if (connect(sock, (struct sockaddr *)&echoServAddr, sizeof(echoServAddr)) < 0)
                DieWithError("connect() failed");
            printf("Connection accepted\n");
            acpt = 1;
            receiver = 1;
        }
        else if (input == 9)
        {

            /* Create a reliable, stream socket using TCP */
            if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
                DieWithError("socket() failed");

            struct sockaddr_in echoServAddr; /* Echo server address */

            unsigned short echoServPort = *ptr;

            /* Construct the server address structure */
            memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Zero out structure */
            echoServAddr.sin_family = AF_INET;                /* Internet address family */
            echoServAddr.sin_addr.s_addr = inet_addr(servIP); /* Server IP address */
            echoServAddr.sin_port = htons(echoServPort);      /* Server port */

            /* Establish the connection to the echo server */
            if (connect(sock, (struct sockaddr *)&echoServAddr, sizeof(echoServAddr)) < 0)
                DieWithError("connect() failed");
            printf("Connection declined\n");
            acpt = 0;
        }
        else
        {
            printf("Invalid input!\n");
        }
    }
}