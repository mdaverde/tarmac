#include <assert.h>
#include <stdio.h>
#include <pthread.h>
#include "./src/word_lock.c"

#define TEST_ERROR -1
#define TEST_SUCCESS 0

#define assert_test(test_name, test_result) \
	int saved_result = test_result; \
	if (saved_result == TEST_SUCCESS) { \
		printf("%s - success\n", test_name); \
	} else if (saved_result == TEST_ERROR) { \
		printf("%s - error\n", test_name); \
	} else { \
		printf("%s - did not understand test response\n", test_name); \
	}  \


void *thread_func() {
	printf("hello from thread\n");
	return NULL;
}

int test_thread_parker() {
	int num_threads = 3;
	pthread_t threads[num_threads];

	for (int i = 0; i < num_threads; i++) {
		pthread_create(&threads[i], NULL, thread_func, NULL);
	}

	for (int i = 0; i < num_threads; i++) {
		pthread_join(threads[i], NULL);
	}

	return TEST_SUCCESS;
}

int main() {
	assert_test("thread_parker", test_thread_parker());
}