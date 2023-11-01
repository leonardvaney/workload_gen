#include <consensus.hpp>

void init_consensus(addr_node_t* list, uint8_t total){
    //1) crée des batch
    //2) ouvre un socket client side avec toutes les autres nodes
    //3) commence un thread qui ouvre un socket serveur pour intercepter les demandes de recover
    //4) loop sur l'envoi de batch (ajouter un délai pour éviter de spam ou attendre un retour des autres nodes avant de continuer?)

    srand(1000);

    batch_t* batch = (batch_t*)malloc(sizeof(batch_t));
    batch->addr = (addr_t*)malloc(sizeof(addr_t)*BATCH_SIZE*NUMBER_OF_BATCH);
    for(uint32_t i = 0; i < BATCH_SIZE*NUMBER_OF_BATCH; ++i){
        size_t random_value = rand();
        double r = (double)random_value;
        size_t cell_number = (size_t)((r / RAND_MAX) * STATE_SIZE);
        batch->addr[i] = cell_number;
    }

    node_list = list;
    total_node = total;
}

void send_batch(){

}