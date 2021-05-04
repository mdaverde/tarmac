#include <sched.h>
#include <stdio.h>

void thread_yield()
{
	if (sched_yield() == -1) {
		perror("thread_yield sched_yield"); // TODO: handle error
	}
}
