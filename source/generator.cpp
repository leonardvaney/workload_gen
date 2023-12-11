#include <generator.hpp>

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

/*void generate_batch(batch_t* batch){
    batch->addr = (addr_t*)malloc(sizeof(addr_t)*BATCH_SIZE*NUMBER_OF_BATCH);
    for(uint32_t i = 0; i < BATCH_SIZE*NUMBER_OF_BATCH; ++i){
        size_t random_value = rand();
        double r = (double)random_value;
        size_t cell_number = (size_t)((r / RAND_MAX) * STATE_SIZE);
        batch->addr[i] = cell_number;
    }
}

void rw_lock_generate_batch(batch_t* batch){
    batch->addr = (addr_t*)malloc(sizeof(addr_t)*BATCH_SIZE*NUMBER_OF_BATCH);
    for(uint32_t i = 0; i < BATCH_SIZE*NUMBER_OF_BATCH; ++i){
        size_t random_value = rand();
        double r = (double)random_value;
        size_t cell_number = (size_t)((r / RAND_MAX) * STATE_SIZE / 2);
        batch->addr[i] = cell_number;
    }
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

        //Analyze packet here:
        uint32_t subpart_to_modify[STATE_SUBPART];
        uint32_t total_subpart = 0;
        for(uint32_t i = 0; i < BATCH_SIZE; ++i){
            uint32_t subpart = (batch->addr[i + BATCH_SIZE*batch_index]) / (STATE_SIZE / STATE_SUBPART);
            //printf("to modify: %u \n", subpart);

            uint8_t already_in = 0;
            for(uint32_t j = 0; j < total_subpart; ++j){
                if(subpart_to_modify[j] == subpart){
                    already_in = 1;
                    break;
                }
            }
            if(already_in == 0){
                subpart_to_modify[total_subpart] = subpart;
                ++total_subpart;
            }
        }

        pthread_mutex_lock(&(state_locks[STATE_SUBPART]));

        for(uint32_t i = 0; i < total_subpart; ++i){
            //printf("subpart: %u \n", subpart_to_modify[i]);
            pthread_mutex_lock(&(state_locks[subpart_to_modify[i]]));
        }

        pthread_mutex_unlock(&(state_locks[STATE_SUBPART]));

        ++epoch;
        execute_batch(batch, epoch, batch_index);
        
        for(uint32_t i = 0; i < total_subpart; ++i){
            pthread_mutex_unlock(&(state_locks[subpart_to_modify[i]]));
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

void* rw_lock_generator(void* args){
    pthread_mutex_t* copy_lock = (pthread_mutex_t*)args;
    batch_t* batch = (batch_t*)malloc(sizeof(batch_t));
    rw_lock_generate_batch(batch);

    uint32_t epoch = 0;
    uint32_t n_batch = 0;
    struct timespec start, begin, now;
    timespec* diff = (timespec*)malloc(sizeof(timespec));
    double total_time, elapsed = 0;

    clock_gettime(CLOCK_REALTIME, &start);

    for(;;){

        clock_gettime(CLOCK_REALTIME, &begin);

        pthread_mutex_lock(copy_lock);
        ++epoch;
        execute_batch(batch, epoch, epoch%NUMBER_OF_BATCH);
        pthread_mutex_unlock(copy_lock);

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
}*/