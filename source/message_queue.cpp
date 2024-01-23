#include <message_queue.hpp>

void init_queue(){
    clock_gettime(CLOCK_REALTIME, &start);

    pthread_mutex_init(&queue_lock, NULL);
    diff = (timespec*)malloc(sizeof(timespec));
    message_queue = (consensus_msg_t*)malloc(sizeof(consensus_msg_t)*CATCHUP_LIMIT);
    front = -1;
    rear = -1;
    stop = 0;
}

void* queue_thread(void* args){
    //Get crash variable + node lock from args

    size_t epoch = 0;
    size_t n_batch = 0;
    batch_t batch;
    batch.addr = (addr_t*)malloc(sizeof(addr_t)*BATCH_SIZE);
    double total_time = 0;
    double elapsed = 0;

    clock_gettime(CLOCK_REALTIME, &begin);

    while(true){

        pthread_mutex_lock(&queue_lock);

        if(is_empty() == 0 && stop == 0){

            consensus_msg_t message = dequeue_message();
            memcpy(batch.addr, &(message.batch), sizeof(addr_t)*BATCH_SIZE);

            execute_batch(&batch, message.epoch, 0);
            ++epoch;
            ++n_batch;

            clock_gettime(CLOCK_REALTIME, &now);

            time_diff(start, now, diff);

            double start_nsec_in_ms = ((double)diff->tv_nsec / 1000000);
            double start_sec_in_ms = ((double)diff->tv_sec *1000);
            total_time = start_sec_in_ms + start_nsec_in_ms;

            time_diff(begin, now, diff);

            double nsec_in_sec = ((double)diff->tv_nsec / 1000000000);
            elapsed += (double)diff->tv_sec + nsec_in_sec;
                                
            if(elapsed >= 1){
                printf("%f: Speed: %f op/s \n", total_time, (n_batch*BATCH_SIZE) / elapsed);
                printf("Front: %d Rear: %d \n", front, rear);
                n_batch = 0;
                elapsed = 0;
            }

            clock_gettime(CLOCK_REALTIME, &begin);
        }

        pthread_mutex_unlock(&queue_lock);
    
    }

}

void stop_queue(){
    pthread_mutex_lock(&queue_lock);
    stop = stop == 0 ? 1 : 0;
    pthread_mutex_unlock(&queue_lock);
}

int is_full() {
    if ((front == rear + 1) || (front == 0 && rear == CATCHUP_LIMIT - 1)) {
        return 1;
    }
    return 0;
}

int is_empty() {
    if (front == -1) {
        return 1;
    }
    return 0;
}

void enqueue_message(consensus_msg_t message) {
    while(is_full()) {
        //Wait
    }
    pthread_mutex_lock(&queue_lock);
    if (front == -1) {
        front = 0;
    }
    rear = (rear + 1) % CATCHUP_LIMIT;
    message_queue[rear] = message;
    pthread_mutex_unlock(&queue_lock);
}

consensus_msg_t dequeue_message() {
    consensus_msg_t message;
    if (is_empty()) {
        return {};
    } 
    else {
        message = message_queue[front];
        if (front == rear) {
        front = -1;
        rear = -1;
        } 
        else {
            front = (front + 1) % CATCHUP_LIMIT;
        }
        return message;
    }
}