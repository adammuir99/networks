/*
** server.c -- Opens a UDP socket on the specified port and waits for
			   a message from the client. If the client sends the
			   message "ftp" respond with "yes", otherwise respond
			   with "no"
			   Sample code retrieved from Beej's Guide to Network
			   Programming
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h>
#include "packet.h"


#define MAXBUFLEN 1200	//Maximum length of the message recieved

struct packet* stringToPacket (char* str);

// get sockaddr, IPv4 or IPv6 (function taken from Beej's):
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char ** argv)
{
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
	struct sockaddr_storage their_addr;
	char buf[MAXBUFLEN];
	socklen_t addr_len;
	char s[INET6_ADDRSTRLEN];

	if (argc != 2){
		printf("Invalid arguments: server <port number>\n");
		return 0;
	}
    char *MYPORT = argv[1];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET6; // set to AF_INET to use IPv4
	hints.ai_socktype = SOCK_DGRAM;	//datagram socket
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, MYPORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("listener: socket");
			continue;
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("listener: bind");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "listener: failed to bind socket\n");
		return 2;
	}

	freeaddrinfo(servinfo);

	printf("listener: waiting to recvfrom...\n");

	addr_len = sizeof their_addr;
	if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
		perror("recvfrom");
		exit(1);
	}

	printf("listener: got packet from %s\n", inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s));
	printf("listener: packet is %d bytes long\n", numbytes);
	buf[numbytes] = '\0';
	printf("listener: packet contains \"%s\"\n", buf);

	//Check if the message recieved by the client is "ftp"
    if (strcmp(buf, (char*)"ftp") == 0 ){
        sendto(sockfd, (char*)"yes", strlen((char*)"yes"), 0, (struct sockaddr *)&their_addr, addr_len);	//Respond yes
    } else {
        sendto(sockfd, (char*)"no", strlen((char*)"no"), 0, (struct sockaddr *)&their_addr, addr_len);		//Respond no
    }


//////////////// Receive Packets //////////////////////


	FILE* fp;
	bool keepLooping = true;

	while(keepLooping) {

		numbytes = recvfrom(sockfd,buf,MAXBUFLEN-1 , 0,(struct sockaddr *)&their_addr, &addr_len);

		if(numbytes == -1) {
			perror("recvfrom");
			exit(1);
		}

		buf[numbytes] = '\0';

		struct packet * newPacket = stringToPacket(buf); // conver the string received to a struct packet
		char* fileName = newPacket->filename;
		int fragmentNumber = newPacket->frag_no; 
		int dataSize = newPacket->size;
		int numOfFragments = newPacket->total_frag;
		char* data = newPacket->filedata;

		printf("Packet has been received!\n");

		if(fragmentNumber == 1) {
			fp = fopen(fileName,"wb"); // opens file stream with write to mode, 'b' is for binary
		} 

		fwrite(data,1,dataSize,fp); // write to file stream fp with the elements from "data"

		if(fragmentNumber == numOfFragments) {
			keepLooping = false; // end of linked list, stop looping
		}
		
		sendto(sockfd, (char*)"ACK", strlen((char*)"ACK"), 0, (struct sockaddr *)&their_addr, addr_len); // Send ACK

		free(newPacket); // free the memory allocated

	} // while
	
	fclose(fp); // close the file stream

	close(sockfd);

	return 0;
}

struct packet* stringToPacket (char* str){
	struct packet* packet_ptr;

	char* str_total_frag, *str_frag_no, *str_size, *str_filename;

	// Retrieving substring until a certain char from https://stackoverflow.com/questions/45832469/get-the-beginning-of-a-string-until-a-given-char
	str_total_frag = strtok(str, ":"); // Get the first token
	str_frag_no = strtok(NULL, ":");
	str_size = strtok(NULL, ":");
	str_filename = strtok(NULL, ":");

	struct packet* new_packet = malloc(sizeof(struct packet));
	new_packet->total_frag = atoi(str_total_frag);
	new_packet->frag_no = atoi(str_frag_no);
	new_packet->size = atoi(str_size);
	new_packet->filename = str_filename;

	int header_len = strlen(str_total_frag) + strlen(str_frag_no) + strlen(str_size) + strlen(str_filename) + 4;

	memcpy(new_packet->filedata, &str[header_len], new_packet->size);

	return new_packet;

}
