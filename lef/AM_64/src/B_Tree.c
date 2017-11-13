#include "bf.h"
#include "B_Tree.h"
#include "Global_Struct.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern int AM_errno;
extern open_files **Open_Files;
extern scan_files **Scan_Files;

//Create and Initialize the root of B-Tree,
//Update the metadata with the new root and the OpenFiles structure
int Initialize_Root(int fileDesc,void * value1,int p0,int p1){
  BF_Block *block;
  BF_Block_Init(&block);                                                          //Initialize Block
  if((AM_errno = BF_AllocateBlock(Open_Files[fileDesc]->fd, block)) != BF_OK){    //Allocate a new block
    BF_Block_Destroy(&block);
    return AM_errno;                                                              //Return error
  }
  int block_num;
  BF_GetBlockCounter(Open_Files[fileDesc]->fd,&block_num);
  block_num -= 1;
  Open_Files[fileDesc]->root_number = block_num;                                  //Update root from Open files structure
  char * data;
  data = BF_Block_GetData(block);                                                 //Read block
  data[0] = 'i';                                                                  //Initialize it as index block
  int k = 1;
  int m = 1;
  memcpy(&data[m],&k,sizeof(int));                                                //Update counter of index block 
  m += sizeof(int);
  memcpy(&data[m],&p0,sizeof(int));                                                //Update pointer to NULL
  m+=sizeof(int);
  memcpy(&data[m],value1,Open_Files[fileDesc]->attrLength1);                      //Insert entry to root
  m +=  Open_Files[fileDesc]->attrLength1;
  memcpy(&data[m],&p1,sizeof(int));
  BF_Block_SetDirty(block);
  BF_UnpinBlock(block);
  //Update the metadata block with the new root block
  BF_GetBlock(Open_Files[fileDesc]->fd,0,block);                                  //Read block
  data = BF_Block_GetData(block); 
  memcpy(&data[13],&block_num,sizeof(int));                                       //Update root tree
  BF_Block_SetDirty(block);
  BF_UnpinBlock(block);
  BF_Block_Destroy(&block);
}




//Compare function for all possible types, return 0 if x1 < x2 else return 1
int compare(void * value1,void *value2,char attrType,int attrLength){
  if(attrType == 'c'){                                                    //If type is char *
    char x1[attrLength];
    char x2[attrLength];
    strncpy(x1,(char *)value1,attrLength);                                //Copy the string in temp_var
    x1[attrLength-1] = '\0';
    strncpy(x2,(char *)value2,attrLength);                                //Copy the string in temp_var
    x2[attrLength-1] = '\0';
    return (strcmp(x1,x2) < 0? 0:1);    //Return 0 if x1 < x2 else return 1
  }
  else{
    if(attrType == 'f'){                                                  //If type is float
      if((*(float *)value1) < *((float *)value2)){
        return 0;                                                         //Return 0 if x1 < x2
      }
    }
    else if(*((int *)value1) < *((int *)value2)){                         //If type is int
      return 0;                                                           //Return 0 if x1 <X2
    }
  }
  return 1;                                                               //Return 1 if x1>=x2
}





//Sort entries in the block and return 0 if new entry fits in block
//othewise return -1 for full block, can use it both data block and index block
int sort(int index,int block_num,void *value1,void *value2){
  BF_Block *block;
  BF_Block_Init(&block);
  char* data;
  BF_GetBlock(Open_Files[index]->fd,block_num,block);                                        //Read block
  data = BF_Block_GetData(block);
  int attrLength1 = Open_Files[index]->attrLength1;                       //Hold attr1 length
  int attrLength2;                                                        //Hold attr2 length
  int * counter = (int *)&(data[1]);                                      //Get the counter of entries
  char id = data[0];
  if(id == 'd'){
    attrLength2 = Open_Files[index]->attrLength2;
    if(*counter >= ((BF_BLOCK_SIZE - 3*sizeof(int)+sizeof(char))/(attrLength1+attrLength2)))
      return -1;
  }
  else{
    attrLength2 = sizeof(int);
    if(*counter >= ((BF_BLOCK_SIZE - 2*sizeof(int)+sizeof(char))/(attrLength1+sizeof(int))))
      return -1;
  }
  int i;
  int m = sizeof(char)+2*sizeof(int);                                       
  int flag = 0;
  //From the last entry to the first of block, find the position to insert the new entry
  for(i=*counter-1;i>=0;i++){
    void * temp;
    temp = &data[m+(attrLength1+attrLength2)*i];
    //Compare the new entry with the current, if new < current then shift right the current
    if(!compare(value1,temp,Open_Files[index]->attrType1,attrLength1)){ 
      memcpy(&data[m+(attrLength1+attrLength2)*(i+1)],(char *)temp,attrLength1);
      temp = &data[m+(attrLength1+attrLength2)*i+attrLength1];
      memcpy(&data[m+(attrLength1+attrLength2)*(i+1)+attrLength1],(char *)temp,attrLength2);
    }
    //Else insert the new entry, right of the current
    else{
      memcpy(&data[m+(attrLength1+attrLength2)*(i+1)],(char *)value1,attrLength1);
      memcpy(&data[m+(attrLength1+attrLength2)*(i+1)+attrLength1],(char *)value2,attrLength2);
      flag = 1;
      break;
    }
  }
  //If new entry is the min key then insert it as first
  if(flag = 0){
    memcpy(&data[m],(char *)value1,attrLength1);
    memcpy(&data[m+attrLength1],(char *)value2,attrLength2);
  }
  *counter = *(counter) + 1;
  memcpy(&data[1],counter,sizeof(int));                                             //Increase by 1 the counter
  BF_Block_SetDirty(block);                                                         //Set block Dirty
  BF_UnpinBlock(block);                                                             //Unpin Block
  BF_Block_Destroy(&block);
  return 0;
}



Split_Data * split(int fileDesc,int block_num,void *value1,void *value2){
  BF_Block *block;
  BF_Block_Init(&block);
  BF_Block *block2;
  BF_Block_Init(&block2);
  char* data;
  BF_GetBlock(Open_Files[fileDesc]->fd,block_num,block);                                                  //Read block
  data = BF_Block_GetData(block);
  if((AM_errno = BF_AllocateBlock(Open_Files[fileDesc]->fd, block2)) != BF_OK){     //Allocate a new block
    BF_CloseFile(Open_Files[fileDesc]->fd);
    BF_Block_Destroy(&block2);
    AM_errno = 2;
    return NULL;                                                                    //Return error
  } 
  int new_block;
  BF_GetBlockCounter(Open_Files[fileDesc]->fd,&new_block);
  new_block -=1;
  char* data2;
  data2 = BF_Block_GetData(block2);
  data2[0] = data[0];
  int *counter = (int *)&data[1];
  //Split data block
  int mid,mod,size,m;
  int attrLength2;
  int attrLength1 = Open_Files[fileDesc]->attrLength1;

  //Split data block
  if(data[0] == 'd'){
    attrLength2 = Open_Files[fileDesc]->attrLength2;
    m = sizeof(char) + 3*sizeof(int);
    size = attrLength1 + attrLength2;
    mid = ((BF_BLOCK_SIZE-m)/size + 1)/2 + 0.5;                                     //Find the number of the entries that move to new block
    memcpy(&data[BF_BLOCK_SIZE-sizeof(int)],&new_block,sizeof(int));                //Update pointer to next
    memcpy(&data2[sizeof(char)+sizeof(int)],&block_num,sizeof(int));                //Update pointer to prev
  }
  //Split index block
  else{
    attrLength2 = sizeof(int);
    m = sizeof(char) + 2*sizeof(int);
    size = attrLength1 + attrLength2;
    mid = ((BF_BLOCK_SIZE-m)/size + 1.5)/2 + 0.5;                                   //Find the number of the entries that move to new block
  }
  mod = (*counter) +1 - mid;
  memcpy(&data2[1],&mid,sizeof(int));                                               //Update the counter to new block
  memcpy(&data[1],&mod,sizeof(int));                                                //Update the counter to old block
  int i;
  int j = mid-1;
  int flag = 0;
  //From the end until the mid, move the entries to new block from the end
  for(i = *counter-1;i<mid;i--){
    //If new value is less than current, move the new to other block
    if(!flag && compare(value1,&data[m+j*size],Open_Files[fileDesc]->attrType1,attrLength1)){
      memcpy(&data2[m+j*size],value1,Open_Files[fileDesc]->attrLength1);                        //Copy new key value
      memcpy(&data2[m+j*size+attrLength1],value2,attrLength2);                                 
      flag = 1;
    }
    //If new value is bigger than current, move current to other block
    else{
      memcpy(&data2[m+j*size],&data[m+i*size],attrLength1);                                     //Copy current key value
      memcpy(&data2[m+j*size+attrLength1],&data[m+i*size+attrLength1],attrLength2);
    }
  }
  Split_Data * split_data = (Split_Data *)malloc(sizeof(Split_Data));       
  split_data->pointer = new_block;
  memcpy(split_data->value,&data2[m],attrLength1);
  BF_Block_SetDirty(block);                                                         //Set old block Dirty
  BF_UnpinBlock(block);                                                             //Unpin old Block
  BF_Block_Destroy(&block);
  BF_Block_SetDirty(block2);                                                         //Set new block Dirty
  BF_UnpinBlock(block2);                                                             //Unpin new Block
  BF_Block_Destroy(&block2);
  return split_data;                                                                 //Return value for split and new pointer
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
		  	if (i<counter)															//that means that there is next pointer
		  	{
		  		memcpy(&next_pointer, &data[m+sizeof(int)+Open_Files[fileDesc]->attrLength1], sizeof(int));

		  	}
		  	BF_Block_SetDirty(block);
		  	BF_UnpinBlock(block);
		  	//Update the connection with the previous data block
		  	int temp_next_pointer = -1;
		  	if (prev_pointer!=-1)
		  	{
		  		int temp_next_pointer;
		  		BF_GetBlock(Open_Files[fileDesc]->fd, prev_pointer, block);
		  		data = BF_Block_GetData(block);
		  		memcpy(&temp_next_pointer, &data[BF_BLOCK_SIZE- sizeof(int)], sizeof(int));
		  		memcpy(&data[BF_BLOCK_SIZE- sizeof(int)], &temp,sizeof(int));
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
