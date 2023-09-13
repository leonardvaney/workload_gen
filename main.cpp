#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>

#define THREADS 1
#define TRANSACTIONS 50000000
#define CELLS_POWER 10
#define CELLS 1 << CELLS_POWER

struct lock_t {
    pthread_mutex_t mutex;
};

struct cell_t {
    struct lock_t lock;
    int value;
};

struct coroutine_args {
    int total_transaction;
    struct cell_t* cells; 
};

void* generate_workload(void* transaction_args) {
    struct coroutine_args* args = (struct coroutine_args*)transaction_args;

    
    for(int i = 0; i < args->total_transaction; ++i){
        uint random_value = rand();
        uint cell_number = (random_value << (32 - CELLS_POWER)) >> (32 - CELLS_POWER);
        
        pthread_mutex_lock(&((&args->cells[cell_number].lock)->mutex));
        args->cells[cell_number].value = args->cells[cell_number].value + 1;
        pthread_mutex_unlock(&((&args->cells[cell_number].lock)->mutex));
    }
}

int main() {

    pthread_t handlers[THREADS];
    struct cell_t all_cells[CELLS];

    srand((unsigned)time(NULL));

    for(int i = 0; i < CELLS; ++i) {
        all_cells[i].value = 0;
        pthread_mutex_init(&((&all_cells[i].lock)->mutex), NULL);
    }

    struct coroutine_args args = { TRANSACTIONS/THREADS, all_cells};

    clock_t begin = clock();

    for(int i = 0; i < THREADS; ++i) {
        pthread_create(&handlers[i], NULL, generate_workload, (void*)&args);
    }
    
    for(int i = 0; i < THREADS; ++i) {
        pthread_join(handlers[i], NULL);
    }

    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    
    uint total_value = 0;
    for(int i = 0; i < CELLS; ++i){
        total_value += all_cells[i].value;
        //printf("%u \n", all_cells[i].value);
    }
    printf("Total value: %u \n", total_value);
    printf("Time spent: %f \n", time_spent);
}
