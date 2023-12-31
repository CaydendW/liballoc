liballoc - A small memory allocator
===================================

This is liballoc, at least, a modified version done by me. This is a fork of [Blanham's repo of liballoc](https://github.com/blanham/liballoc/) with modifications to the C code that I prefer and the ability to do arbitrary alignment. It is an easy to port memory allocator for hobby operating systems and C standard libraries.

Note that this software is not well tested (It has passed every test I have thrown at it) and it maybe have bugs and problems with it. There is no guarantee that this software is secure or safe. Use at your own risk and don't blame me when bugs arise. Submit a pull request instead :)

Using liballoc
==============

There are 7 functions which you need to implement on your system:

```c
int liballoc_lock();
int liballoc_unlock();
void *liballoc_alloc_pages(size_t pages);
int liballoc_free_pages(void *ptr, size_t pages);
size_t liballoc_get_page_size();
void *liballoc_memset(void *s, int c, size_t n);
void *liballoc_memcpy(void *dest, void *src, size_t n);
```

The functions mean the following:
+ liballoc\_lock locks the memory data structures. Normally done with a spinlock or by disabling interrupts. This is up to the programmers digression. Return 0 if the lock is successfully acquired. Anything else is a failure.
+ liballoc\_unlock unlocks/undoes what liballoc\_lock did. Return 0 if the lock was successfully released.
+ liballoc\_alloc\_pages accepts an integer parameter of how many pages to allocate. Return NULL if the memory could not be allocated or a pointer to the memory if the memory could be allocated
+ liballoc\_free\_pages frees previously allocated pages. Accepts a pointer to the memory to be freed and an integer parameter of how many pages are to be freed. Return 0 if the memory was successfully freed and any other number otherwise
+ liballoc\_get\_page\_size returns the page size of the system.
+ liballoc\_memset is a callback to some sort of memset. See `man 3 memset`
+ liballoc\_memcpy is a callback to some sort of memcpy. See `man 3 memcpy`

Have a look at liballoc\_unix.c for an example of how to implement the library on unix like systems. 

NOTE: There are two ways to build the library:

1. Compile the library with a new system file. For example, I've left unix.c with the default distribution. It gets compiled directly into the liballoc_unix.so file.
2. Implement the functions in your application and then just link against the default liballoc.so library when you compile your app.

Liballoc has these 3 C macros that can be changed:
```
#define LIBALLOC_PREFIX(f)
#define LIBALLOC_DEFAULT_ALIGNMENT
#define LIBALLOC_PAGE_COUNT
```

These macros mean the following:
+ LIBALLOC\_PREFIX is the prefix to insert before the name of the function call. Change by modifying the macro.
+ LIBALLOC\_DEFAULT\_ALIGNMENT is the default alignment for malloc, calloc and realloc.
+ LIBALLOC\_PAGE\_COUNT is the number of pages to allocate at a time

Liballoc provides the following 5 functions:
```c
void *LIBALLOC_PREFIX(aligned_alloc)(size_t align, size_t req_size);
void *LIBALLOC_PREFIX(malloc)(size_t size);
void *LIBALLOC_PREFIX(calloc)(size_t nobj, size_t size);
void *LIBALLOC_PREFIX(realloc)(void *p, size_t size);
void LIBALLOC_PREFIX(free)(void *ptr);
```

They all correspond to their equivalent ANSI C and/or POSIX counterparts. aligned\_malloc will allocate memory similar to `man 3 aligned_alloc` except that the alignment may be arbitrary (Not just a power of 2).

Quick Start
===========

You can simply type: "make unix" to build the unix shared library.  Thereafter, you can link it directly into your applications during build or afterwards by export the LD\_PRELOAD environment variable. 

To run bash with the library, for example:
```bash
LD_PRELOAD=/full/path/to/liballoc.so bash
```

The above command will pre-link the library into the application, essentially replacing the default malloc/free calls at runtime. It's quite cool.

License
=======
This work (Like the liballoc before it) is released into the public domain. It is licensed under the unlicense. If this is for whatever reason not practical for your project or needs, it may also be licensed under the MIT license (With or without attribution) or the Do What The F\*ck You Want To Public License.

Credits
=======

Originally by:
Durand Miller

Modifications by:
CaydendW

Original repository by Blanham: [https://github.com/blanham/liballoc/](https://github.com/blanham/liballoc/)
