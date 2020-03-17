#include "scheduler.h"
#include "sched_impl.h"

////////// Thread operations

/**
 * This function initializes thread_info_t.
 */
static void init_thread_info(thread_info_t *info, sched_queue_t *queue) {
	info->queue = queue;  // save the location of the queue for later access

	// initialize an element to be LL
	list_elem_t *n = (list_elem_t*) malloc(sizeof(list_elem_t));
	if (n == NULL)
	{
		perror("ERROR: failed to malloc thread info");
		return;
	}
	list_elem_init(n, info);  // init with thread info
	info->list_element = n;  // point to this element for later access

	sem_init(&(info->exec), 0, 0);  // initialize the thread's semaphore, block by default
}

/**
 * This function destroys a thread info and releases the resources associated with a thread_info_t
 */
static void destroy_thread_info(thread_info_t *info) {
	info->queue = NULL;
	free(info->list_element);
	info->list_element = NULL;
	sem_destroy(&(info->exec));
}

/**
 * This function pushes a thread into the scheduler queue. When the queue is full, it will block until it has room again
 */
static void enter_sched_queue(thread_info_t *info) {
	// check if the queue has space by checking the semaphore, block until it has space
	sem_wait(&(info->queue->queue_sem));

	// push to the queue only when it has space
	list_insert_tail(info->queue->q, info->list_element);
}

/**
 * This function gets called when the worker has completed. It should remove the thread from the scheduler queue
 */
static void leave_sched_queue(thread_info_t *info) {
	// pop from the queue
	list_remove_elem(info->queue->q, info->list_element);

	// update element count in queue, notify any threads waiting to get into the queue
	sem_post(&(info->queue->queue_sem));

	info->queue->currentProcess--;
}

/**
 * This is a function will block until the scheduler allows it to run
 * While on the scheduler queue, block until thread is scheduled
 */
static void wait_for_cpu(thread_info_t * info) {
	// check if the processor is available, if not block on semaphore
	sem_wait(&(info->exec));
}

/**
 * This will be called when the thread is ready to release the CPU
 * Voluntarily relinquish the CPU when this thread's timeslice is over (cooperative multithreading)
 */
static void release_cpu(thread_info_t * info) {
	// post the CPU semaphore, wake the scheduler thread.
	sem_post(&(info->queue->cpu_sem));
}

////////// Scheduler operations

/**
 * This function will initialize a scheduler queue
 */
static void init_sched_queue(sched_queue_t *queue, int queue_size) {
	// initialize the queue
	if (queue == NULL) {
		queue = (sched_queue_t *) malloc(sizeof(sched_queue_t));
	}
	queue->q = (list_t *) malloc(sizeof(list_t));
	if (queue->q == NULL) {
		perror("Error: failed to initialize queue");
		return;
	}
	list_init(queue->q);

	// Set current position to be first slot in queue.
	queue->currentProcess = 0;

	// Initialize the semaphores
	sem_init(&(queue->cpu_sem), 0, 0);
	sem_init(&(queue->queue_sem), 0, queue_size);
}

/**
 * This function destroies a scheduler queue
 * Release the resources associated with a sched_queue_t
 */
static void destroy_sched_queue(sched_queue_t *queue) {
	if (queue != NULL) {
		list_foreach(queue->q, (void *) free);  // free potential remaining elements
		free(queue->q);
		queue = NULL;
	}

	sem_destroy(&(queue->cpu_sem));
	sem_destroy(&(queue->queue_sem));
}

/**
 * This function signals a worker thread to execute
 */
static void wake_up_worker(thread_info_t *info) {
	sem_post(&(info->exec));
}

/**
 * This function blocks until the current worker thread relinquishes the processor
 */
static void wait_for_worker(sched_queue_t *queue) {
	sem_wait(&(queue->cpu_sem));  // block until the CPU semaphore is clear
}

/**
 * Select the next worker thread to execute. Returns NULL if the scheduler queue is empty.
 */
thread_info_t * fifo_next_worker(sched_queue_t *queue)
{
	// Since we are FIFO, we just return the first element in the queue.
	list_elem_t *t = list_get_head(queue->q);
	if (t == NULL)
		return NULL;
	else {
		thread_info_t *ti = (thread_info_t*) (t->datum);
		return ti;
	}
}

/**
 * This function selects the next worker thread, returns NULL if empty
 */
thread_info_t * rr_next_worker(sched_queue_t *queue) {
	// Check if empty
	if (list_size(queue->q) == 0) {
		return NULL;
	}
	// make sure current process doesn't exceed length of queue
	if (queue->currentProcess == list_size(queue->q)) {
		queue->currentProcess = 0;  // start over since exceeded
	}

	// traverse the list until the right element
	list_elem_t *t = list_get_head(queue->q);
	int i;
	for (i = 0; i < queue->currentProcess; i++) {
		if (t == NULL)
			t = list_get_head(queue->q);
		else
			t = t->next;
	}

	thread_info_t *threadInfo = (thread_info_t*) (t->datum);
	queue->currentProcess++;

	return threadInfo;
}

/**
 * This function block until at least one worker thread is in the scheduler queue
 */
static void wait_for_queue(sched_queue_t *queue) {
	int numInQueue = list_size(queue->q);
	while (numInQueue == 0) {
		numInQueue = list_size(queue->q);
	}
}

////////// END functions


////////// Static allocation
/* These are the functions that will be called when we are using a FIFO scheduling method */
sched_impl_t sched_fifo = {
		{ init_thread_info, destroy_thread_info, enter_sched_queue, leave_sched_queue, wait_for_cpu, release_cpu },
		{ init_sched_queue, destroy_sched_queue, wake_up_worker, wait_for_worker, fifo_next_worker, wait_for_queue }
};

/* These are the functions that will be called when we are using a round robin scheduling method */
sched_impl_t sched_rr = {
		{ init_thread_info, destroy_thread_info, enter_sched_queue, leave_sched_queue, wait_for_cpu, release_cpu },
		{ init_sched_queue, destroy_sched_queue, wake_up_worker, wait_for_worker, rr_next_worker, wait_for_queue }
};
