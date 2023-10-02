#include <arpa/inet.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <malloc.h>
#include <unistd.h>
#include <stdint.h>
#include <atomic>

#include <state.hpp>
#include <generator.hpp>
#include <transfer.hpp>


static pthread_mutex_t lock;

void* generator_coroutine(void* args){
    batch_t batch = generate_batch();
    uint32_t epoch = 0;
    for(;;){
        pthread_mutex_lock(&lock);
        ++epoch;
        execute_batch(&batch, epoch);
        pthread_mutex_unlock(&lock);
    }
}
 
int main(int argc, char **argv) {

    //Extract command line arguments
    if(argc != 2){
        printf("Need 1 arg");
        return 0;
    }

    const uint is_serv = atoi(argv[1]);
    pthread_t generator, transfer;
    pthread_mutex_init(&lock, NULL);

    srand((unsigned)time(NULL));

    init_state();

    if(!is_serv){
        pthread_create(&generator, NULL, generator_coroutine, NULL);

        printf("Wait 2 sec\n");
        sleep(2);

        init_client();
        pthread_mutex_lock(&lock);
        send_state();
    }
    else{
        init_server();
        pthread_mutex_lock(&lock);
        receive_state();
    }

    pthread_mutex_unlock(&lock);

    return 0;
}
