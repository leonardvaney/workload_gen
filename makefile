workload : main.o
	g++ main.o -o workload

main.o : main.cpp
	g++ -c main.cpp -lpthread
