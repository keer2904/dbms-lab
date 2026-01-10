#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include <iostream>

int main(int argc, char *argv[]) 

{
  Disk disk_run; //a temporary copy of the disk contents before the starting of a new session. 
  //if the system has a forced shutdown , the previous state of the disk is not lost.
  
  unsigned char buffer1[BLOCK_SIZE];
  Disk::readBlock(buffer1,0);

  for (int i=0;i<10;i++)
  {
    std::cout<<(int)buffer1[i] <<", ";
  }

  unsigned char buffer[BLOCK_SIZE];
  Disk::readBlock(buffer,7000); //reading contents of block 7000 into "buffer' array
  char message[]= "hello";

  memcpy(buffer+20,message,6);
  Disk::writeBlock(buffer,7000); //writing the contents of buffer into block 7000

  unsigned char buff[BLOCK_SIZE];
  char msg[6];

  Disk::readBlock(buff,7000);
  memcpy(msg,buff+20,6);
  std::cout << "\n" << msg << "\n";

  return 0;
}