#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "message.h"

int buf_to_message(const char* buf, struct message* m) {
  char* strs[4];
  int start_i = 0;
  int num = 0;

  for (size_t i = 0; i < sizeof(struct message); i++) {
    if (buf[i] == ':') {
      int len = i - start_i;
      strs[num] = malloc(sizeof(char) * len + 1);
      strncpy(strs[num], buf + start_i, len);
      strs[num][len] = '\0';
      start_i = i + 1;
      num++;
      if (num == 4) break;
    }
  }
  if (num != 4) { // packet not in the right format
    printf("Bad packet %s\n", buf);
    return 1;
  }

  m->type = atoi(strs[0]);
  m->size = atoi(strs[1]);

  strcpy(m->source, strs[2]);
  strcpy(m->session_id, strs[3]);
  strncpy(m->data, buf + start_i, m->size);

  m->data[m->size] = '\0';

  for (int i = 0; i < 4; i++)
    free(strs[i]); // delete malloc'd
  return 0;
}

int send_to_server(int sockfd, message_t type, const char* source, const char* session_id, const char* data) {
  char msg_buf[MAX_MESSAGE];
  bzero(msg_buf, MAX_MESSAGE);
  snprintf(msg_buf, MAX_MESSAGE, "%d:%lu:%s:%s:%s", type, strlen(data), source, session_id, data);

  int err = send(sockfd, msg_buf, strlen(msg_buf) + 1, 0);
  if (err == -1) {
    printf("[send through] Failed to send message: %s\n", msg_buf);
    return 1;
  }
  return 0;
}

void print_message(const struct message* m) {
  printf("[%s] %s says: %s\n", m->session_id, m->source, m->data);
}
