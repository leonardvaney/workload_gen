#pragma once

#include <state.hpp>
#include <message.hpp>
#include <fifo.hpp>

#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>

static addr_node_t* node_list;
static uint8_t total_node;

//Of size total_node - 1;
static int* connfd_list;
//static int* sockfd_list;

void init_consensus(addr_node_t* list, uint8_t total);
/*
void send_batch();

void* open_client_consensus(void* args);

void* open_server(void* args);*/