#ifndef SESSION_H
#define SESSION_H

#include "message.h"

#define MAX_SESSIONS 10
#define MAX_USERS 10

struct user;

struct session {
  size_t sid; // index in session array
  char session_id[MAX_SESSION_ID];
  size_t user_num; // number of users in session
  struct user *users[MAX_USERS];
  struct user *admin;
};

extern fd_set server_fds;
extern struct session *sessions[MAX_SESSIONS];

int new_session(const char* session_id, struct user* admin);
struct session* find_session(const char* session_id);
int session_send(struct session* s, const char* source, const char* msg);
int session_remove_user(struct session* s, struct user* user);
int session_add_user(struct session* s, struct user* user);
int get_session_info(struct session* s, char* dest);
int list_sessions(char* dest);
int check_admin(struct session* s, struct user* cur_user);
#endif