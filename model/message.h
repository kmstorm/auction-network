#ifndef __MESSAGE__
#define __MESSAGE__

#include <stdint.h>
#define BUFFER_SIZE 1024

typedef struct
{
  uint8_t message_type;
  char payload[BUFFER_SIZE];
} Message;

/* Function prototypes */
void init_message(Message *message);
void build_message(Message *message, uint8_t type, const char *payload);
int parse_credentials(const Message *message, char *username, char *password);
void print_message(const Message *message);

#endif
