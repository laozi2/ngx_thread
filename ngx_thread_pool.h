
#ifndef _NGX_THREAD_POOL_H_INCLUDED_
#define _NGX_THREAD_POOL_H_INCLUDED_

#include "ngx_common.h"

typedef struct ngx_thread_task_s  ngx_thread_task_t;

struct ngx_thread_task_s {
    ngx_thread_task_t   *next; //no need set
    ngx_uint_t           id; //task id, no need set
    void                *ctx; //save ctx for handler, user set
    void               (*handler)(void *data); //user set
};


typedef struct ngx_thread_pool_s  ngx_thread_pool_t;


ngx_thread_pool_t* ngx_thread_pool_config(ngx_uint_t threads);

//ngx_thread_task_t *ngx_thread_task_alloc(size_t size);
ngx_int_t ngx_thread_task_post(ngx_thread_pool_t *tp, ngx_thread_task_t *task);

ngx_int_t ngx_thread_pool_init_worker(ngx_thread_pool_t* tp);
void ngx_thread_pool_exit_worker(ngx_thread_pool_t* tp);


#endif /* _NGX_THREAD_POOL_H_INCLUDED_ */
