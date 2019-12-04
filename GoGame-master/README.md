# GoGame
Simple Go Game with MCTS implemented with pure C++, using OpenMP to implement tree parallel and root parallel.
The makefile use intel C++ complier, it is OK to use g++ for the serial version of MCTS


# Description
## What is in this project?
There are 4 implementations of the MCTS: 
- [OK] serial code: A serial implementation of the game, where one bot uses MCTS, the other plays very predictable (next valid cell)
- root: A parallel implementation that uses Root parallelization [CRASHES FOR MULTIPLE THREADS; board becomes somehow empty]
- [OK] leaf: A parallel implementation that uses Leaf parallelization 
- [OK] vs: Implements random, serial, roof and leaf and let the bots play with each other. This implementation uses time as maximum level in monte carlo simulation


# Changelog
* Fixed coding style to match llvm codding style (similar to linux kernel coding style)
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


**Note**: For compiling using icc, change from -fopenmp to -openmp in Makefile (for running on cluster)

**UPDATED:** Makefile uses icc. To use gcc/g++ change CC and for omp, change the flag from -qopenmp (for intel compiler)
to -fopenmp

**UPDATED v2:** Makefile uses gcc, again. To use icc change CC and for omp, change the flag from -fopenmp to -qopenmp


# TODO:
  - pana la finalul saptamanii trebuie sa fie gata openMP + profiling [DONE]
- pe 14 dec trebuie sa fie gata proiectul 
- pe saptamana urmatoare testare si fine tuning
- data viitoare o alta paralelizare
- implementare hibrida = bonus
