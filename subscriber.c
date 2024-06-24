#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_CLIENTS 10
#define MAX_ID_LENGTH 10

void *receive_messages(void *socket)
{
    setbuf(stdout, NULL);
    int client_socket = *((int *)socket);
    char buffer[1024];
    while (1)
    {
        bzero(buffer, 1024);
        ssize_t n = read(client_socket, buffer, 1024);
        if (n < 0)
        {
            break;
        }
        else if (n == 0)
        {
            break;
        }
        else
        {
            if (strcmp(buffer, "close\n") == 0)
            {
                exit(0);
            }
            if (buffer[n - 1] == '\n') // If the last character is a newline
            {
                buffer[n - 1] = '\0'; // Replace it with a null terminator
            }
            printf("%s\n", buffer); // Print the message with a newline
            fflush(stdout);
        }
    }
    close(client_socket);
    exit(0);
    return NULL;
}

int main(int argc, char *argv[])
{
    setbuf(stdout, NULL);
    int client_socket;
    client_socket = socket(AF_INET, SOCK_STREAM, 0);

    // define server address
    struct sockaddr_in server_address;
    server_address.sin_port = htons(atoi(argv[3]));
    server_address.sin_family = AF_INET;
    if (inet_pton(AF_INET, argv[2], &(server_address.sin_addr.s_addr)) <= 0)
    {
        printf("Invalid address / Address not supported \n");
        fflush(stdout); // Flush the output buffer
    }

    // connect to the server
    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        printf("Connection failed \n");
        fflush(stdout); // Flush the output buffer
        return -1;
    }

    // Send client ID to the server
    if (write(client_socket, argv[1] + 1, strlen(argv[1]) - 1) < 0)
    {
        fflush(stdout); // Flush the output buffer
        close(client_socket);
        
    }

    // Create a new thread for receiving messages
    pthread_t receive_thread;
    if (pthread_create(&receive_thread, NULL, receive_messages, &client_socket) < 0)
    {
        perror("could not create thread");
        fflush(stdout); // Flush the output buffer
    }

    // Loop to send messages
    char buffer[256];
    do
    {
        fgets(buffer, 255, stdin);
        write(client_socket, buffer, strlen(buffer));
    } while (strncmp(buffer, "exit", 4) != 0);

    close(client_socket);
    return 0;
}