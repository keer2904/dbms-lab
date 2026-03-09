#include "OpenRelTable.h"
#include <cstring>
#include <cstdlib>
#include <stdlib.h>

OpenRelTableMetaInfo OpenRelTable::tableMetaInfo[MAX_OPEN];

AttrCacheEntry* createList(int length)
{
    AttrCacheEntry* head=(AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
    AttrCacheEntry* curr=head;

    for (int i=1;i<length;i++) 
    {
        curr->next=(AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
        curr=curr->next;
    }
    curr->next=nullptr;
    return head;
}

OpenRelTable::OpenRelTable()
{
    for(int i=0; i<MAX_OPEN; ++i) // why is it ++i initialise all values in relCache and attrCache to be nullptr and all entries in tableMetaInfo to be free
    {
        RelCacheTable::relCache[i]=nullptr;
        AttrCacheTable::attrCache[i]=nullptr;
        OpenRelTable::tableMetaInfo[i].free=true;
    }

    /************** 1) RELATION CACHE TABLE ***************/
    //load the relation and attribute catalog into the relation cache

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
    //load the relation and attribute catalog into the attribute cache

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


    //STAGE-5
    //tableMetaInfo has 2 fields
    tableMetaInfo[RELCAT_RELID].free=false;
    tableMetaInfo[ATTRCAT_RELID].free=false;

    strcpy(tableMetaInfo[RELCAT_RELID].relName,RELCAT_RELNAME);
    strcpy(tableMetaInfo[ATTRCAT_RELID].relName,ATTRCAT_RELNAME);

}

//STAGE-8
OpenRelTable::~OpenRelTable()
{
    //stage-5
    for(int i=2; i<MAX_OPEN;i++)  // close all open relations (from rel-id = 2 onwards)
    {
        if(tableMetaInfo[i].free==false)
        {
            OpenRelTable::closeRel(i);
        }
    }
    /**** Closing the catalog relations in the relation cache ****/

    //releasing the relation cache entry of the attribute catalog

    if (RelCacheTable::relCache[ATTRCAT_RELID]->dirty) //RelCatEntry of the ATTRCAT_RELID-th RelCacheEntry has been modified
    {
        RelCatEntry relCatEntry= RelCacheTable::relCache[ATTRCAT_RELID]->relCatEntry;   //Get the Relation Catalog entry from RelCacheTable::relCache
        Attribute relCatRecord[RELCAT_NO_ATTRS];   
        RelCacheTable::relCatEntryToRecord(&relCatEntry,relCatRecord); 
        RecId recid=RelCacheTable::relCache[ATTRCAT_RELID]->recId;
        RecBuffer relCatBlock(recid.block);
        relCatBlock.setRecord(relCatRecord,recid.slot);
    }
    free(RelCacheTable::relCache[ATTRCAT_RELID]);   // free the memory dynamically allocated to this RelCacheEntry
    
    //releasing the relation cache entry of the relation catalog
    if (RelCacheTable::relCache[RELCAT_RELID]->dirty)
    {

        RelCatEntry relCatEntry= RelCacheTable::relCache[RELCAT_RELID]->relCatEntry;   //Get the Relation Catalog entry from RelCacheTable::relCache
        Attribute relCatRecord[RELCAT_NO_ATTRS];   
        RelCacheTable::relCatEntryToRecord(&relCatEntry,relCatRecord); 
        RecId recid=RelCacheTable::relCache[RELCAT_RELID]->recId;
        RecBuffer relCatBlock(recid.block);
        relCatBlock.setRecord(relCatRecord,recid.slot);
    }
    free(RelCacheTable::relCache[RELCAT_RELID]);  // free the memory dynamically allocated for this RelCacheEntry

    //stage-5
    // free the memory allocated for the attribute cache entries of the relation catalog and the attribute catalog
    
    AttrCacheEntry *temp, *next;
    for (int i=0;i<2;i++)  
    {
        AttrCacheEntry* x= AttrCacheTable::attrCache[i];
        for (x; x!=nullptr; ) 
        {
            AttrCacheEntry* nextentry=x->next;
            free(x);
            x=nextentry; 
        }
    }
}
int OpenRelTable::getRelId(char relName[ATTR_SIZE])  //Only gives relid if slot is occupied and if it matches the relname 
{
    for(int i=0; i<MAX_OPEN; i++) //MAX_OPEN is the size of tableMetainfo array
    {
        if ((tableMetaInfo[i].free == false) && strcmp(relName, tableMetaInfo[i].relName) == 0) //is tableMetaInfo[i].free==false really needed  YES IT IS NEEDED DONT GO PURE BY THE GUIDANCE GIVEN IN DOCUMENTATION
        {
            return i;
        }
    }
    return E_RELNOTOPEN;
}

int OpenRelTable::getFreeOpenRelTableEntry()
{
    for(int i=2; i<MAX_OPEN; i++) //MAX_OPEN is the size of tableMetainfo array
    {
        if (tableMetaInfo[i].free)
        {
            return i;
        }
    }
    return E_CACHEFULL;
}

int OpenRelTable::openRel(char relName[ATTR_SIZE])
{
    int ret=OpenRelTable::getRelId(relName); //just to check if relation is already opened 
    if(ret>=0 && ret<MAX_OPEN)
    {
        return ret; //could be E_RELOPEN right?
    }

    int freeSlot=OpenRelTable::getFreeOpenRelTableEntry();
    if (freeSlot==E_CACHEFULL)
    {
        return E_CACHEFULL;
    }

    int relId=freeSlot;

    /****** Setting up Relation Cache entry for the relation ******/

    Attribute attr;
    strcpy(attr.sVal,relName); //copying input relname into Attribute structure variable


    RelCacheTable::resetSearchIndex(RELCAT_RELID); //why specifically this relid we are resetting?? reset the searchIndex of the relation RELCAT_RELID before calling linearSearch().

    RecId relcatRecId=BlockAccess::linearSearch(RELCAT_RELID, (char*)RELCAT_ATTR_RELNAME, attr, EQ); //linear search expects an array which is why u need typecastin
   
    if (relcatRecId.block==-1 && relcatRecId.slot==-1)
    {
        return E_RELNOTEXIST;
    }

    RecBuffer relCatBuffer(relcatRecId.block);

    Attribute relCatRecord[RELCAT_NO_ATTRS];

    relCatBuffer.getRecord(relCatRecord, relcatRecId.slot);

    RelCatEntry relcatentry;

    RelCacheTable::recordToRelCatEntry(relCatRecord, &relcatentry);

    RelCacheTable::relCache[freeSlot]=(RelCacheEntry*) malloc(sizeof(RelCacheEntry));

    RelCacheTable::relCache[freeSlot]->recId=relcatRecId;

    RelCacheTable::relCache[freeSlot]->relCatEntry=relcatentry; //* isnt required

    /****** Setting up Attribute Cache entry for the relation ******/
    
    int numAttr=relcatentry.numAttrs;
    AttrCacheEntry* listhead=createList(numAttr);

    AttrCacheEntry* node= listhead; //why is this done why cant we directly use listhead

    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    for (int i=0;i<numAttr;i++)
    {
        RecId attrcatRecId=BlockAccess::linearSearch(ATTRCAT_RELID, (char*)ATTRCAT_ATTR_RELNAME, attr, EQ); //checking whther the relation name in the attr catalog is same as the input relation name

        if (attrcatRecId.block==-1 && attrcatRecId.slot==-1)
        {
            break; //dont return bcoz its for loop
        }

        RecBuffer attrCatBuffer(attrcatRecId.block);
        
        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];

        attrCatBuffer.getRecord(attrCatRecord,attrcatRecId.slot); //buffer to record 

        AttrCatEntry attrcatentry;

        AttrCacheTable::recordToAttrCatEntry(attrCatRecord,&attrcatentry); //attrcatrecord also requires star but when u define it automatically points to first element of array like its already a pointer

        node->recId=attrcatRecId;
        node->attrCatEntry=attrcatentry;
        node=node->next;
    }

    AttrCacheTable::attrCache[freeSlot]=listhead; //till now only entry pointed to listhead now entry shud be added into freeslot

    /****** Setting up metadata in the Open Relation Table for the relation******/
    OpenRelTable::tableMetaInfo[relId].free=false; //this is how u defined the array not by putting space u shud put :: instead
    
    strcpy(tableMetaInfo[relId].relName,relName); //shud we do memcpy will it automatically get saved to disk?

    return relId;
}


//stage-5, stage-7 
int OpenRelTable::closeRel(int relId)
{
    if(relId==RELCAT_RELID || relId==ATTRCAT_RELID)
    {
        return E_NOTPERMITTED;
    }
    if(relId<0 || relId>=MAX_OPEN)
    {
        return E_OUTOFBOUND;
    }
    if(OpenRelTable::tableMetaInfo[relId].free)
    {
        return E_RELNOTOPEN;
    }

    /****** Releasing the Relation Cache entry of the relation ******/

    if (RelCacheTable::relCache[relId]->dirty==true)
    {
        RelCatEntry relCatEntry= RelCacheTable::relCache[relId]->relCatEntry;
        Attribute record[RELCAT_NO_ATTRS] ;//here u dont need to do getRecord bcoz u need to add smtg into record
        RelCacheTable::relCatEntryToRecord(&relCatEntry,record); //relCatEntry is not array so u need to put &

        RecId recId= RelCacheTable::relCache[relId]->recId;
        RecBuffer relCatBlock(recId.block);

        relCatBlock.setRecord(record, recId.slot); //before closing table record shud be in table itself 
    }
        
    //stage-5
    free(RelCacheTable::relCache[relId]); 

    /****** Releasing the Attribute Cache entry of the relation ******/
 
    for(AttrCacheEntry* entry=AttrCacheTable::attrCache[relId]; entry !=nullptr ; )
    {
        AttrCacheEntry* nextEntry=entry->next;
        free(entry);
        entry=nextEntry; //dont do entry->next;
    }

    OpenRelTable::tableMetaInfo[relId].free=true;  //Do you need to "free" the relName field?  No, the next openRel() call will simply overwrite the old relName with strcpy()
    RelCacheTable::relCache[relId]=nullptr;
    AttrCacheTable::attrCache[relId]=nullptr;
    
    return SUCCESS;

}