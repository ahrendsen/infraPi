/* Read in jpeg from Raspberry Pi camera captured using 'raspistill --raw'
   and extract raw file with 10-bit values 
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <math.h>

#define LINELEN 256            // how long a string we need to hold the filename
#define RAWBLOCKSIZE 6404096
#define HEADERSIZE 32768
#define ROWSIZE 3264 // number of bytes per row of pixels, including 24 'other' bytes at end
#define IDSIZE 4    // number of bytes in raw header ID string
#define HPIXELS 2592   // number of horizontal pixels on OV5647 sensor
#define VPIXELS 1944   // number of vertical pixels on OV5647 sensor
#define NUMAVG 8   //number of pixels to sum in H and V direction.  so one data point represented by sum[i][k] represents NUMAVG^2 raw pixels


int main(int argc, char* argv[]) {

FILE *fp;  /* input file */
FILE *outfp;
char fname[LINELEN];    // filename to open 
char outfname[LINELEN]; // filename to output CSV
char *ret;

int i,j,k,kk,m,ii,h,v,col;
int r1,r2,c1,c2; //row and column limits, if specified by the user, controll what information is written to csv
unsigned long fileLen;  // number of bytes in file
unsigned long offset;  // offset into file to start reading pixel data
unsigned char *buffer;
unsigned short pixel[HPIXELS];  // array holds 16 bits per pixel
unsigned char split;        // single byte with 4 pairs of low-order bits

unsigned int sum[HPIXELS/NUMAVG];  // used to sum pixels .
unsigned int temp[HPIXELS][NUMAVG]; // used to compute standard deviation
/*  note that as pixels are read in, the sum, average, and standard deviation are computed every NUMAVG data points.
these variables are re-zeroed and reused after they are written to the CSV file */
float stdev[HPIXELS/NUMAVG];
float avg,bkgnd,bkgndstdev;

float globalWavg,globalStdev; //the weighted average of the pixels in the box bound by c1 c2 r1 r2
int globalCount;

if (argc<2){
	printf("Usage ./extractRAWaverage $FILENAME [optional limits:] c1 c2 r1 r2 [optional background] bckgnd bckgndstdev\n");
	return;

}
globalWavg=0.0;
globalStdev=0.0;
globalCount=0;
c1=0;
c2=HPIXELS/NUMAVG;
r1=0;
r2=VPIXELS/NUMAVG;
if (argc>=6){
	//get bounding limits
	c1=atoi(argv[2]);
	c2=atoi(argv[3]);
	r1=atoi(argv[4]);
	r2=atoi(argv[5]);
}

/*
to do: check c1 r1 c2 r2 limits.  greater than zero. less than PIXELS/NUMAVG
and r1<r2
and c1<c2
*/
bkgnd=0.0;
bkgndstdev=0.0;
if (argc==8){
	bkgnd=atof(argv[6]);
	bkgndstdev=atof(argv[7]);
}

	strcpy(fname,argv[1]);
	strcpy(outfname,fname);
	ret=strstr(outfname,".jpg");
	if (ret==NULL){
		printf("unexpected file \n");
	return ;
	}

fp=fopen(fname, "r");
if (fp==NULL)  {
  fprintf(stderr, "Unable to open %s for input.\n",fname);
  return;
  }
	strncpy(ret,".csv",4);
	printf("input: ");
	printf(fname);
	printf("\n");
	printf("output: ");
	printf(outfname);
	printf("\n");

outfp=fopen(outfname,"w");
if (outfp==NULL)  {
  fprintf(stderr, "Unable to open %s for output.\n",outfname);
  return;
  }

  // printf("Opening binary file for input: %s\n",fname);
  // for one file, (TotalFileLength:11112983 - RawBlockSize:6404096) + Header:32768 = 4741655
  // The pixel data is arranged in the file in rows, with 3264 bytes per row.
  // with 3264 bytes per row x 1944 rows we have 6345216 bytes, that is the full 2592x1944 image area.

   //Get file length
   fseek(fp, 0, SEEK_END);
   fileLen=ftell(fp);
    if (fileLen < RAWBLOCKSIZE) {
      fprintf(stderr, "File %s too short to contain expected 6MB RAW data.\n",fname);
      return;
    }
    offset = (fileLen - RAWBLOCKSIZE) ;  // location in file the raw header starts
   fseek(fp, offset, SEEK_SET);

  //printf("File length = %d bytes.\n",fileLen);
  //printf("offset = %d:",offset);

  //Allocate memory for one line of pixel data
  buffer=(unsigned char *)malloc(ROWSIZE+1);
  if (!buffer)
  {
    fprintf(stderr, "Memory error!");
    fclose(fp);
    return;
  }

  //Read one line of pixel data into buffer
  fread(buffer, IDSIZE, 1, fp);
  //printf(" ID:  %c %c %c %c\n",buffer[0], buffer[1], buffer[2], buffer[3]);
  //this is header information in the RAW data block.  it's not useful


  offset = (fileLen - RAWBLOCKSIZE) + HEADERSIZE;  // location in file the raw pixel data starts
  fseek(fp, offset, SEEK_SET);
  //Read one line of pixel data into buffer
  // printf("%x %x %x %x %x\n",buffer[0], buffer[1], buffer[2], buffer[3], buffer[4]);
	//more unused header information

 fprintf(outfp,outfname);
 fprintf(outfp,"\nRAW,%d,%d\n",HPIXELS/NUMAVG,VPIXELS/NUMAVG);
	fprintf(outfp,"COL-ROW,");

	for (i=0; i<(HPIXELS/NUMAVG); i++){
		if ((i>=c1)&(i<=c2))  fprintf(outfp,"%d,",i);		//print column index
	}
	fprintf(outfp,"\n");


		//clear varibles for holding sums and pixelvalues used for stdev.
		for (i=0;i<(HPIXELS/NUMAVG); i++){
			sum[i]=0;
		}
		for (k=0;k<NUMAVG;k++){
		for (i=0;i<HPIXELS;i++){
			temp[i][k]=0;
		}
		}
col=0;
  for (k=0; k < VPIXELS; k++) {  // iterate over pixel ROWS
    fread(buffer, ROWSIZE, 1, fp);  // read next line of pixel data
    j = 0;  // offset into buffer

	/*
		raw bayer matrix pixel information is saved 8-bits at a time (standard unsigned char/int length).  Since the ADC values from
		the camera are 10 bits, they are written by the camera in a packed format. To get the full and actual ADC value into our variable, tThese pieces must be re-asembled.
		The fifth bit contains the two most Least Significant bits  (2LSB)
		PIXEL FORMAT

		[P1-8MSB] [P2-8MSB] [P3-8MSB] [P4-8MSB] [(P2-2LSB)(P2-2LSB)(P3-2LSB)(P4-2LSB)]

	*/

    for (i = 0; i < HPIXELS; i+= 4) {  // iterate over pixel columns
	pixel[i] = buffer[j++] << 2;  // 8 Most Significant Bits
	pixel[i+1] = buffer[j++] << 2;
	pixel[i+2] = buffer[j++] << 2;
	pixel[i+3] = buffer[j++] << 2;
	split = buffer[j++];    // Least Significant two Bits packed from previous 4 pixels
	pixel[i] += ((split & 0b11000000)>>6);  // unpack bits, shift to LSB, and add to 8-bit values
	pixel[i+1] += ((split & 0b00110000)>>4);
	pixel[i+2] += (split & 0b00001100)>>2;
	pixel[i+3] += (split & 0b00000011);  // pixel values (really bayer matrix value) are now 10-bit numbers
    }

	/*
	Since pixel information is read from file, line by line in the buffer, the sum and stdev are computed every NUMAVG rows. I could have declared a 
	matrix varible to hold all of the pixel information, but this would have used much more memory.  

	example pixel and data arrangment.  To keep this table simple, I assume that NUMAVG=4

	k=	kk=
	-----------------	|------------sum[0]-------------||-----------sum[1]-------------||---------sum[2]---- ...	sum[ii]
	0	0	^	|p[0]	p[1]	p[2]	p[3]	||p[4]	p[5]	p[6]	p[7]	||p[8]	p[9]	...		sum[i]
sum[] gets reused	|
	1	1  temp[i][kk]	|p[0]	p[1]	p[2]	p[3]	||p[4]	p[5]	p[6]	p[7]	||p[8]	p[9]	...		sum[i]

	2	2	|

	3	4	|
	-----------------
temp[][] gets reused
	4	0

	5	1
	...

	*/

	kk = k%NUMAVG;
	if (k%NUMAVG==(NUMAVG-1)){
		if ((col>=r1)&(col<=r2)) fprintf(outfp,"%d,",col);
	}

	for (i=0; i<(HPIXELS/NUMAVG); i++){
		for (m=0;m<NUMAVG;m++){
			sum[i]+=pixel[NUMAVG*i+m];
			temp[NUMAVG*i+m][kk] = pixel[NUMAVG*i+m];
		}
		if (k%NUMAVG==(NUMAVG-1)) {
			stdev[i]=0.0;
			avg=sum[i]/pow(NUMAVG,2.0);
			for (v=0;v<NUMAVG;v++){
			for (h=0;h<NUMAVG;h++){
				stdev[i]+=pow(((float) temp[NUMAVG*i+h][v]-avg),2.0);
			}
			}
			stdev[i]=sqrt(stdev[i]/pow(NUMAVG,2.0));
			// only print data to file if it is within the bounding limits
			if ((i >= c1)&(i <= c2)&(col >= r1)&(col <= r2)) {
				fprintf(outfp,"%4.2f,",(avg-bkgnd));
				globalCount++;
				globalWavg+=(avg-bkgnd)/pow(stdev[i],2.0);
				globalStdev+=1/pow(stdev[i],2.0);
				}
		}
	}

	if (k%NUMAVG==(NUMAVG-1)){

		if((col >= r1)&(col <=r2)) fprintf(outfp,"*,");// put a seperator between the average block and stdev block
		// at end of datablock used for averaging 
		// stdard deviations write out 
		for (i=0; i<(HPIXELS/NUMAVG); i++){
			if ((i >= c1)&(i <= c2)&(col >= r1)&(col <= r2)) fprintf(outfp,"%4.2f,",sqrt( pow(stdev[i],2) + pow(bkgndstdev,2)) );
		}
		// reset varibables 
		for (i=0;i<(HPIXELS/NUMAVG); i++){
			sum[i]=0;
		}
 		for (v=0;v<NUMAVG;v++){
		for (h=0;h<HPIXELS;h++){
			temp[h][v]=0;
			}
			}
		if ((col>= r1)&(col <= r2))  fprintf(outfp,"\n");  // add an end-of-line char
	col++;

	}

  } // end for(k..)


	fclose(outfp);
	fclose(fp);  // close file
	free(buffer); // free up memory we allocated


globalWavg=globalWavg/globalStdev;
globalStdev=1/sqrt(globalStdev);

printf("Average %.2f\t Stdev %.2f\n",globalWavg,globalStdev);



} // end main

