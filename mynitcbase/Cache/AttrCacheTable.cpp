#include "AttrCacheTable.h"
#include <cstring>

AttrCacheEntry* AttrCacheTable::attrCache[MAX_OPEN];


int AttrCacheTable::getAttrCatEntry(int relId, int attrOffset, AttrCatEntry* attrCatBuf)
{
    if (relId<0 ||relId>=MAX_OPEN)
    {
        return E_OUTOFBOUND;
    }

    if (attrCache[relId]==nullptr)
    {
        return E_RELNOTOPEN;
    }

    for (AttrCacheEntry* x=attrCache[relId]; x!=nullptr; x=x->next)
    {
        if (x->attrCatEntry.offset==attrOffset)
        {
            *attrCatBuf = x->attrCatEntry;
            return SUCCESS;

        }
    }
    return E_ATTRNOTEXIST;
}

int AttrCacheTable::getAttrCatEntry(int relId, char attrName[ATTR_SIZE], AttrCatEntry* attrCatBuf)
{
    if (relId<0 || relId>=MAX_OPEN) //its >= not >
    {
        return E_OUTOFBOUND;
    }

    if (attrCache[relId] == nullptr) 
    {
        return E_RELNOTOPEN;
    }

    for (AttrCacheEntry* entry=attrCache[relId]; entry!=nullptr;entry=entry->next)
    {
        if (strcmp(entry->attrCatEntry.attrName,attrName)==0)
        {
            strcpy(attrCatBuf->relName, entry->attrCatEntry.relName);
            strcpy(attrCatBuf->attrName, entry->attrCatEntry.attrName);
            attrCatBuf->attrType = entry->attrCatEntry.attrType;
            attrCatBuf->offset = entry->attrCatEntry.offset;
            attrCatBuf->primaryFlag = entry->attrCatEntry.primaryFlag;
            attrCatBuf->rootBlock = entry->attrCatEntry.rootBlock;
            return SUCCESS;
        }
    }
    return E_ATTRNOTEXIST;
}

void AttrCacheTable::recordToAttrCatEntry(union Attribute record[ATTRCAT_NO_ATTRS], AttrCatEntry* attrCatEntry)
{
    strcpy(attrCatEntry->relName, record[ATTRCAT_REL_NAME_INDEX].sVal);
    strcpy(attrCatEntry->attrName, record[ATTRCAT_ATTR_NAME_INDEX].sVal);
    attrCatEntry->attrType=(int)record[ATTRCAT_ATTR_TYPE_INDEX].nVal;
    attrCatEntry->primaryFlag=(bool)record[ATTRCAT_PRIMARY_FLAG_INDEX].nVal;
    attrCatEntry->rootBlock=(int)record[ATTRCAT_ROOT_BLOCK_INDEX].nVal;
    attrCatEntry->offset=(int)record[ATTRCAT_OFFSET_INDEX].nVal;
}

//stage-10
int AttrCacheTable::getSearchIndex(int relId, char attrName[ATTR_SIZE], IndexId *searchIndex) 
{
    if(relId<0 || relId>=MAX_OPEN)
    {
        return E_OUTOFBOUND;
    }

    if(attrCache[relId] ==nullptr)  //entry corresponding to the relId in the Attribute Cache Table is free
    {
        return E_RELNOTOPEN;
    }

    for(AttrCacheEntry* entry= attrCache[relId]; entry !=nullptr;entry=entry->next)
    {
        if (strcmp(entry->attrCatEntry.attrName,attrName)==0)   // attrName of the AttrCatEntry is equal to the input attrName
        {
            searchIndex->block=entry->searchIndex.block;    //copy the searchIndex field of the corresponding Attribute Cache entry in the Attribute Cache Table to input searchIndex variable
            searchIndex->index=entry->searchIndex.index;
            return SUCCESS;
        }
    }
    return E_ATTRNOTEXIST;
}

int AttrCacheTable::getSearchIndex(int relId, int attrOffset, IndexId *searchIndex)
{
    if(relId<0 || relId>=MAX_OPEN)
    {
        return E_OUTOFBOUND;
    }

    if(attrCache[relId] ==nullptr)  //entry corresponding to the relId in the Attribute Cache Table is free
    {
        return E_RELNOTOPEN;
    }

    for(AttrCacheEntry* entry= attrCache[relId]; entry !=nullptr;entry=entry->next)
    {
        if (entry->attrCatEntry.offset==attrOffset)   // attrOffset of the AttrCatEntry is equal to the input offset
        {
            searchIndex->block=entry->searchIndex.block;    //copy the searchIndex field of the corresponding Attribute Cache entry in the Attribute Cache Table to input searchIndex variable
            searchIndex->index=entry->searchIndex.index;
            return SUCCESS;
        }
    }
    return E_ATTRNOTEXIST;
}


int AttrCacheTable::setSearchIndex(int relId, char attrName[ATTR_SIZE], IndexId *searchIndex) 
{
    if(relId<0 || relId>=MAX_OPEN)
    {
        return E_OUTOFBOUND;
    }

    if(attrCache[relId] ==nullptr)  //entry corresponding to the relId in the Attribute Cache Table is free
    {
        return E_RELNOTOPEN;
    }

    for(AttrCacheEntry* entry= attrCache[relId]; entry !=nullptr;entry=entry->next)
    {
        if (strcmp(entry->attrCatEntry.attrName,attrName)==0)   // attrName of the AttrCatEntry is equal to the input attrName
        {
            entry->searchIndex.block= searchIndex->block; 
            entry->searchIndex.index=searchIndex->index;
            return SUCCESS;
        }
    }
    return E_ATTRNOTEXIST;
}

int AttrCacheTable::setSearchIndex(int relId,int attrOffset, IndexId *searchIndex) 
{
    if(relId<0 || relId>=MAX_OPEN)
    {
        return E_OUTOFBOUND;
    }

    if(attrCache[relId] ==nullptr)  //entry corresponding to the relId in the Attribute Cache Table is free
    {
        return E_RELNOTOPEN;
    }

    for(AttrCacheEntry* entry= attrCache[relId]; entry !=nullptr;entry=entry->next)
    {
        if (entry->attrCatEntry.offset==attrOffset)   // attrOffset of the AttrCatEntry is equal to the input offset
        {
            entry->searchIndex.block= searchIndex->block;   //wdym by entry->searchIndex.block
            entry->searchIndex.index=searchIndex->index;
            return SUCCESS;
        }
    }
    return E_ATTRNOTEXIST;
}

int AttrCacheTable::resetSearchIndex(int relId, char attrName[ATTR_SIZE]) 
{
    IndexId indexId= {-1,-1};
    int ret= AttrCacheTable::setSearchIndex(relId,attrName,&indexId);
    return  ret;
}

int AttrCacheTable::resetSearchIndex(int relId,int attrOffset) 
{
    IndexId indexId= {-1,-1};
    int ret= AttrCacheTable::setSearchIndex(relId,attrOffset,&indexId);
    return  ret;
}

//stage-11
int AttrCacheTable::setAttrCatEntry(int relId, char attrName[ATTR_SIZE], AttrCatEntry *attrCatBuf) 
{
    if(relId<0 || relId>=MAX_OPEN) 
    {
        return E_OUTOFBOUND;
    }

    if(attrCache[relId]==nullptr) 
    {
        return E_RELNOTOPEN;
    }

    for(AttrCacheEntry* entry=attrCache[relId];entry!=nullptr;entry=entry->next)
    {
        if (strcmp(entry->attrCatEntry.attrName,attrName)==0)
        {
            entry->attrCatEntry= *attrCatBuf;   // copy the attrCatBuf to the corresponding Attribute Catalog entry in the Attribute Cache Table.
            entry->dirty=true;                  // set the dirty flag of the corresponding Attribute Cache entry in the Attribute Cache Table.
            return SUCCESS;

        }
    }
    return E_ATTRNOTEXIST;
}

int AttrCacheTable::setAttrCatEntry(int relId, int attrOffset, AttrCatEntry *attrCatBuf) 
{
    if(relId<0 || relId>=MAX_OPEN) 
    {
        return E_OUTOFBOUND;
    }

    if(attrCache[relId]==nullptr) 
    {
        return E_RELNOTOPEN;
    }

    for(AttrCacheEntry* entry=attrCache[relId];entry!=nullptr;entry=entry->next)
    {
        if (entry->attrCatEntry.offset==attrOffset)  
        {
            entry->attrCatEntry= *attrCatBuf;   // copy the attrCatBuf to the corresponding Attribute Catalog entry in the Attribute Cache Table.
            entry->dirty=true;                  // set the dirty flag of the corresponding Attribute Cache entry in the Attribute Cache Table.
            return SUCCESS;
        }
    }
    return E_ATTRNOTEXIST;
}

void AttrCacheTable::attrCatEntryToRecord(AttrCatEntry* attrCatEntry,union Attribute record[ATTRCAT_NO_ATTRS])
{
    strcpy(record[ATTRCAT_REL_NAME_INDEX].sVal,attrCatEntry->relName);
    strcpy(record[ATTRCAT_ATTR_NAME_INDEX].sVal,attrCatEntry->attrName);
    record[ATTRCAT_ATTR_TYPE_INDEX].nVal=attrCatEntry->attrType;
    record[ATTRCAT_PRIMARY_FLAG_INDEX].nVal=attrCatEntry->primaryFlag;
    record[ATTRCAT_ROOT_BLOCK_INDEX].nVal=attrCatEntry->rootBlock;
    record[ATTRCAT_OFFSET_INDEX].nVal=attrCatEntry->offset;
}