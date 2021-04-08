#include "session.h"
#include "users.h"
#include <string.h>
#include <stdio.h>

fd_set server_fds;
struct session* sessions[MAX_SESSIONS];

int new_session(const char* session_id, struct user* creator) {
  for (size_t i = 0; i < MAX_SESSIONS; i++) {
    if (sessions[i] == NULL) {
      sessions[i] = malloc(sizeof(struct session));
      (sessions[i])->sid = i;
      (sessions[i])->user_num = 0;
      (sessions[i])->creator = creator;
      bzero(sessions[i]->session_id, MAX_SESSION_ID);
      bzero(sessions[i]->users, MAX_SESSION_ID * sizeof(struct user*));
      strncpy(sessions[i]->session_id, session_id, MAX_SESSION_ID);
      sessions[i]->users[0] = creator;
      sessions[i]->user_num++;

      join_helper(creator, sessions[i]);
      printf("User %s create new session %s\n", creator->name, sessions[i]->session_id);
      return send_to_server(creator->sockfd, NS_ACK, "Server", "Server", sessions[i]->session_id);
    }
  }
  send_to_server(creator->sockfd, UNKNOWN, "Server", "Server", "[Server] max session num reached");
  return 1; // Max session reached
}

struct session* find_session(const char* session_id) {
  for (size_t i = 0; i < MAX_SESSIONS; i++) {
    if (sessions[i] != NULL &&
        (strcmp(sessions[i]->session_id, session_id) == 0)) {
      return sessions[i]; // true
    }
  }
  return NULL; // false
}

int session_send(struct session* s, const char* source, const char* msg) {
  for (size_t i = 0; i < MAX_USERS; ++i) {
    if (s->users[i] != NULL) {
      send_to_server(s->users[i]->sockfd, MESSAGE, source, s->session_id, msg);
    }
  }
  return 0;
}

int session_remove_user(struct session* s, struct user* user) {
  for (size_t i = 0; i < MAX_USERS; i++) {
    if (s->users[i] == user) {
      s->users[i] = NULL;
      s->user_num--;
      if (s->user_num == 0) { // Check if there are no users left in the session
        // Destory session
        sessions[s->sid] = NULL;
        free(s);
        return 0;
      }
    }
  }
  return 1;
}

int session_add_user(struct session* s, struct user* user) {
  for (size_t i = 0; i < MAX_USERS; i++) {
    if (s->users[i] == NULL) {
      s->users[i] = user;
      s->user_num++;
      return 0;
    }
  }
  return 1; // Session full
}

int get_session_info(struct session* s, char* dest) {
  if (dest == NULL){
     return 1;
  }
  snprintf(dest, MAX_DATA, "%d. %s, users in session: %d\n", s->sid + 1, s->session_id, s->user_num);
  return 0;
}

int list_sessions(char* dest) {
  size_t cur_pos = 0;
  char buf[MAX_DATA];
  for (size_t i = 0; i < MAX_SESSIONS; i++) {
    if (sessions[i] != NULL) {
      get_session_info(sessions[i], buf);
      strncpy(dest + cur_pos, buf, MAX_DATA);
      cur_pos += strlen(buf);
    }
  }
  return 0;
}