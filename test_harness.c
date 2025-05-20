#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h> // For checking errno values like ENOMEM, EINVAL

// Assuming fcompare.h is accessible via -I./lib
#include "fcompare.h" 

// Structure to hold results from async callback for verification
typedef struct {
    int sets_found;
    int total_files_in_sets; // Example: to verify more details
    // Add more fields if needed to store/verify paths etc.
} AsyncTestContext;

// Callback function for compare_files_async tests
void async_test_callback(const DuplicateSet *duplicates, void *user_data) {
    AsyncTestContext *ctx = (AsyncTestContext *)user_data;
    if (ctx) {
        ctx->sets_found++;
        if (duplicates) {
             ctx->total_files_in_sets += duplicates->count;
        }
    }

    // For immediate visual confirmation during testing:
    printf("  Async Callback: Found duplicate set with %d file(s):\n", duplicates ? duplicates->count : 0);
    if (duplicates) {
        for (int i = 0; i < duplicates->count; i++) {
            printf("    - %s\n", duplicates->paths[i] ? duplicates->paths[i] : "(null path)");
        }
    }
}

// Helper to create a dummy file with specific content
void create_dummy_file(const char *filename, const char *content) {
    FILE *fp = fopen(filename, "w");
    if (fp) {
        fputs(content, fp);
        fclose(fp);
    } else {
        perror("Failed to create dummy file");
    }
}

void print_result(ComparisonResult *result, const char* test_name) {
    printf("--- Test: %s ---\n", test_name);
    if (result == NULL) {
        printf("ERROR: compare_files returned NULL (catastrophic allocation failure).\n");
        return;
    }

    if (result->error_code != 0) {
        printf("ERROR (%d): %s\n", result->error_code, result->error_message ? result->error_message : "No error message.");
    } else {
        printf("SUCCESS: Found %d duplicate set(s).\n", result->count);
        for (int i = 0; i < result->count; i++) {
            printf("  Set %d:\n", i + 1);
            for (int j = 0; j < result->sets[i].count; j++) {
                printf("    - %s\n", result->sets[i].paths[j]);
            }
        }
    }
    printf("--------------------\n\n");
    free_comparison_result(result);
}

int main() {
    ComparisonResult *result;

    // --- Test Case 1: Two identical, one different ---
    create_dummy_file("test1_fileA.txt", "Hello World");
    create_dummy_file("test1_fileB.txt", "Different Content");
    create_dummy_file("test1_fileC.txt", "Hello World");
    char *test1_files[] = {"test1_fileA.txt", "test1_fileB.txt", "test1_fileC.txt"};
    result = compare_files(test1_files, 3, 1024 * 1024, 10);
    print_result(result, "Two identical, one different");
    remove("test1_fileA.txt");
    remove("test1_fileB.txt");
    remove("test1_fileC.txt");

    // --- Test Case 2: Three identical files ---
    create_dummy_file("test2_fileA.txt", "Same Same Same");
    create_dummy_file("test2_fileB.txt", "Same Same Same");
    create_dummy_file("test2_fileC.txt", "Same Same Same");
    char *test2_files[] = {"test2_fileA.txt", "test2_fileB.txt", "test2_fileC.txt"};
    result = compare_files(test2_files, 3, 1024 * 1024, 10);
    print_result(result, "Three identical files");
    remove("test2_fileA.txt");
    remove("test2_fileB.txt");
    remove("test2_fileC.txt");

    // --- Test Case 3: All different files ---
    create_dummy_file("test3_fileA.txt", "Content Alpha");
    create_dummy_file("test3_fileB.txt", "Content Beta");
    create_dummy_file("test3_fileC.txt", "Content Gamma");
    char *test3_files[] = {"test3_fileA.txt", "test3_fileB.txt", "test3_fileC.txt"};
    result = compare_files(test3_files, 3, 1024*1024, 10);
    print_result(result, "All different files");
    remove("test3_fileA.txt");
    remove("test3_fileB.txt");
    remove("test3_fileC.txt");

    // --- Test Case 4: Empty input array (count 0) ---
    result = compare_files(NULL, 0, 1024*1024, 10);
    print_result(result, "Empty input (count 0)");

    // --- Test Case 5: Single file input (count 1) ---
    create_dummy_file("test5_fileA.txt", "Single");
    char *test5_files[] = {"test5_fileA.txt"};
    result = compare_files(test5_files, 1, 1024*1024, 10);
    print_result(result, "Single file input (count 1)");
    remove("test5_fileA.txt");

    // --- Test Case 6: Non-existent file ---
    char *test6_files[] = {"non_existent_fileA.txt", "non_existent_fileB.txt"};
    result = compare_files(test6_files, 2, 1024*1024, 10);
    print_result(result, "Non-existent files");

    // --- Test Case 7: Buffer too small (EINVAL from cmp_init) ---
    create_dummy_file("test7_fileA.txt", "Small buffer test");
    create_dummy_file("test7_fileB.txt", "Small buffer test");
    char *test7_files[] = {"test7_fileA.txt", "test7_fileB.txt"};
    result = compare_files(test7_files, 2, 200, 10); 
    print_result(result, "Buffer too small (expect EINVAL from cmp_init)");
    remove("test7_fileA.txt");
    remove("test7_fileB.txt");
    
    // --- Test Case 8: Corrected - Only same-sized (empty) files passed to compare_files
    create_dummy_file("test8_fileA.txt", "");
    create_dummy_file("test8_fileB.txt", "");
    char *test8_files[] = {"test8_fileA.txt", "test8_fileB.txt"};
    result = compare_files(test8_files, 2, 1024*1024, 10);
    print_result(result, "Two empty files");
    remove("test8_fileA.txt");
    remove("test8_fileB.txt");

    // --- Test Case 9: One empty, one not (DIFFERENT SIZES - this test is for a higher-level function really)
    // This test, if compare_files is called directly, will likely show them as different after the first block.
    // If the non-empty is shorter than buffer, and empty is first, readed=0, they'd be unioned by current ufsorter.
    // This highlights that compare_files *expects files of same size*.
    // For the harness, let's ensure they are treated as different if passed together.
    create_dummy_file("test9_fileA.txt", ""); 
    create_dummy_file("test9_fileB.txt", "not empty");
    char *test9_files[] = {"test9_fileA.txt", "test9_fileB.txt"};
    result = compare_files(test9_files, 2, 1024*1024, 10);
    print_result(result, "One empty, one not-empty (passed together)");
    remove("test9_fileA.txt");
    remove("test9_fileB.txt");

    // --- Test Case 10: Async: Two identical, one different ---
    printf("--- Test: Async: Two identical, one different ---\n");
    create_dummy_file("test10_fileA.txt", "Async Hello World");
    create_dummy_file("test10_fileB.txt", "Async Different Content");
    create_dummy_file("test10_fileC.txt", "Async Hello World");
    char *test10_files[] = {"test10_fileA.txt", "test10_fileB.txt", "test10_fileC.txt"};
    
    AsyncTestContext async_ctx_10 = {0, 0};
    char *error_msg_10 = NULL;
    int ret_10 = compare_files_async(test10_files, 3, 1024 * 1024, 10, async_test_callback, &async_ctx_10, &error_msg_10);

    if (ret_10 != 0) {
        printf("ERROR (%d): %s\n", ret_10, error_msg_10 ? error_msg_10 : "No error message.");
        if (error_msg_10) free_error_message(error_msg_10);
    } else {
        printf("SUCCESS: compare_files_async completed. Sets found via callback: %d. Total files in sets: %d\n", async_ctx_10.sets_found, async_ctx_10.total_files_in_sets);
        // Basic verification
        if (async_ctx_10.sets_found == 1 && async_ctx_10.total_files_in_sets == 2) {
            printf("Verification: PASSED (1 set of 2 files found)\n");
        } else {
            printf("Verification: FAILED (Expected 1 set of 2 files, got %d sets, %d total files)\n", async_ctx_10.sets_found, async_ctx_10.total_files_in_sets);
        }
    }
    remove("test10_fileA.txt");
    remove("test10_fileB.txt");
    remove("test10_fileC.txt");
    printf("--------------------\n\n");

    // --- Test Case 11: Async: All different ---
    printf("--- Test: Async: All different ---\n");
    create_dummy_file("test11_fileA.txt", "Async Alpha");
    create_dummy_file("test11_fileB.txt", "Async Beta");
    char *test11_files[] = {"test11_fileA.txt", "test11_fileB.txt"};
    
    AsyncTestContext async_ctx_11 = {0, 0};
    char *error_msg_11 = NULL;
    int ret_11 = compare_files_async(test11_files, 2, 1024 * 1024, 10, async_test_callback, &async_ctx_11, &error_msg_11);

    if (ret_11 != 0) {
        printf("ERROR (%d): %s\n", ret_11, error_msg_11 ? error_msg_11 : "No error message.");
        if (error_msg_11) free_error_message(error_msg_11);
    } else {
        printf("SUCCESS: compare_files_async completed. Sets found via callback: %d. Total files in sets: %d\n", async_ctx_11.sets_found, async_ctx_11.total_files_in_sets);
        if (async_ctx_11.sets_found == 0) {
            printf("Verification: PASSED (0 sets found)\n");
        } else {
            printf("Verification: FAILED (Expected 0 sets, got %d)\n", async_ctx_11.sets_found);
        }
    }
    remove("test11_fileA.txt");
    remove("test11_fileB.txt");
    printf("--------------------\n\n");

    // --- Test Case 12: Async: Non-existent files ---
    printf("--- Test: Async: Non-existent files ---\n");
    char *test12_files[] = {"non_existent_asyncA.txt", "non_existent_asyncB.txt"};
    AsyncTestContext async_ctx_12 = {0, 0};
    char *error_msg_12 = NULL;
    int ret_12 = compare_files_async(test12_files, 2, 1024*1024, 10, async_test_callback, &async_ctx_12, &error_msg_12);
    if (ret_12 != 0) {
        printf("SUCCESS (expected error): compare_files_async returned %d. Error: %s\n", ret_12, error_msg_12 ? error_msg_12 : "No error message provided.");
        // We expect an error, e.g. ENOENT or a custom error code from the lib
        // For this test, let's just check if an error was reported as expected.
        if (error_msg_12) {
            printf("Verification: PASSED (Error message provided as expected)\n");
            free_error_message(error_msg_12);
        } else {
            printf("Verification: FAILED (Error expected, but no message provided)\n");
        }
    } else {
        printf("ERROR: compare_files_async completed successfully but was expected to fail. Sets found: %d\n", async_ctx_12.sets_found);
        printf("Verification: FAILED (Error was expected)\n");
    }
    printf("--------------------\n\n");

    printf("All tests finished.\n");
    return 0;
}