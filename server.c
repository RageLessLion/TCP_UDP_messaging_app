#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <math.h>

#include <ctype.h>

#define MAX_CLIENTS 2000
#define MAX_ID_LENGTH 10
#define MAX_UDP_CLIENTS 1000
#define MAX_TCP_CLIENTS 1000
#define MAX_TOPIC_NAME_LENGTH 100
#define MAX_SUBSCRIBERS 100
#define MAX_TOPICS 1000
#define MAX_TOPIC_NAME 1000
#define MAX_CLIENT_SOCKETS_COUNT 2000

int tcp_clients[MAX_TCP_CLIENTS];
int tcp_client_count = 0;

struct sockaddr_in udp_clients[MAX_UDP_CLIENTS];
int udp_client_count = 0;

char client_ids[MAX_CLIENTS][MAX_ID_LENGTH + 1];
int client_sockets[MAX_CLIENT_SOCKETS_COUNT];
int client_sockets_count = 0;

typedef struct
{
    int socket;
    char id[MAX_ID_LENGTH + 1];
} client_info;

struct topic
{
    char name[MAX_TOPIC_NAME_LENGTH + 1];
    int subscribers[MAX_SUBSCRIBERS];
    int subscriber_count;
};

struct topic topics[MAX_TOPICS];
int topic_count = 0;

void publish_message(char *topic_name, char *message)
{
    int i;
    for (i = 0; i < topic_count; i++)
    {
        if (strcmp(topics[i].name, topic_name) == 0)
        {
            for (int j = 0; j < topics[i].subscriber_count; j++)
            {
                int client_socket = topics[i].subscribers[j];
                send(client_socket, message, strlen(message), 0);
            }
            return;
        }
    }

    strcpy(topics[topic_count].name, topic_name);
    topics[topic_count].subscriber_count = 0;
    topic_count++;
}

void process_message(uint8_t *received_data, size_t data_length, struct sockaddr_in udp_client)
{
   
    char header[51];
    strncpy(header, (char *)received_data, 50);
    header[50] = '\0'; 

    
    int i;
    for (i = 0; i < topic_count; i++)
    {
        if (strcmp(topics[i].name, header) == 0)
        {
            break;
        }
    }

    
    if (i == topic_count && topic_count < MAX_TOPICS)
    {
        strncpy(topics[topic_count].name, header, MAX_TOPIC_NAME_LENGTH);
        topics[topic_count].name[MAX_TOPIC_NAME_LENGTH] = '\0'; 
        topics[topic_count].subscriber_count = 0;
        topic_count++;
    }
    char *udp_client_ip = inet_ntoa(udp_client.sin_addr);
    int udp_client_port = ntohs(udp_client.sin_port);
    // Data Type
    uint8_t data_type = received_data[50];
    if (data_type == 0)
    {
        int payload_offset = 51; 

        uint8_t sign_byte = received_data[payload_offset];
        uint32_t unsigned_payload;
        memcpy(&unsigned_payload, received_data + payload_offset + 1, sizeof(uint32_t));
        unsigned_payload = ntohl(unsigned_payload);

        int32_t payload = unsigned_payload;
        if (sign_byte == 1)
        {
            payload = -payload;
        }
        // Format payload string
        char toSendPayload[256]; 
        sprintf(toSendPayload, "%s:%d - %s - %s - %d\n", udp_client_ip, udp_client_port, topics[i].name, "INT", payload);

        
        publish_message(topics[i].name, toSendPayload);
    }
    else if (data_type == 1)
    {
        int payload_offset = 51; 

        uint16_t unsigned_payload1;
        memcpy(&unsigned_payload1, received_data + payload_offset, sizeof(uint16_t));
        unsigned_payload1 = ntohs(unsigned_payload1);

        double payload1 = unsigned_payload1 / 100.0;

        if (data_length > payload_offset + sizeof(uint16_t))
        {
            uint16_t unsigned_payload2;
            memcpy(&unsigned_payload2, received_data + payload_offset + sizeof(uint16_t), sizeof(uint16_t));
            unsigned_payload2 = ntohs(unsigned_payload2);

            double payload2 = unsigned_payload2 / 100.0;
            printf("Payload (as fraction): %.2f / %.2f\n", payload1, payload2);
        }
        else
        {
            char toSendPayload[256];
            sprintf(toSendPayload, "%s:%d - %s - %s - %f\n", udp_client_ip, udp_client_port, topics[i].name, "SHORT_REAL", payload1);
            publish_message(topics[i].name, toSendPayload);
        }
    }
    else if (data_type == 2)
    {
        int payload_offset = 51; // String + data type

        uint8_t sign_byte = received_data[payload_offset];

        uint32_t unsigned_payload;
        memcpy(&unsigned_payload, received_data + payload_offset + 1, sizeof(uint32_t));
        unsigned_payload = ntohl(unsigned_payload);

        uint8_t exponent_byte = received_data[payload_offset + 1 + sizeof(uint32_t)];

        double payload = unsigned_payload * pow(10, -exponent_byte);
        if (sign_byte == 1)
        {
            payload = -payload;
        }

        char payload_str[32];
        sprintf(payload_str, "%.10f", payload);

        // scoate 0 urile
        char *dot = strchr(payload_str, '.');
        if (dot != NULL)
        {
            char *end = payload_str + strlen(payload_str) - 1;
            while (end > dot && *end == '0')
            {
                *end-- = '\0';
            }
            if (end == dot)
            {
                *end = '\0';
            }
        }
        char toSendPayload[256];
        sprintf(toSendPayload, "%s:%d - %s - %s - %s\n", udp_client_ip, udp_client_port, topics[i].name, "FLOAT", payload_str);
        publish_message(topics[i].name, toSendPayload);
    }
    else if (data_type == 3)
    {
        char payload[1501];
        char toSendPayload[1501];
        strncpy(payload, (char *)(received_data + 51), 1500 * sizeof(char));
        payload[1500] = '\0';
        sprintf(toSendPayload, "%s:%d - %s - %s - %s\n", udp_client_ip, udp_client_port, topics[i].name, "STRING", payload);
        publish_message(topics[i].name, toSendPayload);
    }
}

void *client_handler(void *arg)
{
    client_info *info = (client_info *)arg;
    int client_socket = info->socket;
    char *id = info->id;

    char buffer[256];
    int n;

    while (1)
    {
        bzero(buffer, 256);
        n = read(client_socket, buffer, 255);
        if (n > 0)
        {
            buffer[n] = '\0'; 
            if (buffer[n - 1] == '\n')
            {                        
                buffer[n - 1] = '\0'; 
            }
            if (strncmp(buffer, "subscribe", 9) == 0)
            {
                char *topic = strtok(buffer + 10, " "); 
                if (topic != NULL)
                {
                    int topic_found = 0;
                    for (int i = 0; i < MAX_TOPICS; i++)
                    {
                        if (strcmp(topics[i].name, topic) == 0)
                        {
                            topic_found = 1;
                            // Add the client to the list of subscribers
                            topics[i].subscribers[topics[i].subscriber_count++] = client_socket;
                            char msg[256];
                            sprintf(msg, "Subscribed to topic %s", topic);
                            write(client_socket, msg, strlen(msg));
                            break;
                        }
                    }
                    if (!topic_found)
                    {
                        char msg[256];
                        sprintf(msg, "Topic %s not found", topic);
                        write(client_socket, msg, strlen(msg));
                    }
                }
            }
            else if (strncmp(buffer, "unsubscribe", 11) == 0)
            {
                char *topic = strtok(buffer + 12, " ");
                if (topic != NULL)
                {
                    int topic_found = 0;
                    for (int i = 0; i < MAX_TOPICS; i++)
                    {
                        if (strcmp(topics[i].name, topic) == 0)
                        {
                            for (int j = 0; j < topics[i].subscriber_count; j++)
                            {
                                if (topics[i].subscribers[j] == client_socket)
                                {
                                    for (int k = j; k < topics[i].subscriber_count - 1; k++)
                                    {
                                        topics[i].subscribers[k] = topics[i].subscribers[k + 1];
                                    }
                                    topics[i].subscriber_count--; 
                                    char msg[256];
                                    sprintf(msg, "Unsubscribed to topic %s", topic);
                                    write(client_socket, msg, strlen(msg));
                                    break;
                                }
                            }
                            break;
                        }
                    }
                }
            }
            else if (strncmp(buffer, "exit", 4) == 0)
            {
                
                for (int i = 0; i < MAX_CLIENTS; i++)
                {
                    if (strcmp(client_ids[i], id) == 0)
                    {
                        client_ids[i][0] = '\0';

                        
                        int j;
                        for (j = 0; j < client_sockets_count; j++)
                        {
                            if (client_sockets[j] == client_socket)
                            {
                                break;
                            }
                        }

                        
                        for (; j < client_sockets_count - 1; j++)
                        {
                            client_sockets[j] = client_sockets[j + 1];
                        }

                        client_sockets_count--;

                        break;
                    }
                }
                printf("Client C%s disconnected.\n", id);
                fflush(stdout);
                close(client_socket);
                free(info);
                break;
            }
        }
    }
}

int create_socket()
{
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        perror("Error opening socket");
        exit(1);
    }
    return server_socket;
}

void bind_socket(int server_socket, struct sockaddr_in server_address)
{
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        perror("Error on binding");
        exit(1);
    }
}

void listen_socket(int server_socket)
{
    if (listen(server_socket, 5) < 0)
    {
        perror("Error on listen");
        exit(1);
    }
}

int accept_client(int server_socket, struct sockaddr_in client_address, socklen_t client_len)
{
    int client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_len);
    if (client_socket < 0)
    {
        perror("Error on accept");
    }
    return client_socket;
}

int check_id_in_use(char *id)
{
    int id_in_use = 0;
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (strcmp(client_ids[i], id) == 0)
        {
            printf("Client C%s already connected.\n", id);
            fflush(stdout);
            id_in_use = 1;
            break;
        }
    }
    return id_in_use;
}

void store_client_id(char *id)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (client_ids[i][0] == '\0')
        {
            strncpy(client_ids[i], id, MAX_ID_LENGTH);
            client_ids[i][MAX_ID_LENGTH] = '\0';
            break;
        }
    }
}

void create_client_thread(int client_socket, char *id)
{
    pthread_t client_thread;
    client_info *info = malloc(sizeof(client_info));
    info->socket = client_socket;
    strncpy(info->id, id, MAX_ID_LENGTH + 1);
    if (pthread_create(&client_thread, NULL, client_handler, (void *)info) < 0)
    {
        perror("Could not create thread");
        exit(1);
    }
    pthread_detach(client_thread);
}

int main(int argc, char *argv[])
{
    int server_socket, client_socket, udp_socket;
    char *closeMessage = "close\n";
    int n;
    fd_set readfds;
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        client_ids[i][0] = '\0';
    }
    server_socket = create_socket();
    // define server address
    struct sockaddr_in server_address, client_address, udp_client_address;
    socklen_t client_len = sizeof(client_address);
    socklen_t udp_client_len = sizeof(udp_client_address);
    server_address.sin_port = htons(atoi(argv[1]));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;

    bind_socket(server_socket, server_address);
    listen_socket(server_socket);

    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket == -1)
    {
        perror("Could not create UDP socket");
        exit(EXIT_FAILURE);
    }

    // Bind UDP socket
    if (bind(udp_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        perror("UDP bind failed");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        FD_ZERO(&readfds);
        FD_SET(server_socket, &readfds);
        FD_SET(udp_socket, &readfds);
        FD_SET(STDIN_FILENO, &readfds);
        if (select(FD_SETSIZE, &readfds, NULL, NULL, NULL) < 0)
        {
            perror("Select failed");
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(server_socket, &readfds))
        {
            client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_len);
            if (client_socket < 0)
            {
                continue;
            }

            char id[MAX_ID_LENGTH + 1];
            bzero(id, MAX_ID_LENGTH + 1);
            n = read(client_socket, id, MAX_ID_LENGTH);
            if (n < 0)
            {
                printf("Error reading from socket");
                fflush(stdout); // Flush the output buffer
                close(client_socket);
                continue;
            }

            if (check_id_in_use(id))
            {
                send(client_socket, closeMessage, strlen(closeMessage), 0);
                close(client_socket);
                continue;
            }

            store_client_id(id);
            printf("New client C%s connected from %s:%d.\n", id, inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
            fflush(stdout); // Flush the output buffer

            create_client_thread(client_socket, id);
            client_sockets[client_sockets_count++] = client_socket;
        }

        if (FD_ISSET(udp_socket, &readfds))
        {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            char buffer[512];
            bzero(buffer, 512);
            n = recvfrom(udp_socket, buffer, 512, 0, (struct sockaddr *)&client_addr, &client_len);
            if (n < 0)
            {
                printf("ERROR reading from UDP socket");
                fflush(stdout); // Flush the output buffer
            }

        
            process_message((uint8_t *)buffer, n, client_addr); // Call our processing function
        }
    }
    for (int i = 0; i < client_sockets_count; i++)
    {
        send(client_sockets[i], closeMessage, strlen(closeMessage), 0);
        close(client_sockets[i]);
    }
    close(server_socket);

    return 0;
}