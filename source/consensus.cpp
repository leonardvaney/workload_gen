#include <consensus.hpp>

void init_consensus(addr_node_t* list, uint8_t total){
    //1) crée des batch
    //2) ouvre un socket client side avec toutes les autres nodes
    //3) commence un thread qui ouvre un socket serveur pour intercepter les demandes de recover
    //4) loop sur l'envoi de batch (ajouter un délai pour éviter de spam ou attendre un retour des autres nodes avant de continuer?)

    srand(1000);

    batch_t* batch = (batch_t*)malloc(sizeof(batch_t));
    batch->addr = (addr_t*)malloc(sizeof(addr_t)*BATCH_SIZE*NUMBER_OF_BATCH);
    for(uint32_t i = 0; i < BATCH_SIZE*NUMBER_OF_BATCH; ++i){
        size_t random_value = rand();
        double r = (double)random_value;
        size_t cell_number = (size_t)((r / RAND_MAX) * STATE_SIZE);
        batch->addr[i] = cell_number;
    }

    node_list = list;
    total_node = total;

    //Init server side:
    struct sockaddr_in servaddr, cli;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1){
        printf("socket creation failed \n");
    }
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(TRANSFER_PORT);
    
    if((bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr))) != 0){
        printf("socket bind failed \n");
    }
    
    if((listen(sockfd, total_node)) != 0){
        printf("listen failed \n");
    }

    int len = sizeof(cli);
    printf("Wait for connections\n");

    for(int i = 0; i < total_node; ++i){
        connfd = accept(sockfd, (struct sockaddr*)&cli, (socklen_t*)&len);
        if(connfd < 0){
            printf("server accept failed \n");
        }
        else{
            connfd_list[i] = connfd;
        }
    }
}

void send_batch(){

}