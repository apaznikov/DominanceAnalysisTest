#include <pthread.h>
#include <stdio.h>
#include <stdatomic.h> // For C11 atomics

#define TEST_CALL_CLEAN_FUNCTION
// #define TEST_CALL_DANGEROUS_INTRINSIC
// #define TEST_CALL_TRANSITIVELY_DANGEROUS
// #define TEST_CALL_RECURSIVE_CLEAN
// #define TEST_CALL_RECURSIVE_DANGEROUS

// ... (existing global variables and stub functions) ...
// Common global variables that might be used by multiple tests
int g1;
int g2;
int g_common;
int g_loop_test;
int g_parallel_test;
int g_test; // Global for SyncFreeAnalysis tests

// --- New functions for SyncFreeAnalysis tests ---


#if defined(TEST_CALL_DANGEROUS_INTRINSIC) || \
    defined(TEST_CALL_TRANSITIVELY_DANGEROUS) || \
    defined(TEST_CALL_RECURSIVE_DANGEROUS)
// Function containing a "dangerous" instruction
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
void dangerous_function_intrinsic() {
  pthread_mutex_lock(&mutex);
  pthread_mutex_unlock(&mutex);
}
#endif

#if defined(TEST_CALL_TRANSITIVELY_DANGEROUS)
// Function that calls another "dangerous" function
void calls_dangerous_function() {
  // printf("Inside calls_dangerous_function, about to call dangerous_intrinsic\n");
  dangerous_function_intrinsic();
}
#endif

#if defined(TEST_CALL_RECURSIVE_CLEAN)
// Mutually recursive functions (both "clean")
void recursive_a_clean(int count); // Forward declaration
void recursive_b_clean(int count) {
    if (count <= 0) return;
    // printf("Inside recursive_b_clean, count: %d\n", count);
    recursive_a_clean(count - 1);
}
void recursive_a_clean(int count) {
    if (count <= 0) return;
    // printf("Inside recursive_a_clean, count: %d\n", count);
    recursive_b_clean(count - 1);
}
#endif

#ifdef TEST_CALL_CLEAN_FUNCTION
// Function that is "clean" by itself (contains no dangerous instructions)
void clean_function() {
  int local = 1;
  local++;
}

// --- Test 9: Dominance, path has a call to a "clean" function
// (determined by SyncFreeAnalysis)
void test_call_clean_function() {
  int local_val1, local_val2;

  // Access 1 (should be instrumented)
  g_test = 111;
  // local_val1 = g_test;

  // This call should be identified as safe by SyncFreeAnalysis
  clean_function();

  // Access 2 to the same address (SHOULD be removed by dominance optimization,
  // as clean_function is sync-free)
  g_test = 222;
  // local_val2 = g_test;
}
#endif

#ifdef TEST_CALL_DANGEROUS_INTRINSIC
// --- Test 10: Dominance, path has a call to a function with an intrinsic dangerous op ---
void test_call_dangerous_intrinsic() {
  int local_val1, local_val2;

  // Access 1 (should be instrumented)
  g_test = 111;
  // local_val1 = g_test;

  dangerous_function_intrinsic();
  // This call should be identified as dangerous

  // Access 2 to the same address (should NOT be removed, path is unclean)
  g_test = 222;
  // local_val2 = g_test;
}
#endif

#ifdef TEST_CALL_TRANSITIVELY_DANGEROUS
// --- Test 11: Dominance, path has a call to a function that calls another dangerous function ---
void test_call_transitively_dangerous() {
  int local_val1, local_val2;

  // Access 1 (should be instrumented)
  g_test = 111;
  // local_val1 = g_test;

  calls_dangerous_function();

  // calls_dangerous_function -> dangerous_function_intrinsic (dangerous)
  // Therefore, calls_dangerous_function should also be dangerous

  // Access 2 to the same address (should NOT be removed, path is unclean)
  g_test = 222;
  // local_val2 = g_test;
}
#endif

#ifdef TEST_CALL_RECURSIVE_CLEAN
// --- Test 12: Dominance, path has a call to mutually recursive "clean" functions ---
void test_call_recursive_clean() {
    int local_val1, local_val2;

    // Access 1 (should be instrumented)
    g_test = 111;
    // local_val1 = g_test;

    recursive_a_clean(2); // These functions should be identified as safe

    // Access 2 to the same address (SHOULD be removed by dominance optimization)
    g_test = 222;
    // local_val2 = g_test;
}
#endif

#ifdef TEST_CALL_RECURSIVE_DANGEROUS
// Mutually recursive functions (one "dangerous")
void recursive_a_one_dangerous(int count); // Forward declaration
void recursive_b_one_dangerous(int count) {
  if (count <= 0)
    return;
  // printf("Inside recursive_b_one_dangerous, count: %d\n", count);
  dangerous_function_intrinsic(); // Makes this recursion branch dangerous
  recursive_a_one_dangerous(count - 1);
}

void recursive_a_one_dangerous(int count) {
  if (count <= 0)
    return;
  // printf("Inside recursive_a_one_dangerous, count: %d\n", count);
  recursive_b_one_dangerous(count - 1);
}

// --- Test 13: Dominance, path has a call to mutually recursive functions, one of which is dangerous ---
void test_call_recursive_dangerous() {
  int local_val1, local_val2;

  // Access 1 (should be instrumented)
  g_test = 111;
  // local_val1 = g_test;

  recursive_a_one_dangerous(2);
  // recursive_b_one_dangerous contains a dangerous instruction,
  // so both (A and B) should be dangerous.

  // Access 2 to the same address (should NOT be removed, path is unclean)
  g_test = 222;
  // local_val2 = g_test;
}
#endif