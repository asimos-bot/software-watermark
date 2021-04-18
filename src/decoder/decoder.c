#include "decoder/decoder.h"

typedef struct WM_NODE {

	// both 0 based idx, ready to use
	unsigned long hamiltonian_idx;
	unsigned long stack_idx;
} WM_NODE;

typedef struct DECODER {

	#ifdef DEBUG
		GRAPH* graph;
	#endif

	GRAPH* current_node;

	STACKS stacks;

	unsigned long n_bits;
} DECODER;

void label_new_current_node(DECODER* decoder) {

	// get parity	
	uint8_t is_odd = !(decoder->stacks.history.n & 1);

	// get indexes
	unsigned long stack_idx = is_odd ? decoder->stacks.odd.n : decoder->stacks.even.n;
	WM_NODE wm_node_info = { decoder->stacks.history.n, stack_idx };

	add_node_to_stacks(&decoder->stacks, decoder->current_node, is_odd);

	// get WM_NODE on the node
	graph_alloc(decoder->current_node, sizeof(WM_NODE));
	memcpy(decoder->current_node->data, &wm_node_info, decoder->current_node->data_len);
}

DECODER* decoder_create(GRAPH* graph, unsigned long n_bits) {

	DECODER* decoder = malloc( sizeof(DECODER) );
	decoder->current_node = graph;

	create_stacks(&decoder->stacks, n_bits);

	decoder->n_bits = n_bits;

	#ifdef DEBUG
		decoder->graph = graph;
	#endif

	return decoder;
}

uint8_t is_inner_node(DECODER* decoder, GRAPH* node) {

	WM_NODE* node_info = node->data;
	uint8_t is_odd = !(node_info->hamiltonian_idx & 1);

	PSTACK* stack = get_parity_stack(&decoder->stacks, is_odd);

	return !( node_info->stack_idx < stack->n && stack->stack[node_info->stack_idx] == node );
}

#ifdef DEBUG
void print_node(void* data, unsigned int data_len) {

	if( data ) {
		printf("\x1b[33m %lu %s\x1b[0m",
				data_len == sizeof(unsigned long) ? *(unsigned long*)data : ((WM_NODE*)data)->hamiltonian_idx + 1,
				data_len == sizeof(unsigned long) ? "(untouched)" : "(WM_NODE)");
	} else {	
		printf("\x1b[33m null \x1b[0m");
	}
}

void print_stacks(DECODER* decoder) {

	printf("e o h\n");
	for( unsigned long i=0; i < decoder->stacks.history.n; i++) {

		unsigned long even = i < decoder->stacks.even.n ? ((WM_NODE*)decoder->stacks.even.stack[i]->data)->hamiltonian_idx+1 : 0;
		unsigned long odd = i < decoder->stacks.odd.n ? ((WM_NODE*)decoder->stacks.odd.stack[i]->data)->hamiltonian_idx+1 : 0;
		printf("%lu %lu %lu %s\n", even, odd, decoder->stacks.history.stack[i], decoder->stacks.history.n & 1 ? "(e)" : "(o)");
	}
}

void print_inner_node_log(DECODER* decoder, WM_NODE* dest, unsigned long i, uint8_t* bit_arr) {

	printf("ERROR (idx %lu): dest node %lu is inner node\n", i+1, dest->hamiltonian_idx+1);
	printf("dest node info:\n");
	printf("\tis_odd: %d\n", !(dest->hamiltonian_idx & 1));
	printf("\tstack_idx (0-based): %lu\n", dest->stack_idx);
	printf("\thamiltonian_idx (1-based): %lu\n", dest->hamiltonian_idx);
	printf("current bit arr: ");
	for( unsigned long j=0; j < i; j++ ) printf("%hhu ", bit_arr[j]);
	printf("\n");
	print_stacks(decoder);
	graph_print(decoder->graph, print_node);
}

void print_bit_arr(uint8_t* bit_arr, unsigned long n) {

	for(unsigned long i = 0; i < n; i++) printf("%hhu ", bit_arr[i]);
	printf("\n");
}
#endif

uint8_t* get_bit_array2014(DECODER* decoder) {

	uint8_t* bit_arr = malloc( sizeof(uint8_t) * decoder->n_bits );

	// iterate over every node but the last
	for(unsigned long i=0; i < decoder->n_bits; i++ ) {

		// check if there is a backedge ( connection with non-zero data_len is a valid backedge)
		GRAPH* dest_node = get_backedge(decoder->current_node);
		if( dest_node ) {

			WM_NODE* dest = ((WM_NODE*)dest_node->data);

			// check if it is inner node
			if( is_inner_node(decoder, dest_node) ) {
				#ifdef DEBUG
					print_inner_node_log(decoder, dest, i, bit_arr);
				#endif
				free(bit_arr);
				return NULL;
			}

			pop_stacks(&decoder->stacks, dest->stack_idx, dest->hamiltonian_idx);
			// 1 if diff is odd, 0 otherwise
			bit_arr[i] = (i - dest->hamiltonian_idx) & 1;

		// check if this is the first node (the input graph must have the first
		// hamiltonian node as the first node in the list)
		} else if( !decoder->current_node->prev ) {

			bit_arr[0]=1;

		// else the node could always have connected to the first node (since it is
		// never an inner vertex). Since it didn't, it means it has the value inverse
		// to the one it would have if it connected to the first node
		} else {
			// if a bit is 1, the backedge should go to a node with different parity
			// if a bit is 0, the backedge should go to a node with the same parity
			// since there is no backedge here, and we can always connect to the first
			// node, we just need to see what is the current node's parity, to know
			// the bit it encodes (it is the value which can't be represented by an
			// backedge to the first node)

			// 1 - current_node = i. If i is odd, the bit is 0, if i is even the bit is 1.
			bit_arr[i] = !(i & 1);

			// however, if there was a possible backedge for the even stack, this only means
			// that the graph is invalid
			if( get_parity_stack(&decoder->stacks, 0)->n ) {
				free(bit_arr);
				return NULL;
			}
		}

		label_new_current_node(decoder);
		decoder->current_node = get_next_hamiltonian_node2014(decoder->current_node);
	}

	return bit_arr;
}

unsigned long get_node_idx(GRAPH* node) {

	return (*(WM_NODE*)node->data).hamiltonian_idx;
}

uint8_t* get_bit_array2017(DECODER* decoder) {

	uint8_t* bit_arr = malloc( sizeof(uint8_t) * decoder->n_bits );
	memset(bit_arr, 0x00, sizeof(uint8_t) * decoder->n_bits);

	// if positive, decrement it, if it is zero the current node is a forward destination
	int forward_flag = -1;

	// iterate over every node but the last
	for(unsigned long i=0; i < decoder->n_bits; i++ ) {

		if( forward_flag != 0 ) {

			if( get_forward_edge(decoder->current_node) ) {

				bit_arr[i] = 1;
				forward_flag = 2;
			} else if( get_backedge(decoder->current_node) && ( (i - get_node_idx(get_backedge(decoder->current_node))) & 1 ) ) {

				bit_arr[i] = 1;
			} else {

				bit_arr[i] = 0;
			}
		}

		if( forward_flag >= 0 ) forward_flag--;
		label_new_current_node(decoder);
		decoder->current_node = get_next_hamiltonian_node(decoder->current_node);
	}

	return bit_arr;

}

uint8_t* get_bit_sequence(uint8_t* bit_arr, unsigned long n_bits) {

	unsigned long num_bytes = n_bits / 8 + !!( n_bits % 8 ); 
	uint8_t* data = malloc( sizeof(uint8_t) * num_bytes );
	memset(data, 0x00, num_bytes);

	// get trailing_zeros in first byte
	uint8_t offset = n_bits % 8 ? 8 - (n_bits % 8) : 0;

	for( unsigned long i = 0; i < n_bits; i++ ) set_bit( data, i+offset, bit_arr[i] );

	return data;
}

void decoder_free(DECODER* decoder) {

	free_stacks(&decoder->stacks);
	free(decoder);
} 

void* _watermark_decode(GRAPH* graph, unsigned long* num_bytes, unsigned long n_bits, uint8_t* (*bit_array_func)(DECODER* decoder)) {

	if( !is_graph_structure_valid(graph) ) return NULL;

	// 1. get number of bits and set nodes to null
	*num_bytes = n_bits/8 + !!(n_bits%8);

	// 2. create decoder struct
	DECODER* decoder = decoder_create(graph, n_bits);

	// 3. get bit array (nodes are labeled as they are evaluated)
	uint8_t* bit_arr = bit_array_func(decoder);

	if( !bit_arr ) return NULL;

	// 4. turn bit array into bit sequence
	uint8_t* data = get_bit_sequence( bit_arr, n_bits);

	free(bit_arr);
	decoder_free(decoder);

	return data;
}

void* _watermark_decode_with_rs(GRAPH* graph, unsigned long* num_bytes, unsigned long num_rs_bytes, void* (*decode_func)(GRAPH*, unsigned long*)) {

	if( num_rs_bytes >= *num_bytes ) return NULL;

	uint8_t* data = decode_func(graph, num_bytes);

	int result = rs_decode(data, *num_bytes, (uint16_t*)( data + (*num_bytes - num_rs_bytes) ), num_rs_bytes);

	// if there were no errors or they were corrected
	if( result >= 0 ) return data;

	// if there were errors and they could not be corrected
	free(data);
	return NULL;
}

void* watermark2014_decode(GRAPH* graph, unsigned long* num_bytes) {

	return _watermark_decode(graph, num_bytes, get_num_bits2014(graph), get_bit_array2014);
}

void* watermark2014_decode_with_rs(GRAPH* graph, unsigned long* num_bytes, unsigned long num_rs_bytes) {

	return _watermark_decode_with_rs(graph, num_bytes, num_rs_bytes, watermark2014_decode);
}

void* watermark2017_decode(GRAPH* graph, unsigned long* num_bytes) {

	return _watermark_decode(graph, num_bytes, get_num_bits(graph), get_bit_array2017);
}

void* watermark2017_decode_with_rs(GRAPH* graph, unsigned long* num_bytes, unsigned long num_rs_bytes) {

	return _watermark_decode_with_rs(graph, num_bytes, num_rs_bytes, watermark2017_decode);
}
