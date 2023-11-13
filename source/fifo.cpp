#include <fifo.hpp>

void init_fifo(uint8_t total_node){
    //pthread_mutex_init(&fifo_lock, NULL);
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
            //printf("Add something to fifo \n");
        }

        pthread_mutex_unlock(&fifo[i].lock);
    }
}

int get_fifo_msg(uint8_t id, consensus_msg_t* result){
    pthread_mutex_lock(&fifo[id].lock);

    //printf("Will return fifo message \n");
    int success = 0;

    if(fifo[id].size_list == 0){
        //printf("Empty fifo \n");
        //free(result); //Probablement dangereux...
        //result = NULL; //No message in the list
        success = 0;
    }
    else{
        //printf("Not empty fifo \n");
        //printf("%d \n", result->epoch);
        memcpy(result, &(fifo[id].msg_list[0]), sizeof(consensus_msg_t));
        //memset

        //printf("memcpy ok \n");
        //Move every element one index lower
        for(int i = 0; i < fifo[id].size_list-1; ++i){
            fifo[id].msg_list[i] = fifo[id].msg_list[i+1];
        }
        fifo[id].size_list -= 1;

        success = 1;
    }

    //printf("Return fifo message \n");

    pthread_mutex_unlock(&fifo[id].lock);

    return success;
}