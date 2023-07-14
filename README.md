# equalff

**equalff** is fast equal (duplicate) files finder. It uses different algorithm than other duplicate files finders and therefore is faster then others for one run.

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
<stderr>Looking for files ... 123 files found
<stderr>Sorting ... done.
<stderr>Starting fast comparsion.
...
<stdout>file1-location1
<stdout>file1-location2
<stdout>file1-location3
<stdout>
<stdout>file2-location1
<stdout>file2-location2
...
<stderr>Total files being processed: 123
<stderr>Total files being read: 10
<stderr>Total equality clusters: 2
```

### Algorithm
- **equality cluster** is a set of files which are equal in a stage of comparison
1. files are listed and sorted by size (ascending)
1. if the files are equal in size, they are added to equality cluster. (if there is only one file in equality cluster it's skipped)
1. files in equality cluster are compared in byte-blocks (from the beginning of the file)
1. depending on the result of block comparison the equality cluster is split into smaller clusters using the union-find algorithm with compressed structure
1. comparing is repeated to the end of files
1. finally clusters are printed to stdout

### Advantages/Disadvantages
**Advantages**
- It's mostly really faster than others, it reads only those bytes(blocks) which are necessary.
- All files are read no more than once.
- No hash algorithm is used (less processor computation:)

**Disadvantages**
- It does not compute hash of file content so the result cannot be reused.
- There are some memory limits (so not suitable for tiny systems).
- It does not read last bytes in first stage of comparison, where the probability of non-equality is high. This slows down the process a little bit.
- On some filesystems (like FAT) it's not possible to open more than 16 files at once. This can be changed by **--max-of** option.
- Not tested with hardlinks and sparse files.
- Not parallelized.
- On non-SSD disks it may be slower than others utilities (because of file fragmentation). See [#1](https://github.com/jhkst/equalff/issues/1)

### Algorithm
- files are 

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

(\*) average time of multiple runs on one path<br>
(**) probably not exact output (to be checked)

### Licensing
see [LICENSE](LICENSE) file

### Plans
I've plans to add selection and action features on found clusters like
"remove last modified", "remove first modified", "keep files starting on path", etc...
