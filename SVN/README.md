# GoGame
Simple Go Game with MCTS implemented with pure C++, using different technologies to implement root parallelization.


# Description
## What is in this project?
There are 5 implementations of the MCTS: 
- Serial implementation
- OpenMp implementation using root parallelization
- MPI implementation using root parallelization
- PThreads implementation using root parallelization
- Hybrid (MPI + OpenMp)


## Requirements
- Boost library is needed for MPI implementation (to serialize the objects and send them via MPI_Send). The version 1.72.0 should work fine and can be downloaded from here:
https://dl.bintray.com/boostorg/release/1.72.0/source/boost_1_72_0.tar.gz
In makefile there is a rule build_boost that might be used to compile only serialization library (which is needed)


# Changelog
* Fixed coding style to match llvm coding style (similar to linux kernel coding style)
* Fixed diff to match between serial and parallel implementation
* Added a way to enable/disable printf to stdout (use LOG constant)
* Fixed Makefile to run on g++ (GNU version) and added dependencies to rules
* Added entry for GoGame_leaf in Makefile
* Added documentation
* Root parallelization using OMP
* Use compiling directives to enable/disable prints

* Mpi implementation: classes made serializable for broadcasting data among the processes (using boost library), thread 0 is the master and the other ones are slaves.
* Done benchmarking for openmp implementation and the results are realy good (on 8 threads, the average is between 7-8 at the same time, and also there are no deadlocks or race conditions)
* Fixed load balance, by using time limit instead of a number of iterations (for both openmp and mpi)

* Added pthreads implementation using a thread pool (implemented by hand) to avoid the overhead during thread creation and destruction
* Added a hybrid implementation using MPI and OpenMP.

