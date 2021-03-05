// client program
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include "packet.h"

// Sample code retrieved from Beej's Guide to Network Programming

#define MAXBUFLEN 1200	//Maximum length of the message recieved

//Function Prototyping
char* packetToString (struct packet* p);


int main(int argc, const char *argv[]) {

// check for number of arguments. if it isn't 3, exit the program
if (argc != 3) {

    printf("Invalid number of arguments! Program terminating! \n");
    printf("usage: deliver <server address> <server port number>\n");
    exit(1);
} // if

if(strcmp(argv[0], "deliver") != 0) {
    printf("Invalid first argument! Program terminating! \n");
    printf("usage: deliver <server address> <server port number>\n");
    exit(1);
}

int sockfd;
struct sockaddr_in serv_addr;
struct addrinfo hints, *servinfo;

// configure the structs
memset(&hints, 0, sizeof hints); // make sure the struct is empty
hints.ai_family = AF_UNSPEC;    // AF_UNSPEC because we can use either IPv4 or IPv6 
hints.ai_socktype = SOCK_DGRAM; // SOCK_DGRAM because we're using UDP
hints.ai_protocol = 0; // 0 is automatically choose protocol type

// DNS lookup, getaddrinfo() returns a non-zero if there is an error
int rv;
if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0) { //argv[1] is server address, and argv[2] is server port #
 printf("Program terminating because of DNS lookup failure!\n");
 exit(1);
}

// create a socket, socket() returns a -1 if there is an error
  if ((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1) {
    printf("Program terminating because of socket creation failre!\n");
    exit(1);
  }

// Ask for user input
  printf("Please input: ftp <file name>\n");
  char firstInput[100], fileName[100];
  scanf("%s %s", firstInput, fileName);
  //fgets(firstInput,100,stdin);
  //fgets(fileName,100,stdin);
  printf("firstInput: %s, fileName: %s\n", firstInput,fileName);

  if(strcmp(firstInput,"ftp") != 0 ) {
    printf("Program terminating because of invalid input!\n");
    exit(1);
  }

  //Check to see if file exists, access() returns a 0 if access is susccessful
  if(access(fileName, F_OK) != 0 ){ // https://www.gnu.org/software/libc/manual/html_node/Testing-File-Access.html
    printf("Program terminating because file cannot be found!\n");
    exit(1);
  }
  
  // Send Message
  char* message = "ftp";
  int messageLength = strlen(message);

  // Measure the RTT
  clock_t start, end;
  start = clock();    // Message has been sent, start the timer

  sendto(sockfd,message,messageLength, 0, servinfo->ai_addr, servinfo->ai_addrlen);

    // Receive Message
  struct sockaddr_storage from_addr;
  int numBytesReceived;
  char response[MAXBUFLEN];
  socklen_t from_length = sizeof(struct sockaddr_storage);
  memset(&response, 0, sizeof response);
  // maximum length of buffer is 99  because (100-1), the last spot is reserved for null terminating character
  numBytesReceived = recvfrom(sockfd,response,99,0,(struct sockaddr *)&from_addr, &from_length ); 

  end = clock();    // Response has been recieved, stop the timer

  //printf("BEFORE: %s\n", response);
  // Add null terminating character to end of response string received
  response[numBytesReceived] = '\0';


  // Check what the received message is
  if(strcmp(response, "yes") == 0 ){
        printf("A file transfer can start.\n");
    }
    else{
        printf("Reponse isn't 'yes'!\n");
        printf("Response: %s\n", response);
        exit(1);
    }

  printf("The RTT is: %f seconds\n", (float)(end - start) / CLOCKS_PER_SEC);

  // Sending the packet
  struct packet* packet_ptr = createPackets(fileName);

  while (packet_ptr != NULL) {
    // Convert the packet into a string
    char* packet_msg = packetToString(packet_ptr);

    // Send the packet
    sendto(sockfd, packet_msg, MAXBUFLEN, 0, servinfo->ai_addr, servinfo->ai_addrlen);
    printf("Package %d sent.\n", packet_ptr->frag_no);

    // Get Response
    memset(&response, 0, sizeof response);
    numBytesReceived = recvfrom(sockfd, response, MAXBUFLEN-1, 0, (struct sockaddr *)&from_addr, &from_length);
    response[numBytesReceived] = '\0';

    // Check what the received message is
    if(strcmp(response, "ACK") == 0 ){
          printf("ACK\n");
      }
      else{
          printf("Reponse isn't 'ACK'!\n");
          printf("Response: %s\n", response);
          printf("Re-sending package.\n");
          continue;
      }

    struct packet* deallocate_ptr = packet_ptr;   // Point to the current packet
    packet_ptr = packet_ptr->nextPacket;
    // Free the packet that was just sent
    free(deallocate_ptr);
    free(packet_msg);  
  }

    freeaddrinfo(servinfo);  // free the linked list servinfo
    close(sockfd); // close socket descriptor
    return 0;
} // main

char* packetToString (struct packet* p){
  // Allocate the memory for str
  char* str = malloc(MAXBUFLEN);

  // Set str as 0
  memset(str, 0, MAXBUFLEN);

  // Create the header
  int header = sprintf(str, "%d:%d:%d:%s:", p->total_frag, p->frag_no, p->size, p->filename);

  // Copy the file data into the string
  memcpy(str + header, p->filedata, p->size);

  return str;
}