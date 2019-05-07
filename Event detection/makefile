ALL: output
SHELL=/bin/sh
DIRS = 
assign2 : assign2.c
	mpicc -o assign2 assign2.c
output : assign2
	mpirun -np 21 assign2
clean : 
	rm assign2 output.txt 
