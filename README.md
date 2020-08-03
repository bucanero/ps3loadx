# PS3loadX

PS3loadX is my personal evolution of PSL1GHT PS3load sample, using now the Tiny3D library. I hope this version helps to you to work easily ;)

## Features
- You can load SELF files using the net.
- You can load applications from USB/ HDD devices
- You can install applications to the USB or HDD devices from one .zip file
- You can copy applications from USB devices to HDD
- Also you can delete installed applications.

NOTE:
-----

You can include this lines in your app to return to the PS3LoadX application
```c
#include <sys/process.h>
.....
sysProcessExitSpawn2("/dev_hdd0/game/PSL145310/RELOAD.SELF", NULL, NULL, NULL, 0, 1001, SYS_PROCESS_SPAWN_STACK_SIZE_1M);
```

ZIP Format
----------

```
app_folder
|
|---- EBOOT.BIN
|
|---- ICON0.PNG
|
|---- title.txt
```

`app_folder`: folder to install the app
 - in USB devices `/dev_usb000/homebrew/app_folder`
 - from HDD: `/dev_hdd0/game/PSL145310/homebrew/app_folder`

`EBOOT.BIN`: SELF file

`ICON0.PNG`: optional app. image

`title.txt`: it countain one text line with the name app name. It it don't exist PS3LoadX uses `app_folder` as title. i.e: "My application - test 1"

Sending SELF files from the net
-------------------------------

You need send it from the PC using psloadx.exe (see PSL1GHT tools)

For example from one .bat file under windows:
```
set PS3LOAD = tcp:192.168.2.12 // -> PS3 IP

ps3load.exe *.self

pause
```

Installing ZIP files
--------------------

HDD0 is selected by default. To change to USB you need plug one device.

When you send a ZIP file the app ask to you if you want really install it or not in the current device.

Select one Application
----------------------

Use <kbd>LEFT</kbd>/<kbd>RIGHT</kbd> in digital pad

Copying files from USB
----------------------

Press <kbd>CIRCLE</kbd> and select 'Yes' using digital pad

Deleting Applications
---------------------

Press <kbd>SQUARE</kbd> and select 'Yes' using digital pad

Launch Applications
-------------------

Press <kbd>CROSS</kbd> and select 'Yes' using digital pad

Exiting from PS3LoadX
---------------------

Press <kbd>TRIANGLE</kbd> to exit.

Also you can force exit pressing 'PS' (if the network crashes use it)
