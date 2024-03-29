#include <node.hpp>

extern addr_node_t* node_list;
extern uint8_t total_node;

int msleep(long msec)
{
    struct timespec ts;
    int res;

    if (msec < 0){
        errno = EINVAL;
        return -1;
    }

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    do{
        res = nanosleep(&ts, &ts);
    } while(res && errno == EINTR);

    return res;
}

void switch_recover_mode(uint8_t id){
    pthread_mutex_lock(&node_lock);
    recover_mode = recover_mode == 1 ? 0 : 1;
    id_recover = id;
    pthread_mutex_unlock(&node_lock);
}

void accept_node_conn(int start, int end){
    printf("Wait for connections\n");

    struct sockaddr_in cli;
    int len = sizeof(cli);

    //for(int i = 0; i <= total_node-2; ++i){
    for(int i = start; i <= end; ++i){
        int connfd = accept(server_sockfd, (struct sockaddr*)&cli, (socklen_t*)&len);
        if(connfd < 0){
            printf("server accept failed with connfd = %d and server_sockfd = %d \n", connfd, server_sockfd);
        }
        else{
            connfd_list_node[i] = connfd;
        }
    }
}

void* open_client_node(void* args){
    uint8_t id = *((int*)args);
    free(args);
    struct sockaddr_in servaddr, cli;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1){
        printf("socket creation failed \n");
    }
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(node_list[id].ip);
    servaddr.sin_port = htons(node_list[id].port);
    
    while(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) != 0){
        sleep(1);
        printf("connection with the node %d failed, retry \n", id);
    }

    printf("init connection with server %d ok \n", id);

    write_socket(sockfd, (char*)&node_id, sizeof(uint8_t));

    //Wait for 
    while(true){
        pthread_mutex_lock(&node_lock);
        if(recover_mode == 1 && id_recover == id){
            //1) Need to send a hash of 2t+1 hash from 2t+1 state part (starting from part node_id)
            //2) Send t+1 state part (starting from part node_id)

            pthread_mutex_unlock(&node_lock);

            clock_gettime(CLOCK_REALTIME, &start_transfert);

            uint8_t revised_id = node_id > id_recover ? node_id-2 : node_id-1;
            uint8_t t_value = (total_node-2)/3; //-2 because we also remove the consensus node
            unsigned char* hash_result = (unsigned char*)malloc(sizeof(unsigned char) * SHA256_DIGEST_LENGTH);
            hash_msg_t* hash_msg = (hash_msg_t*)malloc(sizeof(hash_msg_t));
            uint32_t* pointeur = get_rw_bit() == 1 ? get_cells() : get_cells() + STATE_SIZE/2;

            for(int i = 0; i < 2*t_value+1; ++i){

                //Send hash here:
                uint64_t start = ((revised_id) * state_part + i*state_part) % (STATE_SIZE/2);
                uint64_t end = start + state_part;
                uint64_t offset = get_rw_bit() == 1 ? 0 : STATE_SIZE/2;
                hash_state_elements(start, end, hash_result);

                memcpy(hash_msg->hash, hash_result, sizeof(unsigned char) * SHA256_DIGEST_LENGTH);
                write_socket(sockfd, (char*)hash_msg, sizeof(hash_msg_t));

                clock_gettime(CLOCK_REALTIME, &now_transfert);

                time_diff(start_batch, now_transfert, diff_transfert);

                double start_nsec_in_ms = ((double)diff_transfert->tv_nsec / 1000000);
                double start_sec_in_ms = ((double)diff_transfert->tv_sec *1000);
                double total_time = start_sec_in_ms + start_nsec_in_ms;
            }

            free(hash_result);
            free(hash_msg);

            //Send t+1 part here:
            uint64_t start = (revised_id) * state_part;
            uint64_t end = (start + state_part*(t_value+1))%(STATE_SIZE/2);

            if(start < end){ //Send a continuous array
                write_socket(sockfd, (char*)(pointeur + start), sizeof(uint32_t)*(end-start));
            }
            else{ //Send two array in two writes
                write_socket(sockfd, (char*)(pointeur + start), sizeof(uint32_t)*(STATE_SIZE/2 - start));
                write_socket(sockfd, (char*)(pointeur), sizeof(uint32_t)*end);
            }

            clock_gettime(CLOCK_REALTIME, &now_transfert);

            time_diff(start_batch, now_transfert, diff_transfert);

            double start_nsec_in_ms = ((double)diff_transfert->tv_nsec / 1000000);
            double start_sec_in_ms = ((double)diff_transfert->tv_sec *1000);
            double total_time = start_sec_in_ms + start_nsec_in_ms;

            time_diff(start_transfert, now_transfert, diff_transfert);

            double nsec_in_sec = ((double)diff_transfert->tv_nsec / 1000000000);
            double elapsed = (double)diff_transfert->tv_sec + nsec_in_sec;

            printf("%f: Transfert time: %f \n", total_time, elapsed);


            pthread_mutex_lock(&node_lock);
            
            //Reset recover mode
            recover_mode = 0;
            
            pthread_mutex_unlock(&node_lock);
            
            break;

        }
        pthread_mutex_unlock(&node_lock);
    }

    return NULL;
}

void* listen_server_node_consensus(void* args){
    uint8_t id = 0;
    uint8_t conn_node_id = 0;
    consensus_msg_t* msg = (consensus_msg_t*)malloc(sizeof(consensus_msg_t));
    //uint8_t* idd = (uint8_t*)malloc(sizeof(uint8_t));
    //*idd = 0;

    read_socket(connfd_list_node[id], (char*)&conn_node_id, sizeof(uint8_t));
    uint8_t revised_id = conn_node_id > node_id ? conn_node_id-2 : conn_node_id-1;

    while(true){
        if(conn_node_id == 0){
            read_socket(connfd_list_node[id], (char*)(msg), sizeof(consensus_msg_t));
            msleep(ENQUEUE_DELAY);
            enqueue_message(msg);
        }
    }
}

void* listen_server_node(void* args){
    uint8_t id = *((uint8_t*)args);
    uint8_t conn_node_id = 0;
    hash_msg_t* hash_msg = (hash_msg_t*)malloc(sizeof(hash_msg_t));
    unsigned char* temp_hash = (unsigned char*)malloc(SHA256_DIGEST_LENGTH);
    uint16_t state_part_pointer = 0;
    uint16_t hash_state_pointer = 0;
    uint16_t end_of_recover = 0;
    double total_time = 0;

    read_socket(connfd_list_node[id], (char*)&conn_node_id, sizeof(uint8_t));
    uint8_t revised_id = conn_node_id > node_id ? conn_node_id-2 : conn_node_id-1;

    //Look for consensus_msg_t (sign that someone need to recover)
    while(true){
        //Received a hash or a state part from a node
        if(hash_state_pointer < 2*((total_node-2)/3)+1){ //Receive a hash

            read_socket(connfd_list_node[id], (char*)(hash_msg), sizeof(hash_msg_t));                
                
            memcpy(hash_result[revised_id][hash_state_pointer], hash_msg->hash, SHA256_DIGEST_LENGTH);
            ++hash_state_pointer;

            clock_gettime(CLOCK_REALTIME, &now_batch);

            time_diff(start_batch, now_batch, diff_batch);

            double start_nsec_in_ms = ((double)diff_batch->tv_nsec / 1000000);
            double start_sec_in_ms = ((double)diff_batch->tv_sec *1000);
            total_time = start_sec_in_ms + start_nsec_in_ms;

            printf("%f: Hash received from %d \n", total_time, conn_node_id);
        }
        else{ //Receive a state part
            if(state_part_pointer < ((total_node-2)/3)+1){

                uint64_t start_for_id = (revised_id) * state_part; //Fixed starting offset by id
                uint64_t start = (start_for_id + state_part_pointer*state_part) % (STATE_SIZE/2);
                uint64_t end = start + state_part;
                uint64_t offset = get_rw_bit() == 1 ? 0 : STATE_SIZE/2;
                uint32_t* pointeur_ro = (get_rw_bit() == 1 ? get_cells() : get_cells() + STATE_SIZE/2); //Write temp value in read-only part as it is garbage value
                uint32_t* pointeur_rw = (get_rw_bit() == 0 ? get_cells() : get_cells() + STATE_SIZE/2); //Pointeur to the place that receive the final correct values
                    
                read_socket(connfd_list_node[id], (char*)(pointeur_ro + start_for_id), sizeof(uint32_t)*(end-start));
                    
                hash_state_elements(start_for_id, start_for_id + state_part, temp_hash);

                clock_gettime(CLOCK_REALTIME, &now_batch);

                time_diff(start_batch, now_batch, diff_batch);

                double start_nsec_in_ms = ((double)diff_batch->tv_nsec / 1000000);
                double start_sec_in_ms = ((double)diff_batch->tv_sec *1000);
                total_time = start_sec_in_ms + start_nsec_in_ms;

                //printf("%f: State part received from %d \n", total_time, conn_node_id);
                    
                    //BESOIN DE COMPARER DE MANIÈRE PLUS EXHAUSTIVE
                if(strncmp((const char*)temp_hash, (const char*)hash_result[revised_id][state_part_pointer], SHA256_DIGEST_LENGTH) == 0){
                    printf("CORRECT HASH ID %d \n", conn_node_id);

                    memcpy(pointeur_rw + start, pointeur_ro + start_for_id, sizeof(uint32_t)*(end-start)); //Src is always fixed, Dst depend on state_part_pointer
                }
                else{
                    printf("Hash are not equal ID %d \n", conn_node_id);
                }

                ++state_part_pointer;
            }
            else{

                hash_state_pointer = 0;
                state_part_pointer = 0;

                pthread_mutex_lock(&node_lock);

                if(recover_mode == total_node-2){

                    printf("End of recover \n");

                    recover_mode = 0;

                    pthread_mutex_unlock(&node_lock);

                    stop_queue();

                    clock_gettime(CLOCK_REALTIME, &now_recover);

                    time_diff(start_recover, now_recover, diff_recover);

                    double nsec_in_sec = ((double)diff_recover->tv_nsec / 1000000000);
                    double elapsed_ = (double)diff_recover->tv_sec + nsec_in_sec;

                    clock_gettime(CLOCK_REALTIME, &now_batch);

                    time_diff(start_batch, now_batch, diff_batch);

                    double start_nsec_in_ms = ((double)diff_batch->tv_nsec / 1000000);
                    double start_sec_in_ms = ((double)diff_batch->tv_sec *1000);
                    total_time = start_sec_in_ms + start_nsec_in_ms;

                    printf("%f: Recovery time: %f \n", total_time, elapsed_);

                    break;
                }
                else{
                    ++recover_mode;
                }
                pthread_mutex_unlock(&node_lock);

                break;
            }
        }
    }

    close(connfd_list_node[id]);

    free(hash_msg);
    free(temp_hash);

    return NULL;
}

void* open_client_node_consensus(void* args){
    uint8_t id = 0;
    uint8_t* idd = (uint8_t*)malloc(sizeof(uint8_t)*(total_node-1));
    struct sockaddr_in servaddr, cli;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1){
        printf("socket creation failed \n");
    }
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(node_list[id].ip);
    servaddr.sin_port = htons(node_list[id].port);
    
    while(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) != 0){
        sleep(1);
        printf("connection with the node %d failed, retry \n", id);
    }

    printf("init connection with server %d ok \n", id);

    write_socket(sockfd, (char*)&node_id, sizeof(uint8_t));

    //Wait for 
    while(true){
        if(simulate_crash == 1){ //If this is the thread assigned to the consensus node and this node is the last id node
            printf("node_id recover: %d \n", node_id);
            clock_gettime(CLOCK_REALTIME, &start_recover);
            stop_queue();
            switch_recover_mode(node_id);
            simulate_crash = 0;
            consensus_msg_t msg = {};
            msg.epoch = 0;
            msg.id_recover = node_id;
            msg.recover = 1;

            write_socket(sockfd, (char*)&msg, sizeof(consensus_msg_t));

            accept_node_conn(1, total_node-2);
            for(int i = 1; i < total_node-1; ++i){
                idd[i] = i;
                pthread_create(&server[i], NULL, &listen_server_node, &(idd[i]));
            }
        }
    }
}

void open_server_node(){
    //Init server side:
    struct sockaddr_in servaddr;
    server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_sockfd == -1){
        printf("socket creation failed \n");
    }
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(node_list[node_id].port);
    
    if((bind(server_sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr))) != 0){
        printf("socket bind failed \n");
    }
    
    if((listen(server_sockfd, 32)) != 0){
        printf("listen failed \n");
    }
}

void received_recover_msg(int id_recover){
    if(id_recover == node_id){
        return;
    }
    
    printf("recover message received with id = %d \n", id_recover);
                    
    stop_queue();

    clock_gettime(CLOCK_REALTIME, &start_copy);

    copy_data(NULL);

    stop_queue();

    clock_gettime(CLOCK_REALTIME, &now_copy);

    time_diff(start_batch, now_copy, diff_copy);

    double start_nsec_in_ms = ((double)diff_copy->tv_nsec / 1000000);
    double start_sec_in_ms = ((double)diff_copy->tv_sec *1000);
    double total_time_ = start_sec_in_ms + start_nsec_in_ms;

    time_diff(start_copy, now_copy, diff_copy);

    double nsec_in_sec = ((double)diff_copy->tv_nsec / 1000000000);
    double elapsed_ = (double)diff_copy->tv_sec + nsec_in_sec;

    printf("%f: Copy time: %f \n", total_time_, elapsed_);

    int* idd = (int*)malloc(sizeof(int));
    *idd = id_recover;
    printf("will create client thread %d \n", id_recover);
    pthread_create(&client[id_recover], NULL, &open_client_node, idd);
    printf("has created client thread %d \n", id_recover);

    switch_recover_mode(id_recover);
}

void init_node(uint8_t id){
    //1) Open server socket to receive data from every other nodes (including consensus)
    //2) Open client socket with every nodes (including consensus) to send state/hash and recover message
    //3) Sleep an indeterminate amount of time

    node_id = id;

    state_part = ((STATE_SIZE/2) / (total_node-2));

    pthread_t queue;
    client = (pthread_t*)malloc(sizeof(pthread_t) * (total_node-1));
    server = (pthread_t*)malloc(sizeof(pthread_t) * (total_node-1));
    connfd_list_node = (int*)malloc(sizeof(int) * (total_node - 1));
    server_sockfd = 0;
    pthread_mutex_init(&node_lock, NULL);
    pthread_mutex_init(&node_crash, NULL);

    diff_recover = (timespec*)malloc(sizeof(timespec));
    diff_copy = (timespec*)malloc(sizeof(timespec));
    diff_batch = (timespec*)malloc(sizeof(timespec));
    diff_transfert = (timespec*)malloc(sizeof(timespec));

    //Init hash list for each possible incoming node
    uint32_t total_hash = 2*((total_node-2)/3)+1;
    hash_result = (unsigned char***)malloc(sizeof(unsigned char**) * total_node-2);
    for(int i = 0; i < total_node-2; ++i){
        hash_result[i] = (unsigned char**)malloc(sizeof(unsigned char*) * total_hash);
        for(int j = 0; j < total_hash; ++j){
            hash_result[i][j] = (unsigned char*)malloc(sizeof(unsigned char) * SHA256_DIGEST_LENGTH);
        }
    }

    uint8_t* idd = (uint8_t*)malloc(sizeof(uint8_t));
    *idd = 0;

    clock_gettime(CLOCK_REALTIME, &start_batch);

    //Init queue thread
    init_queue();
    pthread_create(&queue, NULL, &queue_thread, NULL);

    //Create client threads
    pthread_create(&client[0], NULL, &open_client_node_consensus, idd);

    printf("All client ok \n");

    open_server_node();
    accept_node_conn(0, 0);

    printf("Open server ok \n");

    //Create server threads
    pthread_create(&server[0], NULL, &listen_server_node_consensus, idd);

    printf("Main thread wait for input \n");

    int command = 0;
    int result = 0;
    while(true){
        command = 0;
        result = 0;
        result = scanf("%d", &command);
        if(result == 1){
            if(command == 1){ //Make the node crash
                simulate_crash = 1;
            }
        }
        else{
            //traitement de l’erreur
        }
    }
}