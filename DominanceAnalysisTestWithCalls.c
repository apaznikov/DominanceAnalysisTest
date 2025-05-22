#include <pthread.h>
#include <stdio.h>
#include <stdatomic.h> // For C11 atomics

// #define TEST_SFA_CALL_CLEAN_FUNCTION
// #define TEST_SFA_CALL_DANGEROUS_INTRINSIC
// #define TEST_SFA_CALL_TRANSITIVELY_DANGEROUS
// #define TEST_SFA_CALL_RECURSIVE_CLEAN
#define TEST_SFA_CALL_RECURSIVE_DANGEROUS

// ... (existing global variables and stub functions) ...
// Common global variables that might be used by multiple tests
int g1;
int g2;
int g_common;
int g_loop_test;
int g_parallel_test;
int g_sfa_test; // Global for SyncFreeAnalysis tests

// --- New functions for SyncFreeAnalysis tests ---


#if defined(TEST_SFA_CALL_DANGEROUS_INTRINSIC) || \
    defined(TEST_SFA_CALL_TRANSITIVELY_DANGEROUS) || \
    defined(TEST_SFA_CALL_RECURSIVE_DANGEROUS)
// Function containing a "dangerous" instruction
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
void sfa_dangerous_function_intrinsic() {
  pthread_mutex_lock(&mutex);
  pthread_mutex_unlock(&mutex);
}
#endif

#if defined(TEST_SFA_CALL_TRANSITIVELY_DANGEROUS)
// Function that calls another "dangerous" function
void sfa_calls_dangerous_function() {
  // printf("Inside sfa_calls_dangerous_function, about to call dangerous_intrinsic\n");
  sfa_dangerous_function_intrinsic();
}
#endif

#if defined(TEST_SFA_CALL_RECURSIVE_CLEAN)
// Mutually recursive functions (both "clean")
void sfa_recursive_a_clean(int count); // Forward declaration
void sfa_recursive_b_clean(int count) {
    if (count <= 0) return;
    // printf("Inside sfa_recursive_b_clean, count: %d\n", count);
    sfa_recursive_a_clean(count - 1);
}
void sfa_recursive_a_clean(int count) {
    if (count <= 0) return;
    // printf("Inside sfa_recursive_a_clean, count: %d\n", count);
    sfa_recursive_b_clean(count - 1);
}
#endif

#ifdef TEST_SFA_CALL_CLEAN_FUNCTION
// Function that is "clean" by itself (contains no dangerous instructions)
void sfa_clean_function() {
  int local = 1;
  local++;
}

// --- Test 9: Dominance, path has a call to a "clean" function
// (determined by SyncFreeAnalysis)
void test_sfa_call_clean_function() {
  int local_sfa_val1, local_sfa_val2;

  // Access 1 (should be instrumented)
  g_sfa_test = 111;
  // local_sfa_val1 = g_sfa_test;

  // This call should be identified as safe by SyncFreeAnalysis
  sfa_clean_function();

  // Access 2 to the same address (SHOULD be removed by dominance optimization,
  // as sfa_clean_function is sync-free)
  g_sfa_test = 222;
  // local_sfa_val2 = g_sfa_test;
}
#endif

#ifdef TEST_SFA_CALL_DANGEROUS_INTRINSIC
// --- Test 10: Dominance, path has a call to a function with an intrinsic dangerous op ---
void test_sfa_call_dangerous_intrinsic() {
  int local_sfa_val1, local_sfa_val2;

  // Access 1 (should be instrumented)
  g_sfa_test = 111;
  // local_sfa_val1 = g_sfa_test;

  sfa_dangerous_function_intrinsic();
  // This call should be identified as dangerous

  // Access 2 to the same address (should NOT be removed, path is unclean)
  g_sfa_test = 222;
  // local_sfa_val2 = g_sfa_test;
}
#endif

#ifdef TEST_SFA_CALL_TRANSITIVELY_DANGEROUS
// --- Test 11: Dominance, path has a call to a function that calls another dangerous function ---
void test_sfa_call_transitively_dangerous() {
  int local_sfa_val1, local_sfa_val2;

  // Access 1 (should be instrumented)
  g_sfa_test = 111;
  // local_sfa_val1 = g_sfa_test;

  sfa_calls_dangerous_function();

  // sfa_calls_dangerous_function -> sfa_dangerous_function_intrinsic (dangerous)
  // Therefore, sfa_calls_dangerous_function should also be dangerous

  // Access 2 to the same address (should NOT be removed, path is unclean)
  g_sfa_test = 222;
  // local_sfa_val2 = g_sfa_test;
}
#endif

#ifdef TEST_SFA_CALL_RECURSIVE_CLEAN
// --- Test 12: Dominance, path has a call to mutually recursive "clean" functions ---
void test_sfa_call_recursive_clean() {
    int local_sfa_val1, local_sfa_val2;

    // Access 1 (should be instrumented)
    g_sfa_test = 111;
    // local_sfa_val1 = g_sfa_test;

    sfa_recursive_a_clean(2); // These functions should be identified as safe

    // Access 2 to the same address (SHOULD be removed by dominance optimization)
    g_sfa_test = 222;
    // local_sfa_val2 = g_sfa_test;
}
#endif

#ifdef TEST_SFA_CALL_RECURSIVE_DANGEROUS
// Mutually recursive functions (one "dangerous")
void sfa_recursive_a_one_dangerous(int count); // Forward declaration
void sfa_recursive_b_one_dangerous(int count) {
  if (count <= 0)
    return;
  // printf("Inside sfa_recursive_b_one_dangerous, count: %d\n", count);
  sfa_dangerous_function_intrinsic(); // Makes this recursion branch dangerous
  sfa_recursive_a_one_dangerous(count - 1);
}

void sfa_recursive_a_one_dangerous(int count) {
  if (count <= 0)
    return;
  // printf("Inside sfa_recursive_a_one_dangerous, count: %d\n", count);
  sfa_recursive_b_one_dangerous(count - 1);
}

// --- Test 13: Dominance, path has a call to mutually recursive functions, one of which is dangerous ---
void test_sfa_call_recursive_dangerous() {
  int local_sfa_val1, local_sfa_val2;

  // Access 1 (should be instrumented)
  g_sfa_test = 111;
  // local_sfa_val1 = g_sfa_test;

  sfa_recursive_a_one_dangerous(2);
  // sfa_recursive_b_one_dangerous contains a dangerous instruction,
  // so both (A and B) should be dangerous.

  // Access 2 to the same address (should NOT be removed, path is unclean)
  g_sfa_test = 222;
  // local_sfa_val2 = g_sfa_test;
}
#endif