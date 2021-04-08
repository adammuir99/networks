#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>

#include <arpa/inet.h>
#include "users.h"
#include "message.h"
#include "session.h"

struct user users[USER_NUM] = {
  {.name = "adam", 
   .pass = "muir"},
  {.name = "sherman", 
   .pass = "lin"},
  {.name = "username", 
   .pass = "password"}
};

void init_users() {
  for (size_t i = 0; i < USER_NUM; i++) {
    users[i].active = 0;
    users[i].sockfd = -1;
    users[i].cur_session = NULL;
    bzero(users[i].joined_sessions, 10);
  }
}

int auth_user(int sockfd) {
  struct timeval timeout = {.tv_sec = 10, .tv_usec = 0};
  int err = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
  if (err) {
    printf("auth_user: failed to config socket\n");
    return err;
  }

  char buf[MAX_MESSAGE];
  if (recv(sockfd, buf, MAX_MESSAGE, 0) == EAGAIN) {
    return 0;
  }

  struct message m;
  buf_to_message(buf, &m);

  if (m.type == LOGIN) {
    for (size_t i = 0; i < USER_NUM; i++) {
      if ((strcmp(users[i].name, m.source) == 0) && (strcmp(users[i].pass, m.data) == 0)) {
        if (users[i].active) {
          send_to_server(sockfd, NACK, "Server", "Server", "user already logged in");
          return 0;
        } else {
          send_to_server(sockfd, ACK, "Server", "Server", "");
          users[i].active = 1;
          users[i].sockfd = sockfd;
          printf("User %s connect to server\n", m.source);
          return 1; // return true
        }
      }
    }
    send_to_server(sockfd, NACK, "Server", "Server", "username or password is incorrect");
  } else {
    send_to_server(sockfd, NACK, "Server", "Server", "wrong message type for login");
  }
  return 0; // return false
}

int logout_user(struct user* user) {
  for (size_t i = 0; i < 10; i++) {
    if (user->joined_sessions[i] != NULL) { // remove user in all sessions joined
      user_leave_session(user, user->joined_sessions[i], 0);
    }
  }
  user->active = 0;
  FD_CLR(user->sockfd, &server_fds);
  close(user->sockfd);
  user->sockfd = -1;
  user->cur_session = NULL;
  printf("Successfully logged out %s\n", user->name);
  return 0;
}

int user_join_session(struct user* user, struct session* s) {
  char msg[MAX_DATA];
  if (s == NULL) {
    sprintf(msg, "%s:session does not exist", s->session_id);
    send_to_server(user->sockfd, JN_NAK, "Server", "Server", msg);
    return 1;
  }
  if (s->user_num == MAX_USERS) {
    sprintf(msg, "%s:session already full %d", s->session_id, MAX_USERS);
    send_to_server(user->sockfd, JN_NAK, "Server", "Server", msg);
    return 1;
  }
  int err = session_add_user(s, user);
  join_helper(user, s);
  send_to_server(user->sockfd, JN_ACK, "Server", "Server", s->session_id);
  snprintf(msg, MAX_DATA, "%s joined session %s", user->name, s->session_id);
  session_send(s, "Server", msg);
  return 0;
}

void join_helper(struct user* user, struct session* s) {
  if (user->joined_sessions[s->sid] == NULL) {
    user->joined_sessions[s->sid] = s;
    user->cur_session = s;
  } else {
    printf("[Fatal] User %s already joined session %s\n", user->name, s->session_id);
  }
}

int user_leave_session(struct user* user, struct session* s, int do_ack) {
  if (s == NULL) {
    send_to_server(user->sockfd, UNKNOWN, "Server", "Server", "Session does not exist");
    return 1;
  }

  if (user->joined_sessions[s->sid] == NULL) {
    send_to_server(user->sockfd, MESSAGE, "Server", "Server", "Not in the session specified");
    return 1;
  }
  size_t cur_sid = s->sid;
  char buf_all[MAX_DATA];
  char buf_user[MAX_DATA];

  snprintf(buf_all, MAX_DATA, "%s has left session %s", user->name, s->session_id);
  snprintf(buf_user, MAX_DATA, "leaving session %s...", s->session_id);

  // remove user in session
  if (session_remove_user(s, user)) {
    send_to_server(user->sockfd, MESSAGE, "Server", "Server", "server error!");
    return 1;
  }
  // remove session in user
  user->joined_sessions[cur_sid] = NULL;
  if (user->cur_session == s) {
      user->cur_session = NULL;
  }
  if (sessions[cur_sid] != NULL) {
      session_send(sessions[cur_sid], "Server", buf_all);
  }
  if (do_ack) {
    send_to_server(user->sockfd, MESSAGE, "Server", "Server", buf_user);
  }

  user->joined_sessions[cur_sid] = NULL;
  return 0;
}

int user_send_msg(struct user* user, struct session* s, const char* msg) {
  if (user->joined_sessions[s->sid] == NULL) {
    send_to_server(user->sockfd, MESSAGE, "Server", "Server", "Please join that session first");
    return 1;
  }
  if (s == NULL) {
    send_to_server(user->sockfd, MESSAGE, "Server", "Server", "Session does not exist");
    return 1;
  }
  return session_send(s, user->name, msg);
}

struct user* find_user(const char* username) {
  for (size_t i = 0; i < USER_NUM; i++) {
    if (strcmp(username, users[i].name) == 0) {
      return &users[i];
    }
  }
  return NULL;
}