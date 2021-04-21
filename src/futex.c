#include <unistd.h>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <stdatomic.h>

int sysfutex(int *uaddr, int futex_op, int val, const struct timespec *timeout,
	  int *uaddr2, int val3)
{
	return syscall(SYS_futex, uaddr, futex_op, val, timeout, uaddr2, val3);
}

int generic_futex_wait(int *uaddr, int val_check, const struct timespec *timeout)
{
	return sysfutex(uaddr, FUTEX_WAIT, val_check, timeout, NULL, 0);
}

int futex_wait(_Atomic int *uaddr, const struct timespec *timeout) {
	return generic_futex_wait((int *)uaddr, 1, timeout);
}

int futex_wake(_Atomic int *uaddr)
{
	return sysfutex((int *)uaddr, FUTEX_WAIT, 1, NULL, NULL, 0);
}
