#include "general.h"

/**
This is a program which will take a bunch of text files from the specified folder,
parse all the words (which consist of alphanumeric characters),
and insert them into B-tree.

The novelty that it continuously inserts words, into the most locall tree node, such avoiding one random IO per key
For this purpose it uses a sprecial buffer that "concentrates" similar keys into buckets

And when the bucket is full, its content is added to B-tree in one batch update.

For details see:
M. Barsky, A. Thomo, Z. Toth, C. Zuzarte. 
On-line update of B-trees. 
ACM International Conference on Information and Knowledge Management, CIKM-2010: 149-158
*/
int transfercounter;

int totalKeysInserted=0;

int main(int argc, char *argv[]) {
	int i,j;
	FILE *inputfile;
	FILE *sizefile;
	FILE *btreefile;
	char inputFilePrefix[MAX_PATH_LENGTH];
	int minsubsript;
	int maxsubscript;
	char *fileextension;
	char currInputFileName[MAX_PATH_LENGTH];
	char *inputbuffer;
	unsigned int *hashedwords;
	unsigned int *temparray;  //for sorting
	int docID;
	int totalChars;
	int res;
	int totalwords;
	int distinctWords;
	char btreeFileName[MAX_PATH_LENGTH];
	char sizeFileName[MAX_PATH_LENGTH];
	SystemState_t state;
	Buffer_t buffer;
	int filedelta;
	FILE *bufferfile;
	char bufferfilename[MAX_PATH_LENGTH];
	TopTree_t *tree;

	unsigned int btreesizebuf[1];

	
	if(argc<9)	{
		printf("To run: ./onlineupdate <inputfolder> <inputfileprefix>  <minSubscript> <maxSubscript>" 
			"<fileextension> <outputfolder> <btreefilename> <filedelta>\n");
		
		return RESULT_ERROR;
	}
	
//A. set program arguments
	if(atoi(argv[2])!=-1)
		sprintf(inputFilePrefix,"%s%s", argv[1], argv[2]);
	else
		sprintf(inputFilePrefix,"%s", argv[1]);

	minsubsript=atoi(argv[3]);
	maxsubscript=atoi(argv[4]);
	fileextension=argv[5];
	sprintf(btreeFileName,"%s%s", argv[6], argv[7]);
	sprintf(sizeFileName,"%s_size", btreeFileName);
	sprintf(bufferfilename,"%s_buffer", btreeFileName);
	filedelta=atoi(argv[8]);

//B. initialize Btree, memory pool and state
//B1. Set pointer to BTree file, create the file if does not exist
	if(!(btreefile= fopen ( btreeFileName , "r+b" )))	{
		printf("creating a new btree file\n");
		if(!(btreefile= fopen ( btreeFileName , "wb" )))	{
			printf("Could not create new BTree file %s \n",btreeFileName);
			return RESULT_ERROR;
		}
		fclose(btreefile);
		if(!(btreefile= fopen ( btreeFileName , "r+b" )))	{
			printf("Could not open BTree file %s \n",btreeFileName);
			return RESULT_ERROR;
		}
	}

//B2. Determine size of the BTree file if exists - to know how many nodes are already on disk
	if(!(sizefile= fopen ( sizeFileName , "r+b" )))	{
		printf("creating a new size file\n");
		if(!(sizefile= fopen ( sizeFileName , "wb" )))	{
			printf("Could not create new size file %s \n",sizeFileName);
			return RESULT_ERROR;
		}
		fclose(sizefile);
		if(!(sizefile= fopen ( sizeFileName , "r+b" )))	{
			printf("Could not open size file %s \n",sizeFileName);
			return RESULT_ERROR;
		}
	}	

//B3. Init memory pool and load some nodes in memory
	state.btreefile=btreefile;
	state.sizefile=sizefile;
	if(initMemoryPool(&state))
		return RESULT_ERROR;

	//4. init memory pool
	if(initBuffer(&buffer))
		return RESULT_ERROR;

	//reading input, hashing, parsing and insertion into buffer
	prepareCodeTable();

	//allocate input buffer
	inputbuffer=(char*) calloc (INPUT_BUFFER_MAX, sizeof(char));
	if(inputbuffer==NULL)	{
		printf("Failed to allocate memory for input char buffer of size: %d \n",
			INPUT_BUFFER_MAX);
		return RESULT_ERROR;
	}

	hashedwords=(unsigned int*) calloc (INPUT_BUFFER_MAX, sizeof(unsigned int));
	if(hashedwords==NULL)	{
		printf("Failed to allocate memory for hashed words buffer of size: %d \n",
			INPUT_BUFFER_MAX);
		return RESULT_ERROR;
	}

	temparray=(unsigned int*) calloc (INPUT_BUFFER_MAX, sizeof(unsigned int));
	if(temparray==NULL)	{
		printf("Failed to allocate memory for temp sorting buffer of size: %d \n",
			INPUT_BUFFER_MAX);
		return RESULT_ERROR;
	}

	//process 1 document at a time:
	//parse into words, sort, remove duplicates, add to btree
	for(i=minsubsript;i<=maxsubscript;i++)	{
		docID=i-filedelta;
		sprintf(currInputFileName,"%s%d%s", inputFilePrefix, i,fileextension);
		
		if(!(inputfile= fopen ( currInputFileName , "rb" )))	{
			printf("Could not open input file %s \n",currInputFileName);
			return RESULT_ERROR;
		}

		//read content of a file into a buffer
		res=fread (inputbuffer,sizeof(char),INPUT_BUFFER_MAX,inputfile);
		if(res<INPUT_BUFFER_MAX && res>0)
			totalChars=res;
		else if (res==0)	{
			printf("Error reading File %s . Empty file?\n",currInputFileName);
			return RESULT_ERROR;
		}
		else	{
			printf("File %s is too large (%d) for the input buffer of size %d\n",currInputFileName,res,INPUT_BUFFER_MAX);
			return RESULT_ERROR;
		}

		totalwords=0;
		extractWords(inputbuffer,totalChars,hashedwords,&totalwords);

		sortHashedWords(hashedwords,totalwords,temparray);
	
		distinctWords=0;
		removeDuplicates(hashedwords,totalwords,&distinctWords);
		totalKeysInserted+=distinctWords;
		
		for(j=0;j<distinctWords;j++) {				
			if(insertKeyIntoBuffer( hashedwords[j], docID,&buffer,&state))	{
				printf("Failed to insert key %u from document %d\n",hashedwords[j],i);
				//return RESULT_ERROR;
			}
		}		
	
		resetBTreePath(&state);
		fclose(inputfile);

		printf ("Inserted all keywords from file %s\n",  currInputFileName);
	}
	printf("Serializing buffer\n");

	if(!(bufferfile= fopen ( bufferfilename , "wb" )))	{
		printf("Could not create buffer file %s \n",bufferfilename);
		return RESULT_ERROR;
	}

	tree=&(buffer.tree);
	if(fwrite(tree,sizeof(TopTree_t),1, bufferfile)!=1)	{
		printf("failed to serialize buffer top tree\n");
		return RESULT_ERROR;
	}
	
	if(fwrite(buffer.buckets,sizeof(Bucket_t),MAX_NUMBER_OF_BUCKETS, bufferfile)!=MAX_NUMBER_OF_BUCKETS) {
		printf("failed to serialize buffer buckets\n");
		return RESULT_ERROR;
	}

	finish_SynchronizeData(&state);
	fclose(sizefile);
	if(!(sizefile= fopen ( sizeFileName , "wb" )))	{
		printf("Could not open size file %s for writing new BTree size\n",sizeFileName);
		return RESULT_ERROR;
	}
	btreesizebuf[0]=state.maxNodesOnDisk;
	if(fwrite(btreesizebuf,sizeof(unsigned int),1,sizefile)!=1)	{
		printf("Failed to save new BTree file size\n");
		return RESULT_ERROR;
	}
	fclose(sizefile);
	
	return RESULT_OK;
}

