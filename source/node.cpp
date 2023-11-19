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

    //printf("oui1 \n");

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(node_list[id].ip);
    servaddr.sin_port = htons(node_list[id].port);

    //printf("oui2 \n");
    
    while(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) != 0){
        sleep(1);
        printf("connection with the node %d failed, retry \n", id);
    }


    printf("init connection with server %d ok \n", id);
    //Wait for 
    while(true){
        if(id == 0 && node_id == total_node - 1){ //If this is the thread assigned to the consensus node and this node is the last id node
            sleep(10);
            printf("node_id recover: %d \n", node_id);
            switch_recover_mode(node_id);
            consensus_msg_t msg;
            msg.epoch = 0;
            msg.id_recover = node_id;
            msg.id_sender = node_id;
            msg.recover = 1;
            write(sockfd, &msg, sizeof(consensus_msg_t));
            break;
        }

        pthread_mutex_lock(&node_lock);
        if(recover_mode == 1 && id_recover == id){
            //1) Need to send a hash of 2t+1 hash from 2t+1 state part (starting from part node_id)
            //2) Send t+1 state part (starting from part node_id)

            uint8_t t_value = (total_node-2)/3; //-2 because we also remove the consensus node
            unsigned char* hash_result = (unsigned char*)malloc(sizeof(unsigned char) * SHA256_DIGEST_LENGTH);
            hash_msg_t* hash_msg = (hash_msg_t*)malloc(sizeof(hash_msg_t));

            printf("Will send hash to node %d \n", id_recover);

            for(int i = 0; i < 2*t_value+1; ++i){

                printf("test \n");
                //Send hash here:
                uint64_t start = ((node_id-1) * ((STATE_SIZE/2) / (total_node-1)) + i*((STATE_SIZE/2) / (total_node-1))) % (STATE_SIZE/2);
                uint64_t end = start + ((STATE_SIZE/2) / (total_node-1));
                hash_state_elements(start, end, hash_result);

                printf("Send hash part %d \n", i);

                memcpy(hash_msg->hash, hash_result, sizeof(unsigned char) * SHA256_DIGEST_LENGTH);
                hash_msg->id_sender = node_id;
                hash_msg->state_part = 0;
                write(sockfd, hash_msg, sizeof(hash_msg_t));
            }

            for(int i = 0; i < t_value+1; ++i){
                //Send state part here:
            }

            //Reset recover mode
            recover_mode = 0;

        }
        pthread_mutex_unlock(&node_lock);
        /*consensus_msg_t* msg; (consensus_msg_t*)malloc(sizeof(consensus_msg_t));
        get_fifo_msg(id, msg);
        if(msg != NULL){ //get_fifo_msg se charge de free si il renvoie un msg == NULL
            write(sockfd, msg, sizeof(consensus_msg_t));
            free(msg);
        }*/
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
            //for(int j = 0; i < total_node; ++i){
                //inet_pton(AF_INET, node_list[j].ip, &(result.sin_addr));
                //printf("%d ?= %d && %d ?= %d \n", result.sin_addr.s_addr, cli.sin_addr.s_addr, htons(node_list[j].port), cli.sin_port);
                //if(inet_addr(node_list[j].ip) == cli.sin_addr.s_addr && htons(node_list[j].port) == cli.sin_port){
                //    printf("Will listen from client %d \n", node_list[j].id);
                //    connfd_list_node[node_list[j].id] = connfd;
                    connfd_list_node[i] = connfd;
                //    break;
                //}
            //}
            
        }
    }
}

void* listen_server_node(void* args){
    size_t block = 0;
    uint32_t epoch = 0;
    uint8_t id = *((uint8_t*)args);
    consensus_msg_t* msg = (consensus_msg_t*)malloc(sizeof(consensus_msg_t));
    hash_msg_t* hash_msg = (hash_msg_t*)malloc(sizeof(hash_msg_t));
    uint8_t sender_id = 0;
    uint16_t state_part_pointer = 0;

    //Look for consensus_msg_t (sign that someone need to recover)
    while(true){
        block = read(connfd_list_node[id], (void*)&sender_id, sizeof(uint8_t));
        //printf("sender id: %d \n", sender_id);
        if(sender_id == 0){
            //offsetof();
            block = read(connfd_list_node[id], (void*)(msg)+1, sizeof(consensus_msg_t) - sizeof(uint8_t));
            //msg->id_sender = sender_id;
            if(msg->epoch == epoch || (msg->epoch == 0 && msg->recover == 1)){
                if(msg->recover == 1){ //Received a recover message from the consensus node
                    printf("recover message received with id = %d \n", msg->id_recover);
                    switch_recover_mode(msg->id_recover);
                    break;
                }
                else{
                    printf("id: %d , block: %d , error: %s \n", id, block, strerror(errno));
                    ++epoch;
                }
            }
        }
        else{ //Received a hash or a state part from a node
            if(state_part_pointer < 2*((total_node-2)/3)+1){

                block = read(connfd_list_node[id], (void*)(hash_msg)+1, sizeof(hash_msg_t) - sizeof(uint8_t));

                printf("hash: %s \n", hash_msg->hash);
                memcpy(hash_result[sender_id-1][state_part_pointer], hash_msg->hash, SHA256_DIGEST_LENGTH);
                ++state_part_pointer;
            }
            else{
                


                state_part_pointer = 0;
            }
        }

        /*for(int i = 0; i < 5; ++i){
            printf("%d \n", msg->batch[i]);
        }*/
        //printf("local epoch: %d, msg epoch: %d \n", epoch, msg->epoch);

        //sleep(1);
        //MESSAGE A MODIFIER AVANT D'AJOUTER
        //add_to_fifo(msg);
    }
}





void init_node(uint8_t id){
    //1) Open server socket to receive data from every other nodes (including consensus)
    //2) Open client socket with every nodes (including consensus) to send state/hash and recover message
    //3) Sleep an indeterminate amount of time

    node_id = id;

    pthread_t* client;
    pthread_t* server;
    client = (pthread_t*)malloc(sizeof(pthread_t) * (total_node-1));
    server = (pthread_t*)malloc(sizeof(pthread_t) * (total_node-1));
    pthread_mutex_init(&node_lock, NULL);

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