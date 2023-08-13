#include <liballoc.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

// This can be done with pthread mutexes and probably should but...
static volatile char liballoc_locked = 0;

int liballoc_lock() {
  while (!__sync_bool_compare_and_swap(&liballoc_locked, 0, 1))
    ;
  return 0;
}

int liballoc_unlock() {
  __sync_bool_compare_and_swap(&liballoc_locked, 1, 0);
  return 0;
}

void *liballoc_alloc_pages(size_t pages) {
  void *addr = mmap(NULL, pages * getpagesize(), PROT_READ | PROT_WRITE,
                    MAP_ANON | MAP_PRIVATE, -1, 0);
  if (addr == MAP_FAILED)
    return NULL;
  return addr;
}

int liballoc_free_pages(void *ptr, size_t pages) {
  return munmap(ptr, pages * getpagesize());
}

size_t liballoc_get_page_size() { return getpagesize(); }

void *liballoc_memset(void *s, int c, size_t n) { return memset(s, c, n); }

void *liballoc_memcpy(void *dest, void *src, size_t n) {
  return memcpy(dest, src, n);
}
