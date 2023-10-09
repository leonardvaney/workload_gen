#pragma once

#include <state.hpp>
#include <stdlib.h>

void generate_batch(batch_t* batch);

void* full_lock_generator(void* args);