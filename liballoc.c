#include <liballoc.h>
#include <stddef.h>
#include <stdint.h>

static liballoc_major_t *liballoc_mem_root = NULL;
static liballoc_major_t *liballoc_best_bet = NULL;

static inline void *liballoc_align(void *ptr, size_t align) {
  size_t diff = align - (size_t)(ptr + sizeof(size_t)) % align;
  ptr = (void *)((uintptr_t)ptr + diff + sizeof(size_t));
  *((size_t *)((uintptr_t)ptr - sizeof(size_t))) = diff + sizeof(size_t);
  return ptr;
}

static inline void *liballoc_unalign(void *ptr) {
  return (void *)((uintptr_t)ptr -
                  *((size_t *)((uintptr_t)ptr - sizeof(size_t))));
}

static inline liballoc_major_t *liballoc_allocate_new_page(size_t size) {
  size_t page_size = liballoc_get_page_size();

  size += sizeof(liballoc_major_t) + sizeof(liballoc_minor_t);

  if (!(size % page_size))
    size = size / page_size;
  else
    size = size / page_size + 1;

  if (size < LIBALLOC_PAGE_COUNT)
    size = LIBALLOC_PAGE_COUNT;

  liballoc_major_t *maj = liballoc_alloc_pages(size);
  if (!maj)
    return NULL;

  *maj = (liballoc_major_t){
      .first = NULL,
      .prev = NULL,
      .next = NULL,
      .pages = size,
      .size = size * page_size,
      .usage = sizeof(liballoc_major_t),
  };

  return maj;
}

void *LIBALLOC_PREFIX(aligned_malloc)(size_t req_size, size_t align) {
  liballoc_major_t *maj = liballoc_mem_root;
  size_t size = req_size + align + sizeof(size_t);
  size_t best_size = 0;
  int started_bet = 0;

  if (!align || !size || !req_size)
    return NULL;

  liballoc_lock();

  if (!liballoc_mem_root) {
    liballoc_mem_root = liballoc_allocate_new_page(size);
    maj = liballoc_mem_root;
    if (!liballoc_mem_root) {
      liballoc_unlock();
      return NULL;
    }
  }

  if (liballoc_best_bet) {
    best_size = liballoc_best_bet->size - liballoc_best_bet->usage;
    if (best_size > size + sizeof(liballoc_minor_t)) {
      maj = liballoc_best_bet;
      started_bet = 1;
    }
  }

  while (maj) {
    uintptr_t diff = maj->size - maj->usage;

    if (best_size < diff) {
      liballoc_best_bet = maj;
      best_size = diff;
    }

    if (diff < size + sizeof(liballoc_minor_t)) {
      if (maj->next) {
        maj = maj->next;
        continue;
      }

      if (started_bet) {
        maj = liballoc_mem_root;
        started_bet = 0;
        continue;
      }

      maj->next = liballoc_allocate_new_page(size);
      if (!maj->next)
        break;

      maj->next->prev = maj;
      maj = maj->next;
    }

    if (!maj->first) {
      maj->first = (void *)((uintptr_t)maj + sizeof(liballoc_major_t));
      *maj->first = (liballoc_minor_t){
          .magic = LIBALLOC_MAGIC,
          .prev = NULL,
          .next = NULL,
          .block = maj,
          .size = size,
          .req_size = req_size,
      };

      maj->usage += size + sizeof(liballoc_minor_t);

      void *p = liballoc_align(
          (void *)((uintptr_t)(maj->first) + sizeof(liballoc_minor_t)), align);
      liballoc_unlock();
      return p;
    }

    if ((uintptr_t)maj->first - (uintptr_t)maj - sizeof(liballoc_major_t) >=
        size + sizeof(liballoc_minor_t)) {
      maj->first = maj->first->prev;
      *maj->first = (liballoc_minor_t){
          .magic = LIBALLOC_MAGIC,
          .prev = NULL,
          .next = (void *)((uintptr_t)maj + sizeof(liballoc_major_t)),
          .block = maj,
          .size = size,
          .req_size = req_size,
      };

      maj->first->prev->next = maj->first;

      maj->usage += size + sizeof(liballoc_minor_t);

      void *p = liballoc_align(
          (void *)((uintptr_t)(maj->first) + sizeof(liballoc_minor_t)), align);
      liballoc_unlock();
      return p;
    }

    liballoc_minor_t *min = maj->first;
    while (min) {
      if (!min->next && (uintptr_t)maj - (uintptr_t)min - maj->size -
                                min->size - sizeof(liballoc_minor_t) >=
                            size + sizeof(liballoc_minor_t)) {
        min->next =
            (void *)((uintptr_t)min + sizeof(liballoc_minor_t) + min->size);
        min = min->next;
        *min = (liballoc_minor_t){
            .magic = LIBALLOC_MAGIC,
            .prev = min,
            .next = NULL,
            .block = maj,
            .size = size,
            .req_size = req_size,
        };

        maj->usage += size + sizeof(liballoc_minor_t);

        void *p = liballoc_align(
            (void *)((uintptr_t)min + sizeof(liballoc_minor_t)), align);
        liballoc_unlock();
        return p;
      } else if (min->next && (uintptr_t)min->next - (uintptr_t)min -
                                      min->size - sizeof(liballoc_minor_t) >=
                                  size + sizeof(liballoc_minor_t)) {
        liballoc_minor_t *new_min =
            (void *)((uintptr_t)min + sizeof(liballoc_minor_t) + min->size);
        *new_min = (liballoc_minor_t){
            .magic = LIBALLOC_MAGIC,
            .prev = min,
            .next = min->next,
            .block = maj,
            .size = size,
            .req_size = req_size,
        };

        min->next->prev = new_min;
        min->next = new_min;
        maj->usage += size + sizeof(liballoc_minor_t);

        void *p = liballoc_align(
            (void *)((uintptr_t)new_min + sizeof(liballoc_minor_t)), align);
        liballoc_unlock();
        return p;
      }

      min = min->next;
    }

    if (!maj->next) {
      if (started_bet) {
        maj = liballoc_mem_root;
        started_bet = 0;
        continue;
      }

      maj->next = liballoc_allocate_new_page(size);
      if (!maj->next)
        break;
      maj->next->prev = maj;
    }

    maj = maj->next;
  }

  liballoc_unlock();

  return NULL;
}

void *LIBALLOC_PREFIX(malloc)(size_t size) {
  return LIBALLOC_PREFIX(aligned_malloc)(size, LIBALLOC_DEFAULT_ALIGNMENT);
}

void *LIBALLOC_PREFIX(calloc)(size_t nobj, size_t size) {
  if (!nobj || !size)
    return NULL;

  size_t real_size = nobj * size;

  void *p = LIBALLOC_PREFIX(malloc)(real_size);

  liballoc_memset(p, 0, real_size);

  return p;
}

void *LIBALLOC_PREFIX(realloc)(void *p, size_t size) {
  if (!p)
    return LIBALLOC_PREFIX(malloc)(size);
  if (!size) {
    LIBALLOC_PREFIX(free)(p);
    return NULL;
  }

  liballoc_lock();

  void *ptr = liballoc_unalign(p);
  liballoc_minor_t *min = (void *)((uintptr_t)ptr - sizeof(liballoc_minor_t));

  if (min->magic != LIBALLOC_MAGIC) {
    liballoc_unlock();
    return NULL;
  }

  if (min->req_size >= size) {
    min->req_size = size;
    liballoc_unlock();
    return p;
  }

  liballoc_unlock();

  ptr = LIBALLOC_PREFIX(malloc)(size);
  liballoc_memcpy(ptr, p, min->req_size);
  LIBALLOC_PREFIX(free)(p);

  return ptr;
}

void LIBALLOC_PREFIX(free)(void *ptr) {
  if (!ptr)
    return;

  ptr = liballoc_unalign(ptr);

  liballoc_lock();

  liballoc_minor_t *min = (void *)((uintptr_t)ptr - sizeof(liballoc_minor_t));

  if (min->magic != LIBALLOC_MAGIC) {
    liballoc_unlock();
    return;
  }
  min->magic = LIBALLOC_DEAD;

  liballoc_major_t *maj = min->block;
  maj->usage -= min->size + sizeof(liballoc_minor_t);

  if (min->next)
    min->next->prev = min->prev;
  if (min->prev)
    min->prev->next = min->next;
  if (!min->prev)
    maj->first = min->next;

  if (!maj->first) {
    if (liballoc_mem_root == maj)
      liballoc_mem_root = maj->next;
    if (liballoc_best_bet == maj)
      liballoc_best_bet = NULL;
    if (maj->prev)
      maj->prev->next = maj->next;
    if (maj->next)
      maj->next->prev = maj->prev;

    liballoc_free_pages(maj, maj->pages);
  } else if (liballoc_best_bet &&
             maj->size - maj->usage >
                 liballoc_best_bet->size - liballoc_best_bet->usage)
    liballoc_best_bet = maj;

  liballoc_unlock();
}
