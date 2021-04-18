#include "encoder/encoder.h"

typedef struct ENCODER {

	// nodes have pointer as data, which point to a list
	GRAPH* graph;
	GRAPH* final_node;

	STACKS stacks;
} ENCODER;

enum FORWARD_STATUS {
	NONE=0,
	FORWARD_SOURCE=1,
	FORWARD_INNER=2,
	FORWARD_DESTINATION=4,
	FORWARD_REMOVE_HAMILTONIAN=8
};

#ifdef DEBUG 
void print_stacks_encoder(ENCODER* encoder) {

	printf("e o h\n");
	for( unsigned long i=0; i < encoder->stacks.history.n; i++) {

		unsigned long even = i < encoder->stacks.even.n ? (*(unsigned long*)encoder->stacks.even.stack[i]->data) : 0;
		unsigned long odd = i < encoder->stacks.odd.n ? (*(unsigned long*)encoder->stacks.odd.stack[i]->data) : 0;
		printf("%lu %lu %lu %s\n", even, odd, encoder->stacks.history.stack[i], encoder->stacks.history.n & 1 ? "(e)" : "(o)");
	}
}
#endif

uint8_t get_trailing_zeroes(uint8_t* data, unsigned long data_len) {

	// praticamente python
	for(unsigned long i = 0; i < data_len; i++) {
		if(data[i]) {
			for(uint8_t j = 0; j < 8; j++) {
				if(get_bit(data, i*8+j)) {
					return i*8 + j;
				}
			}
		}
	}

	return data_len*8;
}

void add_node_to_graph(ENCODER* encoder) {

	GRAPH* new_node = graph_empty();

	graph_insert(encoder->final_node, new_node);
	graph_oriented_connect(encoder->final_node, new_node);
	encoder->final_node = new_node;
}

void add_idx(GRAPH* node, unsigned long idx) {

	idx++;
	graph_alloc(node, sizeof(unsigned long));
	*((unsigned long*)node->data) = idx;
}

ENCODER* encoder_create(unsigned long n_bits) {

	ENCODER* encoder = malloc( sizeof(ENCODER) );
	create_stacks(&encoder->stacks, n_bits);
	// create graph (all nodes are created without an index, except for the first one)
	// indices are added as we go through the graph
	unsigned long one = 1;
	encoder->final_node = encoder->graph = graph_create(&one, sizeof(unsigned long));
	add_node_to_stacks(&encoder->stacks, encoder->final_node, 1);

	for(unsigned long i = 1; i < n_bits+1; i++) {
		add_node_to_graph(encoder);
	}

	return encoder;
}

void encoder_free(ENCODER* encoder) {

	free_stacks(&encoder->stacks);
	free(encoder);
}

void encode2014(ENCODER* encoder, void* data, unsigned long total_bits, unsigned long trailing_zeroes) {

	//start from second node
	GRAPH* node = get_next_hamiltonian_node2014(encoder->graph);
	for(unsigned long i = trailing_zeroes+1; i < total_bits; i++) {

		// 0-based index of the node's position in the hamiltonian path
		unsigned long idx = i-trailing_zeroes;
		uint8_t is_odd = !((idx) & 1);
		uint8_t bit = get_bit(data, i);

		// 1. add index to node
		add_idx(node, idx);

		// 2. if bit is 1, connect to a different parity bit, otherwise connect to same parity
		add_backedge2014( &encoder->stacks, node, bit, is_odd );

		// 3. add node to proper parity stack, and to the history stack
		add_node_to_stacks( &encoder->stacks, node, is_odd );
		node = get_next_hamiltonian_node2014(node);
	}
}

uint8_t prev_has_backedge_at_same_parity_stack(GRAPH* node, GRAPH* last_node, uint8_t bit, uint8_t last_bit) {

	uint8_t is_odd = !((*(unsigned long*)node->data) & 1 );

	return ( ( bit ? !is_odd : is_odd ) == ( last_bit ? is_odd : !is_odd ) ) && get_backedge(last_node);
}

unsigned long num_possible_backedges(ENCODER* encoder, GRAPH* node, GRAPH* last_node, uint8_t bit, uint8_t last_bit) {

	uint8_t is_odd = !((*(unsigned long*)node->data) & 1 );

	PSTACK* node_stack = get_parity_stack(&encoder->stacks, bit ? !is_odd : is_odd);

	return node_stack->n - prev_has_backedge_at_same_parity_stack(node, last_node, bit, last_bit);
}

uint8_t update_forward_status(enum FORWARD_STATUS status) {

	// update forward_status
	if( status & FORWARD_DESTINATION ) status &= ~FORWARD_DESTINATION;
	if( status & FORWARD_INNER ) status = (status & ~FORWARD_INNER) | FORWARD_DESTINATION;
	if( status & FORWARD_SOURCE ) status = (status & ~FORWARD_SOURCE) | FORWARD_INNER;
	return status;
}

void encode2017(ENCODER* encoder, void* data, unsigned long total_bits, unsigned long trailing_zeroes) {

	//start from second node
	GRAPH* last_node = encoder->graph;
	uint8_t last_bit = 1;
	GRAPH* node = get_next_hamiltonian_node2014(encoder->graph);
	enum FORWARD_STATUS forward_status = NONE;
	for(unsigned long i = trailing_zeroes+1; i < total_bits; i++) {

		// 0-based index of the node's position in the hamiltonian path
		unsigned long idx = i-trailing_zeroes;
		uint8_t is_odd = !((idx) & 1);
		uint8_t bit = get_bit(data, i);

		if( !( forward_status & FORWARD_DESTINATION ) )  add_idx(node, idx);
		if( forward_status & FORWARD_DESTINATION ) {
			i--;
			// remove hamiltonian edge v-1 -> v if necessary
			if( forward_status & FORWARD_REMOVE_HAMILTONIAN ) {

				graph_oriented_disconnect(last_node, node);
				forward_status &= ~FORWARD_REMOVE_HAMILTONIAN;
			}
			// just to mark that we passed through this node
			node->data_len = UINT_MAX;
		// check if there is an backedge available
		} else if( num_possible_backedges(encoder, node, last_node, bit, last_bit) ) {

			if( forward_status & FORWARD_INNER ) {

				// don't add node to stacks
				if( bit ) {

					// add backedge to v - 1
					graph_oriented_connect(node, last_node);
					// ask to remove hamiltonian in the next iteration
					forward_status |= FORWARD_REMOVE_HAMILTONIAN;
				} else {

					// nothing
				}
			} else {

				// add random backedge
				uint8_t prev_connects_to_same_stack = prev_has_backedge_at_same_parity_stack(node, last_node, bit, last_bit);
				add_backedge(&encoder->stacks, node, prev_connects_to_same_stack, bit, is_odd);
			}
		} else {
			if( bit ) {
				// add node to graph and add forward edge
				forward_status |= FORWARD_SOURCE;
				add_node_to_graph(encoder);
				// works since untouched nodes are source to only one connection
				graph_oriented_connect(node, node->next->next);
			}
			if( !( forward_status & FORWARD_INNER ) ) add_node_to_stacks(&encoder->stacks, node, is_odd);
		}

		forward_status = update_forward_status(forward_status);
		last_node = node;
		last_bit = bit;
		node = node->next;
	}
}

GRAPH* _watermark_encode(void* data, unsigned long data_len, void (*encode)(ENCODER*, void*, unsigned long, unsigned long)) {

	if( !data || !data_len ) return NULL;

	srand(time(0));

	unsigned long trailing_zeroes = get_trailing_zeroes(data, data_len);

	// only zeroes
	if( trailing_zeroes == data_len * 8 ) return NULL;

	unsigned long n_bits = data_len * 8 - trailing_zeroes;

	ENCODER* encoder = encoder_create(n_bits);

	encode(encoder, data, n_bits + trailing_zeroes, trailing_zeroes);

	GRAPH* graph = encoder->graph;

	encoder_free(encoder);
	
	return graph;
}

GRAPH* _watermark_encode_with_rs(void* data, unsigned long data_len, unsigned long num_rs_bytes, GRAPH* (*watermark)(void*, unsigned long)) {

	uint16_t par[num_rs_bytes];
	memset(par, 0x00, num_rs_bytes);

	// get parity data
	rs_encode(data, data_len, par, num_rs_bytes);

	// copy data + parity data
	uint8_t final_data[data_len + num_rs_bytes];
	memcpy(final_data, data, data_len);
	memcpy(final_data + data_len, par, num_rs_bytes * 2);

	return watermark(final_data, data_len);
}

GRAPH* watermark2014_encode(void* data, unsigned long data_len) {

	return _watermark_encode(data, data_len, encode2014);
}

GRAPH* watermark2014_encode_with_rs(void* data, unsigned long data_len, unsigned long num_rs_bytes) {

	return _watermark_encode_with_rs(data, data_len, num_rs_bytes, watermark2014_encode);
}

GRAPH* watermark2017_encode(void* data, unsigned long data_len) {

	return _watermark_encode(data, data_len, encode2017);
}

GRAPH* watermark2017_encode_with_rs(void* data, unsigned long data_len, unsigned long num_rs_bytes) {

	return _watermark_encode_with_rs(data, data_len, num_rs_bytes, watermark2017_encode);
}


