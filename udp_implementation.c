#include <stdio.h>
#include <stdbool.h>
#include <string.h>

    #define PLATFORM_WINDOWS    1
    #define PLATFORM_UNIX       2

    #if defined(_WIN32)
        #define PLATFORM PLATFORM_WINDOWS
    #else
        #define PLATFORM PLATFORM_UNIX
    #endif

    #if PLATFORM == PLATFORM_WINDOWS
        #include <winsock2.h>
        #include <ws2tcpip.h>
    #else // PLATFORM == PLATFORM_UNIX
        #include <unistd.h>
        #include <sys/socket.h>
        #include <netinet/in.h>
        #include <fcntl.h>
        #include <arpa/inet.h>
        #define SOCKADDR        struct sockaddr
        #define SOCKADDR_IN     struct sockaddr_in
        #define INVALID_SOCKET  -1
    #endif

#define BUFFER_SIZE     256

inline unsigned int AssembleIpAddress(unsigned char bytes[static 4]) {
    return (unsigned int)((bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3]);
}

int main(void) {
    /////////////////////////////////////////
    // Initialize sockets (Windows specficic)
    /////////////////////////////////////////
    #if PLATFORM == PLATFORM_WINDOWS
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != NO_ERROR) {
            fprintf(stderr, "Socket initialization failed.\n");
            return 1;
        }
    #endif

    /////////////////////////////////////////
    // Create server socket
    /////////////////////////////////////////
    int handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (handle == INVALID_SOCKET) {
        fprintf(stderr, "Socket creation failed.\n");
        return 1;
    }

    /////////////////////////////////////////
    // Bind the socket to the port
    /////////////////////////////////////////
    int port = 21000;
    SOCKADDR_IN address = {
        .sin_family      = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port        = htons(port),
    };
    if (bind(handle, (SOCKADDR*)&address, sizeof(address)) < 0) {
        fprintf(stderr, "Socket binding failed.\n");
        return 1;
    }
    printf("Socket bind to port %d.\n", port);

    /////////////////////////////////////////
    // Default "blocking mode" to non-blocking
    /////////////////////////////////////////
    #if PLATFORM == PLATFORM_WINDOWS
        DWORD nonBlocking = 1;
        if (ioctlsocket(handle, FIONBIO, &nonBlocking) < 0) {
            fprintf(stderr, "Failed to set non-blocking.\n");
            return 1;
        }
    #else
        int nonBlocking = 1;
        if (fcntl(handle, F_SETFL, O_NONBLOCK, nonBlocking) < 0) {
            fprintf(stderr, "Failed to set non-blocking.\n");
            return 1;
        }
    #endif

    /////////////////////////////////////////
    // Setting up File Descriptor for Select() and initializaing data buffers
    /////////////////////////////////////////
    fd_set readfds;
    int maxFd = handle;
    char receiveBuffer[BUFFER_SIZE];
    char acknowledgement[] = "ACK";
    SOCKADDR_IN lastSender = { 0 };

    /////////////////////////////////////////
    // Main loop
    /////////////////////////////////////////
    while (1) {
        /////////////////////////////////////////
        // File Descriptor initalization. Adds the server UDP socket to the FD set
        /////////////////////////////////////////
        FD_ZERO(&readfds);
        FD_SET(handle, &readfds);

        /////////////////////////////////////////
        // Monitoring the FDs without timeout
        /////////////////////////////////////////
        if (select(maxFd + 1, &readfds, NULL, NULL, NULL) < 0) {
            #if PLARFORM == PLATFORM_WINDOWS
                fprintf(stderr, "Select error: %d\n", WSAGetLastError());
            #else
                perror("Select error");
            #endif
            break;
        }

        /////////////////////////////////////////
        // Checking server socket - receiving data
        /////////////////////////////////////////
        if (FD_ISSET(handle, &readfds)) {
            socklen_t senderLength = sizeof(lastSender);
            int receivedBytes = recvfrom(handle, receiveBuffer, BUFFER_SIZE-1, 0, (SOCKADDR*)&lastSender, &senderLength);
            if (receivedBytes > 0) {
                receiveBuffer[receivedBytes] = '\0';
                printf("Received data from: [%s:%d]\n", inet_ntoa(lastSender.sin_addr), ntohs(lastSender.sin_port));
                // printing to console received bytes...
                printf("[%s]\n", receiveBuffer);

                /////////////////////////////////////////
                // Sending back acknowledgement
                /////////////////////////////////////////
                int sentBytes = sendto(handle, acknowledgement, strlen(acknowledgement),
                                       0, (SOCKADDR*)&lastSender, sizeof(lastSender));

                if (sentBytes < 0) { fprintf(stderr, "Failed to send message"); }
                else printf("%d bytes sent to: [%s:%d]\n", sentBytes, inet_ntoa(lastSender.sin_addr),
                                                                      ntohs(lastSender.sin_port));
            }
        }
    }

    /////////////////////////////////////////
    // Closing socket and cleanup
    /////////////////////////////////////////
    #if PLATFORM == PLATFORM_WINDOWS
        closesocket(handle);
        WSACleanup();
    #else
        close(handle);
    #endif

    printf("Server stops\n");
    return 0;
}
