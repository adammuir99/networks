int command_response();

int login_server(char* clientID, char* password, char* serverIP, char* serverPortNum);
int logout_server();

int joinSession(char* sessionID);
int leaveSession(char* sessionID);
int createSession(char* sessionID);
int list();
int quit();
int text(char* message);
