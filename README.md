ghostmirror v0.9.2
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
* v0.9.2 local mirror now is get from correct but not perfect location
* v0.9.1 unmask service, mkalldir, wrong argument to sd-bus, end first systemd test
* v0.9.0 warning this version not have systemd tested, change option name, t->d, o->O, o->table output, D->systemd linger, tar pax error, sort stability->extimated
* v0.8.4 add internal error for more precise investigation and better output, fix sync interrupt threads not initialized, change another time time out
* v0.8.3 investigation, add basic support for simple server test, not affidable need more and more test
* v0.8.2 ping field and sort, better status sort, prepare for investigation
* v0.8.1 merge wwmdownload now resolve follow location, add proxy in sort and table, fatal error if not downloaded local database
* v0.8.0 concept of stability, full sort this fix die with speed, internal rfield back to struct field, www followlocation, dynamic time retry, default tout 20
* v0.7.1 fix wrong -T description, set max numbers of output mirrors, unique mirrors.
* v0.7.0 add speed type, light, normal, heavy. fix speed benchmark delay to double
* v0.6.0 add select mirror type, http or https or all
* v0.5.8 fix sort to manage new field, not tested
* v0.5.7 merge newversion and notexists in morerecent, highlighted local
* v0.5.6 assertion boolean value
* v0.5.5 fix assertion isheap not exists
* v0.5.4 some testing
* v0.5.3 upload on aur
* v0.5.2 colorized output, switch to array fields, compact table code, remove some magic
* v0.5.1 better progress, if not country set mirrors country is search on remote mirrorlist
* v0.5.0 add all fields sorting
* v0.4.2 only for developer, auto follow tags
* v0.4.1 mem nullterm, info stored in list, meson developer message
* v0.4.0 --list create a list with Server=mirror, country+retry in info
* v0.3.1 autogit, first tag
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
 







