#include "BPlusTree.h"

#include <cstring>
#include <cstdio>

int BPlusTree::numComparisons;  //exercise 10
 
//stage-10

RecId BPlusTree::bPlusSearch(int relId, char attrName[ATTR_SIZE], Attribute attrVal, int op) 
{

    IndexId searchIndex;  //used to store search index for attrName.

    AttrCacheTable::getSearchIndex(relId,attrName,&searchIndex);   
    
    AttrCatEntry attrCatEntry;

    AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry); 
    
    int block, index;

    if (searchIndex.block==-1 && searchIndex.index==-1)  // (search is done for the first time)
    {
        // start the search from the first entry of root.
        block = attrCatEntry.rootBlock;
        index = 0;

        if (block == -1) 
        {
            return RecId{-1, -1};
        }

    } 
    else   //a valid searchIndex points to an entry in the leaf index of the attribute'sB+ Tree which had previously satisfied the op for the given attrVal.
    {
        block = searchIndex.block;
        index = searchIndex.index + 1;  // search is resumed from the next index.

        IndLeaf leaf(block); // load block into leaf using IndLeaf::IndLeaf()

        HeadInfo leafHead;     //used to hold the header of leaf.
        
        leaf.getHeader(&leafHead);

        if (index >= leafHead.numEntries)  // all the entries in the block has been searched; search from the beginning of the next leaf index block.
        {
            block=leafHead.rblock;
            index=0;
            if (block==-1)     // end of linked list reached - the search is done
            {
                return RecId{-1, -1};
            }
        }
    }

    // Traverse through all the internal nodes according to value of attrVal and the operator op  

    /* (This section is only needed when
        - search restarts from the root block (when searchIndex is reset by caller)
        - root is not a leaf
        If there was a valid search index, then we are already at a leaf block
        and the test condition in the following loop will fail)
    */

    while(StaticBuffer::getStaticBlockType(block)==1)
    { 
        IndInternal internalBlk(block); // load the block into internalBlk using IndInternal::IndInternal().
        HeadInfo intHead;

        internalBlk.getHeader(&intHead);  // load the header of internalBlk into intHead

        InternalEntry intEntry;   //used to store an entry of internalBlk.

        if (op==NE|| op==LT || op==LE) 
        {
            /*
            - NE: need to search the entire linked list of leaf indices of the B+ Tree,
            starting from the leftmost leaf index. Thus, always move to the left.

            - LT and LE: the attribute values are arranged in ascending order in the
            leaf indices of the B+ Tree. Values that satisfy these conditions, if
            any exist, will always be found in the left-most leaf index. Thus,
            always move to the left.
            */

            internalBlk.getEntry(&intEntry,0); // load entry in the first slot of the block into intEntry
            block = intEntry.lChild;
        } 
        
        else 
        {
            /*
            - EQ, GT and GE: move to the left child of the first entry that is
            greater than (or equal to) attrVal
            (we are trying to find the first entry that satisfies the condition.
            since the values are in ascending order we move to the left child which
            might contain more entries that satisfy the condition)
            */

            //traverse through all entries of internalBlk and find an entry that satisfies the condition.
            //Hint: the helper function compareAttrs() can be used for comparing
            int i=0;
            for (; i<intHead.numEntries;i++)
            {
                internalBlk.getEntry(&intEntry,i);
                int ret=compareAttrs(intEntry.attrVal, attrVal, attrCatEntry.attrType);
                BPlusTree::numComparisons++;  //exercise 10


                if (((op==EQ || op==GE) && ret>=0) || (op==GT && ret>0))
                //if op == EQ or GE, then intEntry.attrVal >= attrVal or if op == GT, then intEntry.attrVal > attrVal
                {
                    break;
                }

            }

            if (i<intHead.numEntries) //Move to the left child of that entry bcoz i has still not reach intHead.numEntries
            //lets say right now it went thru break statement bcoz your value is 504 
            //and you r searching for records greater than 500
            //if you go left child of 504 u can still have chance of those records 
            {
                block=intEntry.lChild;
            }
            else
            {
                block=intEntry.rChild;  //Move to the right child of the entry of the block
            }
        }
    }

    // NOTE: `block` now has the block number of a leaf index block.

    // Identify the first leaf index entry from the current position that satisfies our condition (moving right)

    while (block != -1) 
    {
        IndLeaf leafBlk(block);   // load the block into leafBlk using IndLeaf::IndLeaf().
        HeadInfo leafHead;

        leafBlk.getHeader(&leafHead);

        Index leafEntry;

        while (index<leafHead.numEntries) 
        {
            leafBlk.getEntry(&leafEntry,index);     // load entry corresponding to block and index into leafEntry

            int cmpVal = compareAttrs(leafEntry.attrVal, attrVal, attrCatEntry.attrType); //comparison between leafEntry's attribute value and input attrVal
            BPlusTree::numComparisons++; //exercise 10
            
            if ((op == EQ && cmpVal == 0) ||(op == LE && cmpVal <= 0) ||(op == LT && cmpVal < 0) ||(op == GT && cmpVal > 0) ||(op == GE && cmpVal >= 0) ||(op == NE && cmpVal != 0))
            {
                // set search index to {block, index}
                searchIndex.block=block; 
                searchIndex.index=index; 
                AttrCacheTable::setSearchIndex(relId, attrName, &searchIndex);  //when they say to set search index dont just equate use this function as well
                return RecId{leafEntry.block, leafEntry.slot};
            } 
            else if ((op == EQ || op == LE || op == LT) && cmpVal > 0)  //future entries will not satisfy EQ, LE, LT since the values are arranged in ascending order in the leaves
            {
                return RecId {-1, -1};
            }
            ++index;    // search next index
        }
        //for all the other op it is guaranteed that the block being searched will have an entry, if it exists, satisying that op.
        if (op != NE)   // only for NE operation, we have to check the entire linked list;
        {
            break;
        }
        block=leafHead.rblock;
        index=0;
    }
    return RecId {-1,-1};  // no entry satisying the op was found; 
}


int BPlusTree::bPlusCreate(int relId, char attrName[ATTR_SIZE]) 
{
    if (relId==RELCAT_RELID || relId==ATTRCAT_RELID)
    {
       return E_NOTPERMITTED;
    }

    AttrCatEntry attrCatBuf;
    int ret=AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatBuf);
    if(ret!=SUCCESS)
    {
        return ret;
    }

    if (attrCatBuf.rootBlock!=-1)       // Index already exists
    {
        return SUCCESS;
    }

    /******Creating a new B+ Tree ******/
    IndLeaf rootBlockBuf;       //using constructor 1
    
    int rootBlock = rootBlockBuf.getBlockNum();     // declare rootBlock to store the blockNumber of the new leaf block
    
    if (rootBlock == E_DISKFULL)    // if there is no more disk space for creating an index
    {
        return E_DISKFULL;
    }

    attrCatBuf.rootBlock = rootBlock;
    AttrCacheTable::setAttrCatEntry(relId, attrName, &attrCatBuf);

    RelCatEntry relCatEntry;

    RelCacheTable::getRelCatEntry(relId,&relCatEntry);

    int block = relCatEntry.firstBlk;

    /***** Traverse all the blocks in the relation and insert them one by one into the B+ Tree *****/
    while (block != -1) 
    {
        RecBuffer recBuf(block);
        unsigned char slotMap[relCatEntry.numSlotsPerBlk];
        recBuf.getSlotMap(slotMap);

        for (int i=0;i<relCatEntry.numSlotsPerBlk;i++)      
        {
            if (slotMap[i]==SLOT_OCCUPIED)  // for every occupied slot of the block
            {
                Attribute record[relCatEntry.numAttrs];
                recBuf.getRecord(record,i);    //load the record corresponding to the slot into `record`

                RecId recId= RecId{block,i};
                int retVal=BPlusTree::bPlusInsert(relId,attrName, record[attrCatBuf.offset], recId);     // insert the attribute value corresponding to attrName from the record
                
                //bPlusInsert will destroy any existing bplus tree if insert fails i.e when disk is full
            
                if (retVal == E_DISKFULL) 
                {
                    return E_DISKFULL;
                }
            }  
        } 

        HeadInfo head;
        recBuf.getHeader(&head);
        block = head.rblock;
    }

    return SUCCESS;
}

int BPlusTree::bPlusDestroy(int rootBlockNum) 
{
    if (rootBlockNum<0 || rootBlockNum>=DISK_BLOCKS) 
    {
        return E_OUTOFBOUND;
    }

    int type = StaticBuffer::getStaticBlockType(rootBlockNum);

    if (type == IND_LEAF) 
    {
        IndLeaf leafBlk(rootBlockNum);
        leafBlk.releaseBlock();
        return SUCCESS;
    } 
    else if (type == IND_INTERNAL) 
    {
        IndInternal internalBlk(rootBlockNum);
        HeadInfo internalHead;
        internalBlk.getHeader(&internalHead);

        /*iterate through all the entries of the internalBlk and destroy the lChild
        of the first entry and rChild of all entries using BPlusTree::bPlusDestroy().
        (the rchild of an entry is the same as the lchild of the next entry.
         take care not to delete overlapping children more than once ) */

        InternalEntry ptr;
        internalBlk.getEntry(&ptr,0);
        BPlusTree::bPlusDestroy(ptr.lChild);     //destroy the lChild of the first entry

        for (int j=0;j<internalHead.numEntries;j++)
        {
            internalBlk.getEntry(&ptr,j);
            BPlusTree::bPlusDestroy(ptr.rChild);
        }

        internalBlk.releaseBlock();
        return SUCCESS;
    } 
    else 
    {
        return E_INVALIDBLOCK;      // block is not an index block
    }
}

int BPlusTree::bPlusInsert(int relId, char attrName[ATTR_SIZE], Attribute attrVal, RecId recId) 
{
    AttrCatEntry attrCatBuf;
    
    int ret=AttrCacheTable::getAttrCatEntry(relId,attrName,&attrCatBuf);

    if (ret!=SUCCESS)
    {
        return ret;
    }

    int rootBlock = attrCatBuf.rootBlock;

    if (rootBlock==-1) 
    {
        return E_NOINDEX;
    }
    int attrType=attrCatBuf.attrType;

    int leafBlkNum = BPlusTree::findLeafToInsert(rootBlock, attrVal, attrType);//find the leaf block to which insertion is to be done

    // insert the attrVal and recId to the leaf block at blockNum
    Index entry;
    entry.attrVal=attrVal;
    entry.block=recId.block;
    entry.slot=recId.slot;
    int retVal=BPlusTree::insertIntoLeaf(relId, attrName, leafBlkNum,entry);
   
    // the insertIntoLeaf() function will propagate the insertion to the required internal nodes by calling the required helper functions like insertIntoInternal() or createNewRoot()

    if (retVal==E_DISKFULL) 
    {
        BPlusTree::bPlusDestroy(rootBlock);     // destroy the existing B+ tree by passing the rootBlock
        attrCatBuf.rootBlock=-1;
        AttrCacheTable::setAttrCatEntry(relId,attrName,&attrCatBuf);
        return E_DISKFULL;
    }

    return SUCCESS;
}

int BPlusTree::findLeafToInsert(int rootBlock, Attribute attrVal, int attrType) 
{
    int blockNum = rootBlock;

    while (StaticBuffer::getStaticBlockType(blockNum)!= IND_LEAF ) 
    { 
        IndInternal internalBlk(blockNum);
        HeadInfo internalHead;
        internalBlk.getHeader(&internalHead);
        int i=0;
        for (;i<internalHead.numEntries;i++)
        {
            InternalEntry ptr;
            internalBlk.getEntry(&ptr,i);  //iterate through all the entries, to find the first entry whose attribute value >= value to be inserted.
            if (compareAttrs(ptr.attrVal,attrVal,attrType)>=0)
            {
                break;
            }
        }
        if (i==internalHead.numEntries) 
        {
            InternalEntry ptr;
            internalBlk.getEntry(&ptr,internalHead.numEntries-1);  //why internalHead.numEntries-1
            blockNum=ptr.rChild;
        } 
        else 
        {
            InternalEntry ptr;
            internalBlk.getEntry(&ptr,i);
            blockNum = ptr.lChild;
        }
    }
    return blockNum;
}

int BPlusTree::insertIntoLeaf(int relId, char attrName[ATTR_SIZE], int blockNum, Index indexEntry) 
{
    AttrCatEntry attrCatBuf;
    AttrCacheTable::getAttrCatEntry(relId,attrName,&attrCatBuf);
    IndLeaf leafBlk(blockNum);

    HeadInfo leafHead;
    leafBlk.getHeader(&leafHead);

    Index indices[leafHead.numEntries + 1];     // to store a list of index entries with existing indices + the new index to insert

    Index leafEntry;

    // Find where to insert
    int insertPos = 0;
    while (insertPos < leafHead.numEntries) {
        leafBlk.getEntry(&leafEntry, insertPos);
        if (compareAttrs(leafEntry.attrVal, indexEntry.attrVal, attrCatBuf.attrType) >= 0)
            break;
        insertPos++;
    }

    // Build the final array
    for (int i = 0; i <= leafHead.numEntries; i++) {
        if (i < insertPos)
            leafBlk.getEntry(&indices[i], i);
        else if (i == insertPos)
            indices[i] = indexEntry;
        else
            leafBlk.getEntry(&indices[i], i - 1);  // shift by 1 why?
    }

    if (leafHead.numEntries != MAX_KEYS_LEAF)       // (leaf block has not reached max limit)
    {
        leafHead.numEntries++;
        leafBlk.setHeader(&leafHead);
        for (int k=0; k<leafHead.numEntries;k++)
        {
            leafBlk.setEntry(&indices[k], k);           //what is indices[k]
        }
        return SUCCESS;
    }

    // If we reached here, the `indices` array has more than entries than can fit in a single leaf index block. 
    // Therefore, we will need to split the entries in `indices` between two leaf blocks. 

    int newRightBlk = splitLeaf(blockNum, indices);     //will return the blockNum of the newly allocated block or E_DISKFULL if there are no more blocks to be allocated.

    if (newRightBlk==E_DISKFULL)
    {
        return E_DISKFULL;
    }
    int ret;
    if (leafHead.pblock!=-1)        // checking whether this has parentblock. -1 means block is nonexistent.
    {  
        // insert the middle value from `indices` into the parent block (i.e the last value of the left block)
        // the middle value will be at index 31 (given by constant MIDDLE_INDEX_LEAF)
        InternalEntry internalEntry;
        internalEntry.attrVal= indices[MIDDLE_INDEX_LEAF].attrVal;
        internalEntry.lChild = blockNum;
        internalEntry.rChild = newRightBlk; 
        ret=insertIntoInternal(relId,attrName,leafHead.pblock,internalEntry);   //why parentblock?
    } 
    else 
    {
        // the current block was the root block and is now split. 
        // a new internal index block needs to be allocated and made the root of the tree.
        ret=createNewRoot(relId, attrName, indices[MIDDLE_INDEX_LEAF].attrVal,blockNum, newRightBlk);
    }

    if (ret==E_DISKFULL)
    {
        return ret;
    }
    return SUCCESS;
}

int BPlusTree::splitLeaf(int leafBlockNum, Index indices[]) 
{
    IndLeaf rightBlk ;       //to obtain new leaf index block that will be used as the right block in splitting
    IndLeaf leftBlk(leafBlockNum);  

    int rightBlkNum = rightBlk.getBlockNum();
    int leftBlkNum = leafBlockNum;

    if (rightBlkNum== E_DISKFULL) 
    {
        return E_DISKFULL;      //failed to obtain a new leaf index block because the disk is full
    }

    HeadInfo leftBlkHeader, rightBlkHeader;

    rightBlk.getHeader(&rightBlkHeader);
    leftBlk.getHeader(&leftBlkHeader);

    rightBlkHeader.numEntries = 32; // (MAX_KEYS_LEAF+1)/2
    rightBlkHeader.pblock = leftBlkHeader.pblock;
    rightBlkHeader.lblock = leftBlkNum;
    rightBlkHeader.rblock = leftBlkHeader.rblock;
    rightBlk.setHeader(&rightBlkHeader);

    leftBlkHeader.numEntries = 32;    //(MAX_KEYS_LEAF+1)/2
    leftBlkHeader.rblock = rightBlkNum;
    leftBlk.setHeader(&leftBlkHeader);

    for (int i=0;i<=MIDDLE_INDEX_LEAF;i++)
    {
        leftBlk.setEntry(&indices[i],i);        // set the first 32 entries of leftBlk = the first 32 entries of indices array
        rightBlk.setEntry(&indices[i+32],i);    // set the first 32 entries of newRightBlk = the next 32 entries of indices array
    }

    return rightBlkNum;
}

int BPlusTree::insertIntoInternal(int relId, char attrName[ATTR_SIZE], int intBlockNum, InternalEntry intEntry) 
{
    AttrCatEntry attrCatBuf;
    AttrCacheTable::getAttrCatEntry(relId,attrName,&attrCatBuf);
    IndInternal intBlk(intBlockNum);

    HeadInfo blockHeader;
    intBlk.getHeader(&blockHeader);
    InternalEntry internalEntries[blockHeader.numEntries + 1];  // Array to hold existing entries + the new entry

    // Update the lChild of the internalEntry immediately following the newly added entry to the rChild of the newly added entry
    
    InternalEntry tempEntry;

    // Find insertion point
    int insertPos = 0;
    while (insertPos < blockHeader.numEntries) {
        intBlk.getEntry(&tempEntry, insertPos);
        if (compareAttrs(tempEntry.attrVal, intEntry.attrVal, attrCatBuf.attrType) >= 0)
            break;
        insertPos++;
    }

    // Build the final array
    for (int i = 0; i <= blockHeader.numEntries; i++) {
        if (i < insertPos)
            intBlk.getEntry(&internalEntries[i], i);
        else if (i == insertPos)
            internalEntries[i] = intEntry;
        else {
            intBlk.getEntry(&internalEntries[i], i - 1);  // shift by 1
        }
    }

    // Fix the lChild of entry AFTER the inserted one
    if (insertPos < blockHeader.numEntries) {
        internalEntries[insertPos + 1].lChild = intEntry.rChild;
    }

    if (blockHeader.numEntries != MAX_KEYS_INTERNAL)    // internal index block has not reached max limit
    {
        blockHeader.numEntries++;
        intBlk.setHeader(&blockHeader);

        for (int j=0;j<blockHeader.numEntries;j++)
        {
            intBlk.setEntry(&internalEntries[j],j);     //what is &internalEntries[j]
        }
        return SUCCESS;
    }

    // If we reached here, the `internalEntries` array has more than entries than can fit in a single internal index block. 
    // Therefore, we will need to split the entries in `internalEntries` between two internal index blocks. 
    // We do this using the splitInternal() function.
    // This function will return the blockNum of the newly allocated block or E_DISKFULL if there are no more blocks to be allocated.

    int newRightBlk = splitInternal(intBlockNum, internalEntries);

    if (newRightBlk==E_DISKFULL) 
    {
        bPlusDestroy(intEntry.rChild);    // This corresponds to the tree built up till now that has not yet been connected to the existing B+ Tree
        return E_DISKFULL;
    }

    int retVal;

    if (blockHeader.pblock!=-1)     // the current block was not the root 
    {  
        InternalEntry internalEntry;
        internalEntry.lChild = intBlockNum;         //intBlockNum=currentblock
        internalEntry.rChild = newRightBlk;
        internalEntry.attrVal = internalEntries[MIDDLE_INDEX_INTERNAL].attrVal;
        retVal=insertIntoInternal(relId,attrName,blockHeader.pblock,internalEntry);      // insert the middle value from `internalEntries` into the parent block 
    } 
    else 
    {
        // the current block was the root block and is now split. A new internal index block needs to be allocated and made the root of the tree.
    
        retVal=createNewRoot(relId, attrName,internalEntries[MIDDLE_INDEX_INTERNAL].attrVal,intBlockNum,newRightBlk);
    }

    if (retVal==E_DISKFULL)
    {
        return retVal;
    }
    return SUCCESS;
}

int BPlusTree::splitInternal(int intBlockNum, InternalEntry internalEntries[]) 
{
    IndInternal rightBlk;     // internal index block that will be used as the right block in the splitting
    IndInternal leftBlk(intBlockNum);

    int rightBlkNum = rightBlk.getBlockNum();   
    int leftBlkNum = intBlockNum;

    if (rightBlkNum==E_DISKFULL) 
    {
        return E_DISKFULL;
    }

    HeadInfo leftBlkHeader, rightBlkHeader;
    // get the headers of left block and right block using 
    rightBlk.getHeader(&rightBlkHeader);
    leftBlk.getHeader(&leftBlkHeader);

    // set rightBlkHeader with the following values
    rightBlkHeader.numEntries=50;       //(MAX_KEYS_INTERNAL)/2   //how 50 for internal and 32 for leaf?
    rightBlkHeader.pblock=leftBlkHeader.pblock;
    rightBlk.setHeader(&rightBlkHeader);
    
    leftBlkHeader.numEntries=50;        //(MAX_KEYS_INTERNAL)/2 
    leftBlk.setHeader(&leftBlkHeader);
    
    /*
    - set the first 50 entries of leftBlk = index 0 to 49 of internalEntries array
    - set the first 50 entries of newRightBlk = entries from index 51 to 100 of internalEntries array using IndInternal::setEntry().
    NOTE: index 50 will be moving to the parent internal index block  
    */

    for (int i=0;i<50;i++)
    {
        leftBlk.setEntry(&internalEntries[i],i);
        rightBlk.setEntry(&internalEntries[i+51],i);        //internal has 101 entries so it goes till 100 hence 51 and not 50
    }

    int type = StaticBuffer::getStaticBlockType(internalEntries[0].lChild); // block type of a child of any entry of the internalEntries array  WHY ANY ENTRY?


    //for each child block of the new right block

    BlockBuffer blockbuffer (internalEntries[MIDDLE_INDEX_INTERNAL+1].lChild);   //51 ka left child 

    HeadInfo blockHheader;
    blockbuffer.getHeader(&blockHheader);
    blockHheader.pblock = rightBlkNum;
    blockbuffer.setHeader(&blockHheader);

    
    for (int i=51;i<101;i++) 
    {
        BlockBuffer blockBuff(internalEntries[i].rChild);     //to access the right child block 
        blockBuff.getHeader(&blockHheader);
        blockHheader.pblock = rightBlkNum;              //Updating the pblock of all entries in new right block
        blockBuff.setHeader(&blockHheader);
    }

    return rightBlkNum;
}

int BPlusTree::createNewRoot(int relId, char attrName[ATTR_SIZE], Attribute attrVal, int lChild, int rChild) 
{
    AttrCatEntry attrCatBuf;
    AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatBuf);

    IndInternal newRootBlk;
   
    int newRootBlkNum =newRootBlk.getBlockNum();

    if (newRootBlkNum == E_DISKFULL) 
    {
        bPlusDestroy(rChild);       //why destryoing just the rchild??
        return E_DISKFULL;
    }
 
    HeadInfo newBlkHeader;
    newRootBlk.getHeader(&newBlkHeader);
    newBlkHeader.numEntries=1;
    newRootBlk.setHeader(&newBlkHeader);

    InternalEntry internalEntry;
    internalEntry.attrVal=attrVal;
    internalEntry.lChild=lChild;
    internalEntry.rChild=rChild;
    newRootBlk.setEntry(&internalEntry,0);

    // update the pblock of those blocks to `newRootBlkNum`
    BlockBuffer leftBlock(lChild);
    BlockBuffer rightBlock(rChild);

    HeadInfo leftHeader;
    HeadInfo rightHeader;

    leftBlock.getHeader(&leftHeader);
    rightBlock.getHeader(&rightHeader);
    leftHeader.pblock=newRootBlkNum;
    rightHeader.pblock=newRootBlkNum;
    leftBlock.setHeader(&leftHeader);
    rightBlock.setHeader(&rightHeader);
    
    // update rootBlock = newRootBlkNum for the entry corresponding to `attrName`in the attribute cache using AttrCacheTable::setAttrCatEntry().

    attrCatBuf.rootBlock=newRootBlkNum;
    AttrCacheTable::setAttrCatEntry(relId,attrName,&attrCatBuf);

    return SUCCESS;
}