<p align="center">
  <img src="screenshot/logo640.jpg" alt="ghostmirror logo" width="160">
</p>

ghostmirror v0.15.2
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

![Alt text](screenshot/simple_progress.png?raw=true "GhostMirrorProgress")
![Alt text](screenshot/simple_output.png?raw=true "GhostMirrorOutput")

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
```bash
# cp ./mirrorlist.new /etc/pacman.d/mirrorlist
```
### Direct replace mirrorlist
If you don't want to create a temporary file and prefer to modify the mirrorlist directly, you can run ghostmirror with sudo on the mirrorlist itself.<br>
the first and second step are same but with sudo and different mirror file<br>
only first time, for generate personal mirrorlist
```bash
$ sudo ghostmirror -PoclLS Italy,Germany,France /etc/pacman.d/mirrorlist 30 state,outofdate,morerecent,ping
```
every time you need refresh
```bash
$ sudo ghostmirror -PmuolsS  /etc/pacman.d/mirrorlist /etc/pacman.d/mirrorlist light state,outofdate,morerecent,extimated,speed
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
set connection timeout in seconds for not reply mirror
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

Bug:
====
try to writing many


