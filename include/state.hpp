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

struct modified_subpart_t {
    uint32_t* part_to_lock;
    uint32_t size;
};

struct batch_t {
    addr_t* addr;
    modified_subpart_t* subpart;
};

static uint32_t* cells;

void init_state();

uint32_t* get_cells();

void write_state(addr_t addr, uint32_t value);

uint32_t read_state(addr_t addr);

void execute_batch(batch_t* batch, uint32_t epoch, uint32_t batch_index);