#include "general.h"

/**
Our keyword tree buffer is built on bits 
(to avoid splitting each node on multiple characters of the alphabet)
So we need bit operations.
*/
unsigned int masks_array32[32]={
	(unsigned int) (1 << 0),
	(unsigned int) (1 << 1),
	(unsigned int) (1 << 2),
	(unsigned int) (1 << 3),
	(unsigned int) (1 << 4),
	(unsigned int) (1 << 5),
	(unsigned int) (1 << 6),
	(unsigned int) (1 << 7),
	(unsigned int) (1 << 8),
	(unsigned int) (1 << 9),
	(unsigned int) (1 << 10),
	(unsigned int) (1 << 11),
	(unsigned int) (1 << 12),
	(unsigned int) (1 << 13),
	(unsigned int) (1 << 14),
	(unsigned int) (1 << 15),
	(unsigned int) (1 << 16),
	(unsigned int) (1 << 17),
	(unsigned int) (1 << 18),
	(unsigned int) (1 << 19),
	(unsigned int) (1 << 20),
	(unsigned int) (1 << 21),
	(unsigned int) (1 << 22),
	(unsigned int) (1 << 23),
	(unsigned int) (1 << 24),
	(unsigned int) (1 << 25),
	(unsigned int) (1 << 26),
	(unsigned int) (1 << 27),
	(unsigned int) (1 << 28),
	(unsigned int) (1 << 29),
	(unsigned int) (1 << 30),
	(unsigned int) (1 << 31)
};

int getBit(unsigned int *sequence, int pos) {
	return ((*sequence & masks_array32[NUM_BITS_INUINT-pos-1])!=0);
}

void setBit(unsigned int *sequence,int pos) {
	(*sequence)|=masks_array32[NUM_BITS_INUINT-pos-1];
}

void printBitSequence(unsigned int *bitseq) {
	int i;
	
	for(i=0;i<NUM_BITS_INUINT;i++)	{		
		int bit=getBit(bitseq ,i);
		printf("%d",bit);
	}
	printf("\n");
}

void printBitSequenceToFile(FILE *file,unsigned int *bitseq) {
	int i;
	
	for(i=0;i<NUM_BITS_INUINT;i++)
	{		
		int bit=getBit(bitseq ,i);
		fprintf(file,"%d",bit);
	}
}

int getLCP(unsigned int *first, unsigned int *second) {
	int b=0;
	
	unsigned int diff=(*first)^(*second);
	unsigned int firstone=(unsigned int) (1 << (NUM_BITS_INUINT-1));//number 1000....

	for(;diff!=0;diff=diff<<1)
	{		
		if(diff &(firstone))
		{
		
			return b;
		}
		b++;
	}
	return NUM_BITS_INUINT;  //generally, we are looking for lcp for 2 different keys, hence this is an error

}

int getLCPwithBitsAfterLCP(unsigned int *first, unsigned int *second,char *bit1, char *bit2)
{
	int b=0;
	
	unsigned int diff=(*first)^(*second);
	unsigned int firstone=(unsigned int) (1 << (NUM_BITS_INUINT-1));//number 1000...

	for(;diff!=0;diff=diff<<1)
	{		
		if(diff &(firstone))
		{
			*bit1=getBit(first,b); //b+1
			*bit2=getBit(second,b);
			return b;
		}
		b++;
	}
	return NUM_BITS_INUINT;  //generally, we are looking for lcp for 2 different keys, hence this is an error
}