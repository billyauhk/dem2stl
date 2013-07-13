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
#include<math.h>

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
#define padding 5         //Padding depth 5mm

float* data;
float vertfact = 10.0f;
float scale = 1.0f;
float north, south, east, west;

// Generate X coordinate and Y coordinate
// with a simple formula without consideration to ellipsoid/spherical
// but the scale correction for central parallel/meridian only
float getX(unsigned int index){
  return index;
}

float getY(unsigned int index){
  return index;
}

float getZ(unsigned int x, unsigned int y){
  float ans;
  ans = data[x*countX+y];  // Get the value
  ans = ans/1852.0f*10.0f; // Convert to 1.0 nm/cm
  ans *= scale*vertfact;   // Consider scale and vertical exaggeration
  return ans;
}

#pragma pack(1)
typedef struct triangle_struct{
  float normal[3];
  float vertex1[3];
  float vertex2[3];
  float vertex3[3];
  uint16_t spacer;
} triangle;

// Compute the normal vector
#define xcomp 0
#define ycomp 1
#define zcomp 2
#define square(x) ((x)*(x))
void calcNormal(triangle* t){
  float magnitude;

  t->normal[0] = (t->vertex2[ycomp]-t->vertex1[ycomp])*(t->vertex3[zcomp]-t->vertex1[zcomp])
                -(t->vertex3[ycomp]-t->vertex1[ycomp])*(t->vertex2[zcomp]-t->vertex1[zcomp]);

  t->normal[1] = (t->vertex2[zcomp]-t->vertex1[zcomp])*(t->vertex3[xcomp]-t->vertex1[xcomp])
                -(t->vertex2[xcomp]-t->vertex1[xcomp])*(t->vertex3[zcomp]-t->vertex1[zcomp]);

  t->normal[2] = (t->vertex2[xcomp]-t->vertex1[xcomp])*(t->vertex3[ycomp]-t->vertex1[ycomp])
                -(t->vertex2[ycomp]-t->vertex1[ycomp])*(t->vertex3[xcomp]-t->vertex1[xcomp]);

  //Normalization
  magnitude = sqrtf(square(t->normal[0])+square(t->normal[1])+square(t->normal[2]));
  t->normal[0] /= magnitude;
  t->normal[1] /= magnitude;
  t->normal[2] /= magnitude;
}

int main(int argc, char* argv[]){
  unsigned int i,j;            // Loop indices
  int xa, xb, ya, yb;          // Boundary values
  int infile, outfile, retVal; // FD + return value tmp
  unsigned int total_size;

  char headerLine[80];         // STL file header line
  unsigned int totalTriangle;  // Total # of triangles
  triangle buffer[2];          // Buffering two triangles

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
    fprintf(stderr,"Error: File cannot be opened.\n");exit(-1);
  }
  if((sizeof(float) == 4) && (__FLOAT_WORD_ORDER__ == __ORDER_LITTLE_ENDIAN__)){
    total_size = sizeof(float)*countX*countY;
  }else{
    if(sizeof(float) != 4){
      fprintf(stderr,"Error: Sizeof float isn't 4-bytes.\n");
    }else{
      fpritnf(stderr,"Seems the float are not little endian\n");
    }
    fprintf(stderr,"Pls compile it again on a good machine!\n");exit(-1);
  }
  if((data = (float*) malloc(sizeof(float)*countX*countY)) == NULL){
    fprintf(stderr,"Error: Cannot allocate memory.\n");exit(-1);
  };
  if((retVal = read(infile, data, total_size)) < total_size){
    fprintf(stderr,"Error: Cannot read the file.\n");exit(-1);
  }
  if((retVal = close(infile)) == 0){
    fprintf(stderr,"Cannot close the file (but we will carry-on).\n");
  }

// Start crunching data
  printf("Convert the endianess first\n");
  for(i=0;i<countX*countY;i++){
    *((uint32_t*) data+i) = ntohl(*((uint32_t*) data+i));
  }
  // Determine the final region to be scanned
  xa = floorf((west-startX)/cellSize);
  xb = ceilf((east-startX)/cellSize);
  ya = floorf((south-startY)/cellSize);
  yb = ceilf((north-startY)/cellSize);
  // Boundary handling
  if(xa < 0) xa=0;
  if(xb >= countX) xb = (countX-1);
  if(ya < 0) ya=0;
  if(yb >= countX) yb = (countY-1);
  printf("Boundary cell numbers (x: %d, %d; y: %d, %d).\n",xa,xb,ya,yb);

  // Open our output file
  if((outfile = open("dem.stl",O_CREAT|O_WRONLY,S_IRUSR|S_IWUSR)) == -1){
    fprintf(stderr,"Cannot open output file. Quit\n");exit(-1);
  }
  // Start our main part, the DEM -> STL
    // Header first
    sprintf(headerLine,"STL by dem2stl from billyauhk\n");
    if((retVal = write(outfile, headerLine, sizeof(char)*80))!=80){
      fprintf(stderr,"Write fail.\n");exit(-1);
    }
    // 4-bytes for total number of facets
    totalTriangle = (yb-ya)*(xb-xa)*2   /* Topography triangles */;// \
//                   +((ya-yb)+(xa-xb))*2 /* 4-sides of base */      \
//                   +2;                  /* Bottom of base  */
    if((retVal = write(outfile, &totalTriangle, sizeof(uint32_t)))!=4){
      fprintf(stderr,"Write fail.\n");exit(-1);
    }
    // A final check of the compilation
    if(sizeof(triangle)!=50){
      fprintf(stderr,"Compiler error! The pragma pack is not sincerley executed.\n");exit(-1);
    }

/* Beware: Paul Bourke reminds that a rule for STL file is that
           all adjacent facets must share two common vertices... */

    // Time for the topography triangles...
    for(i=xa;i<xb;i++){
      for(j=ya;j<yb;j++){
        // First triangle: (i,j) -> (i+1, j) ->  (i, j+1)
        buffer[0].vertex1[0] = getX(i);
        buffer[0].vertex1[1] = getY(j);
        buffer[0].vertex1[2] = getZ(i,j);
        buffer[0].vertex2[0] = getX(i+1);
        buffer[0].vertex2[1] = getY(j);
        buffer[0].vertex2[2] = getZ(i+1,j);
        buffer[0].vertex3[0] = getX(i);
        buffer[0].vertex3[1] = getY(j+1);
        buffer[0].vertex3[2] = getZ(i,j+1);
        calcNormal(buffer+0);

        // Second triangle: (i+1, j) -> (i+1, j+1) -> (i, j+1)
        buffer[1].vertex1[0] = getX(i+1);
        buffer[1].vertex1[1] = getY(j);
        buffer[1].vertex1[2] = getZ(i+1,j);
        buffer[1].vertex2[0] = getX(i+1);
        buffer[1].vertex2[1] = getY(j+1);
        buffer[1].vertex2[2] = getZ(i+1,j+1);
        buffer[1].vertex3[0] = getX(i);
        buffer[1].vertex3[1] = getY(j+1);
        buffer[1].vertex3[2] = getZ(i,j+1);
        calcNormal(buffer+1);

        if((retVal = write(outfile,buffer,100))!=100){
          fprintf(stderr,"Write failed.\n");exit(-1);
        }
      }
    }
    // Time for the boundary/padding triangles

    // Add the 5mm padding at the bottom


  printf("Program finished.\n");
  free(data); data = NULL;
  return 0;
}
