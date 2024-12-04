#include "message.h"
#include <stdio.h>
#include <string.h>

/* Initialize a message */
void init_message(Message *message)
{
  message->message_type = 0;
  memset(message->payload, 0, BUFFER_SIZE);
}

/* Build a message with type and payload */
void build_message(Message *message, uint8_t type, const char *payload)
{
  message->message_type = type;
  strncpy(message->payload, payload, BUFFER_SIZE - 1);
  message->payload[BUFFER_SIZE - 1] = '\0'; // Ensure null termination
}

/* Parse a message payload into username and password */
int parse_credentials(const Message *message, char *username, char *password)
{
  return sscanf(message->payload, "%49[^|]|%49s", username, password) == 2;
}

/* Print a message for debugging */
void print_message(const Message *message)
{
  printf("Message Type: %d\n", message->message_type);
  printf("Payload: %s\n", message->payload);
}
