CC = g++
all: GoGame_serial GoGame_root GoGame_vs GoGame_leaf

GoGame_serial:
	$(CC) GoGame_serial.cpp -g -O3 -std=c++11 -o GoGame_serial
GoGame_root:
	$(CC) GoGame_root.cpp -g -O3 -fopenmp -std=c++11 -o GoGame_root
GoGame_vs:
	$(CC) GoGame_vs.cpp -g -O3 -fopenmp -std=c++11 -o GoGame_vs
GoGame_leaf:
	$(CC) GoGame_leaf.cpp -g -O3 -fopenmp -std=c++11 -o GoGame_leaf

clean:
	rm -fr GoGame_serial GoGame_root GoGame_vs GoGame_leaf	