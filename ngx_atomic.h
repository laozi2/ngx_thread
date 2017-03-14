
#ifndef _NGX_ATOMIC_H_INCLUDED_
#define _NGX_ATOMIC_H_INCLUDED_

#include "ngx_common.h"

//TODO
#define NGX_HAVE_GCC_ATOMIC 1 

#if (NGX_HAVE_GCC_ATOMIC)

/* GCC 4.1 builtin atomic operations */

#define NGX_HAVE_ATOMIC_OPS  1

typedef long                        ngx_atomic_int_t;
typedef unsigned long               ngx_atomic_uint_t;

#if (NGX_PTR_SIZE == 8)
#define NGX_ATOMIC_T_LEN            (sizeof("-9223372036854775808") - 1)
#else
#define NGX_ATOMIC_T_LEN            (sizeof("-2147483648") - 1)
#endif

typedef volatile ngx_atomic_uint_t  ngx_atomic_t;


#define ngx_atomic_cmp_set(lock, old, set)                                    \
    __sync_bool_compare_and_swap(lock, old, set)

#define ngx_atomic_fetch_add(value, add)                                      \
    __sync_fetch_and_add(value, add)

#define ngx_memory_barrier()        __sync_synchronize()

#if ( __i386__ || __i386 || __amd64__ || __amd64 )
#define ngx_cpu_pause()             __asm__ ("pause")
#else
#define ngx_cpu_pause()
#endif

#endif

void ngx_spinlock(ngx_atomic_t *lock, ngx_atomic_int_t value, ngx_uint_t spin);

#define ngx_trylock(lock)  (*(lock) == 0 && ngx_atomic_cmp_set(lock, 0, 1))
#define ngx_unlock(lock)    *(lock) = 0


#endif /* _NGX_ATOMIC_H_INCLUDED_ */
