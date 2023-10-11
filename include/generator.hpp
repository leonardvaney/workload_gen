#pragma once

#include <state.hpp>
#include <stdlib.h>

void generate_batch(batch_t* batch);

void time_diff(timespec start, timespec end, timespec* result);

void* full_lock_generator(void* args);

void* progressive_lock_generator(void* args);