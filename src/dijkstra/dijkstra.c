#include "dijkstra/dijkstra.h"

typedef enum {
    SEQUENCE,
    REPEAT,
    IF,
    WHILE,
    IF_ELSE,
    P_CASE,
    INVALID
} STATEMENT_GRAPH;

typedef struct PRIME_SUBGRAPH {

    NODE* source;
    NODE* sink;
    STATEMENT_GRAPH type;
} PRIME_SUBGRAPH;

PRIME_SUBGRAPH dijkstra_is_non_trivial_prime(NODE* source) {

    PRIME_SUBGRAPH prime = { .source = NULL, .sink = NULL, .type = INVALID};

    if(!source->num_out_neighbours) return prime;

    // sequence or repeat
    if(source->num_out_neighbours == 1 && source->out->to->num_in_neighbours == 1) {
        NODE* dest = source->out->to;
        // if destination has two out neighbours and one of them
        // is source
        if(dest->num_out_neighbours == 2 && graph_get_connection(dest, source)) {

            prime.type = REPEAT;
            prime.sink = dest->out->to == source ? dest->out->next->to : dest->out->to;
            return prime;
        } else if(!graph_get_connection(dest, source)){
            prime.type = SEQUENCE;
            prime.sink = dest;
            return prime;
        }
    // while or if
    } else if(source->num_out_neighbours == 2 &&
            source->graph_idx < source->out->to->graph_idx && source->graph_idx < source->out->next->to->graph_idx) {

        NODE* node1 = source->out->to;
        NODE* node2 = source->out->next->to;

        // if this is a if-then, one of the destinations connects back to the other one
        if(graph_get_connection(node1, node2) || graph_get_connection(node2, node1)) {

            uint8_t node1_connects_to_node2 = !!graph_get_connection(node1, node2);
            NODE* middle_node = node1_connects_to_node2 ? node1 : node2;
            NODE* final_node = node1_connects_to_node2 ? node2 : node1;

            if( middle_node->num_in_neighbours == 1 && middle_node->num_out_neighbours == 1 &&
                final_node->num_in_neighbours == 2) {

                prime.type = IF;
                prime.sink = final_node;
                return prime;
            }
        } else if(graph_get_connection(node1, source) || graph_get_connection(node2, source)) {
        // if this is a while, one of the destinations connects back to source

            uint8_t node1_connects_back = !!graph_get_connection(node1, source);
            NODE* connect_back_node = node1_connects_back ? node1 : node2;
            NODE* other_node = node1_connects_back ? node2 : node1;
            if( connect_back_node->num_in_neighbours == 1 && connect_back_node->num_out_neighbours == 1 &&
                other_node->num_in_neighbours == 1) {

                prime.type = WHILE;
                prime.sink = other_node;
                return prime;
            }
        // if else block
        }
    }

    prime.type = INVALID;
    prime.sink = NULL;
    return prime;
}

void dijkstra_contract(NODE* source, NODE* sink) {

    // delete all nodes between source and sin
    while(source->graph_idx+1 != sink->graph_idx) {
        graph_delete(source->graph->nodes[source->graph_idx+1]);
    }

    // move all outer neighbours from sink to source
    while(sink->out) {

        graph_oriented_connect(source, sink->out->to);
        graph_oriented_disconnect(sink, sink->out->to);
    }

    // delete sink
    graph_delete(sink);

    // delete auto-references
    while(graph_get_connection(source, source)) {
        graph_oriented_disconnect(source, source);
    }
}

int watermark_is_dijkstra(GRAPH* watermark) {

    // graphs already come in topologically sorted from
    // the decoding and encoding processses

    if(watermark->num_connections >= 2 * watermark->num_nodes - 1) return 0;

    // iterate through graph in inverse topological order
    PRIME_SUBGRAPH prime;
    unsigned long num_nodes = watermark->num_nodes;
    // iterate all nodes
    for(unsigned long i = 0; i < num_nodes; i++) {

        NODE* node = watermark->nodes[num_nodes-1-i];
        if(( prime = dijkstra_is_non_trivial_prime(node) ).type != INVALID ) {

            dijkstra_contract(node, prime.sink);
            // repeat until no prime is found in this node
            while(( prime = dijkstra_is_non_trivial_prime(node) ).type != INVALID) {
                dijkstra_contract(node, prime.sink);
            }
        }
    }
    uint8_t result = watermark->num_nodes == 1;
    graph_free(watermark);

    return result;
}

char* watermark_dijkstra_code(GRAPH* watermark) {

    // graphs already come in topologically sorted from
    // the decoding and encoding processses

    if(watermark->num_connections >= 2 * watermark->num_nodes - 1) return 0;

    char* code = NULL;

    // iterate through graph in inverse topological order
    PRIME_SUBGRAPH prime;
    unsigned long num_nodes = watermark->num_nodes;
    // iterate all nodes
    for(unsigned long i = 0; i < num_nodes; i++) {

        NODE* node = watermark->nodes[num_nodes-1-i];
        if(( prime = dijkstra_is_non_trivial_prime(node) ).type != INVALID ) {

            dijkstra_contract(node, prime.sink);
            // repeat until no prime is found in this node
            while(( prime = dijkstra_is_non_trivial_prime(node) ).type != INVALID) {
                dijkstra_contract(node, prime.sink);
            }
        }
    }
    uint8_t result = watermark->num_nodes == 1;
    graph_free(watermark);

    return code;
}

int watermark_dijkstra_equal(GRAPH* a, GRAPH* b) {

    char* a_code = watermark_dijkstra_code(a);
    char* b_code = watermark_dijkstra_code(b);

    uint8_t result = strlen(a_code) == strlen(b_code) && !strcmp(a_code, b_code);
    free(a_code);
    free(b_code);
    return result;
}
