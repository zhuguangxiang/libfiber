#include "fiber_semaphore.h"
#include "fiber_manager.h"
#include "test_helper.h"

int volatile counter = 0;
fiber_semaphore_t semaphore;
#define PER_FIBER_COUNT 100000
#define NUM_FIBERS 100
#define NUM_THREADS 4
#define SEMAPHORE_VALUE (NUM_THREADS * 2)
volatile int old_values[SEMAPHORE_VALUE] = {};
volatile int new_values[SEMAPHORE_VALUE] = {};

void* run_function(void* param)
{
    int i;
    for(i = 0; i < PER_FIBER_COUNT; ++i) {
        fiber_semaphore_wait(&semaphore);
        const int new_after_grab = __sync_add_and_fetch(&counter, 1);
        test_assert(new_after_grab <= SEMAPHORE_VALUE);
        __sync_add_and_fetch(&old_values[new_after_grab - 1], 1);
        fiber_yield();
        const int new_after_release = __sync_sub_and_fetch(&counter, 1);
        test_assert(new_after_release >= 0);
        __sync_sub_and_fetch(&new_values[new_after_release], 1);
        fiber_semaphore_post(&semaphore);
    }
    return NULL;
}

int main()
{
    fiber_manager_init(NUM_THREADS);

    fiber_semaphore_init(&semaphore, SEMAPHORE_VALUE);

    fiber_t* fibers[NUM_FIBERS];
    int i;
    for(i = 0; i < NUM_FIBERS; ++i) {
        fibers[i] = fiber_create(20000, &run_function, NULL);
    }

    for(i = 0; i < NUM_FIBERS; ++i) {
        fiber_join(fibers[i], NULL);
    }

    test_assert(counter == 0);
    for(i = 0; i < SEMAPHORE_VALUE; ++i) {
        printf("old_values[%d] = %d\n", i, old_values[i]);
        printf("new_values[%d] = %d\n", i, new_values[i]);
        test_assert(old_values[i] > 0);
        test_assert(new_values[i] > 0);
        test_assert(fiber_semaphore_getvalue(&semaphore) == SEMAPHORE_VALUE - i);
        test_assert(fiber_semaphore_trywait(&semaphore));
        test_assert(fiber_semaphore_getvalue(&semaphore) == SEMAPHORE_VALUE - i - 1);
    }
    test_assert(!fiber_semaphore_trywait(&semaphore));
    for(i = 0; i < SEMAPHORE_VALUE; ++i) {
        test_assert(fiber_semaphore_getvalue(&semaphore) == i);
        test_assert(fiber_semaphore_post(&semaphore));
        test_assert(fiber_semaphore_getvalue(&semaphore) == i + 1);
    }
    fiber_semaphore_destroy(&semaphore);

    return 0;
}

