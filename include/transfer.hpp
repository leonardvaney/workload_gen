#pragma once

#include <state.hpp>
#include <arpa/inet.h>
#include <unistd.h>

#define TRANSFER_PORT 8887

static int connfd;
static int sockfd;

void init_server();

void init_client();

void send_state();

void receive_state();
