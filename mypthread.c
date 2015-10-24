#include "mypthread.h"

/* mypthread_t* main_thread; */
struct thread_queue *mythread_q;
static int first_call = 1;
int num_of_nodes = 0;
int cur_thread_id = 1;
int glob_thread_id = 0;
int mode = ULONLY;
int num_cores = 1;     //Actual number of cores in the system
int num_hyperthreads = 1;  //Actual number of hyperthreads in the system
int kth = 1;    //Number of kernel threads created in the process
int desired_kth = 1; //Number of kernel threads required to be created 

sem_t sem_mutex;
sem_t sem_cond;
sem_t sem_main;
sem_t sem_kern;

#ifndef CHILD_SIG
#define CHILD_SIG SIGUSR1       /* Signal to be generated on termination
                                   of cloned child */
#endif

void new_thread_node(mypthread_t *temp)
{
   //mypthread_t* temp = (mypthread_t*)malloc(sizeof(mypthread_t));
   ucontext_t *context = (ucontext_t *)malloc(sizeof(ucontext_t));
   
   temp->pthread = (mypthread_info_t *)malloc(sizeof(mypthread_info_t));
 
   temp->pthread->thread_id = ++glob_thread_id;
   temp->pthread->thread_state = PS_NEW;
   temp->pthread->ucontext = context;
   temp->pthread->ucontext->uc_stack.ss_sp = (char *)malloc(sizeof(char) * 4096);
   temp->pthread->ucontext->uc_stack.ss_size = 4096;
   temp->next = NULL;
   temp->prev = NULL;
   return;
}

struct thread_queue* create_thread_queue()
{
   struct thread_queue* q = (struct thread_queue*)malloc(sizeof(struct thread_queue));
   q->front = q->rear = NULL;
   return q; 
}


void enqueue(struct thread_queue* q, mypthread_t* temp)
{

   num_of_nodes++;
   if (q->rear == NULL)
   {
      q->front = q->rear = temp;
      return;
   }
 
   q->rear->next = temp;
   temp->prev = q->rear;
   q->rear = temp;
   q->rear->next = q->front;
   q->front->prev = q->rear;
}

mypthread_t* dequeue_thread(struct thread_queue* q)
{
   if (q->front == NULL)
      return;
 
   num_of_nodes--; 
   mypthread_t *temp = q->front;
   q->front = q->front->next;
   q->rear->next = q->front;
   q->front->prev = q->rear;
   
   if (q->front == NULL)
      q->rear == NULL;

   return temp;
}

mypthread_t* next_active_thread(struct thread_queue *q)
{
    int iter = 0;
    mypthread_t* temp;
    mypthread_t *current_thread = search_thread(q, cur_thread_id);
    temp = current_thread->next;
    while ( temp!= NULL && iter <= num_of_nodes)
    {
       if (temp->pthread->thread_state == PS_ACTIVE)
       {
           return temp;
       }
       else
       {
           temp = temp->next;
       }
       iter++;
    }
    return NULL;
}

mypthread_t* return_head(struct thread_queue *q)
{
    return(q->front);
}

mypthread_t* return_tail(struct thread_queue *q)
{
    return(q->rear);
}

void mypthread_init()
{
    /* main_thread = new_thread_node(); */
    mythread_q = create_thread_queue();
    //cond_queue = create_linear_queue();
    //mutex_queue = create_linear_queue();
    sem_init(&sem_main, 0, 1);
    sem_init(&sem_kern, 0, 1 );
}

mypthread_t* search_thread(struct thread_queue *q, int thread_id)
{
    int iter = 0;
    mypthread_t* temp;  
    //mypthread_t *current_thread = search_thread(mythread_q, cur_thread_id); 
    temp = q->front;
    while(temp != NULL && iter <= num_of_nodes)
    {
       if (temp->pthread->thread_id == thread_id)
           return (temp);
       else
           temp = temp->next;
       iter++;
    }
    return NULL;
}

/*********************************************************
Code for linear queue
*********************************************************/

linear_queue* create_linear_queue()
{
   linear_queue* q = (linear_queue*)malloc(sizeof(linear_queue));
   q->head = q->tail = NULL;
   return q;
}

linear_queue_node* new_linear_node(mypthread_info_t *pthread)
{
   linear_queue_node *temp = (linear_queue_node *)malloc(sizeof(linear_queue_node));
   temp->pthread = pthread;
   temp->next = NULL;
   return temp;
}

void linear_enqueue(linear_queue *q, mypthread_info_t *pthread)
{
   linear_queue_node *temp = new_linear_node(pthread);
   if (q->tail == NULL)
   {
      q->head = q->tail = temp;
      return; 
   }
   q->tail->next = temp;
   q->tail = temp;
   
}

linear_queue_node* linear_dequeue(linear_queue *q)
{
   if (q->head == NULL)
      return NULL;
   linear_queue_node *temp = q->head;
   q->head = q->head->next;
 
   if (q->head == NULL)
   {
      q->tail = NULL;
   } 
   return temp;
}

/********** End of linear queue code *******************/

/*******************************************************
 * Kernel threads specific code
 ******************************************************/

void pthread_init(int kt)
{
    int mode = kt;
    int num_cores = get_num_of_cores();
    int num_hyperthreads = get_num_of_hyperthreads(); 
    //sem_init(&sem_kern, 0, 1 );
    
    switch(mode)
    {
       case ULONLY         : desired_kth = 1;
                             break;
       case KLMATCHCORES   : desired_kth = num_cores;
                             break;
       case KLMATCHHYPER   : desired_kth = num_cores * num_hyperthreads;
                             break;
       case KLALWAYS       : desired_kth = 32767;
                             break;  
    }

    return;
}

void create_kernel_thread() 
{

    const int STACK_SIZE = 65536;       /* Stack size for cloned child */
    char *stack;                        /* Start of stack buffer area */
    char *stackTop;                     /* End of stack buffer area */
    int flags = 0;                          /* Flags for cloning child */
    kargs *args;

    struct sigaction sa;

    flags = flags | CLONE_FILES | CLONE_FS | CLONE_SIGHAND | CLONE_VM;

    stack = malloc(STACK_SIZE);

    if (stack == NULL)
    { 
        printf("Error allocating stack for Kernel");
        exit(0);
    } 

    stackTop = stack + STACK_SIZE;

    args = (kargs *)malloc(sizeof(kargs));
    /*
    if (CHILD_SIG != 0) {
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;
        sa.sa_handler = grimReaper;  //Change this
        if (sigaction(CHILD_SIG, &sa, NULL) == -1)
            errExit("sigaction");
    }
    */

    if (clone(ksched , stackTop, flags , (void *) args) == -1)
    {
        printf("Error during clone");
        exit(0);
    }
    return;
}

void mykthread_create()
{
   if (kth < desired_kth)
   {
      printf("Creating kernel thread with id = %d\t", kth+1); 
      create_kernel_thread();     
      kth++; 
   } 
   return;
}

int ksched(void *args)
{
   while (next_active_thread(mythread_q) != NULL)
   {
       sem_wait(&sem_kern);
       mythread_scheduler(args);
       sem_post(&sem_kern);   
   }

}

int mythread_scheduler(void *args)
{
   kargs *tempargs;
   tempargs = (kargs *)args;
   int kmode;
   mypthread_t thread;
   kmode = tempargs->kmode;
   thread = tempargs->thread; 
   
   
        if (kmode == 1)
        {
           mypthread_t* cur_thread = search_thread(mythread_q, cur_thread_id);

           cur_thread->pthread->thread_state = PS_DEAD;
           free(cur_thread->pthread->ucontext);

           if (cur_thread->pthread->joined_from_th != 0)
           {
               mypthread_t *join_thread = search_thread(mythread_q,cur_thread->pthread->joined_from_th);
               join_thread->pthread->thread_state = PS_ACTIVE;

           }   
           mypthread_t* next_thread = return_head(mythread_q);
           cur_thread_id = next_thread->pthread->thread_id;
           //sem_post(&sem_main);
           setcontext(next_thread->pthread->ucontext);
       }
       else if(kmode == 2)
       {
            int target_thread_id = thread.pthread->thread_id;
            mypthread_t *current_thread = search_thread(mythread_q,cur_thread_id);
            mypthread_t *target_thread = search_thread(mythread_q,target_thread_id);
            if (target_thread->pthread->thread_state != PS_ACTIVE)
            {
                  //sem_post(&sem_main);
                   return;
            }
            else
            {
                current_thread->pthread->thread_state = PS_BLOCKED;
                target_thread->pthread->joined_from_th = cur_thread_id;
                cur_thread_id = target_thread_id;
                //sem_post(&sem_main);
                swapcontext(current_thread->pthread->ucontext, target_thread->pthread->ucontext);
            }
    

       } 
       else if(kmode == 3)
       {
            mypthread_t *current_thread = search_thread(mythread_q, cur_thread_id);
            mypthread_t *next_thread = next_active_thread(mythread_q);
            //printf("Current thread id = %d , next thread id = %d\n", cur_thread_id, next_thread->pthread->thread_id);
            if (cur_thread_id == next_thread->pthread->thread_id)
            {
                return;
            }
            //printf("Thread %d Yielding\n",cur_thread_id);
            cur_thread_id = next_thread->pthread->thread_id;
           //sem_post(&sem_main);
            swapcontext(current_thread->pthread->ucontext, next_thread->pthread->ucontext);
       }
}

/******************************************************
 *  End of kernel thread specific code                *
 *****************************************************/

/*******************************************************
 * Code for pthread library functions                  *
 *******************************************************/

int mypthread_create(mypthread_t *thread, 
                   const pthread_attr_t *attr,
                   void *(*start_routine) (void *),
                   void *arg)
{

   //sem_wait(&sem_main);

   if (first_call == 1)
   {
      mypthread_init();
      mypthread_t* temp = (mypthread_t*)malloc(sizeof(mypthread_t));
      new_thread_node(temp);
      enqueue(mythread_q,temp);
      temp->pthread->thread_state = PS_ACTIVE;
      fprintf(stderr, "Creating thread with id = %d\n",temp->pthread->thread_id);
      /* main_thread->thread_state = PS_ACTIVE; */
      first_call = 0;
   } 
   //sem_post(&sem_main);

   new_thread_node(thread);
   
   getcontext(thread->pthread->ucontext);  
   makecontext(thread->pthread->ucontext, (void (*)()) start_routine, 1, arg);

   //sem_wait(&sem_main);   
   enqueue(mythread_q,thread);
   thread->pthread->thread_state = PS_ACTIVE;
   fprintf(stderr, "Creating thread with id = %d\t",thread->pthread->thread_id);
   mykthread_create();
   //sem_post(&sem_main);
   
   return 0;
}

void mypthread_exit(void *retval)
{
    mypthread_t thread;
    
    kargs *tempargs;

    tempargs = (kargs *)malloc(sizeof(kargs));
    tempargs->thread = thread;
    tempargs->kmode = 1;

    mythread_scheduler((void *)tempargs);

    /*
  //sem_wait(&sem_main);
    mypthread_t* cur_thread = search_thread(mythread_q, cur_thread_id);

    cur_thread->pthread->thread_state = PS_DEAD;
    free(cur_thread->pthread->ucontext);

    if (cur_thread->pthread->joined_from_th != 0)
    {
        mypthread_t *join_thread = search_thread(mythread_q,cur_thread->pthread->joined_from_th);
        join_thread->pthread->thread_state = PS_ACTIVE;
         
    }
    mypthread_t* next_thread = return_head(mythread_q);
    cur_thread_id = next_thread->pthread->thread_id; 
    //sem_post(&sem_main);
    setcontext(next_thread->pthread->ucontext);
   */
    return;
}

int mypthread_join(mypthread_t thread, void **retval)
{
    kargs *tempargs;
    tempargs = (kargs *)malloc(sizeof(kargs));
    tempargs->thread = thread;
    tempargs->kmode = 2;

    mythread_scheduler((void *)tempargs);
    /*
    //sem_wait(&sem_main);
    int target_thread_id = thread.pthread->thread_id;
    mypthread_t *current_thread = search_thread(mythread_q,cur_thread_id);
    mypthread_t *target_thread = search_thread(mythread_q,target_thread_id);
    if (target_thread->pthread->thread_state != PS_ACTIVE)
    {
       //sem_post(&sem_main);
       return 0;
    }
    else
    {
       current_thread->pthread->thread_state = PS_BLOCKED;
       target_thread->pthread->joined_from_th = cur_thread_id;
       cur_thread_id = target_thread_id;
       //sem_post(&sem_main);       
       swapcontext(current_thread->pthread->ucontext, target_thread->pthread->ucontext);  
    }
    */
    return 0;
}

int mypthread_yield(void)
{

    mypthread_t thread;

    kargs *tempargs;
    tempargs = (kargs *)malloc(sizeof(kargs));
    tempargs->thread = thread;
    tempargs->kmode = 3;

    mythread_scheduler((void *)tempargs);
 
    /*
    //sem_wait(&sem_main);
    printf("Inside thread yield\n");
    mypthread_t *current_thread = search_thread(mythread_q, cur_thread_id);
    mypthread_t *next_thread = next_active_thread(mythread_q);
    printf("Current thread id = %d , next thread id = %d\n", cur_thread_id, next_thread->pthread->thread_id);
    if (cur_thread_id == next_thread->pthread->thread_id)
    {
        //sem_post(&sem_main);
        return 0;
    }
    printf("Thread %d Yielding\n",cur_thread_id);
    cur_thread_id = next_thread->pthread->thread_id;
   // sem_post(&sem_main);
    swapcontext(current_thread->pthread->ucontext, next_thread->pthread->ucontext);    
    */

    return 0; 
}

int mysched_yield(void)
{

}

int mypthread_cond_init(mypthread_cond_t *cond,
              const pthread_condattr_t *attr)
{
   cond->cond_block_q = create_linear_queue();
   sem_init(&sem_cond, 0, 1);
}

int mypthread_cond_broadcast(mypthread_cond_t *cond)
{
    linear_queue_node *temp;
    linear_queue_node *iter;

    sem_wait(&sem_cond);
    iter = cond->cond_block_q->head;
   
    while (iter != NULL)
    {    
       temp = linear_dequeue(cond->cond_block_q);
       sem_wait(&sem_main);
       temp->pthread->thread_state = PS_ACTIVE;
       sem_post(&sem_main);
       sem_post(&sem_cond);
       iter = iter->next;
       free(temp);
    }
    return 0;
}

int mypthread_cond_signal(mypthread_cond_t *cond)
{
    linear_queue_node *temp;

    sem_wait(&sem_cond);
    temp = linear_dequeue(cond->cond_block_q);
    sem_wait(&sem_main);
    temp->pthread->thread_state = PS_ACTIVE;
    sem_post(&sem_main);
    sem_post(&sem_cond);
    free(temp);
    return 0;
}

int mypthread_cond_wait(mypthread_cond_t *cond,
                      mypthread_mutex_t *mutex)
{
     sem_wait(&sem_cond);
     mypthread_mutex_unlock(mutex);
     sem_wait(&sem_main);
     mypthread_t *current_thread = search_thread(mythread_q, cur_thread_id);
     current_thread->pthread->thread_state = PS_BLOCKED;
     mypthread_t *next_thread = next_active_thread(mythread_q);
     cur_thread_id = next_thread->pthread->thread_id;
     linear_enqueue(cond->cond_block_q, current_thread->pthread);   
     sem_post(&sem_main);
     sem_post(&sem_mutex);
     swapcontext(current_thread->pthread->ucontext, next_thread->pthread->ucontext );
     mypthread_mutex_lock(mutex);
     return 0; 
}

int mypthread_mutex_init(mypthread_mutex_t *mutex,
                       const pthread_mutexattr_t *attr)
{
    mutex->lock = 0;
    mutex->mutex_block_q = create_linear_queue();
    sem_init(&sem_mutex, 0, 1);
}

int mypthread_mutex_lock(mypthread_mutex_t *mutex)
{
    sem_wait(&sem_mutex);
    if (mutex->lock == 0)
    {
        /* If I am here, that means i can acquire the mutex */
        mutex->lock == 1;
        sem_post(&sem_mutex);
    }   
    else
    {
        /* If I am here, that means I need to block on the mutex */
        sem_wait(&sem_main);
        mypthread_t *current_thread = search_thread(mythread_q, cur_thread_id);
        current_thread->pthread->thread_state = PS_BLOCKED;
        mypthread_t *next_thread = next_active_thread(mythread_q);
        cur_thread_id = next_thread->pthread->thread_id;
        linear_enqueue(mutex->mutex_block_q, current_thread->pthread);
        makecontext(current_thread->pthread->ucontext,(void (*)()) mypthread_mutex_lock,1, mutex);
        sem_post(&sem_main);
        sem_post(&sem_mutex); 
        swapcontext(current_thread->pthread->ucontext, next_thread->pthread->ucontext ); 
    } 

    return 0;
}

int mypthread_mutex_unlock(mypthread_mutex_t *mutex)
{

    linear_queue_node *temp;
    
    sem_wait(&sem_mutex);
    mutex->lock = 0;
    temp = linear_dequeue(mutex->mutex_block_q);
    if (temp!=NULL)
    {
       sem_wait(&sem_main);
       temp->pthread->thread_state = PS_ACTIVE; 
       sem_post(&sem_main);
       free(temp);
    }
    sem_post(&sem_mutex);
    //free(temp);
    return 0; 
}

/************Implemented for extra credit**************/

int mypthread_mutex_destroy(mypthread_mutex_t *mutex)
{
    mutex->lock = 999;
    free(mutex->mutex_block_q);    
}

int mypthread_mutex_trylock(mypthread_mutex_t *mutex)
{
    sem_wait(&sem_mutex);
    if (mutex->lock == 0)
    {
        /* If I am here, that means i can acquire the mutex */
        mutex->lock == 1;
        sem_post(&sem_mutex); 
        return 0;
    }
    
    sem_post(&sem_mutex);
    return -1;
}

/******************************************************/
