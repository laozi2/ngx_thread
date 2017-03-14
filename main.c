#include "ngx_common.h"
#include "ngx_thread.h"
#include "ngx_thread_pool.h"
#include "flog.h"

static void
ngx_thread_handler(void *data)
{
    int v = *(int*)data;
    LOG_INFO("ngx_thread_handler(), data %d, tid %d", v, ngx_log_tid);
}

int main(int argc, char* argv[])
{
    //Flogconf logconf = DEFLOGCONF;
    Flogconf logconf = {"/tmp/tmplog/main",LOGFILE_DEFMAXSIZE,L_LEVEL_MAX,0,1};
    if (0 > LOG_INIT(logconf)) {
        printf("LOG_INIT() failed\n");
        return -1;
    }

    ngx_thread_pool_t* tp = ngx_thread_pool_config(3);
    
    if ( NGX_OK != ngx_thread_pool_init_worker(tp) ) {
        LOG_ERROR("ngx_thread_pool_init_worker() failed");
        goto done;
    }
    
    ngx_thread_task_t    task;
    ngx_memzero(&task, sizeof(ngx_thread_task_t));

    task.handler = ngx_thread_handler;
    int data = 1;
    task.ctx = (void *) &data;
    
    if (ngx_thread_task_post(tp, &task) != NGX_OK) {
        return;
    }
    
    sleep(1);
    
done:
    ngx_thread_pool_exit_worker(tp);

    LOG_EXIT;
    
    return 0;
}