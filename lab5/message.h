#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdlib.h>

#define MAX_SESSION_ID 1080
#define MAX_NAME 256
#define MAX_DATA 2048

#define MAX_MESSAGE sizeof(struct message)

typedef unsigned int message_t;

struct message {
  message_t type;
  size_t size;
  char source[MAX_NAME];
  char session_id[MAX_SESSION_ID];
  char data[MAX_DATA];
};

#define UNKNOWN 0
#define LOGIN 1
#define ACK 2
#define NACK 3
#define EXIT 4
#define JOIN 5
#define JN_ACK 6
#define JN_NAK 7
#define LEAVE_SESS 8
#define NEW_SESS 9
#define NS_ACK 10
#define MESSAGE 11
#define LIST 12
#define QU_ACK 13

#define INVITEOTHER 18
#define INVITE_ACK 19

#define KICK 14
#define ADMIN 15
#define ADMIN_ACK 16
#define ADMIN_NACK 17


int buf_to_message(const char* buf, struct message* m);

// Send message through socket
int send_to_server(int sockfd, message_t type, const char* source, const char* session_id, const char* data);

void print_message(const struct message* m);
#endif
