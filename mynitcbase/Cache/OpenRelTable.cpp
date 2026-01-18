#include "OpenRelTable.h"
#include <cstring>
#include <cstdlib>
#include <stdlib.h>

OpenRelTable::OpenRelTable()
{
    for(int i=0; i<MAX_OPEN; ++i)
    {
        RelCacheTable::relCache[i]=nullptr;
        AttrCacheTable::attrCache[i]=nullptr;
    }

    /************** 1) RELATION CACHE TABLE ***************/

    RecBuffer relCatBlock(RELCAT_BLOCK);  //disk to buffer allocation
    Attribute relCatRecord[RELCAT_NO_ATTRS]; //a seperate data structure to get the record from buffer
    relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_RELCAT);  //using get record function to transafer from buffer to relcatrecord

    struct RelCacheEntry relCacheEntry;  //no need to reinitialize in the bottom one cuz it gets overwritten
    RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);  //relcacheentry has relcatentry so we need to do relCacheEntry.relCatEntry and since it needs a pointer arg it shud be &
    relCacheEntry.recId.block=RELCAT_BLOCK;  //recId structure has block and slot number 
    relCacheEntry.recId.slot=RELCAT_SLOTNUM_FOR_RELCAT;

    RelCacheTable::relCache[RELCAT_RELID]=(struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));  //allocate space for relcachetable
    *(RelCacheTable::relCache[RELCAT_RELID])=relCacheEntry;  //first entry=index=0 put the cache record into it

    //do samething for attribute
    //no need to reinitialize relcache entry

    //relation catalog first index has relation catalog table  data
    // relation catalog second index has attribute catalog table data 
    //so, you dont need to reinitialize buffer and get data from attribute catalog to put data of attribute catalog into the cache
    //please note what all are changing from the above cuz there are not much changes. like most are just relcat only

    relCatBlock.getRecord(relCatRecord,RELCAT_SLOTNUM_FOR_ATTRCAT);
    RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
    relCacheEntry.recId.block=RELCAT_BLOCK;
    relCacheEntry.recId.slot=RELCAT_SLOTNUM_FOR_ATTRCAT;

    //block 4 slot 1 for attribute catalog data in relation catalog 
    //block 4 slot 0 for relation catalog data in relation catalog

    RelCacheTable::relCache[ATTRCAT_RELID]=(struct RelCacheEntry*)malloc(sizeof(RelCacheEntry)); //why do u need struct??
    *(RelCacheTable::relCache[ATTRCAT_RELID])=relCacheEntry;

    /******************* 2) ATTRIBUTE CACHE TABLE *********************/

    RecBuffer attrCatBlock(ATTRCAT_BLOCK); //disk to buffer
    Attribute attrCatRecord[ATTRCAT_NO_ATTRS]; //seperate data structure

    AttrCacheEntry* attrhead=nullptr;
    AttrCacheEntry* curr=nullptr;

    for (int i=0; i< 6; i++)
    {
        //creating a node for each entry in the cache cuz each entry has a linkedlist.
        AttrCacheEntry* entry= (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry)); //why no struct??
        attrCatBlock.getRecord(&attrCatRecord[0],i);  //no need of ATTRCAT_SLOTNUM_FOR_ATTRCAT
//we can give &attrCatRecord[0] or just attrCatRecord cuz since its an array it wud auatomatically point to first index but if u give &attrCatRecord its wrong it treats that entire array as one unit which is wrong
       
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &entry->attrCatEntry);
        entry->recId.block=ATTRCAT_BLOCK; //its arrow not .
        entry->recId.slot=i; //its slot not slotnum
        entry->next=nullptr;

        if (attrhead == nullptr)
        {
            attrhead=entry;
            curr=attrhead; //curr points to the first node
        }
        else
        {
            curr->next=entry; 
            curr=curr->next;
        }  
    }
    AttrCacheTable::attrCache[RELCAT_RELID]=attrhead; //add the header pointer to attr cache table 


    attrhead=nullptr;
    curr=nullptr;

    for (int i=6; i<12; i++)
    {
        AttrCacheEntry* entry= (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
        attrCatBlock.getRecord(&attrCatRecord[0],i);

        AttrCacheTable::recordToAttrCatEntry(attrCatRecord,&entry->attrCatEntry);
        entry->recId.block=ATTRCAT_BLOCK; //recid almost like primary key rec id is combo of slot and block
        entry->recId.slot=i;

        entry->next=nullptr;

        if (attrhead ==nullptr)
        {
            attrhead=entry;
            curr=attrhead;
        }
        else
        {
            curr->next=entry;
            curr=curr->next;
        }
    }
    AttrCacheTable::attrCache[ATTRCAT_RELID]=attrhead;



    //exercise 
    //student relation 

    relCatBlock.getRecord(relCatRecord,2);
    RelCacheTable::recordToRelCatEntry(relCatRecord,&relCacheEntry.relCatEntry);
    relCacheEntry.recId.block=RELCAT_BLOCK;
    relCacheEntry.recId.slot=2;

    RelCacheTable::relCache[ATTRCAT_RELID+1]=(struct RelCacheEntry* )(malloc(sizeof(RelCacheEntry)));
    *(RelCacheTable::relCache[ATTRCAT_RELID+1])=relCacheEntry; //ATTRCAT_RELID=1 the next entry shud be at 2 

    //student attribute
    
    attrhead=nullptr;
    curr=nullptr;

    int studentnumattr=relCacheEntry.relCatEntry.numAttrs;

    for(int i=12;i<12+studentnumattr;i++)
    {
        AttrCacheEntry* entry= (AttrCacheEntry*)(malloc(sizeof(AttrCacheEntry))); 
        attrCatBlock.getRecord(attrCatRecord,i); //buffer to cat record (seperate data structure)
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord,&entry->attrCatEntry) ;//cat record is going inside cat entry
        entry->recId.block=ATTRCAT_BLOCK; //assuming only 1 block has all the attribute values
        entry->recId.slot=i;

        entry->next=nullptr; //its entry->next=null not curr->next=null

        if(attrhead==nullptr)
        {
            attrhead=entry;
            curr=attrhead; //its not curr=entry 
        } 
        else
        {
            curr->next=entry;
            curr=curr->next;
        }
    }
    AttrCacheTable::attrCache[ATTRCAT_RELID+1]=attrhead;
}

OpenRelTable::~OpenRelTable()
{
    for (int i=0;i<MAX_OPEN;i++)
    {
        free(RelCacheTable::relCache[i]);
        AttrCacheEntry* x= AttrCacheTable::attrCache[i];
        for (x; x!=nullptr; )
        {
            AttrCacheEntry* nextentry=x->next;
            free(x);
            x=nextentry; //if next entru is null loop will stop else it will keep looping and freeing
        }
    }
}