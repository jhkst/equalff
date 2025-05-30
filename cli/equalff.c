#define _FILE_OFFSET_BITS 64
#define _XOPEN_SOURCE 700

#include "fcompare.h"
#include "salloc.h"
#include <errno.h>
#include <fcntl.h>
#include <ftw.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef USE_FDS
#define USE_FDS 15
#endif

#define DEFAULT_MAX_OPEN_FILES FOPEN_MAX

typedef struct {
    char *filepath;
    off_t st_size;
} file_item;

typedef struct _item {
    file_item *fi;
    struct _item *next;
} item;

typedef struct {
    item *list_head;    // Sentinel node for the linked list
    item *list_tail;    // Points to the last actual item added to the list
    size_t files_count; // Total number of files collected
} FileCollectionCtx;

static FileCollectionCtx g_file_collection;

static int cli_global_first_output_emitted = 0; // 0 = false, 1 = true

typedef struct {
    int clusters_found_this_call; // Tracks clusters found for a specific call to compare_files_async
} CliAsyncCallbackLocalContext;

item *
add_item(item *current_tail, file_item *fi) {
    current_tail->next = (item *) salloc(sizeof(item), handle_exit);
    current_tail = current_tail->next;

    current_tail->fi = fi;
    current_tail->next = NULL;
    return current_tail; // The new tail node
}

/**
 * Create array of file_item pointers from linked list.
 * @param list_sentinel_head sentinel head of linked list
 * @param num_files total number of actual files (from g_file_collection.files_count)
 * @return array of file_item pointers
 */
file_item **
create_array_from_list(item *list_sentinel_head, size_t num_files) {
    int i = 0;
    item *current_node = list_sentinel_head->next; // Start from the first actual item
    file_item **fis =
            (file_item **) salloc(sizeof(file_item *) * num_files, handle_exit);

    while (current_node != NULL) {
        if (i >= num_files) {
            fprintf(stderr, "Error: More items in list than files_count in create_array_from_list\n");
            // This indicates a discrepancy, possibly exit or handle error
            exit(EXIT_FAILURE); 
        }
        fis[i++] = current_node->fi;
        current_node = current_node->next;
    }
    // After the loop, i should ideally be equal to num_files.
    // If i < num_files, the list was shorter than expected.
    if (i != num_files) {
         fprintf(stderr, "Warning: files_count (%zu) and items copied to array (%d) mismatch.\n", num_files, i);
         // This might be an issue depending on whether num_files was an exact count or an estimate.
         // For this program, it should be exact.
    }
    return fis;
}

/**
 * Free linked list structure (the item nodes themselves).
 * file_item structs pointed to by fi are freed separately by free_file_items_array.
 * @param list_sentinel_head sentinel head of linked list
 */
void
free_file_list_nodes(item *list_sentinel_head) {
    item *current_node = list_sentinel_head->next; // Start with the first actual item
    while (current_node != NULL) {
        item *node_to_free = current_node;
        current_node = current_node->next;
        free(node_to_free);
    }
    free(list_sentinel_head); // Free the sentinel node itself
}

/**
 * Free array of file_item pointers and the file_items themselves.
 * @param count number of items in array (from g_file_collection.files_count)
 * @param file_items_array array of file_item pointers
 */
void
free_file_items_array(size_t count, file_item **file_items_array) {
    for (size_t i = 0; i < count; i++) {
        if (file_items_array[i] != NULL) { // Defensive check
            free(file_items_array[i]->filepath); // Free the strdup'd path
            free(file_items_array[i]);          // Free the file_item struct
        }
    }
}

/**
 * Compare two file_item pointers by their size.
 * @param p1 first file_item pointer
 * @param p2 second file_item pointer
 * @return 1 if first is greater, -1 if second is greater, 0 if equal
 */
int
cmpsizerev(const void *p1, const void *p2) {
    file_item **fi1 = (file_item **) p1;
    file_item **fi2 = (file_item **) p2;
    off_t a1 = (*fi1)->st_size;
    off_t a2 = (*fi2)->st_size;

    return (a1 < a2) - (a1 > a2);
    // return (a1 > a2) - (a1 < a2);
}

/**
 * Process file entry in directory tree. See nftw(3) for more information.
 * @param filepath path to file
 * @param info stat structure
 * @param typeflag type of file
 * @param pathinfo additional information
 * @return 0
 */
int
process_entry(const char *filepath, const struct stat *info, const int typeflag,
              struct FTW *pathinfo) {
    if (S_ISREG(info->st_mode)) {
        file_item *fi =
                (file_item *) salloc(sizeof(file_item), handle_exit);

        fi->filepath = sstrdup(filepath, handle_exit);
        fi->st_size = info->st_size;

        g_file_collection.list_tail = add_item(g_file_collection.list_tail, fi);
        g_file_collection.files_count++;
    }
    return 0;
}

// Callback function to print duplicates as they are found
static void cli_output_callback(const DuplicateSet *duplicates, void *user_data) {
    CliAsyncCallbackLocalContext *local_ctx = (CliAsyncCallbackLocalContext *)user_data;

    // The original output format has a blank line before each new set of duplicates.
    fprintf(stdout, "\n"); 

    for (int i = 0; i < duplicates->count; i++) {
        fprintf(stdout, "%s\n", duplicates->paths[i]);
    }
    cli_global_first_output_emitted = 1; // Mark that some output has occurred for overall formatting
    if (local_ctx) {
        local_ctx->clusters_found_this_call++;
    }
}

int
process_same_size_async(file_item *files[], int count, int max_buffer, int max_open_files) {
    char **name = (char **) salloc(sizeof(char *) * count, NULL); 
    if (!name) {
        fprintf(stderr, "Error: Failed to allocate memory for file names in process_same_size_async.\n");
        return 0; 
    }
    for (int i = 0; i < count; i++) {
        name[i] = files[i]->filepath;
    }

    CliAsyncCallbackLocalContext local_cb_ctx = {0};
    char *error_msg = NULL;

    int ret_code = compare_files_async(name, count, max_buffer, max_open_files, 
                                       cli_output_callback, &local_cb_ctx, &error_msg);
    free(name); 

    if (ret_code != 0) {
        fprintf(stderr, "Error during file comparison: %s (Code: %d)\n",
                error_msg ? error_msg : strerror(ret_code), 
                ret_code);
        if (error_msg) {
            free_error_message(error_msg);
        }
        return 0; // Return 0 clusters if an error occurred during the async call itself
    }
    // If compare_files_async succeeded, errors during salloc/sstrdup in callbacks
    // are handled internally by the library and shouldn't stop cli_output_callback from being called for other sets.
    // The number of clusters actually printed and counted is in local_cb_ctx.
    return local_cb_ctx.clusters_found_this_call;
}

/**
 * Print usage and exit.
 * @param execname name of executable
 */
void
print_usage_exit(char *execname) {
    fprintf(stderr, "Usage: %s [OPTIONS] <DIRECTORY> [DIRECTORY]...\n", execname);
    fprintf(stderr,
            "Find duplicate files in the specified directories based on their content.\n\n");
    fprintf(stderr,
            "Mandatory arguments for long options are also mandatory for short options.\n");
    fprintf(stderr,
            "  -f, --same-fs             Only process files within the same filesystem\n");
    fprintf(stderr,
            "  -s, --follow-symlinks     Follow symbolic links when processing files\n");
    fprintf(stderr,
            "  -b, --max-buffer=SIZE     Set the maximum memory buffer (in bytes) for file comparison (default 8192, min 128)\n");
    fprintf(stderr,
            "  -o, --max-of=COUNT        Set the maximum number of open files (default %d)\n",
            DEFAULT_MAX_OPEN_FILES);
    fprintf(stderr,
            "  -m, --min-file-size=SIZE  Only check files with a size greater than or equal to SIZE (default 1)\n");
    fprintf(stderr,
            "  -h, --help                Display this help message and exit\n");
    exit(1);
}

/**
 * Print files to stdout.
 * @param fi array of file_item pointers
 * @param count number of items in array
 */
void
print_files(file_item **fi, int count) {
    fprintf(stdout, "\n");
    for (int i = 0; i < count; i++) {
        fprintf(stdout, "%s\n", fi[i]->filepath);
    }
}

/**
 * Process files in array.
 * @param fis array of file_item pointers
 * @param num_files_in_array total number of files in fis (from g_file_collection.files_count)
 * @param max_buffer maximum memory buffer for files comparing
 * @param max_open_files maximum open files
 * @param min_file_size minimum file size
 */
void
process_files_array(file_item **fis, size_t num_files_in_array, int max_buffer, int max_open_files, int min_file_size) {
    if (num_files_in_array == 0) {
        return;
    }

    qsort(fis, num_files_in_array, sizeof(file_item *), cmpsizerev);
    fprintf(stderr, "done.\nStarting fast comparison.\n");

    cli_global_first_output_emitted = 0; // Reset for this processing run
    int stat_cluster_count = 0;
    
    size_t current_idx = 0;
    while (current_idx < num_files_in_array) {
        size_t group_start_idx = current_idx;
        long long current_group_file_size = fis[group_start_idx]->st_size;

        // Advance current_idx to find the end of the current group of same-sized files
        current_idx++; 
        while (current_idx < num_files_in_array && fis[current_idx]->st_size == current_group_file_size) {
            current_idx++;
        }
        
        int count_in_this_group = current_idx - group_start_idx;

        if (count_in_this_group > 1) {
            // Special handling for zero-byte files if min_file_size allows them
            if (current_group_file_size == 0 && min_file_size <= 0) {
                // All zero-byte files are considered one set of duplicates by original logic
                print_files(&fis[group_start_idx], count_in_this_group);
                stat_cluster_count++;
                if(count_in_this_group > 0) cli_global_first_output_emitted = 1; // Mark output occurred
            } 
            // For non-zero sized files that meet min_file_size criteria
            else if (current_group_file_size > 0 && current_group_file_size >= min_file_size) {
                stat_cluster_count += process_same_size_async(&fis[group_start_idx], count_in_this_group, max_buffer, max_open_files);
            }
            // Files smaller than min_file_size (and not zero meeting the condition above) are skipped
        }
    }

    if (cli_global_first_output_emitted) {
      fprintf(stdout, "\n"); // Ensure a final newline if any output was made, to separate from stderr summary
    }
    fprintf(stderr, "Total files being processed: %zu\n", num_files_in_array);
    // fprintf(stderr, "Total files being read: %d\n", stat_total_readed_files); // This stat is hard to get now
    fprintf(stderr, "Total equality clusters: %d\n", stat_cluster_count);
}

/**
 * Process folders.
 * @param folders_cnt number of folders
 * @param folders array of folder names
 * @param opt_same_fs process files only on one filesystem
 * @param opt_follow_symlinks follow symlinks when processing files
 * @param opt_buffer_size maximum memory buffer for files comparing
 * @param opt_max_open_files maximum open files
 * @param opt_min_file_size minimum file size
 */
void
process_folders(int folders_cnt,
                char **folders,
                int opt_same_fs,
                int opt_follow_symlinks,
                int opt_buffer_size, int opt_max_open_files, int opt_min_file_size) {
    
    // Initialize the global file collection context
    g_file_collection.list_head = (item *) salloc(sizeof(item), handle_exit);
    g_file_collection.list_head->fi = NULL;    // Sentinel node has no file_item
    g_file_collection.list_head->next = NULL;
    g_file_collection.list_tail = g_file_collection.list_head; // List is empty, tail is head
    g_file_collection.files_count = 0;

    fprintf(stderr, "Looking for files ... ");

    int flag = 0;
    if (!opt_follow_symlinks) {
        flag |= FTW_PHYS;
    }
    if (opt_same_fs) {
        flag |= FTW_MOUNT;
    }

    for (int i = 0; i < folders_cnt; i++) {
        int result =
                nftw(folders[i], process_entry, USE_FDS, flag);
        if (result != 0) {
            fprintf(stderr, "Cannot process %s: %s\n", folders[i],
                    strerror(errno));
        }
    }
    if (g_file_collection.files_count > 0) {
        fprintf(stderr, "%zu files found\nSorting ... ", g_file_collection.files_count);

        file_item **fis = create_array_from_list(g_file_collection.list_head, g_file_collection.files_count);
        
        process_files_array(fis, g_file_collection.files_count, opt_buffer_size, opt_max_open_files, opt_min_file_size);

        // Cleanup sequence:
        // 1. Free the content pointed to by the array (file_item structs and their filepaths)
        free_file_items_array(g_file_collection.files_count, fis);
        // 2. Free the array of pointers itself
        free(fis);
        // 3. Free the linked list nodes (item structs)
        free_file_list_nodes(g_file_collection.list_head);
        
        // Reset global context pointers for safety, though not strictly necessary if program exits soon
        g_file_collection.list_head = NULL;
        g_file_collection.list_tail = NULL;
        g_file_collection.files_count = 0;

    } else {
        fprintf(stderr, "No files to process\n");
    }
}

/**
 * Main function.
 * @param argc number of arguments
 * @param argv array of arguments
 * @return exit code
 */
int
main(int argc, char *argv[]) {
    int opt_same_fs = 0;
    int opt_follow_symlinks = 0;
    int opt_buffer_size = 8192; // Set a default positive buffer size
    int opt_max_open_files = DEFAULT_MAX_OPEN_FILES;
    int opt_min_file_size = 1;
    char **folders;

    static struct option long_options[] = {
            {"same-fs",         no_argument,       0, 'f'},
            {"follow-symlinks", no_argument,       0, 's'},
            {"max-buffer",      required_argument, 0, 'b'},
            {"max-of",          required_argument, 0, 'o'},
            {"min-file-size",   required_argument, 0, 'm'},
            {"help",            no_argument,       0, 'h'},
            {0, 0,                                 0, 0}
    };

    int c;

    while ((c = getopt_long(argc, argv, "fsb:o:m:h", long_options, NULL)) != -1) {
        switch (c) {
            case 'f':
                opt_same_fs = 1;
                break;
            case 's':
                opt_follow_symlinks = 1;
                break;
            case 'b':
                opt_buffer_size = atoi(optarg);
                if (opt_buffer_size <= 0) {
                    fprintf(stderr, "Error: max-buffer must be a positive integer.\n");
                    print_usage_exit(argv[0]);
                }
                break;
            case 'o':
                opt_max_open_files = atoi(optarg);
                if (opt_max_open_files <= 0) {
                    fprintf(stderr, "Error: max-of must be a positive integer.\n");
                    print_usage_exit(argv[0]);
                }
                break;
            case 'm':
                opt_min_file_size = atoi(optarg);
                if (opt_min_file_size < 0) {
                    fprintf(stderr, "Error: min-file-size must be a non-negative integer.\n");
                    print_usage_exit(argv[0]);
                }
                break;
            case 'h':
                print_usage_exit(argv[0]);
                break;
            case '?':
                print_usage_exit(argv[0]);
                break;
            default:
                fprintf(stderr, "Internal error: unhandled option character '%c'.\n", c);
                print_usage_exit(argv[0]);
        }
    }

    if (optind >= argc) {
        print_usage_exit(argv[0]);
    }

    int folder_cnt = argc - optind;

    folders = (char **) salloc(sizeof(char *) * folder_cnt, handle_exit);

    for (int i = optind; i < argc; i++) {
        folders[i - optind] = argv[i];
    }

    process_folders(folder_cnt, folders, opt_same_fs, opt_follow_symlinks,
                    opt_buffer_size, opt_max_open_files, opt_min_file_size);

    free(folders);

    return 0;
}