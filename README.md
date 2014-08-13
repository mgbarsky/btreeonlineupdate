btreeonlineupdate
=================

This is an implementation  of a buffering system for online (not delayed) update of B-trees.

The main idea is to collect keywords to be indexed in a special in-memory buffer, called dynamic buckets.
Buckets are attached as leaves to a keyword tree. Once a bucket is full, it splits into two, 
based on the difference of bits after their longest common prefixes (LCPs).
The top keyword tree is updated, and buckets-leaves re-attached.

Once the total number of available buckets is exhausted, one bucket -
namely with keywords that are the most similar (in that that they share the longest LCP) - 
gets emptied into the main B-tree index, in a single batch insertion.

The full minimalistic code for batch update of B-trees was written 
because none of the freely available B-tree implementations offer a batch update.

As a result, this system can continuously perform an update of the main B-tree index 
with the much smaller cost of random disk I/Os, without idle time needed for synchronizing buffer with the main index 
in all other batch update scenarios proposed so far.

The system is especially useful for indexing all keywords of a text, 
because each new document contains a set of unordered keywords, 
which - if inserted naively - will incur at least one random disk I/O per insertion.

How the buffer operates is described in:
M. Barsky, A. Thomo, Z. Toth, C. Zuzarte. 
On-line update of B-trees. 
ACM International Conference on Information and Knowledge Management, CIKM-2010: 149-158


<h1>To compile:</h1>
<pre><code>
make
</pre></code>

<h1>To run:</h1>

To run a program (./onlineupdate) you need to provide the following ordered list of input parameters

<h2>Parameters</h2>

1. 'inputfolder' - the folder which contains input text files. 
For this simplistic implementation, all files in the input folder are required 
to share the common prefix and be numbered consecutively: for example file1, file2 etc.

2. 'inputfileprefix' - specifies the common prefix shared by all input files, 
in order to construct a file name for each input file - in our example it is 'file'.

3. 'min subscript' - numeration of input files starts from it.

4. 'max subscript' - numeration of input files ends with it.

5. 'input file extension' - for example .txt if we have input files: file1.txt, file2.txt etc.

6. 'output folder' - where to store the disk-resident B-tree

7. 'b-tree file name' - name of the output B-tree file.

6. 'file number delta' - this was used for testing (to insert different document IDs by running the program on the same input). Set it to 0.


<h1>Sample usage:</h1>

In folder 'sample_input' there are 57 files from the Guttenberg collection.
Folder also contains SAMPLE_RUN.txt with an example of how to run onlineupdate.


