/*DO NOT MODIFY THIS FILE*/

#ifndef	__SCHEDULER__H__
#define	__SCHEDULER__H__

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>

#include "list.h"

// Structure prototypes
struct process; 
// typedef defines a new variable type process_t and you can use it to create a variable just like you would use any other variable type (i.e. int, float, etc.)
typedef struct process process_t;

struct sched_queue; 
typedef struct sched_queue sched_queue_t;

/* This is a simple simulation of process functions
    it contains functions that the process_function will use
    so it must be able to (in the order defined below)
    - Initilize the process information and the queue it belongs to
    - Destroy the process image (Free memory, destroy semaphores)
    - Terminate the process (save the process data in a file and signal the queue the process has been removed updating the ready and sched_queue semaphores accordingly)
    - Allow the process to go back to the scheduling queue after time slice
*/
typedef struct process_ops{
    void            (*init_process_info)            (process_t *info, sched_queue_t *queue);
    void            (*destroy_process_info)         (process_t *info);
    void            (*wait_for_cpu)                 (process_t *info);
    void            (*enter_sched_queue)            (sched_queue_t *queue, process_t *info);
    void            (*return_to_queue)              (sched_queue_t *queue, list_elem_t *elt);
    void            (*terminate_process)            (process_t *info, float completionTime, FILE *completionTimes);
}process_ops_t;

/* This is a simple simulation of kernel functions,
    it contains functions to control the execution
    of processes on the CPU, so it must be able to (in the order defined below):
    - Initialize all necessary semaphores and mutex on the scheduler queue
    - Destroy the queue and everything in it once the system is done executing processes
    - Send a signal to a process and wake it so it can control the CPU
    - Make the dispatcher wait until the process' time on the CPU expires or the process terminates
*/
typedef struct scheduler_ops {
    void            (*init_sched_queue)             (sched_queue_t *queue, int queue_size, float time_slice);
    void            (*destroy_sched_queue)          (sched_queue_t *queue);
    void            (*signal_process)               (process_t *info);
    void            (*wait_for_process)             (sched_queue_t *queue);
    process_t *     (*next_process)                 (sched_queue_t *queue);
    void            (*wait_for_queue)               (sched_queue_t *queue);
    void            (*wait_for_cpu)                 (sched_queue_t *info);
    void            (*release_cpu)                  (sched_queue_t *queue);

}sched_ops_t;

/* This structure combines both sets of functions to be accessed from the same location */
typedef struct scheduler{
    process_ops_t process_ops;
    sched_ops_t sched_ops;
}scheduler_t;


// to be initialized in scheduler.c
extern scheduler_t dispatch_rr;
extern scheduler_t dispatch_fifo;


#endif /* __SCHEDULER__H__ */
