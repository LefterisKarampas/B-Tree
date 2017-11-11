#include "AM.h"
#include "bf.h"
#include "Global_Struct.h"
//#include "traverse.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

int AM_errno = AME_OK;

open_files **Open_Files = NULL;
scan_files **Scan_Files = NULL;

int traverse(int fileDesc, int block_num,void* value1);
int compare(void *value1,void *value2,char attrType,int attrLength1);

/*Initialize the global structures and BF*/
void AM_Init() {
  BF_Init(LRU);                                                              //Initialize the BF
  int i;
  Open_Files = (open_files **)malloc(sizeof(open_files *)*MAXOPENFILES);     //Create array of pointers to open_files
  for(i=0;i<MAXOPENFILES;i++){
    Open_Files[i] = NULL;
  }
  Scan_Files = (scan_files **)malloc(sizeof(scan_files *)*MAXSCANS);         //Create array of pointers to scan_files
  for(i=0;i<MAXSCANS;i++){
    Scan_Files[i] = NULL;                                                    //Initialize each pointert to NULL
  }
	return;
}


/*Create a new file based on B+ Tree and initialize the first block with the metadata */
int AM_CreateIndex(char *fileName, 
	               char attrType1, 
	               int attrLength1, 
	               char attrType2, 
	               int attrLength2) {
  int fd;
  BF_Block *block;
  BF_Block_Init(&block);                                                            //Initialize Block
  if((AM_errno = BF_CreateFile(fileName)) != BF_OK){                                //Create a new Block File
    BF_Block_Destroy(&block);
    return AM_errno;                                                                //Return error
  }

  if((AM_errno = BF_OpenFile(fileName, &fd)) != BF_OK){                              //Open the file
    BF_Block_Destroy(&block);
    return AM_errno;                                                                //Return error
  }

  if((AM_errno = BF_AllocateBlock(fd, block)) != BF_OK){                            //Allocate a new block
    BF_CloseFile(fd);
    BF_Block_Destroy(&block);
    return AM_errno;                                                                //Return error
  } 
  int root_num = -1;																//default root number
  char* data;
  data = BF_Block_GetData(block);
  int m = 0;
  memcpy(data, "B+", sizeof(char)*3);                                               //Initialize the new file as B+ file
  m+=3;
  data[3] = attrType1;                                                              //Store the type of key  
  m += 1;
  memcpy(data+m,(char *)&attrLength1,sizeof(int));                                   //Store the length of key 
  m+=sizeof(int);
  data[m] = attrType2;                                                              //Store the type second field
  m+=1;
  memcpy(data+m,(char *)&attrType2,sizeof(int));                                     //Store the length of second field 
  m+=sizeof(int);           
  memcpy(data+m,&root_num,sizeof(int));                                                  //Set root as not exists
  BF_Block_SetDirty(block);                                                         //Set block Dirty
  BF_UnpinBlock(block);                                                             //Unpin Block
  BF_CloseFile(fd);                                                                 //Close file
  BF_Block_Destroy(&block);
  return AME_OK;
}


int AM_DestroyIndex(char *fileName) {

  int i;
  for (i=0;i<MAXOPENFILES;i++)		
  {
  	if (Open_Files[i]!=NULL)
  	{
  		if (strcmp(fileName,Open_Files[i]->filename))	                                  //check for filename
  			continue;
  		return AM_errno;				                                                        //file open - Error
  	}
  }
  unlink(fileName);						                                                       //delete file	
  return AME_OK;
}


int AM_OpenIndex (char *fileName) {
  int fd;
  char *data;
  BF_Block *block;
  BF_Block_Init(&block);
  if((AM_errno = BF_OpenFile(fileName, &fd)) != BF_OK)
  {
    BF_Block_Destroy(&block);
    return AM_errno;
  }
  BF_GetBlock(fd,0,block);
  data = BF_Block_GetData(block);
  if (strcmp(data,"B+"))
  {
    BF_UnpinBlock(block);
    BF_Block_Destroy(&block);
    return AM_errno;
  }
  int i;
  for (i=0;i<MAXOPENFILES;i++)
  {
    if (Open_Files[i] == NULL)	                                                    //find empty cell ,create struct and fill it
    {
    	int m=3;
    	Open_Files[i] = malloc(sizeof(open_files));		
    	Open_Files[i]->fd = fd;
    	memcpy(&(Open_Files[i]->filename),&fileName,strlen(fileName)+1);
    	memcpy(&(Open_Files[i]->attrType1),&data[m],sizeof(char));
    	m+=1;
    	memcpy(&(Open_Files[i]->attrLength1),&data[m],sizeof(int));
    	m+=sizeof(int);
    	memcpy(&(Open_Files[i]->attrType2),&data[m],sizeof(char));
    	m+=1;
    	memcpy(&(Open_Files[i]->attrLength2),&data[m],sizeof(int));
    	m+=sizeof(int);
    	memcpy(&(Open_Files[i]->root_number),&data[m],sizeof(int));
    	BF_UnpinBlock(block);                                                                          
    	BF_Block_Destroy(&block);
    	return i;			                                                                //return cell's position
    } 	
  }
  return -1;
}


int AM_CloseIndex (int fileDesc) {
  int i;

  for (i=0;i<MAXSCANS;i++)                                                      //Search for active scans	
  {
    if (Scan_Files[i]!=NULL)
    {
      if (Scan_Files[i]->fd == fileDesc)                                        //If exists
        return AM_errno;                                                        //Return error
    }
  }

  for (i=0;i<MAXOPENFILES;i++)		                                              
  {
  	if (Open_Files[i]!=NULL)
  	{
  		if (Open_Files[i]->fd == fileDesc)
  		{
  			free(Open_Files[i]);
  			Open_Files[i]=NULL;	                                                    
  		}
  	}
  }

  if((AM_errno = BF_CloseFile(fileDesc)) != BF_OK)
	 return AM_errno;
  return AME_OK;
}


int AM_InsertEntry(int fileDesc, void *value1, void *value2) {
	
	
  return AME_OK;
}


int AM_OpenIndexScan(int fileDesc, int op, void *value) {
  return AME_OK;
}


void *AM_FindNextEntry(int scanDesc) {
	
}


int AM_CloseIndexScan(int scanDesc) {
  return AME_OK;
}


void AM_PrintError(char *errString) {
  fprintf(stderr,"%s",errString);
  switch(AM_errno){
    case 1:
      break;
    case 2:
      break;
    case 3:
      break;
    default:
      ;
  }
}

/*Destroy all the global structures and the BF*/
void AM_Close() {
  int i;
  if(Open_Files != NULL){
    for(i=0;i<MAXOPENFILES;i++){
      if(Open_Files[i] != NULL){
        free(Open_Files[i]);
      }
    }
    free(Open_Files);
  }
  if(Scan_Files != NULL){
    for(i=0;i<MAXSCANS;i++){
      if(Scan_Files[i] != NULL){
        free(Scan_Files[i]);
      }
    }
    free(Scan_Files);
  }
  BF_Close();
}

int traverse(int fileDesc, int block_num,void* value1)
{

	BF_Block *block;
  	BF_Block_Init(&block); 
  	char* data;
  	int counter, i, m=0 , pointer_value,temp;

  	if ((AM_errno = BF_GetBlock(Open_Files[fileDesc]->fd, block_num, block)) != BF_OK)
  	{
  		BF_Block_Destroy(&block);
  		return AM_errno;
  	}

  	data = BF_Block_GetData(block);
  	if (data[0] == 'd')												// in data block , stop here
  	{
  		BF_UnpinBlock(block);
  		BF_Block_Destroy(&block);
  		return -1;													//Return -1 for data block
  	}

  	m+=1;
  	memcpy(&counter, &data[m], sizeof(int));												
  	if (value1!=NULL)
  	{
  		m+= 2*sizeof(int);											//position: first key in block
  		int next_pointer=-1;											
  		int prev_pointer=-1;
	  	void* val2;

	  	for (i=0;i<counter;i++)
	  	{
	  		memcpy(val2, &data[m], Open_Files[fileDesc]->attrLength1);
	  		if (!compare(value1, val2, Open_Files[fileDesc]->attrType1, Open_Files[fileDesc]->attrLength1))
	  			break;
	  		else
	  		{
	  			memcpy(&prev_pointer, &data[m-sizeof(int)], sizeof(int));						//hold previous pointer block number
	  			m+= (sizeof(int) + Open_Files[fileDesc]->attrLength1); 							//go next key
	  		}
	  	}

	  	m-=sizeof(int);
		memcpy(&pointer_value, &data[m], sizeof(int));
		//If pointer == -1 then we have to create a new data block
		if (pointer_value == -1)
		{
			//Insert the pointer of the new block in index block
			int temp;															//The new Block's id
			BF_GetBlockCounter(Open_Files[fileDesc]->fd, &temp);
			memcpy(&data[m], &temp, sizeof(int));								//update new block pointer
		  	//Get the next pointer data block to connect it with the new data block
		  	if (i<c)															//that means that there is next pointer
		  	{
		  		memcpy(&next_pointer, &data[m+sizeof(int)+Open_Files[fileDesc]->attrLength1], sizeof(int));

		  	}
		  	BF_Block_SetDirty(block);
		  	BF_UnpinBlock(block);
		  	//Update the connection with the previous data block
		  	if (prev_pointer!=-1)
		  	{
		  		int temp_next_pointer;
		  		BF_GetBlock(Open_Files[fileDesc]->fd, prev_pointer, block);
		  		data = BF_Block_GetData(block);
		  		memcpy(&temp_next_pointer, &data[BF_BLOCK_SIZE- sizeof(int)], sizeof(int)]);
		  		memcpy(&data[BF_BLOCK_SIZE- sizeof(int)], &temp,sizeof(int)]);
				BF_Block_SetDirty(block);
		  		BF_UnpinBlock(block);
		  	}
		  	//Store the next_pointer
		  	if (temp_next_pointer!=-1)
		  	{
		  		next_pointer = temp_next_pointer;
		  	}
		  	//Create the new data block and initilize it
			BF_AllocateBlock(Open_Files[fileDesc]->fd, block);
			data[0] = 'd';
			int temp_num=0;
			memcpy(&data[1], &temp_num, sizeof(int));
			//set first and last pointer to previous and next values
			memcpy(&data[sizeof(char)+sizeof(int)], &prev_pointer, sizeof(int));				//Initialize the prev pointer								
			memcpy(&data[BF_BLOCK_SIZE - sizeof(int)], &next_pointer, sizeof(int));				//Initialize the next pointer
			BF_Block_SetDirty(block);
			BF_UnpinBlock(block);
			//Update the conneciton with the next data block 
			if (next_pointer!=-1)
			{
				BF_GetBlock(Open_Files[fileDesc]->fd, next_pointer, block);
				data = BF_Block_GetData(block);
				memcpy(&data[sizeof(char)+sizeof(int)], &temp, sizeof(int));
				BF_Block_SetDirty(block);
				BF_UnpinBlock(block);
			}

		}
		else
			BF_UnpinBlock(block);
		BF_Block_Destroy(&block);
		return pointer_value;					//Return the block		

	}
	//Find the most left block for OpenScanIndex (NOT_EQUAL,LESS,LESS_OR_EQUAL)
	else
	{
		m+=sizeof(int);
		for (i=0;i<=counter;i++)
		{
			memcpy(&pointer_value, &data[m], sizeof(int));
			if (pointer_value!=-1)
				return pointer_value;
			else
				m+= sizeof(int) + Open_Files[fileDesc]->attrLength1;
		}
	}
}

int compare(void *value1,void *value2,char attrType,int attrLength1)
{
  if(attrType == 'c')
  {
    char x1[attrLength1];
    char x2[attrLength1];
    strncpy(x1,(char *)value1,attrLength1);
    x1[attrLength1-1] = '\0';
    strncpy(x2,(char *)value2,attrLength1);
    x2[attrLength1-1] = '\0';
    return (strcmp(x1,x2) < 0? 0:1);
  }
  else
  {
    if(attrType == 'f')
    {
      if((*(double *)value1) < *((double *)value2))
      {
        return 0; 
      }
    }
    else if(*((int *)value1) < *((int *)value2))
    {
      return 0;
    }
  }
  return 1;
}


