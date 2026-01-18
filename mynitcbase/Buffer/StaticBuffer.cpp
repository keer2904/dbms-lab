#include "StaticBuffer.h"

// Both these arrays are static members of the class and hence need to be explicitly declared before they can be used.
unsigned char StaticBuffer::blocks[BUFFER_CAPACITY][BLOCK_SIZE];
struct BufferMetaInfo StaticBuffer::metainfo[BUFFER_CAPACITY];

StaticBuffer::StaticBuffer()
{
    for (int i=0; i<BUFFER_CAPACITY;i++)
    {
        metainfo[i].free=true; //check metainfor structure it has bool free
    }
}

StaticBuffer::~StaticBuffer()
{

/*
At this stage, we are not writing back from the buffer to the disk since we are
not modifying the buffer. So, we will define an empty destructor for now. In
subsequent stages, we will implement the write-back functionality here.
*/

}

int StaticBuffer::getFreeBuffer(int blockNum)
{
    if (blockNum < 0 || blockNum > DISK_BLOCKS)
    {
        return E_OUTOFBOUND;
    }

    int allocatedBuffer;

    for (int i=0; i<BUFFER_CAPACITY;i++)
    {
        if (metainfo[i].free==true)
        {
            allocatedBuffer=i;
            break;
        }
    }

    metainfo[allocatedBuffer].free=false;
    metainfo[allocatedBuffer].blockNum=blockNum;

    return allocatedBuffer;
}

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