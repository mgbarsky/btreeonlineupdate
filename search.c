#include "general.h"
/**
Performs search for the appropriate leaf of b-tree where a given word belongs
*/
extern unsigned int codetable[256];

//when called, check that the code returned is not 0
unsigned int getWordHash(char *word, int len) {
	int i;
	unsigned int wordHash=0;
	unsigned int code;

	for(i=0;i<len;i++)
	{
		code=codetable[(size_t)word[i]];
		if(code!=0)		{
			wordHash=(wordHash>>1)+code;
		}
		else
			return 0;
	}
	return wordHash;
}

int findWordHashInBTree(SystemState_t *state, unsigned int key,  int *totalDocs) {
	int i,counter=0;
	enum BOOL endOfSearch=FALSE;
	
	enum BOOL keyNotFound=FALSE;
	BTreeNode_t *leafNode;	
	
	int nextDocID;
	Data_t nextDoc;

	resetBTreePath(state);
	while (endOfSearch==FALSE)	{
		if(searchKeyDown(state,key))  //key not found
			endOfSearch=TRUE;
		else	{
			leafNode=state->lastPath[state->curTreeLevel];
			keyNotFound=FALSE;
			for(i=0;i<leafNode->header.keysCount && keyNotFound==FALSE;i++)		{
				if(leafNode->data[i].value==key)	{
					counter++;
					nextDocID=leafNode->data[leafNode->data[i].pointer].pointer;
					while(nextDocID!=0)	{
						nextDoc=leafNode->data[nextDocID];
						counter++;
						nextDocID=nextDoc.pointer;
					}					
				}
				else if(leafNode->data[i].value>key) {
					keyNotFound=TRUE;
					endOfSearch=TRUE;
				}
			}
			if(keyNotFound==FALSE) {//all keys in the node matched the query
				state->curTreeLevel--;
				if(searchKeyUp(state,key))
					endOfSearch=TRUE;
			}
		}
	}

	*totalDocs=counter;
	return 0;
}

int searchKeyDown(SystemState_t *state, unsigned int key) {
	int i;
	BTreeNode_t *currNode;
	BTreeNode_t *childNode=NULL;

	unsigned int childNodeID;

	if(state->lastPath[state->curTreeLevel]->header.nodeType==LEAF) { //reached the leaf	
		return 0;
	}
	
	currNode=state->lastPath[state->curTreeLevel];

	for(i=state->lastPathCurrentPointers[state->curTreeLevel];i<currNode->header.keysCount;i++)	{
		if(key<=currNode->data[i].value) {
			state->lastPathCurrentPointers[state->curTreeLevel]=i+1;  //to start the next search

			childNodeID=currNode->data[i].pointer;

			childNode=getNode(state, childNodeID );
			if(childNode==NULL) {
				printf ("Node with ID %u not found while searching down for key %u.\n",childNodeID, key);
				return RESULT_ERROR;
			}
			
			state->curTreeLevel++;
			
			state->lastPath[state->curTreeLevel]=childNode;
			state->lastPathCurrentPointers[state->curTreeLevel]=0;  

			return searchKeyDown(state,key);
		}
	}	
	
	return RESULT_ERROR;
}


int searchKeyUp(SystemState_t *state, unsigned int key) {

	BTreeNode_t *currNode;

	if(state->curTreeLevel==0) //reached the root
		return 0;
	
	currNode=state->lastPath[state->curTreeLevel];
	
	if(key<=currNode->data[state->lastPathCurrentPointers[state->curTreeLevel]].value 
		&& state->lastPathCurrentPointers[state->curTreeLevel]<currNode->header.keysCount) {  //belongs to this internal node
		return 0;//searchDown(state, key, keyLength);	
	}
	else {
		state->lastPathCurrentPointers[state->curTreeLevel]=0;
		state->curTreeLevel--;

		return searchKeyUp(state,key);
	}
}
