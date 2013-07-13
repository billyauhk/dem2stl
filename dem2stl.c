/* dem2stl.c -- convert DEM from Daniele to STL files
                for 3D-printing
   This code is highly hard-coded for the said purpose...
   as I don't mean to make it general purpose,
   but would still demonstrates my defensive habit.

   Output model unit is specified here to be 1.0 mm
   And a 5-mm back-plate will be added so that it does not fall apart
*/

/* Invocation:
  ./dem2stl vertfactor scale north south east west
        vertfact -- vertical exaggeration
        scale -- scale at center
        north, south, east, west -- boundaries
*/

#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#include<stdint.h>

/* Let me put the SAGA GIS "Load Binary Raw Data" parameters here
Unit Name: Meter, Z-multiplier: 1
No Data Value: -9999
Data offset, Line offset, Line endset: all 0 bytes
Data type: 4 Byte Floating Point
Byte order: Big Endian (Motorola)
Line order: Top to bottom
*/
#define countX 13639      //Cell Count X: 13639
#define countY 9547       //Cell Count Y: 9547
#define cellSize 0.000045 //Cell Size: 0.000045
#define startX 113.8217   //Left Border X: 113.8217
#define startY 22.5735    //Lower Border Y: 22.5735

float* data;
float vertfact = 10.0f;
float scale = 1.0f;
float north, south, east, west;

// Generate X coordinate and Y coordinate
// with a simple formula without consideration to ellipsoid/spherical
// but the scale correction for central parallel/meridian only
float getX(unsigned int index){

  return 0;
}

float getY(unsigned int index){

  return 0;
}

float getZ(unsigned int x, unsigned int y){
  float ans;
  ans = data[x*countX+y]; // Get the value
  ans = ans/1852*10;      // Convert to 1.0 nm/cm
  ans *= scale*vertfact;  // Consider scale and vertical exaggeration
  return ans;
}

int main(int argc, char* argv[]){
  unsigned int i,j;            // Loop indices
  unsigned int xa, xb, ya, yb; // Boundary values
  int infile, outfile, retVal; // FD + return value tmp
  unsigned int total_size;

// Read Arguments
  if(argc > 1){
    vertfact = atof(argv[1]);
  }
  printf("Vertical exaggeration: %.2f\n", vertfact);
  if(argc > 2){
    scale = atof(argv[2]);
  }
  printf("Scale: %.2f nm/cm at model center\n", scale);

  north = startY + cellSize*countY;
  south = startY;
  east = startX + cellSize*countX;
  west = startX;
  if(argc > 6){
    north = atof(argv[3]);
    south = atof(argv[4]);
    east = atof(argv[5]);
    west = atof(argv[6]);
    if((north<south) || (east<west)){
      fprintf(stderr,"The order is north south east west.\n");
      fprintf(stderr,"And HK is on the North and East Hemisphere...\n");
      exit(-1);
    }
  }else if(argc > 3){
    printf("Something wrong. You should give 4 parameters for the boundary.\n");
    printf("But I would carry on with the default values.\n");
  }

// Open and Read file
  if((infile = open("dem",O_RDONLY)) == -1){
    fprintf(stderr,"Error: File cannot be opened.\n");
    exit(-1);
  }
  if((sizeof(float) == 4) && (__FLOAT_WORD_ORDER__ == __ORDER_LITTLE_ENDIAN__)){
    total_size = sizeof(float)*countX*countY;
  }else{
    if(sizeof(float) != 4){
      fprintf(stderr,"Error: Sizeof float isn't 4-bytes.\n");
    }else{
      fpritnf(stderr,"Seems the float are not little endian\n");
    }
    fprintf(stderr,"Pls compile it again on a good machine!\n");
    exit(-1);
  }
  if((data = (float*) malloc(sizeof(float)*countX*countY)) == NULL){
    fprintf(stderr,"Error: Cannot allocate memory.\n");
    exit(-1);
  };
  if((retVal = read(infile, data, total_size)) < total_size){
    fprintf(stderr,"Error: Cannot read the file.\n");
    exit(-1);
  }
  if((retVal = close(infile)) == 0){
    fprintf(stderr,"Cannot close the file (but we will carry-on).\n");
  }

// Start crunching data
  printf("Convert the endianess first\n");
  for(i=0;i<countX*countY;i++){
    *((uint32_t*) data+i) = ntohl(*((uint32_t*) data+i));
    if(data[i]!=1.0f){printf("%.2f\n",data[i]);}
  }
  // Determine the final region to be scanned

  // Open our output file

  // Start our main part, the DEM -> STL

  // Add the 5mm padding at the bottom

  printf("Program finished.\n");
  free(data); data = NULL;
  return 0;
}
