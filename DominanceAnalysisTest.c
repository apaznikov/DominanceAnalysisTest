#include <pthread.h>
#include <stdio.h>
#include <stdatomic.h> // For C11 atomics

#define TEST_SIMPLE_DOMINANCE
// #define TEST_DOMINANCE_THROUGH_IF
// #define TEST_DOMINANCE_UNCLEAR_PATH_CALL
// #define TEST_DOMINANCE_UNCLEAR_PATH_SYNC
// #define TEST_DOMINANCE_READ_AFTER_WRITE
// #define TEST_DOMINANCE_WRITE_AFTER_READ
// #define TEST_DOMINANCE_LOOP
// #define TEST_NO_DOMINANCE_PARALLEL_BRANCHES

// Common global variables that might be used by multiple tests
int g1;
int g2;
int g_common; // Renamed 'g' to 'g_common' to avoid clashes
int g_loop_test; // Renamed 'g_loop'
int g_parallel_test; // Renamed 'g_parallel'

// For Test 2 & 8
volatile int test_condition = 1;
volatile int test_condition2 = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void potentially_dangerous_call_func() {
  pthread_mutex_lock(&mutex);
  pthread_mutex_unlock(&mutex);
}

#ifdef TEST_SIMPLE_DOMINANCE
// --- Test 1: Simple dominance in a single thread (no calls between accesses) ---
void test_simple_dominance() {
  // Access 1 (should be instrumented)
  g1 = 111;

  // Some operations without function calls or synchronization
  int x_simple = 555;
  x_simple = x_simple * 2;

  // Access 2 to the same address (should be removed by dominance optimization)
  g1 = 222;

  // Access to a different address (should be instrumented)
  g2 = 333;
}
#endif

#ifdef TEST_DOMINANCE_THROUGH_IF
// --- Test 2: Dominance through a conditional operator (clear path) ---
void test_dominance_through_if() {
  // Access 1 (should be instrumented)
  g_common = 111;
  int x = 0;
  x++;
  g_common = 123;

  if (test_condition) {
    // Some operations without function calls or synchronization
    int x_if = 5;
    x_if = x_if * 2;

    // Access 2 to the same address (should be removed by dominance optimization)
    // The block with g_common = 1; dominates this block, path is clear.
    g_common = 222;
  } else {
    // Access 3 to the same address (should be removed by dominance optimization)
    // The block with g_common = 1; dominates this block, path is clear.
    g_common = 333;
  }
}
#endif

#ifdef TEST_DOMINANCE_UNCLEAR_PATH_CALL
// --- Test 3: Dominance, but path is "not clear" due to a function call ---
void test_dominance_unclear_path_call() {
  // Access 1 (should be instrumented)
  g_common = 1;

  potentially_dangerous_call_func();

  // Access 2 to the same address (should NOT be removed, as potentially_dangerous_call_func() breaks the "clear" path)
  g_common = 2;
}
#endif

#ifdef TEST_DOMINANCE_UNCLEAR_PATH_SYNC
// --- Test 4: Dominance, but path is "not clear" due to a synchronization primitive ---
void test_dominance_unclear_path_sync() {
  // Access 1 (should be instrumented)
  g_common = 111;

  // Synchronization primitive (e.g., memory fence or atomic operation with barrier semantics)
  atomic_thread_fence(memory_order_seq_cst); // C11

  // Access 2 to the same address (should NOT be removed, as the barrier breaks the "clear" path)
  g_common = 222;
}
#endif

#ifdef TEST_DOMINANCE_READ_AFTER_WRITE
// --- Test 5: Dominance, different access types (read after write) ---
void test_dominance_read_after_write() {
  // Access 1: Write (should be instrumented)
  g_common = 111;

  // Some operations without function calls or synchronization
  int x_raw = g_common + 555;
  (void)x_raw; // Suppress unused variable warning

  // Access 2: Read of the same address (should be removed by dominance optimization,
  // as the dominating write g_common=10 covers this read, and the path is clear)
  int val_raw = g_common;
}
#endif

#ifdef TEST_DOMINANCE_WRITE_AFTER_READ
// --- Test 6: Dominance, different access types (write after read - should not be removed by read's dominance) ---
void test_dominance_write_after_read() {
  g_common = 111; // Initialization

  potentially_dangerous_call_func();

  // Access 1: Read (should be instrumented)
  int val_war = g_common;
  (void)val_war;

  // potentially_dangerous_call_func();

  // Some operations
  int x_war = 555;
  (void)x_war; // Suppress unused variable warning

  // Access 2: Write to the same address (should NOT be removed by the dominating read,
  // as a read does not cover a write)
  g_common = 222;
}
#endif

#ifdef TEST_DOMINANCE_LOOP
// --- Test 7: Dominance in a loop (simple case) ---
void test_dominance_loop() {
  for (int i = 0; i < 2; ++i) {
    // Loop will execute twice
    // Access 1 in loop (on the first iteration, should be instrumented)
    // On the second iteration, this access is dominated by the access from the first iteration,
    // BUT! The end of the loop iteration (increment i, condition check) might be seen
    // as breaking the "clear" path for simple inter-iteration dominance analysis.
    // If your isPathClear is smart enough for loops, it might be removed.
    // For a basic implementation, it will likely NOT be removed on the second iteration.
    g_loop_test = i;

    // Some operations
    int x_loop = g_loop_test + 1;
    (void)x_loop; // Suppress unused variable warning

    // Access 2 in loop, same address (on each iteration, should be removed
    // by access 1 of the current iteration, if the path within the iteration is clear)
    g_loop_test = i + 10;
  }
}
#endif

#ifdef TEST_NO_DOMINANCE_PARALLEL_BRANCHES
// --- Test 8: No dominance (parallel branches) ---
void test_no_dominance_parallel_branches() {
  if (test_condition) {
    // test_condition is 1
    // Access in one branch (should be instrumented)
    g_parallel_test = 1;
  }

  // No direct dominance between these branches for g_parallel_test
  // if test_condition and test_condition2 can be different

  if (test_condition2) {
    // test_condition2 is 0
    // Access in another branch (should be instrumented, though this branch is not taken)
    g_parallel_test = 2;
  } else {
    // Access in another branch (should be instrumented)
    g_parallel_test = 3;
  }
}
#endif

/*
void *thread_function_for_tests(void *arg) {
#ifdef TEST_SIMPLE_DOMINANCE
    test_simple_dominance();
#endif

#ifdef TEST_DOMINANCE_THROUGH_IF
    test_dominance_through_if();
#endif

#ifdef TEST_DOMINANCE_UNCLEAR_PATH_CALL
    test_dominance_unclear_path_call();
#endif

#ifdef TEST_DOMINANCE_UNCLEAR_PATH_SYNC
    test_dominance_unclear_path_sync();
#endif

#ifdef TEST_DOMINANCE_READ_AFTER_WRITE
    test_dominance_read_after_write();
#endif

#ifdef TEST_DOMINANCE_WRITE_AFTER_READ
    test_dominance_write_after_read();
#endif

#ifdef TEST_DOMINANCE_LOOP
    test_dominance_loop();
#endif

#ifdef TEST_NO_DOMINANCE_PARALLEL_BRANCHES
    test_no_dominance_parallel_branches();
#endif

  return NULL;
}
*/

/*
int main(int argc, char **argv) {
  pthread_t t;
  printf("Starting TSan Dominance Tests\n");

  // --- Select Test to Run ---
  // To run a specific test, define its macro when compiling.
  // For example: gcc -DTEST_SIMPLE_DOMINANCE your_file.c -fsanitize=thread -pthread -o test_program
  // If no test is defined, it will just run an empty thread.

#if !defined(TEST_SIMPLE_DOMINANCE) && \
        !defined(TEST_DOMINANCE_THROUGH_IF) && \
        !defined(TEST_DOMINANCE_UNCLEAR_PATH_CALL) && \
        !defined(TEST_DOMINANCE_UNCLEAR_PATH_SYNC) && \
        !defined(TEST_DOMINANCE_READ_AFTER_WRITE) && \
        !defined(TEST_DOMINANCE_WRITE_AFTER_READ) && \
        !defined(TEST_DOMINANCE_LOOP) && \
        !defined(TEST_NO_DOMINANCE_PARALLEL_BRANCHES)
  printf("No specific test defined. Running an empty thread.\n");
#endif

  if (pthread_create(&t, NULL, thread_function_for_tests, NULL) != 0) {
    perror("Failed to create thread");
    return 1;
  }
  if (pthread_join(t, NULL) != 0) {
    perror("Failed to join thread");
    return 1;
  }

  printf("TSan Dominance Tests Finished\n");
  return 0;
}
*/