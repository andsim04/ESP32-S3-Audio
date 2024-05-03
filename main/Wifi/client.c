/*
 * IPK.2023L
 *
 * Demonstration of trivial TCP client.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <limits.h>
#include <stdbool.h>
#include "esp_err.h"
#include "esp_log.h"

static const char *TAG_TCP = "client";

typedef struct
{
    char *server_hostname;
    char *port_string;
    char *path;
} Config;

typedef enum
{
    ERROR,
    SUCCESS,
} Status;

#define CHECK_ERROR(val) \
  if (val == ERROR)      \
  {                       \
    return EXIT_FAILURE; \
  }

#define CHECK_PERROR(condition, message, server_info) \
  if (condition)                                      \
  {                                                   \
    perror(message);                                  \
    freeaddrinfo(server_info);                        \
    return EXIT_FAILURE;                              \
  }

#define CHECK_ERROR_MESSAGE(condition, message, server_info) \
  if (condition)                                             \
  {                                                          \
    fprintf(stderr, message);                                \
    freeaddrinfo(server_info);                               \
    return EXIT_FAILURE;                                     \
  }


Status GetAddrForDomain(Config *config, struct addrinfo **server_info);

// Sending buffer size
#define SEND_BUFSIZE 1024

// Receive buffer size (<= 1) - does not really matter with TCP
#define RECEIVE_BUFSIZE 1024

// Terminate connection character
#define TERMINATE_CHAR '!'

int odosli_subor(char* hostName, int port, char* fileName)
{

    char send_buffer[SEND_BUFSIZE] = {0};
    Config config;
    config.server_hostname = hostName;
    config.port_string = port;
    config.path = fileName;
    // Check and parse arguments
   

    // Resolve domain to IP address
    struct addrinfo *server_info;
    CHECK_ERROR(GetAddrForDomain(&config, &server_info));

    // Create socket
    int client_socket = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
    CHECK_PERROR(client_socket < 0, "Socket", server_info);

    // Optionally set socket options with setsockopt

    // Set receive timeout to 2 seconds
    struct timeval timeval = {
            .tv_sec = 2};
    CHECK_PERROR(setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &timeval, sizeof(timeval)) < 0, "Setsockopt", server_info);

    // Connect to the remote server
    CHECK_PERROR(connect(client_socket, server_info->ai_addr, server_info->ai_addrlen) < 0, "Connect", server_info);

    // Get messages from stdin and send them to the server
    ESP_LOGE(TAG_TCP,"INFO: Connected to the server!\n");
    FILE *fp = fopen(config.path, "rb");
    int size;
    fseek(fp, 0L, SEEK_END);
    size = ftell(fp);
    rewind(fp);
    printf("%d\n", size);
    bool terminate_char_found = false;
    send(client_socket, &size, sizeof(int) , 0);
    int celkovo = 0;
    while (!feof(fp))
    {

        fread(send_buffer, SEND_BUFSIZE, 1, fp); 
        if (send(client_socket, send_buffer, sizeof(send_buffer), 0) <= 0) {
            ESP_LOGE(TAG_TCP, "No data has been sent to the server!\n");
        }
        
    }
    fclose(fp);
    char receive_buffer[RECEIVE_BUFSIZE] = {0};

     ESP_LOGE(TAG_TCP,"Terminating ...\n");

    // Close the socket
    close(client_socket);

    // Free addrInfo
    freeaddrinfo(server_info);

    return EXIT_SUCCESS;
}

// Get IPv4 address for domain and port in config
Status GetAddrForDomain(Config *config, struct addrinfo **server_info)
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_protocol = 0;           // Protocol

    int status = getaddrinfo(config->server_hostname, config->port_string, &hints, server_info);

    if (status != 0 || (*server_info)->ai_addr == NULL)
    {
        ESP_LOGE(TAG_TCP, "getaddrinfo: failed to resolve hostname!\n");
        return ERROR;
    }

    return SUCCESS;
}