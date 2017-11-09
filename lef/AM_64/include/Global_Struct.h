#ifndef _G_STRUCT_H_
#define _G_STRUCT_H_

#define MAXOPENFILES 20
#define MAXSCANS 20


typedef struct open_files{
	int fd;
	char * filename;
	char attrType1;
	int attrLength1;
	char attrType2;
	int attrLength2;
	int root_number;

}open_files;


typedef struct scan_files{
	int fd;
	int id_block;
	int record_number;
	int op;
	void * value;
}scan_files;



#endif