#include <generator.hpp>

batch_t generate_batch(){

    batch_t batch;

    for(uint32_t i = 0; i < BATCH_SIZE; ++i){
        size_t random_value = rand();
        double r = (double)random_value;
        size_t cell_number = (size_t)((r / RAND_MAX) * STATE_SIZE);
        batch.addr[i] = cell_number;
    }

    return batch;
}

void execute_batch(batch_t* batch, uint32_t epoch){
    for(uint32_t i = 0; i < BATCH_SIZE; ++i){
        write_state(batch->addr[i], epoch);
    }
}