
#ifndef __SCHED__TESTS__
#define __SCHED__TESTS__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <semaphore.h>
#include <pthread.h>

#include "scheduler.h"
#include "sched_threads.h"
#include "list.h"

int testValue;
int semValue;
extern const int QUEUE_SIZE;

// Function prototypes
void *test_wait_for_queue(void *arg);
void *process_function(void *arg);
void *short_term_scheduler(void *arg);
void *long_term_scheduler(void *arg);

// Test prototypes
void init_queue_test(sched_queue_t *queue, char *argv[]);
void signal_process_test(sched_queue_t *queue, char *argv[]);
void wait_for_process_test(sched_queue_t *queue, char *argv[]);
void next_process_test(sched_queue_t *queue, char *argv[]);
void wait_for_queue_test(sched_queue_t *queue, char *argv[]);
void init_process_test(sched_queue_t *queue);
void destroy_process_test();
void terminate_process_test(sched_queue_t *queue);
void return_to_sched_queue_test(sched_queue_t *queue, list_elem_t *elt);

void tests(sched_queue_t *queue, char *argv[]){
    
    // Tests inital queue values
    init_queue_test(queue, argv);       // 5 pts

    // Tests initial process values
    init_process_test(queue);           // 5 pts

    // Tests the correct termination of the process
    terminate_process_test(queue);      // 5 pts
    
    // Test for signal_process 
    signal_process_test(queue, argv);   // 5 pts

    // Test for wait_for_process
    wait_for_process_test(queue, argv); // 5 pts

    // Test for wait_for_queue
    wait_for_queue_test(queue, argv);   // 5 pts

    // Test for next_process
    //next_process_test(queue, argv);   // 20 pts

    // If all passes this will print
    fprintf(stdout, "Congratulations! you passed all my tests, this doesn't mean the program is 100 percent correct but it means you are doing good\nyou need to manually check if the long_term_scheduler is adding processes to the queue when they arrive, waiting for processes to arrive, and terminating properly at the end of the file\nalso, the long_term_scheduler and short_term_scheduler functions are accessing the queue mutually excluded\n\n");

    // long_term_scheduler // 20 pts
    // short_term_scheduler // 20 pts
}

void *test_wait_for_queue(void *arg){
	sched_queue_t *queue = (sched_queue_t*)arg;
	sleep(5);
	testValue = 1;
	sem_post(&queue->ready_sem);	
	pthread_exit(0);
}


void init_queue_test(sched_queue_t *queue, char *argv[]){
    // list test
    assert(list_size(&queue->lst) == 0 && "Linked List cannot be NULL");

    // queue cpu_sem test
    sem_getvalue(&queue->cpu_sem, &semValue);
    assert(semValue == 1 && "Schedulers should have access to CPU");

    // ready_sem test
    sem_getvalue(&queue->ready_sem, &semValue);
    assert(semValue == 0 && "There are no processes in the queue initially");

    // sched_queue_sem test
    sem_getvalue(&queue->sched_queue_sem, &semValue);
    assert(semValue == atoi(argv[2]) && "There should be QUEUE_SIZE spaces available in the LL");

    // mutex test
    assert(pthread_mutex_lock(&queue->lock) == 0 && "No one is using the queue yet, it should be unlocked");
    pthread_mutex_unlock(&queue->lock);

    // time_slice test
    assert(queue->time_slice == atof(argv[3]) && "The queue assigned time_slice should be given by the user");

    // global time starts at 0
    assert(queue->global_time == 0);

    fprintf(stdout, "Init queue \t\t*pass*\n");
}

void signal_process_test(sched_queue_t *queue, char *argv[]){
    process_t *testprocess = (process_t*) malloc(sizeof(process_t));
    sem_init(&testprocess->cpu_sem, 0, 0);
    queue->ops.sched_ops.signal_process(testprocess);
    sem_getvalue(&testprocess->cpu_sem, &semValue);
    assert(semValue == 1 && "The scheduler should signal the process cpu semaphore");
    fprintf(stdout, "Signal process \t\t*pass*\n");
}


void wait_for_process_test(sched_queue_t *queue, char *argv[]){
    queue->ops.sched_ops.wait_for_process(queue);
    sem_getvalue(&queue->cpu_sem, &semValue);
    assert(semValue == 0 && "The scheduler should wait on its own cpu semaphore");
    sem_post(&queue->cpu_sem);
    fprintf(stdout, "Wait for process \t*pass*\n");
}


void wait_for_queue_test(sched_queue_t *queue, char *argv[]){
    testValue = 0;
    pthread_t wait_for_queue_thread;
    pthread_create(&wait_for_queue_thread, 0, test_wait_for_queue, (void*)queue);
    queue->ops.sched_ops.wait_for_queue(queue);
    assert(testValue == 1 && "The scheduler needs to wait for a single process to enter the queue if this is failing your scheduler does not actually stop for a process");
    sem_getvalue(&queue->ready_sem, &semValue);
    assert(semValue == 1 && "The scheduler should wait, but not modify permanently the value of the number of processes in the queue");
    sem_wait(&queue->ready_sem);
    fprintf(stdout, "Wait for queue \t\t*pass*\n");
}

void next_process_test(sched_queue_t *queue, char *argv[]){
    // QUEUE_SIZE
    int QUEUE_SIZE = atoi(argv[2]);

    pthread_t test_threads[QUEUE_SIZE];

    // Populate the queue with dummy processes
    for (int i = 0; i < QUEUE_SIZE; i++){
           process_t *new_process = (process_t*) malloc(sizeof(process_t));
           new_process->serviceTime = 10;
           new_process->pid = i;
           queue->ops.process_ops.init_process_info(new_process, queue);
           pthread_create(&test_threads[i], NULL, process_function, (void *)new_process->context);
           pthread_detach(test_threads[i]); // detach to let it run independently
    }
    
    // Wait for queue to be full
    do{
        sem_getvalue(&queue->sched_queue_sem, &semValue);
    }while(semValue != 0);

    // The tests below will check the process_function and the next_process functions, and whether they are able to run and terminate correctly in either fifo or rr.
    if(!strcmp(argv[1], "-fifo")){
        process_t *info;
        
        for (int i = 0; i < QUEUE_SIZE; i++){
                queue->ops.sched_ops.wait_for_cpu(queue);
                info = queue->ops.sched_ops.next_process(queue);
                fprintf(stdout, "Start execution of process %d\n", info->pid);
                queue->ops.sched_ops.signal_process(info);
                queue->ops.sched_ops.wait_for_process(queue);
                queue->ops.sched_ops.release_cpu(queue);
                assert(list_size(&queue->lst) == QUEUE_SIZE-1-i && "Are you getting the front of the queue? Are you removing the node when you do?");
        }
	fprintf(stdout, "Process_function and next_process_fifo \t\t*pass*\n");
    }
    else if(!strcmp(argv[1], "-rr")){
        process_t *info;
        
        for(int j = 0; j < 3; j++){
                for (int i = 0; i < QUEUE_SIZE; i++){
                        queue->ops.sched_ops.wait_for_cpu(queue);
                        info = queue->ops.sched_ops.next_process(queue);
                        fprintf(stdout, "Start execution of process %d\n", info->pid);
                        queue->ops.sched_ops.signal_process(info);
                        queue->ops.sched_ops.wait_for_process(queue);
                        queue->ops.sched_ops.release_cpu(queue);
                        assert(list_size(&queue->lst) == QUEUE_SIZE-1-i && "Are you getting the front of the queue? Is the process adding itself at the end of the queue?");
                }
        }
	fprintf(stdout, "Process_function and next_process_rr \t\t*pass*\n");
    }
}

void init_process_test(sched_queue_t *queue){
    process_t *myProcess = (process_t*)malloc(sizeof(process_t));
    queue->ops.process_ops.init_process_info(myProcess, queue);

     // process cpu_sem test
    sem_getvalue(&myProcess->cpu_sem, &semValue);
    assert(semValue == 0 && "Processes must wait for the short_term_schedulers to call on them before they can access the CPU.");

    // process queue test
    assert(myProcess->queue == queue && "The process must have a pointer to the queue it belongs to.");

    // process context
    assert(myProcess->context != NULL && "The process new context is NULL");

    // process list element
    assert(myProcess->context->datum == myProcess && "The context datum holds the reference to the process itself");

    fprintf(stdout, "Init process \t\t*pass*\n");
}

void terminate_process_test(sched_queue_t *queue){
    //QUEUE_SIZE
    int QUEUE_SIZE;
    sem_getvalue(&queue->sched_queue_sem, &QUEUE_SIZE);

    // Create test process with test data
    FILE *testTerminate = fopen("test.out", "w");
    process_t *myProcess = (process_t*)malloc(sizeof(process_t));
    queue->ops.process_ops.init_process_info(myProcess, queue);
    myProcess->arrivalTime = 10;
    myProcess->serviceTime = 20;
    myProcess->pid = 1000;
    // "Add" process to queue
    sem_wait(&myProcess->queue->sched_queue_sem);
    sem_post(&myProcess->queue->ready_sem);
    queue->ops.process_ops.terminate_process(myProcess, 0, testTerminate);
    fclose(testTerminate);

    // Check correct values: 
    testTerminate = fopen("test.out", "r");
    char *line = (char*)malloc(100);
    fgets(line, 100, testTerminate);

    assert(!strcmp(line,"1000 10 20.000000 0.000000\n") && "Save your process data in the following format 'processID arrivalTime serviceTime completionTime'");
    sem_getvalue(&myProcess->queue->ready_sem, &semValue);
    assert(semValue == 0 && "Number of ready processes decrease when a process terminates");
    sem_getvalue(&myProcess->queue->sched_queue_sem, &semValue);
    assert(semValue == QUEUE_SIZE && "Number of available locations in the queue increases when a process terminates");
    fclose(testTerminate);
    remove("test.w");

    fprintf(stdout, "Terminate process \t*pass*\n");
}


#endif