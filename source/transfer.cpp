#include <transfer.hpp>

void init_server(){
    struct sockaddr_in servaddr, cli;
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1){
        printf("socket creation failed \n");
    }

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(8887);
    
    if((bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr))) != 0){
        printf("socket bind failed \n");
    }
    
    if((listen(sockfd, 1)) != 0){
        printf("listen failed \n");
    }

    int len = sizeof(cli);

    printf("Wait for a connection\n");
    connfd = accept(sockfd, (struct sockaddr*)&cli, (socklen_t*)&len);
    if(connfd < 0){
        printf("server accept failed \n");
    }
}

void init_client(){
    struct sockaddr_in servaddr, cli;
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1){
        printf("socket creation failed \n");
    }

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(8887);
    
    if(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) != 0){
        printf("connection with the server failed \n");
    }
}

void send_state(){
    size_t total_written = 0;
    size_t written = 0;
    char* new_buffer = (char*)get_cells();

    while(total_written != STATE_SIZE*4){
        new_buffer = new_buffer + written;
        written = write(sockfd, new_buffer, STATE_SIZE*4 - total_written);
        total_written += written;
        printf("write: %ld \n", written);
    }
}

void receive_state(){
    size_t total_read = 0;
    size_t block = 0; 
    char* new_buffer = (char*)get_cells();

    while(total_read != STATE_SIZE*4){
        new_buffer = new_buffer + block;
        block = read(connfd, (void*)new_buffer, STATE_SIZE*4 - total_read);
        total_read += block;
        printf("read: %ld vs buffer size: %ld \n", total_read, STATE_SIZE);

        if(block < 0){
            //...check errno
        }
    }
}