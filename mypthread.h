#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <semaphore.h>
#include "cpu_info.h"
#include <sched.h>

#define PS_NEW 1
#define PS_RUNNABLE 2
#define PS_ACTIVE 3
#define PS_BLOCKED 4
#define PS_DEAD 5
#define PS_RUNNING 6
#define PS_MUTEX_WAIT 7
#define PS_COND_WAIT 8

#define ULONLY 0
#define KLMATCHCORES 1
#define KLMATCHHYPER 2
#define KLALWAYS 3

struct pthread_info 
{
   int thread_id;
   int thread_state;
   ucontext_t *ucontext;
   int joined_from_th;
}; 

typedef struct pthread_info mypthread_info_t;

struct pthread_node
{
   mypthread_info_t *pthread;
   struct pthread_node *next;
   struct pthread_node *prev;
};

typedef struct pthread_node mypthread_t;

struct thread_queue
{
   mypthread_t *front;
   mypthread_t *rear;
};

struct linear_q_node
{
   mypthread_info_t *pthread;
   struct linear_q_node *next;
}; 

typedef struct linear_q_node linear_queue_node;

typedef struct
{
   linear_queue_node *head;
   linear_queue_node *tail;
} linear_queue;

 
//linear_queue *cond_queue;
//linear_queue *mutex_queue;

//typedef int mypthread_mutexattr_t;

typedef struct {
   int lock;
   linear_queue *mutex_block_q;
} mypthread_mutex_t;

typedef struct {
   linear_queue *cond_block_q;
} mypthread_cond_t;

mypthread_t* search_thread(struct thread_queue *q, int thread_id);

typedef struct {
    mypthread_t thread;
    int kmode;
} kargs;

int mypthread_create(mypthread_t *thread, 
                   const pthread_attr_t *attr,
                   void *(*start_routine) (void *),
                   void *arg);

void mypthread_exit(void *retval);

int mysched_yield(void);

int mypthread_cond_init(mypthread_cond_t *cond,
              const pthread_condattr_t *attr);

int mypthread_cond_broadcast(mypthread_cond_t *cond);

int mypthread_cond_signal(mypthread_cond_t *cond);

int mypthread_cond_wait(mypthread_cond_t *cond,
                      mypthread_mutex_t *mutex);

int mypthread_mutex_init(mypthread_mutex_t *mutex,
                       const pthread_mutexattr_t *attr);

int mypthread_mutex_lock(mypthread_mutex_t *mutex);

int mypthread_mutex_unlock(mypthread_mutex_t *mutex);

int mypthread_join(mypthread_t thread, void **value_ptr);

int schedule_kernel_thread(void *);

int ksched(void *);

int mythread_scheduler(void *);

/* Implemented for extra credit *********************/

int mypthread_mutex_destroy(mypthread_mutex_t *mutex);

int mypthread_mutex_trylock(mypthread_mutex_t *mutex);

/* **************************************************/
