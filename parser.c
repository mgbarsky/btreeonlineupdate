#include "general.h"
/**
This parser uses an extremely fast word extraction method
proposed in:
George Forman and Evan Kirshenbaum. 2008. 
Extremely fast text feature extraction for classification and indexing. 
(CIKM '08). 1221-1230. 

Sorting is used to remove duplicate words
*/

char lowerchartable[256];
unsigned int codetable[256];
unsigned int randnumbers[256];
unsigned int stopwords[32];


void swap(unsigned int *hashedWords,int i, int j) {
    unsigned int t = hashedWords[i];
    hashedWords[i] = hashedWords[j];
    hashedWords[j] = t;
}

int randint(int l, int u) {
    return rand()%(u-l+1)+l;
}

void quicksort(unsigned int *hashedWords,int l, int u) {
    int i, m;
    if (l >= u) return;
    swap(hashedWords,l, randint(l, u));
    m = l;
    for (i = l+1; i <= u; i++)
        if (hashedWords[i]<hashedWords[l])
            swap(hashedWords,++m, i);
    swap(hashedWords,l, m);
    quicksort(hashedWords,l, m-1);
    quicksort(hashedWords, m+1, u);
}

int extractWords(char *inputbuffer, int totalChars, unsigned int *hashedwords, int *totalwords) {
	int i=0;
	int currWord=0;
	unsigned int wordHash=0;
	unsigned int code;

	for(i=0;i<totalChars;i++)	{
		code=codetable[(size_t)inputbuffer[i]];
		if(code!=0)		{
			wordHash=(wordHash>>1)+code;
		}
		else		{
			if(wordHash!=0)
			{
				hashedwords[currWord++]=wordHash;
				wordHash=0;
			}
		}
	}

	if(wordHash!=0)	{
		hashedwords[currWord++]=wordHash;		
	}

	*totalwords=currWord;
	return 0;
}

void mergesorted(unsigned int *hashedwords,int sortedFrom,int sortedTo,
				 int newSortedFrom,int newSortedTo, unsigned int *tempArray){
	int i,j,k;

	for(k=0,i=sortedFrom,j=newSortedFrom;i<sortedTo && j<newSortedTo;k++)
	{
		if(hashedwords[i]<=hashedwords[j])
		{
			tempArray[k]=hashedwords[i];
			i++;
		}
		else
		{
			tempArray[k]=hashedwords[j];
			j++;
		}
	}

	while(i<sortedTo)
	{
		tempArray[k]=hashedwords[i];
		k++;
		i++;
	}

	while(j<newSortedTo)
	{
		tempArray[k]=hashedwords[j];
		k++;
		j++;
	}

	for(i=0;i<newSortedTo;i++)
		hashedwords[i]=tempArray[i];
}

int sortHashedWords(unsigned int *hashedwords,int totalwords, unsigned int *tempArray){
	int toSortFrom=0;
	int toSortTo=MIN(totalwords,SORTING_BUFFER_MAX);
	int sortedFrom=0;
	int sortedTo;
	int remains;
	
	srand(time(NULL));

	quicksort(hashedwords,toSortFrom,toSortTo-1);
	sortedTo=toSortTo;
	remains=totalwords-toSortTo;
	
	while(remains>SORTING_BUFFER_MAX)	{
		toSortFrom+=SORTING_BUFFER_MAX;
		toSortTo+=SORTING_BUFFER_MAX;
		remains-=SORTING_BUFFER_MAX;
		quicksort(hashedwords,toSortFrom,toSortTo-1);
		mergesorted(hashedwords,sortedFrom,sortedTo,toSortFrom,toSortTo,tempArray);
		sortedTo=toSortTo;
	}
		
	if(remains>0)	{
		toSortFrom+=SORTING_BUFFER_MAX;
		toSortTo+=remains;
		quicksort(hashedwords,toSortFrom,toSortTo-1);
		mergesorted(hashedwords,sortedFrom,sortedTo,toSortFrom,toSortTo,tempArray);
	}

	return 0;
}

int removeDuplicates(unsigned int *hashedwords,int totalwords, int *distinctWords){
	int i;
	int distinctCounter=0;  //at least 1 word - the first is distinct

	for(i=1;i<totalwords;i++)
	{
		if(hashedwords[i]!=hashedwords[distinctCounter])
		{
			hashedwords[++distinctCounter]=hashedwords[i];
		}
	}

	*distinctWords=distinctCounter+1;
	return 0;
}



int generateRandomNumbers() {
	randnumbers[0]=41;
	randnumbers[1]=18467;
	randnumbers[2]=6334;
	randnumbers[3]=26500;
	randnumbers[4]=19169;
	randnumbers[5]=15724;
	randnumbers[6]=11478;
	randnumbers[7]=29358;
	randnumbers[8]=26962;
	randnumbers[9]=24464;
	randnumbers[10]=5705;
	randnumbers[11]=28145;
	randnumbers[12]=23281;
	randnumbers[13]=16827;
	randnumbers[14]=9961;
	randnumbers[15]=491;
	randnumbers[16]=2995;
	randnumbers[17]=11942;
	randnumbers[18]=4827;
	randnumbers[19]=5436;
	randnumbers[20]=32391;
	randnumbers[21]=14604;
	randnumbers[22]=3902;
	randnumbers[23]=153;
	randnumbers[24]=292;
	randnumbers[25]=12382;
	randnumbers[26]=17421;
	randnumbers[27]=18716;
	randnumbers[28]=19718;
	randnumbers[29]=19895;
	randnumbers[30]=5447;
	randnumbers[31]=21726;
	randnumbers[32]=14771;
	randnumbers[33]=11538;
	randnumbers[34]=1869;
	randnumbers[35]=19912;
	randnumbers[36]=25667;
	randnumbers[37]=26299;
	randnumbers[38]=17035;
	randnumbers[39]=9894;
	randnumbers[40]=28703;
	randnumbers[41]=23811;
	randnumbers[42]=31322;
	randnumbers[43]=30333;
	randnumbers[44]=17673;
	randnumbers[45]=4664;
	randnumbers[46]=15141;
	randnumbers[47]=7711;
	randnumbers[48]=28253;
	randnumbers[49]=6868;
	randnumbers[50]=25547;
	randnumbers[51]=27644;
	randnumbers[52]=32662;
	randnumbers[53]=32757;
	randnumbers[54]=20037;
	randnumbers[55]=12859;
	randnumbers[56]=8723;
	randnumbers[57]=9741;
	randnumbers[58]=27529;
	randnumbers[59]=778;
	randnumbers[60]=12316;
	randnumbers[61]=3035;
	randnumbers[62]=22190;
	randnumbers[63]=1842;
	randnumbers[64]=288;
	randnumbers[65]=30106;
	randnumbers[66]=9040;
	randnumbers[67]=8942;
	randnumbers[68]=19264;
	randnumbers[69]=22648;
	randnumbers[70]=27446;
	randnumbers[71]=23805;
	randnumbers[72]=15890;
	randnumbers[73]=6729;
	randnumbers[74]=24370;
	randnumbers[75]=15350;
	randnumbers[76]=15006;
	randnumbers[77]=31101;
	randnumbers[78]=24393;
	randnumbers[79]=3548;
	randnumbers[80]=19629;
	randnumbers[81]=12623;
	randnumbers[82]=24084;
	randnumbers[83]=19954;
	randnumbers[84]=18756;
	randnumbers[85]=11840;
	randnumbers[86]=4966;
	randnumbers[87]=7376;
	randnumbers[88]=13931;
	randnumbers[89]=26308;
	randnumbers[90]=16944;
	randnumbers[91]=32439;
	randnumbers[92]=24626;
	randnumbers[93]=11323;
	randnumbers[94]=5537;
	randnumbers[95]=21538;
	randnumbers[96]=16118;
	randnumbers[97]=2082;
	randnumbers[98]=22929;
	randnumbers[99]=16541;
	randnumbers[100]=4833;
	randnumbers[101]=31115;
	randnumbers[102]=4639;
	randnumbers[103]=29658;
	randnumbers[104]=22704;
	randnumbers[105]=9930;
	randnumbers[106]=13977;
	randnumbers[107]=2306;
	randnumbers[108]=31673;
	randnumbers[109]=22386;
	randnumbers[110]=5021;
	randnumbers[111]=28745;
	randnumbers[112]=26924;
	randnumbers[113]=19072;
	randnumbers[114]=6270;
	randnumbers[115]=5829;
	randnumbers[116]=26777;
	randnumbers[117]=15573;
	randnumbers[118]=5097;
	randnumbers[119]=16512;
	randnumbers[120]=23986;
	randnumbers[121]=13290;
	randnumbers[122]=9161;
	randnumbers[123]=18636;
	randnumbers[124]=22355;
	randnumbers[125]=24767;
	randnumbers[126]=23655;
	randnumbers[127]=15574;
	randnumbers[128]=4031;
	randnumbers[129]=12052;
	randnumbers[130]=27350;
	randnumbers[131]=1150;
	randnumbers[132]=16941;
	randnumbers[133]=21724;
	randnumbers[134]=13966;
	randnumbers[135]=3430;
	randnumbers[136]=31107;
	randnumbers[137]=30191;
	randnumbers[138]=18007;
	randnumbers[139]=11337;
	randnumbers[140]=15457;
	randnumbers[141]=12287;
	randnumbers[142]=27753;
	randnumbers[143]=10383;
	randnumbers[144]=14945;
	randnumbers[145]=8909;
	randnumbers[146]=32209;
	randnumbers[147]=9758;
	randnumbers[148]=24221;
	randnumbers[149]=18588;
	randnumbers[150]=6422;
	randnumbers[151]=24946;
	randnumbers[152]=27506;
	randnumbers[153]=13030;
	randnumbers[154]=16413;
	randnumbers[155]=29168;
	randnumbers[156]=900;
	randnumbers[157]=32591;
	randnumbers[158]=18762;
	randnumbers[159]=1655;
	randnumbers[160]=17410;
	randnumbers[161]=6359;
	randnumbers[162]=27624;
	randnumbers[163]=20537;
	randnumbers[164]=21548;
	randnumbers[165]=6483;
	randnumbers[166]=27595;
	randnumbers[167]=4041;
	randnumbers[168]=3602;
	randnumbers[169]=24350;
	randnumbers[170]=10291;
	randnumbers[171]=30836;
	randnumbers[172]=9374;
	randnumbers[173]=11020;
	randnumbers[174]=4596;
	randnumbers[175]=24021;
	randnumbers[176]=27348;
	randnumbers[177]=23199;
	randnumbers[178]=19668;
	randnumbers[179]=24484;
	randnumbers[180]=8281;
	randnumbers[181]=4734;
	randnumbers[182]=53;
	randnumbers[183]=1999;
	randnumbers[184]=26418;
	randnumbers[185]=27938;
	randnumbers[186]=6900;
	randnumbers[187]=3788;
	randnumbers[188]=18127;
	randnumbers[189]=467;
	randnumbers[190]=3728;
	randnumbers[191]=14893;
	randnumbers[192]=24648;
	randnumbers[193]=22483;
	randnumbers[194]=17807;
	randnumbers[195]=2421;
	randnumbers[196]=14310;
	randnumbers[197]=6617;
	randnumbers[198]=22813;
	randnumbers[199]=9514;
	randnumbers[200]=14309;
	randnumbers[201]=7616;
	randnumbers[202]=18935;
	randnumbers[203]=17451;
	randnumbers[204]=20600;
	randnumbers[205]=5249;
	randnumbers[206]=16519;
	randnumbers[207]=31556;
	randnumbers[208]=22798;
	randnumbers[209]=30303;
	randnumbers[210]=6224;
	randnumbers[211]=11008;
	randnumbers[212]=5844;
	randnumbers[213]=32609;
	randnumbers[214]=14989;
	randnumbers[215]=32702;
	randnumbers[216]=3195;
	randnumbers[217]=20485;
	randnumbers[218]=3093;
	randnumbers[219]=14343;
	randnumbers[220]=30523;
	randnumbers[221]=1587;
	randnumbers[222]=29314;
	randnumbers[223]=9503;
	randnumbers[224]=7448;
	randnumbers[225]=25200;
	randnumbers[226]=13458;
	randnumbers[227]=6618;
	randnumbers[228]=20580;
	randnumbers[229]=19796;
	randnumbers[230]=14798;
	randnumbers[231]=15281;
	randnumbers[232]=19589;
	randnumbers[233]=20798;
	randnumbers[234]=28009;
	randnumbers[235]=27157;
	randnumbers[236]=20472;
	randnumbers[237]=23622;
	randnumbers[238]=18538;
	randnumbers[239]=12292;
	randnumbers[240]=6038;
	randnumbers[241]=24179;
	randnumbers[242]=18190;
	randnumbers[243]=29657;
	randnumbers[244]=7958;
	randnumbers[245]=6191;
	randnumbers[246]=19815;
	randnumbers[247]=22888;
	randnumbers[248]=19156;
	randnumbers[249]=11511;
	randnumbers[250]=16202;
	randnumbers[251]=2634;
	randnumbers[252]=24272;
	randnumbers[253]=20055;
	randnumbers[254]=20328;
	randnumbers[255]=22646;
	return 0;
}

int prepareTable()
{
	lowerchartable[0]=0;
	lowerchartable[1]=0;
	lowerchartable[2]=0;
	lowerchartable[3]=0;
	lowerchartable[4]=0;
	lowerchartable[5]=0;
	lowerchartable[6]=0;
	lowerchartable[7]=0;
	lowerchartable[8]=0;
	lowerchartable[9]=0;
	lowerchartable[10]=0;
	lowerchartable[11]=0;
	lowerchartable[12]=0;
	lowerchartable[13]=0;
	lowerchartable[14]=0;
	lowerchartable[15]=0;
	lowerchartable[16]=0;
	lowerchartable[17]=0;
	lowerchartable[18]=0;
	lowerchartable[19]=0;
	lowerchartable[20]=0;
	lowerchartable[21]=0;
	lowerchartable[22]=0;
	lowerchartable[23]=0;
	lowerchartable[24]=0;
	lowerchartable[25]=0;
	lowerchartable[26]=0;
	lowerchartable[27]=0;
	lowerchartable[28]=0;
	lowerchartable[29]=0;
	lowerchartable[30]=0;
	lowerchartable[31]=0;
	lowerchartable[32]=0;
	lowerchartable[33]=0;
	lowerchartable[34]=0;
	lowerchartable[35]=0;
	lowerchartable[36]=0;
	lowerchartable[37]=0;
	lowerchartable[38]=0;
	lowerchartable[39]=0;
	lowerchartable[40]=0;
	lowerchartable[41]=0;
	lowerchartable[42]=0;
	lowerchartable[43]=0;
	lowerchartable[44]=0;
	lowerchartable[45]=0;
	lowerchartable[46]=0;
	lowerchartable[47]='/';
	lowerchartable[48]='0';
	lowerchartable[49]='1';
	lowerchartable[50]='2';
	lowerchartable[51]='3';
	lowerchartable[52]='4';
	lowerchartable[53]='5';
	lowerchartable[54]='6';
	lowerchartable[55]='7';
	lowerchartable[56]='8';
	lowerchartable[57]='9';
	lowerchartable[58]=0;
	lowerchartable[59]=0;
	lowerchartable[60]=0;
	lowerchartable[61]=0;
	lowerchartable[62]=0;
	lowerchartable[63]=0;
	lowerchartable[64]='@';
	lowerchartable[65]='a';
	lowerchartable[66]='b';
	lowerchartable[67]='c';
	lowerchartable[68]='d';
	lowerchartable[69]='e';
	lowerchartable[70]='f';
	lowerchartable[71]='g';
	lowerchartable[72]='h';
	lowerchartable[73]='i';
	lowerchartable[74]='j';
	lowerchartable[75]='k';
	lowerchartable[76]='l';
	lowerchartable[77]='m';
	lowerchartable[78]='n';
	lowerchartable[79]='o';
	lowerchartable[80]='p';
	lowerchartable[81]='q';
	lowerchartable[82]='r';
	lowerchartable[83]='s';
	lowerchartable[84]='t';
	lowerchartable[85]='u';
	lowerchartable[86]='v';
	lowerchartable[87]='w';
	lowerchartable[88]='x';
	lowerchartable[89]='y';
	lowerchartable[90]='z';
	lowerchartable[91]=0;
	lowerchartable[92]=0;
	lowerchartable[93]=0;
	lowerchartable[94]=0;
	lowerchartable[95]='_';
	lowerchartable[96]=0;
	lowerchartable[97]='a';
	lowerchartable[98]='b';
	lowerchartable[99]='c';
	lowerchartable[100]='d';
	lowerchartable[101]='e';
	lowerchartable[102]='f';
	lowerchartable[103]='g';
	lowerchartable[104]='h';
	lowerchartable[105]='i';
	lowerchartable[106]='j';
	lowerchartable[107]='k';
	lowerchartable[108]='l';
	lowerchartable[109]='m';
	lowerchartable[110]='n';
	lowerchartable[111]='o';
	lowerchartable[112]='p';
	lowerchartable[113]='q';
	lowerchartable[114]='r';
	lowerchartable[115]='s';
	lowerchartable[116]='t';
	lowerchartable[117]='u';
	lowerchartable[118]='v';
	lowerchartable[119]='w';
	lowerchartable[120]='x';
	lowerchartable[121]='y';
	lowerchartable[122]='z';
	lowerchartable[123]=0;
	lowerchartable[124]=0;
	lowerchartable[125]=0;
	lowerchartable[126]=0;
	lowerchartable[127]=0;
	lowerchartable[128]=0;
	lowerchartable[129]=0;
	lowerchartable[130]=0;
	lowerchartable[131]=0;
	lowerchartable[132]=0;
	lowerchartable[133]=0;
	lowerchartable[134]=0;
	lowerchartable[135]=0;
	lowerchartable[136]=0;
	lowerchartable[137]=0;
	lowerchartable[138]=0;
	lowerchartable[139]=0;
	lowerchartable[140]=0;
	lowerchartable[141]=0;
	lowerchartable[142]=0;
	lowerchartable[143]=0;
	lowerchartable[144]=0;
	lowerchartable[145]=0;
	lowerchartable[146]=0;
	lowerchartable[147]=0;
	lowerchartable[148]=0;
	lowerchartable[149]=0;
	lowerchartable[150]=0;
	lowerchartable[151]=0;
	lowerchartable[152]=0;
	lowerchartable[153]=0;
	lowerchartable[154]=0;
	lowerchartable[155]=0;
	lowerchartable[156]=0;
	lowerchartable[157]=0;
	lowerchartable[158]=0;
	lowerchartable[159]=0;
	lowerchartable[160]=0;
	lowerchartable[161]=0;
	lowerchartable[162]=0;
	lowerchartable[163]=0;
	lowerchartable[164]=0;
	lowerchartable[165]=0;
	lowerchartable[166]=0;
	lowerchartable[167]=0;
	lowerchartable[168]=0;
	lowerchartable[169]=0;
	lowerchartable[170]=0;
	lowerchartable[171]=0;
	lowerchartable[172]=0;
	lowerchartable[173]=0;
	lowerchartable[174]=0;
	lowerchartable[175]=0;
	lowerchartable[176]=0;
	lowerchartable[177]=0;
	lowerchartable[178]=0;
	lowerchartable[179]=0;
	lowerchartable[180]=0;
	lowerchartable[181]=0;
	lowerchartable[182]=0;
	lowerchartable[183]=0;
	lowerchartable[184]=0;
	lowerchartable[185]=0;
	lowerchartable[186]=0;
	lowerchartable[187]=0;
	lowerchartable[188]=0;
	lowerchartable[189]=0;
	lowerchartable[190]=0;
	lowerchartable[191]=0;
	lowerchartable[192]=0;
	lowerchartable[193]=0;
	lowerchartable[194]=0;
	lowerchartable[195]=0;
	lowerchartable[196]=0;
	lowerchartable[197]=0;
	lowerchartable[198]=0;
	lowerchartable[199]=0;
	lowerchartable[200]=0;
	lowerchartable[201]=0;
	lowerchartable[202]=0;
	lowerchartable[203]=0;
	lowerchartable[204]=0;
	lowerchartable[205]=0;
	lowerchartable[206]=0;
	lowerchartable[207]=0;
	lowerchartable[208]=0;
	lowerchartable[209]=0;
	lowerchartable[210]=0;
	lowerchartable[211]=0;
	lowerchartable[212]=0;
	lowerchartable[213]=0;
	lowerchartable[214]=0;
	lowerchartable[215]=0;
	lowerchartable[216]=0;
	lowerchartable[217]=0;
	lowerchartable[218]=0;
	lowerchartable[219]=0;
	lowerchartable[220]=0;
	lowerchartable[221]=0;
	lowerchartable[222]=0;
	lowerchartable[223]=0;
	lowerchartable[224]=0;
	lowerchartable[225]=0;
	lowerchartable[226]=0;
	lowerchartable[227]=0;
	lowerchartable[228]=0;
	lowerchartable[229]=0;
	lowerchartable[230]=0;
	lowerchartable[231]=0;
	lowerchartable[232]=0;
	lowerchartable[233]=0;
	lowerchartable[234]=0;
	lowerchartable[235]=0;
	lowerchartable[236]=0;
	lowerchartable[237]=0;
	lowerchartable[238]=0;
	lowerchartable[239]=0;
	lowerchartable[240]=0;
	lowerchartable[241]=0;
	lowerchartable[242]=0;
	lowerchartable[243]=0;
	lowerchartable[244]=0;
	lowerchartable[245]=0;
	lowerchartable[246]=0;
	lowerchartable[247]=0;
	lowerchartable[248]=0;
	lowerchartable[249]=0;
	lowerchartable[250]=0;
	lowerchartable[251]=0;
	lowerchartable[252]=0;
	lowerchartable[253]=0;
	lowerchartable[254]=0;
	lowerchartable[255]=0;

	return 0;
}

int prepareCodeTable()
{
	int i;

	prepareTable();
	generateRandomNumbers();
	for(i=0;i<256;i++)
	{
		if(lowerchartable[i]!=0)
			codetable[i]=randnumbers[i];
		else
			codetable[i]=0;
	}
	return 0;
}

int prepareStopWordsSortedList() {  
	int totalwords=0, distinctWords=0;
	unsigned int temparray[1];
		
	char * stopsentence="I a about an are as at be by for from how in is it of on or that the this to was what when where who will with";

	extractWords(stopsentence,114,stopwords,&totalwords);
	printf("Total %d stop words\n",totalwords);
	
	sortHashedWords(stopwords,totalwords,temparray);	
		
	removeDuplicates(stopwords,totalwords,&distinctWords);
	printf("Total non-duplicates %d stop words\n",distinctWords);

	return 0;
}
