ghostmirror v0.9.3
==================
Introduction:
=============
We've all experienced moments when there seemed to be no packages to update, only to realize days later that the issue was an out-of-sync mirror.<br>
Despite the numerous tools available on Arch Linux for managing mirrors, none of them fully met my expectations.<br>
So, I set out to solve this problem.<br>
While writing the software, I realized that by continuously adding features, ghostmirror has become a great tool for the mirror maintainers themselves.<br>
In fact, besides displaying mirrors that have errors, it can investigate and show the possible causes that generated the error.

Description:
============
What does GhostMirror do?<br>
It compares the mirror databases with the local database and provides a detailed description of whether the mirror's packages are more or less up-to-date compared to our local database.<br>
It can analyze the mirrors and display in-depth errors or the names of packages that are not updated.<br>
Thanks to the custom sorting mode, it can create a list of mirrors based on each user's needs.<br>
If you don't have time to update the mirror list manually, by adding a single command-line argument, the systemd service will be automatically activated. Feel free to forget about the mirrors.<br>

Install:
========
You can use your favorite AUR helper to install it.
```bash
$ yay -S ghostmirror
```
otherwise you can find PKGBUILD in distro dir
```bash
$ cd distro && makepkg -sirc
```

Manual Build
============
This section is for developer

##Require:
libcurl, zlib, systemd-libs

##Build:
```bash
$ meson setup build
$ cd build
$ ninja
```

###Debug:
for enable very verbose output.
```bash
$ meson configure -Debug=4
```
warning this is only for contributor, enable auto versioning and auto push
```bash
$ meson configure -Developer=true
```

Usage:
======
You can use the software in three ways: manually, automatically, or for investigation.
##Manually
In this mode, you will perform all the steps manually.
###First step
in the first step you need to search a good quantity of mirrors.<br>
-P for get progress and output colors, -o for get output table.<br>
-c for select country, for example Italy,Germany,France<br>
-l where do you save list<br>
-L max numbers of output mirror in list<br>
-S sort mode, first add state, in this mode remove error mirror, after you can add outofdate, in this mode display first the mirror sync, can also add morerecent to ensure you never go out of sync, and at the end, you can add ping to try to prioritize the closest ones.
```bash
$ ghostmirror -PoclLS Italy,Germany,France ./mirrorlist.new 30 state,outofdate,morerecent,ping
```
###Second step
Now, instead of taking the mirrors from the global list, we will better evaluate the mirrors found in the first step.<br>
While the first step will be performed only once or rarely, this step will be the one you repeat periodically.<br>
-P -o -l is same the previous step<br>
-m tell software to use a local mirror list, and -u for use only uncommented mirror<br>
-s for apply a real test for mirror speed<br>
-S change a sort mode, remove ping ad add extimated for get more stable mirror and speed for reorder speed
```bash
$ ghostmirror -PmuolsS  ./mirrorlist.new ./mirrorlist.new light state,outofdate,morerecent,extimated,speed
```
###Last step
now you save your old mirrorlist
```bash
# cp /etc/pacman.d/mirrorlist /etc/pacman.d/mirrorlist.bak
```
now save new mirrorlist
```
# cp /etc/pacman.d/mirrorlist /etc/pacman.d/mirrorlist
```

##Automatically
In this mode, you will perform the second step automatically.
###Prepare
you need manual make dir for a new location of mirrorlist, need location where user can edit this without root privilege
```bash
$ mkdir ~/.config/ghostmirror
```
now you need to inform pacman where you have stored mirrorlist<br>
edit file /etc/pacman.conf, search and replace this line and change <username> with your username.
```
[core]
Include = /home/<username>/.config/ghostmirror/mirrorlist

[extra]
Include = /home/<username>/.config/ghostmirror/mirrorlist
```
###First step
same manually mode you need create a big good mirrorlist
```bash
$ ghostmirror -PoclLS Italy,Germany,France ~/.config/ghostmirror/mirrorlist 30 state,outofdate,morerecent,ping
```
###Last step
the arguments,lsS, you pass to ghostmirror at this point it's saved for auto reuse it in service.<br>
so, the only difference with manually second step is -D option, this enable ghostmirror.timer and loginctl linger<br>
```bash
$ ghostmirror -PoDumlsS  ~/.config/ghostmirror/mirrorlist ~/.config/ghostmirror/mirrorlist light state,outofdate,morerecent,extimated,speed
```
Now you can forget about mirrors forever.

##Investigation
It's be simple, add -i option and it's show the list of error server with motivation.

Reference
=========
the software accept long options with -- and assign value without space but with =
```bash
--option
--option value
--option=value
```
accept short option with - or multiple option, followed by value
```bash
-o
-o value
-abc valueA valueB valueC
```
-a --arch <required string><br>
    select arch, default 'x86_64'<br>
-m --mirrorfile <required string><br>
    use mirror file instead of downloading mirrorlist<br>
-c --country <required string><br>
    select country from mirrorlist<br>
-C --country-list <not required argument><br>
    show all possibile country<br>
-u --uncommented <not required argument><br>
    use only uncommented mirror<br>
-d --downloads <required unsigned integer><br>
    set numbers of parallel download, default '4'<br>
-O --timeout <required unsigned integer><br>
    set timeout in seconds for not reply mirror, default '20's<br>
-p --progress <not required argument><br>
    show progress, default false<br>
-P --progress-colors <not required argument><br>
    same -p but with colors<br>
-o --output <not required argument><br>
    enable table output<br>
-s --speed <required string><br>
    test speed for downloading one pkg, light, normal, heavy<br>
-S --sort <required string><br>
    sort result for any of fields, mutiple fields supported<br>
-l --list <required string><br>
    create a file with list of mirrors, stdout as arg for output here<br>
-L --max-list <required unsigned integer><br>
    set max numbers of output mirrors<br>
-T --type <required string><br>
    select mirrors type, http,https,all<br>
-i --investigate <not required argument><br>
    search mirror errors to detect the problem<br>
-D --systemd <not required argument><br>
    auto manager systemd.timer<br>
-h --help <not required argument><br>
    display this<br>



State:
======
* v0.9.3 doc have a problem, need to write the doc
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


