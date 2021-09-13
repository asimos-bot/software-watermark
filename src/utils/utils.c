#include "utils/utils.h"

STACK* stack_create(unsigned long max_nodes) {

    STACK* stack = malloc(sizeof(STACK));
    stack->stack = calloc(max_nodes, sizeof(unsigned long));
    stack->n = 0;
    return stack;
}

void stack_push(STACK* stack, unsigned long node) {
    stack->stack[stack->n++] = node;
}
unsigned long stack_pop(STACK* stack) {
    return stack->n ? stack->stack[--stack->n] : ULONG_MAX;
}
void stack_pop_until(STACK* stack, unsigned long size) {

    stack->n = size;
}
unsigned long stack_get(STACK* stack) {
    return stack->n ? stack->stack[stack->n-1] : ULONG_MAX;
}
void stack_free(STACK* stack) {
    free(stack->stack);
    free(stack);
}
void stack_print(STACK* stack) {

    for(unsigned long i = 0; i < stack->n; i++) printf("%lu ", stack->stack[i]);
    printf("\n");
}

uint8_t get_bit(uint8_t* data, unsigned long idx) {

    uint8_t byte_idx = idx%8 ? 8-idx%8 : 0;
    idx = idx/8;
    return (data[idx] >> byte_idx) & 0x1;
}

void set_bit(uint8_t* data, unsigned long idx, uint8_t value) {

    uint8_t byte_idx = idx%8 ? 8-idx%8 : 0;
    idx = idx/8;
    if(value)
        data[idx] = (1 << byte_idx) | data[idx];
    else
        data[idx] = ~(1 << byte_idx) & data[idx];
}

void invert_binary_sequence(uint8_t* data, unsigned long size) {

    unsigned long n_bits = size * 8;
    for(unsigned long i = 0; i < n_bits; i++) {

        uint8_t bit1 = get_bit(data, i);
        uint8_t bit2 = get_bit(data, n_bits-i-1);
        set_bit(data, i, bit2);
        set_bit(data, n_bits-i-1, bit1);
    }
}

unsigned long get_first_positive_bit_index(uint8_t* data, unsigned long size_in_bytes) {

    for(unsigned long i = 0; i < size_in_bytes; i++)
        if(data[i])
            for(uint8_t j = 0; j < 8; j++)
                if(get_bit(data, i*8 + j)) return i*8+j;
    return ULONG_MAX;
}

uint8_t* get_sequence_from_bit_arr(uint8_t* bit_arr, unsigned long n_bits, unsigned long* num_bytes) {

    uint8_t n_zeros_on_the_left = n_bits%8 ? 8 - n_bits % 8 : 0;
    *num_bytes = n_bits/8 + !!n_zeros_on_the_left;
    uint8_t* data = malloc(*num_bytes);
    data[0] = 0;

    for(unsigned long i = 0; i < n_bits; i++) set_bit(data, i+n_zeros_on_the_left, bit_arr[i]);
    return data;
}

uint8_t binary_sequence_equal(uint8_t* data1, uint8_t* data2, unsigned long num_bytes1, unsigned long num_bytes2) {

    for(unsigned long i = 0; i < num_bytes1 && i < num_bytes2; i++) {

        if(data1[num_bytes1-i-1] != data2[num_bytes2-i-1]) return 0;
    }
    return 1;
}
