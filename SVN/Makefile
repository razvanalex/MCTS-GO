CC = g++
BOOST_LDPATH = ./boost_1_72_0/

all: GoGame_serial GoGame_root_omp GoGame_root_mpi GoGame_root_lpthreads GoGame_root_hybrid

build_boost:
	cd ./boost_1_72_0/; \
	./bootstrap.sh --prefix=.; \
	./b2 --with-serialization

GoGame_serial: GoGame_serial.cpp
	$(CC) GoGame_serial.cpp -O3 -std=c++11 -o GoGame_serial

GoGame_root_omp: GoGame_root_omp.cpp
	$(CC) GoGame_root_omp.cpp -O3 -fopenmp -std=c++11 -o GoGame_root_omp

GoGame_root_mpi: GoGame_root_mpi.cpp
	mpiicpc GoGame_root_mpi.cpp -O3 -std=c++11 -o GoGame_root_mpi -I${BOOST_LDPATH} -L${BOOST_LDPATH}/lib -lboost_serialization
	
GoGame_root_lpthreads: GoGame_root_lpthreads.cpp
	$(CC) GoGame_root_lpthreads.cpp -O3 -std=c++11 -o GoGame_root_lpthreads -lpthread

GoGame_root_hybrid: GoGame_root_hybrid.cpp
	mpiicpc GoGame_root_hybrid.cpp -O3 -fopenmp -std=c++11 -o GoGame_root_hybrid -I${BOOST_LDPATH} -L${BOOST_LDPATH}/lib -lboost_serialization


TIME_SIM=0.1
TIME_PLAY=0.05

# Set NUM_THREADS for omp and lpthreads and NUM_PROCS for mpi and hybrid
run_serial:
	./GoGame_serial ${TIME_SIM} ${TIME_PLAY} > out_serial

run_omp:
	./GoGame_root_omp ${TIME_SIM} ${TIME_PLAY} ${NUM_THREADS} > out_omp_${NUM_THREADS}

run_mpi:
	LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${BOOST_LDPATH}/lib/ mpirun -np ${NUM_PROCS} ./GoGame_root_mpi ${TIME_SIM} ${TIME_PLAY} > out_mpi_${NUM_PROCS}

run_lpthreas:
	./GoGame_root_lpthreads ${TIME_SIM} ${TIME_PLAY} ${NUM_THREADS} > out_pthreads_${NUM_THREADS}

run_hybrid:
	LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${BOOST_LDPATH}/lib/ mpirun -np ${NUM_PROCS} ./GoGame_root_hybrid ${TIME_SIM} ${TIME_PLAY} > out_hybrid_${NUM_PROCS}


clean:
	rm -fr GoGame_serial GoGame_root_omp GoGame_root_mpi GoGame_root_lpthreads GoGame_root_hybrid

