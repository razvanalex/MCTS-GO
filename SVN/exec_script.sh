#/bin/bash

RUN_MODE="SERIAL"

TIME_SIM=0.1
TIME_PLAY=0.01
NUM_THREADS=1

# Run
if [ $RUN_MODE = "SERIAL" ]; then
	echo "Running SERIAL"
	make run_serial TIME_SIM=$TIME_SIM TIME_PLAY=$TIME_PLAY
elif [ $RUN_MODE = "OMP" ]; then
	echo "Running OMP"
	make run_omp
elif [ $RUN_MODE = "PTHREAD" ]; then
	echo "Running PTHREADS"
	make run_lpthreas
elif [ $RUN_MODE = "MPI" ]; then
	echo "Running MPI"
	make run_mpi
elif [ $RUN_MODE = "HYBRID" ]; then
	echo "Running HYBRID"
	make run_hybrid
fi
