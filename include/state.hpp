#pragma once

#include <stdio.h>
#include <stdint.h>
#include <malloc.h>
#include <pthread.h>
#include <string.h>

#include <openssl/sha.h>

#ifdef LOCAL
#define STATE_SIZE 1200000
#else
#define STATE_SIZE 12000000000
#endif

#define STATE_SUBPART 10000

#ifdef LOCAL
#define BATCH_SIZE 100
#else
#define BATCH_SIZE 128000
#endif

#define NUMBER_OF_BATCH 10

typedef uint64_t addr_t;

struct batch_t {
    addr_t* addr;
};

static uint32_t* cells;

static uint8_t rw_bit = 1; //0 = can rw on first half (ro on second), 1 = can rw on second half (ro on first)

static pthread_mutex_t transfer_lock;

void init_state();

uint32_t* get_cells();

void write_state(addr_t addr, uint32_t value);

uint32_t read_state(addr_t addr);

void execute_batch(batch_t* batch, uint32_t epoch, uint32_t batch_index);

void change_rw_bit();

uint8_t get_rw_bit();

void* copy_data(void* args);

void hash_state_elements(uint64_t start, uint64_t end, unsigned char* result);