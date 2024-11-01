#ifndef __SCHEDULER__C__
#define __SCHEDULER__C__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include "scheduler.h"
#include "sched_threads.h"

void init_process_info(process_t *info, sched_queue_t *queue){
    // initiliaze the process:
    info->queue = queue; //initializes the queue pointer of the process information to a reference to the scheduler queue
    // Add the reference to the queue in the process info
    // Initialize the context with some space
    info->context = malloc(sizeof(list_elem_t)); //allocates mem for a new node to store the process info
    // Initialize the list element with datum pointing to the process
    //info->context->datum: is a pointer to a node in the doubly link that stores the process info
    info->context->datum = info; //new node points to the process_t info (this is the process information)
    list_elem_init(info->context, info->context->datum);  //adds a new node of that process containing the process information
    //points to a node(info that needs to be intialized), the information for that node
    // Initialize the process' CPU semaphore so it must wait for the short_term_scheduler before it can get the CPU
    sem_open(&info->cpu_sem, 0, 0); 
}

void destroy_process_info(process_t *info){
    // Destroy the context
    free(info->context->datum);
    // Destoy the semaphore
    sem_close(&info->cpu_sem);
} 

void wait_for_cpu_user(process_t *info){
    // Wait for the scheduler to signal this process
    sem_wait(&info->cpu_sem);
}

void wait_for_cpu_kernel(sched_queue_t *queue){
    // Wait for the scheduler to signal this process
    sem_wait(&queue->sched_queue_sem);
}

void release_cpu(sched_queue_t *queue){
    // Let the scheduler know the process is done using the cpu
    sem_post(&queue->cpu_sem);

}

void return_to_queue(sched_queue_t *queue, list_elem_t *elt){
    process_t *info = (process_t *) elt->datum; 
    // Add the element to the back of the list with mutual exclusion
    pthread_mutex_lock(&info->queue->lock);
    list_insert_tail(&info->queue->lst, elt);
    sem_post(&queue->sched_queue_sem);
    pthread_mutex_unlock(&info->queue->lock);
}

void enter_sched_queue(sched_queue_t *queue, process_t *info){
    // Enter the queue for the first time: 
    sem_wait(&queue->sched_queue_sem);
    pthread_mutex_lock(&queue->lock);  
    // check if the queue is not full, 
    if(list_get_head(&queue->lst) == NULL){
        list_insert_head(&queue->lst, info->context);
        pthread_mutex_unlock(&queue->lock);
        sem_post(&queue->ready_sem);    
    }else{
        pthread_mutex_unlock(&queue->lock);
    }
    // modify the queue with mutual exclusion, 
    // and signal the queue has one more ready process
}

void terminate_process(process_t *info, float completionTime, FILE *file){
    // Write the final values about the process
    info->completionTime = completionTime;
    file = fopen("completedProcesses.txt", "a");
    // fprintf(file, "Terminating process with PID %d\n", info->pid); 
    fprintf(file, "%d %d %f %f\n", info->pid, info->arrivalTime, info->serviceTime, info->completionTime);
    fclose(file);

    // record the info as: processID arrivalTime serviceTime completionTime
    // update the values of the ready_sem and sched_queue_sem to represent one less ready process in the queue and one more empty space in the queue. 
    sem_post(&info->queue->sched_queue_sem);
    sem_wait(&info->queue->ready_sem); //
}

void init_sched_queue(sched_queue_t *queue, int queue_size, float time_slice)
{

	// initialize the scheduler's semaphores, mutex, and queue
    sem_open(&queue->sched_queue_sem, 0, queue_size); //max # of processes in sched que
    sem_open(&queue->ready_sem, 0, 0); //amount of ready processes should be 0 since it is the start 
    sem_open(&queue->cpu_sem, 0, 1); //cpu sem avaiable from the beginning 

    queue->time_slice = time_slice; 

    pthread_mutex_init(&queue->lock, 0);
    list_init(&queue->lst);

}

void destroy_sched_queue(sched_queue_t *queue)
{
    // destroy semaphores and mutex
    sem_close(&queue->sched_queue_sem);
    sem_close(&queue->ready_sem);
    sem_close(&queue->cpu_sem);
    pthread_mutex_destroy(&queue->lock);
}

void signal_process(process_t *info)
{
    // signal the process that the CPU is free
    sem_post(&info->cpu_sem);
}

void wait_for_process(sched_queue_t *queue)
{
    // make the dispatcher wait until CPU is available
    sem_wait(&queue->cpu_sem);
}

void wait_for_queue(sched_queue_t *queue)
{
    // make the queue wait until there are ready processes in the queue. 
    // HINT: You have a semaphore that tells you if there are processes ready to run. Leave the value unchanged once you are done waiting.

    sem_wait(&queue->ready_sem);
    sem_post(&queue->ready_sem);
}

process_t *next_process_fifo(sched_queue_t *queue)
{
	process_t *info = NULL;
	list_elem_t *elt = NULL;

    // access queue with mutual exclusion
    pthread_mutex_lock(&queue->lock);
    // get the front element of the queue
    elt = list_get_head(&queue->lst);

    // if the element is not NULL remove the element and retrieve the process data
    // ONLY IN FIFO update the queue time_slice to allow the process to run for its entire serviceTime
    if (elt != NULL) {
        info = (process_t *) elt->datum;
        list_remove_elem(&queue->lst, elt);
        info->queue->time_slice = info->serviceTime;
    }
    pthread_mutex_unlock(&queue->lock);

    // return the process info
	return info;
}

process_t *next_process_rr(sched_queue_t *queue)
{
	process_t *info = NULL;
	list_elem_t *elt = NULL;

    // access queue with mutual exclusion
	pthread_mutex_lock(&queue->lock);
    // get the front element of the queue
	elt = list_get_head(&queue->lst);
    // if the element is not NULL remove the element and retrieve the process data
	if (elt != NULL) {
        info = (process_t *) elt->datum;
        list_remove_elem(&queue->lst, elt);
	}
    pthread_mutex_unlock(&queue->lock);
    // return the process info
	return info;
}

// Dispatcher operations
scheduler_t dispatch_fifo = {
        {init_process_info, destroy_process_info, wait_for_cpu_user, enter_sched_queue, return_to_queue, terminate_process},
        {init_sched_queue, destroy_sched_queue, signal_process, wait_for_process, next_process_fifo, wait_for_queue, wait_for_cpu_kernel, release_cpu } 
    };
scheduler_t dispatch_rr = {
        {init_process_info, destroy_process_info, wait_for_cpu_user, enter_sched_queue, return_to_queue, terminate_process},
        {init_sched_queue, destroy_sched_queue, signal_process, wait_for_process, next_process_rr, wait_for_queue, wait_for_cpu_kernel, release_cpu } 
    };

#endif