
#include "ngx_atomic.h"
#include "ngx_thread.h"
#include "ngx_thread_pool.h"
#include "flog.h"


typedef struct {
    ngx_thread_task_t        *first;
    ngx_thread_task_t       **last;
} ngx_thread_pool_queue_t;

#define ngx_thread_pool_queue_init(q)                                         \
    (q)->first = NULL;                                                        \
    (q)->last = &(q)->first


struct ngx_thread_pool_s {
    ngx_thread_mutex_t        mtx;
    ngx_thread_pool_queue_t   queue;
    ngx_int_t                 waiting;
    ngx_thread_cond_t         cond;

    //ngx_log_t                *log;

    //ngx_str_t                 name;
    ngx_uint_t                threads;
    ngx_int_t                 max_queue;
};


static ngx_int_t ngx_thread_pool_init(ngx_thread_pool_t *tp);
static void ngx_thread_pool_destroy(ngx_thread_pool_t *tp);
static void ngx_thread_pool_exit_handler(void *data);

static void *ngx_thread_pool_cycle(void *data);
static void ngx_thread_pool_handler();

static ngx_uint_t               ngx_thread_pool_task_id;
static ngx_atomic_t             ngx_thread_pool_done_lock;
//static ngx_thread_pool_queue_t  ngx_thread_pool_done;

static ngx_thread_pool_t g_tp; //me

ngx_int_t   ngx_ncpu = 1;

static ngx_int_t
ngx_thread_pool_init(ngx_thread_pool_t *tp)
{
    int             err;
    pthread_t       tid;
    ngx_uint_t      n;
    pthread_attr_t  attr;

    //if (ngx_notify == NULL) {
    //    ngx_log_error(NGX_LOG_ALERT, log, 0,
    //           "the configured event method cannot be used with thread pools");
    //    return NGX_ERROR;
    //}

    ngx_thread_pool_queue_init(&tp->queue);

    if (ngx_thread_mutex_create(&tp->mtx) != NGX_OK) {
        return NGX_ERROR;
    }

    if (ngx_thread_cond_create(&tp->cond) != NGX_OK) {
        (void) ngx_thread_mutex_destroy(&tp->mtx);
        return NGX_ERROR;
    }

    //tp->log = log;

    err = pthread_attr_init(&attr);
    if (err) {
        //ngx_log_error(NGX_LOG_ALERT, log, err,
        //              "pthread_attr_init() failed");
		LOG_ERROR("pthread_attr_init() failed");
        return NGX_ERROR;
    }

#if 0
    err = pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN);
    if (err) {
        //ngx_log_error(NGX_LOG_ALERT, log, err,
        //              "pthread_attr_setstacksize() failed");
		LOG_ERROR("pthread_attr_setstacksize() failed");
        return NGX_ERROR;
    }
#endif

    for (n = 0; n < tp->threads; n++) {
        err = pthread_create(&tid, &attr, ngx_thread_pool_cycle, tp);
        if (err) {
            //ngx_log_error(NGX_LOG_ALERT, log, err,
            //              "pthread_create() failed");
			LOG_ERROR("pthread_create() failed");
            return NGX_ERROR;
        }
    }

    (void) pthread_attr_destroy(&attr);

    return NGX_OK;
}


static void
ngx_thread_pool_destroy(ngx_thread_pool_t *tp)
{
    ngx_uint_t           n;
    ngx_thread_task_t    task;
    volatile ngx_uint_t  lock;

    ngx_memzero(&task, sizeof(ngx_thread_task_t));

    task.handler = ngx_thread_pool_exit_handler;
    task.ctx = (void *) &lock;

    for (n = 0; n < tp->threads; n++) {
        lock = 1;

        if (ngx_thread_task_post(tp, &task) != NGX_OK) {
            return;
        }

        while (lock) {
            ngx_sched_yield();
        }

        //task.event.active = 0;
    }

    (void) ngx_thread_cond_destroy(&tp->cond);

    (void) ngx_thread_mutex_destroy(&tp->mtx);
}


static void
ngx_thread_pool_exit_handler(void *data)
{
    ngx_uint_t *lock = data;

    *lock = 0;

    pthread_exit(0);
}


//ngx_thread_task_t *
//ngx_thread_task_alloc(size_t size)
//{
//    ngx_thread_task_t  *task;
//
//    //task = ngx_pcalloc(pool, sizeof(ngx_thread_task_t) + size);
//    task = malloc(sizeof(ngx_thread_task_t) + size);
//    if (task == NULL) {
//        return NULL;
//    }
//
//    task->ctx = task + 1;
//
//    return task;
//}


ngx_int_t
ngx_thread_task_post(ngx_thread_pool_t *tp, ngx_thread_task_t *task)
{
    //if (task->event.active) {
        //ngx_log_error(NGX_LOG_ALERT, tp->log, 0,
        //              "task #%ui already active", task->id);
        //return NGX_ERROR;
    //}

    if (ngx_thread_mutex_lock(&tp->mtx) != NGX_OK) {
        return NGX_ERROR;
    }

    if (tp->waiting >= tp->max_queue) {
        (void) ngx_thread_mutex_unlock(&tp->mtx);

        //ngx_log_error(NGX_LOG_ERR, tp->log, 0,
        //              "thread pool \"%V\" queue overflow: %i tasks waiting",
        //              &tp->name, tp->waiting);
        return NGX_ERROR;
    }

    //task->event.active = 1;

    task->id = ngx_thread_pool_task_id++;
    task->next = NULL;

    if (ngx_thread_cond_signal(&tp->cond) != NGX_OK) {
        (void) ngx_thread_mutex_unlock(&tp->mtx);
        return NGX_ERROR;
    }

    *tp->queue.last = task;
    tp->queue.last = &task->next;

    tp->waiting++;

    (void) ngx_thread_mutex_unlock(&tp->mtx);

    //ngx_log_debug2(NGX_LOG_DEBUG_CORE, tp->log, 0,
    //               "task #%ui added to thread pool \"%V\"",
    //               task->id, &tp->name);

    return NGX_OK;
}


static void *
ngx_thread_pool_cycle(void *data)
{
    ngx_thread_pool_t *tp = data;

    int                 err;
    sigset_t            set;
    ngx_thread_task_t  *task;

#if 0
    ngx_time_update();
#endif

    //ngx_log_debug1(NGX_LOG_DEBUG_CORE, tp->log, 0,
    //               "thread in pool \"%V\" started", &tp->name);

    sigfillset(&set);

    sigdelset(&set, SIGILL);
    sigdelset(&set, SIGFPE);
    sigdelset(&set, SIGSEGV);
    sigdelset(&set, SIGBUS);

    err = pthread_sigmask(SIG_BLOCK, &set, NULL);
    if (err) {
        //ngx_log_error(NGX_LOG_ALERT, tp->log, err, "pthread_sigmask() failed");
        return NULL;
    }

    for ( ;; ) {
        if (ngx_thread_mutex_lock(&tp->mtx) != NGX_OK) {
            return NULL;
        }

        /* the number may become negative */
        tp->waiting--;

        while (tp->queue.first == NULL) {
            if (ngx_thread_cond_wait(&tp->cond, &tp->mtx)
                != NGX_OK)
            {
                (void) ngx_thread_mutex_unlock(&tp->mtx);
                return NULL;
            }
        }

        task = tp->queue.first;
        tp->queue.first = task->next;

        if (tp->queue.first == NULL) {
            tp->queue.last = &tp->queue.first;
        }

        if (ngx_thread_mutex_unlock(&tp->mtx) != NGX_OK) {
            return NULL;
        }

#if 0
        ngx_time_update();
#endif

        //ngx_log_debug2(NGX_LOG_DEBUG_CORE, tp->log, 0,
        //               "run task #%ui in thread pool \"%V\"",
        //               task->id, &tp->name);

        task->handler(task->ctx);

        //ngx_log_debug2(NGX_LOG_DEBUG_CORE, tp->log, 0,
        //               "complete task #%ui in thread pool \"%V\"",
        //               task->id, &tp->name);

        task->next = NULL;
        
        //ngx_spinlock(&ngx_thread_pool_done_lock, 1, 2048);
        //
        //*ngx_thread_pool_done.last = task;
        //ngx_thread_pool_done.last = &task->next;
        //
        //ngx_memory_barrier();
        //
        //ngx_unlock(&ngx_thread_pool_done_lock);

		//this is nginx call(notify) event after the task is done
        //(void) ngx_notify(ngx_thread_pool_handler);
    }
}


//static void
//ngx_thread_pool_handler()
//{
//    ngx_event_t        *event;
//    ngx_thread_task_t  *task;
//
//    //ngx_log_debug0(NGX_LOG_DEBUG_CORE, ev->log, 0, "thread pool handler");
//
//    ngx_spinlock(&ngx_thread_pool_done_lock, 1, 2048);
//
//    task = ngx_thread_pool_done.first;
//    ngx_thread_pool_done.first = NULL;
//    ngx_thread_pool_done.last = &ngx_thread_pool_done.first;
//
//    ngx_memory_barrier();
//
//    ngx_unlock(&ngx_thread_pool_done_lock);
//
//    while (task) {
//        //ngx_log_debug1(NGX_LOG_DEBUG_CORE, ev->log, 0,
//        //               "run completion handler for task #%ui", task->id);
//
//        event = &task->event;
//        task = task->next;
//
//        event->complete = 1;
//        event->active = 0;
//
//        event->handler(event);
//    }
//}
//

//config me
ngx_thread_pool_t* 
ngx_thread_pool_config(ngx_uint_t threads)
{
	ngx_int_t max_queue = 65536;
	if (threads < 1) {
		threads = 1;
	}
	
	ngx_thread_pool_t* tp = &g_tp;
	
	if (tp->threads) {
		return tp;
	}
	
	tp->threads = threads;
	tp->max_queue = max_queue;
	
	return tp;
}

ngx_int_t
ngx_thread_pool_init_worker(ngx_thread_pool_t* tp)
{
    //ngx_uint_t                i;
    //ngx_thread_pool_t       **tpp;
    //ngx_thread_pool_conf_t   *tcf;

    //if (ngx_process != NGX_PROCESS_WORKER
    //    && ngx_process != NGX_PROCESS_SINGLE)
    //{
    //    return NGX_OK;
    //}

    //tcf = (ngx_thread_pool_conf_t *) ngx_get_conf(cycle->conf_ctx,
    //                                              ngx_thread_pool_module);
    //
    //if (tcf == NULL) {
    //    return NGX_OK;
    //}
    //
    //ngx_thread_pool_queue_init(&ngx_thread_pool_done);
    //
    //tpp = tcf->pools.elts;
    //
    //for (i = 0; i < tcf->pools.nelts; i++) {
    //    if (ngx_thread_pool_init(tpp[i], cycle->log, cycle->pool) != NGX_OK) {
    //        return NGX_ERROR;
    //    }
    //}
	
	ngx_ncpu = sysconf(_SC_NPROCESSORS_ONLN);
	LOG_INFO("ncpu %d",ngx_ncpu);
	
	if (ngx_thread_pool_init(tp) != NGX_OK) {
		return NGX_ERROR;
	}

    return NGX_OK;
}


void
ngx_thread_pool_exit_worker(ngx_thread_pool_t* tp)
{
    //ngx_uint_t                i;
    //ngx_thread_pool_t       **tpp;
    //ngx_thread_pool_conf_t   *tcf;

    //if (ngx_process != NGX_PROCESS_WORKER
    //    && ngx_process != NGX_PROCESS_SINGLE)
    //{
    //    return;
    //}
    //
    //tcf = (ngx_thread_pool_conf_t *) ngx_get_conf(cycle->conf_ctx,
    //                                              ngx_thread_pool_module);
    //
    //if (tcf == NULL) {
    //    return;
    //}
    //
    //tpp = tcf->pools.elts;
    //
    //for (i = 0; i < tcf->pools.nelts; i++) {
    //    ngx_thread_pool_destroy(tpp[i]);
    //}
	
	ngx_thread_pool_destroy(tp);
}
