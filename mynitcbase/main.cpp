#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include <iostream>

int main(int argc, char *argv[]) {
  Disk disk_run;
  StaticBuffer buffer;
  OpenRelTable cache;
  for (int relId=0;relId<2;relId++)
  {
      RelCatEntry* relCatBuf;
      RelCacheTable::getRelCatEntry(relId, relCatBuf);
      printf("Relation name: %s\n", relCatBuf->relName);

      for (int j=0; j<relCatBuf->numAttrs; j++)
      {
          AttrCatEntry* attrCatBuf;
          AttrCacheTable::getAttrCatEntry(relId,j,attrCatBuf);

          const char *attrType=attrCatBuf->attrType==NUMBER ?"NUM":"STR"; //imp dont forget this

          printf(" %s: %s\n", attrCatBuf->attrName, attrType);
      }
  }

  return 0;
}