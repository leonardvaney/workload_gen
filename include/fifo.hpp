#pragma once

#include <message.h>

#include <pthread.h>
#include <stdint.h>

//#define MAX_FIFO 256
#define MAX_MSG_LIST 10

struct fifo_t {
    consensus_msg_t* msg_list;
    size_t size_list; //Need guards if equals to MAX_MSG_LIST
    pthread_mutex_t lock;
};

static pthread_mutex_t fifo_lock;

static uint8_t fifo_size;
static fifo_t* fifo; 

void init_fifo(uint8_t total_node); //Init one fifo for each id node (minus the consensus one)

void add_to_fifo(consensus_msg_t* msg); //Add a consensus msg to every fifo

int get_fifo_msg(uint8_t id, consensus_msg_t* result); //Store a copy of the oldest message from fifo number "id" in "result" and remove this message from the fifo