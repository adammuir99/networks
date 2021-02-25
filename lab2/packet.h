// header file that contains the packet struct and the declarations of functions that will be used on packets

#ifndef headerFile
#define headerFile

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


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

	struct packet * firstPacket;
	struct packet *previousPacket;
	char data[maxFragmentLen];
	int fragmentNumber, dataSize;
	
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

		newPacket->nextPacket = NULL;	// assign the next packet as Null for now

		if(fragmentNumber == 1) {	// if fragment number is 1, it is the start of the linked list
			firstPacket = newPacket;
		}
		
		else {	// if not the start of the linked list, the next packet after the previous one is the current one

			previousPacket->nextPacket = newPacket;
		}

		dataSize = fread(data, sizeof(char), maxFragmentLen, fp); // read an array of 1000 elements, each of size char, from the fp stream and store into data
		newPacket->filename = fileName;
		newPacket->frag_no = fragmentNumber;
		newPacket->size = dataSize;
		newPacket->total_frag = numOfFragments;
		memcpy(newPacket->filedata, data, dataSize); // copy dataSize amount of bytes from data to newPacket->filedata

		previousPacket = newPacket;	// make the previous packet the current packet
	
	} // for loop

	fclose(fp);	// close the file stream
	return firstPacket;	// return the head of the linked list

}	// createPackets




#endif
