/* dem2stl.c -- convert DEM from Daniele to STL files
                for 3D-printing
   This code is highly hard-coded for the said purpose...
   as I don't mean to make it general purpose,
   but would still demonstrates my defensive habit.
*/

/* Invocation:
  ./dem2stl vertfactor
        vertfact -- vertical exaggeration
*/

/* Let me put the SAGA GIS "Load Binary Raw Data" parameters here
Cell Count X: 13639
Cell Count Y: 9547
Cell Size: 0.000045
Left Border X: 22.5735
Lower Border Y: 113.8217
Unit Name: Meter, Z-multiplier: 1
No Data Value: -9999
Data offset, Line offset, Line endset: all 0 bytes
Data type: 4 Byte Floating Point
Byte order: Big Endian (Motorola)
Line order: Top to bottom
*/

#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>

#define countX 13639
#define countY 9547

int main(int argc, char* argv[]){
  int fd, retVal;
  unsigned int total_size;
  float* data;
  float vertfact = 10.0f;

// Read Arguments
  if(argc > 1){
    vertfact = atof(argv[1]);
  }
  printf("Vertical exaggeration: %.2f\n", vertfact);

// Open and Read file
  if((fd = open("dem",O_RDONLY)) == -1){
    fprintf(stderr,"Error: File cannot be opened.\n");
    exit(-1);
  }
  if(sizeof(float) == 4){
    total_size = sizeof(float)*countX*countY;
  }else{
    fprintf(stderr,"Error: Sizeof float isn't 4-bytes.\nCode it again.!\n");
    exit(-1);
  }
  if((data = (float*) malloc(sizeof(float)*countX*countY)) == NULL){
    fprintf(stderr,"Error: Cannot allocate memory.\n");
    exit(-1);
  };
  if((retVal = read(fd, data, total_size)) < total_size){
    fprintf(stderr,"Error: Cannot read the file.\n");
    exit(-1);
  }
  if((retVal = close(fd)) == 0){
    fprintf(stderr,"Cannot close the file (but we will carry-on).\n");
  }

// Start crunching data
  
  printf("Program finished.\n");
  return 0;
}
