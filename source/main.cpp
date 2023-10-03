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

static int is_serv;

void full_lock(){
    pthread_t generator;
    pthread_mutex_t lock;

    pthread_mutex_init(&lock, NULL);

    if(!is_serv){
        pthread_create(&generator, NULL, full_lock_generator, &lock);

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

}
 
int main(int argc, char **argv) {

    //Extract command line arguments
    if(argc != 2){
        printf("Need 1 arg");
        return 0;
    }

    is_serv = atoi(argv[1]);

    srand((unsigned)time(NULL));

    init_state();

    //Implementation
    full_lock();

    return 0;
}
