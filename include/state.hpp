#pragma once

#include <stdio.h>
#include <stdint.h>
#include <malloc.h>
#include <pthread.h>

#ifdef LOCAL
#define STATE_SIZE 100000
#else
#define STATE_SIZE 16000000000
#endif

#define STATE_SUBPART 10000

#ifdef LOCAL
#define BATCH_SIZE 1000
#else
#define BATCH_SIZE 64000
#endif

#define NUMBER_OF_BATCH 100

typedef uint64_t addr_t;

struct batch_t {
    addr_t* addr;
};

static uint32_t* cells;

static uint8_t rw_bit = 0; //0 = can rw on first half (ro on second), 1 = can rw on second half (ro on first)

void init_state();

uint32_t* get_cells();

void write_state(addr_t addr, uint32_t value);

uint32_t read_state(addr_t addr);

void execute_batch(batch_t* batch, uint32_t epoch, uint32_t batch_index);

void change_rw_bit();

uint8_t get_rw_bit();

void* copy_data(void* args);