/*
 * Copyright (c) 2016, Mathias Brossard <mathias@brossard.org>.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file threadpool.c
 * @brief Threadpool implementation file
 */

#include <stdlib.h>
#include <time.h>

#ifdef __linux__
#include <pthread.h>
#include <unistd.h>
#endif

#include "tinycthread_pool.h"


/**
 * @function void *threadpool_thread(void *threadpool)
 * @brief the worker thread
 * @param threadpool the pool which own the thread
 */
static void *threadpool_thread(void *threadpool);

int threadpool_free(threadpool_t *pool);

threadpool_t *threadpool_create(int thread_count, int queue_size, int flags)
{
    threadpool_t *pool;
    int i;
    (void) flags;

    if(thread_count <= 0 || thread_count > MAX_THREADS || queue_size <= 0 || queue_size > MAX_QUEUE) {
        return NULL;
    }

    if((pool = (threadpool_t *)malloc(sizeof(threadpool_t))) == NULL) {
        goto err;
    }

    /* Initialize */
    pool->thread_count = 0;
    pool->queue_size = queue_size;
    pool->head = pool->tail = pool->count = 0;
    pool->shutdown = pool->started = 0;
    pool->busy_thread = 0;

    /* Allocate thread and task queue */
    pool->threads = (thrd_t *)malloc(sizeof(thrd_t) * MAX_THREADS);
    pool->queue = (threadpool_task_t *)malloc(sizeof(threadpool_task_t) * queue_size);

    /* Initialize mutex and conditional variable first */
    if((mtx_init(&(pool->lock), NULL) != thrd_success) ||
       (mtx_init(&(pool->thread_lock), NULL) != thrd_success) ||
       (cnd_init(&(pool->notify)) != thrd_success) ||
       (pool->threads == NULL) ||
       (pool->queue == NULL)) {
        goto err;
    }

    /* Start worker threads */
    for(i = 0; i < thread_count; i++) {
        if(thrd_create(&(pool->threads[i]), (thrd_start_t)threadpool_thread, (void*)pool) != thrd_success) {
            threadpool_destroy(pool, 0);
            return NULL;
        }
        pool->thread_count++;
        pool->started++;
    }

    return pool;

 err:
    if(pool) {
        threadpool_free(pool);
    }
    return NULL;
}

int threadpool_add(threadpool_t *pool, void (*function)(void *), void *argument, int flags)
{
    int err = 0;
    int next;
    (void) flags;

    if(pool == NULL || function == NULL) {
        return threadpool_invalid;
    }

    if(mtx_lock(&(pool->lock)) != thrd_success) {
        return threadpool_lock_failure;
    }

    next = (pool->tail + 1) % pool->queue_size;

    do {
        /* Are we full ? */
        if(pool->count == pool->queue_size) {
            err = threadpool_queue_full;
            break;
        }

        /* Are we shutting down ? */
        if(pool->shutdown) {
            err = threadpool_shutdown;
            break;
        }

        /* Add task to queue */
        pool->queue[pool->tail].function = function;
        pool->queue[pool->tail].argument = argument;
        pool->tail = next;
        pool->count += 1;

        /* If all the threads are busy */
        if (pool->busy_thread == pool->thread_count) {
            if (pool->thread_count <= MAX_THREADS) {
                /* try to create more thread to the threadpool */
                int new_thread_count = pool->thread_count * 2;
                if (new_thread_count >= MAX_THREADS) { new_thread_count = MAX_THREADS; }
                int counter = 0;
                while(pool->started < new_thread_count){
                    if (thrd_create(&(pool->threads[counter]),(thrd_start_t) threadpool_thread, (void*)pool) != thrd_success) {
                        counter++;
                        continue;
                    }
                    pool->thread_count++;
                    pool->started++;
                }
                pool->thread_count = new_thread_count;
            }
        }

        /* pthread_cond_broadcast */
        if(cnd_signal(&(pool->notify)) != thrd_success) {
            err = threadpool_lock_failure;
            break;
        }
    } while(0);

    if(mtx_unlock(&pool->lock) != thrd_success) {
        err = threadpool_lock_failure;
    }

    return err;
}

int threadpool_destroy(threadpool_t *pool, int flags)
{
    int i, err = 0;

    if(pool == NULL) {
        return threadpool_invalid;
    }

    if(mtx_lock(&(pool->lock)) != thrd_success) {
        return threadpool_lock_failure;
    }

    do {
        /* Already shutting down */
        if(pool->shutdown) {
            err = threadpool_shutdown;
            break;
        }

        pool->shutdown = (flags & threadpool_graceful) ?
            graceful_shutdown : immediate_shutdown;

        /* Wake up all worker threads */
        if((cnd_broadcast(&(pool->notify)) != thrd_success) ||
           (mtx_unlock(&(pool->lock)) != thrd_success)) {
            err = threadpool_lock_failure;
            break;
        }

        /* Join all worker thread */
        for(i = 0; i < pool->thread_count; i++) {
            if(thrd_join(pool->threads[i], NULL) != thrd_success) {
                err = threadpool_thread_failure;
            }
        }
    } while(0);

    /* Only if everything went well do we deallocate the pool */
    if(!err) {
        threadpool_free(pool);
    }
    return err;
}

int threadpool_free(threadpool_t *pool)
{
    if(pool == NULL || pool->started > 0) {
        return -1;
    }

    /* Did we manage to allocate ? */
    if(pool->threads) {
        free(pool->threads);
        free(pool->queue);
 
        /* Because we allocate pool->threads after initializing the
           mutex and condition variable, we're sure they're
           initialized. Let's lock the mutex just in case. */
        mtx_lock(&(pool->lock));
        mtx_destroy(&(pool->lock));
        cnd_destroy(&(pool->notify));
    }
    free(pool);    
    return 0;
}


static void *threadpool_thread(void *threadpool)
{
    threadpool_t *pool = (threadpool_t *)threadpool;
    threadpool_task_t task;

    for(;;) {
        /* Lock must be taken to wait on conditional variable */
        mtx_lock(&(pool->lock));

        /* Wait on condition variable, check for spurious wakeups.
           When returning from cnd_wait(), we own the lock. */
        while((pool->count == 0) && (!pool->shutdown)) {
            cnd_wait(&(pool->notify), &(pool->lock));
        }

        if((pool->shutdown == immediate_shutdown) ||
           ((pool->shutdown == graceful_shutdown) &&
            (pool->count == 0))) {
            break;
        }
        
        /* Grab our task */
        task.function = pool->queue[pool->head].function;
        task.argument = pool->queue[pool->head].argument;
        pool->head = (pool->head + 1) % pool->queue_size;
        pool->count -= 1;

        /* Unlock */
        mtx_unlock(&(pool->lock));

        mtx_lock(&(pool->thread_lock));
        pool->busy_thread++;
        mtx_unlock(&(pool->thread_lock));

        /* Get to work */
        (*(task.function))(task.argument);

        mtx_lock(&(pool->thread_lock));
        pool->busy_thread--;
        mtx_unlock(&(pool->thread_lock));

        //The pool size has been modified, terminate this thread
        if (pool->thread_count > pool->started) {
            break;
        }
    }

    pool->started--;

    mtx_unlock(&(pool->lock));
    thrd_exit(0);
    return(NULL);
}