# equalff

**equalff** is fast equal (duplicate) files finder. It uses different algorithm than other duplicate files finders and therefore is faster then others for one run*.

### Compile
To compile it simply run **make** in the project root.

### Usage
```
Usage: equalff [OPTIONS] <FOLDER> [FOLDER]...
Find duplicate files in FOLDERs according to their content.

Mandatory arguments to long options are mandatory for short options too.
  -f, --same-fs             process files only on one filesystem
  -s, --follow-symlinks     follow symlinks when processing files
  -b, --max-buffer=SIZE     maximum memory buffer (in bytes) for files comparing
  -o, --max-of=COUNT        force maximum open files (default 16)
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

file1-location1
file1-location2
file1-location3

file2-location1
file2-location2

Total files being processed: 123
Total files being read: 10
Total equality clusters: 2
```

### Advantages/Disadvantages
**Advantages**
- It's really faster than others, it reads only those bytes(blocks) which are necessary.

**Disadvantages**
- It does not compute hash of file content so the result cannot be reused
- There are some memory limits (so not suitable for tiny systems).

### Comparsion
* TBD

### Licensing
see [LICENSE](LICENSE) file
