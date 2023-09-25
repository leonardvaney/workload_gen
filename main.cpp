#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <malloc.h>
#include <unistd.h>
#include <stdint.h>

//#define THREADS 2
//#define CELLS_POWER 25
//#define CELLS (1 << CELLS_POWER)
//#define TRANSACTION_SET_SIZE 10000


struct coroutine_args {
    uint transaction_set_size;
    size_t* transaction_set;
    uint* cells;
    };

void* execute_workload(void* transaction_args) {
    struct coroutine_args* args = (struct coroutine_args*)transaction_args;

    //Time and value checks    
    uint total_value = 0;
    uint previous_total_value = 0;

    struct timespec begin, now;
    clock_gettime(CLOCK_REALTIME, &begin);

    for(size_t i = 0;; ++i){
        size_t cell = args->transaction_set[i%args->transaction_set_size];
        args->cells[cell] += 1;
        
        /*total_value += 1;

        //Check if elapsed time is bigger than 1s
        clock_gettime(CLOCK_REALTIME, &now);
        double time_spent = (double)(now.tv_sec - begin.tv_sec);
        if(time_spent >= 2.0){        

            printf("Number of transactions after 1s: %u \n", (total_value - previous_total_value));

            previous_total_value = total_value;
            total_value = 0;
            clock_gettime(CLOCK_REALTIME, &begin);    
        }*/
    }
}
 

int main(int argc, char **argv) {

    //Extract command line arguments
    if(argc != 4){
        printf("Need 3 args");
        return 0;
    }

    const uint threads = atoi(argv[1]);
    const size_t cells_size = atol(argv[2])/threads;
    const uint transaction_set_size = atoi(argv[3])/threads;
    

    pthread_t handlers[threads];
    coroutine_args args[threads];
    size_t** transaction = (size_t**)malloc(sizeof(size_t*) * threads);
    uint** all_cells = (uint**)malloc(sizeof(uint*) * threads);
    srand((unsigned)time(NULL));

    //Init memory
    for(uint i = 0; i < threads; ++i){
        all_cells[i] = (uint*)malloc(sizeof(uint) * cells_size);
        for(size_t j = 0; j < cells_size; ++j){
            all_cells[i][j] = 1;
        }
    }

    printf("%lu \n", threads * malloc_usable_size(all_cells[0]));

    //Init transaction set
    for(uint i = 0; i < threads; ++i){
        transaction[i] = (size_t*)malloc(sizeof(size_t) * transaction_set_size);
        for(uint j = 0; j < transaction_set_size; ++j){
            size_t random_value = rand();
            double r = (double)random_value;
            size_t cell_number = (size_t)((r / RAND_MAX) * cells_size);
            transaction[i][j] = cell_number;
        }
    }

    //Thread creation
    for(uint i = 0; i < threads; ++i) {
        args[i] = { transaction_set_size, transaction[i], all_cells[i] };        
        pthread_create(&handlers[i], NULL, execute_workload, (void*)&args[i]);
    }

    //Time and value checks    
    /*uint total_value = 0;
    uint previous_total_value = 0;

    struct timespec begin, now;
    clock_gettime(CLOCK_REALTIME, &begin);*/


    sleep(100000);
    //Check in an infinite loop if elapsed time is bigger than 1s
    /*for(;;){
        clock_gettime(CLOCK_REALTIME, &now);
        double time_spent = (double)(now.tv_sec - begin.tv_sec);
        if(time_spent >= 1.0){
            for(uint i = 0; i < threads; ++i){
                for(uint j = 0; j < cells_size; ++j){
                    total_value += all_cells[i][j];
                }
            }        

            printf("Number of transactions after 1s: %u \n", (total_value - previous_total_value));

            previous_total_value = total_value;
            total_value = 0;
            clock_gettime(CLOCK_REALTIME, &begin);
        }
        
    }*/
}
