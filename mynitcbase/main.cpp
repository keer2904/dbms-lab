#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include <iostream>

int main(int argc, char *argv[]) {
  Disk disk_run;
  StaticBuffer buffer;
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