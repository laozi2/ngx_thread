
#ifndef _NGX_THREAD_H_INCLUDED_
#define _NGX_THREAD_H_INCLUDED_

//#if (NGX_THREADS)

#include "ngx_common.h"

typedef pthread_mutex_t  ngx_thread_mutex_t;

ngx_int_t ngx_thread_mutex_create(ngx_thread_mutex_t *mtx);
ngx_int_t ngx_thread_mutex_destroy(ngx_thread_mutex_t *mtx);
ngx_int_t ngx_thread_mutex_lock(ngx_thread_mutex_t *mtx);
ngx_int_t ngx_thread_mutex_unlock(ngx_thread_mutex_t *mtx);


typedef pthread_cond_t  ngx_thread_cond_t;

ngx_int_t ngx_thread_cond_create(ngx_thread_cond_t *cond);
ngx_int_t ngx_thread_cond_destroy(ngx_thread_cond_t *cond);
ngx_int_t ngx_thread_cond_signal(ngx_thread_cond_t *cond);
ngx_int_t ngx_thread_cond_wait(ngx_thread_cond_t *cond, ngx_thread_mutex_t *mtx);


typedef pid_t      ngx_tid_t;
#define NGX_TID_T_FMT         "%P"
ngx_tid_t ngx_thread_tid(void);
#define ngx_log_tid           ngx_thread_tid()

extern ngx_int_t    ngx_ncpu;

#endif /* _NGX_THREAD_H_INCLUDED_ */
