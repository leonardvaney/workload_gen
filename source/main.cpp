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
#include <consensus.hpp>
#include <node.hpp>
#include <message.h>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>

extern addr_node_t* node_list;
extern uint8_t total_node;

static int is_serv;

/*std::string sha256(const std::string str){
  unsigned char hash[SHA256_DIGEST_LENGTH];

    uint32_t x = 0;
    uint32_t y = 1;
    uint32_t z = 2;

  SHA256_CTX sha256;
  SHA256_Init(&sha256);
  for(int i = 0; i < 3; ++i){
    //SHA256_Update(&sha256, &i, sizeof(uint32_t));
  }
  SHA256_Update(&sha256, &x, sizeof(uint32_t));
  SHA256_Update(&sha256, &y, sizeof(uint32_t));
  SHA256_Update(&sha256, &z, sizeof(uint32_t));
  SHA256_Final(hash, &sha256);

  std::stringstream ss;

  for(int i = 0; i < SHA256_DIGEST_LENGTH; i++){
    ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>( hash[i] );
  }
  return ss.str();
}*/

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
    pthread_mutex_t copy_lock;

    pthread_mutex_init(&copy_lock, NULL); //Internal copy lock

    if(!is_serv){
        pthread_create(&generator, NULL, rw_lock_generator, &copy_lock);

        printf("Wait 5 sec\n");
        sleep(5);

        init_client();
    
        uint8_t bit = get_rw_bit();

        pthread_create(&copy, NULL, copy_data, &copy_lock);
        
        while(bit == get_rw_bit()){
            //Wait for copy_data
        }

        send_state_rw_lock();

        printf("Wait 5 sec\n");
        sleep(5);

    }
    else{
        init_server();
        receive_state_rw_lock();
    }
}
 
int main(int argc, char **argv) {

    //Extract command line arguments
    if(argc != 2){
        printf("Need 1 arg");
        return 0;
    }

    //is_serv = atoi(argv[1]);
    uint8_t node_id = atoi(argv[1]);

    addr_node_t* n_list = (addr_node_t*)malloc(sizeof(addr_node_t) * 1);

    //Get nodes info
    FILE* file;
    char* line = NULL;
    size_t len = 0;
    ssize_t read;
    int count = 0;

    file = fopen("nodes.txt", "r");
    if (file == NULL){
        printf("can't open file \n");
        exit(EXIT_FAILURE);
    }

    while ((read = getline(&line, &len, file)) != -1) {
        count += 1;
        node_list = (addr_node_t*)realloc(node_list, sizeof(addr_node_t) * count);

        int token_nbr = 0;
        char *token = strtok(line, " ");
        node_list[count-1].id = atoi(token);
        token = strtok(NULL, " ");
        node_list[count-1].ip = strdup(token);
        token = strtok(NULL, " ");
        node_list[count-1].port = atoi(token);
    }

    fclose(file);

    /*for(int i = 0; i < count; ++i){
        printf("node: %d %s %d \n", node_list[i].id, node_list[i].ip, node_list[i].port);
    }*/

    //node_list = n_list;
    total_node = count;

    printf("node id: %d \n", node_id);

    /*for(int i = 0; i < total_node; ++i){
        printf("%d \n", node_list[i]);
    }*/



    //std::cout << sha256("Terminal Root") << '\n';

    //return 0;

    if(node_id == 0){
        printf("init consensus \n");
        init_fifo(total_node);
        init_consensus();
    }
    else{
        printf("init node \n");
        init_state();
        init_node(node_id);
    }

    //srand((unsigned)time(NULL));

    //init_state();

    //Implementation
    //full_lock();
    //progressive_lock();
    //rw_lock();

    return 0;
}
