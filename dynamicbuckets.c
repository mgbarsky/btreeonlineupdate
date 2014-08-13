#include "general.h"
/**
That is where the whole idea of dynamic buffer is implemented
Each word is first added to a in-mem buffer, organized as a set of buckets.

Each bucket is a leaf in a keyword tree.

Once it is full, the bucket can split (still in memory).

When the total number of buckets which can be held in memory is exhausted,
the fullest bucket where words share the longest prefix is detached and transferred in one batch update to the main B-tree index.
By the cost of almost one random disk I/O.
*/
extern int transfercounter;

//starting point
int insertKeyIntoBuffer(unsigned int key, unsigned int docID, 
                    Buffer_t *buffer, SystemState_t *state) {
	int bucketID;
	int res;

	bucketID=findBucketForKey(key,buffer,state);
	
	if(bucketID==RESULT_NOT_FOUND)	{
		printf("Could not find bucket for key %u \n",key);
		return 1;
	}

	
	res=addKeyToBucket(buffer,bucketID,key,docID);

	if(res==RESULT_ERROR)	{
		printf("Failed to insert key %u to bucket %d\n",key,bucketID);
		return 1;
	}	
	
	return RESULT_OK;
}


int initBuffer(Buffer_t *buffer) {
	Bucket_t *buckets;

    buckets=(Bucket_t*) calloc (MAX_NUMBER_OF_BUCKETS, sizeof(Bucket_t));
	if(buckets==NULL)	{
		printf("Failed to allocate memory for %d buckets\n",
			MAX_NUMBER_OF_BUCKETS);
		return RESULT_ERROR;
	}

	buffer->buckets=buckets;
	buffer->currentTreeLevel=0;
	buffer->tree.header.bucketsCounter=1;
	buffer->tree.header.freeBucketID=0;
	buffer->tree.header.nodesCounter=1;
	buffer->tree.header.freeNodePos=0;

	buffer->tree.nodes[0].children[0]=0; //root node
	buffer->tree.nodes[0].children[1]=0;
	
	buffer->tree.nodes[0].incomingEdgeLength=0;

	buffer->buckets[1].header.keysCount=0;
	buffer->buckets[1].header.LCPinBits=NUM_BITS_INUINT;
	
	return RESULT_OK;
}

int traverseAndWriteBucketsToBTree(TopTreeNode_t *parent,Buffer_t *buffer,
                                                    SystemState_t *state) {
	int i,bucketID;
	Bucket_t *bucket;
	if(parent->children[0]!=0)	{
		if(parent->children[0]<0)	{
			bucketID=-(parent->children[0]);
			bucket=&buffer->buckets[bucketID];

			for(i=0;i<bucket->header.keysCount;i++)	{
				if(insertSortedKeyFromBuffer(state, bucket->data[i].value,bucket->data[i].pointer))
					exit(1);				
			}
            transfercounter++;
		}
		else {//internal node		
			traverseAndWriteBucketsToBTree(&(buffer->tree.nodes[parent->children[0]]),buffer,state);
		}
	}

	if(parent->children[1]!=0) {
		if(parent->children[1]<0)	{
			bucketID=-(parent->children[1]);
			bucket=&buffer->buckets[bucketID];

			for(i=0;i<bucket->header.keysCount;i++)	{
				if(insertSortedKeyFromBuffer(state, bucket->data[i].value,
					bucket->data[i].pointer))
					exit(1);				
			}
            transfercounter++;
		}
		else {//internal node
			traverseAndWriteBucketsToBTree(&(buffer->tree.nodes[parent->children[1]]),buffer,state);
		}
	}
	
	return RESULT_OK;
}

// empty all buckets into BTree - by traversals
int synchronizeBuffer(Buffer_t *buffer,SystemState_t *state ) {
	int i,bucketID;
	Bucket_t *bucket;
	TopTreeNode_t *root=&(buffer->tree.nodes[0]);

	if(root->children[0]!=0) {
		if(root->children[0]<0)	{
			bucketID=-(root->children[0]);
			bucket=&buffer->buckets[bucketID];

			for(i=0;i<bucket->header.keysCount;i++)	{
				if(insertSortedKeyFromBuffer(state, bucket->data[i].value,bucket->data[i].pointer))
					exit(1);				
			}
            transfercounter++;
		}
		else { //internal node
			traverseAndWriteBucketsToBTree(&(buffer->tree.nodes[root->children[0]]),buffer,state);
		}
	}

	if(root->children[1]!=0) {
		if(root->children[1]<0)	{
			bucketID=-(root->children[1]);
			bucket=&buffer->buckets[bucketID];

			for(i=0;i<bucket->header.keysCount;i++)	{
				if(insertSortedKeyFromBuffer(state, bucket->data[i].value,bucket->data[i].pointer))
					exit(1);				
			}
            transfercounter++;
		}
		else { //internal node
			traverseAndWriteBucketsToBTree(&(buffer->tree.nodes[root->children[1]]),buffer,state);
		}
	}
	
	return RESULT_OK;
}

//searches blindly in the tree, following only 1 bit of the corresponding child (0,1 )
int blindSearch(int *distanceFromRoot, Buffer_t* buffer, unsigned int key, SystemState_t *state) {
	int childID=buffer->lastPath[buffer->currentTreeLevel].node->children[(size_t)buffer->lastPath[buffer->currentTreeLevel].whatChild];
	if(childID<0) //reached the bucket 
		return RESULT_OK;
	
	//child is an internal node	
	if(childID==0)
		return RESULT_ERROR;
	buffer->currentTreeLevel++;
	buffer->lastPath[buffer->currentTreeLevel].node=&buffer->tree.nodes[childID];
	buffer->lastPath[buffer->currentTreeLevel].nodeID=childID;

	*distanceFromRoot+=buffer->lastPath[buffer->currentTreeLevel].node->incomingEdgeLength;
	buffer->lastPath[buffer->currentTreeLevel].whatChild=getBit(&key,(*distanceFromRoot));
	return blindSearch(distanceFromRoot,buffer,key, state);
}


int findBucketForKey(unsigned int key, Buffer_t *buffer, SystemState_t *state) {
	int distanceFromRoot=0;
	int i;
	int ID;
	int lcp;
	int treenodeID;
	char bit1, bit2;
	Bucket_t *bucket;

	buffer->currentTreeLevel=0;

	buffer->lastPath[0].node=&buffer->tree.nodes[0];
	buffer->lastPath[0].nodeID=0;
	buffer->lastPath[0].whatChild=getBit(&key,0);

	//first time insertion to a new bucket as a child of a root
	if(buffer->tree.nodes[0].children[(size_t)buffer->lastPath[0].whatChild]==0) {
		ID=getFreeBucketID(buffer,state);
		buffer->tree.nodes[0].children[(size_t) buffer->lastPath[0].whatChild]=-ID;
		return ID;
	}

	//1. follow the path from the root blindly until the leaf is reached
	if(blindSearch(&distanceFromRoot, buffer, key, state)==RESULT_ERROR)
		return RESULT_ERROR;
	ID=buffer->lastPath[buffer->currentTreeLevel].node->children[(size_t)buffer->lastPath[buffer->currentTreeLevel].whatChild];
	
	if(ID>=0)
		return RESULT_ERROR;

	//verify that the bucket is correct
	bucket=&buffer->buckets[-ID];

	if(bucket->header.keysCount==0)  //this happens if the split was required higher in the tree in the prev step and created a new bucket for this specific key
		return -ID;
	
	lcp=getLCPwithBitsAfterLCP(&key,&(bucket->data[0].value),&bit1,&bit2);
	
	
	if(distanceFromRoot==0 || lcp>=distanceFromRoot+1) { //child of root node or belongs to this buffer
		//verify that the bucket has space
		if(bucket->header.keysCount<MAX_KEYS_PER_BUCKET)
			return -ID;
		else {
			if(buffer->tree.header.freeBucketID>0 
                || buffer->tree.header.bucketsCounter<MAX_NUMBER_OF_BUCKETS) {
				if(splitBucket(state, buffer,(-ID),distanceFromRoot)==RESULT_ERROR)	{
					printf("error splitting bucket\n");
					return RESULT_ERROR;
				}
			}
			else {
				if(transferOneBucketToBTree(buffer,state)==RESULT_ERROR) {
					printf("transfer bucket to btree failed\n");
					return RESULT_ERROR;
				}
			}
			return findBucketForKey(key, buffer, state);
		}
	}
	else { //verification failed, new split required
		if(moveUpInTree(&distanceFromRoot, buffer,lcp)==RESULT_ERROR) {
			printf("moveUpInTree failed\n");
			return RESULT_ERROR;
		}
			
		ID=getFreeBucketID(buffer,state);
		if(ID!=RESULT_NOT_FOUND) {
			buffer->buckets[ID].header.keysCount=0;
			
			//add internal node
			treenodeID=getFreeNodeID(buffer);
			buffer->lastPath[buffer->currentTreeLevel].node->incomingEdgeLength-=(distanceFromRoot-lcp);

			for(i=0;i<2;i++) {
				buffer->tree.nodes[treenodeID].children[i]=buffer->lastPath[buffer->currentTreeLevel].node->children[i];
				buffer->lastPath[buffer->currentTreeLevel].node->children[i]=0;
			}

			buffer->tree.nodes[treenodeID].incomingEdgeLength=distanceFromRoot-lcp;

			buffer->lastPath[buffer->currentTreeLevel].node->children[(size_t)bit1]=-ID;
			buffer->lastPath[buffer->currentTreeLevel].node->children[(size_t)bit2]=treenodeID;
			return ID;
		}
		else { //need to empty one bucket
			if(transferOneBucketToBTree(buffer,state)==RESULT_ERROR) {
				printf("transfer bucket to btree failed\n");
				return RESULT_ERROR;
			}
			return findBucketForKey(key, buffer, state);
		}		
	}	
}

//moves up in lastPath to find the split point for a given LCP obtained after verification
int moveUpInTree(int *distanceFromRoot, Buffer_t *buffer, int lcp) {
	if(lcp>(*distanceFromRoot - buffer->lastPath[buffer->currentTreeLevel].node->incomingEdgeLength))
		return RESULT_OK;
	if(lcp==(*distanceFromRoot - buffer->lastPath[buffer->currentTreeLevel].node->incomingEdgeLength))
		return RESULT_ERROR;

	*distanceFromRoot-=buffer->lastPath[buffer->currentTreeLevel].node->incomingEdgeLength;
	buffer->currentTreeLevel--;
	return moveUpInTree(distanceFromRoot, buffer,  lcp);
}

//returns free id, the next available id or -1 if the buffer is full
int getFreeBucketID(Buffer_t *buffer, SystemState_t *state) {
	int id;

	id=buffer->tree.header.freeBucketID;
	if(id>0) {
		buffer->buckets[id].header.keysCount=0;
		buffer->buckets[id].header.LCPinBits=NUM_BITS_INUINT;
		
		buffer->tree.header.freeBucketID=0;
		return id;
	}

	id=buffer->tree.header.bucketsCounter;
	if(id<MAX_NUMBER_OF_BUCKETS) {
		buffer->buckets[id].header.keysCount=0;
		buffer->buckets[id].header.LCPinBits=NUM_BITS_INUINT;
	
		buffer->tree.header.bucketsCounter++;
		return id;  
	}
	return RESULT_NOT_FOUND;
}

//returns position in an array of tree nodes
//in general, it cannot happen that there is no such position, since 
//when we free the bucket, we free the top tree nodes
int getFreeNodeID(Buffer_t *buffer) {
	int id;

	id=buffer->tree.header.freeNodePos;
	
	if(id>0) {
		buffer->tree.nodes[id].children[0]=0;
		buffer->tree.nodes[id].children[1]=0;
		
		buffer->tree.nodes[id].incomingEdgeLength=0;
		buffer->tree.header.freeNodePos=0;
		return id;
	}

		
	id=buffer->tree.header.nodesCounter;
	if(id<MAX_TOP_TREE_NODES)	{
		buffer->tree.nodes[id].children[0]=0;
		buffer->tree.nodes[id].children[1]=0;
		
		buffer->tree.nodes[id].incomingEdgeLength=0;
		
		buffer->tree.header.nodesCounter++;
		return id; 
	}
	return RESULT_NOT_FOUND;
}

int splitBucket(SystemState_t *state,Buffer_t *buffer,
                int bucketID, int parentDistanceFromRoot) {
	int i,j;
	Bucket_t *bucket;
	
	int lcp;
	int newBucketID;
	Bucket_t *newBucket;
	int bitAfterLCP;
	
	int newLCP, prevLCP;
	int tmp;

	TopTreeNode_t *parentNode;
	TopTreeNode_t *splitNode;
	int newNodeID;


	bucket=&buffer->buckets[bucketID];
	prevLCP=bucket->header.LCPinBits;
	
	//all keys in this bucket are equal - 
    //we transfer to BTree all except 1 - in order to not to change the tree yet
	if(bucket->header.LCPinBits==NUM_BITS_INUINT) {
		for(i=1;i<bucket->header.keysCount;i++)	{
			if(insertSortedKeyFromBuffer(state, bucket->data[i].value, 
                                            bucket->data[i].pointer)==RESULT_ERROR)	{
				printf("equal keys insertion failed during split\n");
				return RESULT_ERROR;
			}
		}

		bucket->header.keysCount=1;
		resetBTreePath(state);
		return RESULT_OK;
	}


	lcp=bucket->header.LCPinBits;
	newBucketID=getFreeBucketID(buffer, state);
	//cannot happen that there are no free buckets, since we tested it before calling split

	newBucket=&buffer->buckets[newBucketID];
	
	//we split based on the bit after LCP
	//it should be at least 1 key which differs by this bit, since 
	//othervise the LCP would be NUM_BITS_INUINT - all keys would have been equal
	bitAfterLCP=0;
		
	newLCP=NUM_BITS_INUINT; //set max possible

	for(i=0;i<bucket->header.keysCount && bitAfterLCP==0;i++)	{
		bitAfterLCP=getBit(&(bucket->data[i].value),lcp); //lcp is 1 bigger than the position
		if(bitAfterLCP!=0)	{
			i--;
		}
		else {
			newBucket->data[i]=bucket->data[i];
			(newBucket->header.keysCount)++;
		
			if(i>0)	{
				tmp=getLCP(&(newBucket->data[i-1].value),&(newBucket->data[i].value));					
				if(tmp<newLCP)
					newLCP=tmp;
			}
		}
	}
	
	newBucket->header.LCPinBits=newLCP;
	if(newLCP<=prevLCP)	{
		printf("Logic error - split bucket - new LCP %d <= prevLCP %d in new bucket\n",newLCP,prevLCP);
		exit(1);
	}
	
	//update with the new key order in the old bucket	
	newLCP=NUM_BITS_INUINT;	

	for(j=0;i<bucket->header.keysCount;i++,j++)	{
		bucket->data[j]=bucket->data[i];
		
		if(j>0)	{
			tmp=getLCP(&(bucket->data[j-1].value),&(bucket->data[j].value));
			
			if(tmp<newLCP)
				newLCP=tmp;
		}
	}
	bucket->header.keysCount=j;
		
	bucket->header.LCPinBits=newLCP;
	if(newLCP<=prevLCP)	{
		printf("Logic error - split bucket - new LCP %d <= prevLCP %d in old bucket\n",newLCP,prevLCP);
		exit(1);
	}

	// update tree - the path is still in the last path
	//we just need to split an edge which was leading to bucketID before	
	newNodeID=getFreeNodeID(buffer);
	splitNode=&buffer->tree.nodes[newNodeID];		
		
	parentNode=buffer->lastPath[buffer->currentTreeLevel].node;
	parentNode->children[(size_t)buffer->lastPath[buffer->currentTreeLevel].whatChild]=newNodeID;

	splitNode->incomingEdgeLength=prevLCP-parentDistanceFromRoot;
	splitNode->children[0]=-newBucketID;
	splitNode->children[1]=-bucketID;
	return RESULT_OK;
}

void traverseForDeepestInternalNode(Buffer_t *buffer, int distanceFromRoot, 
                                        int maxLCP, TopTreeNode_t *currParent) {
	TopTreeNode_t *currChild;

	if(currParent->children[0]<0 && currParent->children[1]<0)
		return;
	
	if(currParent->children[0]>0) {	
		currChild=&(buffer->tree.nodes[currParent->children[0]]);

		if(distanceFromRoot+currChild->incomingEdgeLength>maxLCP) {
			maxLCP=distanceFromRoot+currChild->incomingEdgeLength;
			buffer->parentOfDeepestBucket.node=currParent;
			buffer->parentOfDeepestBucket.whatChild=0;
		}
		distanceFromRoot+=currChild->incomingEdgeLength;
		traverseForDeepestInternalNode(buffer, distanceFromRoot, maxLCP, currChild);
	}

	if(currParent->children[1]>0) {	
		currChild=&(buffer->tree.nodes[currParent->children[1]]);

		if(distanceFromRoot+currChild->incomingEdgeLength>maxLCP) {
			maxLCP=distanceFromRoot+currChild->incomingEdgeLength;
			buffer->parentOfDeepestBucket.node=currParent;
			buffer->parentOfDeepestBucket.whatChild=1;
		}
		distanceFromRoot+=currChild->incomingEdgeLength;
		traverseForDeepestInternalNode(buffer, distanceFromRoot, maxLCP, currChild);
	}
}

int transferOneBucketToBTree(Buffer_t *buffer,SystemState_t *state)  { //also performs delete operation in the tree
	int i;
	int maxLCP=0;
	TopTreeNode_t *tree_root;
	int distanceFromRoot=0;
	TopTreeNode_t *parent;
	TopTreeNode_t *internalChild;
	Bucket_t *bucket;	
	int bucketID;	
	int remainingBucketID;	

	tree_root=&(buffer->tree.nodes[0]);	
	
	traverseForDeepestInternalNode(buffer,distanceFromRoot,maxLCP,tree_root);

	//after this the child of buffer->parentOfDeepestBucket contains the parent of some deepest internal node
	parent=buffer->parentOfDeepestBucket.node;
	internalChild=&(buffer->tree.nodes[parent->children[(size_t)buffer->parentOfDeepestBucket.whatChild]]);

	//both children of this internal node are buckets
	//since otherwise the maxlcp would go deeper
	
	if(buffer->buckets[-internalChild->children[0]].header.keysCount
                    > buffer->buckets[-internalChild->children[1]].header.keysCount) {
		bucketID=-(internalChild->children[0]);
		remainingBucketID=-(internalChild->children[1]);
	}
	else {
		bucketID=-(internalChild->children[1]);
		remainingBucketID=-(internalChild->children[0]);
	}

	bucket=&(buffer->buckets[bucketID]);
	
	for(i=0;i<bucket->header.keysCount;i++)	{
		if(insertSortedKeyFromBuffer(state, bucket->data[i].value, bucket->data[i].pointer)
                                                                                ==RESULT_ERROR)	{
			printf("Failed to insert keys from buffer during transferOneBuckettoBTree\n");
			return RESULT_ERROR;
		}
	}

	bucket->header.keysCount=0;
	resetBTreePath(state);
	buffer->tree.header.freeBucketID=bucketID;

	// update top tree - just remove internal node
	// at most 1 internal node is removed - his pos in the array can be reused
	buffer->tree.header.freeNodePos=parent->children[(size_t)buffer->parentOfDeepestBucket.whatChild];
	parent->children[(size_t)buffer->parentOfDeepestBucket.whatChild]=-remainingBucketID;	
    transfercounter++;
	return RESULT_OK;
}

int addKeyToBucket(Buffer_t *buffer, int bucketID, unsigned int key, unsigned int docID) {
	int i,j, newLCP;
	Bucket_t *bucket;
	
	bucket=&buffer->buckets[bucketID];
	
	for(i=0;i<bucket->header.keysCount;i++)	{
		if(bucket->data[i].value>=key)
		{
			newLCP=getLCP(&(bucket->data[i].value), &key);
			//move pointers
			for(j=bucket->header.keysCount;j>i;j--)	{
				bucket->data[j]=bucket->data[j-1];
			}
			//add key pointer to be stored
			bucket->data[i].value=key;
			bucket->data[i].pointer=docID;
		
			//update key count
			bucket->header.keysCount++;
			
			//update lcp
			if(newLCP<bucket->header.LCPinBits)
				bucket->header.LCPinBits=newLCP;
	
			return RESULT_OK;
		}
	}	

	//add to the end of the bucket
	bucket->data[i].value=key;
	bucket->data[i].pointer=docID;
	
	//update key count
	bucket->header.keysCount++;

	if(bucket->header.keysCount==1)
		bucket->header.LCPinBits=NUM_BITS_INUINT;
	else {
		newLCP=getLCP(&key, &(bucket->data[bucket->header.keysCount-2].value));
		if(newLCP<bucket->header.LCPinBits)
			bucket->header.LCPinBits=newLCP;
	}
	
	return RESULT_OK;
}


