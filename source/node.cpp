#include <node.hpp>

void* open_client_node(void* args){

    printf("init client \n");

    uint8_t id = *((uint8_t*)args);

    struct sockaddr_in servaddr, cli;
    
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1){
        printf("socket creation failed \n");
    }

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(node_list[id].ip);
    servaddr.sin_port = htons(node_list[id].port);
    
    while(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) != 0){
        printf("connection with the node %d failed, retry \n", id+1);
    }


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

    connfd_list_node = (int*)malloc(sizeof(int) * (total_node-1));

    struct sockaddr_in servaddr, cli;
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
    uint8_t id = *((uint8_t*)args);
    consensus_msg_t* msg;

    //Look for consensus_mgs_t (sign that someone need to recover)
    while(true){
        block = read(connfd_list_node[id], (void*)msg, sizeof(consensus_msg_t));

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

    printf("ok \n");
    printf("total node: %d \n", total_node);

    //Create client threads
    for(int i = 0; i < total_node-1; ++i){
        printf("will create client thread \n");
        uint8_t idd = i;
        pthread_create(&client[i], NULL, open_client_node, &idd);
    }

    open_server_node();

    printf("op server ok");

    //Create server threads
    for(int i = 0; i < total_node-1; ++i){
        uint8_t idd = i;
        pthread_create(&server[i], NULL, listen_server_node, &idd);
    }


    sleep(100000000);
}