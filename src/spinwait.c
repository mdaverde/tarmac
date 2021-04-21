
#include <stdbool.h>
#include "./thread.c"

void cpu_spin(volatile int iterations)
{
	int i = 0;
	while (i < iterations) {
		i++;
	}
}

struct spin_wait {
	int counter; // can be uint?
};

struct spin_wait sw_init()
{
	struct spin_wait sw = {
		.counter = 0,
	};
	return sw;
}

void sw_reset(struct spin_wait *sw)
{
	sw->counter = 0;
}

bool sw_spin(struct spin_wait *sw)
{
	if (sw->counter >= 10) {
		return false;
	}
	sw->counter += 1;
	if (sw->counter <= 3) {
		cpu_spin(1 << sw->counter);
	} else {
		thread_yield();
	}
	return true;
}

void sw_spin_no_yield(struct spin_wait *sw)
{
	sw->counter += 1;
	if (sw->counter > 10) {
		sw->counter = 10;
	}
	cpu_spin(1 << sw->counter);
}