#include <state.hpp>

void init_state(){
    rw_bit = 0;
    cells = (uint32_t*)malloc(sizeof(uint32_t) * STATE_SIZE);
    for(uint64_t i = 0; i < STATE_SIZE; ++i){
        cells[i] = 0;
    }
}

uint32_t* get_cells(){
    return cells;
}

void write_state(addr_t addr, uint32_t value){
    cells[addr] = value;
}

uint32_t read_state(addr_t addr){
    return cells[addr];
}

void execute_batch(batch_t* batch, uint32_t epoch, uint32_t batch_index){
    for(uint32_t i = 0; i < BATCH_SIZE; ++i){
        //printf("addr: %d \n", batch->addr[i + BATCH_SIZE*batch_index]);
        if(rw_bit == 1){
            write_state(batch->addr[i + BATCH_SIZE*batch_index] + STATE_SIZE/2, epoch);    
        }
        else{
            write_state(batch->addr[i + BATCH_SIZE*batch_index], epoch);
        }
    }
}

void change_rw_bit(){
    if(rw_bit == 0){
        rw_bit = 1;
    }
    else{
        rw_bit = 0;
    }
}

uint8_t get_rw_bit(){
    return rw_bit;
}

void* copy_data(void* args){
    pthread_mutex_t* copy_lock = ((pthread_mutex_t*)args);

    pthread_mutex_lock(copy_lock);

    change_rw_bit();

    if(rw_bit == 0){ //Transfer from second half to first half
        memcpy(cells, cells + (STATE_SIZE / 2), sizeof(*cells) * STATE_SIZE / 2 );
    }
    else{ //Transfer from first half to second half
        memcpy(cells + (STATE_SIZE / 2), cells, sizeof(*cells) * STATE_SIZE / 2);
    }

    pthread_mutex_unlock(copy_lock);
}