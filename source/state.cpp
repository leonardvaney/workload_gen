#include <state.hpp>

void init_state(){
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
        write_state(batch->addr[i + BATCH_SIZE*batch_index], epoch);
    }
}