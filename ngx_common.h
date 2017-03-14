
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_COMMON_H_INCLUDED_
#define _NGX_COMMON_H_INCLUDED_


//#include <ngx_config.h>
//#include <ngx_core.h>

//#if (NGX_THREADS)

#include <stdint.h>
//#include <sys/types.h>
//#include <sys/time.h>
#include <unistd.h>
//#include <stdarg.h>
//#include <stddef.h>             /* offsetof() */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <sys/syscall.h> 

#define  NGX_OK          0
#define  NGX_ERROR      -1
#define  NGX_AGAIN      -2
#define  NGX_BUSY       -3
#define  NGX_DONE       -4
#define  NGX_DECLINED   -5
#define  NGX_ABORT      -6

typedef intptr_t        ngx_int_t;
typedef uintptr_t       ngx_uint_t;
typedef int               ngx_err_t;

//have sched.h
#define NGX_HAVE_SCHED_YIELD 1

#if (NGX_HAVE_SCHED_YIELD)
#define ngx_sched_yield()  sched_yield()
#else
#define ngx_sched_yield()  usleep(1)
#endif


#define ngx_memzero(buf, n)       (void) memset(buf, 0, n)
#define ngx_memset(buf, c, n)     (void) memset(buf, c, n)

#define NGX_LINUX 1

#endif /* _NGX_COMMON_H_INCLUDED_ */
