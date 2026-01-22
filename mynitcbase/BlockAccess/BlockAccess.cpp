#include "BlockAccess.h"

#include <cstring>

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
