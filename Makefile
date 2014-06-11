all: dem2stl sphere2off earth2off
etopo1_ice_c_i2:
	#wget http://www.ngdc.noaa.gov/mgg/global/relief/ETOPO1/data/ice_surface/cell_registered/binary/etopo1_ice_c_i2.zip
	unzip etopo1_ice_c_i2.zip
dem2stl: dem2stl.c
	gcc dem2stl.c -o dem2stl -lm
sphere2off: sphere2off.cpp
	g++ sphere2off.cpp -o sphere2off -frounding-math -lCGAL -lboost_thread -lmpfr
earth2off: earth2off.cpp
	g++ earth2off.cpp -o earth2off -frounding-math -lCGAL -lboost_thread -lmpfr -lm
