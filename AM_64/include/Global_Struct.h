#ifndef _G_STRUCT_H_
#define _G_STRUCT_H_

#define MAXOPENFILES 20
#define MAXSCANS 20


typedef struct open_files{
	int fd;							//FileDescriptor 
	char * filename;				//FileName
	char attrType1;					//Type of first field
	int attrLength1;				//Length of first field
	char attrType2;					//Type of second field
	int attrLength2;				//Length of second field
	int root_number;				//B+Tree's root (block number)
}open_files;


typedef struct scan_files{
	int fd;							//Index to OpenFiles
	int id_block;					//Block number
	int record_number;				//Position of record
	int op;							//Comparison operator
	void * value;					//Comparison value
}scan_files;



#endif