#pragma once

#include <state.hpp>

struct addr_node_t {
    uint8_t id;
    char* ip;
    uint16_t port;
};

struct consensus_msg_t {
    uint32_t epoch; //epoch to update or recover
    uint8_t recover; //1 if a node need to recover, 0 otherwise
    uint8_t id; //id of the node that need to recover
};

struct hash_msg_t {
    //TO DEFINE
};