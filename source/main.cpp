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

        printf("Wait 5 sec\n");
        sleep(5);

        init_client();
        pthread_mutex_lock(&lock);
        send_state();

        pthread_mutex_unlock(&lock);

        printf("Wait 5 sec\n");
        sleep(5);

        //close_client();
    }
    else{
        init_server();
        pthread_mutex_lock(&lock);
        receive_state();

        pthread_mutex_unlock(&lock);

        //close_server();
    }
}

void progressive_lock(){
    pthread_t generator;
    pthread_mutex_t* state_locks = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t)*(STATE_SUBPART + 1)); //+ 1 to have a global lock
    
    for(size_t i = 0; i < STATE_SUBPART+1; ++i){
        pthread_mutex_init(&(state_locks[i]), NULL);
    }

    if(!is_serv){
        pthread_create(&generator, NULL, progressive_lock_generator, &state_locks);

        printf("Wait 5 sec\n");
        sleep(5);

        init_client();

        pthread_mutex_lock(&(state_locks[STATE_SUBPART]));
        
        for(size_t i = 0; i < STATE_SUBPART; ++i){
            pthread_mutex_lock(&(state_locks[i]));
        }

        pthread_mutex_unlock(&(state_locks[STATE_SUBPART]));

        send_state_progressive_lock(state_locks);

        printf("Wait 5 sec\n");
        sleep(5);
    }
    else{
        init_server();
        receive_state();
    }
}

void rw_lock(){
    pthread_t generator, copy;
    pthread_mutex_t* locks = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t)*2);

    pthread_mutex_init(&(locks[0]), NULL); //Internal copy lock
    pthread_mutex_init(&(locks[1]), NULL); //Send lock

    if(!is_serv){
        pthread_create(&generator, NULL, rw_lock_generator, &locks);

        printf("Wait 5 sec\n");
        sleep(5);

        init_client();
        
        //pthread_mutex_lock(&(locks[0]));

        change_rw_bit();
        pthread_create(&copy, NULL, copy_data, &locks);
        
        send_state_rw_lock();

        //pthread_mutex_unlock(&lock);

        printf("Wait 5 sec\n");
        sleep(5);

        //close_client();
    }
    else{
        init_server();
        //pthread_mutex_lock(&lock);
        receive_state_rw_lock();

        //pthread_mutex_unlock(&lock);

        //close_server();
    }
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
    //full_lock();
    //progressive_lock();
    rw_lock();

    return 0;
}
