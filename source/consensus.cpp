#include <consensus.hpp>

void send_batch(consensus_msg_t msg){
    consensus_msg_t* copy = (consensus_msg_t*)malloc(sizeof(consensus_msg_t));
    memcpy(copy, &msg, sizeof(consensus_msg_t));
    add_to_fifo(copy); //Pas sûr que ça soit correct
    free(copy);
}

void* open_client_consensus(void* args){
    uint8_t id = *((uint8_t*)args);

    struct sockaddr_in servaddr, cli;
    
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1){
        printf("socket creation failed \n");
    }

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(node_list[id+1].ip);
    servaddr.sin_port = htons(node_list[id+1].port);
    
    while(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) != 0){
        sleep(1);
        printf("connection with the node %d failed, retry \n", id+1);
    }

    uint8_t x = 0;
    write(sockfd, &x, sizeof(uint8_t));

    int success = 0;
    int written = 0;
    consensus_msg_t* msg = (consensus_msg_t*)malloc(sizeof(consensus_msg_t));

    //Get consensus_msg_t and send them
    while(true){
        
        //sleep(2);

        //consensus_msg_t message = {{1,1,1}, 1, 0, 0, 0};

        //write(sockfd, &message, sizeof(consensus_msg_t));
        
        //msg->epoch = 10;
        success = get_fifo_msg(id, msg);
        //printf("msg: %p \n", msg);
        if(success == 1){ //get_fifo_msg se charge de free si il renvoie un msg == NULL
            //printf("Send msg to %d \n", id+1);
            
            while(written != sizeof(consensus_msg_t)){
                written += write(sockfd, ((char*)msg) + written, sizeof(consensus_msg_t) - written);
            }

            written = 0;

            //free(msg);
        }
    }
}

void open_server_consensus(){
    //Init server side:

    connfd_list_consensus = (int*)malloc(sizeof(int) * (total_node-1));

    struct sockaddr_in servaddr, cli;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1){
        printf("socket creation failed \n");
    }
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(node_list[0].port);
    
    if((bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr))) != 0){
        printf("socket bind failed \n");
    }
    
    if((listen(sockfd, 32)) != 0){
        printf("listen failed \n");
    }

    int len = sizeof(cli);
    printf("Wait for connections \n");

    for(int i = 0; i < total_node-1; ++i){
        int connfd = accept(sockfd, (struct sockaddr*)&cli, (socklen_t*)&len);
        if(connfd < 0){
            printf("server accept failed \n");
        }
        else{
            connfd_list_consensus[i] = connfd;
        }
    }
}

void* listen_server_consensus(void* args){
    size_t block = 0; 
    uint8_t id = *((uint8_t*)args);
    uint8_t conn_node_id = 0;
    consensus_msg_t* msg = (consensus_msg_t*)malloc(sizeof(consensus_msg_t));

    read(connfd_list_consensus[id], &conn_node_id, sizeof(uint8_t));

    //Look for consensus_mgs_t (sign that someone need to recover)
    while(true){
        
        while(block != sizeof(consensus_msg_t)){
            block += read(connfd_list_consensus[id], ((char*)msg) + block, sizeof(consensus_msg_t) - block);
        }

        if(block > 0){
            printf("Received recover request \n");
            msg->epoch = 0;
            msg->recover = 1;
            add_to_fifo(msg);
        }

        block = 0;

        //MESSAGE A MODIFIER AVANT D'AJOUTER
        //add_to_fifo(msg);
    }
}

void init_consensus(){
    //1) crée des batch
    //2) ouvre un socket client side avec toutes les autres nodes
    //3) commence un thread qui ouvre un socket serveur pour intercepter les demandes de recover
    //4) loop sur l'envoi de batch (ajouter un délai pour éviter de spam ou attendre un retour des autres nodes avant de continuer?)

    srand(1000);

    uint32_t epoch = 0;

    batch_t* batch = (batch_t*)malloc(sizeof(batch_t) * NUMBER_OF_BATCH);
    for(int i = 0; i < NUMBER_OF_BATCH; ++i){
        batch[i].addr = (addr_t*)malloc(sizeof(addr_t)*BATCH_SIZE);
    }

    for(uint32_t i = 0; i < NUMBER_OF_BATCH; ++i){
        for(uint32_t j = 0; j < BATCH_SIZE; ++j){
            size_t random_value = rand();
            double r = (double)random_value;
            size_t cell_number = (size_t)((r / RAND_MAX) * STATE_SIZE/2);
            //printf("i: %d, j: %d \n", i, j);
            batch[i].addr[j] = cell_number;
        }
    }

    printf("batch ok \n");

    pthread_t* client;
    pthread_t* server;
    client = (pthread_t*)malloc(sizeof(pthread_t) * (total_node-1));
    server = (pthread_t*)malloc(sizeof(pthread_t) * (total_node-1));

    uint8_t* idd;

    //Create client threads
    for(int i = 0; i < total_node-1; ++i){
        idd = (uint8_t*)malloc(sizeof(uint8_t));
        *idd = i;
        pthread_create(&client[i], NULL, &open_client_consensus, idd);
    }

    open_server_consensus();

    printf("op server ok \n");

    //Create server threads
    for(int i = 0; i < total_node-1; ++i){
        idd = (uint8_t*)malloc(sizeof(uint8_t));
        *idd = i;
        pthread_create(&server[i], NULL, &listen_server_consensus, idd);
    }

    //Loop on batch to send
    while (true)
    {
        //if(epoch < 100){
            consensus_msg_t msg;
            memcpy(msg.batch, batch[epoch%NUMBER_OF_BATCH].addr, sizeof(addr_t)*BATCH_SIZE);
            msg.epoch = epoch;
            msg.id_recover = 0;
            //msg.id_sender = 0;
            msg.recover = 0;
            if(BATCH_WAIT != 0){
                sleep(BATCH_WAIT);
            }
            send_batch(msg);
            epoch += 1;
        //}
    }
    
}