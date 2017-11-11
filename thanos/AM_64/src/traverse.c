#include <stdio.h>
#include "traverse.h"
#include "Global_Struct.h"
#include "bf.h"

traverse(int fileDesc, void *value1, void *value2,open_files **Open_Files)
{
	BF_Block *block;
  	BF_Block_Init(&block); 
  	char* data;
	
	if (Open_Files[fileDesc]->root_number == -1)
	{
  		if((AM_errno = BF_AllocateBlock(Open_Files[fileDesc]->fd, block)) != BF_OK)
  		{                            
    		BF_CloseFile(Open_Files[fileDesc]->fd);
    		BF_Block_Destroy(&block);
    		return AM_errno;
    	}
    	int m=0, counter=1, def_pointer_num = -1; 
    	data = BF_Block_GetData(block);
    	data[m] = 'i';												//mark it as index block
    	m+=1;
    	memcpy(data+m,&counter,sizeof(int));						//number of key values  
    	m+=sizeof(int);
    	memcpy(data+m, &def_pointer_num, sizeof(int));				//initialize left pointer
    	m+=sizeof(int);
    	if (Open_Files[fileDesc]->attrType1 == 'c')
    	{
    		memcpy(data+m, (char *)value1, strlen(value1)+1);
    		m+=strlen(value1)+1;
    	}
    	else if (Open_Files[fileDesc]->attrType1 == 'i')
    	{
    		memcpy(data+m, (int)value1,sizeof(int));
    		m+=sizeof(int);
    	}
    	else
    	{
    		memcpy(data+m, (float)value1,sizeof(float));
    		m+=sizeof(float);
    	}
    	memcpy(data+m, &def_pointer_num, sizeof(int));				//initialize right pointer
    	BF_GetBlock(Open_Files[fileDesc]->fd, 0, block);			//access metadata
    	data = BF_Block_GetData(block);
    	int new_root =1;
    	memcpy(data+13, &new_root,sizeof(int));						//update root value
    	Open_Files[fileDesc]->root_number = new_root;				//update root value in Open_Files
    	


	}
	else
	{
		BF_GetBlock(Open_Files[fileDesc]->fd, Open_Files[fileDesc]->root_number, block);
		data = BF_Block_GetData(block);

	}


}