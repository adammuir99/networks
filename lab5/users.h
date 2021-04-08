#ifndef USERS_H
#define USERS_H

#include "message.h"

#define USER_NUM 3

struct session;

struct user {
  char name[MAX_NAME];
  char pass[256];
  struct session* cur_session;
  struct session* joined_sessions[10];
  int sockfd; // connection between server and client
  int active; // whether is user is active
};

extern struct user users[USER_NUM];

void init_users();
int auth_user(int sockfd);
int logout_user(struct user* user);
int user_join_session(struct user* user, struct session* s);
int user_leave_session(struct user* user, struct session* s, int do_ack);
int user_send_msg(struct user* user, struct session* s, const char* msg);
struct user* find_user(const char* username);
void join_helper(struct user* user, struct session* s);
int user_invite(struct user * user1, struct user* user2, struct session* s);

#endif
