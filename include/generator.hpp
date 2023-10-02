#pragma once

#include <state.hpp>
#include <stdlib.h>

#define BATCH_SIZE 64000

struct batch_t {
    addr_t addr[BATCH_SIZE];
};

batch_t generate_batch();

void execute_batch(batch_t* batch, uint32_t epoch);