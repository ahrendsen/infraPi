/*


do not use. unless




*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define LINELEN 256            // how long a string we need to hold the filename
#define RAWBLOCKSIZE 6404096
#define HEADERSIZE 32768
#define ROWSIZE 3264 // number of bytes per row of pixels, including 24 'other' bytes at end
#define IDSIZE 4    // number of bytes in raw header ID string
#define HPIXELS 2592   // number of horizontal pixels on OV5647 sensor
#define VPIXELS 1944   // number of vertical pixels on OV5647 sensor

int main(int argc, char* argv[])
{
FILE *fp;  /* input file */
FILE *outfp;
char fname[LINELEN];    /* filename to open */
char outfname[LINELEN];
char *ret;

int i,j,k,kk;       /* loop variables (Hey, I learned FORTRAN early...) */
unsigned long fileLen;  // number of bytes in file
unsigned long offset;  // offset into file to start reading pixel data
unsigned char *buffer;
unsigned short pixel[HPIXELS];  // array holds 16 bits per pixel
unsigned char split;        // single byte with 4 pairs of low-order bits


if (argc==2) strcpy(fname,argv[1]);

strcpy(outfname,fname);
ret=strstr(outfname,".jpg");
if (ret==NULL){
	printf("unexpected file \n");
	return ;
} 
strncpy(ret,".csv",4);
printf("input: ");
printf(fname);
printf("\n");
printf("output: ");
printf(outfname);
printf("\n");


/* program start */

outfp=fopen(outfname,"w");
fp=fopen(fname, "r");

if (fp==NULL)  {
  fprintf(stderr, "Unable to open %s for input.\n",fname);
  return;
  }
   fseek(fp, 0, SEEK_END);
   fileLen=ftell(fp);
    if (fileLen < RAWBLOCKSIZE) {
      fprintf(stderr, "File %s too short to contain expected 6MB RAW data.\n",fname);
      return;
    }
    offset = (fileLen - RAWBLOCKSIZE) ;  // location in file the raw header starts
   fseek(fp, offset, SEEK_SET);  
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
  // now on to the pixel data


  offset = (fileLen - RAWBLOCKSIZE) + HEADERSIZE;  // location in file the raw pixel data starts
  fseek(fp, offset, SEEK_SET);  
  //Read one line of pixel data into buffer

//  printf("%x %x %x %x %x\n",buffer[0], buffer[1], buffer[2], buffer[3], buffer[4]);

  fprintf(outfp,"RAW\n%d,%d\n",HPIXELS,VPIXELS);  // ASCII PGM format header  (grey bitmap, 16 bits per pixel)
  
  for (k=0; k < VPIXELS; k++) {  // iterate over pixel rows
    fread(buffer, ROWSIZE, 1, fp);  // read next line of pixel data
    j = 0;  // offset into buffer
    for (i = 0; i < HPIXELS; i+= 4) {  // iterate over pixel columns
      pixel[i] = buffer[j++] << 2;
      pixel[i+1] = buffer[j++] << 2;
      pixel[i+2] = buffer[j++] << 2;
      pixel[i+3] = buffer[j++] << 2;
      split = buffer[j++];    // low-order packed bits from previous 4 pixels
      pixel[i] += ((split & 0b11000000)>>6);  // unpack them bits, add to 16-bit values, left-justified
      pixel[i+1] += ((split & 0b00110000)>>4);
	pixel[i+2] += (split & 0b00001100)>>2;
      pixel[i+3] += (split & 0b00000011);
    }

    for (i = 0; i < (HPIXELS); i++) {
 	fprintf(outfp,"%d,",pixel[i]);
//	matrix[i][k]=pixel[i];
    }

 fprintf(outfp,"\n");  // add an end-of-line char

  } // end for(k..)


  fprintf(outfp,"\n");  // add an end-of-line char

 fclose(outfp);
  fclose(fp);  // close file  
  free(buffer); // free up that memory we allocated

} // end main

