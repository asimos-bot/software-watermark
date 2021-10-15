#ifndef CONNECTION_H
#define CONNECTION_H

#include <stdio.h>
#include <string.h>

struct NODE; //just so we can point to NODE struct before including node.h

typedef enum DIRECTION {
    IN,
    OUT
} DIRECTION;

//connections, pointing to the next connection on the simple linked list
typedef struct CONNECTION{

    struct NODE* parent; // node this connection struct belongs to
    struct NODE* to; // the node this connections goes to
    struct NODE* from; // the node this connection comes from
    struct CONNECTION* next;
    struct CONNECTION* prev;
} CONNECTION;

#include "node/node.h" //we can't include it before 'struct NODE;'

//create empty connection
CONNECTION* connection_create(NODE* parent, NODE* from, NODE* to);

void connection_insert(CONNECTION* connection, CONNECTION* conn);
void connection_insert_in_neighbour(CONNECTION* connection_root, NODE* node);
void connection_insert_out_neighbour(CONNECTION* connection_root, NODE* node);

CONNECTION* connection_search_in_neighbour(CONNECTION* connection_node, NODE* graph_node);
CONNECTION* connection_search_out_neighbour(CONNECTION* connection_node, NODE* graph_node);

void connection_delete(CONNECTION* conn);
uint8_t connection_delete_in_neighbour(CONNECTION* connection_node, NODE* graph_node);
uint8_t connection_delete_out_neighbour(CONNECTION* connection_node, NODE* graph_node);

void connection_free(CONNECTION* connection_root);

void connection_print(CONNECTION* connection, void (*)(FILE*, NODE*));

#endif
