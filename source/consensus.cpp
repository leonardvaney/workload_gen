#include <consensus.hpp>

void send_batch(consensus_msg_t msg){
    consensus_msg_t* copy;
    memcpy(copy, &msg, sizeof(consensus_msg_t));
    add_to_fifo(copy); //Pas sûr que ça soit correct
}

void* open_client(void* args){
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
        printf("connection with the node %d failed, retry \n", id+1);
    }


    //Get consensus_msg_t and send them
    while(true){
        consensus_msg_t* msg; (consensus_msg_t*)malloc(sizeof(consensus_msg_t));
        get_fifo_msg(id, msg);
        if(msg != NULL){ //get_fifo_msg se charge de free si il renvoie un msg == NULL
            write(sockfd, msg, sizeof(consensus_msg_t));
            free(msg);
        }
    }
}

void open_server(){
    //Init server side:
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
    printf("Wait for connections\n");

    for(int i = 0; i < total_node-1; ++i){
        int connfd = accept(sockfd, (struct sockaddr*)&cli, (socklen_t*)&len);
        if(connfd < 0){
            printf("server accept failed \n");
        }
        else{
            connfd_list[i] = connfd;
        }
    }
}

void* listen_server(void* args){
    
    size_t block = 0; 
    uint8_t id = *((uint8_t*)args);
    consensus_msg_t* msg;

    //Look for consensus_mgs_t (sign that someone need to recover)
    while(true){
        block = read(connfd_list[id], (void*)msg, sizeof(consensus_msg_t));

        //MESSAGE A MODIFIER AVANT D'AJOUTER
        add_to_fifo(msg);
    }
}

void init_consensus(addr_node_t* list, uint8_t total){
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
            size_t cell_number = (size_t)((r / RAND_MAX) * STATE_SIZE);
            printf("i: %d, j: %d \n", i, j);
            batch[i].addr[j] = cell_number;
        }
    }

    printf("batch ok \n");

    node_list = list;
    total_node = total;

    pthread_t* client;
    pthread_t* server;
    client = (pthread_t*)malloc(sizeof(pthread_t) * (total_node-1));
    server = (pthread_t*)malloc(sizeof(pthread_t) * (total_node-1));

    //Create client threads
    for(int i = 0; i < total_node-1; ++i){
        uint8_t id = i;
        pthread_create(&client[i], NULL, open_client, &id);
    }

    open_server();

    printf("op server ok");

    //Create server threads
    for(int i = 0; i < total_node-1; ++i){
        uint8_t id = i;
        pthread_create(&server[i], NULL, listen_server, &id);
    }

    //Loop on batch to send
    while (true)
    {
        consensus_msg_t msg = {batch[epoch%NUMBER_OF_BATCH], epoch, 0, 0};
        send_batch(msg);
        epoch += 1;
    }
    
}