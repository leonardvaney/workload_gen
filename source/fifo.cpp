#include <fifo.hpp>

void init_fifo(uint8_t total_node){
    pthread_mutex_init(&fifo_lock, NULL);
    fifo_size = total_node - 1;
    fifo = (fifo_t*)malloc(sizeof(fifo_t)*fifo_size);

    //Init every fifo
    for(int i = 0; i < fifo_size; ++i){
        pthread_mutex_init(&fifo[i].lock, NULL);
        fifo[i].msg_list = (consensus_msg_t*)malloc(sizeof(consensus_msg_t) * MAX_MSG_LIST);
        fifo[i].size_list = 0;
    }
}

void add_to_fifo(consensus_msg_t* msg){
    pthread_mutex_lock(&fifo_lock);
    for(int i = 0; i < fifo_size; ++i){
        pthread_mutex_lock(&fifo[i].lock);

        if(fifo[i].size_list == MAX_MSG_LIST){
            
            //Certainement possible de faire mieux
            pthread_mutex_unlock(&fifo[i].lock);
            while(true){
                pthread_mutex_lock(&fifo[i].lock);
                
                if(fifo[i].size_list < MAX_MSG_LIST){
                    fifo[i].msg_list[fifo[i].size_list] = *msg;
                    fifo[i].size_list += 1;        
                    break;
                }

                pthread_mutex_unlock(&fifo[i].lock);
            }


        }
        else{
            fifo[i].msg_list[fifo[i].size_list] = *msg;
            fifo[i].size_list += 1;
        }

        pthread_mutex_unlock(&fifo[i].lock);
    }
    pthread_mutex_unlock(&fifo_lock);
}

int get_fifo_msg(uint8_t id, consensus_msg_t* result){
    pthread_mutex_lock(&fifo[id].lock);

    int success = 0;

    if(fifo[id].size_list == 0){
        success = 0;
    }
    else{
        memcpy(result, &(fifo[id].msg_list[0]), sizeof(consensus_msg_t));
        for(int i = 0; i < fifo[id].size_list-1; ++i){
            fifo[id].msg_list[i] = fifo[id].msg_list[i+1];
        }
        fifo[id].size_list -= 1;

        success = 1;
    }

    pthread_mutex_unlock(&fifo[id].lock);

    return success;
}