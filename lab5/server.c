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

#include "message.h"
#include "users.h"
#include "session.h"

// Handles user operations
int handle_user_req();

int main(int argc, char const * argv[]) {
  if (argc != 2){
		printf("Invalid arguments: server <port number>\n");
		return 0;
	}

  unsigned int port = atoi(argv[1]);
  struct addrinfo hints, *res;
  bzero(&hints, sizeof(hints));
  hints.ai_flags = AI_PASSIVE; // Use my IP
  hints.ai_family = AF_INET; // Use IPv4
  hints.ai_socktype = SOCK_STREAM; // Use TCP

  if (getaddrinfo(NULL, argv[1], &hints, &res) == -1) {
    printf("error: getaddrinfo\n");
    return 0;
  }

  int sockfd; // Listening on sockfd
  struct addrinfo *iter;
  for(iter = res; iter != NULL; iter = res->ai_next) {
    sockfd = socket(iter->ai_family, iter->ai_socktype, iter->ai_protocol);
    if (sockfd == -1) {
      printf("Attempt to find available sock failed ... \n");
      continue;
    }
    if (bind(sockfd, iter->ai_addr, iter->ai_addrlen) == -1) {
      close(sockfd);
      printf("Attempt to bind sock failed ...\n");
      continue;
    }
    break;
  }

  freeaddrinfo(res);

  if (listen(sockfd, 10)) {
    close(sockfd);
    fprintf(stderr, "Failed to listen at sockfd %d ...\n", sockfd);
    return 0;
  }

  printf("server: listening on %d ...\n", port);

  int main_sockfd = sockfd;
  int max_sockfd;
  while(1) {
    FD_ZERO(&server_fds);
    FD_SET(main_sockfd, &server_fds);
    max_sockfd = main_sockfd;
    for (size_t i = 0; i < USER_NUM; i++) {
      if (users[i].active) {
        FD_SET(users[i].sockfd, &server_fds);
        if (users[i].sockfd > max_sockfd)
          max_sockfd = users[i].sockfd;
      }
    }

    // always handle max sockfd first
    if (select(max_sockfd + 1, &server_fds, NULL, NULL, NULL) < 0){
      perror("error: select");
    }

    if (FD_ISSET(main_sockfd, &server_fds)) {
      // Login request
      struct sockaddr new_addr;
      socklen_t new_addrlen;

      int new_sockfd = accept(sockfd, &new_addr, &new_addrlen);
      if (new_sockfd < 0) {
        perror("error: accept");
        continue;
      }

      if (!auth_user(new_sockfd))
        close(new_sockfd);
    } else {
      // Other request
      for (size_t i = 0; i < USER_NUM; i++) {
        struct user* cur_user = &users[i];
        if (!cur_user->active){
           continue;
        }
        if (FD_ISSET(cur_user->sockfd, &server_fds)) {
          char buf[MAX_MESSAGE];
          struct message m;
          buf[MAX_MESSAGE - 1] = '\0'; // avoid overflow
          int err = recv(cur_user->sockfd, buf, MAX_MESSAGE, 0);
          if (err == -1) {
            printf("Failed to receive message from sockfd %d\n", cur_user->sockfd);
            continue;
          }

          if (buf_to_message(buf, &m)) {
            // If the packet is mal formed the user might be lost
            // logout the user on server
            logout_user(cur_user);
            return 1;
          }

          // Process each situation
          switch (m.type) {
          case NEW_SESS:
            new_session(m.session_id, cur_user);
            break;
          case EXIT:
            logout_user(cur_user);
            break;
          case JOIN:
            user_join_session(cur_user, find_session(m.session_id));
            break;
          case LEAVE_SESS:
            user_leave_session(cur_user, find_session(m.session_id), 1);
            break;
          case LIST: {
            char msg[MAX_DATA];
            list_sessions(msg);
            send_to_server(cur_user->sockfd, QU_ACK, "Server", "Server", msg);
          }
            break;
          case MESSAGE:
            user_send_msg(cur_user, find_session(m.session_id), m.data);
            break;
          case KICK:
            for(struct user* iter = find_session(m.session_id)->users; iter != NULL; iter++){
              if (iter->name == m.data){
                user_leave_session(iter, find_session(m.session_id), 1);
                break;
              }
            }
          case ADMIN:{
            //printf("ADMIN HERE\n");
            struct session* s = find_session(m.session_id);

            if (s == NULL) {
              send_to_server(cur_user->sockfd, ADMIN_NACK, "Server", "Server", " ");
              break;
            }
            check_admin(s, cur_user);
            break;
          }
         }
        } // if
      } // for loop
    }
  }
  return 0;
}
