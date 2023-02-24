# New LeelaGUI

This is a new LeelaGUI with some additional features to the forked LeelaGUI.  
For information on the original LeelaGUI, see the second part of this resume and the following:  
https://sjeng.org/leela.html
  
The main features added are as follows.  
- Japanese language support based on I18N ("internationalization")  
  Specifically, we provide a Japanese catalog file and some literal modifications on the source.  
  Note: i18n is a number abbreviation, its "18" being due to the 18 letters in nternationalizatio between the leading i and the ending n in internationalization.  
- Communication with KataGo engine  
  - Communication with KataGo Analysis engine via query interface  
  - Communication with the KataGo GTP engine via the GTP interface  

## Specification changes when using the Leela engine
- Changes to the New Game dialog  
  Countdown time (Byo yomi time) can be specified in the New Game dialog.  
- Changes to the Game Score dialog  
  If the "dispute" button in the Score Dialog has the same processing as the "OK" button, the "dispute" button is not displayed.  
- Change the coordinate scale of the board  
  The font of the coordinate scale on the board was slightly enlarged for easier reading, and lowercase letters were changed to uppercase letters.  
- Change the display size of the star points on the board  
  Display the stars on the board a little larger.  
- Ownership display shape change  
  Change the ownership display from a circle to a rectangle (because if you own 100%, you won't know if you have a stone or not).  
- Changes to the Settings dialog  
  Added "japanese" checkboxes in Settings Dialog. Restart the program to reflect the result.  
- Change installation destination  
  When installed on Windows with "Install for all users" specified, the default installation destination was "C:\Program Files (x86)", even for 64-bit executables, but this has been changed to "C:\Program Files".  

## Additional features when KataGo engine is selected
- Changes to the New Game dialog  
  - You can specify the used engine for new game.  
  - You can specify the rule for new game.  
  - By selecting Specify a number in Engine max level, the number of visits per move can be specified as a fine number from 1.  
- Changes to the Game Score dialog  
  Shows the average thinking time and average number of visits per move for KataGo during the current game.  
- Changes to the Settings dialog  
  - You can specify the used engine for rated game.  
  - You can specify the rules for rated game.  

## Build from source code with cmake
wxWidgets-3.2 or later requires cmake 3.24 or later.  
```
git clone https://github.com/MAOmao000/LeelaGUI.git
cd LeelaGUI
mkdir build
cd build
cmake .. -DUSE_GPU=1 -DUSE_WLCOPY=0 -DBOOST_ROOT=C:\boost\x64 -DOpenCL_ROOT=C:\OpenCL-SDK\install -DOPENBLAS_ROOT=C:\OpenBLAS-0.3.20-x64 -DwxWidgets_ROOT_DIR=C:\wxWidgets-3.2.0 -DwxWidgets_CONFIGURATION=mswu -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++
cmake --build .
```
  
- The arguments are as follows.  
	+ -DUSE_GPU=0(or1)  
	Specify if OpenCL (GPU) is used (optional)  
	+ -DUSE_WLCOPY=0(or1)  
	Specify if clipboard copy is done by wl-copy command (optional)  
	+ -DBOOST_ROOT=xxx  
	Specify the folder where boost will be installed (optional)  
	+ -DOpenCL_ROOT=xxx  
	Specify the folder where OpenCL will be installed (optional)  
	+ -DOPENBLAS_ROOT=xxx  
	Specify the folder where OpenBLAS will be installed (optional)  
	+ -DwxWidgets_ROOT_DIR=xxx  
	Specifies the folder where wxWidgets is installed (Windows option)  
	+ -DwxWidgets_CONFIGURATION=xxx  
	Configuration to use (e.g., msw, mswd, mswu, mswunivud, etc.) (Windows option)  
	+ -DCMAKE_C_COMPILER=/usr/bin/clang  
	C language compiler is clang (Required for Linux)  
	+ -DCMAKE_CXX_COMPILER=/usr/bin/clang++  
	C++ language compiler is clang++ (Required for Linux)  

The following is the readme for the original LeelaGUI.  

About
=====

This is the original source of the Go/Baduk program 'Leela'.

The engine itself is in the [Leela repo](https://github.com/gcp/Leela). This one contains the graphical user interface.

Screenshot
==========
![Screenshot of LeelaGUI](https://sjeng.org/leelaviz8.png "Leela")

Building
========

The engine needs boost and OpenCL libraries and development headers (and/or OpenBLAS) to compile. The GUI was built in wxWidgets, using wxFormBuilder for layouts. The installer
requires NSIS.

The included Makefile likely needs some modification to work on random Linux systems. Likewise the Visual Studio projects will need their include and lib directories modified.

The GUI assumes the engine source has been unpacked alongside it, in an "engine" subdir
on Linux, or in a dir set in Visual Studio configuration on Visual Studio. The engine's repository is [here on Github as well](https://github.com/gcp/Leela).

Contributing
============

I do not accept contributions to or bug reports about this code. Maintaining an open source effort is a lot of work, and I do not have the time and energy to maintain the project at this point in time. If you wish to improve it, feel free to fork the repostiory and make your fork the best one there is. But do me **ONE** favor, and do keep Leela's name in your fork..

License
=======

The code is licensed under the MIT license.
