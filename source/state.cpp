#include <state.hpp>

void init_state(){
    cells = (uint32_t*)malloc(sizeof(uint32_t) * STATE_SIZE);
    for(uint64_t i = 0; i < STATE_SIZE; ++i){
        cells[i] = 0;
    }
}

void write_state(addr_t addr, uint32_t value){
    cells[addr] = value;
}

uint32_t read_state(addr_t addr){
    return cells[addr];
}