/*DO NOT MODIFY THIS FILE*/

#ifndef ___SCHED_THREADS___
#define ___SCHED_THREADS___
 
#include "list.h"
#include "scheduler.h"

// scheduler thread function prototypes
void *long_term_scheduler(void *arg);
void *short_term_scheduler(void *arg);
void *process_function(void *arg);

/* Processes will be created with some basic info (in the order defined below):
    - The process ID
    - The process expected service time
    - The process arrival time
    - The time the process completes its execution (record this from the global time when process completes)
    - The process context (a node in a linked list)
    - Access to the CPU which should be initiality unavailable until the dispatcher schedules the process
    After creaction the process will be moved to a 
    queue where the process will wait for execution.
*/
struct process{
    int pid;
    float serviceTime;
    int arrivalTime;
    float completionTime;
    list_elem_t *context;
    sched_queue_t *queue;
    sem_t cpu_sem; 
};

/* The queue must be able to control (in the order defined below):
    - A linked list to act as our process queue
    - A dispatcher program (this structure will control everything about the execution of processess from initializing the queue, choosing ready processes for execution and eventually deleting the queue)
    - Access to the CPU (the semaphore should allow only 1 process for execution)
    - Access to the QUEUE (this mutex should protect the integrity of the queue)
    - The max size of the queue (this sempahore should keep track of the maximum amount of processes allowed at the same time in the queue)
    - The current amount of processes in the queue (this semaphore should keep track of the current number of ready processes in the queue)
*/
struct sched_queue{
    list_t lst;
    float time_slice;
    float global_time;
    pthread_mutex_t lock;
    scheduler_t ops;
    sem_t cpu_sem, ready_sem, sched_queue_sem;
};

#endif