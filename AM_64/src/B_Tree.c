#include "bf.h"
#include "B_Tree.h"
#include "AM.h"
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
  //Initialize Block
  BF_Block_Init(&block);
  //Allocate a new block 
  if((AM_errno = BF_AllocateBlock(Open_Files[fileDesc]->fd, block)) != BF_OK){ 
    BF_Block_Destroy(&block);
    AM_errno = 5;
    return AM_errno;  //Return error
  }

  //Get block number
  int block_num;
  BF_GetBlockCounter(Open_Files[fileDesc]->fd,&block_num);
  block_num -= 1;

  //Update root from Open files structure
  Open_Files[fileDesc]->root_number = block_num;
  char * data;
  //Read block
  data = BF_Block_GetData(block);
  memset(data,0,BF_BLOCK_SIZE);
  data[0] = 'i';          //Initialize it as index block
  int m = 1;
  int k = 1;
  memcpy(&data[m],&k,sizeof(int));   //Update counter of index block 
  m += sizeof(int);
  memcpy(&data[m],&p0,sizeof(int));  //Update pointer
  m+=sizeof(int);
  memcpy(&data[m],value1,Open_Files[fileDesc]->attrLength1);  //Insert entry to root
  m+=Open_Files[fileDesc]->attrLength1;
  memcpy(&data[m],&p1,sizeof(int));  //Update pointer
  BF_Block_SetDirty(block);
  BF_UnpinBlock(block);
  //Update the metadata block with the new root block
  BF_GetBlock(Open_Files[fileDesc]->fd,0,block);        //Read block
  data = BF_Block_GetData(block); 
  memcpy(&data[13],&block_num,sizeof(int));             //Update the root number of the B+ tree
  BF_Block_SetDirty(block);
  BF_UnpinBlock(block);
  BF_Block_Destroy(&block);
  return block_num;
}




//Compare function for all possible types, return 0 if x1 < x2 else return 1
int compare(void * value1,void *value2,char attrType,int attrLength){
  if(attrType == 'c'){                                                    //If type is char *
    char x1[attrLength];
    char x2[attrLength];
    memcpy(x1,(char *)value1,attrLength);                                //Copy the string in temp_var
    x1[attrLength-1] = '\0';
    memcpy(x2,(char *)value2,attrLength);                                //Copy the string in temp_var
    x2[attrLength-1] = '\0';
    return (strcmp(x1,x2) < 0? 0:1);    //Return 0 if x1 < x2 else return 1
  }
  else{
    if(attrType == 'f'){                                                 //If type is float
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


int op_function(void * value1,void *value2,char attrType,int attrLength,int op){
  switch(op){
    case EQUAL:{
      if(attrType == 'f'){
        return !((*(float *)value1) == (*(float *)value2));
      }
      else if(attrType == 'i'){
        return !((*(int *)value1) == (*(int *)value2));
      }
      else{
        return (strcmp((char *)value1,(char *)value2) == 0?0:1);
      }
    }
    case NOT_EQUAL:{
      if(attrType == 'f'){
        return ((*(float *)value1) == (*(float *)value2));
      }
      else if(attrType == 'i'){
        return ((*(int *)value1) == (*(int *)value2));
      }
      else{
        return !strcmp((char *)value1,(char *)value2);
      }
    }
    case LESS_THAN:{
      return compare(value1,value2,attrType,attrLength);
    }
    case GREATER_THAN:{
      if(attrType == 'f'){
        return !((*(float *)value1) > (*(float *)value2));
      }
      else if(attrType == 'i'){
        return !((*(int *)value1) > (*(int *)value2));
      }
      else{
        return (strcmp((char *)value1,(char *)value2) > 0?0:1);
      }
    }
    case LESS_THAN_OR_EQUAL:{
      return (compare(value1,value2,attrType,attrLength) && op_function(value1, value2, attrType,attrLength,1));
    }
    case GREATER_THAN_OR_EQUAL:{
      if(attrType == 'f'){
        float t1 = *(float *)value1;
        float t2 = *(float *)value2;
        return ((t1 >= t2) == 1?0:1);
      }
      else if(attrType == 'i'){
        int t1 = *(int *)value1;
        int t2 = *(int *)value2;
        return ((t1 >= t2) == 1?0:1);
      }
      else{
        return (strcmp((char *)value1,(char *)value2) >= 0?0:1);
      }
    }
  }
}




//Sort entries in the block and return 0 if new entry fits in block
//othewise return -1 for full block, can use it both data block and index block
int sort(int fileDesc,char *data,BF_Block * block,int block_num,void *value1,void *value2){
  int attrLength1 = Open_Files[fileDesc]->attrLength1;    //Hold attr1 length
  int attrLength2;                                        //Hold attr2 length
  int counter;                                            //Get the counter of entries
  int size;
  memcpy(&counter,&(data[sizeof(char)]),sizeof(int));
  if(data[0] == 'd'){
    attrLength2 = Open_Files[fileDesc]->attrLength2;
    size = attrLength1+attrLength2;
    if(counter >= ((BF_BLOCK_SIZE - 3*sizeof(int)-sizeof(char))/size)){
      return -1;
    }
  }
  else{
    attrLength2 = sizeof(int);
    size = attrLength1 + attrLength2;
    if(counter >= ((BF_BLOCK_SIZE - 2*sizeof(int)-sizeof(char))/size)){
      return -1;
    }
  }
  int i;
  int m = sizeof(char)+2*sizeof(int);                                       
  int flag = 0;
  //From the last entry to the first of block, find the position to insert the new entry
  for(i=counter-1;i>=0;i--){
    //Compare the new entry with the current, if new < current then shift right the current
    if(!compare(value1,&(data[m+size*i]),Open_Files[fileDesc]->attrType1,attrLength1)){
      memcpy(&data[m+size*(i+1)],&(data[m+size*i]),attrLength1);
      memcpy(&(data[m+size*(i+1)+attrLength1]),&(data[m+size*i+attrLength1]),attrLength2);
    }
    //Else insert the new entry, right of the current
    else{
      memcpy(&data[m+size*(i+1)],value1,attrLength1);
      memcpy(&data[m+size*(i+1)+attrLength1],value2,attrLength2);
      flag = 1;
      break;
    }
  }
  //If new entry is the min key then insert it as first
  if(flag == 0){
    memcpy(&(data[m]),value1,attrLength1);
    memcpy(&(data[m+attrLength1]),value2,attrLength2);
  }
  counter = counter + 1;
  memcpy(&(data[1]),&counter,sizeof(int));     //Increase by 1 the counter
  BF_Block_SetDirty(block);                    //Set block Dirty
  return 0;
}






void split(int fileDesc,BF_Block * block,char *data,int block_num,void *value1,void *value2,Split_Data ** split_data){
  BF_Block *block2;
  BF_Block_Init(&block2);
  char* data2;
  //Create a new block and initialize it
  BF_AllocateBlock(Open_Files[fileDesc]->fd, block2);
  data2 = BF_Block_GetData(block2);

  memset(data2,0,BF_BLOCK_SIZE);
  int new_blocknum;
  //Get new block number
  BF_GetBlockCounter(Open_Files[fileDesc]->fd,&new_blocknum);
  new_blocknum = new_blocknum-1;
  
  int counter,mid,mod,next_pointer,prev_pointer;
  int attrLength1 = Open_Files[fileDesc]->attrLength1;
  int attrLength2;
  
  //Set block id;
  data2[0] = data[0];
  memcpy(&counter,&(data[1]),sizeof(int));
  int m = 2*sizeof(int) +sizeof(char);
  int size;
  //Data block
  if(data[0] == 'd'){
    mid = (int)(((double)(counter+1)/2) +0.5);
    mod = (counter+1)-mid;

    attrLength2 = Open_Files[fileDesc]->attrLength2;
    size = attrLength1 + attrLength2;
    
    int i,temp_mid = -1;
    for(i=0;i<counter-1;i++){
      if(i >= mid && temp_mid != -1){
        break;
      }
      if(!op_function(&(data[m+size*i]),&data[m+(i+1)*size],Open_Files[fileDesc]->attrType1,attrLength1,NOT_EQUAL)){
        temp_mid = i;
      }
    }
    //Fully block with the same value
    if(temp_mid == -1){
      if(!op_function(&(data[m]),value1,Open_Files[fileDesc]->attrType1,attrLength1,EQUAL)){
        fprintf(stderr,"Fully duplicate in a block!\n");
        exit(1);
      }
      //new value greater than block's value
      if(!compare(&(data[m]),value1,Open_Files[fileDesc]->attrType1,attrLength1)){
        //Get the next_pointer
        memcpy(&next_pointer,&(data[BF_BLOCK_SIZE-sizeof(int)]),sizeof(int));
        //Set next_pointer to new_block
        memcpy(&(data[BF_BLOCK_SIZE-sizeof(int)]),&new_blocknum,sizeof(int));
        //Set prev_pointer to old_block
        memcpy(&(data2[m-sizeof(int)]),&block_num,sizeof(int));
        //Set new_pointer to old block's next_pointer
        memcpy(&(data2[BF_BLOCK_SIZE-sizeof(int)]),&next_pointer,sizeof(int));
        BF_Block_SetDirty(block);

        if(next_pointer != -1){
          BF_Block *block3;
          char *data3;
          BF_Block_Init(&block3);
          BF_GetBlock(Open_Files[fileDesc]->fd,next_pointer,block3);
          data3 = BF_Block_GetData(block3);
          memcpy(&(data3[m-sizeof(int)]),&new_blocknum,sizeof(int));
          BF_Block_SetDirty(block3);
          BF_UnpinBlock(block3);
          BF_Block_Destroy(&block3);
        }
      } 
      else{
        //Get the prev_pointer
        memcpy(&prev_pointer,&(data[m-sizeof(int)]),sizeof(int));
        //Set prev_pointer to new_block
        memcpy(&(data[m-sizeof(int)]),&new_blocknum,sizeof(int));
        //Set next_pointer to old_block
        memcpy(&data2[BF_BLOCK_SIZE-sizeof(int)],&block_num,sizeof(int));
        //Set prev_pointer to old block's prev_pointer
        memcpy(&data2[m-sizeof(int)],&prev_pointer,sizeof(int));
        BF_Block_SetDirty(block);
  
        if(prev_pointer != -1){
          BF_Block *block3;
          char * data3;
          BF_Block_Init(&block3);
          BF_GetBlock(Open_Files[fileDesc]->fd,prev_pointer,block3);
          data3 = BF_Block_GetData(block3);
          memcpy(&(data3[BF_BLOCK_SIZE-sizeof(int)]),&new_blocknum,sizeof(int));
          BF_Block_SetDirty(block3);
          BF_UnpinBlock(block3);
          BF_Block_Destroy(&block3);
        }
      }
      memcpy(&(data2[m]),value1,attrLength1);
      memcpy(&(data2[m+attrLength1]),value2,attrLength2);
      int c = 1;
      memcpy(&(data2[1]),&c,sizeof(int));

      memcpy((*split_data)->value,value1,attrLength1);
      memcpy((*split_data)->pointer,&new_blocknum,sizeof(int));
      BF_Block_SetDirty(block2);
      BF_UnpinBlock(block2);
      BF_Block_Destroy(&block2);
      return ;
    }
    //Not full with the same value
    else{
      //If new value is less or equal  
      if(compare(&(data[m+temp_mid*size]),value1,Open_Files[fileDesc]->attrType1,attrLength1)){
        mid = counter - temp_mid -1;
        mod = temp_mid +1;
      }
      //else new value is greater
      else{
        mid = counter - temp_mid;
        mod = temp_mid +1;
      }
      memcpy(&next_pointer,&(data[BF_BLOCK_SIZE-sizeof(int)]),sizeof(int));
      memcpy(&(data[BF_BLOCK_SIZE-sizeof(int)]),&new_blocknum,sizeof(int));
      memcpy(&data2[m-sizeof(int)],&block_num,sizeof(int));
      memcpy(&data2[BF_BLOCK_SIZE-sizeof(int)],&next_pointer,sizeof(int));
    }
  }
  //Index block
  else{
    size = attrLength1 + sizeof(int);
    mid = counter/2;
    mod = counter-mid;
    attrLength2 = sizeof(int);
  }
  int j = mid-1;
  int i = counter-1;
  int flag = 0;
  int null_pointer = -1;

  while(j>=0){
    if(!flag && !op_function(value1,&(data[m+i*size]),Open_Files[fileDesc]->attrType1,attrLength1,GREATER_THAN_OR_EQUAL)){
      memcpy(&(data2[m+j*size]),value1,attrLength1);
      memcpy(&(data2[m+j*size+attrLength1]),value2,attrLength2);
      flag = 1;
    }
    else{
      memcpy(&(data2[m+j*size]),&(data[m+i*size]),attrLength1);
      memcpy(&(data2[m+j*size+attrLength1]),&(data[m+i*size+attrLength1]),attrLength2);
      if(data[0] == 'i'){
        memcpy(&data[m+i*size+attrLength1],&null_pointer,attrLength2);
      }
      i--;
    }
    j--;
  }
  memcpy(&(data[1]),&mod,sizeof(int));
  memcpy(&(data2[1]),&mid,sizeof(int)); 
  BF_Block_SetDirty(block);
  BF_Block_SetDirty(block2);

  if(flag == 0){
    sort(fileDesc,data,block,block_num,value1,value2);
    if(data[0] == 'i'){
      mod--;
      memcpy(&(data[1]),&mod,sizeof(int));
    }
  }
  //printf("Block %c: values %d -> mod: %d - mid:%d\n",data[0],counter+1,mod,mid);

  memcpy((*split_data)->pointer,&new_blocknum,sizeof(int));

  if(data[0] == 'i'){
    memcpy((*split_data)->value,&data[m+mod*size],attrLength1);
    memcpy(&(data2[m-sizeof(int)]),&(data[m+mod*size+attrLength1]),sizeof(int));
    memcpy(&(data[m+mod*size+attrLength1]),&null_pointer,sizeof(int));
  }
  else{
    memcpy((*split_data)->value,&(data2[m]),attrLength1);
  }
  BF_Block_SetDirty(block);
  BF_Block_SetDirty(block2);
  //BF_UnpinBlock(block);
  BF_UnpinBlock(block2);
  if(data[0] == 'd' && next_pointer != -1){
    BF_GetBlock(Open_Files[fileDesc]->fd,next_pointer,block2);
    data2 = BF_Block_GetData(block2);
    memcpy(&data2[m-sizeof(int)],&new_blocknum,sizeof(int));
    BF_Block_SetDirty(block2);
    BF_UnpinBlock(block2);
  }
  BF_Block_Destroy(&block2);
  //return split_data;

}




//Return the block_number that we have to follow for reaching the correct data block 
int traverse(int fileDesc, int block_num,void* value1,int *op)
{
	BF_Block *block;
	BF_Block_Init(&block); 
	char* data;
	int counter;
  int m=0;
  int i = 0; 
  int pointer_value = -1;
  int new_block = -1;
	if ((AM_errno = BF_GetBlock(Open_Files[fileDesc]->fd, block_num, block)) != BF_OK)
	{
		BF_Block_Destroy(&block);
		return AM_errno;
	}

	data = BF_Block_GetData(block);


  m+=1;
  memcpy(&counter, &(data[m]), sizeof(int));

  //Data block, check for InsertEntry or ScanIdex
	if (data[0] == 'd')
	{
    BF_UnpinBlock(block);
    BF_Block_Destroy(&block);
    return -1;						//Return -1 for data block
	}

  //If we have index block search where we have to go!
	m+= 2*sizeof(int);				//position: first key in block
	int next_pointer=-1;											
	int prev_pointer=-1;
  int temp_prev;
	for (i=0;i<counter;i++)
	{
		if (!compare(value1, &(data[m]), Open_Files[fileDesc]->attrType1, Open_Files[fileDesc]->attrLength1)){
      break;
    }
		else
		{
      //hold previous pointer block number
      if(i == 0){
			 memcpy(&prev_pointer, &data[m-sizeof(int)], sizeof(int));
      }
      else{
        //Corner case maybe
        memcpy(&temp_prev, &data[m-sizeof(int)], sizeof(int));
        if(temp_prev != -1){
          prev_pointer = temp_prev;
        }
      }
      //go to the next key
			m+= (sizeof(int) + Open_Files[fileDesc]->attrLength1);
		}
	}

	m-=sizeof(int);
	memcpy(&pointer_value, &data[m], sizeof(int));


	//If pointer == -1 then we have to create a new data block
	if (pointer_value == -1)
	{
		//Insert the pointer of the new block in index block
		BF_GetBlockCounter(Open_Files[fileDesc]->fd, &new_block);
		memcpy(&data[m], &new_block, sizeof(int));								//update new block pointer
	  //Get the next pointer data block to connect it with the new data block
    //that means that there is next pointer
  	if (i<counter)
  	{
  		memcpy(&next_pointer, &data[m+sizeof(int)+Open_Files[fileDesc]->attrLength1], sizeof(int));
  	}
  	BF_Block_SetDirty(block);
  	BF_UnpinBlock(block);

	  //Update the connection with the previous data block
  	int temp_next_pointer = -1;
  	if (prev_pointer!=-1)
  	{
  		BF_GetBlock(Open_Files[fileDesc]->fd, prev_pointer, block);
  		data = BF_Block_GetData(block);
      memcpy(&temp_next_pointer, &(data[BF_BLOCK_SIZE- sizeof(int)]), sizeof(int));
  		memcpy(&(data[BF_BLOCK_SIZE- sizeof(int)]), &new_block,sizeof(int));
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
    data = BF_Block_GetData(block);
		data[0] = 'd';
		int counter_entry=0;
		memcpy(&data[1], &counter_entry, sizeof(int));
		//set first and last pointer to previous and next values
    //Initialize the prev pointer
		memcpy(&data[sizeof(char)+sizeof(int)], &prev_pointer, sizeof(int));
    //Initialize the next pointer							
		memcpy(&data[BF_BLOCK_SIZE - sizeof(int)], &next_pointer, sizeof(int));
		BF_Block_SetDirty(block);
		BF_UnpinBlock(block);


		//Update the connection with the next data block if it exists
		if (next_pointer!=-1)
		{
			BF_GetBlock(Open_Files[fileDesc]->fd, next_pointer, block);
			data = BF_Block_GetData(block);
			memcpy(&data[sizeof(char)+sizeof(int)], &new_block, sizeof(int));
			BF_Block_SetDirty(block);
			BF_UnpinBlock(block);
		}
    BF_Block_Destroy(&block);
    return new_block;
	}
	else{
    BF_UnpinBlock(block);
    BF_Block_Destroy(&block);
    return pointer_value;
  }
}




int Find_ScanIndex_position(char * data,int block_num,void *value1,int counter,int fileDesc,int *op){
  int m = sizeof(char)+2*sizeof(int);
  int size = Open_Files[fileDesc]->attrLength1 + Open_Files[fileDesc]->attrLength2;
  int i = 0;
  switch(*op){
    case EQUAL:{
      for(i=0;i<counter;i++){
        if((!op_function(value1,&data[m+size*i],Open_Files[fileDesc]->attrType1,Open_Files[fileDesc]->attrLength1,EQUAL))){
          *op = i;
          break;
        }
      }
      break;
    }
    case GREATER_THAN:{
      for(i=0;i<counter;i++){
        if((!op_function(value1,&data[m+size*i],Open_Files[fileDesc]->attrType1,Open_Files[fileDesc]->attrLength1,LESS_THAN))){
          *op = i;
          break;
        }
      }
      break;
    }
    case GREATER_THAN_OR_EQUAL:{
      for(i=0;i<counter;i++){
        if((op_function(&data[m+size*i],value1,Open_Files[fileDesc]->attrType1,Open_Files[fileDesc]->attrLength1,LESS_THAN))){
          *op = i;
          break;
        }
      }
      break;
    }
    case LESS_THAN:{
      if(counter > 0){
        if(!compare(&(data[m]),value1,Open_Files[fileDesc]->attrType1,Open_Files[fileDesc]->attrLength1)){
          *op = 0;
        }
        else{
          *op = -1;
        }
      }
      break;
    }
    case LESS_THAN_OR_EQUAL:{
      if(counter > 0){
        if(!compare(&(data[m]),value1,Open_Files[fileDesc]->attrType1,Open_Files[fileDesc]->attrLength1) || 
          !op_function(&(data[m]),value1,Open_Files[fileDesc]->attrType1,Open_Files[fileDesc]->attrLength1,EQUAL)){
          *op = 0;
        }
        else{
          *op = -1;
        }
      }
      break;
    }
    case NOT_EQUAL:{
      int flag = 0;
      for(i=0;i<counter;i++){
        if(!op_function(&(data[m+i*size]),value1,Open_Files[fileDesc]->attrType1,Open_Files[fileDesc]->attrLength1,NOT_EQUAL)){
          *op = i;
          flag = 1;
          break;
        }
      }
      //If all the values in the block are the same with the search value
      //Check if there is next block to set position here
      if(flag == 0){
        int next_pointer = -1;
        memcpy(&next_pointer,&(data[BF_BLOCK_SIZE-sizeof(int)]),sizeof(int));
        if(next_pointer != -1){
          *op = 0;
          return next_pointer;
        }
        else{
          *op = -1;
        }
      }
      break;
    }
  }
  if(i == counter){
    *op = -1;
  }
  return block_num;
}



int Find_Scan(int fileDesc,int block_num,void *value,int *id_block,int *op){
  if(block_num == -1){
    *op = -1;
    return -1;
  }


  BF_Block *block;
  BF_Block_Init(&block);
  char *data;
  int counter;
  int pointer_value;
  int i;
  if ((AM_errno = BF_GetBlock(Open_Files[fileDesc]->fd, block_num, block)) != BF_OK)
  {
    BF_Block_Destroy(&block);
    printf("%d\n",block_num);
    AM_PrintError("Failed open BF_GetBlock");
    return AM_errno;
  }


  data = BF_Block_GetData(block);
  if(data[0] == 'd'){
    memcpy(&counter,&(data[1]),sizeof(int));
    pointer_value = Find_ScanIndex_position(data,block_num,value,counter,fileDesc,op);
    *id_block = pointer_value;
    BF_UnpinBlock(block);
    BF_Block_Destroy(&block);
    return -1;
  }
  //Index find scan block
  else{
    memcpy(&counter,&(data[1]),sizeof(int));
    if(((*op) == EQUAL) || ((*op) == GREATER_THAN) || ((*op) == GREATER_THAN_OR_EQUAL)){
      int m = 2*sizeof(int) + sizeof(char);
      
      for (i=0;i<counter;i++)
      {
        if (!compare(value, &(data[m]), Open_Files[fileDesc]->attrType1, Open_Files[fileDesc]->attrLength1)){
          memcpy(&pointer_value, &data[m-sizeof(int)], sizeof(int));
          if((*op) == 1 || pointer_value != -1){
            break;
          }
        }
        else
        {
          //go to the next key
          m+= (sizeof(int) + Open_Files[fileDesc]->attrLength1);
        }
      }
    }
    else{
      int m = sizeof(char) + sizeof(int);
      for (i=0;i<=counter;i++)
      {
        memcpy(&pointer_value, &data[m], sizeof(int));
        if (pointer_value!=-1){
          break;
        }
        else{
          m+= sizeof(int) + Open_Files[fileDesc]->attrLength1;
        }
      }
    }
    if(pointer_value == -1){
      *op = -1;
    }
  }
  BF_UnpinBlock(block);
  BF_Block_Destroy(&block);
  return pointer_value;
}
