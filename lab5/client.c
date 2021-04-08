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

#include "commands.h"

int main(int argc, char *argv[]) {
  printf("Client ready: /login <username> <password> <address> <port>\n");
  // set of file descriptors
  fd_set fds;

  while(1) {
    FD_ZERO(&fds);
    FD_SET(fileno(stdin), &fds);

    if (client_sock > 0) {
      FD_SET(client_sock, &fds);
      select(client_sock + 1, &fds, NULL, NULL, NULL);
    } else {
      select(fileno(stdin) + 1, &fds, NULL, NULL, NULL);
    }

    // Receive message
    if (isloggedin() && FD_ISSET(client_sock, &fds)) {
      char buf[MAX_MESSAGE];
      recv(client_sock, buf, MAX_MESSAGE, 0);
      buf[MAX_MESSAGE - 1] = '\0'; // Avoid overflow
      struct message m;
      buf_to_message(buf, &m);
      if(m.type == INVITEOTHER){
	char answer[MAX_DATA];
	printf("Server: %s has invited you to join session %s\n", m.source, m.session_id);
	while(1) {
		printf("Would you like to join the session? Please answer: yes or no \n");
		scanf("%s",answer);
		if(strncmp(answer, "yes", 1) == 0 ) {
			join_session(m.session_id);
			break;
		} else if( strncmp(answer, "no", 1) == 0) {
			break;
		} else {
			printf("Invalid answer!\n");
		}
	}
      }
      print_message(&m);
    } else if (FD_ISSET(fileno(stdin), &fds)) {
      menu();
    }
  }
  return 0;
}
