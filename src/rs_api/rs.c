#include "rs_api/rs.h"
#include "utils/utils.h"
#include "rs_api/rslib.h"

#define XOR_MASK 0x00

#define show_bits(bits,len) fprintf(stderr, "%s:%d:" #bits ":", __FILE__, __LINE__);\
  for(unsigned long i = 0; i < len; i++) {\
    fprintf(stderr, "%hhu", get_bit(bits, i));\
  }\
  fprintf(stderr, "\n");

struct rs_control* get_rs_struct(int symbol_size, int parity_len) {

  int gfpoly, fcr, prim;
  switch(symbol_size) {
    case 3:
      gfpoly = 0x3;
      fcr = 1;
      prim = 1;
      break;
    default:
    case 8:
      gfpoly = 0x187;
      fcr = 0;
      prim = 1;
      break;
  }

	// parity_len == nroots
  #if defined(_OPENMP)
    #pragma omp critical
    {
  #endif
  struct rs_control* rs = init_rs(symbol_size, gfpoly, fcr, prim, parity_len);
  if(!rs) {
    fprintf(stderr, "'init_rs' returned %p for symbol_size: %d and parity_len: %d\n", (void*)rs, symbol_size, parity_len);
    exit(EXIT_FAILURE);
  }
  return rs;
  # if defined(_OPENMP)
    }
  #endif
}

// give data, data_len and parity_len, get parity back
int rs_encode8(uint8_t* data, int data_len, uint8_t* parity, int number_of_parity_symbols) {

	struct rs_control* rs = get_rs_struct(8, number_of_parity_symbols);

  uint16_t parity16[number_of_parity_symbols];
  memset(parity16, 0x00, sizeof(uint16_t) * number_of_parity_symbols);

  #if defined(_OPENMP)
    #pragma omp critical
  #endif
	int res = encode_rs8(rs, data, data_len, parity16, XOR_MASK);
  for(int i = 0; i < number_of_parity_symbols; i++) parity[i] = parity16[i];

  #if defined(_OPENMP)
    #pragma omp critical
  #endif
	free_rs(rs);
  return res;
}

int rs_encode(uint8_t* data, int data_len, uint16_t* parity, int number_of_parity_symbols, int symsize) {
  struct rs_control* rs = get_rs_struct(symsize, number_of_parity_symbols);
  memset(parity, 0x00, sizeof(uint16_t) * number_of_parity_symbols);

  #if defined(_OPENMP)
    #pragma omp critical
  #endif
	int res = encode_rs8(rs, data, data_len, parity, XOR_MASK);
  uint8_t has_non_zero_symbol = 0;
  for(unsigned long i = 0; i < (unsigned long)number_of_parity_symbols; i++) {
    if(parity[i]) {
      has_non_zero_symbol = 1;
      break;
    }
  }
  if(!has_non_zero_symbol) {
    return -1;
  }

  #if defined(_OPENMP)
    #pragma omp critical
  #endif
  free_rs(rs);
  return res;
}

// give data, data_len, parity and parity_len, data array is changed to correct errors
// if errors can't be corrected, -1 is returned, otherwise number of errors is returned (can be 0)
int rs_decode8(uint8_t* data, int data_len, uint8_t* parity, int number_of_parity_symbols) {

	struct rs_control* rs = get_rs_struct(8, number_of_parity_symbols);

  uint16_t parity16[number_of_parity_symbols];
  for(int i = 0; i < number_of_parity_symbols; i++) parity16[i] = parity[i];

  int numerr;
  #if defined(_OPENMP)
    #pragma omp critical
  #endif

  // if a bit is removed, the parity may be zero
  uint8_t has_non_zero_symbol = 0;
  for(unsigned long i = 0; i < (unsigned long)number_of_parity_symbols; i++) {
    if(parity[i]) {
      has_non_zero_symbol = 1;
      break;
    }
  }
  if(!has_non_zero_symbol) {
    parity[number_of_parity_symbols - 1] = 0x01;
  }

	numerr = decode_rs8(rs, data, parity16, data_len, NULL, 0, NULL, XOR_MASK, NULL);
  #if defined(_OPENMP)
    #pragma omp critical
  #endif
	free_rs(rs);

	return numerr == -74 ? -1 : numerr;
}

int rs_decode(uint8_t* data, int data_len, uint16_t* parity, int number_of_parity_symbols, int symsize) {

	struct rs_control* rs = get_rs_struct(symsize, number_of_parity_symbols);

  int numerr;
  #if defined(_OPENMP)
    #pragma omp critical
  #endif
  // if a bit is removed, the parity may be zero
  uint8_t has_non_zero_symbol = 0;
  for(unsigned long i = 0; i < (unsigned long)number_of_parity_symbols; i++) {
    if(parity[i]) {
      has_non_zero_symbol = 1;
      break;
    }
  }
  if(!has_non_zero_symbol) {
    parity[number_of_parity_symbols - 1] = 0x01;
  }
	numerr = decode_rs8(rs, data, parity, data_len, NULL, 0, NULL, XOR_MASK, NULL);

  #if defined(_OPENMP)
    #pragma omp critical
  #endif
	free_rs(rs);

	return numerr == -74 ? -1 : numerr;
}

void* append_rs_code8(void* data, unsigned long* data_len, unsigned long num_parity_symbols) {

    uint8_t parity[num_parity_symbols];
    rs_encode8(data, *data_len, parity, num_parity_symbols);

    unsigned long new_len = (*data_len) + num_parity_symbols;
    uint8_t* data_with_parity = malloc(new_len);
    memcpy(data_with_parity, data, *data_len);
    memcpy(data_with_parity+(*data_len), parity, num_parity_symbols);
    *data_len = new_len;
    return data_with_parity;
}

void copy_unmerged_arr_to_merged_arr(void* from, void* to, unsigned long num_symbols, unsigned long symbol_size, unsigned long num_from_element_bytes, unsigned long* next_bit) {
  unsigned long offset = num_from_element_bytes * 8 - symbol_size;
  for(unsigned long i = 0; i < num_symbols; i++) {
    void* from_element = ((uint8_t*)from) + ( num_from_element_bytes * i );
    for(unsigned long j = 0; j < symbol_size; j++) {
      set_bit(to, *next_bit, get_bit(from_element, offset + j));
      (*next_bit)++;
    }
  }
}

void copy_merged_arr_to_unmerged_arr(void* from, void* to, unsigned long num_symbols, unsigned long symbol_size, unsigned long num_to_element_bytes, unsigned long* next_bit) {

    unsigned long offset = num_to_element_bytes * 8 - symbol_size;
    for(unsigned long i = 0; i < num_symbols; i++) {
      void* to_element = ((uint8_t*)to) + ( num_to_element_bytes * i );
      for(unsigned long j = 0; j < symbol_size; j++) {
        set_bit(to_element, offset + j, get_bit(from, *next_bit));
        (*next_bit)++;
      }
    }
}

void* append_rs_code(void* data, unsigned long* num_data_symbols, unsigned long num_parity_symbols, unsigned long symsize) {

  // 0. initialize variables
  unsigned long data_bits = (*num_data_symbols) * symsize;
  unsigned long parity_bits = num_parity_symbols * symsize;
  unsigned long total_bits = data_bits + parity_bits;
  unsigned long total_bytes = total_bits / 8 + !!(total_bits % 8);
  unsigned long symbol_bytes = symsize / 8 + !!(symsize % 8);

  uint16_t parity[num_parity_symbols];
  memset(parity, 0x00, sizeof(uint16_t) * num_parity_symbols);
  uint8_t* res = NULL;
  // 1. unmerge data symbols and encode
  unmerge_arr(data, *num_data_symbols, symbol_bytes, symsize, (void**)&res);
  int encode_result = rs_encode(res, *num_data_symbols, parity, num_parity_symbols, symsize);
  if(encode_result == -1) {
    return NULL;
  }
  // 2. convert parity elements to big endian if necessary, to put LSBs on the right
  if( is_little_endian_machine() ) {
    for(unsigned long i = 0; i < num_parity_symbols; i++) {
      uint8_t* parity_element = (uint8_t*)&parity[i];
      uint8_t tmp = parity_element[0];
      parity_element[0] = parity_element[1];
      parity_element[1] = tmp;
    }
  }
  // 3. alloc memory for data + parity sequence
  uint8_t* data_with_parity = malloc(total_bytes);
  memset(data_with_parity, 0x00, total_bytes);
  // 4. copy data bits
  unsigned long next_bit = 0;
  copy_unmerged_arr_to_merged_arr(res, data_with_parity, *num_data_symbols, symsize, symbol_bytes, &next_bit);
  // 5. copy parity bits
  copy_unmerged_arr_to_merged_arr(parity, data_with_parity, num_parity_symbols, symsize, sizeof(uint16_t), &next_bit);
  // 6. set 'num_data_symbols' to total number of bits of the final sequence
  *num_data_symbols = total_bits;
  free(res);

  return data_with_parity; 
}

// 'num_parity_symbols' will hold the original data sequence size
uint8_t* remove_rs_code8(uint8_t* data, unsigned long data_len, unsigned long* num_parity_symbols) {

    unsigned long original_size = data_len - (*num_parity_symbols);

    if( rs_decode8(data, original_size, (uint8_t*)(data+original_size), *num_parity_symbols) != -1 ) {
        *num_parity_symbols = original_size;
        return realloc(data, original_size);
    } else {
        free(data);
        return NULL;
    }
}

uint8_t* remove_rs_code(uint8_t* data, unsigned long num_data_symbols, unsigned long num_parity_symbols, int symsize) {

    // 0. initialize variables
    unsigned long num_data_bits = num_data_symbols * symsize;
    unsigned long num_data_bytes = num_data_bits / 8 + !!(num_data_bits % 8);
    unsigned long symbol_bytes = symsize / 8 + !!(symsize % 8);
 
    // 1. unmerge data
    uint8_t* res = NULL;
    unmerge_arr(data, num_data_symbols, symbol_bytes, symsize, (void**)&res);

    // 2. unmerge parity, separating it from data
    uint16_t parity[num_parity_symbols];
    memset(parity, 0x00, sizeof(uint16_t) * (num_parity_symbols));
    unsigned long next_bit = num_data_bits;
    copy_merged_arr_to_unmerged_arr(data, parity, num_parity_symbols, symsize, sizeof(uint16_t), &next_bit);

    // 3. convert parity from big endian to little endian, if necessary
    if( is_little_endian_machine() ) {
      for(unsigned long i = 0; i < num_parity_symbols; i++) {
        uint8_t* parity_element = (uint8_t*)&parity[i];
        uint8_t tmp = parity_element[0];
        parity_element[0] = parity_element[1];
        parity_element[1] = tmp;
      }
    }
    int decode_status = rs_decode(res, num_data_symbols, parity, num_parity_symbols, symsize);
    if( decode_status != -1 ) {
      merge_arr(res, &num_data_symbols, sizeof(uint8_t), symsize); 
      return realloc(res, num_data_bytes);
    } else {
      free(res);
      return NULL;
    }
}
