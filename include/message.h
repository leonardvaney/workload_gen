#pragma once

#include <state.hpp>

struct addr_node_t {
    uint8_t id;
    char* ip;
    uint16_t port;
};

struct consensus_msg_t {
    uint8_t id_sender;
    addr_t batch[BATCH_SIZE]; //batch to send
    uint32_t epoch; //epoch to recover
    uint8_t recover; //1 if a node need to recover, 0 otherwise
    uint8_t id_recover; //id of the node that need to recover
}__attribute__((packed));

struct hash_msg_t {
    uint8_t id_sender;
    uint32_t state_part;
    unsigned char hash[SHA256_DIGEST_LENGTH];
}__attribute__((packed));

/*struct msg_t {
    uint8_t id_sender;
    union
    {
        consensus_msg_t cons;
        hash_msg_t hash;
    };
    
};*/

extern addr_node_t* node_list;
extern uint8_t total_node;
