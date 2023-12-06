#include <node.hpp>

extern addr_node_t* node_list;
extern uint8_t total_node;

void switch_recover_mode(uint8_t id){
    pthread_mutex_lock(&node_lock);
    recover_mode = recover_mode == 1 ? 0 : 1;
    id_recover = id;
    pthread_mutex_unlock(&node_lock);
}


void* open_client_node(void* args){

    uint8_t id = *((uint8_t*)args);

    //printf("init client %d \n", id);

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

    write(sockfd, &node_id, sizeof(uint8_t));

    //Wait for 
    while(true){
        if(id == 0 && node_id == total_node - 1){ //If this is the thread assigned to the consensus node and this node is the last id node
            sleep(5);
            printf("node_id recover: %d \n", node_id);
            clock_gettime(CLOCK_REALTIME, &start_recover);
            switch_recover_mode(node_id);
            consensus_msg_t msg;
            msg.epoch = 0;
            msg.id_recover = node_id;
            msg.recover = 1;
            write(sockfd, &msg, sizeof(consensus_msg_t));
            return NULL;
        }

        pthread_mutex_lock(&node_lock);
        if(recover_mode == 1 && id_recover == id){
            //1) Need to send a hash of 2t+1 hash from 2t+1 state part (starting from part node_id)
            //2) Send t+1 state part (starting from part node_id)

            clock_gettime(CLOCK_REALTIME, &start_transfert);

            size_t written = 0;
            uint8_t t_value = (total_node-2)/3; //-2 because we also remove the consensus node
            unsigned char* hash_result = (unsigned char*)malloc(sizeof(unsigned char) * SHA256_DIGEST_LENGTH);
            hash_msg_t* hash_msg = (hash_msg_t*)malloc(sizeof(hash_msg_t));
            uint32_t* pointeur = get_rw_bit() == 1 ? get_cells() : get_cells() + STATE_SIZE/2;

            //printf("Will send hash to node %d \n", id_recover);

            for(int i = 0; i < 2*t_value+1; ++i){

                //Send hash here:
                uint64_t start = ((node_id-1) * ((STATE_SIZE/2) / (total_node-1)) + i*((STATE_SIZE/2) / (total_node-1))) % (STATE_SIZE/2);
                uint64_t end = start + ((STATE_SIZE/2) / (total_node-1));
                uint64_t offset = get_rw_bit() == 1 ? 0 : STATE_SIZE/2;
                hash_state_elements(start, end, hash_result);

                //printf("Start: %lu , End: %lu \n", start+offset, end+offset);

                //printf("hash: %s \n", hash_result);

                memcpy(hash_msg->hash, hash_result, sizeof(unsigned char) * SHA256_DIGEST_LENGTH);
                write(sockfd, hash_msg, sizeof(hash_msg_t));
            }

            //printf("Will send state part to node %d \n", id_recover);

            //Send t+1 part here:
            uint64_t start = (node_id-1) * ((STATE_SIZE/2) / (total_node-1));
            uint64_t end = (start + ((STATE_SIZE/2) / (total_node-1))*(t_value+1))%(STATE_SIZE/2);

            if(start < end){ //Send a continuous array
                while(written != sizeof(uint32_t)*(end-start)){
                    written += write(sockfd, (char*)(pointeur + start) + written, sizeof(uint32_t)*(end-start) - written);
                }
            }
            else{ //Send two array in two writes
                while(written != sizeof(uint32_t)*(STATE_SIZE/2 - start)){
                    written += write(sockfd, (char*)(pointeur + start) + written, sizeof(uint32_t)*(STATE_SIZE/2 - start) - written);
                }
                written = 0;
                while (written != sizeof(uint32_t)*end){
                    written += write(sockfd, (char*)(pointeur) + written, sizeof(uint32_t)*end - written);
                }
                
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


            //Reset recover mode
            recover_mode = 0;

        }
        pthread_mutex_unlock(&node_lock);
    }
}

void open_server_node(){
    //Init server side:

    connfd_list_node = (int*)malloc(sizeof(int) * (total_node - 1));

    struct sockaddr_in servaddr, cli, result;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1){
        printf("socket creation failed \n");
    }
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(node_list[node_id].port);
    
    if((bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr))) != 0){
        printf("socket bind failed \n");
    }
    
    if((listen(sockfd, 32)) != 0){
        printf("listen failed \n");
    }

    int len = sizeof(cli);
    printf("Wait for connections\n");

    for(int i = 0; i < total_node-1; ++i){
        int connfd = accept(sockfd, (struct sockaddr*)&cli, (socklen_t*)&len);
        //printf("ip: %d, port: %d \n", cli.sin_addr.s_addr, cli.sin_port);
        if(connfd < 0){
            printf("server accept failed \n");
        }
        else{
            connfd_list_node[i] = connfd;
        }
    }
}

void* listen_server_node(void* args){
    size_t block = 0;
    uint32_t epoch = 0;
    uint8_t id = *((uint8_t*)args);
    uint8_t conn_node_id = 0;
    consensus_msg_t* msg = (consensus_msg_t*)malloc(sizeof(consensus_msg_t));
    hash_msg_t* hash_msg = (hash_msg_t*)malloc(sizeof(hash_msg_t));
    unsigned char* temp_hash = (unsigned char*)malloc(SHA256_DIGEST_LENGTH);
    //uint8_t sender_id = 0;
    uint16_t state_part_pointer = 0;
    uint16_t hash_state_pointer = 0;
    uint16_t end_of_recover = 0;
    uint32_t n_batch = 0;
    double total_time, elapsed = 0;
    uint8_t begin_flag = 0; //permet de ne pas relancer le timer si on reçoit un message de recover
    //uint8_t hash_done = 0;

    read(connfd_list_node[id], &conn_node_id, sizeof(uint8_t));

    printf("Receive from %d \n", conn_node_id);

    if(conn_node_id == 0){
        //clock_gettime(CLOCK_REALTIME, &start_batch);
    }

    //Look for consensus_msg_t (sign that someone need to recover)
    while(true){
        if(conn_node_id == 0){

            if(begin_flag == 0){
                clock_gettime(CLOCK_REALTIME, &begin_batch);
            }
            else{
                begin_flag = 0;
            }
            //printf("Will receive consensus message \n");

            //offsetof();
            block = 0;
            while(block != sizeof(consensus_msg_t)){
                block += read(connfd_list_node[id], (char*)(msg) + block, sizeof(consensus_msg_t) - block);
            }
            //printf("id: %d , epoch: %d , block: %d , error: %s \n", conn_node_id, msg->epoch, block, strerror(errno));
            //msg->id_sender = sender_id;
            if(msg->epoch == 0 && msg->recover == 1){
                //if(msg->recover == 1){ //Received a recover message from the consensus node
                    printf("recover message received with id = %d \n", msg->id_recover);
                    
                    if(node_id == total_node-1){
                        continue;
                    }

                    clock_gettime(CLOCK_REALTIME, &start_copy);

                    copy_data(NULL);

                    clock_gettime(CLOCK_REALTIME, &now_copy);

                    time_diff(start_batch, now_copy, diff_copy);

                    double start_nsec_in_ms = ((double)diff_copy->tv_nsec / 1000000);
                    double start_sec_in_ms = ((double)diff_copy->tv_sec *1000);
                    double total_time_ = start_sec_in_ms + start_nsec_in_ms;

                    time_diff(start_copy, now_copy, diff_copy);

                    double nsec_in_sec = ((double)diff_copy->tv_nsec / 1000000000);
                    double elapsed_ = (double)diff_copy->tv_sec + nsec_in_sec;

                    printf("%f: Copy time: %f \n", total_time_, elapsed_);

                    /*for(int i = 0; i < STATE_SIZE; ++i){
                        printf("%d \n", read_state(i));
                    }*/
                    begin_flag = 1;

                    switch_recover_mode(msg->id_recover);
                    //return NULL;
                }
            else{
                    if(node_id == total_node-1){
                        //return NULL;
                        continue; //Read batch but don't process them
                    }

                    //batch_t batch = (batch_t)msg->batch;
                    batch_t batch;
                    batch.addr = (addr_t*)malloc(sizeof(addr_t)*BATCH_SIZE);
                    memcpy(batch.addr, &(msg->batch), sizeof(addr_t)*BATCH_SIZE);
                    execute_batch(&batch, msg->epoch, 0);
                    free(batch.addr);
                    ++epoch;
                    ++n_batch;

                    clock_gettime(CLOCK_REALTIME, &now_batch);

                    time_diff(start_batch, now_batch, diff_batch);

                    double start_nsec_in_ms = ((double)diff_batch->tv_nsec / 1000000);
                    double start_sec_in_ms = ((double)diff_batch->tv_sec *1000);
                    total_time = start_sec_in_ms + start_nsec_in_ms;

                    time_diff(begin_batch, now_batch, diff_batch);

                    double nsec_in_sec = ((double)diff_batch->tv_nsec / 1000000000);
                    elapsed += (double)diff_batch->tv_sec + nsec_in_sec;

                    //printf("elapsed %f \n", elapsed);
                    
                    if(elapsed >= 1){
                        printf("%f: Speed: %f op/s \n", total_time, (n_batch*BATCH_SIZE) / elapsed);
                        n_batch = 0;
                        elapsed = 0;
                    }

            }
            //}
        }
        else{ //Received a hash or a state part from a node
            if(hash_state_pointer < 2*((total_node-2)/3)+1){ //Receive a hash

                block = 0;
                while(block != sizeof(hash_msg_t)){
                    block += read(connfd_list_node[id], (char*)(hash_msg)+block, sizeof(hash_msg_t)-block);
                }
                
                //printf("hash: %s \n", hash_msg->hash);
                memcpy(hash_result[conn_node_id-1][hash_state_pointer], hash_msg->hash, SHA256_DIGEST_LENGTH);
                ++hash_state_pointer;
            }
            else{ //Receive a state part
                if(state_part_pointer < ((total_node-2)/3)+1){

                    block = 0;
                    uint64_t start_for_id = (conn_node_id-1) * ((STATE_SIZE/2) / (total_node-1)); //Fixed starting offset by id
                    uint64_t start = start_for_id + state_part_pointer*((STATE_SIZE/2) / (total_node-1)) % (STATE_SIZE/2);
                    uint64_t end = start + ((STATE_SIZE/2) / (total_node-1));
                    uint64_t offset = get_rw_bit() == 1 ? 0 : STATE_SIZE/2;
                    uint32_t* pointeur_ro = (get_rw_bit() == 1 ? get_cells() : get_cells() + STATE_SIZE/2); //Write temp value in read-only part as it is garbage value
                    uint32_t* pointeur_rw = (get_rw_bit() == 0 ? get_cells() : get_cells() + STATE_SIZE/2); //Pointeur to the place that receive the final correct values

                    
                    while(block != sizeof(uint32_t)*(end-start)){
                        block += read(connfd_list_node[id], (char*)(pointeur_ro + start_for_id) + block, sizeof(uint32_t)*(end-start) - block);
                        //printf("block size: %d \n", block);
                    }
                    
                    hash_state_elements(start_for_id, start_for_id + ((STATE_SIZE/2) / (total_node-1)), temp_hash);
                    
                    //BESOIN DE COMPARER DE MANIÈRE PLUS EXHAUSTIVE
                    if(strncmp((const char*)temp_hash, (const char*)hash_result[conn_node_id-1][state_part_pointer], SHA256_DIGEST_LENGTH) == 0){
                        printf("CORRECT HASH ID %d \n", conn_node_id);
                        //printf("start = %d end = %d \n", start, end);
                        //printf("start hash: %d , end hash: %d \n", start + offset, offset + start + ((STATE_SIZE/2) / (total_node-1)));
                        //printf("part hash: %s \n", temp_hash);

                        memcpy(pointeur_rw + start_for_id + state_part_pointer*((STATE_SIZE/2) / (total_node-1)), pointeur_ro + start_for_id, sizeof(uint32_t)*(end-start)); //Src is always fixed, Dst depend on state_part_pointer
                    }
                    else{
                        printf("Hash are not equal ID %d \n", conn_node_id);

                        //printf("start = %d end = %d \n", start, end);
                        //printf("start hash: %d , end hash: %d \n", start + offset, offset + start + ((STATE_SIZE/2) / (total_node-1)));
                        //printf("part hash: %s \n", temp_hash);
                    }

                    ++state_part_pointer;
                }
                else{

                    hash_state_pointer = 0;
                    state_part_pointer = 0;

                    pthread_mutex_lock(&node_lock);

                    if(recover_mode == total_node-2){
                        /*for(int i = 0; i < STATE_SIZE; ++i){
                            printf("%d \n", read_state(i));
                        }*/

                        printf("End of recover \n");
                        //recover_mode = 0;

                        recover_mode = 0;
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
                    }
                    else{
                        ++recover_mode;
                    }

                    pthread_mutex_unlock(&node_lock);
                }
            }
        }
    }
}





void init_node(uint8_t id){
    //1) Open server socket to receive data from every other nodes (including consensus)
    //2) Open client socket with every nodes (including consensus) to send state/hash and recover message
    //3) Sleep an indeterminate amount of time

    node_id = id;

    /*if(node_id == total_node-1){
        for(int i = 0; i < STATE_SIZE; ++i){
            write_state(i, 0);
        }
    }*/

    pthread_t* client;
    pthread_t* server;
    client = (pthread_t*)malloc(sizeof(pthread_t) * (total_node-1));
    server = (pthread_t*)malloc(sizeof(pthread_t) * (total_node-1));
    pthread_mutex_init(&node_lock, NULL);

    diff_recover = (timespec*)malloc(sizeof(timespec));
    diff_batch = (timespec*)malloc(sizeof(timespec));
    diff_copy = (timespec*)malloc(sizeof(timespec));
    diff_transfert = (timespec*)malloc(sizeof(timespec));

    clock_gettime(CLOCK_REALTIME, &start_batch);

    //Init hash list for each possible incoming node
    uint32_t total_hash = 2*((total_node-2)/3)+1;
    hash_result = (unsigned char***)malloc(sizeof(unsigned char**) * total_node-2);
    for(int i = 0; i < total_node-2; ++i){
        hash_result[i] = (unsigned char**)malloc(sizeof(unsigned char*) * total_hash);
        for(int j = 0; j < total_hash; ++j){
            hash_result[i][j] = (unsigned char*)malloc(sizeof(unsigned char) * SHA256_DIGEST_LENGTH);
        }
    }

    uint8_t* idd;

    //printf("ok \n");
    //printf("total node: %d \n", total_node);
    for(int i = 0; i < total_node; ++i){
        printf("%d \n", htons(node_list[i].port));
    }

    //Create client threads
    for(int i = 0; i < total_node-1; ++i){
        idd = (uint8_t*)malloc(sizeof(uint8_t));
        *idd = i >= node_id ? i+1 : i;
        printf("will create client thread %d \n", *idd);
        pthread_create(&client[i], NULL, open_client_node, idd);
    }

    printf("All client ok \n");

    open_server_node();

    printf("Open server ok \n");

    //Create server threads
    for(int i = 0; i < total_node-1; ++i){
        idd = (uint8_t*)malloc(sizeof(uint8_t));
        *idd = i;
        pthread_create(&server[i], NULL, listen_server_node, idd);
    }

    printf("Main thread sleep \n");

    sleep(100000000);
}