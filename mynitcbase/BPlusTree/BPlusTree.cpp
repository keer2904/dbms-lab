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
