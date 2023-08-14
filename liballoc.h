#ifndef __LIBALLOC_H__
#define __LIBALLOC_H__

#include <stddef.h>
#include <stdint.h>

#define LIBALLOC_PREFIX(f) f

#define LIBALLOC_DEFAULT_ALIGNMENT 64

#define LIBALLOC_PAGE_COUNT 64

#define LIBALLOC_MAGIC 0xcafebabe
#define LIBALLOC_DEAD 0xdeadbeef

struct liballoc_minor;

typedef struct liballoc_major {
  struct liballoc_minor *first;
  struct liballoc_major *prev;
  struct liballoc_major *next;
  size_t pages;
  size_t size;
  size_t usage;
} liballoc_major_t;

typedef struct liballoc_minor {
  uint32_t magic;
  struct liballoc_minor *prev;
  struct liballoc_minor *next;
  liballoc_major_t *block;
  size_t size;
  size_t req_size;
} liballoc_minor_t;

int liballoc_lock();
int liballoc_unlock();
void *liballoc_alloc_pages(size_t pages);
int liballoc_free_pages(void *ptr, size_t pages);
size_t liballoc_get_page_size();
void *liballoc_memset(void *s, int c, size_t n);
void *liballoc_memcpy(void *dest, void *src, size_t n);

void *LIBALLOC_PREFIX(aligned_alloc)(size_t align, size_t req_size);
void *LIBALLOC_PREFIX(malloc)(size_t size);
void *LIBALLOC_PREFIX(calloc)(size_t nobj, size_t size);
void *LIBALLOC_PREFIX(realloc)(void *p, size_t size);
void LIBALLOC_PREFIX(free)(void *ptr);

#endif
