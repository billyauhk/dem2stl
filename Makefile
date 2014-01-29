all: dem2stl sphere2off earth2off
dem2stl: dem2stl.c
	gcc dem2stl.c -o dem2stl -lm
sphere2off: sphere2off.cpp
	g++ sphere2off.cpp -o sphere2off -frounding-math -lCGAL -lboost_thread -lmpfr
earth2off: earth2off.cpp
	g++ earth2off.cpp -o earth2off -frounding-math -lCGAL -lboost_thread -lmpfr -lm
