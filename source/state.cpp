#include <state.hpp>

void init_state(){
    rw_bit = 0;
    pthread_mutex_init(&transfer_lock, NULL);
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
    change_rw_bit();

    if(rw_bit == 0){ //Transfer from second half to first half
        memcpy(cells, cells + (STATE_SIZE / 2), sizeof(*cells) * STATE_SIZE / 2 );
    }
    else{ //Transfer from first half to second half
        memcpy(cells + (STATE_SIZE / 2), cells, sizeof(*cells) * STATE_SIZE / 2);
    }
    return NULL;
}

void hash_state_elements(uint64_t start, uint64_t end, unsigned char* result){
    unsigned char hash[SHA256_DIGEST_LENGTH];

    SHA256_CTX sha256;
    SHA256_Init(&sha256);

    if(rw_bit == 0){
        SHA256_Update(&sha256, get_cells() + STATE_SIZE/2 + start, sizeof(uint32_t) * (end-start));
    }
    else{
        SHA256_Update(&sha256, get_cells() + start, sizeof(uint32_t) * (end-start));
    }

    SHA256_Final(hash, &sha256);
    memcpy(result, hash, sizeof(unsigned char) * SHA256_DIGEST_LENGTH);
}