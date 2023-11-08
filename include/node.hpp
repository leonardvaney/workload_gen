#pragma once

#include <state.hpp>
#include <message.h>

#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>

static uint8_t node_id;

//Of size total_node - 1;
static int* connfd_list_node;

void init_node(uint8_t id);