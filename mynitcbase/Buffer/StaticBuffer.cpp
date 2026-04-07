#include "StaticBuffer.h"
#include <cstring> //need to include this to make sure memcpy works

//stage-3, stage-6, stage-7
// Both these arrays are static members of the class and hence need to be explicitly declared before they can be used.
unsigned char StaticBuffer::blocks[BUFFER_CAPACITY][BLOCK_SIZE];
struct BufferMetaInfo StaticBuffer::metainfo[BUFFER_CAPACITY];

unsigned char StaticBuffer::blockAllocMap[DISK_BLOCKS];

StaticBuffer::StaticBuffer()
{
    unsigned char buffer[BLOCK_SIZE];
    for (int i=0; i<4; i++)  //i is basically bmap value 
    {
        //here its read and then copy contents
        Disk::readBlock(buffer, i);
        memcpy(blockAllocMap+(i*BLOCK_SIZE), buffer, BLOCK_SIZE); //copy blockAllocMap blocks from disk to buffer
    }

    for (int i=0; i<BUFFER_CAPACITY;i++)
    {
        metainfo[i].free=true;
        metainfo[i].blockNum=-1;
        metainfo[i].dirty=false;
        metainfo[i].timeStamp=-1;
    }
}

//stage-6
// Writing back all modified blocks on system exit
StaticBuffer::~StaticBuffer()
{   
    for (int i=0; i<4; i++)  //i is basically bmap value 
    {
        //here its copy and then write contents
        unsigned char buffer[BLOCK_SIZE]; //since u are writing contents better to define everytime else it will get overwritten
        memcpy(buffer, blockAllocMap+(i*BLOCK_SIZE), BLOCK_SIZE); //copy blockAllocMap blocks from disk to buffer
        Disk::writeBlock(buffer, i);
    }
    
    for (int i=0; i<BUFFER_CAPACITY;i++)
    {
        if (metainfo[i].free==false && metainfo[i].dirty==true)
        {
            Disk::writeBlock(StaticBuffer::blocks[i], metainfo[i].blockNum);
        }
    }
}

//stage-6 adds the below to getFreeBuffer()
 // if a free buffer is not available, find the buffer with the largest timestamp  
//     IF IT IS DIRTY, write back to the disk using Disk::writeBlock()
//     set bufferNum = index of this buffer
// update the metaInfo entry corresponding to bufferNum with free:false, dirty:false, blockNum:the input block number, timeStamp:0.
int StaticBuffer::getFreeBuffer(int blockNum)
{
    if (blockNum < 0 || blockNum > DISK_BLOCKS)
    {
        return E_OUTOFBOUND;
    }

    for(int j = 0; j < BUFFER_CAPACITY; j++)
    {
        if(metainfo[j].free == false)
        {
            metainfo[j].timeStamp++;
        }
    }

    int allocatedBuffer=-1;

    for (int i=0; i<BUFFER_CAPACITY;i++)
    {
        if (metainfo[i].free==true)
        {
            allocatedBuffer=i;
            break;
        }
    }
// if a free buffer is not available, find the buffer with the largest timestamp  
    if (allocatedBuffer==-1)
    {
        int max_time=-1;
        int max_index=0;
        for (int i=0;i<BUFFER_CAPACITY;i++)
        {
            if (metainfo[i].timeStamp>max_time)
            {
                max_time=metainfo[i].timeStamp;
                max_index=i;
            }
        }
//     IF IT IS DIRTY, write back to the disk using Disk::writeBlock()
        if (metainfo[max_index].dirty==true)
        {
            Disk::writeBlock(StaticBuffer::blocks[max_index], metainfo[max_index].blockNum);
        }
        allocatedBuffer=max_index;
    }

    metainfo[allocatedBuffer].free=false;
    metainfo[allocatedBuffer].dirty=false;
    metainfo[allocatedBuffer].timeStamp=0;         //BUT WHY
    metainfo[allocatedBuffer].blockNum=blockNum;

    return allocatedBuffer;
}


//stage-3
int StaticBuffer::getBufferNum(int block_num)
{
    if (block_num < 0 || block_num > DISK_BLOCKS)
    {
        return E_OUTOFBOUND;
    }

     for (int i=0; i<BUFFER_CAPACITY;i++)
    {
        if (metainfo[i].blockNum==block_num)
        {
            return i;
        }
    }
    return E_BLOCKNOTINBUFFER;
}

//stage-6

int StaticBuffer::setDirtyBit(int blockNum)
{
    int ret=StaticBuffer::getBufferNum(blockNum);
    if (ret ==E_BLOCKNOTINBUFFER)
    {
        return E_BLOCKNOTINBUFFER;
    }

    if(ret==E_OUTOFBOUND)
    {
        return E_OUTOFBOUND;
    }

    metainfo[ret].dirty=true;
    return SUCCESS;
}

//stage-10
int StaticBuffer::getStaticBlockType(int blockNum)
{
    if (blockNum<0 || blockNum>=DISK_BLOCKS)
    {
        return E_OUTOFBOUND;
    }

    unsigned char blockType=blockAllocMap[blockNum];  //Access the entry in block allocation map corresponding to the blockNum argument

    return (int)blockType;
}