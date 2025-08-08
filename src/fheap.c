#include <stdint.h>
#include <inttypes.h>
#include <math.h>

#define NOB_STRIP_PREFIX
#include "nob.h"

#include "fheap.h"

void fheap_init(fheap *h)
{
  h->min = h->root = NULL;
  h->n = 0;
  return;
}

void fheap_node_tree_clear(fheap_node *root)
{
  if (root == NULL)
    return;

  if (root->child == NULL)
    // leaf node
    return;
  fheap_node_tree_clear(root->child);

  fheap_node *node = root->next;
  while (node != root)
  {
    fheap_node_tree_clear(node);
    node = node->next;
  }

  free(root);
  return;
}

void fheap_clear(fheap h)
{
  fheap_node_tree_clear(h.root);
  return;
}

fheap_node find_min(fheap h)
{
  return *h.min;
}

void fheap_merge(fheap *rop, fheap op)
{
  if (op.root == NULL)
    // nothing to be done
    return;

  if (rop->root == NULL)
  {
    rop->root = op.root;
    rop->min = op.min;
    rop->n = op.n;
    return;
  }

  /*
   * rop->root         op.root
   *     ^               ^
   *     # -># -># ->#   # -># ->#
   *     |----<------|   |-<-----|
   *
   * |> op.root->prev->next = rop->root;
   * rop->root         op.root
   *     ^               ^
   *     # -># -># ->#   # -># ->#
   *     |----<------|   |- - - -|
   *     |- - - - - - -<- - - - -|
   *
   * |> rop->root->prev->next = op.root
   * rop->root         op.root
   *     ^               ^
   *     # -># -># ->#  ># -># ->#
   *     |- - - - - -|   |- - - -|
   *     |- - - - - - -<- - - - -|
   *
   * |> temp = op.root->prev
   * |> op.root->prev = rop->root->prev
   * rop->root         op.root   temp
   *     ^               ^       ^
   *     # -># -># -># -># -># ->#
   *     |- - - - - -|
   *     |- - - - - - -<- - - - -|
   *
   * |> rop->root->prev = temp
   * rop->root         op.root
   *     ^               ^
   *     # -># -># -># -># -># ->#
   *     |-------------<---------|
   */

  op.root->prev->next = rop->root;
  rop->root->prev->next = op.root;
  fheap_node *temp = op.root->prev;
  op.root->prev = rop->root->prev;
  rop->root->prev = temp;

  if (op.min->key < rop->min->key)
    rop->min = op.min;

  rop->n = rop->n + op.n;

  return;
}

void fheap_insert(fheap *rop, double key, uint64_t val)
{
  fheap_node *node = malloc(sizeof(fheap_node));
  node->child = NULL;
  node->prev = node;
  node->next = node;
  node->deg = 0;
  node->key = key;
  node->val = val;
  fheap op = {.root = node, .min = node, .n = 1};
  fheap_merge(rop, op);

  return;
}

uint64_t root_count(fheap *h)
{
  uint64_t count = 0;
  fheap_node *node = h->root->next;
  while ((node = node->next) != h->root)
    ++count;

  return count;
}

key_val_pair extract_min(fheap *h)
{
  // print_fheap(h);

  if (h->min == NULL)
  {
    nob_log(NOB_ERROR, "extract min from empty heap!!\n");
    exit(1);
  }

  const fheap_node min = *h->min;

  /*
   * step 1:
   *   delete the root and make its child roots
   * step 2:
   *   consolidate the heap by
   */

  if (h->n == 1)
  {
    h->min = h->root = NULL;
    h->n = 0;
    return (key_val_pair){.key = min.key, .val = min.val};
  }

  if (h->min->child == NULL)
  {
    h->min->prev->next = h->min->next;
    h->min->next->prev = h->min->prev;

    // h->min->next != h->min because h->n > 1 ie there is another node, ie no element of the roots are linked to themselves
    h->min = h->min->next;
  }
  else
  {
    if (h->min->next != h->min)
    {
      fheap_node *const end_of_child_chain = h->min->child->prev;
      end_of_child_chain->next = h->min->next;
      h->min->next->prev = end_of_child_chain;
      h->min->prev->next = h->min->child;
      h->min->child->prev = h->min->prev;
    }

    h->min = h->min->child;
  }
  // ensure root is still pointing to a valid position
  h->root = h->min;

  --h->n;

  // consolidate the heap

  // array of pointers
  size_t len_A = log2(h->n) * 2 + 1;
  fheap_node **A = calloc(len_A, sizeof(fheap_node *));
  A[h->root->deg] = h->root;

  /*
   * in order to not iterate over the changing doubly-linked list of the roots
   * which introduces difficulties, we list them before and iterate over those
   * roots is an array of pointers to fheap_nodes pointing to the roots
   */

  const uint64_t r_cnt = root_count(h);
  fheap_node **roots = calloc(r_cnt, sizeof(fheap_node *));
  roots[0] = h->root;

  uint64_t i = 0;
  for (fheap_node *node = h->root->next; (node = node->next) != h->root; ++i)
    roots[i] = node;

  for (i = 0; i < r_cnt; ++i)
  {
    fheap_node *node = roots[i];
    uint32_t d = node->deg;

    /*
     * while we find a node with the same number of children as us:
     *   add the one with the highest key as a child of the other
     *   check again now that our degree is higher
     */
    fheap_node *y;
    while ((y = A[d]) != NULL)
    {
      // ensure node->key <= y->key
      if (node->key > y->key)
      {
        fheap_node *t = y;
        y = node;
        node = t;
      }

      if (y == h->root)
        h->root = node;

      // remove y from roots
      y->prev->next = y->next;
      y->next->prev = y->prev;

      // add y as a children of node
      if (node->child == NULL)
      {
        y->prev = y;
        y->next = y;
        node->child = y;
      }
      else
      {
        y->prev = node->child->prev;
        node->child->prev->next = y;
        node->child->prev = y;
        y->next = node->child;
      }

      ++node->deg;

      A[d] = NULL;
      ++d;
      if (d >= len_A)
        break;
    }
    A[d] = node;

    node = node->next;
  }

  free(roots);
  free(A);

  // find the min amongst the roots
  fheap_node *node = h->root;
  do
  {
    if (node->key < h->min->key)
      h->min = node;
  } while ((node = node->next) != h->root);

  return (key_val_pair){.key = min.key, .val = min.val};
}

void print_cycle(fheap_node *root)
{
  printf("(%lf, %"PRIu64")", root->key, root->val);
  fheap_node *node = root->next;

  while (node != root)
  {
    printf(" -> (%lf, %"PRIu64")", node->key, node->val);
    node = node->next;
  }

  putchar('\n');

  return;
}

void print_fheap_inside(const fheap_node *const root)
{
  if (root == NULL)
    return;

  printf("(%lf, %"PRIu64")", root->key, root->val);
  if (root->child != NULL)
  {
    putchar('[');
    print_fheap_inside(root->child);
    putchar(']');
  }

  fheap_node *node = root->next;

  while (node != root)
  {
    printf(" -> (%lf, %"PRIu64")", node->key, node->val);

    if (node->child != NULL)
    {
      putchar('[');
      print_fheap_inside(node->child);
      putchar(']');
    }

    node = node->next;
  }

  return;
}

void print_fheap(const fheap *const h)
{
  print_fheap_inside(h->root);
  putchar('\n');
  return;
}
