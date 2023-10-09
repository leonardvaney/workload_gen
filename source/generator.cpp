#include <generator.hpp>

void generate_batch(batch_t* batch){

    batch->addr = (addr_t*)malloc(sizeof(addr_t)*BATCH_SIZE*NUMBER_OF_BATCH);

    for(uint32_t i = 0; i < BATCH_SIZE*NUMBER_OF_BATCH; ++i){
        size_t random_value = rand();
        double r = (double)random_value;
        size_t cell_number = (size_t)((r / RAND_MAX) * STATE_SIZE);
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

        //Diff between begin and now
        long time_spent_sec = (now.tv_sec - begin.tv_sec);
        long time_spent_nsec = (now.tv_nsec - begin.tv_nsec);
        if(time_spent_nsec < 0){
            time_spent_sec -= 1;
            time_spent_nsec += 1000000000;
        }

        //Diff between start and now
        long time_spent_start_sec = (now.tv_sec - start.tv_sec);
        long time_spent_start_nsec = (now.tv_nsec - start.tv_nsec);
        if(time_spent_start_nsec < 0){
            time_spent_start_sec -= 1;
            time_spent_start_nsec += 1000000000;
        }

        double start_nsec_in_ms = ((double)time_spent_start_nsec / 1000000);
        double start_sec_in_ms = ((double)time_spent_start_sec *1000);
        total_time = start_sec_in_ms + start_nsec_in_ms;


        double nsec_in_sec = ((double)time_spent_nsec / 1000000000);
        elapsed += (double)time_spent_sec + nsec_in_sec;
        
        if(elapsed >= 1){
            printf("%f: Speed: %f op/s \n", total_time, (n_batch*BATCH_SIZE) / elapsed);
            n_batch = 0;
            elapsed = 0;
        }
    }
} 