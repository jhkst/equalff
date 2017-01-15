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

### Advantages/Disadvantages
**Advantages**
- It's really faster than others, it reads only those bytes(blocks) which are necessary.

**Disadvantages**
- It does not compute hash of file content so the result cannot be reused.
- There are some memory limits (so not suitable for tiny systems).

### Comparsion
TBD

| utility name | language | exact command | average time(*) |
|--------------|----------|---------------|-----------------|
| [fslint](http://www.pixelbeat.org/fslint/)                | python |                 |
| [dupedit](http://www.pixelbeat.org/fslint/)                | python |                 |
| [fdupes](https://github.com/adrianlopezroche/fdupes)      | C      |                 |
| [finddup](http://finddup.sourceforge.net/)                | perl   |                 |
| [rmdupe](https://github.com/IgnorantGuru/rmdupe)          | shell  |                 |
| [dupmerge](https://sourceforge.net/projects/dupmerge/)    | C      |                 |
| [dupseek](http://www.beautylabs.net/software/dupseek.html)| perl   |                 |
| [fdf](https://github.com/harski/fdf)                      | C      |                 |
| [freedups](http://www.stearns.org/freedups/)              | perl   |                 |
| [freedup](http://freedup.org/)                            | C      |                 |           
| [liten](https://code.google.com/archive/p/liten/)         | python |                 |
| [liten2](https://code.google.com/archive/p/liten2/)                | python |                 |
| [rdfind](https://rdfind.pauldreik.se/)                    | C++    |                 |
| [ua](https://github.com/euedge/ua)                        | C++    |                 |
| [findrepe](https://github.com/franci/findrepe)            | Java   |                 |
| [fdupe](https://neaptide.org/projects/fdupe/)             | perl   |                 |
| [ssdeep](http://ssdeep.sourceforge.net/)(**)              | C/C++  |                 |
| [duff](http://duff.dreda.org/)                            | C      |                 |
| equalff                                                   | C      |                 |

(\*) average time of multiple runs on one path<br>
(**) probably not exact output (to be checked)

### Licensing
see [LICENSE](LICENSE) file

### Plans
I've plans to add selection and action features on found clusters like
"remove last modified", "remove first modified", "keep files starting on path", etc...