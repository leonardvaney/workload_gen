#include <arpa/inet.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <malloc.h>
#include <unistd.h>
#include <stdint.h>
#include <atomic>


struct coroutine_args {
    std::atomic_flag* flag;
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

        while (args->flag->test_and_set()) {} //Spin in case of copy over internet

        size_t cell = args->transaction_set[i%args->transaction_set_size];
        args->cells[cell] += 1;
        
        total_value += 1;

        args->flag->clear();

        //Check if elapsed time is bigger than 1s
        /*clock_gettime(CLOCK_REALTIME, &now);
        double time_spent = (double)(now.tv_sec - begin.tv_sec);
        if(time_spent >= 2.0){        

            printf("Number of transactions after 1s: %u \n", (total_value - previous_total_value));

            previous_total_value = total_value;
            total_value = 0;
            clock_gettime(CLOCK_REALTIME, &begin);    
        }*/
    }
}

void server_receive(int sockfd, void* buffer, size_t buffer_size){
    size_t total_read = 0;
    size_t block = 0; 
    char* new_buffer = (char*)buffer;

    while(total_read != buffer_size){
        new_buffer = new_buffer + block;
        block = read(sockfd, (void*)new_buffer, buffer_size - total_read);
        total_read += block;
        printf("read: %ld vs buffer size: %ld \n", total_read, buffer_size);
    }
}

void client_send(int sockfd, void* buffer, size_t buffer_size){
    int x = write(sockfd, buffer, buffer_size);
    printf("write: %d \n", x);
}
 

int main(int argc, char **argv) {

    //Extract command line arguments
    if(argc != 5){
        printf("Need 4 args");
        return 0;
    }

    const uint threads = atoi(argv[1]);
    const size_t cells_size = atol(argv[2])/threads;
    const uint transaction_set_size = atoi(argv[3])/threads;
    const uint is_serv = atoi(argv[4]);
    

    pthread_t handlers[threads];
    coroutine_args args[threads];
    size_t** transaction = (size_t**)malloc(sizeof(size_t*) * threads);
    uint** all_cells = (uint**)malloc(sizeof(uint*) * threads);
    std::atomic_flag* flags_array = (std::atomic_flag*)malloc(sizeof(std::atomic_flag) * threads);
    srand((unsigned)time(NULL));

    //Init memory
    for(uint i = 0; i < threads; ++i){
        all_cells[i] = (uint*)malloc(sizeof(uint) * cells_size);
        for(size_t j = 0; j < cells_size; ++j){
            all_cells[i][j] = 0;
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

    //Init atomic flags
    for(uint i = 0; i < threads; ++i){
        flags_array[i].clear();
    }


    //Thread creation
    for(uint i = 0; i < threads; ++i) {
        args[i] = { flags_array+i, transaction_set_size, transaction[i], all_cells[i] };        
        pthread_create(&handlers[i], NULL, execute_workload, (void*)&args[i]);
    }


    sleep(1);

    //Claim all flags before moving forward
    for(uint i = 0; i < threads; ++i){
        while((flags_array+i)->test_and_set()) {}
    }


    //Server
    if(is_serv == 1){
        int sockfd, connfd, len;
        struct sockaddr_in servaddr, cli;
    
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if(sockfd == -1){
            printf("socket creation failed \n");
        }

        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servaddr.sin_port = htons(9998);
    
        if((bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr))) != 0){
            printf("socket bind failed \n");
        }
    
        if((listen(sockfd, 5)) != 0){
            printf("listen failed \n");
        }

        len = sizeof(cli);
    
        connfd = accept(sockfd, (struct sockaddr*)&cli, (socklen_t*)&len);
        if(connfd < 0){
            printf("server accept failed \n");
        }
        else{
            for(int i = 0; i < threads; ++i){
                server_receive(connfd, all_cells[i], cells_size*sizeof(uint));
            }
        }

        close(sockfd);
    }

    //Client
    if(is_serv == 0){
        int sockfd, connfd;
        struct sockaddr_in servaddr, cli;
    
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if(sockfd == -1){
            printf("socket creation failed \n");
        }

        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
        servaddr.sin_port = htons(9998);
    
        if(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) != 0){
            printf("connection with the server failed \n");
        }
        else{
            for(int i = 0; i < threads; ++i){
                client_send(sockfd, all_cells[i], cells_size*sizeof(uint));
            }
        }

        close(sockfd);
    }




    for(uint i = 0; i < threads; ++i){
        for(size_t j = 0; j < cells_size; ++j){
            printf("%u \n", all_cells[i][j]);
        }
    }

    //Release all flags
    for(uint i = 0; i < threads; ++i){
        (flags_array+i)->clear();
    }
    

    return 0;




    //Time and value checks    
    /*uint total_value = 0;
    uint previous_total_value = 0;

    struct timespec begin, now;
    clock_gettime(CLOCK_REALTIME, &begin);*/


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
