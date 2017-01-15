#define _XOPEN_SOURCE 700

#include "fcompare.h"
#include "salloc.h"
#include <fcntl.h>
#include <ftw.h>
#include <getopt.h>
#include <string.h>

#ifndef USE_FDS
#define USE_FDS 15
#endif

typedef struct {
    char *filepath;
    off_t st_size;
} file_item;

typedef struct _item {
    file_item *fi;
    struct _item *next;
} item;

item *current_file;
size_t total_files = 0;

item *
add_item(item *where, file_item *fi) {
    where->next = (item *) salloc(sizeof(item), handle_exit);
    where = where->next;

    where->fi = fi;
    where->next = NULL;
    return where;
}

file_item **
create_array(item *head) {
    int i = 0;
    item *current = head;
    item *hlp;
    file_item **fis =
            (file_item **) salloc(sizeof(file_item) * total_files, handle_exit);

    while (current->next) {
        hlp = current->next;
        fis[i++] = hlp->fi;
        current = hlp;
    }
    return fis;
}

void
free_list(item *head) {
    item *current = head;
    item *hlp;
    while (current->next) {
        hlp = current->next;
        free(current);
        current = hlp;
    }
    free(current);
}

void
free_file_items(int count, file_item **file_items) {
    for (int i = 0; i < count; i++) {
        free(file_items[i]->filepath);
        free(file_items[i]);
    }
}

int
cmpsizerev(const void *p1, const void *p2) {
    file_item **fi1 = (file_item **) p1;
    file_item **fi2 = (file_item **) p2;
    off_t a1 = (*fi1)->st_size;
    off_t a2 = (*fi2)->st_size;

    return (a1 < a2) - (a1 > a2);
    // return (a1 > a2) - (a1 < a2);
}

int
process_entry(const char *filepath, const struct stat *info, const int typeflag,
              struct FTW *pathinfo) {
    if (S_ISREG(info->st_mode)) {
        file_item *fi =
                (file_item *) salloc(sizeof(file_item), handle_exit);

        fi->filepath = strdup(filepath);
        fi->st_size = info->st_size;

        current_file = add_item(current_file, fi);
        total_files++;
    }
    return 0;
}

int
process_same_size(file_item *files[], int count, int max_buffer) {
    char **name = (char **) salloc(sizeof(char *) * count, handle_exit);
    for (int i = 0; i < count; i++) {
        name[i] = files[i]->filepath;
    }
    int res = compare_files(name, count, max_buffer);
    free(name);
    return res;
}

void
print_usage_exit(char *execname) {
    fprintf(stderr, "Usage: %s [OPTIONS] <FOLDER> [FOLDER]...\n", execname);
    fprintf(stderr,
            "Find duplicate files in FOLDERs according to their content.\n\n");
    fprintf(stderr,
            "Mandatory arguments to long options are mandatory for short options too.\n");
    fprintf(stderr,
            "  -f, --same-fs             process files only on one filesystem\n");
    fprintf(stderr,
            "  -s, --follow-symlinks     follow symlinks when processing files\n");
    fprintf(stderr,
            "  -b, --max-buffer=SIZE     maximum memory buffer (in bytes) for files comparing\n");
    fprintf(stderr,
            "  -o, --max-of=COUNT        force maximum open files (default %d)\n",
            MAX_OPEN_FILES);
    fprintf(stderr,
            "  -m, --min-file-size=SIZE  check only file with size grater or equal to size (default 1)\n");
    exit(1);
}

void
print_files(file_item **fi, int count) {
    fprintf(stdout, "\n");
    for (int i = 0; i < count; i++) {
        fprintf(stdout, "%s\n", fi[i]->filepath);
    }
}

void
process_files(file_item **fis, int min_file_size, int max_buffer) {
    qsort(fis, total_files, sizeof(file_item *), cmpsizerev);
    fprintf(stderr, "done.\nStarting fast comparsion.\n");

    int stat_cluster_count = 0;
    int stat_total_files_read = 0;
    off_t last_size = fis[0]->st_size;
    int start = 0;
    int count = 1;
    for (int i = 1; i < total_files + 1; i++) {
        if (i < total_files && last_size == fis[i]->st_size) {
            count++;
        } else {
            if (count > 1) {
                if (last_size == 0 && min_file_size <= 0) {
                    print_files(&fis[start], count);

                    stat_cluster_count++;
                    stat_total_files_read += count;
                } else if (last_size > min_file_size) {
                    int cluster_count =
                            process_same_size(&fis[start], count,
                                              max_buffer);

                    stat_cluster_count += cluster_count;
                    stat_total_files_read += count;
                }
            }
            count = 1;
            start = i;
        }
        if (i < total_files) {
            last_size = fis[i]->st_size;
        }
    }

    fprintf(stderr, "Total files being processed: %zu\n", total_files);
    fprintf(stderr, "Total files being read: %d\n", stat_total_files_read);
    fprintf(stderr, "Total equality clusters: %d\n", stat_cluster_count);
}

void
process_folders(int folders_cnt,
                char **folders,
                int opt_same_fs,
                int opt_follow_symlinks,
                int opt_buffer_size, int opt_max_open_files, int opt_min_file_size) {
    item *file_list_head = (item *) salloc(sizeof(item), handle_exit);
    current_file = file_list_head;

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
                    strerror(result));
        }
    }
    if (total_files > 0) {
        fprintf(stderr, "%zu files found\nSorting ... ", total_files);

        file_item **fis = create_array(file_list_head);
        free_list(file_list_head);

        process_files(fis, opt_min_file_size, opt_buffer_size);

        free_file_items(total_files, fis);
        free(fis);
    } else {
        fprintf(stderr, "No files to process\n");
    }
}

int
main(int argc, char *argv[]) {
    int opt_same_fs = 0;
    int opt_follow_symlinks = 0;
    int opt_buffer_size = -1;
    int opt_max_open_files = -1;
    int opt_min_file_size = 1;
    char **folders;

    static struct option long_options[] = {
            {"same-fs",         no_argument,       0, 0},
            {"follow-symlinks", no_argument,       0, 0},
            {"max-buffer",      required_argument, 0, 0},
            {"max-of",          required_argument, 0, 0},
            {"min-file-size",   required_argument, 0, 0},
            {0, 0,                                 0, 0}
    };

    int option_index = 0;

    while ((getopt_long(argc, argv, "fsb:o:m:", long_options, &option_index)) !=
           -1) {
        switch (option_index) {
            case 0:
                opt_same_fs = 1;
                break;
            case 1:
                opt_follow_symlinks = 1;
                break;
            case 2:
                opt_buffer_size = atoi(optarg);
                break;
            case 3:
                opt_max_open_files = atoi(optarg);
                break;
            case 4:
                opt_min_file_size = atoi(optarg);
                break;
            default:
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