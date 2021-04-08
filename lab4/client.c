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
#include "users.h"
#include "message.h"

int main(int argc, char *argv[]) {
  printf("Client ready: /login <username> <password> <address> <port>\n");
  // set of file descriptors
  fd_set fds;

  while(1) {
    FD_ZERO(&fds);
    FD_SET(fileno(stdin), &fds);

    if (clientSockFd > 0) {
      FD_SET(clientSockFd, &fds);
      select(clientSockFd + 1, &fds, NULL, NULL, NULL);
    } else {
      select(fileno(stdin) + 1, &fds, NULL, NULL, NULL);
    }

    // Receive message
    if (isloggedin() && FD_ISSET(clientSockFd, &fds)) {
      char buf[MAX_MESSAGE];
      recv(clientSockFd, buf, MAX_MESSAGE, 0);
      buf[MAX_MESSAGE - 1] = '\0'; // Avoid overflow
      struct message m;
      parse_message(buf, &m);
      print_message(&m);
    } else if (FD_ISSET(fileno(stdin), &fds)) {


	int err = 0;
  char session_id[MAX_FIELD];
  char username[MAX_FIELD];
  char command[MAX_COMMAND_LEN];
  scanf("%s", command);

  if (strcmp(command, "/login") == 0) {
    if (!isloggedin()) {
      char name[MAX_NAME];
      char pass[MAX_PASS];
      char server_ip[MAX_FIELD];
      char server_port[MAX_FIELD];
      scanf(" %s %s %s %s", name, pass, server_ip, server_port);
      err = login(name, pass, server_ip, server_port);
    } else {
      printf("Already logged in as %s\n", cur_user->name);
    }
  } else if (strcmp(command, "/logout") == 0) {

	if(!isloggedin()) { printf("Not yet logged in; Please login first\n"); }      
	else {err = logout(); }

  } else if (strcmp(command, "/joinsession") == 0) {

	if(!isloggedin()){ printf("Not yet logged in; Please login first\n"); }      
	else {
      		scanf(" %s", session_id);
      		err = join_session(session_id);
    	}

  } else if (strcmp(command, "/leavesession") == 0) {
    	
	if(!isloggedin()){ printf("Not yet logged in; Please login first\n"); }      
	else {
      		scanf(" %s", session_id);
      		err = leave_session(session_id);
    	}

  } else if (strcmp(command, "/createsession") == 0) {

	if(!isloggedin()){ printf("Not yet logged in; Please login first\n"); }      
	else {
      		scanf(" %s", session_id);
      		err = create_session(session_id);
    	}

  } else if (strcmp(command, "/list") == 0) {

    	if(!isloggedin()){ printf("Not yet logged in; Please login first\n"); }      
	else{
      		err = list();
    	}

  } else if (strcmp(command, "/quit") == 0) {
    err = quit();
  } else {
    	if(!isloggedin()){ printf("Not yet logged in; Please login first\n"); }      
	else {
      		// get all text in terminl and send
      		char msg_buf[MAX_DATA];
      		strcpy(msg_buf, command);
      		int offset = strlen(command);
      		fgets(msg_buf + offset, MAX_DATA - offset, stdin);
      		err = send_message(msg_buf);
    }
  }
  return err;
    }
  }
  return 0;
}
