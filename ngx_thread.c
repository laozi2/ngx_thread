
#include "ngx_atomic.h"
#include "ngx_thread.h"
#include "flog.h"


/*
 * All modern pthread mutex implementations try to acquire a lock
 * atomically in userland before going to sleep in kernel.  Some
 * spins before the sleeping.
 *
 * In Solaris since version 8 all mutex types spin before sleeping.
 * The default spin count is 1000.  It can be overridden using
 * _THREAD_ADAPTIVE_SPIN=100 environment variable.
 *
 * In MacOSX all mutex types spin to acquire a lock protecting a mutex's
 * internals.  If the mutex is busy, thread calls Mach semaphore_wait().
 *
 *
 * PTHREAD_MUTEX_NORMAL lacks deadlock detection and is the fastest
 * mutex type.
 *
 *   Linux:    No spinning.  The internal name PTHREAD_MUTEX_TIMED_NP
 *             remains from the times when pthread_mutex_timedlock() was
 *             non-standard extension.  Alias name: PTHREAD_MUTEX_FAST_NP.
 *   FreeBSD:  No spinning.
 *
 *
 * PTHREAD_MUTEX_ERRORCHECK is usually as fast as PTHREAD_MUTEX_NORMAL
 * yet has lightweight deadlock detection.
 *
 *   Linux:    No spinning.  The internal name: PTHREAD_MUTEX_ERRORCHECK_NP.
 *   FreeBSD:  No spinning.
 *
 *
 * PTHREAD_MUTEX_RECURSIVE allows recursive locking.
 *
 *   Linux:    No spinning.  The internal name: PTHREAD_MUTEX_RECURSIVE_NP.
 *   FreeBSD:  No spinning.
 *
 *
 * PTHREAD_MUTEX_ADAPTIVE_NP spins on SMP systems before sleeping.
 *
 *   Linux:    No deadlock detection.  Dynamically changes a spin count
 *             for each mutex from 10 to 100 based on spin count taken
 *             previously.
 *   FreeBSD:  Deadlock detection.  The default spin count is 2000.
 *             It can be overriden using LIBPTHREAD_SPINLOOPS environment
 *             variable or by pthread_mutex_setspinloops_np().  If a lock
 *             is still busy, sched_yield() can be called on both UP and
 *             SMP systems.  The default yield loop count is zero, but
 *             it can be set by LIBPTHREAD_YIELDLOOPS environment
 *             variable or by pthread_mutex_setyieldloops_np().
 *   Solaris:  No PTHREAD_MUTEX_ADAPTIVE_NP.
 *   MacOSX:   No PTHREAD_MUTEX_ADAPTIVE_NP.
 *
 *
 * PTHREAD_MUTEX_ELISION_NP is a Linux extension to elide locks using
 * Intel Restricted Transactional Memory.  It is the most suitable for
 * rwlock pattern access because it allows simultaneous reads without lock.
 * Supported since glibc 2.18.
 *
 *
 * PTHREAD_MUTEX_DEFAULT is default mutex type.
 *
 *   Linux:    PTHREAD_MUTEX_NORMAL.
 *   FreeBSD:  PTHREAD_MUTEX_ERRORCHECK.
 *   Solaris:  PTHREAD_MUTEX_NORMAL.
 *   MacOSX:   PTHREAD_MUTEX_NORMAL.
 */



ngx_int_t
ngx_thread_mutex_create(ngx_thread_mutex_t *mtx)
{
    ngx_err_t            err;
    pthread_mutexattr_t  attr;

    err = pthread_mutexattr_init(&attr);
    if (err != 0) {
        //ngx_log_error(NGX_LOG_EMERG, log, err,
        //              "pthread_mutexattr_init() failed");
		LOG_ERROR("pthread_mutexattr_init() failed");
        return NGX_ERROR;
    }

    err = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    if (err != 0) {
        //ngx_log_error(NGX_LOG_EMERG, log, err,
        //              "pthread_mutexattr_settype"
        //              "(PTHREAD_MUTEX_ERRORCHECK) failed");
		LOG_ERROR("pthread_mutexattr_settype() failed");
        return NGX_ERROR;
    }

    err = pthread_mutex_init(mtx, &attr);
    if (err != 0) {
        //ngx_log_error(NGX_LOG_EMERG, log, err,
        //              "pthread_mutex_init() failed");
		LOG_ERROR("pthread_mutex_init() failed");
        return NGX_ERROR;
    }

    err = pthread_mutexattr_destroy(&attr);
    if (err != 0) {
        //ngx_log_error(NGX_LOG_ALERT, log, err,
        //              "pthread_mutexattr_destroy() failed");
		LOG_ERROR("pthread_mutexattr_destroy() failed");
    }

    //ngx_log_debug1(NGX_LOG_DEBUG_CORE, log, 0,
    //               "pthread_mutex_init(%p)", mtx);
	LOG_DEBUG("pthread_mutex_init(%p)", mtx);
    return NGX_OK;
}


ngx_int_t
ngx_thread_mutex_destroy(ngx_thread_mutex_t *mtx)
{
    ngx_err_t  err;

    err = pthread_mutex_destroy(mtx);
    if (err != 0) {
        //ngx_log_error(NGX_LOG_ALERT, log, err,
        //              "pthread_mutex_destroy() failed");
		LOG_ERROR("pthread_mutex_destroy() failed");
        return NGX_ERROR;
    }

    //ngx_log_debug1(NGX_LOG_DEBUG_CORE, log, 0,
    //               "pthread_mutex_destroy(%p)", mtx);
	LOG_DEBUG("pthread_mutex_destroy(%p)", mtx);
    return NGX_OK;
}


ngx_int_t
ngx_thread_mutex_lock(ngx_thread_mutex_t *mtx)
{
    ngx_err_t  err;

    //ngx_log_debug1(NGX_LOG_DEBUG_CORE, log, 0,
    //               "pthread_mutex_lock(%p) enter", mtx);
	LOG_DEBUG("pthread_mutex_lock(%p)", mtx);
	
    err = pthread_mutex_lock(mtx);
    if (err == 0) {
        return NGX_OK;
    }

    //ngx_log_error(NGX_LOG_ALERT, log, err, "pthread_mutex_lock() failed");
	LOG_ERROR("pthread_mutex_lock() failed");
    return NGX_ERROR;
}


ngx_int_t
ngx_thread_mutex_unlock(ngx_thread_mutex_t *mtx)
{
    ngx_err_t  err;

    err = pthread_mutex_unlock(mtx);

#if 0
    ngx_time_update();
#endif

    if (err == 0) {
        //ngx_log_debug1(NGX_LOG_DEBUG_CORE, log, 0,
        //               "pthread_mutex_unlock(%p) exit", mtx);
		LOG_DEBUG("pthread_mutex_lock(%p)", mtx);
        return NGX_OK;
    }

    //ngx_log_error(NGX_LOG_ALERT, log, err, "pthread_mutex_unlock() failed");
	LOG_ERROR("pthread_mutex_unlock() failed");
    return NGX_ERROR;
}



//-----thread_cond-------

ngx_int_t
ngx_thread_cond_create(ngx_thread_cond_t *cond)
{
    ngx_err_t  err;

    err = pthread_cond_init(cond, NULL);
    if (err == 0) {
        //ngx_log_debug1(NGX_LOG_DEBUG_CORE, log, 0,
        //               "pthread_cond_init(%p)", cond);
		LOG_DEBUG("pthread_mutex_lock(%p)", cond);
        return NGX_OK;
    }

    //ngx_log_error(NGX_LOG_EMERG, log, err, "pthread_cond_init() failed");
    LOG_ERROR("pthread_cond_init() failed");
	return NGX_ERROR;
}


ngx_int_t
ngx_thread_cond_destroy(ngx_thread_cond_t *cond)
{
    ngx_err_t  err;

    err = pthread_cond_destroy(cond);
    if (err == 0) {
        //ngx_log_debug1(NGX_LOG_DEBUG_CORE, log, 0,
        //               "pthread_cond_destroy(%p)", cond);
		LOG_DEBUG("pthread_cond_destroy(%p)", cond);
        return NGX_OK;
    }

    //ngx_log_error(NGX_LOG_EMERG, log, err, "pthread_cond_destroy() failed");
	LOG_ERROR("pthread_cond_destroy() failed");
    return NGX_ERROR;
}


ngx_int_t
ngx_thread_cond_signal(ngx_thread_cond_t *cond)
{
    ngx_err_t  err;

    err = pthread_cond_signal(cond);
    if (err == 0) {
        //ngx_log_debug1(NGX_LOG_DEBUG_CORE, log, 0,
        //               "pthread_cond_signal(%p)", cond);
		LOG_DEBUG("pthread_cond_signal(%p)", cond);
        return NGX_OK;
    }

    //ngx_log_error(NGX_LOG_EMERG, log, err, "pthread_cond_signal() failed");
    LOG_ERROR("pthread_cond_signal() failed");
	return NGX_ERROR;
}


ngx_int_t
ngx_thread_cond_wait(ngx_thread_cond_t *cond, ngx_thread_mutex_t *mtx)
{
    ngx_err_t  err;

    //ngx_log_debug1(NGX_LOG_DEBUG_CORE, log, 0,
    //               "pthread_cond_wait(%p) enter", cond);
	LOG_DEBUG("pthread_cond_wait(%p) enter", cond);
	
    err = pthread_cond_wait(cond, mtx);

#if 0
    ngx_time_update();
#endif

    if (err == 0) {
        //ngx_log_debug1(NGX_LOG_DEBUG_CORE, log, 0,
        //               "pthread_cond_wait(%p) exit", cond);
		LOG_DEBUG("pthread_cond_wait(%p) exit", cond);
        return NGX_OK;
    }

    //ngx_log_error(NGX_LOG_ALERT, log, err, "pthread_cond_wait() failed");
	LOG_ERROR("pthread_cond_wait() failed");
    return NGX_ERROR;
}


//--------------ngx_thread_tid-----------

#if (NGX_LINUX)

/*
 * Linux thread id is a pid of thread created by clone(2),
 * glibc does not provide a wrapper for gettid().
 */

ngx_tid_t
ngx_thread_tid(void)
{
    return syscall(SYS_gettid);
}

#else
ngx_tid_t
ngx_thread_tid(void)
{
    return (uint64_t) (uintptr_t) pthread_self();
}

#endif

//spinlock

void
ngx_spinlock(ngx_atomic_t *lock, ngx_atomic_int_t value, ngx_uint_t spin)
{

#if (NGX_HAVE_ATOMIC_OPS)

    ngx_uint_t  i, n;

    for ( ;; ) {

        if (*lock == 0 && ngx_atomic_cmp_set(lock, 0, value)) {
            return;
        }

        if (ngx_ncpu > 1) {

            for (n = 1; n < spin; n <<= 1) {

                for (i = 0; i < n; i++) {
                    ngx_cpu_pause();
                }

                if (*lock == 0 && ngx_atomic_cmp_set(lock, 0, value)) {
                    return;
                }
            }
        }

        ngx_sched_yield();
    }

#else

//#if (NGX_THREADS)

#error ngx_spinlock() or ngx_atomic_cmp_set() are not defined !

//#endif

#endif

}

