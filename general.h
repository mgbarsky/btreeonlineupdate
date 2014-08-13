#ifndef GENERAL_H
#define GENERAL_H
/*BUFFER
*******************
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>

#define MAX_PATH_LENGTH 250
#define MIN(a, b) ((a)<=(b) ? (a) : (b))
#define MAX(a, b) ((a)>=(b) ? (a) : (b))

//----------------execution results constants
#define RESULT_OK 0
#define RESULT_RETRY 2
#define RESULT_ERROR 1
#define RESULT_NOT_FOUND -1
#define RESULT_SKIP -2 //for debugging

// ENUMS

//----------------

//-----------general enums
enum BOOL {FALSE, TRUE};
//------------------

//-----------btree enums
enum node_t{ ROOT, INTERNAL,	LEAF};

//-----------------------

//-------------------

// STRUCTURES

//----------------------

//-------parser structures

int prepareCodeTable();
#define INPUT_BUFFER_MAX 2000000 //2 MB assumed the largest size of 1 document
#define SORTING_BUFFER_MAX 50000 // TBD check - seems 100000 is OK -check what is faster
//50000 ints are sorted and then merged, since otherwise stack overflow in quicksort
int extractWords(char *inputbuffer, int totalChars, unsigned int *hashedwords, int *totalwords);
int sortHashedWords(unsigned int *hashedwords,int totalwords, unsigned int *tempArray);
int removeDuplicates(unsigned int *hashedwords,int totalwords,int *distinctWords);
int prepareStopWordsSortedList();
int removeDuplicatesAndStopWords(unsigned int *hashedwords,int totalwords, int *distinctWords);

int testParsing(char *inputbuffer,int totalChars,unsigned int *hashedwords, int distinctWords);
//-------------------------

//--------btree structures
#define MAX_DATA_PER_NODE  5100 //for page of 4096 bytes: header size=16 bytes, each data entry is 8 bytes
#define MAX_TREE_HEIGHT 10
#define MAX_UNSIGNED_INT 4294967295

typedef struct
{
	short keysCount;  //2
	short dataFreePosArrID;  //2
	unsigned int nodeID;  //4
	enum node_t nodeType; //4
	unsigned int maxKey;  // 4 
}NodeHeader_t;

/*

	BTREE WITH BUFFER
**************************************

This is the main data structure for BTree nodes
value can be the key, pointer points to a position in data array where the first
document ID is stored (They filled from the end of the array)
Afterwards, each element in the array contains document ID where the key occurs,
in the value field, and in the pointer field it contains 
the position of the next document where the same key occurs. 0 means no more documents
-1 means that the keys exist in the next sibling node 
- there may be a lot of documents for the same key
this is needed mostly for search - not implemented for now
*/
typedef struct
{
	unsigned int value;  //4
	int pointer;  //4	
}Data_t;

typedef struct
{
	NodeHeader_t header;  //2
	Data_t data[MAX_DATA_PER_NODE];
}BTreeNode_t;






//----------memorypool structures
#define MAX_NODES_INMEM 10000

typedef struct
{
	unsigned int nodeID;	
	enum BOOL isOccupied;	
}InMemNodeInfo_t;

typedef struct
{		
	int currentFreePosition;	
	BTreeNode_t *nodes;
}MemoryPool_t;

typedef struct
{	
	unsigned int maxNodesOnDisk;
	BTreeNode_t * lastPath[MAX_TREE_HEIGHT];
	short lastPathCurrentPointers [MAX_TREE_HEIGHT];  //show position in the node afer current insertion 
	short curTreeLevel;
	FILE *btreefile;
	FILE *sizefile;
	InMemNodeInfo_t *memPoolPointers;
	MemoryPool_t *memPool;	
}SystemState_t;


int initMemoryPool(SystemState_t *state);
BTreeNode_t* createNewNode (SystemState_t *state, enum node_t node_type );
int flashNodeToDisk (SystemState_t *state, int arrPointersPos, enum BOOL setFree, enum BOOL isNew);
int getFreeSpotInBuffer(SystemState_t *state, int currFreePos);
enum BOOL isRecentlyUsed(SystemState_t *state, unsigned int nodeID);
BTreeNode_t* getNode(SystemState_t *state, unsigned int nodeID);
BTreeNode_t* loadNodeFromDisk (SystemState_t *state, unsigned int nodeID);
int finish_SynchronizeData(SystemState_t *state);

//----------Disk read-write diskaccess.c
unsigned int getBTreeSize(SystemState_t *state);
int moveInBTreeFile(FILE* btreefile, unsigned int nodeID);

//------------btree functions
int insertSortedKeyFromBuffer(SystemState_t *state, unsigned int  key, int documentID);
int findLeafToInsert(SystemState_t *state, unsigned int keyTobeInserted);
int searchUp(SystemState_t *state, unsigned int key);
int searchDown(SystemState_t *state, unsigned int key);
int splitLeaf(SystemState_t *state, unsigned int keyTobeInserted);
int updateLeafParentAfterSplit(SystemState_t *state, 
						    BTreeNode_t* newLeaf, unsigned int keyTobeInserted) ;
int splitRoot(SystemState_t *state, unsigned int keyTobeInserted, 
			  unsigned int keyForParentUpdate,  unsigned int childID);
int splitInternal(SystemState_t *state, unsigned int keyTobeInserted,  int nodeLevel , 
				 unsigned int keyForParentUpdate,  unsigned int childID);
int updateNodeParentAfterSplit(SystemState_t *state, 
						    int nodeLevel, BTreeNode_t* newNode, unsigned int keyTobeInserted);
int resetBTreePath(SystemState_t *state);
int checkCircularReference(BTreeNode_t *currentLeaf);
int invalidLeaf(BTreeNode_t *currLeaf);


//-------------key search
int findWordHashInBTree(SystemState_t *state, unsigned int key,  int *totalDocs);
int searchKeyDown(SystemState_t *state, unsigned int key);
int searchKeyUp(SystemState_t *state, unsigned int key);

//---------bitoperations
#define NUM_BITS_INUINT 32
int getBit(unsigned int *sequence,int pos);
void setBit(unsigned int *sequence,int pos);
void printBitSequence(unsigned int *bitseq);
void printBitSequenceToFile(FILE *file,unsigned int *bitseq);

int getLCP(unsigned int *first, unsigned int *second);
int getLCPwithBitsAfterLCP(unsigned int *first, unsigned int *second,char *bit1, char *bit2);

//----------dynamic buckets
//-----buckets structures
#define MAX_KEYS_PER_BUCKET 1280//about 1/10 of keys per BTree node
#define MAX_NUMBER_OF_BUCKETS 400//100 tbc
#define MAX_TOP_TREE_NODES 800 // always twice MAX_NUMBER_OF_BUCKETS

typedef struct
{
	int keysCount;
	int LCPinBits;
}BucketHeader_t;

typedef struct
{
	BucketHeader_t header;
	Data_t data[MAX_KEYS_PER_BUCKET];
}Bucket_t;

typedef struct
{
	int nodesCounter;
	int freeNodePos;  //when removing 1 bucket we can free 1 node
	int bucketsCounter;
	int freeBucketID;	
}TopTreeHeader_t;

typedef struct
{
	short children[2];
	short incomingEdgeLength;
}TopTreeNode_t;

typedef struct
{
	TopTreeNode_t *node;
	int nodeID; //TBC remove ???
	char whatChild; //0-for left, 1 for right
}TopTreeNodePointer_t;

typedef struct
{
	TopTreeHeader_t header;
	TopTreeNode_t nodes[MAX_TOP_TREE_NODES];
}TopTree_t;

typedef struct
{
	TopTree_t tree;
	Bucket_t *buckets;
	TopTreeNodePointer_t parentOfDeepestBucket;
	TopTreeNodePointer_t lastPath[NUM_BITS_INUINT];
	int currentTreeLevel;
}Buffer_t;


int initBuffer(Buffer_t *buffer);
int synchronizeBuffer(Buffer_t *buffer,SystemState_t *state );

int insertKeyIntoBuffer(unsigned int key, unsigned int docID, Buffer_t *buffer, SystemState_t *state);
int moveUpInTree(int *distanceFromRoot, Buffer_t *buffer, int lcp);
int findBucketForKey(unsigned int key, Buffer_t *buffer, SystemState_t *state);
int blindSearch(int *distanceFromRoot, Buffer_t* buffer, unsigned int key, SystemState_t *state);

int getFreeBucketID(Buffer_t *buffer, SystemState_t *state);
int getFreeNodeID(Buffer_t *buffer);


int transferOneBucketToBTree(Buffer_t *buffer,SystemState_t *state) ;
void traverseForDeepestInternalNode(Buffer_t *buffer, int distanceFromRoot, int maxLCP, TopTreeNode_t *currParent);

int addKeyToBucket(Buffer_t *buffer, int bucketID, unsigned int key, unsigned int docID);
int splitBucket(SystemState_t *state,Buffer_t *buffer,int bucketID, int parentDistanceFromRoot);



//----tests
int fprintBuffer(FILE *logfile, Buffer_t *buffer);
#endif
