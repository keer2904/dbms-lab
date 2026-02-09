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
int Algebra::select(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE], char attr[ATTR_SIZE], int op, char strVal[ATTR_SIZE])
{
    int srcRelId= OpenRelTable::getRelId(srcRel);
    if (srcRelId == E_RELNOTOPEN)
    {
        return E_RELNOTOPEN;
    }

    AttrCatEntry attrCatEntry;

    int ret=AttrCacheTable::getAttrCatEntry(srcRelId, attr, &attrCatEntry);

    if (ret==E_ATTRNOTEXIST)
    {
        return ret;
    }

    int type= attrCatEntry.attrType;
    Attribute attrVal;
    if (type==NUMBER)
    {
        if (isNumber(strVal))
        {
            attrVal.nVal=atof(strVal);
        }
        else
        {
            return E_ATTRTYPEMISMATCH;
        }
    }
    else if (type==STRING)
    {
        strcpy(attrVal.sVal, strVal);
    }

    RelCacheTable::resetSearchIndex(srcRelId);

    RelCatEntry relCatbufEntry;

    RelCacheTable::getRelCatEntry(srcRelId, &relCatbufEntry);

      /************************
         The following code prints the contents of a relation directly to the output
        console. Direct console output is not permitted by the actual the NITCbase
        specification and the output can only be inserted into a new relation. We will
        be modifying it in the later stages to match the specification.
        ***********************WDYMMM*/
    // Printing the attribute column names first
    
    printf("|");
    for (int i=0; i<relCatbufEntry.numAttrs; ++i) //why is nit ++i not i++
    {
        AttrCatEntry attrCatEntry;

        AttrCacheTable::getAttrCatEntry(srcRelId, i, &attrCatEntry);

        printf(" %s |", attrCatEntry.attrName);
    }
    printf("\n");

    while(true)
    {
        RecId searchRes =BlockAccess::linearSearch(srcRelId, attr, attrVal, op);

        if (searchRes.block != -1 && searchRes.slot !=-1)
        {
            RecBuffer recbuffer(searchRes.block);

            int recNumAttrs=relCatbufEntry.numAttrs;

            Attribute record[recNumAttrs];

            recbuffer.getRecord(record, searchRes.slot);

            printf("|");
            for (int i=0; i<relCatbufEntry.numAttrs; ++i) 
            {
                AttrCatEntry attrCatEntry;

                AttrCacheTable::getAttrCatEntry(srcRelId, i, &attrCatEntry);

                if (attrCatEntry.attrType==NUMBER)
                {
                    printf(" %d |", (int)record[i].nVal);
                }
                else
                {
                    printf(" %s |", record[i].sVal);
                }
            }
            printf("\n");
        }
        else
        {
            break;
        }
    }
    return SUCCESS;
}


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