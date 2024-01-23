#pragma once

#include <stdio.h>
#include <message.h>
#include <node.hpp>

#ifdef LOCAL
#define CATCHUP_LIMIT 10000
#else
#define CATCHUP_LIMIT 13000
#endif

static pthread_mutex_t queue_lock;

static consensus_msg_t* message_queue;
static int front; 
static int rear;
static int stop;

static struct timespec start, begin, now;
static timespec* diff;

void init_queue();

void* queue_thread(void* args);

void stop_queue();

int is_full();

int is_empty();

void enqueue_message(consensus_msg_t* message);

consensus_msg_t dequeue_message();