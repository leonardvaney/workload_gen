PROGRAM = workload

$(PROGRAM) : main.o
	g++ main.o -o $(PROGRAM)

main.o : main.cpp
	g++ -c main.cpp -lpthread
	
clean:
	rm *.o $(PROGRAM)
