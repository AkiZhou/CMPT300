#ifndef	__SCHED_IMPL__H__
#define	__SCHED_IMPL__H__

#include <semaphore.h>
#include "list.h"
#include <stdlib.h>
#include <stdio.h>

struct thread_info {
	sched_queue_t *queue;  // points to the queue
	list_elem_t *list_element;  // points to the element container for LL
	sem_t exec;
};

struct sched_queue {
	list_t * q;  // actual queue

	sem_t queue_sem;  // semaphore for processor
	sem_t cpu_sem;  // semaphore for queue

	int currentProcess;  //  indicates the running process for rr
};

#endif /* __SCHED_IMPL__H__ */
