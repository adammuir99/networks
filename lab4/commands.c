

// Global Variables
int clientSockFd;

int login_server(char* clientID, char* password, char* serverIP, char* serverPort){

	struct addrinfo hints, *servinfo, *p;    
	int rv;    

	if (argc != 2) {
		fprintf(stderr,"usage: client hostname\n");        
		exit(1);
	}    

	memset(&hints, 0, sizeof hints);    
	hints.ai_family = AF_UNSPEC;    
	hints.ai_socktype = SOCK_STREAM; 
   
	if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {        
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));        
		return 1;    
	}

	// loop through all the results and connect to the first we can    
	for(p = servinfo; p != NULL; p = p->ai_next) {   
		if ((clientSockFd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
			perror("client: socket");            
			continue;
		}       

		if (connect(clientSockFd, p->ai_addr, p->ai_addrlen) == -1) {           
			close(clientSockFd);            
			perror("client: connect");            
			continue;        
		}        

		break;    
	}    

	if (p == NULL) {        
		fprintf(stderr, "client: failed to connect\n");        
		return 2;    
	}

	


} // login_server


