Project 2: Thread Scheduler
======================

INSTRUCTIONS
============

1. OVERVIEW
===========

In this project, you will write a user-mode thread scheduler. The basic purpose
of a scheduler is to multiplex use of the computer across several threads
of execution. This project deals with two different schedulers, the long term scheduler
which simply moves new processes into the scheduler queue, and the dispatcher which looks in
the scheduler queue and schedules a process from it. You will implement both, for use in a  
cooperative multi-threading system.  Along the way, you'll also learn about implementing
object-oriented constructs in low-level procedural languages like C.

This assignment consists of implementing the core functionality of the
scheduler (Step 4) and answering 5 questions (Step 5).  Code for
Step 4 goes in scheduler.c and sched_threads.c


2. THEORY OF OPERATION
======================

The program starts in schedulerSimulation.c, here we are creating 2 threads to act as
the short term scheduler (dispatcher), and another as the long term scheduler, we are also
creating an object that holds the scheduling functions such as initialize/destroy a queue for processes,
wait for a process to finish using the CPU, select a process for execution, and wait for the queue to
have processes ready for execution; as well as process functions such as initialize/destroy the process,
enter the scheduler queue, terminate the process, etc.

The long term scheduler thread job is to read a file that contains information about 
processes (arrival time and service time) and create a process object (process_t) when it arrives
then allow the process to enter the scheduler queue; the short term scheduler thread is going to 
take a process in the queue of the scheduler queue object and "execute" it. Since this is  
a simulation "execute" means the process will gain control of the cpu using a semaphore and stay 
idle for a moment then terminate and record its information in a file. 
The program ends when the last process in the queue finishes execution. 

The program as it stands right now can be compiled using make, and then it can be executed with the following commands
  ./scheduler -fifo QUEUE_SIZE 
  ./scheduler -rr QUEUE_SIZE SERVICE_TIME
the first command runs the program using a first in first out algorithm, and the second a round robin algorithm.
QUEUE_SIZE is an integer value that determines the maximum number of processes permitted in the scheduler queue at once
SERVICE_TIME is a float value that determines the maximum allowed time in the CPU per scheduling
"make test" runs some rudimentary tests that will check most of the functions you need to create. 
In many cases I cannot test for mutual exclusion so it is your responsibility to maintain mutual exclusion between threads.

3. FILE LAYOUT
==============

The project distribution consists of the following source files:

scheduler.c (implement your code here)
  Your implementation for process_ops and scheduler_ops goes here, 
  descriptions for each process is provided in the scheduler.h. 

scheduler.h
  Defines the process_ops, scheduler_ops, and scheduler structures.

sched_threads.c (implement your code here)
  Your implementation for long_term_scheduler, short_term_scheduler, 
  and process_function goes here, descriptions for each is throughout the file.

sched_threads.h
  Defines the structures process and sched_queue which contain the necessary information
  to maintain the process, and schedulers while the program is running.

schedulerSimulation.c  
  This is where the program starts, the short term and long term 
  scheduler threads are created and allowed to execute freely.

list.h
  Defines the basic operations on a bidirectional linked list data
  structure.  The elements of the list, of type list_elem_t, include
  a void *datum where you can store pointers to whatever kind of
  data you like.  You don't have to use this linked list library,
  but it will probably come in handy.

list.c
  Implements the linked list operations.

processes.txt
  Contains 500 lines describing processes using the following format:
    arrivalTime serviceTime
  For example a line that reads:
    1234 12.345
  would mean the process is arriving at global_time 1234 and needs to execute 12.345 ms

Please take a look at the source files and familiarize yourself with how
they work.  Think about how structures containing function pointers compare
to classes and virtual methods in C++.  If you'd like to learn more, read
about the virtual function table in C++.  The struct containing function
pointers technique employed in this project is also used by C GUI libraries like
GTK+ and to define the operations of loadable modules, such as file systems,
within the Linux kernel.


4. PROGRAMMING
==============

Now you're ready to implement the core of the scheduler.
For this purpose, you should only modify scheduler.c. Please see scheduler.h for the
descriptions of what functions you must implement.

    A.  Finish the definitions of the process_ops:
        1. void init_process_info (process_t *info, sched_queue_t *queue)
            Input: Reference to the working process, reference to the working queue
            Output: N/A
            Description: This function initializes the process' link to the queue, process' LL element,
            and process' CPU semaphore (the semaphore should block the process immediately when it requests to run)
        2. void destroy_process_info(process_t *info)
            Input: Reference to the working process
            Output: N/A
            Description: This function destroys the process' LL element, and CPU semaphore
        3. void wait_for_cpu_user(process_t *info)
            Input: Reference to the working process
            Output: N/A
            Description: This functions allows the process to request access to the CPU, the process should block
            immediately if the short term scheduler has not granted access to the CPU
        4.  void enter_sched_queue(sched_queue_t *queue, process_t *info)
            Input: Reference to working process
            Output: N/A
            Description: This function should first check if there is space in the scheduler queue to enter,
            if there is space then add process to the back of the scheduler queue, the queue must be mutually excluded
            once the process is part of the queue, let the schedulers know there is a new ready process with a semaphore
        5. void return_to_queue(sched_queue_t *queue, list_elem_t *elt)
            Input: Reference to queue, and reference to working process
            Output: N/A
            Description: This function simply returns the process to the back of the scheduler queue. 
            ONLY APPLICABLE FOR THE RR ALGORITHM
        6. void terminate_process(process_t *info, int completionTime, FILE *file)
            Input: Reference to the working process, the time the process completed (get this from the current 
            global_time when the process terminates), and the opened file to write the results of your process in the format:
            <processID arrivalTime serviceTime completionTime>
    B.  Finish the definition of the sched_ops:
        1. void init_sched_queue(sched_queue_t *queue, int queue_size, float time_slice)
            Input: Reference to a sched_queue structure, the size of the queue_size
            Output: N/A
            Description: This function should initialize sched_queue_sem to the size of the queue,
            ready_sem to the number of current ready processes in the queue when the simulation starts,
            cpu_sem to allow only 1 process in the CPU, 
            the lock of the queue to mutually exclude, 
            and the linked list (use the functions given in list.[hc])
        2. void destroy_sched_queue(sched_queue_t *queue)
            Input: Reference to a sched_queue structure
            Output: N/A
            Description: This function should destroy all elements of the queue.
        3. void signal_process(process_t *info)
            Input: Reference to the working process
            Output: N/A
            Description: This function will be used by the scheduler to signal a process the cpu is available
            HINT: The process will be waiting on its own cpu_sem so it must receive a signal to that 
            semaphore, later when the process finishes its time_slice the process will need to signal 
            the cpu_sem of the dispatcher
        4. void wait_for_process(sched_queue_t *queue)
            Input: Reference to a sched_queue structure
            Output: N/A
            Description: This function will be used after signal_process() by the dispatcher to wait for the process 
            to finish executing, the dispatcher will wait on its own cpu_sem.
        5. process_t *next_process_fifo(sched_queue_t *queue)
            Input: Reference to a sched_queue structure
            Output: Reference to the front process of the queue
            Description: Implement an algorithm that removes and returns the head of our linked list in the queue.
            Set time_slice equal to the serviceTime of the process to execute.
        5. process_t *next_process_rr(sched_queue_t *queue)
            Input: Reference to a sched_queue structure
            Output: Reference to the front process of the queue
            Description: Implement an algorithm that removes and returns the head of our linked list in the queue.
            Don't worry about returning the process to the queue, the process will do that by itself when it finishes
            its time_slice. DO NOT CHANGE THE queue's time_slice. 
        6. void wait_for_queue(sched_queue_t *queue)
            Input: Reference to a sched_queue structure
            Output: N/A
            Description: Make the dispatcher wait until there is at least one ready process in the queue. 
            HINT: There is a semaphore that should be positive only if there is at least 1 process in the queue.
        7. void wait_for_cpu_kernel(sched_queue_t *queue)
            Input: Reference to a sched_queue structure
            Output: N/A
            Description: Make the scheduler wait until the CPU is available. This function will only be used by the 
            short_term_scheduler and long_term_scheduler it will allow only one of those schedulers to use the CPU at a time.
        8. void release_cpu(sched_queue_t *queue)
            Input: Reference to a sched_queue structure
            Output: N/A
            Description: Signal the queue's CPU semaphore that the CPU is now free.
    C.  Complete thread functions:
        1. void *process_function(void *arg)
            Input: A reference to a linked list element.
            Output: N/A
            Description: This function should control the process entering and exiting the CPU during its full lifetime
            the process will loop until it has executed for its entire serviceTime, the process will need to wait for
            the dispatcher to give it access to the cpu, once it gains access it should sleep for time_slice time or the
            remaining serviceTime whichever is smaller. Then update the global_time and either add the process to the 
            back of the queue, or terminate the process by recording its information (processID arrivalTime serviceTime completionTime) 
            in the completedProcesses file, and changing the appropiate semaphores to allow another process to enter the queue.
        2. void *short_term_scheduler(void *arg)
            Input: A reference to a sched_queue structure.
            Output: N/A
            Description: This function will first wait for a process to arrive into the queue then start scheduling processes
            while the long term scheduler is still running or there are processes in the queue. The dispatcher will try to gain control of the
            cpu using cpu_sem (this thread will compete for the cpu with the long term scheduler), once it gets control use the dispatcher functions
            to get the front process in the queue, signal the process so it gets control of the cpu, then wait for the process to end using the cpu.
            If there is no process in the queue, wait for one to arrive.
        3. void *long_term_scheduler(void *arg)
            Input: A reference to a sched_queue structure.
            Output: N/A
            Description: This function only job is to read the file processes.txt, retrieve the process arrival time, and service time in that order;
            then create a process object and assign a unique pid, the arrivalTime, the serviceTime, the linked list item that will be stored in the 
            queue, the reference to the queue that will have this process, and initialize the cpu_sem so the process cannot access the cpu 
            immediately and must wait for the dispatcher to activate it. Create the process thread using the process function and insert the 
            process into the back of the queue (linked list).
    C. Analyze the data:
        Run 10 simulations of the fifo algorithm with QUEUE_SIZE 5, 10, 100; 10 simulations of the rr algorithm with QUEUE_SIZE 5, 10, 100, and TIME_SLICE 2, 4 and 40; 
        for each simulation get the average turn around time, and average wait time of every process, create 2 line graphs 
        where the X-axis is the type of simulation and the Y-axis is the average TAT or average WT. You may take any approach you want to do this
        creative solutions will receive EXTRA CREDIT. You can create a new function to read the completionTime files after they are generated 
        and calculate the TAT and WT of every process, then get the average TAT and WT for the simulation. If you want, you can change the format
        of the completedProcesses file to have more information or read it with other programs i.e. csv, xml, json, etc.

Concurrency problems:

 - There will be 2 threads adding and removing processes from a common queue, as well
 as a bunch of new processes/threads so it needs to be strictly mutually excluded so 
 only 1 of those many threads can actually modify the queue (either to add or remove a process).

 - There is only 1 cpu in the system, scheduler threads only need to find the CPU free, while process threads need to wait 
 for the short_term_scheduler to let them in so you have to make sure the process is not going to "execute" at the same time 
 as one of the schedulers. Use the cpu_sem in the process to block each individual process in its own queue, and await a 
 signal from the short_term_scheduler; use the cpu_sem in the scheduler sched_queue to either allow one of the schedulers 
 to execute or wait for a process to finish its execution. How would this work? The short_term_scheduler may use the process 
 context to access its semaphore and signal it, then wait on its own cpu_sem to block the long term scheduler and wait 
 for the process to signal the short_term_scheduler back when its time_slice expires.

- The size of the scheduler queue is limited so use sched_queue_sem to control the maximum (allowed) number of 
processes in the queue at the same time, and use ready_sem to keep track of the number of processes/threads
currently loaded in the queue. HINT: Think of producers/consumers problem.


5. QUESTIONS
============

Q 1  What are some pros and cons of using the struct of function pointers
     approach as we did in the project to link different modules?  Does it
     significantly affect performance?  Give some examples of when you would
     and wouldn't use this approach, and why.

Q 2  Briefly describe the synchronization constructs you needed to implement
     this project--i.e., how you mediated admission of threads to the scheduler
     queue and how you made sure only the scheduled thread would run at any
     given time.

Q 3  Does it matter whether the call to sched_ops->wait_for_queue(queue) in
     short_term_scheduler() actually does anything?  How would it affect correctness
     if it just returned right away?  How about performance?

Q 4  Why is there variables in scheduler.h declared 'extern'?  What
     would happen if it was not declared 'extern'?  What would happen
     if they were not declared without the 'extern' in any file?

Q 5  Explain how you would alter the program to demonstrate the "convoy"
     effect, when a large compute bound job that never yields to another
     thread slows down all other jobs in a FIFO scheduled system? See Page
     402, Stallings, the paragraph starting "Another difficulty with FCFS is
     that it tends to favor processor-bound processes over I/O bound
     processes".  Why is it difficult to show the benefits of Round Robin
     scheduling in this case using the current implementation in the project?



6. SUBMISSION
===============
1. Complete all parts/questions of Step 4 and 5 and have the most recent version of the program on
    GitHub
2. Submit in BB a picture of your program running make test in Codespaces.
3. Submit in BB a graph of the Turn Around Times of every process. Like the one shown in BB.
4. Submit in BB a graph of the Wait Times of every process. Like the one shown in BB.
5. The automatic tests check for general organization, but they do not ensure the program works. 
  Instead you can test for correctness by:
  1. Making sure a process may not execute if a scheduler is running.
  2. Comparing your Graphs with mine (I used time_slice = 2). They are not going to be equal, but
      you can tell when the program is incorrect, if it differs by a lot of every process.
  3. Test on a smaller processes.txt file, you can remove ~490 processes from the file and Test
      with only 10 or so. You can manually calculate what should be their turn around and wait times.

      
