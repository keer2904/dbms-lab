#include "RelCacheTable.h"
#include <cstring>

RelCacheEntry* RelCacheTable::relCache[MAX_OPEN]; // relCache is a static variable which has the type relcacheentry in the relcachetable class

int RelCacheTable::getRelCatEntry(int relId, RelCatEntry* relCatBuf)
{
    if (relId<0 ||relId>=MAX_OPEN) //relId is from 0 to 11
    {
        return E_OUTOFBOUND;
    }
    if (relCache[relId]==nullptr)
    {
        return E_RELNOTOPEN;
    }

    *relCatBuf=relCache[relId]->relCatEntry; //make a buffer pointer to point to the cache entry
    return SUCCESS;
}

void RelCacheTable::recordToRelCatEntry(union Attribute record[RELCAT_NO_ATTRS], RelCatEntry* relCatEntry)
{
    strcpy(relCatEntry->relName, record[RELCAT_REL_NAME_INDEX].sVal);
    relCatEntry->numAttrs=(int)record[RELCAT_NO_ATTRIBUTES_INDEX].nVal;
    relCatEntry->numRecs=(int)record[RELCAT_NO_RECORDS_INDEX].nVal;
    relCatEntry->firstBlk=(int)record[RELCAT_FIRST_BLOCK_INDEX].nVal;
    relCatEntry->lastBlk=(int)record[RELCAT_LAST_BLOCK_INDEX].nVal;
    relCatEntry->numSlotsPerBlk=(int)record[RELCAT_NO_SLOTS_PER_BLOCK_INDEX].nVal;
}

int RelCacheTable::getSearchIndex(int relId, RecId* searchIndex)
{
    if (relId <0|| relId>=MAX_OPEN)
    {
        return E_OUTOFBOUND;
    }
    if (relCache[relId]==nullptr)
    {
        return E_RELNOTOPEN;
    }
    *searchIndex=relCache[relId]->searchIndex; 
    return SUCCESS;
}

int RelCacheTable::setSearchIndex(int relId, RecId* searchIndex)
{
    if (relId <0|| relId>=MAX_OPEN)
    {
        return E_OUTOFBOUND;
    }
    if (relCache[relId]==nullptr)
    {
        return E_RELNOTOPEN;
    }
     relCache[relId]->searchIndex=*searchIndex;
     return SUCCESS;

}

int RelCacheTable::resetSearchIndex(int relId) 
{
    RecId searchIndex;
    searchIndex.block=-1;
    searchIndex.slot=-1;
    RelCacheTable::setSearchIndex(relId,&searchIndex);
    return SUCCESS;
}
