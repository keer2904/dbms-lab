#include "Schema.h"

#include <cmath>
#include <cstring>

int Schema::openRel(char relName[ATTR_SIZE])
{
    int ret=OpenRelTable::openRel(relName);
    if (ret >=0)
    {
        return SUCCESS;
    }
    return ret;
}

int Schema::closeRel(char relName[ATTR_SIZE])
{
    if ((strcmp(relName,"ATTRCAT")==0) || (strcmp(relName,"RELCAT")==0))
    {
        return E_NOTPERMITTED;
    }

    int relId=OpenRelTable::getRelId(relName);

    if(relId == E_RELNOTOPEN) //E_RELNOTOPEN=-89
    {
        return E_RELNOTOPEN;
    }

    return OpenRelTable::closeRel(relId);
}

//cant do rename operation when table is OPEN

int Schema::renameRel(char oldRelName[ATTR_SIZE], char newRelName[ATTR_SIZE])
{
    if (strcmp(oldRelName,RELCAT_RELNAME)==0 || strcmp(oldRelName, ATTRCAT_RELNAME)==0 || strcmp(newRelName,RELCAT_RELNAME)==0 || strcmp(newRelName, ATTRCAT_RELNAME)==0)
    {
        return E_NOTPERMITTED;
    }

    int ret= OpenRelTable::getRelId(oldRelName);

    if (ret!=E_RELNOTOPEN)  
    {
        return E_RELOPEN;
    }

    int retVal=BlockAccess::renameRelation(oldRelName,newRelName);
    return retVal;
}

//cant do rename operation when table is OPEN

int Schema::renameAttr(char *relName, char *oldAttrName, char *newAttrName)
{
    if (strcmp(relName,"RELCAT_RELNAME")==0 || strcmp(relName, "ATTRCAT_RELNAME")==0)
    {
        return E_NOTPERMITTED;
    }

    int ret= OpenRelTable::getRelId(relName);

    if (ret!=E_RELNOTOPEN)
    {
        return E_RELOPEN;
    }

    int retVal=BlockAccess::renameAttribute(relName, oldAttrName, newAttrName);
    return retVal;
}

//stage-8
int Schema::createRel(char relName[],int nAttrs, char attrs[][ATTR_SIZE], int attrtype[])
{
    Attribute relNameAsAttribute;
    strcpy(relNameAsAttribute.sVal, relName);
    RecId targetRelId;
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    targetRelId=BlockAccess::linearSearch(RELCAT_RELID,(char *)"RelName", relNameAsAttribute,EQ);
    
    if (targetRelId.block!=-1 && targetRelId.slot!=-1)
    {
        return E_RELEXIST;
    }

    for (int i=0; i<nAttrs-1;i++)
    {
        for (int j=i+1;j<nAttrs;j++)
        {
            if (strcmp(attrs[i],attrs[j])==0)
            {
                return E_DUPLICATEATTR;
            }
        }
    }

    Attribute relCatRecord[RELCAT_NO_ATTRS];  //used to store the record corresponding to the new relation
    strcpy(relCatRecord[RELCAT_REL_NAME_INDEX].sVal, relName);
    relCatRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal=nAttrs;
    relCatRecord[RELCAT_NO_RECORDS_INDEX].nVal=0;
    relCatRecord[RELCAT_FIRST_BLOCK_INDEX].nVal=-1;
    relCatRecord[RELCAT_LAST_BLOCK_INDEX].nVal=-1;
    relCatRecord[RELCAT_NO_SLOTS_PER_BLOCK_INDEX].nVal= floor((2016 / (16 * nAttrs + 1)));
    
    int retVal=BlockAccess::insert(RELCAT_RELID,relCatRecord);
    
    if (retVal!=SUCCESS)  //in what scenarios does it return not success- here it will be E_MAXRELATIONS
    {
        return retVal;
    }
    
    for (int i=0;i<nAttrs;i++)
    {
       Attribute attrCatRecord[ATTRCAT_NO_ATTRS];  //we are creating record for every attribute 
       strcpy(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal,relName);
       strcpy(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,attrs[i]);
       attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal=(double)attrtype[i];
       attrCatRecord[ATTRCAT_PRIMARY_FLAG_INDEX].nVal=-1;
       attrCatRecord[ATTRCAT_ROOT_BLOCK_INDEX].nVal=-1;
       attrCatRecord[ATTRCAT_OFFSET_INDEX].nVal=i;

       int retVal=BlockAccess::insert(ATTRCAT_RELID,attrCatRecord);
       
       if(retVal!=SUCCESS) //in what scenarios does it return not success
        {
            Schema::deleteRel(relName);
            return E_DISKFULL;  //disk is full will not come for relation catalog cuz its just one block and attribute catalog spans over many blocks hence its diskfull here and in the prev retval ist just return retval 
        }
    }
    return SUCCESS;
}

int Schema::deleteRel(char *relName)
{
     
    if (strcmp(RELCAT_RELNAME,relName)==0 ||strcmp(ATTRCAT_RELNAME,relName)==0)
    {
        return E_NOTPERMITTED;
    }

    int relid= OpenRelTable::getRelId(relName);
    if (relid !=E_RELNOTOPEN)  //if relation is open it wud return a normal number not E_RELOPEN so dont do relid==E_RELOPEN
    {
        return E_RELOPEN;
    }
    int retVal=BlockAccess::deleteRelation(relName);
    return retVal;

    /* the only that should be returned from deleteRelation() is E_RELNOTEXIST.
       The deleteRelation call may return E_OUTOFBOUND from the call to
       loadBlockAndGetBufferPtr, but if your implementation so far has been
       correct, it should not reach that point. That error could only occur
       if the BlockBuffer was initialized with an invalid block number.
    */
}