// header file that contains the packet struct and the declarations of functions that will be used on packets

#ifndef headerFile
#define headerFile

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
struct packet createPackets(char* fileName){

	struct packet* firstPacket, previousPacket;
	
	// open file, and read-only (b is for binary files as well)
	FILE* fp = fopen(fileName, "rb"); 

	// get file size
	fseek(fp,0, SEEK_END);
	int fileSize = ftell(fp);
	fseek(fp,0, SEEK_SET);

	numOfFragments = (fileSize / maxFragmentLen) + 1;

	// create linked list of packets

	



}




#endif