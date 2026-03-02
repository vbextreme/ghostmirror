# ghostmirror v0.18.5

<p align="center">
  <img src="screenshot/logo640.jpg" alt="ghostmirror logo" width="160">
</p>

## **Introduction:**
We've all experienced moments when there seemed to be no packages to update, only to realize days later that the issue was an out-of-sync mirror.
Despite the numerous tools available on Arch Linux for managing mirrors, none of them fully met my expectations.
So, I set out to solve this problem.
While writing the software, I realized that by continuously adding features, ghostmirror has become a great tool for the mirror maintainers themselves.
In fact, besides displaying mirrors that have errors, it can investigate and show the possible causes that generated the error.

## **Description:**
What does GhostMirror do?
It compares the mirror databases with the local database and provides a detailed description of whether the mirror's packages are more or less up-to-date compared to our local database.
It can analyze the mirrors and display in-depth errors or the names of packages that are not updated.
Thanks to the custom sorting mode, it can create a list of mirrors based on each user's needs.
If you don't have time to update the mirror list manually, by adding a single command-line argument, the systemd service will be activated automatically. Feel free to forget about the mirrors.

![Alt text](screenshot/simple_progress.png?raw=true "GhostMirrorProgress")
![Alt text](screenshot/simple_output.png?raw=true "GhostMirrorOutput")

# Install:
Use your favorite AUR helper to install it.

```bash
yay -S ghostmirror
```

Otherwise you can find PKGBUILD in the distro directory and copy PKGBUILD to a local directory like ~/build
cd into that directory and run:

```bash
makepkg -sirc
```

# Usage:
You can use the software in three ways: manually, automatically, or for investigation.

## Manually
In this mode, you will perform all the steps manually.
### First step
In the first step you need to generate a larger (global) list of mirrors.
* -P to get progress and output colors, -o to output as a table
* -c to select country, for example Italy,Germany,France,"United States" (no spaces, just comma between names)
* -l for location of the file, where to save the list
* -L max number of output (mirrors in list)
* -S sort mode: can be combined (see example below), state - in this mode remove mirrors with errors, outofdate - in this mode display first the mirror sync, morerecent - to ensure you never go out of sync, ping - to prioritize the closest ones

```bash
ghostmirror -PoclLS Italy,Germany,France ./mirrorlist.new 30 state,outofdate,morerecent,ping
```

### Second step
Now, working from the saved list from the above command, we will further evaluate the mirrors.
While the first step will be performed only once or rarely, this step will be the one you repeat periodically.
* -P -o -l is same the previous step
* -m tell software to use a local mirror list, and -u when using an uncommented mirrorlist
* -s to apply a real test for mirror speed (see All Options below)
* -S change sort mode, remove ping, add estimated - to get a more stable mirror, add speed - to reorder by speed

```bash
ghostmirror -PmuolsS  ./mirrorlist.new ./mirrorlist.new light state,outofdate,morerecent,estimated,speed
```

### Last step
Now backup your old mirrorlist and move the temporary mirrorlist to /etc/pacman.d/mirrorlist

```bash
cp /etc/pacman.d/mirrorlist /etc/pacman.d/mirrorlist.bak
cp ./mirrorlist.new /etc/pacman.d/mirrorlist
```

## Directly replace mirrorlist
If you don't want to create a temporary file, you can run ghostmirror with sudo copying over the mirrorlist itself.
The first and second step are same as above, but adding sudo permissions and referencing /etc/pacman.d/mirrorlist directly.
Run once, the first time, to generate the larger (global) mirrorlist.

```bash
sudo ghostmirror -PoclLS Italy,Germany,France /etc/pacman.d/mirrorlist 30 state,outofdate,morerecent,ping
```
Then anytime you wish to refresh the list, update with the below command

```bash
sudo ghostmirror -PmuolsS  /etc/pacman.d/mirrorlist /etc/pacman.d/mirrorlist light state,outofdate,morerecent,estimated,speed
```

## Automatic Operation
In this mode, the above step is performed automatically.

### Prepare
Now you need to make a directory for the new mirrorlist, this location needs to be where a user can edit it without root privilege

```bash
mkdir ~/.config/ghostmirror
```

Now you need to inform pacman where you have stored the mirrorlist.
Edit the file /etc/pacman.conf

```bash
sudo nano /etc/pacman.conf
```

Search for and replace these lines, changing `<username>` below with your username.

```bash
[core]
Include = /home/<username>/.config/ghostmirror/mirrorlist

[extra]
Include = /home/<username>/.config/ghostmirror/mirrorlist
```

### Last steps
Doing the same as was done in the manual mode, create a large (global) mirrorlist

```bash
ghostmirror -PoclLS Italy,Germany,France ~/.config/ghostmirror/mirrorlist 30 state,outofdate,morerecent,ping
```

In this step, you pass the arguments "-lsS" to ghostmirror, and it's saved for automatic operation by the systemd service.
This time adding the "-D" option, which enables the systemd file ghostmirror.timer and loginctl linger.

```bash
ghostmirror -PoDumlsS  ~/.config/ghostmirror/mirrorlist ~/.config/ghostmirror/mirrorlist light state,outofdate,morerecent,estimated,speed
```

### Systemd commands
Show the timer - this shows what timers are present in systemd and time left

```bash
sudo systemctl --user list-timers
```

Force-start ghostmirror before timer ellapsed

```bash
sudo systemctl --user start ghostmirror.service
```

### End - Now you can forget about mirrors forever

## Investigation Mode
Simply add the "-i " to the options and it will show the server list, the server error and the cause.

## Reference
the software accept long options with "--" and you assign values without space but with "=" See the examples below.

```bash
--option
--option value <- doesn't work
--option=value 
```

accept short option with - or multiple option, followed by value

```bash
-o
-o value
-abc valueA valueB valueC
```

## All Options
    -a --arch <required string>
	    Select arch, default 'x86_64'
    -m --mirrorfile <required string>
	    Use a mirror file instead of downloading mirrorlist. 
        Without -m and -u ghostmirror downloads mirrorlist and searches everything in the mirror.
	    You can use -m /etc/pacman.d/mirrorlist.pacnew if you not want to download a mirrorlist but used a local list.
    -c --country <required string>
	    Select country from mirrorlist.
	    If the mirrorlist is not same as the default mirrorlist and ##Country is not set, command will fail.
        If country name is not in the proper format, or it's a mixed mirror, the command will fail.
    -C --country-list <not required argument>
	    List all possibile countries.
    -u --uncommented <not required argument>
	    Use only with an uncommented mirror, by default use commented mirrors.
	    When used with "-m" it will check the local mirrolist  -mu /etc/pacman.d/mirrorlist.
    -d --downloads <required unsigned integer>
	     Set numbers of parallel download, default '4'.
	     Too many parallel downloads and it will likely fail: 1,4,8,16 are good values, >16 can cause problems of failures.
    -O --timeout <required unsigned integer>
	     Set connection timeout in seconds for the cutoff of the mirror to reply.
    -p --progress <not required argument>
	     Show progress, default false
    -P --progress-colors <not required argument>
	    Same as -p but with -o will add a color table of results at the end.
    -o --output <not required argument>
	    Enable the table output, with -P to display with colors.
    -s --speed <required string>
	    Test speed for downloading:
	        light: download one small package ~6MiB
	        normal: download light + normal package ~250Mib
	        heavy: download light+normal+heavy packege ~350MiB, total download >500Mib
    -S --sort <required string>
	    Sort result for any of fields display in table, multiple fields supported.
		    country and mirror is sorted by name
		    proxy first false, last true
		    state first success, last error
		    outofdate, retry and ping, display first less value.
		    uptodate, morerecent, sync, speed and estimated, display first with value.
    -l --list <required string>
	    Save new mirrorlist in file passed as argument.
	    Special name, stdout, can be used for write to stdout file.
    -L --max-list <required unsigned integer>
	    Set max numbers of output mirrors
    -T --type <required string>
	    Select mirrors type, http,https,all
    -i --investigate <required string>
	    Search mirror errors to detect the problem.
	    Can select mode: error, outofdate, all.
		    error: investigate only on error.
		    outofdate: investigate only on outofdate package.
		    all: same as passing -i error,outofdate
    -D --systemd <not required argument>
	     Auto manage systemd.timer
	     When you pass this option the software activate loginctl linger if not enabled.
	     Auto creates ghostmirror.service and ghostmirror.timer
	     The config.service starts ghostmirror in the same mode you haved executed it, with only differences that need -l
	     For example: if you execute ghostmirror -DmuldsS <mirrorlist> <mirrorlist> 16 light estimated,speed
	     The service will always start with <mirrorlist> 16 parallel downloads, speed light and estimated,speed sort
	     To change you can simply repeat the command with new options and mirrorlist names
	     The expire timer is the first element in table and is dynamic, and should change every time counting down to next sync
    -t --time <required string>
	     In systemd timer, when estimate date has elapsed, you can set a time. 
	     You can set a specific time using format hh:mm:ss when starting the service, default is 00:00:00
	     Validate input with systemd-analyzer calendar hh:mm:ss before use.
    -f --fixed-time <required string>
	     In systemd timer use a fixed time instead of estimated time.
	     Validate input with systemd-analyzer calendar hh:mm:ss before use
    -h --help <not required argument>
	     Display this file

### Developers
This section is for developers:

**Requires:**
libcurl, zlib, systemd-libs

**Build:**

```bash
meson setup build
cd build
ninja
```

**Debug:**
Enable very verbose output

```bash
meson configure -Debug=4
```

**warning this is only for contributors, enable auto versioning and auto push**

```bash
meson configure -Developer=true
```

### Bug
'empty'
