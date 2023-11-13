#pragma once

#include <state.hpp>

struct addr_node_t {
    uint8_t id;
    char* ip;
    uint16_t port;
};

struct consensus_msg_t {
    addr_t batch[BATCH_SIZE]; //batch to send
    uint32_t epoch; //epoch to recover
    uint8_t recover; //1 if a node need to recover, 0 otherwise
    uint8_t id_recover; //id of the node that need to recover
    uint8_t id_sender; //id of the sender
};

struct hash_msg_t {
    //TO DEFINE
};

extern addr_node_t* node_list;
extern uint8_t total_node;
