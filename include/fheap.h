#ifndef __FHEAP__
#define __FHEAP__

#include <stdint.h>

// 48 bytes
typedef struct fheap_node_s
{
  double key;
  uint64_t val;
  struct fheap_node_s *prev, *next, *child;
  // as we do not implement decrease-key operation mark is not needed
  // uint8_t mark;
  uint32_t deg;
} fheap_node;

typedef struct fheap_s
{
  fheap_node *root, *min;
  uint64_t n;
} fheap;

typedef struct key_val_pair_s
{
  double key;
  uint64_t val;
} key_val_pair;

void fheap_init(fheap *h);
void fheap_clear(fheap h);
fheap_node find_min(fheap h);
// merges op into rop, op is set to rop
void fheap_merge(fheap *rop, fheap op);
void fheap_insert(fheap *rop, double key, uint64_t val);
key_val_pair extract_min(fheap *h);

void print_cycle(fheap_node *root);
void print_fheap(const fheap *const h);

#endif // __FHEAP__