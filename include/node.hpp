#pragma once

#include <state.hpp>
#include <message.h>
#include <generator.hpp>

#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

static uint8_t node_id;

//Of size total_node - 1;
static int* connfd_list_node;

static pthread_mutex_t node_lock;
static int recover_mode; //0 if no recover, 1 if recover activated for an external node
static uint8_t id_recover; //node that recover

static unsigned char*** hash_result;

static struct timespec start_recover, now_recover;
static timespec* diff_recover;

static struct timespec start_batch, begin_batch, now_batch;
static timespec* diff_batch;


void init_node(uint8_t id);