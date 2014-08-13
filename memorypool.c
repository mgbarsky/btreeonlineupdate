#include "general.h"

/*
This routine initializes the memory pool buffer
It allocates memory for an array of Nodes
and for an array of node pointers
Then it loads all the nodes into a memory in case that the size of on-disk B-tree
is less than MAX_NODES_INMEM
Otherwise it loads only root node, since we cannot predict what nodes will be used first
If btree does not contain any nodes, it creates a root node.
In any case it assigns the root node
to the lastPath [0]
*/
int initMemoryPool(SystemState_t *state)
{
	unsigned int nodesInFile,i;
	int res;
	MemoryPool_t *memPool;
	InMemNodeInfo_t *memPoolPointers;
	BTreeNode_t* node;

	//1. Allocate memory for memory pool pointer
	memPool=(MemoryPool_t *) calloc (1, sizeof(MemoryPool_t));
	if(memPool==NULL)
	{
		printf("Failed to allocate memory for Memory pool of size: %lu \n",
			(sizeof(MemoryPool_t)));
		return 1;
	}

	// 2. allocate memory for BTree nodes array 
	memPool->nodes=(BTreeNode_t *) calloc (MAX_NODES_INMEM, sizeof(BTreeNode_t));
	if(memPool->nodes==NULL)
	{
		printf("Failed to allocate memory for BTreeNode_t memory pool of size: %d \n",
			MAX_NODES_INMEM);
		return 1;
	}
	
	//3. Allocate memory for node pointers array
	memPoolPointers=(InMemNodeInfo_t *) calloc (MAX_NODES_INMEM, sizeof(InMemNodeInfo_t));
	if(memPoolPointers==NULL)
	{
		printf("Failed to allocate memory for InMemNodeInfo_t memory pool pointers of size: %d \n",
			MAX_NODES_INMEM);
		return 1;
	}

	//5. determine the number of nodes on disk
	nodesInFile=getBTreeSize(state);

	//6. set state fields pointers at the allocated arrays
	state->memPool=memPool;
//	memory pool is empty, Current free position is 0
	state->memPool->currentFreePosition=0;
	state->memPoolPointers=memPoolPointers;
	state->maxNodesOnDisk=nodesInFile;

	//7. Depending on the number of nodes in btree file
	//7a. No nodes - create root node
	if(nodesInFile==0)   
	{
		//it creates and makes it persistent - writes to disk
		//the last created new Node is always in state->bufferNode
		node=createNewNode(state, ROOT); 

		memPoolPointers[0].nodeID=0;
	
		memPoolPointers[0].isOccupied=TRUE;
		state->maxNodesOnDisk++;
		state->curTreeLevel=0;
		state->lastPath[0]=node;

		state->memPool->currentFreePosition++;		
		return 0;
	}

	//7b. If nodes fit, load them all
	if(nodesInFile<MAX_NODES_INMEM)   
	{
		res=fread(&state->memPool->nodes[0],sizeof(BTreeNode_t),nodesInFile,state->btreefile);
		rewind(state->btreefile);
		if((unsigned int)res!=nodesInFile)
		{
			printf("Error reading Btree nodes from file: supposed to read %u and read %d \n",nodesInFile,res);
			return 1;
		}
		
		for(i=0;i<nodesInFile;i++)
		{
			memPoolPointers[i].nodeID=state->memPool->nodes[i].header.nodeID;
			memPoolPointers[i].isOccupied=TRUE;
			
		}		

		state->curTreeLevel=0;
		state->lastPath[0]=&(state->memPool->nodes[0]);
		state->memPool->currentFreePosition=nodesInFile;
		
		printf("There were %d nodes in an existing BTree file\n",nodesInFile);
		state->lastPathCurrentPointers[0]=0;
		return 0;
	}

	//7c. The general situation when file is big so we load only the root node
	res=fread(&state->memPool->nodes[0],sizeof(BTreeNode_t),1,state->btreefile);
	rewind(state->btreefile);
	if(res!=1)
	{
		printf("Error reading Btree root from file\n");
		return 1;
	}

	memPoolPointers[0].nodeID=0;
	memPoolPointers[0].isOccupied=TRUE;

	state->curTreeLevel=0;
	state->lastPath[0]=&(state->memPool->nodes[0]);
	state->memPool->currentFreePosition=1;	
	state->lastPathCurrentPointers[0]=0;
	
	return 0;
}


/*This routine creates a new TreeNode of specified type,
it makes it persistent - writes to file
then it puts it into a free spot in memory pool and sets
the bufferNode pointer to it
*/
BTreeNode_t* createNewNode (SystemState_t *state, enum node_t node_type )
{
	int currFreePos=state->memPool->currentFreePosition;
	int newFreePos;

	if(currFreePos<0 || currFreePos>MAX_NODES_INMEM)
	{
		printf("INVALID current free position in mem pool buffer\n");
		exit(1);
	}
	if(node_type==ROOT) // happens only once - when there is no root node
	{
		//clear all fields of the nodes array at pos 0
		state->memPoolPointers[0].nodeID=0; //root node
		state->memPoolPointers[0].isOccupied=TRUE;

		state->memPool->nodes[0].header.keysCount=0;
		state->memPool->nodes[0].header.dataFreePosArrID=MAX_DATA_PER_NODE-1;
		state->memPool->nodes[0].header.nodeID=0;
		state->memPool->nodes[0].header.nodeType=ROOT;
			
		flashNodeToDisk(state,0, FALSE, TRUE);	

		state->curTreeLevel=0;
		state->lastPath[0]=&(state->memPool->nodes[0]);
		state->lastPathCurrentPointers[state->curTreeLevel]=0;
	
		return &state->memPool->nodes[0];
	}

	
	newFreePos=getFreeSpotInBuffer(state,  currFreePos);
	
	if(newFreePos==RESULT_NOT_FOUND)
		return NULL;

	state->memPoolPointers[newFreePos].nodeID=(state->maxNodesOnDisk)++; //next ID	
	
	state->memPool->nodes[newFreePos].header.keysCount=0;
	state->memPool->nodes[newFreePos].header.dataFreePosArrID=MAX_DATA_PER_NODE-1;
	state->memPool->nodes[newFreePos].header.nodeID=state->memPoolPointers[newFreePos].nodeID;
	state->memPool->nodes[newFreePos].header.nodeType=node_type;

	state->memPoolPointers[newFreePos].isOccupied=TRUE;
	state->memPoolPointers[newFreePos].nodeID=state->memPoolPointers[newFreePos].nodeID;
	flashNodeToDisk(state,newFreePos, FALSE, TRUE);
	
	state->memPool->currentFreePosition=newFreePos+1; 
	return &(state->memPool->nodes[newFreePos]);
}


/*
This routine writes to btree file the node which is in memPool at position arrPointersPos
*/
int flashNodeToDisk (SystemState_t *state, int arrPointersPos, 
					 enum BOOL setFree, enum BOOL isNew)
{
	unsigned int nodeID=state->memPool->nodes[arrPointersPos].header.nodeID;
	int res;

	if(isNew==TRUE)
	{
		fseek(state->btreefile, 0, SEEK_END);
	}

	else
	{
		if(moveInBTreeFile(state->btreefile,nodeID))
		{
			printf("error finding position in BTree file for node writing\n");
			return RESULT_ERROR;
		}
	}


	res=fwrite(&(state->memPool->nodes[arrPointersPos]), sizeof (BTreeNode_t), 1, state->btreefile);
	if(res!=1)
	{
		printf("failed to write BTree node %u to file\n",nodeID);
		return RESULT_ERROR;
	}

	rewind(state->btreefile);
	if(setFree==TRUE)
	{
		state->memPool->currentFreePosition=arrPointersPos;
		if(arrPointersPos<0 || arrPointersPos>=MAX_NODES_INMEM)
		{
			printf("Flushed to disk a node from an invalid position in buffer\n");
			exit(1);
		}
		state->memPoolPointers[arrPointersPos].isOccupied=FALSE;
	}

	return 0;
}


/*This finds the next available spot in memory buffer 
The buffer is searched as a circular array
if node is not in use - was flushed and setfree - the position is returned
if node is in use, we check if it was used in the lastPath - which we keep track
of the lastPath to the last accessed leaf node.
If it was used, we try the next spot.
When at the end of an memPool array - we start from 1 - 
0 spot is reserved to the root node which is always in memory
*/
int getFreeSpotInBuffer(SystemState_t *state, int currFreePos)
{
	int i;
	
	for(i=currFreePos;i<MAX_NODES_INMEM;i++)
	{
		if(state->memPoolPointers[i].isOccupied==FALSE)
			return i;
		if(isRecentlyUsed(state,state->memPoolPointers[i].nodeID)==FALSE)
		{
			flashNodeToDisk(state,i, TRUE, FALSE);
			return i;
		}
	}

	for(i=1;i<currFreePos;i++)
	{
		if(state->memPoolPointers[i].isOccupied==FALSE)
			return i;
		if(isRecentlyUsed(state,state->memPoolPointers[i].nodeID)==FALSE)
		{
			flashNodeToDisk(state,i, TRUE, FALSE);
			return i;
		}
	}

	printf("Failed to find free spot in memory pool buffer\n");
	return RESULT_NOT_FOUND;;  //error - no free spot has been found
}


/*keep the node in memory pool if it was recently used and stored in lastPath*/
enum BOOL isRecentlyUsed(SystemState_t *state, unsigned int nodeID)
{	
	int i;
	for (i=0;i<=state->curTreeLevel;i++)
	{
		if(state->lastPath[i]->header.nodeID==nodeID)
			return TRUE;
	}
	return FALSE;
}


/*This routine returns the handler to the node with node id=nodeID 
It first tries to find it in memory pool, and if not found, loads it from disk
and puts into a free spot in memory pool
*/
BTreeNode_t* getNode(SystemState_t *state, unsigned int nodeID)
{
	// gets handler to a node by its ID
	//first looks into state->memPoolPointers
	//if not found - loadNodeFromDisk
	int i;
	for(i=0;i<MAX_NODES_INMEM;i++)
	{
		if(state->memPoolPointers[i].nodeID==nodeID)
		{
			return &(state->memPool->nodes[i]);
		}
	}

	//if not in mem - load from disk
	return loadNodeFromDisk (state, nodeID);
}


/*Loads node from disk into free spot 
Moves in btree file nodeID positions, nodeID corresponds to the position of a node in BTree file
*/
BTreeNode_t* loadNodeFromDisk (SystemState_t *state, unsigned int nodeID)
{
	int res;
	//we are  loading node into free spot 
	int currFreePos=state->memPool->currentFreePosition;
	int newFreePos;
	

	if(currFreePos<0 || currFreePos>MAX_NODES_INMEM)
	{
		printf("Invalid current free spot position in mem pool buffer\n");
		exit(1);
	}

	newFreePos=getFreeSpotInBuffer(state, currFreePos);
	
	//we read node nodeID into free spot
	rewind(state->btreefile);
	if(moveInBTreeFile(state->btreefile,nodeID))
	{
		printf("error finding position in BTree file for node reading\n");
		return NULL;
	}

	res=fread(&state->memPool->nodes[newFreePos],sizeof(BTreeNode_t),1,state->btreefile);
	if(res!=1)
	{
		printf("error reading node %u in BTree file\n",nodeID);
		return NULL;
	}
	
	rewind(state->btreefile);
	state->memPoolPointers[newFreePos].nodeID=nodeID;
	state->memPoolPointers[newFreePos].isOccupied=TRUE;

	state->memPool->currentFreePosition=newFreePos+1;
	return &state->memPool->nodes[newFreePos];
}


/*
writes in-memory nodes which labeled as occupied 
	(they may have undergone changes since they were read) to disk
	*/
int finish_SynchronizeData(SystemState_t *state)
{	
	int i;
	for(i=0;i<MAX_NODES_INMEM;i++)
	{
		if(state->memPoolPointers[i].isOccupied==TRUE)
		{
			if(flashNodeToDisk (state, i, TRUE, FALSE))
				return RESULT_ERROR;
		}
	}
	return 0;
}
