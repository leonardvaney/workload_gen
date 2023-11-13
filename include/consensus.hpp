#pragma once

#include <state.hpp>
#include <message.h>
#include <fifo.hpp>

#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>

extern addr_node_t* node_list;
extern uint8_t total_node; 

//Of size total_node - 1;
static int* connfd_list_consensus;
//static int* sockfd_list;

void init_consensus();
/*
void send_batch();

void* open_client_consensus(void* args);

void* open_server(void* args);*/