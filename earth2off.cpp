// sphere2stl.cpp 
/* A temporary folder for generating a STL file for printing of the Earth.
   Using ETOPO1 data from NGDC, ice-surface cell-registered two-byte integers
   Work on the earth directly. Depends on libCGAL (thus boost_thread + MPFR)
   Caution: Heavy hard-code ahead!
*/

#include <linux/types.h>
#include <stdio.h>
#include <CGAL/Surface_mesh_default_triangulation_3.h>
#include <CGAL/Complex_2_in_triangulation_3.h>
#include <CGAL/make_surface_mesh.h>
#include <CGAL/Implicit_surface_3.h>
#include <CGAL/IO/Complex_2_in_triangulation_3_file_writer.h>
#include <fstream>

// default triangulation for Surface_mesher
typedef CGAL::Surface_mesh_default_triangulation_3 Tr;

// c2t3
typedef CGAL::Complex_2_in_triangulation_3<Tr> C2t3;

typedef Tr::Geom_traits GT;
typedef GT::Sphere_3 Sphere_3;
typedef GT::Point_3 Point_3;
typedef GT::FT FT;

typedef FT (*Function)(Point_3);

typedef CGAL::Implicit_surface_3<GT, Function> Surface_3;

int16_t *data;

#define value(lat,lon) (data[ ((uint16_t) ((90.0-lat)*60.0)) * 21600 + ((uint16_t) ((lon+180.0)*60.0) )])

#define PI 3.141592654

FT sphere_function(Point_3 p){
  const FT x2 = p.x()*p.x(), y2 = p.y()*p.y(), z2 = p.z()*p.z();
  float lat, lon, dist;
  dist = x2 + y2 + z2;
  lat = asinf(p.z()/sqrtf(dist))*90.0/(PI/2);
  lon = atan2f(p.y(),p.x())*180.0/PI;
  //printf("%f %f %f:%f %f %d\n", p.x(), p.y(), p.z(), lat, lon, value(lat,lon));
// 63.5mm -> 2.5 inches
  if(isnan(lat) || value(lat,lon)==-32768){
    return -1000;
  }else{
    return x2 + y2 + z2 - (63.5+value(lat,lon)/10000.0*6.35)*(63.5+value(lat,lon)/10000.0*6.35);
  }
}

int main(){
  FILE *infile;

  const size_t totalSize = sizeof(uint16_t)*21600*10800;

  printf("ETOPO World!\n");
  data = (int16_t*) malloc(totalSize);
  infile = fopen("etopo1_ice_c_i2.bin","r");
  if(fread((void*) data, sizeof(uint16_t), 21600*10800, infile) != 21600*10800){
    fprintf(stderr,"File read error!\n");exit(-1);
  };
  fclose(infile);

  for(float i=90;i>-90;i-=180/45){
    for(float j=-180;j<180;j+=360/90){
      printf("%c",value(i,j)>0?'+':' ');
    }
    printf("\n");
  }

  printf("CGAL World!\n");
  Tr tr;          // 3D-Delaunay triangulation
  C2t3 c2t3 (tr); // 2D-complex in 3D-Delaunay triangulation

  // Defining the surface
  Surface_3 surface(sphere_function,                     // pointer to function
                    Sphere_3(CGAL::ORIGIN, 76.2*76.2));  // bounding sphere (3-inch radius)
  // Note that "2." above is the *squared* radius of the bounding sphere!

  // Defining meshing criteria
  CGAL::Surface_mesh_default_criteria_3<Tr> criteria(30.,   // angular bound
                                                     0.3,  // radius bound
                                                     0.3); // distance bound

 // meshing surface
  CGAL::make_surface_mesh(c2t3, surface, criteria, CGAL::Non_manifold_tag());

  std::cout << "Final number of points: " << tr.number_of_vertices() << "\n";
  std::ofstream out("earth.off");
  CGAL::output_surface_facets_to_off(out, c2t3);
  std::cout << "Program completed!\n" << "\n";

  free(data);data=NULL;
  return 0;
}
