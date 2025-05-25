ghostmirror v0.10.5
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

![Alt text](screenshot/simple_output.png?raw=true "GhostMirror")

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

Usage:
======
You can use the software in three ways: manually, automatically, or for investigation.
## Manually
In this mode, you will perform all the steps manually.
### First step
in the first step you need to search a good quantity of mirrors.<br>
-P for get progress and output colors, -o for get output table.<br>
-c for select country, for example Italy,Germany,France<br>
-l where do you save list<br>
-L max numbers of output mirror in list<br>
-S sort mode, first add state, in this mode remove error mirror, after you can add outofdate, in this mode display first the mirror sync, can also add morerecent to ensure you never go out of sync, and at the end, you can add ping to try to prioritize the closest ones.
```bash
$ ghostmirror -PoclLS Italy,Germany,France ./mirrorlist.new 30 state,outofdate,morerecent,ping
```
### Second step
Now, instead of taking the mirrors from the global list, we will better evaluate the mirrors found in the first step.<br>
While the first step will be performed only once or rarely, this step will be the one you repeat periodically.<br>
-P -o -l is same the previous step<br>
-m tell software to use a local mirror list, and -u for use only uncommented mirror<br>
-s for apply a real test for mirror speed<br>
-S change a sort mode, remove ping, add extimated for get more stable mirror and speed for reorder speed
```bash
$ ghostmirror -PmuolsS  ./mirrorlist.new ./mirrorlist.new light state,outofdate,morerecent,extimated,speed
```
### Last step
now you save your old mirrorlist
```bash
# cp /etc/pacman.d/mirrorlist /etc/pacman.d/mirrorlist.bak
```
now save new mirrorlist
```
# cp ./mirrorlist.new /etc/pacman.d/mirrorlist
```

## Automatically
In this mode, you will perform the second step automatically.
### Prepare
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
### First step
same manually mode you need create a big good mirrorlist
```bash
$ ghostmirror -PoclLS Italy,Germany,France ~/.config/ghostmirror/mirrorlist 30 state,outofdate,morerecent,ping
```
### Last step
the arguments,lsS, you pass to ghostmirror at this point it's saved for auto reuse it in service.<br>
so, the only difference with manually second step is -D option, this enable ghostmirror.timer and loginctl linger<br>
```bash
$ ghostmirror -PoDumlsS  ~/.config/ghostmirror/mirrorlist ~/.config/ghostmirror/mirrorlist light state,outofdate,morerecent,extimated,speed
```
### Use of systemd
show the timer
```bash
$ systemctl --user list-timers
```
force start ghostmirror before timer ellapsed
```bash
$ systemctl --user start ghostmirror.service
```

### End
Now you can forget about mirrors forever.

## Investigation
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
## All Options
### -a --arch <required string>
select arch, default 'x86_64'
### -m --mirrorfile <required string>
use mirror file instead of downloading mirrorlist, without -m and -u ghostmirror download mirrorlist and search in all mirror.<br>
you can use -m /etc/pacman.d/mirrorlist.pacnew if you not want downloading mirrorlist but used local list.<br>
### -c --country <required string>
select country from mirrorlist.<br>
if mirrorlist is not same the default mirrorlist and not have ##Country or mixed mirror, the selection country fail
### -C --country-list <not required argument>
show all possibile country
### -u --uncommented <not required argument>
use only uncommented mirror, by default use commented and uncommented mirror.<br>
with -m is simple to check local mirrolist  -mu /etc/pacman.d/mirrorlist.
### -d --downloads <required unsigned integer>
set numbers of parallel download, default '4'.<br>
if you abuse the download is more simple to fail, 1,4,8,16 is good value, >16 you can try but is not very affidable
### -O --timeout <required unsigned integer>
set timeout in seconds for not reply mirror, default 5s
### -p --progress <not required argument>
show progress, default false
### -P --progress-colors <not required argument>
same -p but with -o add color table
### -o --output <not required argument>
enable table output, with -P display with colors
### -s --speed <required string>
test speed for downloading:<br>
light: download one small package ~6MiB<br>
normal: download light + normal package ~250Mib<br>
heavy: download light+normal+heavy packege ~350MiB, total download >500Mib
### -S --sort <required string>
sort result for any of fields display in table, mutiple fields supported.<br>
country and mirror is sorted by name<br>
proxy first false, last true<br>
state first success last error<br>
outofdate, retry and ping, display first less value<br>
uptodate, morerecent, sync, speed and extimated, display first great value<br>
### -l --list <required string>
save new mirrorlist in file passed as argument.<br>
special name, stdout, can be used for write to stdout file.
### -L --max-list <required unsigned integer>
set max numbers of output mirrors
### -T --type <required string>
select mirrors type, http,https,all
### -i --investigate <required string>
search mirror errors to detect the problem.<br>
can select mode: error, outofdate, all.<br>
error: investigate only on error.<br>
outofdate: investigate only on outofdate package.<br>
all: same passing -i error,outofdate
### -D --systemd <not required argument>
auto manager systemd.timer.<br>
when you pass this option the software activate login linger if not ebabled.<br>
auto create ghostmirror.service and ghostmirror.timer<br>
the config.service start ghostmirror in the same mode you haved executed it, with only differences that need -l.<br>
for exaples if you execute: -DmuldsS <mirrorlist> <mirrorlist> 16 light extimated,speed<br>
the service is always start with <mirrorlist> 16 parallel downloads, speed light and extimated,speed sort.<br>
for change you can simple repeat a command.<br>
the expire timer is the first element in table and is dinamic, can change every time.<br>
### -t --time <required string>
in systemd timer whend extimate date is ellapsed can set a time. Set specific hh:mm:ss when start service, default 00:00:00.<br>
validate input with systemd-analyzer calendar hh:mm:ss before use
### -f --fixed-time <required string>
in systemd timer use fixed time instead of extimated time.<br>
validate input with systemd-analyzer calendar hh:mm:ss before use
### -h --help <not required argument>
display this

Manual Build
============
This section is for developer

## Require:
libcurl, zlib, systemd-libs

## Build:
```bash
$ meson setup build
$ cd build
$ ninja
```

### Debug:
for enable very verbose output.
```bash
$ meson configure -Debug=4
```
warning this is only for contributor, enable auto versioning and auto push
```bash
$ meson configure -Developer=true
```


State:
======
* v0.10.5 default tout to 0 for less spurious value, try to download when server not reply to ping some server not reply to ping, compare only exists package
* v0.10.4 issue(13) fix systemd evnironment need to manually intervent, manual stop service, manually delete systemd user ghostmirror.service, recall step ghostmirror -D; fix zstream check error freeze decompression; add investigate malicius redirect from corrupted data and malformed url
* v0.10.3 investigate: add check internet connection, fix investigate not display all mirror, add ability do display investigate when all mirrors fail. fix retry is not updated
* v0.10.2 removed sync field, is not portable and not have very good information, add more performance, change default timeout to 8s
* v0.10.1 permanently removed the use of local database. with systemd if it fails for 5 attempts it will stop and try again the following day
* v0.10.0 from issue 6, ghostmirror required the first mirror to be working in order to have a database similar to the one on your PC in order to perform the comparison. Given the developments this is no longer necessary and an alternative mirror will now be sought
* v0.9.22 fix path_explode ..
* v0.9.21 fix pkgbuild update after git, fix man documentation
* v0.9.20 bash completion(request 4), fix environment(issue 5)
* v0.9.19 merge man page, check on generate man page, test.
* v0.9.18 returned to the old method of the pre-calculated and fixed date in the calculation of time, theoretically it should never fail, I had unnecessarily complicated things
* v0.9.17 fix unable restart unit if not file exists
* v0.9.16 systemd oncalendar does not work as expected, changed the time calculation and forcing method, remove warning sign
* v0.9.15 systemd oncalendar does not work as expected, changed the time calculation and forcing method
* v0.9.14 custom time and fixed time, specific option for path, wrong indentation
* v0.9.13 pkgbuild doc, thanks, fix possible issue by scan-build, valgrind success (only remain memory when exit from software, its not problem), this is first candidate for stable version
* v0.9.12 more doc and wiki.
* v0.9.11 the prev version not build, missing gm.h. add screenshot.
* v0.9.10 little more doc, change conf for waiting nss-lockup.service, removed opt show unknonw option at end of argument, opt usage show is array
* v0.9.9 support all flags to systemd service
* v0.9.8 more precise and elegant stability
* v0.9.7 there was a piece of test code left
* v0.9.5 strong systemd: better config for autorestart, auto use local db. get local pacman database if many fail remote local database. can use manually RESTART_COUNT=999. Versioning config for automatic reconfigure when new version is released
* v0.9.4 doc have a problem, need to write the doc
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


