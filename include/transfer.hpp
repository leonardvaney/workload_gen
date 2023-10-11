#pragma once

#include <state.hpp>
#include <arpa/inet.h>
#include <unistd.h>

#define TRANSFER_PORT 8886

static int connfd;
static int sockfd;

void init_server();

void init_client();

void send_state();

void send_state_progressive_lock(pthread_mutex_t* state_locks);

void receive_state();
