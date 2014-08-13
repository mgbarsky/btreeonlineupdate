#include "general.h"


unsigned int getBTreeSize(SystemState_t *state)
{
	unsigned int buf[1];
	int filesize;
	int res;

	fseek (state->sizefile, 0, SEEK_END);
    filesize=ftell (state->sizefile);

	if(filesize==0)
		return 0;

	if(filesize!=sizeof(unsigned int))
	{
		printf("error reading size  file \n");
		exit(1);
	}

	rewind(state->sizefile);
	res=fread(&buf[0],sizeof(unsigned int),1,state->sizefile);

	if(res!=1)
	{
		printf("error reading size  file \n");
		exit(1);
	}

	rewind(state->sizefile);
	return buf[0];

}

//TBD - more efficient sizeof() division and multiplication
int moveInBTreeFile(FILE* btreefile, unsigned int nodeID)
{
	unsigned int remainder;
	unsigned int max_move=2000000000/sizeof(BTreeNode_t);

	rewind(btreefile);
	if(nodeID<max_move)
	{
		fseek(btreefile, (int)nodeID*sizeof(BTreeNode_t), SEEK_SET);
		return 0;
	}


	remainder=nodeID;
	
	
	while(remainder>=max_move)
	{
		fseek(btreefile, max_move*sizeof(BTreeNode_t), SEEK_CUR);
		remainder-=max_move;
	}

	fseek(btreefile, (int)remainder*sizeof(BTreeNode_t), SEEK_CUR);
	return 0;
}