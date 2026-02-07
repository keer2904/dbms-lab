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
