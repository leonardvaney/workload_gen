#include <transfer.hpp>

void init_server(){
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
    servaddr.sin_port = htons(TRANSFER_PORT);
    
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

void send_state_progressive_lock(pthread_mutex_t* state_locks){
    size_t total_written = 0;
    size_t written = 0;
    uint32_t subpart_unlocked = 0;
    char* new_buffer = (char*)get_cells();

    while(total_written != STATE_SIZE*4){
        new_buffer = new_buffer + written;
        written = write(sockfd, new_buffer, STATE_SIZE*4 - total_written);
        total_written += written;

        printf("write: %ld \n", written);

        //Handle progressive unlocking
        uint32_t part_to_unlock = (total_written / 4) / (STATE_SIZE / STATE_SUBPART);
        if(part_to_unlock > subpart_unlocked){
            for(int i = subpart_unlocked; i < part_to_unlock; ++i){
                pthread_mutex_unlock(&(state_locks[i]));
            }

            subpart_unlocked = part_to_unlock;
        }

    }
}

void send_state_rw_lock(){
    size_t total_written = 0;
    size_t written = 0;
    char* new_buffer = (char*)get_cells();

    if(get_rw_bit() == 0){
        new_buffer += STATE_SIZE*2; //On transfert la seconde moitié uniquement
    }

    while(total_written != STATE_SIZE*2){
        new_buffer = new_buffer + written;
        written = write(sockfd, new_buffer, STATE_SIZE*2 - total_written);
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
        printf("read: %lu vs buffer size: %d \n", total_read, STATE_SIZE*4);

        if(block < 0){
            //...check errno
        }
    }
}

void receive_state_rw_lock(){
    size_t total_read = 0;
    size_t block = 0; 
    char* new_buffer = (char*)get_cells();

    //A adapter si on veut gérer des scénarios plus complexes

    while(total_read != STATE_SIZE*2){
        new_buffer = new_buffer + block;
        block = read(connfd, (void*)new_buffer, STATE_SIZE*2 - total_read);
        total_read += block;
        printf("read: %lu vs buffer size: %d \n", total_read, STATE_SIZE*2);
    }
}