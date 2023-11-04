#pragma once

#include <state.hpp>
#include <transfer.hpp>
#include <message.hpp>
#include <stdlib.h>

static addr_node_t* node_list;
static uint8_t total_node;

//Of size total_node - 1;
static int* connfd_list;
static int* sockfd_list;

void init_consensus(addr_node_t* list, uint8_t total);

void send_batch();