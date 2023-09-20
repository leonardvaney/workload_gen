#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>

#define THREADS 2
#define CELLS_POWER 25
#define CELLS (1 << CELLS_POWER)
#define TRANSACTION_SET_SIZE 10000


struct coroutine_args {
    uint* transaction_set;
    uint* cells;
    };

void* execute_workload(void* transaction_args) {
    struct coroutine_args* args = (struct coroutine_args*)transaction_args;
    
    for(uint i = 0;; ++i){
        uint cell = args->transaction_set[i%TRANSACTION_SET_SIZE];
        args->cells[cell] += 1;
    }
}
 

int main() {

    pthread_t handlers[THREADS];
    coroutine_args args[THREADS];
    uint** transaction = (uint**)malloc(sizeof(uint*) * THREADS);
    uint** all_cells = (uint**)malloc(sizeof(uint*) * THREADS);
    srand((unsigned)time(NULL));

    //Init memory
    for(uint i = 0; i < THREADS; ++i){
        all_cells[i] = (uint*)malloc(sizeof(uint) * CELLS);
        for(uint j = 0; j < CELLS; ++j){
            all_cells[i][j] = 0;
        }
    }

    //Init transaction set
    for(uint i = 0; i < THREADS; ++i){
        transaction[i] = (uint*)malloc(sizeof(uint) * TRANSACTION_SET_SIZE);
        for(uint j = 0; j < TRANSACTION_SET_SIZE; ++j){
            uint random_value = rand();
            uint cell_number = random_value%CELLS;
            transaction[i][j] = cell_number;
        }
    }

    //Thread creation
    for(uint i = 0; i < THREADS; ++i) {
        args[i] = { transaction[i], all_cells[i] };        
        pthread_create(&handlers[i], NULL, execute_workload, (void*)&args[i]);
    }

    //Time and value checks    
    uint total_value = 0;
    uint previous_total_value = 0;

    struct timespec begin, now;
    clock_gettime(CLOCK_REALTIME, &begin);

    //Check in an infinite loop if elapsed time is bigger than 1s
    for(;;){
        clock_gettime(CLOCK_REALTIME, &now);
        double time_spent = (double)(now.tv_sec - begin.tv_sec);
        if(time_spent >= 1.0){
            for(uint i = 0; i < THREADS; ++i){
                for(uint j = 0; j < CELLS; ++j){
                    total_value += all_cells[i][j];
                }
            }        

            printf("Number of transactions after 1s: %u \n", (total_value - previous_total_value));

            previous_total_value = total_value;
            total_value = 0;
            clock_gettime(CLOCK_REALTIME, &begin);
        }
        
    }
}
