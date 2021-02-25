// header file that contains the packet struct and the declarations of functions that will be used on packets

#ifndef headerFile
#define headerFile

#include <math.h>

#define maxFragmentLen 1000


struct packet {
	
	unsigned int total_frag;
	unsigned int frag_no;
	unsigned int size;
	char* filename;
	char filedata[maxFragmentLen];
	struct packet* nextPacket;
	

};

// given the file name, process the file into segments and create a linked list of packets
struct packet * createPackets(char* fileName){

	struct packet * firstPacket, previousPacket;
	
	// open file, and read-only (b is for binary files as well)
	FILE* fp = fopen(fileName, "rb"); 

	// get file size
	fseek(fp,0, SEEK_END);
	int fileSize = ftell(fp);
	fseek(fp,0, SEEK_SET);

	 int numOfFragments = ceil(fileSize / maxFragmentLen);

	// create linked list of packets

	for (fragmentNumber = 1; fragmentNumber <= numOfFragments; fragmentNumber ++) {

		struct packet * newPacket = (struct packet *)malloc(sizeof(struct packet));

		if(newPacket == NULL) {

			printf("Unable to allocate memory for storing packet information. Exiting!");
			exit(1);
		}

		if(fragmentNumber == 1) {	// if fragment number is 1, it is the start of the linked list
			firstPacket = newPacket;
			firstPacket->nextPacket = NULL;	// assign the next packet as Null for now
		}

		

		
	
		


	} // for loop




}




#endif
