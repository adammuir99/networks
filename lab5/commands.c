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
#include <time.h>

#include "users.h"
#include "message.h"
#include "commands.h"

// Global variables
struct user* cur_user = NULL; // current user
// whether user in a session
int in_session = 0;
char cur_session[MAX_SESSION_ID];
int client_sock = -1; // current socket

#define LOGIN_CHECK                                             \
  if(!isloggedin())                                             \
    { printf("Log in before issuing commands\n"); }      \
  else

int menu() {

  int err = 0;
  char session_id[MAX_FIELD];
  char username[MAX_FIELD];
  char command[MAX_COMMAND_LEN];
  scanf("%s", command);

  if (strcmp(command, "/login") == 0) {
    if (!isloggedin()) {
      char name[MAX_NAME];
      char pass[256];
      char server_ip[MAX_FIELD];
      char server_port[MAX_FIELD];
      scanf(" %s %s %s %s", name, pass, server_ip, server_port);
      err = login(name, pass, server_ip, server_port);
    } else {
      printf("Already logged in as %s\n", cur_user->name);
    }
  } else if (strcmp(command, "/logout") == 0) {
    LOGIN_CHECK {
      err = logout();
    }
  } else if (strcmp(command, "/joinsession") == 0) {
    LOGIN_CHECK {
      scanf(" %s", session_id);
      err = join_session(session_id);
    }
  } else if (strcmp(command, "/leavesession") == 0) {
    LOGIN_CHECK {
      scanf(" %s", session_id);
      err = leave_session(session_id);
    }
  } else if (strcmp(command, "/kick") == 0) {
    	
	if(!isloggedin()){ printf("Not yet logged in; Please login first\n"); }      
	else {
          char name[MAX_NAME];
          scanf(" %s", name);
      		scanf(" %s", session_id);
      		err = kick_session(session_id, name);
    	}

  } else if (strcmp(command, "/createsession") == 0) {
    LOGIN_CHECK {
      scanf(" %s", session_id);
      err = create_session(session_id);
    }
  } else if (strcmp(command, "/list") == 0) {
    LOGIN_CHECK{
      err = list();
    }
  } else if (strcmp(command, "/quit") == 0) {
    err = quit();
  } else if(strcmp(command,"/invite")== 0) {
	LOGIN_CHECK{
		scanf(" %s ", username);
		scanf(" %s", session_id);
		err = invite(username,session_id);
	}
  } else {
    LOGIN_CHECK {
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

int isloggedin() {
  return cur_user != NULL;
}

int request(message_t type, const char* source, const char* session_id, const char* data) {
  return send_to_server(client_sock, type, source, session_id, data);
}

// Remember to free body since it's malloced
int recv_ack(message_t ack_type, message_t nak_type, int* retval, char** body) {
  char msg_buf[sizeof(struct message)];
  int err = recv(client_sock, msg_buf, sizeof(struct message), 0);
  struct message m;
  buf_to_message(msg_buf, &m);

  *body = malloc(sizeof(char) * (m.size + 1));
  strcpy(*body, m.data);

  if (m.type == ack_type) {
    *retval = 1;
  } else if (m.type == nak_type) {
    *retval = 0;
  }
  return 0;
}

int login(const char* name, const char* pass, const char* server_ip, const char* server_port) {
  // Create the socket
  struct addrinfo hints, *servinfo, *p;
  int rv;
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  if ((rv = getaddrinfo(server_ip, server_port, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }
  // loop through all the results and connect to the first we can
  for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((client_sock = socket(p->ai_family, p->ai_socktype,
                              p->ai_protocol)) == -1) {
      perror("client: socket");
      continue;
    }
    if (connect(client_sock, p->ai_addr, p->ai_addrlen) == -1) {
      close(client_sock);
      perror("client: connect");
      continue;
    }
    break;
  }
  if (p == NULL) {
    fprintf(stderr, "client: failed to connect\n");
    return 2;
  }
  // Connected, now send the message
  request(LOGIN, name, "", pass);

  // Receive ACK or NAK from server
  int isack;
  char* result = NULL;
  recv_ack(ACK, NACK, &isack, &result);
  if (!isack) {
    printf("Login failed: %s\n", result);
    return 3;
  }
  free(result);
  cur_user = malloc(sizeof(struct user));
  strcpy(cur_user->name, name);
  strcpy(cur_user->pass, pass);
  printf("Successfully loggged in as %s\n", cur_user->name);
  return 0;
}

int logout() {
  int err = request(EXIT, cur_user->name, "", "");
  close(client_sock);
  client_sock = -1;
  if (err) {
    printf("Could not logout\n");
  } else {
    printf("%s logged out \n", cur_user->name);
    free(cur_user);
    cur_user = NULL;
  }
  return err;
}

int join_session(const char* session_id) {
  int err = request(JOIN, cur_user->name, session_id, "");
  if (err) return err;
  int isack;
  char* result = NULL;
  err = recv_ack(JN_ACK, JN_NAK, &isack, &result);
  if (err) {
    free(result);
    return err;
  }
  if(isack) {
    strncpy(cur_session, session_id, MAX_SESSION_ID);
    in_session = 1;
    printf("Successfully joined session %s\n", cur_session);
    err = 0;
  } else {
    printf("failed to join session: %s\n", result);
    err = 1;
  }
  free(result);
  return err;
}

int leave_session(const char* session_id) {
  request(LEAVE_SESS, cur_user->name, session_id, "");
  printf("Leave session %s\n", session_id);
  if (strcmp(cur_session, session_id) == 0) {
    in_session = 0;
  }
  return 0;
}

int kick_session(const char* session_id, const char* name) {
  request(ADMIN, cur_user->name, session_id, cur_user->name);
  int isack;
  char* result = NULL;
  recv_ack(ADMIN_ACK, ADMIN_NACK, &isack, &result);
  if (isack == 0){  // NACK means the user is not an admin
    printf("Only an admin can kick, make sure you enter a valid name and session\n");
    return 0;
  }

  char text[256];
  char text2[256];
  strcpy(text, name);
  strcat(text, " has been kicked from ");
  strcat(text, session_id);
  request(MESSAGE, "Server", cur_session, text);
  strcpy(text2, name);
  strcat(text2, "1");
  sleep(1);
  request(MESSAGE, "Server", cur_session, text2);
  request(KICK, "Server", session_id, name);
  return 0;
}

int create_session(const char* session_id) {
  request(NEW_SESS, cur_user->name, session_id, "");
  int isack;
  char* result = NULL;
  if (recv_ack(NS_ACK, UNKNOWN, &isack, &result) != 1) {
    strncpy(cur_session, session_id, MAX_SESSION_ID);
    in_session = 1;
    printf("Session created %s\n", result);
  }
  free(result);
  return 0;
}

int list() {
  int err = request(LIST, cur_user->name, "", "");
  if (err) return err;
  int isack;
  char* result = NULL;
  err = recv_ack(QU_ACK, UNKNOWN, &isack, &result);
  if (err) {
    printf("Could not list sessions\n");
  } else {
    printf("The following sessions are currently running: \n%s\n", result);
  }
  free(result);
  return err;
}

int quit() {
  int err = 0;
  if (cur_user != NULL) {
    err = logout();
  }
  if (err) {
    printf("Could not to quit\n");
    return err;
  }
  printf("\nQuiting\n");
  exit(0);
}

int send_message(const char* text) {
  if (!in_session) {
    printf("You must be in a session to chat\n");
    return 1;
  }
  int err = request(MESSAGE, cur_user->name, cur_session, text);
  if (err) return err;
  printf("message sent...\n");
  return 0;
}

int invite(const char* user, const char* sessionID){

	int code = request(INVITEOTHER, cur_user->name, sessionID, user);
	if(code){ return code;}
	int ack;

	char* result = NULL;

	code = recv_ack(INVITE_ACK, UNKNOWN, &ack, &result);

	if(code) {
		printf("Failed to invite %s: %s\n", user, result);
	} else {
		printf("%s\n", result);
	}

	free(result);
	return code;
}
