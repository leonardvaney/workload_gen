#pragma once

#include <stdio.h>
#include <stdint.h>
#include <malloc.h>
#include <pthread.h>

#define STATE_SIZE 100000

typedef uint64_t addr_t;

static uint32_t* cells;

void init_state();

void write_state(addr_t addr, uint32_t value);

uint32_t read_state(addr_t addr);