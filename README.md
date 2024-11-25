ghostmirror v0.3.0
==================
Introduction:
=============
We've all experienced moments when there seemed to be no packages to update, only to realize days later that the issue was an out-of-sync mirror.<br>
Despite the numerous tools available on Arch Linux for managing mirrors, none of them fully met my expectations.<br>
So, I set out to solve this problem.

Description:
============
What does GhostMirror do?
It compares the mirror databases with the local database and provides a detailed description of whether the mirror's packages are more or less up-to-date compared to our local database

State:
======
* v0.3.0 sorting result
* v0.2.2 more stable, removed useless code, auto versioning.
* v0.2.1 check current mirror used and show
* v0.2.0 use mirrorlist for get current remote mirror in use in this mode not required -Syu, calcolate speed mirror
* v0.1.0 remove useless options, check if server have all files in db, little better output
* v0.0.0 first alpha version, be careful

Bug:
====
try to writing many

Require:
========
libcurl, zlib 

Build:
======
```bash
$ meson setup build
$ cd build
$ ninja
```

Install:
========
for now is better to run local with
```bash
$ cd build
$ ./ghostmirror ......
```
but if you are crazy you can
```bash
$ sudo ninja install
```

Usage:
======
-h --help for get app options<br>
example, check all Italy mirrors<br>
```bash
$./build/ghostmirror -cp Italy
```
example, check all Italy mirrors with speed test<br>
```bash
$./build/ghostmirror -cps Italy
```
 example, check all Italy and Germany mirrors with speed test<br>
```bash
$./build/ghostmirror -cps Italy,Germany
```
 







