#pragma once

#include <state.hpp>
#include <arpa/inet.h>
#include <unistd.h>

static int connfd;
static int sockfd;

void init_server();

void init_client();

void send_state();

void receive_state();