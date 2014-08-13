#include "general.h"
/**
Simplistic proof-of-concept implementation of B-tree
This B-tree code was written because none of the existing open-source B-tree implementations perform 
batch update of B-tree.

We need batch update (inserting multiple keys into the same B-tree node at once)
in order to execute online update
*/

//inserts key from buffer
int insertSortedKeyFromBuffer(SystemState_t *state, unsigned int  key, int documentID)
{
	BTreeNode_t *currentLeaf;
	Data_t *leafData;

	short i,j;
	short nextDocPos;
	Data_t *nextDoc;

	//1. Searches for an appropriate leaf - the result of findleaf is that the 
	//appropriate leaf is in the last path at currtreelevel
	//the leaf can hold one more key, the split if needed is already performed
	if(findLeafToInsert(state, key))	{
		return 1;
	}
	
	currentLeaf=(state->lastPath[state->curTreeLevel]);
	leafData=&currentLeaf->data[0];

	for(i=state->lastPathCurrentPointers[state->curTreeLevel];
		i< currentLeaf->header.keysCount;i++)	{
		if(leafData[i].value>key)	{
			//shift
			for(j=currentLeaf->header.keysCount;j>i;j--) {
				leafData[j]=leafData[j-1];
			}

			//add data
			leafData[i].value=key;

			//the document id will be stored in the data array starting from the end backwards
			leafData[i].pointer=currentLeaf->header.dataFreePosArrID;
		
			leafData[currentLeaf->header.dataFreePosArrID].value=documentID;
			leafData[currentLeaf->header.dataFreePosArrID].pointer=0; //unique key - occurs only in this document
			

			(currentLeaf->header.dataFreePosArrID)--;

			//update position to start from in the next insert
			state->lastPathCurrentPointers[state->curTreeLevel]=i;
			
			(currentLeaf->header.keysCount)++;			
			return 0;			
		}

		if(leafData[i].value==key) { //the same key but a different document 		
			nextDoc=&leafData[leafData[i].pointer];
			nextDocPos=nextDoc->pointer;
		
			while(nextDocPos!=0)	{
				nextDoc=&leafData[nextDocPos];
				nextDocPos=nextDoc->pointer;			
			}

			nextDoc->pointer=currentLeaf->header.dataFreePosArrID;
		
			leafData[currentLeaf->header.dataFreePosArrID].value=documentID;
			leafData[currentLeaf->header.dataFreePosArrID].pointer=0; //end of chain of document ids
			
			(currentLeaf->header.dataFreePosArrID)--;
			//update position to start from in the next insert
			state->lastPathCurrentPointers[state->curTreeLevel]=i;			
			
			return 0;		
		}							
	}
	
	//if not inserted inside array - add to the end of the array - means the key is bigger than any key already added
	//but smaller than the artificial largest key of this leaf node
	leafData[i].value=key;

	//the document id will be stored in the data array starting from the end backwards
	leafData[i].pointer=currentLeaf->header.dataFreePosArrID;
	
	leafData[currentLeaf->header.dataFreePosArrID].value=documentID;
	leafData[currentLeaf->header.dataFreePosArrID].pointer=0; //unique key - occurs only in this document	

	(currentLeaf->header.dataFreePosArrID)--;

	//update position to start from in the next insert
	state->lastPathCurrentPointers[state->curTreeLevel]=i;
	
	(currentLeaf->header.keysCount)++;				
	return 0;
}


/*
If current node is a leaf - checks if it is enough space for a new key
and returns - leaf for insertion is at the end of lastPath
Else
finds a leaf, checks for space,
splits the leaf if needed
*/
int findLeafToInsert(SystemState_t *state, unsigned int keyTobeInserted)
{
	
	BTreeNode_t *leafNode;
	BTreeNode_t *rootNode;
	
	//current node will always be leaf or root
	//1. if current node is a leaf - 
	if(state->lastPath[state->curTreeLevel]->header.nodeType==LEAF)
	{
		leafNode=(state->lastPath[state->curTreeLevel]);
		if(keyTobeInserted<=leafNode->header.maxKey)
		{
			//check if the new key can be added (space) - we need at most 2 Data_t slots: 1 for key 1 for docID
			//key goes into slot keysCount, and docID in dataFreePosArrID
			if(leafNode->header.keysCount+1<leafNode->header.dataFreePosArrID-1)
			{
				return 0;
			}
			//and if no			
			
			//split leaf and return corresponding leaf  (pos in it is not set)  (in last path information)
			//all the settings of new current node and path and
			//positions in path pointers are set inside the splitLeaf
			return splitLeaf(state, keyTobeInserted);
						
		}

		//1a if the key does not belong to this leaf (reset its pointer
		//- go up path and find corresponding branch and down to leaf
		state->lastPathCurrentPointers[state->curTreeLevel]=0;
		state->curTreeLevel--;
		if(searchUp(state, keyTobeInserted))
			return 1;
		return findLeafToInsert(state,keyTobeInserted);
	}

	//2. if current node is internal (or root)- check its largest value
	//then do search down and return corresponding leaf
	if(state->curTreeLevel==0)	{
		rootNode=(state->lastPath[state->curTreeLevel]);		

		//1. first time insertion -  artifiacial key is added to the root and to the leaf
		if(rootNode->header.keysCount==0)	{
			rootNode->header.maxKey=MAX_UNSIGNED_INT;			
			leafNode=createNewNode(state, LEAF);
			
			rootNode->data[0].value=MAX_UNSIGNED_INT;
			rootNode->data[0].pointer=leafNode->header.nodeID;
			rootNode->header.keysCount=1;					
			
			leafNode->header.maxKey=MAX_UNSIGNED_INT;
		
			state->curTreeLevel=1;
			state->lastPath[state->curTreeLevel]=leafNode;
			state->lastPathCurrentPointers[state->curTreeLevel]=0;
			return findLeafToInsert(state,keyTobeInserted);
		}
		
		if(searchDown(state, keyTobeInserted))
			return 1;
		return findLeafToInsert(state,keyTobeInserted);

	}
	
	//if current node was not root and not leaf - it is error
	printf("Current node was not the root and not the leaf -which is impossible not in the first time not after the prev insertion\n");
	return 1;
}

//searches along last path until appropriate node is found or 
//root node which is always appropriate
//then continues search down till the corresponding leaf is located
int searchUp(SystemState_t *state, unsigned int key) {

	BTreeNode_t *currNode;

	if(state->curTreeLevel==0) //reached the root
		return searchDown(state, key);
	
	currNode=(state->lastPath[state->curTreeLevel]);
	
	if(key<=currNode->header.maxKey) {  //belongs to this internal node	
		return searchDown(state, key);	
	}
	else {
		state->lastPathCurrentPointers[state->curTreeLevel]=0;
		state->curTreeLevel--;

		return searchUp(state,key);
	}
}


//searches only inside internal nodes, returns a corresponding leaf as the lastPath[curTreeLevel]
int searchDown(SystemState_t *state, unsigned int key) {
	short i;
	BTreeNode_t *currNode;
	BTreeNode_t *childNode=NULL;

	unsigned int childNodeID;

	if(state->lastPath[state->curTreeLevel]->header.nodeType==LEAF) {//reached the leaf
		return 0;
	}
	
	currNode=(state->lastPath[state->curTreeLevel]);
	
	for(i=state->lastPathCurrentPointers[state->curTreeLevel];i<currNode->header.keysCount;i++)	{
		if(key <=currNode->data[i].value) {
			state->lastPathCurrentPointers[state->curTreeLevel]=i;			
			childNodeID=currNode->data[i].pointer;
			childNode=getNode(state, childNodeID );
			
			if(childNode==NULL)	{
				printf ("Node with ID %u not found while searching down for key %u.\n",childNodeID, key);
				return 1;
			}
			
			state->curTreeLevel++;
			
			state->lastPath[state->curTreeLevel]=childNode;
			state->lastPathCurrentPointers[state->curTreeLevel]=0;  
			return searchDown(state,key);
		}
	}
	
	printf ("Key %u not found in node %u while searching down\n",key, currNode->header.nodeID);
	return 1;
}

/*
splits leaf into 2 leaves
*/
int splitLeaf(SystemState_t *state, unsigned int keyTobeInserted) {
	short half;
	BTreeNode_t *oldLeaf;
	BTreeNode_t *newLeaf;
	Data_t currDoc;
	short nextDocArrPos;
	Data_t tempData[MAX_DATA_PER_NODE];
	short i;
	unsigned int key_unique;

	oldLeaf=state->lastPath[state->curTreeLevel];

	half=oldLeaf->header.keysCount/2;

	//1. create new leaf	
	newLeaf=createNewNode (state, LEAF );
	if(newLeaf==NULL)	{
		printf("Failed to create new leaf during leaf node split\n");
		return 1;
	}

	//copy data to the tempData array
	for(i=0;i<MAX_DATA_PER_NODE;i++)
		tempData[i]=oldLeaf->data[i];

	if(half>0)	{
		// new leaf in the first half since we dont want to update the max key value in  the old leaf
		// 2. set max key data of the new leaf
		newLeaf->header.maxKey=oldLeaf->data[half-1].value;	

		//3. copy keys and docs to the new leaf 
		for(i=0;i<half;i++)	{
			//copy key data 
			newLeaf->data[i].value=oldLeaf->data[i].value;
			newLeaf->data[i].pointer=newLeaf->header.dataFreePosArrID;
	
			//copy docs list
			currDoc=oldLeaf->data[oldLeaf->data[i].pointer];
			newLeaf->data[newLeaf->header.dataFreePosArrID].value=currDoc.value;
		
			nextDocArrPos=currDoc.pointer;
	
			while (nextDocArrPos!=0)	{
				newLeaf->data[newLeaf->header.dataFreePosArrID].pointer=(newLeaf->header.dataFreePosArrID-1);
		
				currDoc=oldLeaf->data[nextDocArrPos];
				newLeaf->header.dataFreePosArrID--;
				newLeaf->data[newLeaf->header.dataFreePosArrID].value=currDoc.value;
				nextDocArrPos=currDoc.pointer;
		
			}
			newLeaf->data[newLeaf->header.dataFreePosArrID].pointer=0;
			newLeaf->header.dataFreePosArrID--;		
		}
	
		newLeaf->header.keysCount=i;

		//4. update keys in the old leaf. The maxkey remains with no change	
		oldLeaf->header.dataFreePosArrID=MAX_DATA_PER_NODE-1;

		for(i=half;i<oldLeaf->header.keysCount;i++)	{
			oldLeaf->data[i-half].value=tempData[i].value;
			oldLeaf->data[i-half].pointer=oldLeaf->header.dataFreePosArrID;
	
			//copy docs list
			currDoc=tempData[tempData[i].pointer];
			oldLeaf->data[oldLeaf->header.dataFreePosArrID].value=currDoc.value;
		
			nextDocArrPos=currDoc.pointer;
	
			while (nextDocArrPos!=0)	{
				oldLeaf->data[oldLeaf->header.dataFreePosArrID].pointer=(oldLeaf->header.dataFreePosArrID-1);
				currDoc=tempData[nextDocArrPos];
				oldLeaf->header.dataFreePosArrID--;
				oldLeaf->data[oldLeaf->header.dataFreePosArrID].value=currDoc.value;
				nextDocArrPos=currDoc.pointer;
		
			}
			oldLeaf->data[oldLeaf->header.dataFreePosArrID].pointer=0;
			oldLeaf->header.dataFreePosArrID--;	
		}

		oldLeaf->header.keysCount=i-half;	
	}

	else //the oldLeaf contains only 1 key, the rest is the list of documents
		//the key is only here: oldLeaf->data[0].value
	{
		key_unique=oldLeaf->data[0].value;
		
		half=MAX_DATA_PER_NODE/2;
		// 2. set max key data of the new leaf -
		newLeaf->header.maxKey=key_unique;	
		newLeaf->data[0].value=key_unique;
		newLeaf->data[0].pointer=newLeaf->header.dataFreePosArrID;
		//3. copy docs to the new leaf 		
		currDoc=oldLeaf->data[oldLeaf->data[0].pointer];
		i=1;

		//copy docs list (half of it)
		while(i<half)	{			
			i++;
			newLeaf->data[newLeaf->header.dataFreePosArrID].value=currDoc.value;			
			if(i<half)
				newLeaf->data[newLeaf->header.dataFreePosArrID].pointer
					=newLeaf->header.dataFreePosArrID-1;
			else
				newLeaf->data[newLeaf->header.dataFreePosArrID].pointer=0;
					
			nextDocArrPos=currDoc.pointer;
						
			currDoc=oldLeaf->data[nextDocArrPos];
			newLeaf->header.dataFreePosArrID--;				
		}
		
		newLeaf->header.keysCount=1;

		//4. update keys in the old leaf. The maxkey remains with no change	
		oldLeaf->header.dataFreePosArrID=MAX_DATA_PER_NODE-1;
		//oldLeaf->data[0].value remains the same
		oldLeaf->data[0].pointer=oldLeaf->header.dataFreePosArrID;

		oldLeaf->data[oldLeaf->header.dataFreePosArrID].value=currDoc.value;
		oldLeaf->data[oldLeaf->header.dataFreePosArrID].pointer
				=oldLeaf->header.dataFreePosArrID-1;
		oldLeaf->header.dataFreePosArrID--;
		
		nextDocArrPos=currDoc.pointer;
		while(nextDocArrPos!=0)	{
			currDoc=tempData[nextDocArrPos];
			oldLeaf->data[oldLeaf->header.dataFreePosArrID].value=currDoc.value;
			if(currDoc.pointer!=0)	{
				oldLeaf->data[oldLeaf->header.dataFreePosArrID].pointer
					=oldLeaf->header.dataFreePosArrID-1;				
			}
			else
				oldLeaf->data[oldLeaf->header.dataFreePosArrID].pointer=0;	
			oldLeaf->header.dataFreePosArrID--;	
			nextDocArrPos=currDoc.pointer;
		}

		oldLeaf->header.keysCount=1;	
	}
	//5. Depending on where the current key falls to - set currentNode at the end of the current path
	//position in current node after split is reset to 0
	state->lastPathCurrentPointers[state->curTreeLevel]=0;

	if(keyTobeInserted<=newLeaf->header.maxKey)
		state->lastPath[state->curTreeLevel]=newLeaf;

	//6. updates parent ponters in lastPath nodes
	if(updateLeafParentAfterSplit(state,   newLeaf,  keyTobeInserted))
		return 1;

	return 0;
}

/*
We need to update parent node with the link to a newly created leaf
*/
int updateLeafParentAfterSplit(SystemState_t *state, 
						    BTreeNode_t* newLeaf, unsigned int keyTobeInserted)  {	
	short i,j;
	BTreeNode_t *parentNode;
	
	parentNode=state->lastPath[state->curTreeLevel -1];

	//1. check if there is enough space to insert the new max value of a new child leaf
	if(parentNode->header.keysCount<MAX_DATA_PER_NODE)	{
		for(j=0;j<parentNode->header.keysCount;j++)  //new key is always smaller than one of old keys, since we create new leaf from the smallest half
		{
			if(keyTobeInserted<=parentNode->data[j].value) {
			//shift to insert new value before the current pointer of this node			
				for(i=parentNode->header.keysCount;i>j;i--)
				{
					parentNode->data[i]=parentNode->data[i-1];
				}
				
				parentNode->data[j].value=newLeaf->header.maxKey;
				parentNode->data[j].pointer=newLeaf->header.nodeID;
			
				(parentNode->header.keysCount)++;

				//we reset the position to start the search here also for safety
				state->lastPathCurrentPointers[state->curTreeLevel -1]=0;				
			
				return 0;
			}
		}
		printf("Logic error. The key to be inserted from a new leaf is not smaller than any existing key in the parent node.\n");
		return 1;
	}
	
	//new key cannot be added - requires node split
	//during the split the key of a new leaf is also inserted
	if(parentNode->header.nodeType==ROOT)	{		
		splitRoot(state, keyTobeInserted,newLeaf->header.maxKey, newLeaf->header.nodeID);		
	}
	else	{
		splitInternal(state, keyTobeInserted, (short)(state->curTreeLevel-1),newLeaf->header.maxKey, newLeaf->header.nodeID);		
	}

	return 0;
}

/*
splits root, shifts everything in lastPath
*/
int splitRoot(SystemState_t *state, unsigned int keyTobeInserted, 
			  unsigned int keyForParentUpdate,  unsigned int childID) {
	//create 2 new nodes - shift path nodes by 1. currentLevel increases
	short half;
	BTreeNode_t *leftNode;
	BTreeNode_t *rightNode;
	BTreeNode_t *rootNode;
	
	short i,j;	
	enum BOOL inserted=FALSE;

	rootNode=state->lastPath[0];

	half=rootNode->header.keysCount/2;
	//split the node in half (by number of keys)
	
	//1. create new leaft child
	leftNode=createNewNode (state, INTERNAL );
	if(leftNode==NULL)	{
		printf("failed to create new node as a left child of a splitting root\n");
		return 1;
	}

	// 2. set max key of the left child
	leftNode->header.maxKey=rootNode->data[half-1].value;
	
	//3. copy half of the data
	for(i=0;i<half;i++)	{
		leftNode->data[i]=rootNode->data[i];	
	}

	leftNode->header.keysCount=i;
	
	//4. create new right child
	rightNode= createNewNode (state, INTERNAL );
	if(rightNode==NULL)	{
		printf("failed to create new node as a right child of a splitting root\n");
		return 1;
	}

	// 5. set max key of the right child
	rightNode->header.maxKey=rootNode->header.maxKey;

	//6. copy second half of data	
	for(i=half;i<rootNode->header.keysCount;i++) {
		rightNode->data[i-half]=rootNode->data[i];	
	}

	rightNode->header.keysCount=i-half;

	//6. Reset root node anew with 2 pointers to left and right child
	//the max key remains the same
	//set pointer to the left child
	rootNode->data[0].value=leftNode->header.maxKey;
	rootNode->data[0].pointer=leftNode->header.nodeID;
	
	rootNode->data[1].value=rightNode->header.maxKey;
	rootNode->data[1].pointer=rightNode->header.nodeID;

	rootNode->header.keysCount=2;
	
	//7. shift lastpath (except root) and lastpath pointers
	for(i=state->curTreeLevel+1;i>1;i--) {
		state->lastPath[i]=state->lastPath[i-1];
		state->lastPathCurrentPointers[i]=0;
	}
	state->lastPath[0]=rootNode;
	state->lastPathCurrentPointers[0]=0;

	//5. Split was required to insert some key (the max key of the splitted child
	if(keyForParentUpdate<=leftNode->header.maxKey)	{
		for(i=0;i<leftNode->header.keysCount && inserted==FALSE;i++)	{
			if(keyForParentUpdate <= leftNode->data[i].value)	{
				//shift and insert
				for(j=leftNode->header.keysCount;j>i;j--)	{
					leftNode->data[j]=leftNode->data[j-1];
				}
				leftNode->data[i].value=keyForParentUpdate;
				leftNode->data[i].pointer=childID;

				leftNode->header.keysCount++;
				inserted=TRUE;
			}		
		}
		state->lastPath[1]=leftNode;
	}
	else //new child pointer goes to the right node
	{
		for(i=0;i<rightNode->header.keysCount && inserted==FALSE;i++)	{
			if(keyForParentUpdate <= rightNode->data[i].value)	{
				//shift and insert
				for(j=rightNode->header.keysCount;j>i;j--)	{
					rightNode->data[j]=rightNode->data[j-1];
				}
				
				rightNode->data[i].value=keyForParentUpdate;
				rightNode->data[i].pointer=childID;	

				rightNode->header.keysCount++;
				inserted=TRUE;
			}		
		}
		state->lastPath[1]=rightNode;
	}

	state->lastPathCurrentPointers[1]=0;
	state->curTreeLevel++;
	return 0;
}

/*
splits internal node
nodelevel is a level of a node to split
can be called recursively to split parents
*/
int splitInternal(SystemState_t *state, unsigned int keyTobeInserted,  int nodeLevel , 
				 unsigned int keyForParentUpdate,  unsigned int childID) {
	short half;
	BTreeNode_t *oldNode;
	BTreeNode_t *newNode;
	short i,j;

	enum BOOL inserted=FALSE;

	oldNode=state->lastPath[nodeLevel];
	
	half=oldNode->header.keysCount/2;
	
	//2. create new node
	newNode=createNewNode (state, INTERNAL );
	if(newNode==NULL)	{
		printf("failed to create new node during internal node split \n");
		return 1;
	}

	// new node in the first half since we dont want to update the max key value in the old node
	// 3. set max key of a new leaf
	newNode->header.maxKey=oldNode->data[half-1].value;
	
	for(i=0;i<half;i++)	{
		newNode->data[i]=oldNode->data[i];	
	}
	newNode->header.keysCount=i;

	for(i=half;i<oldNode->header.keysCount;i++)	{
		//copy key data pointers
		oldNode->data[i-half]=oldNode->data[i];	
	}

	oldNode->header.keysCount=i-half;

	//5. Split was required to insert some key (the max key of the splitted child
	if(keyForParentUpdate<=newNode->header.maxKey)	{
		for(i=0;i<newNode->header.keysCount && inserted==FALSE;i++)	{
			if(keyForParentUpdate<=newNode->data[i].value)	{
				//shift and insert
				for(j=newNode->header.keysCount;j>i;j--) {
					newNode->data[j]=newNode->data[j-1];
				}
				newNode->data[i].value=keyForParentUpdate;
				newNode->data[i].pointer=childID;

				newNode->header.keysCount++;
				inserted=TRUE;
			}		
		}
		state->lastPath[nodeLevel]=newNode;
	}
	else //new child pointer goes to the old node
	{
		for(i=0;i<oldNode->header.keysCount && inserted==FALSE;i++)	{
			if(keyForParentUpdate<=oldNode->data[i].value)	{
				//shift and insert
				for(j=oldNode->header.keysCount;j>i;j--)	{
					oldNode->data[j]=oldNode->data[j-1];
				}
				oldNode->data[i].value=keyForParentUpdate;
				oldNode->data[i].pointer=childID;
			
				oldNode->header.keysCount++;
				inserted=TRUE;
			}		
		}
		state->lastPath[nodeLevel]=oldNode;
	}

	//5. reset search start
	state->lastPathCurrentPointers[nodeLevel]=0;

	//after splitting 
	//updates parent pointers in lastPath nodes
	if(updateNodeParentAfterSplit(state,  nodeLevel ,newNode, keyTobeInserted))
		return 1;

	return 0;
}


int updateNodeParentAfterSplit(SystemState_t *state, 
						    int nodeLevel, BTreeNode_t* newNode, unsigned int keyTobeInserted) {
	short i,j;
	BTreeNode_t *parentNode;
	parentNode=state->lastPath[nodeLevel -1];	
	
	//check if it can accomodate one more key
	if(parentNode->header.keysCount<MAX_DATA_PER_NODE)	{		
		for(j=0;j<parentNode->header.keysCount;j++)	{
			if(newNode->header.maxKey<=parentNode->data[j].value)	{
				//shift to insert new value before the current pointer of this node
				for(i=parentNode->header.keysCount;i>j;i--)		{
					parentNode->data[i]=parentNode->data[i-1];
				}
			
				parentNode->data[j].value=newNode->header.maxKey;
				parentNode->data[j].pointer=newNode->header.nodeID;
			
				parentNode->header.keysCount++;
				state->lastPathCurrentPointers[nodeLevel -1]=0;					
				return 0;		
			}
		}
	}

	//new key cannot be added - requires node split
	if(parentNode->header.nodeType==ROOT) {
		return splitRoot(state, keyTobeInserted,newNode->header.maxKey, newNode->header.nodeID);
	}
	else {
		return splitInternal(state, keyTobeInserted,nodeLevel-1, 
			newNode->header.maxKey, newNode->header.nodeID);
	}

	return 0;
}

int resetBTreePath(SystemState_t *state) {
	short i;

	for(i=0;i<=state->curTreeLevel;i++)
		state->lastPathCurrentPointers[i]=0;
	state->curTreeLevel=0;
	
	return 0;
}

int checkCircularReference(BTreeNode_t *currentLeaf) {	
	int i;
	for(i=0;i<currentLeaf->header.keysCount;i++) {
		if(currentLeaf->data[i].pointer==i)	{
			printf("Circular reference 1\n");
			return 1;
		}
	}

	for(i=MAX_DATA_PER_NODE-1;i>currentLeaf->header.dataFreePosArrID;i--) {
		if(currentLeaf->data[i].pointer==i)	{
			printf("Circular reference 2\n");
			return 1;
		}
	}

	if(currentLeaf->header.keysCount>=currentLeaf->header.dataFreePosArrID)	{
		printf("Keys and docs overlap\n");
		return 1;
	}
	
	return 0;
}

int invalidLeaf(BTreeNode_t *currLeaf) {
	int i;
	for(i=0;i<currLeaf->header.keysCount;i++)	{
		if(currLeaf->data[i].value==0)	{
			printf("Zero key\n");
			return 1;
		}
		if(currLeaf->data[i].pointer==0)	{
			printf("Zero pointer to doc\n");
			return 1;
		}
	}

	for(i=MAX_DATA_PER_NODE-1;i>currLeaf->header.dataFreePosArrID;i--)	{
		if(currLeaf->data[i].value<=0)	{
			printf("Invalid (zero) docID\n");
			return 1;
		}
	}
	return 0;
}