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
int sort(int fileDesc,int block_num,void *value1,void *value2){
  BF_Block *block;
  BF_Block_Init(&block);
  char* data;
  BF_GetBlock(Open_Files[fileDesc]->fd,block_num,block);                                        //Read block
  data = BF_Block_GetData(block);
  char id = data[0];
  int attrLength1 = Open_Files[fileDesc]->attrLength1;                       //Hold attr1 length
  int attrLength2;                                                        //Hold attr2 length
  int counter;                                           //Get the counter of entries
  strncpy((char *)&counter,&(data[sizeof(char)]),sizeof(int));
  if(id == 'd'){
    attrLength2 = Open_Files[fileDesc]->attrLength2;
    if(counter >= ((BF_BLOCK_SIZE - 3*sizeof(int)-sizeof(char))/(attrLength1+attrLength2)))
      return -1;
  }
  else{
    attrLength2 = sizeof(int);
    if(counter >= ((BF_BLOCK_SIZE - 2*sizeof(int)-sizeof(char))/(attrLength1+sizeof(int))))
      return -1;
  }
  int i;
  int m = sizeof(char)+2*sizeof(int);                                       
  int flag = 0;
  //From the last entry to the first of block, find the position to insert the new entry
  void * temp;
  if(Open_Files[fileDesc]->attrType1 == 'i'){
    temp = (int *)malloc(sizeof(int));
  }else if(Open_Files[fileDesc]->attrType1 == 'f'){
   temp = (float *)malloc(sizeof(float));
  }
  else{
    temp = (char *)malloc(sizeof(char)*Open_Files[fileDesc]->attrLength1);
  }
  for(i=counter-1;i>=0;i--){
    memcpy(temp,&(data[m+(attrLength1+attrLength2)*i]),sizeof(attrLength1));
    //Compare the new entry with the current, if new < current then shift right the current
    if(!compare(value1,temp,Open_Files[fileDesc]->attrType1,attrLength1)){
      //printf("Bigger\n"); 
      memcpy(&data[m+(attrLength1+attrLength2)*(i+1)],temp,attrLength1);
      memcpy(temp,&data[m+(attrLength1+attrLength2)*i+attrLength1],attrLength2);
      memcpy(&data[m+(attrLength1+attrLength2)*(i+1)+attrLength1],temp,attrLength2);
    }
    //Else insert the new entry, right of the current
    else{
      //printf("smaller\n"); 
      memcpy(&data[m+(attrLength1+attrLength2)*(i+1)],value1,attrLength1);
      memcpy(&data[m+(attrLength1+attrLength2)*(i+1)+attrLength1],value2,attrLength2);
      flag = 1;
      break;
    }
  }
  //If new entry is the min key then insert it as first
  if(flag == 0){
    memcpy(&(data[m]),value1,attrLength1);
    memcpy(&(data[m+attrLength1]),value2,attrLength2);
  }
  //free(temp);
  counter = counter + 1;
  memcpy(&(data[1]),&counter,sizeof(int));                                             //Increase by 1 the counter
  BF_Block_SetDirty(block);                                                         //Set block Dirty
  BF_UnpinBlock(block);
  BF_Block_Destroy(&block);
  return 0;
}





Split_Data * split(int fileDesc,BF_Block * block,char *data,int block_num,void *value1,void *value2){
  BF_Block *block2;
  BF_Block_Init(&block2);
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
  int temp_next_pointer = -1;                                                            //Pointer to next before split
  int attrLength2;
  int attrLength1 = Open_Files[fileDesc]->attrLength1;
  mid = ((*counter)+1)/2 ;                                               //Find the number of the entries that move to new block
  if((((*counter)+1) % 2 != 0)){
    mid+=1;
  }
  m = sizeof(char) + 2*sizeof(int);
  //Split data block
  if(data[0] == 'd'){
    attrLength2 = Open_Files[fileDesc]->attrLength2;
    memcpy(&temp_next_pointer,&data[BF_BLOCK_SIZE-sizeof(int)],sizeof(int));        //Read the pointer to next before split
    memcpy(&data[BF_BLOCK_SIZE-sizeof(int)],&new_block,sizeof(int));                //Update pointer to next
    memcpy(&data2[sizeof(char)+sizeof(int)],&block_num,sizeof(int));                //Update pointer to prev
    memcpy(&data2[BF_BLOCK_SIZE-sizeof(int)],&temp_next_pointer,sizeof(int));                //Update pointer to prev
  }
  //Split index block
  else{
    attrLength2 = sizeof(int);
  }
  size = attrLength1 + attrLength2;
  mod = (*counter)+1 - mid;
  int i = (*counter)-1;
  int j = mid -1;
  int flag = 0;
  int null_pointer = -1;

  //From the end until the mid, move the entries to new block from the end
  if(data[0] == 'd'){
    /*int temp_mid = mid;
    while(mod <= (*counter) && !op_function(&(data[m+size*(mod-1)]), &(data[m+size*mod]),Open_Files[fileDesc]->attrType1,attrLength1,1)){
        printf("%d == %d\n",*(int *)&(data[m+size*(mod-1)]),*(int*)&(data[m+size*(mod)]));
        printf("ITS TIME %d %d\n",mod,temp_mid);
        mod++;
        temp_mid--;
    }
    if(mod == (*counter)){
      temp_mid = mid;
      mod = (*counter)- mid;
      while(!op_function(&(data[m+size*(mod-1)]), &(data[m+size*mod]),Open_Files[fileDesc]->attrType1,attrLength1,1)){
        mod--;
        temp_mid++;
        if(mod < 0){
          fprintf(stderr,"Error overflow with the same value!\n");
          exit(1);
        }

      }
    }
    //We have a full block with the same value
    if(!op_function(&(data[m]),&(data[m+((*counter)-1)]),Open_Files[fileDesc]->attrType1,attrLength1,1)){
      if(!op_function(&(data[m]),value1,Open_Files[fileDesc]->attrType1,attrLength1,1)){
        fprintf(stderr,"Error overflow with the same value!\n");
        exit(1);  
      }
      else{
        printf("%d != %d\n",*(int *)&data[m],*(int *)value1);
        memcpy(&data2[m+j*size],value1,Open_Files[fileDesc]->attrLength1);                        //Copy new key value
        memcpy(&data2[m+j*size+attrLength1],value2,attrLength2);
        mod = 1;
        mid = *counter; 
        flag = 1;
        j = -1;
      }
    }
    else{
      int k = 0;
      int p = 0;
      while(!op_function(&(data[m+size*(mod-1)]), &(data[m+size*mod]),Open_Files[fileDesc]->attrType1,attrLength1,1)){
        printf("%d == %d\n",*(int *)&(data[m+size*(mod-1)]),*(int *)&(data[m+size*mod]));
        if(p % 2 != 0){
          mod -= 2*k;
          if(mod <= 0){
            printf("Error %d\n",mod);
            exit(1);
          }
        }
        else{
          mod +=2*k+1;
          k++;
          if(mod > *counter){
            printf("Error %d\n",mod);
            exit(1);
          }
        }
        p++;
      }
      mid = *counter -mod;
    }
    printf("OUT: %d != %d,\nmid: %d - mod: %d\n",*(int *)&(data[m+size*(mod-1)]),*(int *)&(data[m+size*mod]),mid,mod);
    */j = mid -1;
    while(j>=0){
      //If new value is greater than current, move the new to other block
      if(!flag && compare(value1,&(data[m+i*size]),Open_Files[fileDesc]->attrType1,attrLength1)){
        memcpy(&data2[m+j*size],value1,Open_Files[fileDesc]->attrLength1);                        //Copy new key value
        memcpy(&data2[m+j*size+attrLength1],value2,attrLength2);                                 
        flag = 1;
      }
      //If new value is less than current, move current to other block
      else{
        memcpy(&data2[m+j*size],&(data[m+i*size]),attrLength1);                                     //Copy current key value
        memcpy(&data2[m+j*size+attrLength1],&data[m+i*size+attrLength1],attrLength2);
        //Each key-pointer has moved to new block, set the old pointer to Null
        i--;
      }
      j--;
    }
    memcpy(&data2[1],&mid,sizeof(int));                                               //Update the counter to new block
    memcpy(&data[1],&mod,sizeof(int));                                                //Update the counter to old block
  }
  else{
    if(mid > mod){
      j--;
    }
    for(i = (*counter)-1;i>=mid-1;){
      //If new value is greater than current, move the new to other block
      if(!flag && compare(value1,&(data[m+i*size]),Open_Files[fileDesc]->attrType1,attrLength1)){
        memcpy(&data2[m+j*size],value1,Open_Files[fileDesc]->attrLength1);                        //Copy new key value
        memcpy(&data2[m+j*size+attrLength1],value2,attrLength2);                                 
        flag = 1;
      }
      //If new value is less than current, move current to other block
      else{
        memcpy(&data2[m+j*size],&data[m+i*size],attrLength1);                                     //Copy current key value
        memcpy(&data2[m+j*size+attrLength1],&data[m+i*size+attrLength1],attrLength2);
        //Each key-pointer has moved to new block, set the old pointer to Null
        i--;
      }
      j--;
    }
    memcpy(&data2[1],&mod,sizeof(int));                                               //Update the counter to new block
    memcpy(&data[1],&mid,sizeof(int));                                                //Update the counter to old block
  }
  BF_Block_SetDirty(block);   

  //If new value has not inserted yet (it belongs to old block), sort the old block
  if(flag == 0){
    sort(fileDesc,block_num,value1,value2);
    memcpy(&data[sizeof(char)],&mod,sizeof(int));                          
  }

  Split_Data * split_data = (Split_Data *)malloc(sizeof(Split_Data));
  if(Open_Files[fileDesc]->attrType1 == 'i'){
    split_data->value = (int *)malloc(Open_Files[fileDesc]->attrLength1);
  }
  else if(Open_Files[fileDesc]->attrType1 == 'f'){
   split_data->value = (float *)malloc(Open_Files[fileDesc]->attrLength1);
  }
  else{
   split_data->value = (char *)malloc(Open_Files[fileDesc]->attrLength1);
  }       
  split_data->pointer = &new_block;
  if(data[0] == 'd'){
    memcpy(split_data->value,&(data2[m]),attrLength1);
  }
  else{
    if(mid > mod){
      memcpy(split_data->value,&data[m+mod*size],attrLength1);                      //Get the key which has to go up
      memcpy(&data2[sizeof(char)+sizeof(int)],&data[m+mod*size+attrLength1],sizeof(int));   //Relocate the pointer
      memcpy(&data[m+mod*size+attrLength1],&null_pointer,attrLength2);
    }
    else{
      memcpy(split_data->value,&data[m+(mod-1)*size],attrLength1);
      memcpy(&data2[sizeof(char)+sizeof(int)],&data[m+(mod-1)*size+attrLength1],sizeof(int));   //Relocate the pointer
      memcpy(&data[m+(mod-1)*size+attrLength1],&null_pointer,attrLength2);
    }
  }

  BF_Block_SetDirty(block);                                                         //Set old block Dirty
  BF_Block_SetDirty(block2);                                                         //Set new block Dirty
  BF_UnpinBlock(block2);                                                  //Unpin new Block
  if((data[0] == 'd') && (temp_next_pointer != -1)){
    BF_GetBlock(Open_Files[fileDesc]->fd,temp_next_pointer,block2);                   //Read block
    data2 = BF_Block_GetData(block);
    memcpy(&data2[sizeof(char)+sizeof(int)],&new_block,sizeof(int));
    BF_Block_SetDirty(block2);                                                         //Set new block Dirty
    BF_UnpinBlock(block2);                                                             //Unpin new Block
  }
  BF_Block_Destroy(&block2);
  return split_data;                                                                 //Return value for split and new pointer
}






int traverse(int fileDesc, int block_num,void* value1,int *op)
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
  m+=1;
  memcpy(&counter, &data[m], sizeof(int));
	if (data[0] == 'd')												// in data block , stop here
	{
    int size = Open_Files[fileDesc]->attrLength1 + Open_Files[fileDesc]->attrLength2;
    m = sizeof(char)+2*sizeof(int);
    switch(*op){
      case 1:{
        for(i=0;i<counter;i++){
          if((!op_function(value1,&data[m+size*i],Open_Files[fileDesc]->attrType1,Open_Files[fileDesc]->attrLength1,*op))){
            *op = i;
            break;
          }
        }
        break;
      }
      case 4:{
        for(i=0;i<counter;i++){
          if((!op_function(value1,&data[m+size*i],Open_Files[fileDesc]->attrType1,Open_Files[fileDesc]->attrLength1,3))){
            *op = i;
            break;
          }
        }
        break;
      }
      case 6:{
        for(i=0;i<counter;i++){
          if((!op_function(value1,&data[m+size*i],Open_Files[fileDesc]->attrType1,Open_Files[fileDesc]->attrLength1,1))){
            *op = i;
            break;
          }
        }
        break;
      }
      default:{
        i = 0;
        *op = i;
        break;
      }
    }
    if(i == counter){
      *op = -1;
    }
    BF_UnpinBlock(block);
    BF_Block_Destroy(&block);
    return -1;													//Return -1 for data block
	}
	if (value1!=NULL)
	{
		m+= 2*sizeof(int);											//position: first key in block
		int next_pointer=-1;											
		int prev_pointer=-1;
    void* val2;
    if(Open_Files[fileDesc]->attrType1 == 'i'){
      val2 = (int *)malloc(sizeof(int));
    }else if(Open_Files[fileDesc]->attrType1 == 'f'){
     val2 = (float *)malloc(sizeof(float));
    }
    else{
      val2 = (char *)malloc(sizeof(char)*Open_Files[fileDesc]->attrLength1);
    }
  	for (i=0;i<counter;i++)
  	{
  		memcpy(val2, &(data[m]), Open_Files[fileDesc]->attrLength1);
  		if (!compare(value1, val2, Open_Files[fileDesc]->attrType1, Open_Files[fileDesc]->attrLength1)){
        break;
      }
  		else
  		{
  			memcpy(&prev_pointer, &data[m-sizeof(int)], sizeof(int));						//hold previous pointer block number
  			m+= (sizeof(int) + Open_Files[fileDesc]->attrLength1); 							//go next key
  		}
  	}
  	m-=sizeof(int);
		memcpy(&pointer_value, &data[m], sizeof(int));
		//If pointer == -1 then we have to create a new data block
    //
    if(*op == -1){
  		if (pointer_value == -1)
  		{
  			//Insert the pointer of the new block in index block
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
        data = BF_Block_GetData(block);
  			data[0] = 'd';
  			int counter_entry=0;
  			memcpy(&data[1], &counter_entry, sizeof(int));
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
  		else{
        BF_UnpinBlock(block);
        BF_Block_Destroy(&block);
        return pointer_value;
      }
      BF_UnpinBlock(block);
      BF_Block_Destroy(&block);
      return temp;          //Return the block
    }
    else{
      while(pointer_value == -1 && i < counter){
          m += sizeof(int) + Open_Files[fileDesc]->attrLength1;
          memcpy(&pointer_value, &data[m], sizeof(int));
          i++;
      }
    }
	}
	//Find the most left block for OpenScanIndex (NOT_EQUAL,LESS,LESS_OR_EQUAL)
	else
	{
		m+=sizeof(int);
		for (i=0;i<=counter;i++)
		{
			memcpy(&pointer_value, &data[m], sizeof(int));
			if (pointer_value!=-1){
        break;
      }
			else
				m+= sizeof(int) + Open_Files[fileDesc]->attrLength1;
		}
	}
  BF_UnpinBlock(block);
  BF_Block_Destroy(&block);
  return pointer_value;

}




int op_function(void * value1,void *value2,char attrType,int attrLength,int op){
  switch(op){
    case 1:{
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
    case 2:{
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
    case 3:{
      return compare(value1,value2,attrType,attrLength);
    }
    case 4:{
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
    case 5:{
      return (compare(value1,value2,attrType,attrLength) && op_function(value1, value2, attrType,attrLength,1));
    }
    case 6:{
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

