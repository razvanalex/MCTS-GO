CC = g++

all: GoGame_serial GoGame_root GoGame_vs GoGame_leaf GoGame_root_omp GoGame_root_mpi


GoGame_serial: GoGame_serial.cpp
	$(CC) GoGame_serial.cpp -g -O3 -std=c++11 -o GoGame_serial

GoGame_root_omp: GoGame_root_omp.cpp
	$(CC) GoGame_root_omp.cpp -g -O0 -fopenmp -std=c++11 -o GoGame_root_omp

GoGame_root_mpi: GoGame_root_mpi.cpp
	mpiicpc GoGame_root_mpi.cpp -O0 -std=c++11 -o GoGame_root_mpi -lboost_serialization
	
GoGame_root_lpthreads: GoGame_root_lpthreads.cpp
	$(CC) GoGame_root_lpthreads.cpp -O0 -g -std=c++11 -o GoGame_root_lpthreads -lpthread

GoGame_root_hybrid: GoGame_root_hybrid.cpp
	mpiicpc GoGame_root_hybrid.cpp -O3 -fopenmp -std=c++11 -o GoGame_root_hybrid -lboost_serialization


GoGame_root: GoGame_root.cpp
	$(CC) GoGame_root.cpp -g -O3 -fopenmp -std=c++11 -o GoGame_root

GoGame_vs: GoGame_vs.cpp
	$(CC) GoGame_vs.cpp -g -O3 -fopenmp -std=c++11 -o GoGame_vs

GoGame_leaf: GoGame_leaf.cpp
	$(CC) GoGame_leaf.cpp -g -O3 -fopenmp -std=c++11 -o GoGame_leaf


clean:
	rm -fr GoGame_serial GoGame_root GoGame_vs GoGame_leaf GoGame_root_omp GoGame_root_mpi
