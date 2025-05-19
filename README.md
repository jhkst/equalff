# Equalff

**Equalff** is a high-speed duplicate file finder. It employs a unique algorithm, making it faster than other duplicate file finders for a single run.

## Compilation
To compile the project, simply execute **make** in the project root.

## Installation

To install the compiled binary and its man page, run:

```sh
make install
```

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
  -b, --max-buffer=SIZE     maximum memory buffer (in bytes) for files comparing
  -o, --max-of=COUNT        force maximum open files (default 20, 0 for indefinite open files)
  -m, --min-file-size=SIZE  check only file with size grater or equal to size (default 1)
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

**Cons**
- Does not compute file content hash, so results cannot be reused.
- Has some memory limitations, making it unsuitable for systems with limited memory.
- Does not read the last bytes in the first comparison stage, where the probability of inequality is high, slightly slowing down the process.
- On some filesystems (like FAT), it's not possible to open more than 16 files at once. This can be adjusted with the **--max-of** option.
- Not tested with hardlinks and sparse files.
- Not parallelized.
- May be slower than other utilities on non-SSD disks due to file fragmentation. See [#1](https://github.com/jhkst/equalff/issues/1)


### Comparison
TBD

| utility name                                               | language | exact command | average time(*) |
|------------------------------------------------------------|----------|---------------|-----------------|
| [fslint](http://www.pixelbeat.org/fslint/)                 | python   |               |                 |
| [dupedit](http://www.pixelbeat.org/fslint/)                | python   |               |                 |
| [fdupes](https://github.com/adrianlopezroche/fdupes)       | C        |               |                 |
| [finddup](http://finddup.sourceforge.net/)                 | perl     |               |                 |
| [rmdupe](https://github.com/IgnorantGuru/rmdupe)           | shell    |               |                 |
| [dupmerge](https://sourceforge.net/projects/dupmerge/)     | C        |               |                 |
| [dupseek](http://www.beautylabs.net/software/dupseek.html) | perl     |               |                 |
| [fdf](https://github.com/harski/fdf)                       | C        |               |                 |
| [freedups](http://www.stearns.org/freedups/)               | perl     |               |                 |
| [freedup](http://freedup.org/)                             | C        |               |                 |
| [liten](https://code.google.com/archive/p/liten/)          | python   |               |                 |
| [liten2](https://code.google.com/archive/p/liten2/)        | python   |               |                 |
| [rdfind](https://rdfind.pauldreik.se/)                     | C++      |               |                 |
| [ua](https://github.com/euedge/ua)                         | C++      |               |                 |
| [findrepe](https://github.com/franci/findrepe)             | Java     |               |                 |
| [fdupe](https://neaptide.org/projects/fdupe/)              | perl     |               |                 |
| [ssdeep](http://ssdeep.sourceforge.net/)(**)               | C/C++    |               |                 |
| [duff](http://duff.dreda.org/)                             | C        |               |                 |
| equalff                                                    | C        |               |                 |

(\*) Average time of multiple runs on one path<br>
(**) Output may not be exact (to be checked)

### Licensing
See the [LICENSE](LICENSE) file for licensing information.

### Plans
Plans are in place to add selection and action features for found clusters, such as "remove last modified", "remove first modified", "keep files starting on path", etc.
