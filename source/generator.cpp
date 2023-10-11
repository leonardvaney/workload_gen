#include <generator.hpp>

void generate_batch(batch_t* batch){

    batch->addr = (addr_t*)malloc(sizeof(addr_t)*BATCH_SIZE*NUMBER_OF_BATCH);
    batch->subpart = (modified_subpart_t*)malloc(sizeof(modified_subpart_t)*NUMBER_OF_BATCH);
    for(uint32_t i = 0; i < NUMBER_OF_BATCH; ++i){
        batch->subpart[i].part_to_lock = NULL;
        batch->subpart[i].size = 0;
    }

    for(uint32_t i = 0; i < BATCH_SIZE*NUMBER_OF_BATCH; ++i){
        size_t random_value = rand();
        double r = (double)random_value;
        size_t cell_number = (size_t)((r / RAND_MAX) * STATE_SIZE);
        batch->addr[i] = cell_number;
        
        //For progressive lock
        uint32_t subpart = cell_number / STATE_SUBPART;
        uint8_t already_in = 0;
        uint32_t batch_number = i / BATCH_SIZE;

        //printf("batch number: %d , batch subpart size: %d \n", batch_number, batch->subpart[batch_number].size);

        for(uint32_t j = 0; j < batch->subpart[batch_number].size; ++j){
            //printf("subpart: %d \n", batch->subpart[batch_number].part_to_lock[j]);
            already_in = (batch->subpart[batch_number].part_to_lock[j] == subpart) ? 1 : already_in | 0;
        }

        if(already_in == 0){
            batch->subpart[batch_number].size += 1;
            batch->subpart[batch_number].part_to_lock = (uint32_t*)realloc(batch->subpart[batch_number].part_to_lock, sizeof(uint32_t) * batch->subpart[batch_number].size);
            batch->subpart[batch_number].part_to_lock[batch->subpart[batch_number].size - 1] = subpart;
        }
    }
}

void time_diff(timespec start, timespec end, timespec* result){
    //Diff between start and end
    long time_spent_sec = (end.tv_sec - start.tv_sec);
    long time_spent_nsec = (end.tv_nsec - start.tv_nsec);
    if(time_spent_nsec < 0){
        time_spent_sec -= 1;
        time_spent_nsec += 1000000000;
    }
    result->tv_nsec = time_spent_nsec;
    result->tv_sec = time_spent_sec;
}

void* full_lock_generator(void* args){
    pthread_mutex_t* lock = (pthread_mutex_t*)args;
    batch_t* batch = (batch_t*)malloc(sizeof(batch_t));
    generate_batch(batch);
    uint32_t epoch = 0;
    uint32_t n_batch = 0;
    struct timespec start, begin, now;
    timespec* diff = (timespec*)malloc(sizeof(timespec));
    double total_time, elapsed = 0;

    clock_gettime(CLOCK_REALTIME, &start);

    for(;;){

        clock_gettime(CLOCK_REALTIME, &begin);

        pthread_mutex_lock(lock);
        ++epoch;
        execute_batch(batch, epoch, epoch%NUMBER_OF_BATCH);
        pthread_mutex_unlock(lock);

        n_batch += 1;

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
            n_batch = 0;
            elapsed = 0;
        }
    }
}

void* progressive_lock_generator(void* args){
    pthread_mutex_t* state_locks = *((pthread_mutex_t**)args);
    batch_t* batch = (batch_t*)malloc(sizeof(batch_t));

    generate_batch(batch);
    uint32_t epoch = 0;
    uint32_t n_batch = 0;
    struct timespec start, begin, now;
    timespec* diff = (timespec*)malloc(sizeof(timespec));
    double total_time, elapsed = 0;

    clock_gettime(CLOCK_REALTIME, &start);

    for(;;){

        clock_gettime(CLOCK_REALTIME, &begin);

        uint32_t batch_index = epoch%NUMBER_OF_BATCH;

        for(uint32_t i = 0; i < batch->subpart[batch_index].size; ++i){
            pthread_mutex_lock(&(state_locks[batch->subpart[batch_index].part_to_lock[i]]));
        }

        ++epoch;
        execute_batch(batch, epoch, batch_index);
        
        for(uint32_t i = 0; i < batch->subpart[batch_index].size; ++i){
            pthread_mutex_unlock(&(state_locks[batch->subpart[batch_index].part_to_lock[i]]));
        }
        
        n_batch += 1;

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
            n_batch = 0;
            elapsed = 0;
        }
    }
}