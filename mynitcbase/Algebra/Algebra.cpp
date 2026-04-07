#include "Algebra.h"
#include <cstring>
#include <iostream>


bool isNumber(char *str)  // will return if a string can be parsed as a floating point number
{
    int len;
    float ignore;
    int ret=sscanf(str, "%f %n", &ignore, &len);
    return ret ==1 && len==strlen(str);

    /*
    sscanf returns the number of elements read, so if there is no float matching
    the first %f, ret will be 0, else it'll be 1

    %n gets the number of characters read. this scanf sequence will read the
    first float ignoring all the whitespace before and after. and the number of
    characters read that far will be stored in len. if len == strlen(str), then
    the string only contains a float with/without whitespace. else, there's other
    characters.
  */
}

//stage-7

int Algebra:: insert(char relName[ATTR_SIZE], int nAttrs, char record[][ATTR_SIZE])  //WHY 2D ARRAY WHAT IS THAT??
{
    if (strcmp(relName,"RELATIONCAT")==0 || strcmp(relName,"ATTRIBUTECAT")==0)
    {
        return E_NOTPERMITTED;
    }

    int relId=OpenRelTable::getRelId(relName);

    if(relId==E_RELNOTOPEN)
    {
        return E_RELNOTOPEN;
    }

    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(relId, &relCatEntry);

    if (relCatEntry.numAttrs !=nAttrs)
    {
        return E_NATTRMISMATCH;
    }

    union Attribute recordValues[nAttrs];

    // Converting 2D char array of record values to Attribute array recordValues
    
    for (int i=0;i<nAttrs;i++)
    {
        
        AttrCatEntry attrCatEntry;
        AttrCacheTable::getAttrCatEntry(relId,i, &attrCatEntry);

        int type=attrCatEntry.attrType;
        if (type==NUMBER)
        {
            if (isNumber(record[i]))
            {
                recordValues[i].nVal=atof(record[i]); 
            }
            else
            {
                return E_ATTRTYPEMISMATCH;
            }
        }

        else if(type ==STRING)
        {
            strcpy(recordValues[i].sVal,record[i]);
        }
    }

    int retVal= BlockAccess::insert(relId,recordValues);

    return retVal;

}

//stage-9, stage-10

int Algebra::select(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE], char attr[ATTR_SIZE], int op, char strVal[ATTR_SIZE]) 
{
    int srcRelId= OpenRelTable::getRelId(srcRel);
    if (srcRelId ==E_RELNOTOPEN)
    {
        return E_RELNOTOPEN;
    }
    AttrCatEntry attrCatEntry;
    int retVal=AttrCacheTable::getAttrCatEntry(srcRelId,attr,&attrCatEntry);
    
    if (retVal!=SUCCESS)
    {
        return E_ATTRNOTEXIST;
    }

    Attribute attrVal;
    int type = attrCatEntry.attrType;

    if (type == NUMBER)
    {
        if (isNumber(strVal))
        {
            attrVal.nVal=atof(strVal); //convert it into double
        }
        else
        {
            return E_ATTRTYPEMISMATCH;
        }
    }
    else if (type == STRING)
    {
        strcpy(attrVal.sVal,strVal);
    }

    RelCatEntry relCatBuf;
    RelCacheTable::getRelCatEntry(srcRelId, &relCatBuf);
    int src_nAttrs = relCatBuf.numAttrs;  
 
    char attr_names[src_nAttrs][ATTR_SIZE];

    int attr_types[src_nAttrs];

    for (int i=0; i<src_nAttrs;i++)
    {
        AttrCatEntry attrCatEntry;
        AttrCacheTable::getAttrCatEntry(srcRelId,i, &attrCatEntry);
        strcpy(attr_names[i],attrCatEntry.attrName);
        attr_types[i]=attrCatEntry.attrType;             //fill the attr_names, attr_types arrays that we declared with the entries of corresponding attributes
    }

    retVal=Schema::createRel(targetRel,src_nAttrs,attr_names,attr_types);
    if (retVal!=SUCCESS)
    {
        return retVal;
    }
    int targetRelId=OpenRelTable::openRel(targetRel); //Open the newly created target relation to store the target relid 
    
    if (targetRelId<0) //If opening fails
    {
        Schema::deleteRel(targetRel);
        return targetRelId;
    }
    /*** Selecting and inserting records into the target relation ***/

    RelCacheTable::resetSearchIndex(targetRelId);
    Attribute record[src_nAttrs];
    RelCacheTable::resetSearchIndex(srcRelId);
   
    AttrCacheTable::resetSearchIndex(srcRelId,attr);

    BPlusTree::numComparisons=0;
    BlockAccess::numLinearComparisons=0;
    //The BlockAccess::search() function can either do a linearSearch or a B+ tree search. 
    //reset the search index of the relation in the relation cache using RelCacheTable::resetSearchIndex().
    //Reset the search index in the attribute cache for the select condition attribute with name given by the argument `attr`.
    //Both these calls are necessary to ensure that search begins from the first record.
    
//searches the record acc to att=attrVal then it adds to the taregtRel table
    while (BlockAccess::search(srcRelId,record,attr,attrVal,op)==SUCCESS)   //please look into these parameters and see why each of it is taken.
    {
        int ret = BlockAccess::insert(targetRelId, record); 

        if (ret!=SUCCESS)
        {
            Schema::closeRel(targetRel);
            Schema::deleteRel(targetRel);
            return ret;
        }

    }
    printf("Number of BPlus Tree Comparisons: %d\n", BPlusTree::numComparisons);
    printf("Number of Linear Search Comparisons: %d\n", BlockAccess::numLinearComparisons);
    Schema::closeRel(targetRel);
    return SUCCESS;
}

int Algebra::project(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE]) 
{
    int srcRelId = OpenRelTable::getRelId(srcRel);
    if(srcRelId == E_RELNOTOPEN)   //if srcRel is not open in open relation table
    {
        return E_RELNOTOPEN;
    }
    RelCatEntry relCatBuf;
    RelCacheTable::getRelCatEntry(srcRelId,&relCatBuf);

    int numAttrs=relCatBuf.numAttrs;
    
    char attrNames[numAttrs][ATTR_SIZE];
    int attrTypes[numAttrs];

    for (int i=0;i<numAttrs;i++)
    {
        AttrCatEntry attrCatEntry;
        AttrCacheTable::getAttrCatEntry(srcRelId,i,&attrCatEntry);
        strcpy(attrNames[i],attrCatEntry.attrName);
        attrTypes[i]=attrCatEntry.attrType;
    }

    /*** Creating and opening the target relation ***/

    int ret= Schema::createRel(targetRel,numAttrs,attrNames,attrTypes);
    if(ret!=SUCCESS)  //if the createRel returns an error code, then return that value.
    {
        return ret;
    }
    
    int targetRelId=OpenRelTable::openRel(targetRel);
    if (targetRelId<0)    //If opening fails,
    {
        Schema::deleteRel(targetRel);
        return targetRelId;
    }


    /*** Inserting projected records into the target relation ***/

    RelCacheTable::resetSearchIndex(srcRelId); //how do you know which id you shud give target or src
    Attribute record[numAttrs];


    while (BlockAccess::project(srcRelId,record)== SUCCESS)
    {
        ret = BlockAccess::insert(targetRelId, record);

        if (ret!=SUCCESS) 
        {
            Schema::closeRel(targetRel);
            Schema::deleteRel(targetRel);
            return ret;
        }
    }
    Schema::closeRel(targetRel);
    return SUCCESS;
}

int Algebra::project(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE], int tar_nAttrs, char tar_Attrs[][ATTR_SIZE]) 
{
    int srcRelId = OpenRelTable::getRelId(srcRel);
    if(srcRelId== E_RELNOTOPEN)
    {
        return E_RELNOTOPEN;
    }
    RelCatEntry relCatBuf;
    
    RelCacheTable::getRelCatEntry(srcRelId,&relCatBuf);
    int numAttrs=relCatBuf.numAttrs;  //numAttrs=src_nAttrs

    int attr_offset[tar_nAttrs];
    int attr_types[tar_nAttrs];

    //Checking if attributes of target are present in the source relation and storing its offsets and types

    for (int i=0;i<tar_nAttrs;i++)  //why is it not numAttrs
    {
        AttrCatEntry attrCatBuf;
        int ret=AttrCacheTable::getAttrCatEntry(srcRelId,tar_Attrs[i],&attrCatBuf); //please understand the parameters why is it not just i
        if (ret!=SUCCESS)
        {
            return E_ATTRNOTEXIST;
        }
        attr_offset[i]=attrCatBuf.offset;
        attr_types[i]=attrCatBuf.attrType;
    }

    /*** Creating and opening the target relation ***/

    int retVal=Schema::createRel(targetRel,tar_nAttrs,tar_Attrs,attr_types);
    if (retVal!=SUCCESS)
    {
        return retVal;
    }
    int targetRelId=OpenRelTable::openRel(targetRel);

    if (targetRelId<0)
    {
        Schema::deleteRel(targetRel);
        return targetRelId;
    }

    /*** Inserting projected records into the target relation ***/
    
    RelCacheTable::resetSearchIndex(srcRelId);
    Attribute record[numAttrs];

    while (BlockAccess::project(srcRelId, record)==SUCCESS) 
    {
        Attribute proj_record[tar_nAttrs];

        for (int i=0; i<tar_nAttrs;i++)
        {
            proj_record[i] = record[attr_offset[i]];
        }
        int ret = BlockAccess::insert(targetRelId, proj_record);
        if (ret!=SUCCESS) 
        {
            Schema::closeRel(targetRel);
            Schema::deleteRel(targetRel);
            return ret;
        }
    }
    Schema::closeRel(targetRel);
    return SUCCESS;
}