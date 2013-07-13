default: dem2stl

dem2stl: dem2stl.c
	gcc dem2stl.c -o dem2stl -lm
