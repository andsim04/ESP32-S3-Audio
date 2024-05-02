/*
 * IPK.2024
 *
 * Demonstration of trivial UDP client.
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

typedef struct
{
  char *server_hostname;
  int port_string;
  char *fileName;
} Config;

typedef enum
{
  ERROR,
  SUCCESS,
} Status;

#define CHECK_ERROR(val) \
  if (val == ERROR)      \
  {                      \
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

Status GetConfigFromCLArguments(int argc, const char *argv[], Config *config);
Status GetAddrForDomain(Config *config, struct addrinfo **server_info);

// Message size
// MAX_MESSAGE_SIZE > 1472 generally undergoes IP fragmentation (the value depends on MTU of the path)
// MAX_MESSAGE_SIZE > 65,507 is unsendable on IPv4, sets errno to "Message too long" (limit of IPv4 header)
// MAX_MESSAGE_SIZE > 65,535 is unsendable on IPv6, sets errno to "Message too long" (limit of UDP header)
#define MAX_MESSAGE_SIZE 1024

#define TERMINATE_CHAR '!'

int odosliNaServer(char* fileName)
{

  char message_buffer[MAX_MESSAGE_SIZE] = {0};
  Config config;
  config.port_string = 10025;
  config.server_hostname = "158.193.140.91";
  config.fileName = fileName;

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

  // Get messages from stdin and send them to the server
  printf("Type your messages, terminate with '%c' or Control+D\n", TERMINATE_CHAR);

  bool terminate_char_found = false;
  while (!terminate_char_found)
  {
    // Read bufsize or a single line
    if (fgets(message_buffer, MAX_MESSAGE_SIZE, stdin) == NULL) // fgets reads at most BUFSIZE - 1
    {
      message_buffer[0] = TERMINATE_CHAR; // In case of EOF, just insert TERMINATE_CHAR
    }

    // Check terminating character
    char *terminate_character = strchr(message_buffer, TERMINATE_CHAR);
    if (terminate_character != NULL)
    {
      *terminate_character = '\0';
      terminate_char_found = true;
    }

    // Whether to include \0 or not;
    int message_length = terminate_char_found ? strlen(message_buffer) + 1 : strlen(message_buffer);

    // The message gets send as a whole or not at all
    int bytes_sent = sendto(client_socket, message_buffer, message_length, 0, server_info->ai_addr, server_info->ai_addrlen);
    CHECK_PERROR(bytes_sent < 0, "Sendto", server_info);
  }

  // Receive message back from the server, message is terminated with \0
  memset(message_buffer, 0, MAX_MESSAGE_SIZE);

  // Free addrInfo - no longer needed
  freeaddrinfo(server_info);

  // Receive message back from the server
  struct sockaddr_storage server_addr;
  socklen_t server_len = sizeof(server_addr);

  terminate_char_found = false;
  while (!terminate_char_found)
  {
    int received_bytes = recvfrom(client_socket, message_buffer, MAX_MESSAGE_SIZE, 0, (struct sockaddr *)&server_addr, &server_len);
    if (received_bytes <= 0)
    {
      fprintf(stderr, "Server timeout!\n");
      break;
    }

    // Print message, check if \0 was present in the message
    for (int i = 0; i < received_bytes; i++)
    {
      printf("%c", message_buffer[i]);
      if (message_buffer[i] == '\0')
      {
        terminate_char_found = true;
        break;
      }
    }
  }

  printf("Terminating ...\n");

  // Close the socket
  close(client_socket);

  return EXIT_SUCCESS;
}

// Parse CL Arguments
Status GetConfigFromCLArguments(int argc, const char *argv[], Config *config)
{
  if (argc != 3)
  {
    fprintf(stderr, "usage: %s <hostname> <port>\n", argv[0]);
    return ERROR;
  }

  // Port
  int port_for_validation = atoi(argv[2]);
  if (port_for_validation < 0 || port_for_validation > USHRT_MAX)
  {
    fprintf(stderr, "Port number out of range\n");
    return ERROR;
  }
  config->port_string = argv[2];

  // Hostname
  config->server_hostname = argv[1];
}

// Get IPv4 address for domain and port in config
Status GetAddrForDomain(Config *config, struct addrinfo **server_info)
{
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;      // IPv4
  hints.ai_socktype = SOCK_DGRAM; // UDP
  hints.ai_protocol = 0;          // Protocol

  int status = getaddrinfo(config->server_hostname, config->port_string, &hints, server_info);

  if (status != 0 || (*server_info)->ai_addr == NULL)
  {
    fprintf(stderr, "getaddrinfo: failed to resolve hostname!\n");
    return ERROR;
  }

  return SUCCESS;
}