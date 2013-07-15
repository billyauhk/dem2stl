/* dem2stl.c -- convert DEM from Daniele to STL files
                for 3D-printing
   This code is highly hard-coded for the said purpose...
   as I don't mean to make it general purpose,
   but would still demonstrates my defensive habit.

   Output model unit is specified here to be 1.0 mm
   And a 5-mm back-plate will be added so that it does not fall apart
*/

/* Invocation:
  ./dem2stl skip vertfactor scale north south east west
        skip -- (rude) downsampling figure, default 10
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
(It seems dem_newpar.par is better than dem.par we were using)
Unit Name: Meter, Z-multiplier: 1
No Data Value: -9999
Data offset, Line offset, Line endset: all 0 bytes
Data type: 4 Byte Floating Point
Byte order: Big Endian (Motorola)
Line order: Top to bottom
*/
#define countX 13639           //Cell Count X: 13639
#define countY 9547            //Cell Count Y: 9547
#define cellSize 4.5454545e-05 //Cell Size
#define startX 113.82414054f   //Left Border X
#define startY 22.57197092f    //Lower Border Y
#define padding 5.0f           //Padding depth 5mm

float* data;
unsigned int skip = 10;
float vertfact = 10.0f;
float scale = 1.0f;
float north, south, east, west;

#define deg2rad (3.1415926f/180.0f)
// Generate X coordinate and Y coordinate
// with a simple formula without consideration to ellipsoid/spherical
// but the scale correction for central parallel/meridian only
float getX(int index){
  float ans;
  ans = (index-countX/2)*60*1852*cellSize;
  ans *= cos(22*deg2rad)/scale;
  return ans;
}

float getY(int index){
  float ans;
  ans = (-index+countY/2)*60*1852*cellSize;
  ans *= cos(22*deg2rad)/scale;
  return ans;
}

float getZ(unsigned int x, unsigned int y){
  float ans;
  char sign;
  if(x<0 || x>=countX)fprintf(stderr,"Strange X: %u",x);
  if(y<0 || y>=countY)fprintf(stderr,"Strange Y: %u",y);
  ans = data[y*countX+x];  // Get the value
  sign = (ans>1.0f);
  ans = ans/1852.0f*10.0f; // Convert to 1.0 nm/cm
  ans *= vertfact/scale;   // Consider scale and vertical exaggeration
  if(sign) ans += padding*0.1f; // Add some padding if it is land
  return ans+padding;
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
  unsigned int i,j;           // Loop indices
  int xa, xb, ya, yb;          // Boundary values
  int infile, outfile, retVal; // FD + return value tmp
  unsigned int total_size;

  char headerLine[80];         // STL file header line
  unsigned int totalTriangle; // Total # of triangles
  triangle buffer[2];          // Buffering two triangles

// Read Arguments
  if(argc > 1){
    skip = atoi(argv[1]);
  }
  printf("Down-sampling ratio: %u\n",skip);
  if(argc > 2){
    vertfact = atof(argv[2]);
  }
  printf("Vertical exaggeration: %.2f\n", vertfact);
  if(argc > 3){
    scale = atof(argv[3]);
  }
  printf("Scale: %.2f nm/cm at model center\n", scale);

  west = startX;
  east = countX*cellSize+startX;
  north = startY;
  south = startY-countY*cellSize;

  if(argc > 7){
    north = atof(argv[4]);
    south = atof(argv[5]);
    east = atof(argv[6]);
    west = atof(argv[7]);
    if((north<south) || (east<west)){
      fprintf(stderr,"The order is north south east west.\n");
      fprintf(stderr,"And HK is on the North and East Hemisphere...\n");
      exit(-1);
    }
  }else if(argc > 4){
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
  xa = ceilf((west-startX)/cellSize);
  xb = floorf((east-startX)/cellSize);
  // startY - lat as the file is scanned north to south
  ya = ceilf((startY-north)/cellSize);
  yb = floorf((startY-south)/cellSize);
  // Boundary handling
  if(xa < 0) xa=0;
  if(xb >= countX) xb = (countX-1);
  if(ya < 0) ya=0;
  if(yb >= countX) yb = (countY-1);
  printf("Boundary cell numbers (x: %d, %d; y: %d, %d).\n",xa,xb,ya,yb);
  printf("Estimated size of the model: %.0f mm x %.0f mm\n",ceilf(getX(xb)-getX(xa)),ceilf(getY(ya)-getY(yb)));
  printf("That should be smaller than 203mm x 152mm x 152 mm (uPrint SE)\n");

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
    totalTriangle = 0;
    totalTriangle += (yb-ya)/skip*(xb-xa)/skip*2;     /* Topography triangles */
    totalTriangle += ceilf(((yb-ya)+(xb-xa))/skip)*4; /* 4-sides of base */
    totalTriangle += (yb-ya)/skip*(xb-xa)/skip*2;     /* Bottom of base  */
    printf("Expected total triangles: %u\n",totalTriangle);
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
    for(i=xa;(i+skip <xb)&&(i+skip <countX);i+=skip){
      for(j=ya;(j+skip <yb)&&(j+skip <countY);j+=skip){
        // Notice, larger i is on the East and larger j is pointing South
        // First triangle: (i,j) -> (i,j+1) -> (i+1, j)
        buffer[0].vertex1[0] = getX(i);
        buffer[0].vertex1[1] = getY(j);
        buffer[0].vertex1[2] = getZ(i,j);
        buffer[0].vertex2[0] = getX(i);
        buffer[0].vertex2[1] = getY(j+skip);
        buffer[0].vertex2[2] = getZ(i,j+skip);
        buffer[0].vertex3[0] = getX(i+skip);
        buffer[0].vertex3[1] = getY(j);
        buffer[0].vertex3[2] = getZ(i+skip,j);
        calcNormal(buffer+0);

        // Second triangle: (i+1, j) -> (i, j+1) -> (i+1, j+1)
        buffer[1].vertex1[0] = getX(i+skip);
        buffer[1].vertex1[1] = getY(j);
        buffer[1].vertex1[2] = getZ(i+skip,j);
        buffer[1].vertex2[0] = getX(i);
        buffer[1].vertex2[1] = getY(j+skip);
        buffer[1].vertex2[2] = getZ(i,j+skip);
        buffer[1].vertex3[0] = getX(i+skip);
        buffer[1].vertex3[1] = getY(j+skip);
        buffer[1].vertex3[2] = getZ(i+skip,j+skip);
        calcNormal(buffer+1);

        if((retVal = write(outfile,buffer,100))!=100){
          fprintf(stderr,"Write failed.\n");exit(-1);
        }
      }
    }

    // Time for the boundary/padding triangles
    // Padding along parallels
    for(i=xa;(i+skip <xb)&&(i+skip <countX);i+=skip){
      for(j=ya;j<=yb;j+=(yb-ya)/skip*skip){
        // First triangle
        buffer[0].vertex1[0] = getX(i);
        buffer[0].vertex1[1] = getY(j);
        buffer[0].vertex1[2] = 0;
        buffer[0].vertex2[0] = getX(i+skip);
        buffer[0].vertex2[1] = getY(j);
        buffer[0].vertex2[2] = 0;
        buffer[0].vertex3[0] = getX(i);
        buffer[0].vertex3[1] = getY(j);
        buffer[0].vertex3[2] = getZ(i,j);
        calcNormal(buffer+0);

        // Second triangle
        buffer[1].vertex1[0] = getX(i+skip);
        buffer[1].vertex1[1] = getY(j);
        buffer[1].vertex1[2] = 0;
        buffer[1].vertex2[0] = getX(i+skip);
        buffer[1].vertex2[1] = getY(j);
        buffer[1].vertex2[2] = getZ(i+skip,j);
        buffer[1].vertex3[0] = getX(i);
        buffer[1].vertex3[1] = getY(j);
        buffer[1].vertex3[2] = getZ(i,j);
        calcNormal(buffer+1);

        if((retVal = write(outfile,buffer,100))!=100){
          fprintf(stderr,"Write failed.\n");exit(-1);
        }
      }
    }
    // Padding along meridians
    for(j=ya;(j+skip <yb)&&(j+skip <countY);j+=skip){
      for(i=xa;i<=xb;i+=(xb-xa)/skip*skip){
        // Notice, larger i is on the East and larger j is pointing South
        // First triangle
        buffer[0].vertex1[0] = getX(i);
        buffer[0].vertex1[1] = getY(j);
        buffer[0].vertex1[2] = 0;
        buffer[0].vertex2[0] = getX(i);
        buffer[0].vertex2[1] = getY(j+skip);
        buffer[0].vertex2[2] = 0;
        buffer[0].vertex3[0] = getX(i);
        buffer[0].vertex3[1] = getY(j);
        buffer[0].vertex3[2] = getZ(i,j);
        calcNormal(buffer+0);

        // Second triangle
        buffer[1].vertex1[0] = getX(i);
        buffer[1].vertex1[1] = getY(j+skip);
        buffer[1].vertex1[2] = 0;
        buffer[1].vertex2[0] = getX(i);
        buffer[1].vertex2[1] = getY(j+skip);
        buffer[1].vertex2[2] = getZ(i,j+skip);
        buffer[1].vertex3[0] = getX(i);
        buffer[1].vertex3[1] = getY(j);
        buffer[1].vertex3[2] = getZ(i,j);
        calcNormal(buffer+1);

        if((retVal = write(outfile,buffer,100))!=100){
          fprintf(stderr,"Write failed.\n");exit(-1);
        }
      }
    }
    // Add the 5mm padding at the bottom
    for(i=xa;(i+skip <xb)&&(i+skip <countX);i+=skip){
      for(j=ya;(j+skip <yb)&&(j+skip <countY);j+=skip){
        // First triangle
        buffer[0].vertex1[0] = getX(i);
        buffer[0].vertex1[1] = getY(j);
        buffer[0].vertex1[2] = 0;
        buffer[0].vertex2[0] = getX(i+skip);
        buffer[0].vertex2[1] = getY(j);
        buffer[0].vertex2[2] = 0;
        buffer[0].vertex3[0] = getX(i);
        buffer[0].vertex3[1] = getY(j+skip);
        buffer[0].vertex3[2] = 0;
        calcNormal(buffer+0);

        // Second triangle
        buffer[1].vertex1[0] = getX(i+skip);
        buffer[1].vertex1[1] = getY(j);
        buffer[1].vertex1[2] = 0;
        buffer[1].vertex2[0] = getX(i+skip);
        buffer[1].vertex2[1] = getY(j+skip);
        buffer[1].vertex2[2] = 0;
        buffer[1].vertex3[0] = getX(i);
        buffer[1].vertex3[1] = getY(j+skip);
        buffer[1].vertex3[2] = 0;
        calcNormal(buffer+1);

        if((retVal = write(outfile,buffer,100))!=100){
          fprintf(stderr,"Write failed.\n");exit(-1);
        }
      }
    }

  printf("Program finished.\n");
  free(data); data = NULL;
  return 0;
}
