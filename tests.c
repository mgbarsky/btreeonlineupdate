#include "general.h"

extern unsigned int codetable[256];

int testParsing(char *inputbuffer,int totalChars,unsigned int *hashedwords, int distinctWords) {
	int i=0;
	int j;
	unsigned int wordHash=0;
	int code;
	int found=0;

	for(i=0;i<totalChars;i++)
	{
		code=codetable[inputbuffer[i]];
		if(code!=0)
		{
			wordHash=(wordHash>>1)+code;
		}
		else
		{
			if(wordHash!=0)
			{
				found=0;
				for(j=0;j<distinctWords && !found;j++)
				{
					if(hashedwords[j]==wordHash)
						found=1;
				}
				if(!found)
				{
					printf("Token not found\n");
					return RESULT_ERROR;
				}
				wordHash=0;
			}
		}
	}
	if(wordHash!=0)
	{
		found=0;
		for(j=0;j<distinctWords && !found;j++)
		{
			if(hashedwords[j]==wordHash)
				found=1;
		}
		if(!found)
		{
			printf("Token not found\n");
			return RESULT_ERROR;
		}	
	}
	
	return 0;
}

int fprintBucketKeys(FILE *logfile, Bucket_t *bucket)
{
	int i;

	for(i=0;i<bucket->header.keysCount;i++)
	{
		fprintf(logfile,"key ");
		printBitSequenceToFile(logfile,&(bucket->data[i].value));
		fprintf(logfile," in document %d\n",bucket->data[i].pointer);
	}	
	return 0;
}

int fprintTTNode(FILE *logfile, TopTree_t *tree, TopTreeNode_t *parent, int tabsCounter)
{
	TopTreeNode_t *child;
	int i;

	for(i=0;i<tabsCounter;i++)
		fprintf(logfile,"\t");
	fprintf(logfile,"Node: length=%d child 0=%d child 1=%d\n",
		parent->incomingEdgeLength,parent->children[0],parent->children[1]);
	tabsCounter++;

	if(parent->children[0]==0)
	{
		printf("Invalid 0 child\n");
		exit(1);
	}

	if(parent->children[0]>0)
	{
		child=&(tree->nodes[parent->children[0]]);
		fprintTTNode(logfile,tree,child,tabsCounter);
	}

	if(parent->children[1]==0)
	{
		printf("Invalid 1 child\n");
		exit(1);
	}

	if(parent->children[1]>0)
	{
		child=&(tree->nodes[parent->children[1]]);
		fprintTTNode(logfile,tree,child,tabsCounter);
	}

	return 0;
}

int fprintTopTree(FILE *logfile, Buffer_t *buffer)
{
	int tabsCounter=1;
	TopTreeNode_t *currNode;
	TopTree_t *tree=&(buffer->tree);

	currNode=&(tree->nodes[0]);

	fprintf(logfile,"ROOT node of TT of length=%d child 0=%d child 1=%d\n",
		currNode->incomingEdgeLength, currNode->children[0], currNode->children[1]);

	if(tree->nodes[0].children[0]>0)
	{
		currNode=&tree->nodes[tree->nodes[0].children[0]];
	
		fprintTTNode(logfile,tree,currNode, tabsCounter);
	}

	if(tree->nodes[0].children[1]>0)
	{
		currNode=&tree->nodes[tree->nodes[1].children[0]];
	
		fprintTTNode(logfile,tree,currNode, tabsCounter);
	}
	return 0;
}

int fprintBuffer(FILE *logfile, Buffer_t *buffer)
{
	int totalTopTreeNodes;
	int totalBuckets;
	int i;
	Bucket_t *bucket;

	totalTopTreeNodes=buffer->tree.header.nodesCounter;
	totalBuckets=buffer->tree.header.bucketsCounter;

	fprintf(logfile,"Now the buffer contains %d tt nodes, buckets counter is %d, freeNodeID=%d, freeBucketID=%d\n",
		totalTopTreeNodes,totalBuckets,buffer->tree.header.freeNodePos, buffer->tree.header.freeBucketID);
	fprintf(logfile,"The tree is:\n");
	fprintTopTree(logfile, buffer);


	fprintf(logfile,"The buckets are:\n");
	for(i=1;i<totalBuckets;i++)
	{
		bucket=&buffer->buckets[i];
		if(bucket->header.keysCount==0)
			fprintf(logfile,"Bucket %d is empty\n",i);
		else
		{
			fprintf(logfile,"Bucket %d has %d keys and %d LCP between them. The keys are:\n",
				i,bucket->header.keysCount,bucket->header.LCPinBits);
			fprintBucketKeys(logfile,bucket);
		}
	}
	return 0;
}