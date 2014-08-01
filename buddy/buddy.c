#include <stdio.h>
#include <stdlib.h>

#define N          20
#define MEM_SIZE   1 << N

typedef struct header header_t;

struct header {
  int         used;
  size_t      order;
  header_t*   next;
  header_t*   previous;
};

char      memory[MEM_SIZE];
header_t* freelist[N+1];

/*
 * initializes memory allocator to a known state 
 */
static void init_mem() {
  size_t i;
  for (i=0; i<=N; ++i) {
    freelist[i] = NULL;
  }
  header_t *header = freelist[N] = (header_t*)memory;
  header->used = 0;
  header->order = N;
  header->next = NULL;
  header->previous = NULL;
}

/*
 * compute X that X is the min value that makes log2(X) >= n
 */
static size_t block_order(size_t n) {
  size_t order = 1;
  size_t acc = 1;
  while (n > acc) {
    acc = (acc << 1) | 1;
    order++;
  }
  return order;
}

/*
 * puts a block of memory pointed by `h` in the freelist with order `order`
 */
static void add_free_block(size_t order, header_t *h) {
  h->used = 0;
  h->order = order;
  h->next = freelist[order];
  h->previous = NULL;
  if (freelist[order]) {
    freelist[order]->previous = h;
  }
  freelist[order] = h;
}

/*
 * remove a block of memory pointed by `h` from the freelist with order 
 * `order`
 */
static void remove_free_block(size_t order, header_t *h) {
  if (h->previous) {
    h->previous->next = h->next;
  } else {
    freelist[order] = h->next;
  }
  if (h->next) {
    h->next->previous = h->previous;
  }
}

/*
 * split a block h until get a block of order `order`
 */
static header_t* split_free_block(header_t* h, size_t order) {
  remove_free_block(h->order, h);
  while (1) {
    if (h->order == order) {
      return h;
    } else {
      size_t neworder = h->order - 1;
      header_t *h2 = ((void*)h) + (1 << neworder);
      add_free_block(neworder, h2);
      h->order = neworder;
    }
  }
}

/*
 * take a free block of order `order`
 */
static void* get_free_block(size_t order) {
  header_t *h = freelist[order];
  if (h) {
    remove_free_block(order, h);
  } else {
    size_t i;
    for (i=order+1; !h && (i <= N); ++i) {
      if (freelist[i]) {
	h = split_free_block(freelist[i], order);
      }
    }
  }
  return h;
}

/*
 * allocs memory of size `n`
 */
void* my_alloc(size_t n) {
  if (n == 0) {
    return NULL;
  } else {
    size_t order = block_order(n + sizeof(header_t));
    if (order > N) {
      return NULL;
    } else {
      header_t *h = get_free_block(order);
      if (!h) {
	return NULL;
      }
      h->used = 1;
      h->order = order;
      return ((void*)h) + sizeof(header_t);
    }
  }
}

/*
 * release memory pointer by `p`
 */
void my_free(void *p) {
  header_t *h = p - sizeof(header_t);
  size_t order = h->order;  
  while (1) {
    size_t pos = (size_t)h - (size_t)memory;
    size_t buddypos = pos ^ (1 << order);
    header_t *buddy = (header_t*)((void*)memory + buddypos);
    if (buddy->used || (buddy->order != order)) {
      add_free_block(order, h);
      return;
    } else {
      if (h > buddy) {
	void *temp = h;
	h = buddy;
	buddy = temp;
      }
      remove_free_block(order, buddy);
      order++;
    }
  }
}

int main() {
  init_mem();
  int *p = my_alloc(sizeof(int));
  *p = 10;
  printf("%d\n", *p);
  my_free(p);
  return EXIT_SUCCESS;
}
