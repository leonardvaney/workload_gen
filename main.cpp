#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>

#define THREADS 8
#define TRANSACTIONS 50000000
#define CELLS_POWER 10
#define CELLS (1 << CELLS_POWER)
#define TRANSACTION_SET_SIZE 10000


struct coroutine_args {
    int total_transaction;
    int thread_number;
    int* transaction_set;
    int** cells;
    };

void* generate_workload(void* transaction_args) {
    struct coroutine_args* args = (struct coroutine_args*)transaction_args;
    
    for(int i = 0; i < args->total_transaction; ++i){
        int cell = args->transaction_set[i%TRANSACTION_SET_SIZE];
        args->cells[args->thread_number][cell] += 1;
    }
}

int main() {

    pthread_t handlers[THREADS];
    coroutine_args args[THREADS];
    int** all_cells = (int**)malloc(sizeof(int*) * THREADS);

    for(int i = 0; i < THREADS; ++i){
        all_cells[i] = (int*)malloc(sizeof(int) * CELLS);
        for(int j = 0; j < CELLS; ++j){
            all_cells[i][j] = 0;
        }
    }

    srand((unsigned)time(NULL));

    for(int i = 0; i < THREADS; ++i) {
        for(int j = 0; j < CELLS; ++j){
            all_cells[i][j] = 0;
        }
    }

    int transaction[TRANSACTION_SET_SIZE];
    for(int i = 0; i < TRANSACTION_SET_SIZE; ++i){
        uint random_value = rand();
        uint cell_number = random_value%CELLS;
        transaction[i] = cell_number;
    }

    clock_t begin = clock();

    for(int i = 0; i < THREADS; ++i) {
        args[i] = { TRANSACTIONS/THREADS, i, transaction, all_cells};        
        pthread_create(&handlers[i], NULL, generate_workload, (void*)&args[i]);
    }
    
    for(int i = 0; i < THREADS; ++i) {
        pthread_join(handlers[i], NULL);
    }

    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    
    uint total_value = 0;
    for(int i = 0; i < THREADS; ++i){
        for(int j = 0; j < CELLS; ++j){
            total_value += all_cells[i][j];
        }
        free(all_cells[i]);
    }
    free(all_cells);
    
    printf("Total value: %u \n", total_value);
    printf("Time spent: %f \n", time_spent);
}
