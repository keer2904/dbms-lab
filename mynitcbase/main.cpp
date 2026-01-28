#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include <iostream>

int main(int argc, char *argv[]) { //to fetch the catalog entries from the CACHE instead of the disk. 
  Disk disk_run;
  StaticBuffer buffer;  
//   //SAME STAGE-2 CODE
//   // create objects for the relation catalog and attribute catalog
//   RecBuffer relCatBuffer(RELCAT_BLOCK);
//   RecBuffer attrCatBuffer(ATTRCAT_BLOCK);

//   HeadInfo relCatHeader;
//   HeadInfo attrCatHeader;

//   // load the headers of both the blocks into relCatHeader and attrCatHeader.
//   // (we will implement these functions later)
//   relCatBuffer.getHeader(&relCatHeader);
//   attrCatBuffer.getHeader(&attrCatHeader);

//   for (int i=0; i< relCatHeader.numEntries; i++) /* i = 0 to total relation count */ {

//     Attribute relCatRecord[RELCAT_NO_ATTRS]; // will store the record from the relation catalog

//     relCatBuffer.getRecord(relCatRecord, i);

//     printf("Relation: %s\n", relCatRecord[RELCAT_REL_NAME_INDEX].sVal);

//     for (int j=0 ; j< attrCatHeader.numEntries;j++) /* j = 0 to number of entries in the attribute catalog */ {

//       // declare attrCatRecord and load the attribute catalog entry into it

//       Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
//       attrCatBuffer.getRecord(attrCatRecord, j);

//       if (strcmp(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal,relCatRecord[RELCAT_REL_NAME_INDEX].sVal)==0)   //sVal is string value of union attribute
//         /* attribute catalog entry corresponds to the current relation */
//         {
//           const char *attrType = attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal == NUMBER ? "NUM" : "STR";
//           printf("  %s: %s\n", attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal/* get the attribute name */, attrType);
//         } 
//     }
//     printf("\n");
//   }
//   return 0;  


// *********CACHE******//
  OpenRelTable cache;
  for (int relId=0;relId<3;relId++)
  {
      RelCatEntry relCatBuf;  //dont do * here just bcoz u need pointer to that function. if u do that it is actually declaration not intialization so what happens is
      // it goes to garbage address and when u try to make changes or write to tht entry it gives u segmentation fault 
      //now if u dont put pointer its a normal variable whose address is allocated by tht class so just pass address. 
      //so if u have to do * first u need to intialize which means use malloc like in attrcacheentry openreltable.cpp.
            RelCacheTable::getRelCatEntry(relId, &relCatBuf);
      printf("Relation name: %s\n", relCatBuf.relName);

      for (int j=0; j<relCatBuf.numAttrs; j++)
      {
          AttrCatEntry attrCatBuf;
          AttrCacheTable::getAttrCatEntry(relId,j,&attrCatBuf);

          const char *attrType=attrCatBuf.attrType==NUMBER ?"NUM":"STR"; //imp dont forget this

          printf(" %s: %s\n", attrCatBuf.attrName, attrType);
      }
  }

  return 0;
}
