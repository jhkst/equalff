# Equalff

**Equalff** is a high-speed duplicate file finder. It employs a unique algorithm, making it faster than other duplicate file finders for a single run. The project is structured as a core dynamic library (`libequalff.so`) providing the duplicate detection logic, and a command-line utility (`equalff`) that utilizes this library.

## Compilation
To compile the project (both the library and the command-line utility), simply execute:
```sh
make all
```
(or just `make`)

To build only the `libequalff.so` dynamic library:
```sh
make library
```

To compile and run the internal test harness for the library:
```sh
make -f Makefile.test run
```

## Installation

To install the compiled `equalff` binary and its man page, run:
```sh
make install
```
This installs the `equalff` utility to `/usr/local/bin` (by default) and the man page.
**Note:** This command does **not** install the `libequalff.so` dynamic library into a system-wide directory (e.g., `/usr/local/lib`). If you intend to develop other programs that link against `libequalff.so`, you will need to ensure the library and its headers (in the `lib/` directory) are accessible to your compiler and linker, or install them manually to appropriate system locations.

By default, this will install `equalff` to `/usr/local/bin` and the man page to `/usr/local/share/man/man1`. You can specify a different installation prefix by setting the `PREFIX` variable:

```sh
make install PREFIX=~/.local
```

To uninstall, run:

```sh
make uninstall
```

(If you used a custom `PREFIX` during installation, you need to specify it for uninstallation as well.)

## Usage
```
Usage: ./equalff [OPTIONS] <DIRECTORY> [DIRECTORY]...
Find duplicate files in FOLDERs according to their content.

Mandatory arguments to long options are mandatory for short options too.
  -f, --same-fs             process files only on one filesystem
  -s, --follow-symlinks     follow symlinks when processing files
  -b, --max-buffer=SIZE     maximum memory buffer (in bytes) for file comparison (default 8192, min 128)
  -o, --max-of=COUNT        Set the maximum number of open files (default is FOPEN_MAX, typically 20). If 0 or negative, adapts to the number of files in a comparison group.
  -m, --min-file-size=SIZE  check only file with size grater or equal to size (default 1)
  -h, --help                Display this help message and exit
```

**Example:**
```
$ equalff ~/
```
**Result:**
```
Looking for files ... 123 files found
Sorting ... done.
Starting fast comparsion.
...
file1-location1
file1-location2
file1-location3

file2-location1
file2-location2
...
Total files being processed: 123
Total files being read: 10
Total equality clusters: 2
```

### Algorithm
- An **equality cluster** is a set of files that are identical at a certain comparison stage.
1. Files are sorted by size in ascending order.
1. Files of the same size are added to an equality cluster. (Clusters with only one file are skipped.)
1. Files in an equality cluster are compared in byte-blocks, starting from the beginning of the file.
1. Based on the comparison results, the equality cluster is divided into smaller clusters using the union-find algorithm with a compressed structure.
1. The comparison process is repeated until the end of the files.
1. Finally, the clusters are printed to stdout.

## Pros and Cons
**Pros**
- Generally faster than other tools as it only reads necessary bytes(blocks).
- Each file is read only once.
- Does not use a hash algorithm, reducing processor computation.
- Core logic available as a reusable dynamic library (`libequalff.so`).

**Cons**
- Does not compute file content hash, so results cannot be reused.
- Has some memory limitations, making it unsuitable for systems with limited memory.
- Does not read the last bytes in the first comparison stage, where the probability of inequality is high, slightly slowing down the process.
- On some filesystems (like FAT), it's not possible to open more than 16 files at once. This can be adjusted with the **--max-of** option.
- Not tested with hardlinks and sparse files.
- Not parallelized.
- May be slower than other utilities on non-SSD disks due to file fragmentation. See [#1](https://github.com/jhkst/equalff/issues/1)

## Using the Library (libequalff)

The core duplicate finding logic is available in `lib/libequalff.so`. This library provides functions to integrate duplicate detection into your own C projects.

1.  **Headers**: Include `fcompare.h` from the `lib/` directory. This header contains the function prototypes and type definitions for the public API.
2.  **Compilation & Linking**:
    *   Add an include path for the library's headers: `-I/path/to/equalff-lib/lib`
    *   Add a library path for linking: `-L/path/to/equalff-lib/lib`
    *   Link against the library: `-lequalff`
    *   You may also need to set an `rpath` for the executable to find `libequalff.so` at runtime if it's not in a standard system location, or use `LD_LIBRARY_PATH` (Linux) / `DYLD_LIBRARY_PATH` (macOS).

### Synchronous API

The primary synchronous function is `compare_files`:

```c
ComparisonResult* compare_files(
    char *name[],         // Array of file paths to compare.
    int count,            // Number of files in the name array.
    int max_buffer,       // Maximum memory buffer (bytes) per file for comparison.
    int max_open_files    // Max number of simultaneously open files.
);
```

This function processes the given files and returns a `ComparisonResult` structure containing all sets of duplicate files found. The caller is responsible for freeing this structure using `free_comparison_result()`.

```c
void free_comparison_result(ComparisonResult *result);
```

Refer to `test_harness.c` for an example of how to call these functions.

### Asynchronous (Callback-based) API

For applications that prefer to process duplicate sets as they are identified (after all file content has been compared), an asynchronous-style API is available via `compare_files_async`.

```c
// Callback function signature
typedef void (*DuplicateFoundCallback)(const DuplicateSet *duplicates, void *user_data);

int compare_files_async(
    char *file_paths[],             // Array of file paths to compare.
    int count,                      // Number of files in the file_paths array.
    int max_buffer_per_file,        // Max memory buffer (bytes) per file for comparison.
    int max_open_files,             // Max number of simultaneously open files.
    DuplicateFoundCallback callback,  // Callback function invoked for each duplicate set.
    void *user_data,                // Arbitrary data pointer passed to the callback.
    char **error_message_out        // Out-parameter for error message string.
);
```

**Key features and behavior:**
*   The `compare_files_async` function performs the full comparison of all provided files.
*   As each complete set of duplicate files is identified (i.e., files confirmed to be identical to their EOF), the provided `callback` function is invoked.
*   The `DuplicateSet` pointer passed to the callback, and the file paths within it, are valid **only for the duration of the callback**. If you need to retain this information, you must copy it within your callback implementation.
*   The function returns `0` on success. If a non-zero value is returned, an error occurred. In this case, `error_message_out` may point to an allocated string describing the error. This error string must be freed by the caller.

To free an error message string obtained from `compare_files_async` (or other future API functions that might use this pattern), use `free_error_message()`:

```c
void free_error_message(char *error_message);
```

This callback-based approach allows for more flexible handling of results, especially if you don't need to collect all duplicate sets in memory at once before processing them.

### Data Structures

*   `DuplicateSet`: Represents a single set of duplicate files.
    ```c
    typedef struct {
        char **paths;       // Array of file path strings
        int count;          // Number of paths in this set
    } DuplicateSet;
    ```
*   `ComparisonResult` (used by synchronous API):
    ```c
    typedef struct {
        DuplicateSet *sets; // Array of DuplicateSet structures
        int count;          // Number of duplicate sets found
        int error_code;     // 0 on success, non-zero on error
        char *error_message; // Description of the error (must be freed if not NULL by free_comparison_result)
    } ComparisonResult;
    ```

Refer to `test_harness.c` for examples of using both the synchronous and asynchronous APIs.


### Comparison
The following table shows results from a benchmark run on a consistent dataset using the tools configured in `benchmark.py`.

| Utility Name | Language | Homepage | Exact Command | Time (seconds) (*) |
|--------------|----------|----------|---------------|--------------------|
| equalff | C | [equalff](https://github.com/jhkst/equalff) | `equalff <DIRECTORY>` | 0.190 |
| fdupes | C | [adrianlopezroche/fdupes](https://github.com/adrianlopezroche/fdupes) | `fdupes -r <DIRECTORY>` | 0.680 |
| rdfind | C++ | [rdfind.pauldreik.se](https://rdfind.pauldreik.se/) | `rdfind -makeresultsfile false <DIRECTORY>` | 0.580 |
| fslint_findup | Python | [pixelbeat.org/fslint]() | `/usr/share/fslint/fslint/findup <DIRECTORY>` | 0.900 |
| duff | C | [elmindreda/duff](https://github.com/elmindreda/duff) | `duff -r -a <DIRECTORY>` | 0.680 |
| ssdeep | C/C++ | [ssdeep-project.github.io](https://ssdeep-project.github.io/ssdeep/index.html) | `ssdeep -r -l -p <DIRECTORY>` | 22.790 |
| finddup_perforate | C | [perforate-linux (SF)](https://sourceforge.net/projects/perforate-linux/) | `finddup` | 3.230 |
| freedups | Perl | [stearns.org/freedups](http://www.stearns.org/freedups/) | `freedups <DIRECTORY>` | 2.760 |
| rmdupe_shell | Shell | [IgnorantGuru/rmdupe](https://github.com/IgnorantGuru/rmdupe) | `rmdupe_shell -R --sim <DIRECTORY>` | Timeout |
| dupseek_script | Perl | [beautylabs.net/dupseek](http://www.beautylabs.net/software/dupseek.html) | `dupseek_script <DIRECTORY>` | 0.860 |
| fdf_harski | C | [harski/fdf](https://github.com/harski/fdf) | `/usr/local/bin/fdf <DIRECTORY>` | 17.020 |

(\*) Time in seconds for a single execution on the test dataset. The dataset is regenerated for each full run of the `benchmark.py` script.<br>

### Licensing
See the [LICENSE](LICENSE) file for licensing information.

### Plans
Plans are in place to add selection and action features for found clusters, such as "remove last modified", "remove first modified", "keep files starting on path", etc.