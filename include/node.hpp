#pragma once

#include <state.hpp>
#include <message.h>
#include <generator.hpp>
#include <transfer.hpp>

#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

static uint8_t node_id;

//Of size total_node - 1;
static int* connfd_list_node;

static int simulate_crash;

static pthread_mutex_t node_lock, node_crash;
static int recover_mode; //0 if no recover, 1 if recover activated for an external node
static uint8_t id_recover; //node that recover

static size_t state_part;

static unsigned char*** hash_result;

static struct timespec start_recover, now_recover;
static timespec* diff_recover;

static struct timespec start_batch, begin_batch, now_batch;
static timespec* diff_batch;

static struct timespec start_copy, begin_copy, now_copy;
static timespec* diff_copy;

static struct timespec start_transfert, begin_transfert, now_transfert;
static timespec* diff_transfert;


void init_node(uint8_t id);