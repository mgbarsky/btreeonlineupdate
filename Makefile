CC = gcc
CFLAGOPT = -O3 -Wall 
CFLAGS = -D_LARGEFILE_SOURCE
CFLAGS += -fno-exceptions
CFLAGS += -finline-functions
CFLAGS += -funroll-loops
CFLAGOFFSET = -D_FILE_OFFSET_BITS=64

# Source files
OU_SRC=parser.c bitoperations.c btree.c diskaccess.c search.c memorypool.c dynamicbuckets.c main.c

# Binaries
all: onlineupdate

#streams the lines of the input file (in any format - fasta, text, compressed) and counts k-mers
onlineupdate: $(OU_SRC)
	$(CC) $(CFLAGOPT) $(CFLAGOFFSET) $(CFLAGS) $^ -o $@ 

clean:  
	rm onlineupdate
