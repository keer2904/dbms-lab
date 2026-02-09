#include "BlockAccess.h"
#include <cstring>
#include <stdio.h>
//stage-4

RecId BlockAccess::linearSearch(int relId, char attrName[ATTR_SIZE],union Attribute attrVal, int op) //why RecId
{
    RecId prevRecId; //no need of * here
    RelCacheTable::getSearchIndex(relId, &prevRecId); 

    int block,slot;
    if (prevRecId.block==-1 && prevRecId.slot ==-1)
    {
        RelCatEntry relCatBuf; //no need of * here
        RelCacheTable::getRelCatEntry(relId, &relCatBuf);
        block=relCatBuf.firstBlk;
        slot=0;       
    }

    else
    {
        block=prevRecId.block;
        slot=prevRecId.slot+1;
    }

    while (block!=-1)
    {
        RecBuffer recbuffer(block); //you shud not always use:: 
        HeadInfo head;
        recbuffer.getHeader(&head);
        
        Attribute rec[head.numAttrs];  //dont do just union Attribute* rec u need an array as input to getRecord
        
        recbuffer.getRecord(rec, slot);

        unsigned char slotMap[head.numSlots]; // getSlotMap function has the array as an input so, we have to define like that

        recbuffer.getSlotMap(slotMap);

        if (slot>=head.numSlots) //if no more slots in this block goto next block
        {
            block=head.rblock;
            slot=0;
            continue; // continue to the beginning of this while loop but why though whats the logic
        }

        if(slotMap[slot] == SLOT_UNOCCUPIED)
        {
            slot++;
            continue;
        }
        
        AttrCatEntry attrCatBuf;

        AttrCacheTable::getAttrCatEntry(relId, attrName,&attrCatBuf);

        Attribute attrOffset= rec[attrCatBuf.offset];
        int cmpVal = compareAttrs(attrOffset, attrVal, attrCatBuf.attrType); //why and what are we comparing??

        if (
            (op == NE && cmpVal != 0) ||    // if op is "not equal to"
            (op == LT && cmpVal < 0) ||     // if op is "less than"
            (op == LE && cmpVal <= 0) ||    // if op is "less than or equal to"
            (op == EQ && cmpVal == 0) ||    // if op is "equal to"
            (op == GT && cmpVal > 0) ||     // if op is "greater than"
            (op == GE && cmpVal >= 0)       // if op is "greater than or equal to"
        ) 
        {
            RecId searchIndex={block,slot};
            RelCacheTable::setSearchIndex(relId, &searchIndex);

            return searchIndex;
        }
        slot++;
    }
    return RecId{-1,-1}; // no record in the relation with Id relid satisfies the given condition
}


//stage -6

int BlockAccess::renameRelation(char oldName[ATTR_SIZE], char newName[ATTR_SIZE])
{
    RelCacheTable::resetSearchIndex(RELCAT_RELID);  

    Attribute newRelationName;
    strcpy(newRelationName.sVal,newName);

    RecId recNewId=BlockAccess::linearSearch(RELCAT_RELID,(char*)RELCAT_ATTR_RELNAME,newRelationName,EQ);

    if (recNewId.block!=-1 && recNewId.slot!=-1) //If relation with name newName already exists 
    {
        return E_RELEXIST;
    }

    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute oldRelationName;

    strcpy(oldRelationName.sVal,oldName);

    RecId recOldId=BlockAccess::linearSearch(RELCAT_RELID,(char*) RELCAT_ATTR_RELNAME,oldRelationName,EQ);

    if (recOldId.block==-1 && recOldId.slot==-1)  //If relation with name oldName does not exist
    {
        return E_RELNOTEXIST;
    }
    
    RecBuffer bufferRel(RELCAT_BLOCK);
    Attribute rec[RELCAT_NO_ATTRS];
    bufferRel.getRecord(rec, recOldId.slot);  

    strcpy(rec[RELCAT_REL_NAME_INDEX].sVal,newName); //it is the index where relname of rel catalog is present BUT WHY ARE YOU CHANGING NAME OF REL CAT?

    bufferRel.setRecord(rec, recOldId.slot);

    //CHANGING RELNAME IN ATTRIBUTE CATALOG

    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);  
    int numAttrs= rec[RELCAT_NO_ATTRIBUTES_INDEX].nVal ;//pls learn the structure of these INDEX where is it present anol

    for (int i=0; i<numAttrs;i++ )
    {
        RecId recOldId=BlockAccess::linearSearch(ATTRCAT_RELID,(char*) ATTRCAT_ATTR_RELNAME,oldRelationName,EQ);

        RecBuffer bufferAttr(recOldId.block); //dont put attrcat_block
        Attribute rec[ATTRCAT_NO_ATTRS];

        bufferAttr.getRecord(rec, recOldId.slot);  

        strcpy(rec[ATTRCAT_REL_NAME_INDEX].sVal,newName);

        bufferAttr.setRecord(rec, recOldId.slot);
    }

    return SUCCESS;
} 

//stage-6
int BlockAccess::renameAttribute(char relName[ATTR_SIZE], char oldName[ATTR_SIZE], char newName[ATTR_SIZE])
{
    RelCacheTable::resetSearchIndex(RELCAT_RELID);  
    Attribute relNameAttr;

    strcpy(relNameAttr.sVal,relName);

    RecId recid= BlockAccess::linearSearch(RELCAT_RELID,(char*)RELCAT_ATTR_RELNAME,relNameAttr,EQ); //checking in relation catalog 
    //instead of just writing "RelName" do RELCAT_ATTR_RELNAME but what does it mean
    //also why char* typecasting is required it gave me warning

    if (recid.block==-1 && recid.slot==-1)
    {
        return E_RELNOTEXIST;   
    }

    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    RecId attrRecId{-1, -1}; //initialising new rec id 
    Attribute rec[ATTRCAT_NO_ATTRS];

    //find the correct attr;
    while (true) 
    {
        RecId recOldId=BlockAccess::linearSearch(ATTRCAT_RELID,(char*) ATTRCAT_ATTR_RELNAME,relNameAttr,EQ); //checking in attribute catalog
        
        if (recOldId.block==-1 && recOldId.slot==-1)
        {
            break;
        }
        
        RecBuffer buff(recOldId.block);
        buff.getRecord(rec,recOldId.slot);

        if (strcmp(rec[ATTRCAT_ATTR_NAME_INDEX].sVal,oldName)==0)  //what is ATTRCAT_ATTR_NAME_INDEX what is present in each index of this rec Attribute
        {
            attrRecId=recOldId;
            strcpy(rec[ATTRCAT_ATTR_NAME_INDEX].sVal,newName);
            buff.setRecord(rec,attrRecId.slot);
            break;
        }

        if (strcmp(rec[ATTRCAT_ATTR_NAME_INDEX].sVal,newName)==0) 
        {
            return E_ATTREXIST;
        }
    }
    if (attrRecId.block==-1 && attrRecId.slot==-1)
    {
        return E_ATTRNOTEXIST;
    }
    return SUCCESS;
}


int BlockAccess::insert(int relId, Attribute *record) 

{
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(relId, &relCatEntry);
    
    int blockNum = relCatEntry.firstBlk; //WHY   //first record block of the relation (from the rel-cat entry)
    
    RecId rec_id = {-1, -1};  // rec_id will be used to store where the new record will be inserted

    int numOfSlots = relCatEntry.numSlotsPerBlk;
    int numOfAttributes = relCatEntry.numAttrs; 

    int prevBlockNum = -1;  //block number of the last element in the linked list = -1 WHY IS IT -1??

    //Traversing the linked list of existing record blocks of the relation until a free slot is found OR until the end of the list
    while (blockNum != -1) 
    {
        // create a RecBuffer object for blockNum (using appropriate constructor!)
        RecBuffer currBlock(blockNum);

        // get header of block(blockNum) using RecBuffer::getHeader() function
        struct HeadInfo head;
        currBlock.getHeader(&head); // see the difference here and getSlotMap in BlockBuffer.cpp

        // get slot map of block(blockNum) using RecBuffer::getSlotMap() function
        unsigned char slotMap[numOfSlots];
        currBlock.getSlotMap(slotMap);

        int freeSlot=-1;
        for (int i=0;i<numOfSlots;i++)
        {
            if(slotMap[i]==SLOT_UNOCCUPIED)
            {
                freeSlot=i;
                break;
            }
        }

        if (freeSlot==-1)
        {
            prevBlockNum = blockNum;
            blockNum = head.rblock ;
        }
        else
        {
            rec_id.block=blockNum;
            rec_id.slot=freeSlot;
            break;
        }
    }

    if (rec_id.slot ==-1 && rec_id.block==-1)
    {
        if (relId==RELCAT_RELID)    // if relation is RELCAT, do not allocate any more blocks
        {
            return E_MAXRELATIONS;
        }
        else
        {
            // get a new record block (using the appropriate RecBuffer constructor!)  WHICH CONSTRUCTOR TO USE AND WHY

            RecBuffer newBlock;
            int ret=newBlock.getBlockNum();
            if (ret == E_DISKFULL) 
            {
                return E_DISKFULL;
            }

            rec_id.block=ret;
            rec_id.slot=0; //its 0 not -1

            struct HeadInfo NewHead;
            newBlock.getHeader(&NewHead);  //whya are u doing get header when u are actually setting the header

            NewHead.blockType=REC; //its a pointer here not arrow!!
            //NewHead.lblock=-1;   lblock = -1 if linked list of existing record blocks was empty else lblock = prevBlockNum 
            NewHead.lblock=prevBlockNum;
            NewHead.pblock=-1;
            NewHead.rblock=-1;
            NewHead.numEntries=0;
            NewHead.numSlots=numOfSlots;
            NewHead.numAttrs=numOfAttributes;

            newBlock.setHeader(&NewHead);

            unsigned char newSlotMap[numOfSlots];
            for (int i=0;i<numOfSlots;i++)
            {
                newSlotMap[i]=SLOT_UNOCCUPIED;   
            }
            newBlock.setSlotMap(newSlotMap);

            if (prevBlockNum != -1)
            {
                RecBuffer prevBlock(prevBlockNum);

                struct HeadInfo prevHead;
                prevBlock.getHeader(&prevHead);

                prevHead.rblock=rec_id.block;

                prevBlock.setHeader(&prevHead);
            }
            else
            {
                relCatEntry.firstBlk=rec_id.block; //WHY firstblk specifically what abt other blocks??
                RelCacheTable::setRelCatEntry(relId, &relCatEntry);  // update first block field in the relCatentry to the new block (using RelCacheTable::setRelCatEntry() function)
            }

            relCatEntry.lastBlk=rec_id.block;
            RelCacheTable::setRelCatEntry(relId, &relCatEntry);  // update last block field in the relation catalog entry to the new block (using RelCacheTable::setRelCatEntry() function)   
        }
    }
    
    RecBuffer insertblock(rec_id.block);

    insertblock.setRecord(record, rec_id.slot);

    //update the slot map of the block by marking entry of the slot to which record was inserted as occupied)
    unsigned char insertSlotMap[numOfSlots];
    insertblock.getSlotMap(insertSlotMap);
    insertSlotMap[rec_id.slot]=SLOT_OCCUPIED;
    insertblock.setSlotMap(insertSlotMap);

    struct HeadInfo header;
    
    insertblock.getHeader(&header);
    header.numEntries++;
    insertblock.setHeader(&header);
    relCatEntry.numRecs++;
    RelCacheTable::setRelCatEntry(relId,&relCatEntry);
    return SUCCESS;
}
