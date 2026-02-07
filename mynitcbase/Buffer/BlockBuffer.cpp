#include "BlockBuffer.h"
#include <cstdlib>
#include <cstring>

// the declarations for these functions can be found in "BlockBuffer.h"

int compareAttrs(union Attribute attr1, union Attribute attr2, int attrType)
{
    double diff;
    if (attrType==STRING)
    {
        diff=strcmp(attr1.sVal,attr2.sVal);
    }
    else
    {
        diff=attr1.nVal-attr2.nVal;
    }

    if (diff>0)
    {
        return 1;
    }

    if (diff<0)
    {
        return -1;
    }
    if (diff==0)
    {
        return 0;
    }
    return SUCCESS;
}
BlockBuffer::BlockBuffer(int blockNum) 
{
  // initialise this.blockNum with the argument
  this -> blockNum = blockNum;
}

// calls the parent class constructor
RecBuffer::RecBuffer(int blockNum) : BlockBuffer::BlockBuffer(blockNum) 
{}

// load the block header into the argument pointer
int BlockBuffer::getHeader(struct HeadInfo *head) {

      unsigned char * bufferPtr;
      int ret= loadBlockAndGetBufferPtr(&bufferPtr);
      if (ret!=SUCCESS)
      {
            return ret;
      }
      // populate the numEntrmemcpy(&head->numEntries, buffer +16, 4);ies, numAttrs and numSlots fields in *head
      memcpy(&head->numSlots, bufferPtr + 24, 4);
      memcpy(&head->numAttrs, bufferPtr +20, 4);
      memcpy(&head->numEntries, bufferPtr +16, 4);
      memcpy(&head->rblock, bufferPtr + 12, 4);
      memcpy(&head->lblock, bufferPtr + 8, 4);

      return SUCCESS;
}

// load the record at slotNum into the argument pointer
int RecBuffer::getRecord(union Attribute *rec, int slotNum) 
{
      struct HeadInfo head;
      unsigned char * bufferPtr;

      // get the header using this.getHeader() function
      this -> getHeader(&head);

      int attrCount = head.numAttrs;
      int slotCount = head.numSlots;

      // read the block at this.blockNum into a buffer

      int ret= loadBlockAndGetBufferPtr(&bufferPtr);
      if (ret !=SUCCESS)
      {
            return ret;
      }

      /* record at slotNum will be at offset HEADER_SIZE + slotMapSize + (recordSize * slotNum)
        - each record will have size attrCount * ATTR_SIZE
        - slotMap will be of size slotCount
      */
      unsigned char *slotPointer = bufferPtr+ HEADER_SIZE + slotCount + (attrCount * ATTR_SIZE * slotNum);   //buffer2 holds the blockNum  attrsize=16

      // load the record into the rec data structure
      memcpy(rec, slotPointer, attrCount * ATTR_SIZE);

      return SUCCESS;
}
int RecBuffer::getSlotMap(unsigned char *slotMap)
{
      unsigned char *bufferPtr;

      int ret=loadBlockAndGetBufferPtr(&bufferPtr);  //gives integer value and gets the starting address of the buffer containing the block.

      if(ret !=SUCCESS)
      {
            return ret;
      }
      struct HeadInfo head;
      this -> getHeader(&head);
      int slotCount=head.numSlots;

      unsigned char *slotMapInBuffer=bufferPtr+HEADER_SIZE;
      memcpy(slotMap, slotMapInBuffer, slotCount);  // i didnt put this line so what happ was it was printing the entire slotmap with all 0's as well 
      //Without copying the slotMap data, the slotMap array in BlockAccess::linearSearch() remains uninitialized so it will read all the data including 0's

      return SUCCESS;
}

//stage-6 till now no implementation of set record pls delete it from before stages

int RecBuffer::setRecord(union Attribute* rec, int slotNum)
{
      unsigned char *bufferPtr;

      int ret= loadBlockAndGetBufferPtr(&bufferPtr);

      if (ret!=SUCCESS)
      {
            return ret;
      }

      struct HeadInfo head;

      // get the header using this.getHeader() function
      this -> getHeader(&head);
      

      int attrCount=head.numAttrs;
      int slotCount=head.numSlots;

      if (slotNum<0 || slotNum>=slotCount)
      {
            return E_OUTOFBOUND;
      }

    
    /* offset bufferPtr should point to the beginning of the record at required
       slot. 
       record at slot x will be at bufferPtr + HEADER_SIZE + (x*recordSize)
       a record will be of size ATTR_SIZE * numAttrs
       copy the record from `rec` to buffer using memcpy
    */

      unsigned char *slotPointer = bufferPtr+ HEADER_SIZE + slotCount+ (slotNum*ATTR_SIZE*attrCount);   

      // copies a record into a buffer block.
      memcpy(slotPointer, rec, attrCount * ATTR_SIZE);

      StaticBuffer::setDirtyBit(this->blockNum);    

    /* (the above function call should not fail since the block is already
       in buffer and the blockNum is valid. If the call does fail, there
       exists some other issue in the code) */

      return SUCCESS;
}

//stage-6 has added  the following
// if present (!=E_BLOCKNOTINBUFFER),
        // set the timestamp of the corresponding buffer to 0 and increment the
        // timestamps of all other occupied buffers in BufferMetaInfo.

int BlockBuffer::loadBlockAndGetBufferPtr( unsigned char ** bufferPtr)
{
      int bufferNum= StaticBuffer::getBufferNum(this->blockNum); //.h has already defined the blocknum so u just need to put arrow

      if (bufferNum != E_BLOCKNOTINBUFFER)
      {
            StaticBuffer::metainfo[bufferNum].timeStamp=0; // no need of struct BufferMetaInfo StaticBuffer::metainfo[BUFFER_CAPACITY]; but why??

            for(int i=0; i<BUFFER_CAPACITY;i++)
            {
                  if (StaticBuffer::metainfo[i].free==true && i!=bufferNum)
                  {
                        StaticBuffer::metainfo[i].timeStamp+=1;
                  } 
            }
      }
      else
      {
            bufferNum=StaticBuffer::getFreeBuffer(this ->blockNum);

            if (bufferNum == E_OUTOFBOUND)  //this checks if the block num provided by get free buffer is out of bound or not 
            {
                  return E_OUTOFBOUND;
            }
            Disk::readBlock(StaticBuffer::blocks[bufferNum], this->blockNum);
      }

      *bufferPtr=StaticBuffer::blocks[bufferNum]; //its not blocks[blockNum] its block[bufferNum] we defined a pointer cuz its easy to put that as a paraemeter when other functions call this function its better than actually doing blocks[blockNum].
      return SUCCESS;
}