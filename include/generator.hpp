#pragma once

#include <state.hpp>
#include <stdlib.h>

#ifdef LOCAL
#define BATCH_SIZE 1000
#else
#define BATCH_SIZE 64000
#endif

struct batch_t {
    addr_t addr[BATCH_SIZE];
};

batch_t generate_batch();

void execute_batch(batch_t* batch, uint32_t epoch);

void* full_lock_generator(void* args);