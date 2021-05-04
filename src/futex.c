#include <unistd.h>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <stdatomic.h>

static int sysfutex(int *uaddr, int futex_op, int val, const struct timespec *timeout, int *uaddr2,
					int val3)
{
	return syscall(SYS_futex, uaddr, futex_op, val, timeout, uaddr2, val3);
}

int futex_wait(_Atomic int *uaddr, const struct timespec *timeout)
{
	return sysfutex((int *)uaddr, FUTEX_WAIT | FUTEX_PRIVATE_FLAG, 1, timeout, NULL, 0);
}

int futex_wake(_Atomic int *uaddr)
{
	return sysfutex((int *)uaddr, FUTEX_WAKE | FUTEX_PRIVATE_FLAG, 1, NULL, NULL, 0);
}
