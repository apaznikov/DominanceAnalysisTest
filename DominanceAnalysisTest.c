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

// For Test 3
// "Dangerous" function (or a function TSan cannot prove safe)
void potentially_dangerous_call_func() {
  // This function might contain synchronization or be opaque to analysis.
  // For simplicity of the test, it's empty, but TSan should consider it dangerous.
  printf("Call made to potentially_dangerous_call_func\n");
}

// For Test 4
atomic_int_least32_t test_sync_var = 0;


void *thread_function_for_tests(void *arg) {

#ifdef TEST_SIMPLE_DOMINANCE
    // --- Test 1: Simple dominance in a single thread (no calls between accesses) ---
    printf("--- Running TEST_SIMPLE_DOMINANCE ---\n");
    // Access 1 (should be instrumented)
    g1 = 111;
    // printf("g1_access1 = %d\n", g1);

    // Some operations without function calls or synchronization
    int x_simple = 555;
    x_simple = x_simple * 2;

    // Access 2 to the same address (should be removed by dominance optimization)
    g1 = 222;
    // printf("g1_access2 = %d\n", g1);

    // Access to a different address (should be instrumented)
    g2 = 333;
    // printf("g2_access = %d\n", g2);
    // printf("--- Finished TEST_SIMPLE_DOMINANCE ---\n\n");
#endif

#ifdef TEST_DOMINANCE_THROUGH_IF
  // --- Test 2: Dominance through a conditional operator (clear path) ---
  printf("--- Running TEST_DOMINANCE_THROUGH_IF ---\n");
  // Access 1 (should be instrumented)
  g_common = 111;
  int x = 0;
  x++;
  g_common = 123;
  // printf("g_common_outer = %d\n", g_common);

  if (test_condition) {
    // Some operations without function calls or synchronization
    int x_if = 5;
    x_if = x_if * 2;

    // Access 2 to the same address (should be removed by dominance optimization)
    // The block with g_common = 1; dominates this block, path is clear.
    g_common = 222;
    // printf("g_common_if_branch = %d\n", g_common);
  } else {
    // Access 3 to the same address (should be removed by dominance optimization)
    // The block with g_common = 1; dominates this block, path is clear.
    g_common = 333;
    // printf("g_common_else_branch = %d\n", g_common);
  }
  // printf("--- Finished TEST_DOMINANCE_THROUGH_IF ---\n\n");
#endif

#ifdef TEST_DOMINANCE_UNCLEAR_PATH_CALL
    // --- Test 3: Dominance, but path is "not clear" due to a function call ---
    printf("--- Running TEST_DOMINANCE_UNCLEAR_PATH_CALL ---\n");
    // Access 1 (should be instrumented)
    g_common = 1;
    // printf("g_common_access1 = %d\n", g_common);

    potentially_dangerous_call_func();

    // Access 2 to the same address (should NOT be removed, as potentially_dangerous_call_func() breaks the "clear" path)
    g_common = 2;
    // printf("g_common_access2 = %d\n", g_common);
    printf("--- Finished TEST_DOMINANCE_UNCLEAR_PATH_CALL ---\n\n");
#endif

#ifdef TEST_DOMINANCE_UNCLEAR_PATH_SYNC
    // --- Test 4: Dominance, but path is "not clear" due to a synchronization primitive ---
    printf("--- Running TEST_DOMINANCE_UNCLEAR_PATH_SYNC ---\n");
    // Access 1 (should be instrumented)
    g_common = 111;
    // printf("g_common_access1 = %d\n", g_common);

    // Synchronization primitive (e.g., memory fence or atomic operation with barrier semantics)
    // __sync_synchronize(); // GCC specific
    atomic_thread_fence(memory_order_seq_cst); // C11

    // Access 2 to the same address (should NOT be removed, as the barrier breaks the "clear" path)
    g_common = 222;
    printf("g_common_access2 = %d\n", g_common);
    printf("--- Finished TEST_DOMINANCE_UNCLEAR_PATH_SYNC ---\n\n");
#endif

#ifdef TEST_DOMINANCE_READ_AFTER_WRITE
  // --- Test 5: Dominance, different access types (read after write) ---
  printf("--- Running TEST_DOMINANCE_READ_AFTER_WRITE ---\n");
  // Access 1: Write (should be instrumented)
  g_common = 10;
  // printf("g_common_write_access = %d\n", g_common);
  printf("g_common_write_access = \n");

  // Some operations without function calls or synchronization
  int x_raw = g_common + 5;
  (void)x_raw; // Suppress unused variable warning

  // Access 2: Read of the same address (should be removed by dominance optimization,
  // as the dominating write g_common=10 covers this read, and the path is clear)
  int val_raw = g_common;
  printf("g_common_read_access = %d\n", val_raw);
  printf("--- Finished TEST_DOMINANCE_READ_AFTER_WRITE ---\n\n");
#endif

#ifdef TEST_DOMINANCE_WRITE_AFTER_READ
    // --- Test 6: Dominance, different access types (write after read - should not be removed by read's dominance) ---
    printf("--- Running TEST_DOMINANCE_WRITE_AFTER_READ ---\n");
    g_common = 111; // Initialization
    printf("g_common_write_access\n");

    // Access 1: Read (should be instrumented)
    int val_war = g_common;
    (void)val_war;

    // Some operations
    int x_war = 555;
    (void)x_war; // Suppress unused variable warning

    // Access 2: Write to the same address (should NOT be removed by the dominating read,
    // as a read does not cover a write)
    g_common = 222;
    // printf("g_common_write_access = %d\n", g_common);
    // printf("--- Finished TEST_DOMINANCE_WRITE_AFTER_READ ---\n\n");
#endif

#ifdef TEST_DOMINANCE_LOOP
  // --- Test 7: Dominance in a loop (simple case) ---
  printf("--- Running TEST_DOMINANCE_LOOP ---\n");
  for (int i = 0; i < 2; ++i) {
    // Loop will execute twice
    // Access 1 in loop (on the first iteration, should be instrumented)
    // On the second iteration, this access is dominated by the access from the first iteration,
    // BUT! The end of the loop iteration (increment i, condition check) might be seen
    // as breaking the "clear" path for simple inter-iteration dominance analysis.
    // If your isPathClear is smart enough for loops, it might be removed.
    // For a basic implementation, it will likely NOT be removed on the second iteration.
    g_loop_test = i;
    // printf("g_loop_test_outer_iter%d = %d\n", i, g_loop_test);

    // Some operations
    int x_loop = g_loop_test + 1;
    (void)x_loop; // Suppress unused variable warning

    // Access 2 in loop, same address (on each iteration, should be removed
    // by access 1 of the current iteration, if the path within the iteration is clear)
    g_loop_test = i + 10;
    // printf("g_loop_test_inner_iter%d = %d\n", i, g_loop_test);
  }
  printf("--- Finished TEST_DOMINANCE_LOOP ---\n\n");
#endif

#ifdef TEST_NO_DOMINANCE_PARALLEL_BRANCHES
    // --- Test 8: No dominance (parallel branches) ---
    printf("--- Running TEST_NO_DOMINANCE_PARALLEL_BRANCHES ---\n");
    if (test_condition) { // test_condition is 1
        // Access in one branch (should be instrumented)
        g_parallel_test = 1;
        // printf("g_parallel_test_if1 = %d\n", g_parallel_test);
    }

    // No direct dominance between these branches for g_parallel_test
    // if test_condition and test_condition2 can be different

    if (test_condition2) { // test_condition2 is 0
         // Access in another branch (should be instrumented, though this branch is not taken)
        g_parallel_test = 2;
        // printf("g_parallel_test_if2 = %d\n", g_parallel_test);
    } else {
        // Access in another branch (should be instrumented)
        g_parallel_test = 3;
        // printf("g_parallel_test_else2 = %d\n", g_parallel_test);
    }
    printf("--- Finished TEST_NO_DOMINANCE_PARALLEL_BRANCHES ---\n\n");
#endif

  return NULL;
}

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