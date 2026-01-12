#include "BlockBuffer.h"
#include <cstdlib>
#include <cstring>

// the declarations for these functions can be found in "BlockBuffer.h"

BlockBuffer::BlockBuffer(int blockNum) {
  // initialise this.blockNum with the argument
  this -> blockNum = blockNum;
}

// calls the parent class constructor
RecBuffer::RecBuffer(int blockNum) : BlockBuffer::BlockBuffer(blockNum) {}

// load the block header into the argument pointer
int BlockBuffer::getHeader(struct HeadInfo *head) {
  unsigned char buffer[BLOCK_SIZE];

  // read the block at this.blockNum into the buffer
    Disk::readBlock(buffer, this->blockNum);
  // populate the numEntries, numAttrs and numSlots fields in *head
  memcpy(&head->numSlots, buffer + 24, 4);
  memcpy(&head->numEntries, buffer +16, 4);
  memcpy(&head->numAttrs, buffer +20 /* fill this */, 4);
  memcpy(&head->rblock, buffer + 12 /* fill this */, 4);
  memcpy(&head->lblock, buffer + 8 /* fill this */, 4);

  return SUCCESS;
}

// load the record at slotNum into the argument pointer
int RecBuffer::getRecord(union Attribute *rec, int slotNum) 
{
  struct HeadInfo head;
  unsigned char buffer2[BLOCK_SIZE];

  // get the header using this.getHeader() function
  this -> getHeader(&head);

  int attrCount = head.numAttrs;
  int slotCount = head.numSlots;

  // read the block at this.blockNum into a buffer

  Disk::readBlock(buffer2, this->blockNum);

  /* record at slotNum will be at offset HEADER_SIZE + slotMapSize + (recordSize * slotNum)
     - each record will have size attrCount * ATTR_SIZE
     - slotMap will be of size slotCount
  */
  unsigned char *slotPointer = buffer2+ HEADER_SIZE + slotCount + (attrCount * ATTR_SIZE * slotNum);   //buffer2 holds the blockNum  attrsize=16

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