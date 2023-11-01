#pragma once

#include <state.hpp>
#include <transfer.hpp>
#include <stdlib.h>

static addr_node_t* node_list;
static uint8_t total_node;

void init_consensus(addr_node_t* list, uint8_t total);

void send_batch();