CC = icpc

all: GoGame_serial GoGame_root GoGame_vs GoGame_leaf GoGame_root_app

GoGame_serial: GoGame_serial.cpp
	$(CC) GoGame_serial.cpp -g -O3 -std=c++11 -o GoGame_serial

GoGame_root: GoGame_root.cpp
	$(CC) GoGame_root.cpp -g -O3 -fopenmp -std=c++11 -o GoGame_root

GoGame_root_app: GoGame_root_app.cpp
	$(CC) GoGame_root_app.cpp -g -O0 -qopenmp -std=c++11 -o GoGame_root_app

GoGame_vs: GoGame_vs.cpp
	$(CC) GoGame_vs.cpp -g -O3 -fopenmp -std=c++11 -o GoGame_vs

GoGame_leaf: GoGame_leaf.cpp
	$(CC) GoGame_leaf.cpp -g -O3 -fopenmp -std=c++11 -o GoGame_leaf

clean:
	rm -fr GoGame_serial GoGame_root GoGame_vs GoGame_leaf	