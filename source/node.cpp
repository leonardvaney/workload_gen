#include <node.hpp>

extern addr_node_t* node_list;
extern uint8_t total_node; 


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

    //Look for consensus_mgs_t (sign that someone need to recover)
    while(true){
        //block = read(connfd_list_node[id], (void*)msg, sizeof(consensus_msg_t));

        block = read(connfd_list_node[id], (void*)msg, sizeof(consensus_msg_t));

        /*for(int i = 0; i < 5; ++i){
            printf("%d \n", msg->batch[i]);
        }*/
        printf("local epoch: %d, msg epoch: %d \n", epoch, msg->epoch);
        if(msg->epoch == epoch){
            printf("id: %d , block: %d , error: %s \n", id, block, strerror(errno));
            ++epoch;
        }

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