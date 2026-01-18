#include "BlockBuffer.h"
#include <cstdlib>
#include <cstring>

// the declarations for these functions can be found in "BlockBuffer.h"

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


int RecBuffer::setRecord(union Attribute *rec, int slotNum) 
{
      struct HeadInfo head;
      unsigned char buffer2[BLOCK_SIZE];

      // get the header using this.getHeader() function
      this -> getHeader(&head);

      int attrCount = head.numAttrs;
      int slotCount = head.numSlots;

      // read the block at this.blockNum into a buffer

      Disk::readBlock(buffer2, this->blockNum);

      unsigned char *slotPointer = buffer2+ HEADER_SIZE + slotCount + (attrCount * ATTR_SIZE * slotNum);   //buffer2 holds the blockNum recordsize=attrCount * ATTR_SIZE

      memcpy(slotPointer, rec, attrCount * ATTR_SIZE);   //copies the new record data from rec into the buffer at the location pointed to by slotPointer

      Disk::writeBlock(buffer2, this->blockNum);

      return SUCCESS;
}

int BlockBuffer::loadBlockAndGetBufferPtr( unsigned char ** bufferPtr)
{
      int bufferNum= StaticBuffer::getBufferNum(this->blockNum);

      if (bufferNum == E_BLOCKNOTINBUFFER)
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