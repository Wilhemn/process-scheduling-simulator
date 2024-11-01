#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>

#include "scheduler.h"
#include "sched_threads.h"

// Some global variables
static const int MAX_LINE_LENGTH = 255;
static bool longTermRunning = false; // This variable is used to keep the short_term_scheduler running while the long_term_scheduler is running.
FILE *completedProcesses; // This file is going to be opened by the long_term_scheduler and then used by the process_function to write the final values of the process

void *process_function(void *arg){
    // get the element and process info from the arg anything else you need can be accessed starting from one of these locations
    list_elem_t *elt = (list_elem_t*) arg;
    process_t *info = (process_t *) elt->datum;
    process_ops_t *proc_ops = &info->queue->ops.process_ops;
    sched_ops_t *sched_ops = &info->queue->ops.sched_ops;

    // TODO try to enter scheduler queue
    //proc_ops->enter_sched_queue(info->queue, info)
    proc_ops->enter_sched_queue(info->queue, info->context->datum);
    //sem_wait on sched_que sem
        //lock mutex
            //add process to the back of the list
        //unlock mutex
    //sem_post the ready_sem
    
    // so you do not modify the service time in the process image, you will need that information
    float serviceTime = info->serviceTime; 

    while(serviceTime > 0){
        // TODO request access to CPU using process semaphore
        //sem_wait on the cpu_sem of the process
        // sem_wait(&info->cpu_sem);
        // proc_ops->wait_for_cpu(info);

        // time slice alloted to process
        float time_slice = info->queue->time_slice;
        
        // if the process will not finish in this time slice then increment global time equal to time slice otherwise increment by the remaining service time
        // decrease process service time by time slice
        if(serviceTime > time_slice){
            // Process does some useless work DONT MODIFY THIS 
            sleep(time_slice/1000.0);
            //////////////////////
            // TODO update global and service time
            info->queue->global_time += time_slice;
            serviceTime -= time_slice;
        }
        else{
            // Process does some useless work DONT MODIFY THIS
            sleep(serviceTime/1000.0);
            //////////////////////
            // TODO update global and service time
            info->queue->global_time += serviceTime;
            serviceTime = 0;
        }
    
        fprintf(stdout, "Finished time slice of process %d, time remaining = %f\n", info->pid, serviceTime);
        // if serviceTime is not 0 insert the process to the back of the list (only applicable when RR is the scheduling algorithm) DONT FORGET ABOUT MUTUAL EXCLUSION!
        if(serviceTime > 0 ){
            fprintf(stdout, "Inserting process %d to back of the queue\n", info->pid);
            // TODO return process to queue
            proc_ops->return_to_queue(info->queue, elt);
        }
        // else record the info of the process in the completionTimes file, and signal a new empty slot in the queue
        else{
            fprintf(stdout, "Teminating process %d\n", info->pid);
            // TODO terminate the process
            // info->completionTime = info->queue->global_time; 
            proc_ops->terminate_process(info->context->datum, info->queue->global_time, completedProcesses);

        }
        // TODO signal queue the time slice is complete
        // sched_ops->release_cpu();
        sched_ops->release_cpu(info->queue);
    }
    // TODO Remove process image from memory
    // proc_ops->destroy_process_info();
    proc_ops->destroy_process_info(info->context->datum);

    // Terminate the thread controlling the process
    pthread_exit(0);
}

void *short_term_scheduler(void *arg){
    sched_queue_t *queue  = (sched_queue_t*) arg;
    sched_ops_t *sched_ops = &queue->ops.sched_ops;

    // TODO wait for queue to have processes
    sched_ops->wait_for_queue(queue); // queue -> p1
                                        //sched_queue_sem = SIZE_QUEUE = -1
                                        //ready_sem = 0 -> -1 -> 0 (when added) should be 1
                                        //if queue is empty
    // sched_ops->release_cpu(queue);
    // start scheduling processes let the dispatcher run until the long-term exits and the queue is empty
    while(longTermRunning || list_size(&queue->lst)){   
        // TODO wait for cpu to be available
        //sem_wait queue cpu_sem
        // sched_ops->wait_for_cpu(queue);
        sched_ops->wait_for_process(queue);

        // TODO choose one process from the queue list
        process_t *p = sched_ops->next_process(queue);

        // TODO If there is at least one process in the queue, execute it for time_slice amount
        if(p != NULL){
            fprintf(stdout, "Start execution of process %d\n", p->pid);
            // TODO activate the process
            sched_ops->signal_process(p);

            // TODO wait for process to finish using CPU
            sched_ops->wait_for_process(queue);
            sched_ops->release_cpu(queue);
        }
        // TODO else wait for 1 to arrive to the queue, don't forget to release the cpu
        else{
            sched_ops->wait_for_queue(queue);
            sched_ops->release_cpu(queue);
        }
        // TODO release cpu
        sched_ops->release_cpu(queue);
              
    }

    // exit thread
    pthread_exit(0);
}

void *long_term_scheduler(void *arg){
    sched_queue_t *queue  = (sched_queue_t*) arg;
    sched_ops_t *sched_ops = &queue->ops.sched_ops;
    process_ops_t *proc_ops = &queue->ops.process_ops;

    // variables to read the processes from the file
    int arrival_time;
    float service_time;
    char *process_info = (char *)malloc(MAX_LINE_LENGTH);
    int pid = 1000;
    FILE *process_list = fopen("processes.txt", "r");
    
    // check if the file opened successfullym if not terminate the thread
    if(!process_list){
        fprintf(stderr, "File not found!");
        // terminate thread
        pthread_exit(0);
    }
    else {
        // use longTermRunning variable to let the dispatcher know the long term scheduler is running
        longTermRunning = true;
    }

    // open the completionTimes file for writting so processes can write their information there
    completedProcesses = fopen("completedProcesses.txt", "w");

    while(fgets(process_info, MAX_LINE_LENGTH, process_list)){
        // TODO request access to the cpu
        sched_ops->wait_for_process(queue);
        // TODO read process information from file and parse arrival time, service time
        char *token = strtok(process_info, " \n");
        while(token != NULL){
            arrival_time = atoi(token);  //arrival time to int
            token = strtok(NULL, " \n");
            service_time = atof(token); //service time to float
            token = strtok(NULL, " \n"); // move to next token
        }
        //arrival_time = 100, global_time = 90
    
        while(arrival_time > queue->global_time){
            fprintf(stdout, "Waiting for process %d to arrive at %d, currently time %f\n", pid, arrival_time, queue->global_time);

            // TODO Implement a spin wait for when the process hasn't arrived
            // yield until the process arrives, don't forget to release the cpu before you yield and get it again after yeilding.
            //release cpu (abstract)
            //sched_yield() (actual cpu)
            //request cpu
            sched_ops->release_cpu(queue);
            sched_yield();
            sched_ops->wait_for_process(queue);
            // pass some time to allow process to arrive
            queue->global_time += 1;
        }
        fprintf(stdout, "New process %d arrived at %d, currently time %f\n", pid, arrival_time, queue->global_time);        
        
        // when process arrives create the process address space
        pthread_t process_thread;
        process_t *p = (process_t*)malloc(sizeof(process_t));
        p->pid = pid++;
        p->serviceTime = service_time;
        p->arrivalTime = arrival_time;

        // TODO initialize the the process image
        proc_ops->init_process_info(p, queue);
 
        // Create a thread to control the process execution
        pthread_create(&process_thread, NULL, process_function, (void*)p->context);
        pthread_detach(process_thread); // detach to let it run independently

        // TODO release cpu
        sched_ops->release_cpu(queue);

        // get ready to read next line
        process_info = (char *)malloc(MAX_LINE_LENGTH);

    }
    // let the dispatcher know the list of processes ended, what does the short_term_scheduler is constantly looking at in its loop?
    longTermRunning = false;

    fclose(completedProcesses);
    fclose(process_list);
    // free(process_info);
    // exit thread
    pthread_exit(0);
}