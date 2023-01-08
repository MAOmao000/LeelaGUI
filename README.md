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

The following changes were made.  
- Changes to the New Game dialog  
  Countdown time (Byo yomi time) can be specified in the New Game dialog.  
- Changes to the Game Score dialog  
  If the "dispute" button in the Score Dialog has the same processing as the "OK" button, the "dispute" button is not displayed.  
- Change the coordinate scale of the board  
  The font of the coordinate scale on the board was slightly enlarged for easier reading, and lowercase letters were changed to uppercase letters.  
- Changes to the Settings dialog  
  Added "japanese" checkboxes in Settings Dialog. Restart the program to reflect the result.  
- Change installation destination  
  When installed on Windows with "Install for all users" specified, the default installation destination was "C:\Program Files (x86)", even for 64-bit executables, but this has been changed to "C:\Program Files".  

## Additional features when KataGo engine is selected
- Changes to the New Game dialog  
  By selecting Specify a number in Engine max level, the number of visits per move can be specified as a fine number from 1.  
- Changes to the Game Score dialog  
  Shows the average thinking time and average number of visits per move for KataGo during the current game.  
- Changes to the Settings dialog  
  Default rules allow selection of Chinese and Japanese.  
- Initial configuration file  
  KataGo configuration definition information can be changed in the initial configuration file (.ini file).  

## LeelaGUI_OpenCL.ini" or "LeelaGUI.ini" file when using KataGo engine
Initial configuration file (.ini file) has been optional since v2.0.0.  

```
katago_OpenCL.exe analysis -config analysis_example.cfg -model kata1-b40c256-s11840935168-d2898845681.bin.gz -override-config "numAnalysisThreads=1,numSearchThreadsPerAnalysisThread=8"

{
  "rules":"chinese",             # Rules to be used when playing a game of Baduk (default:japanese)
  "whiteHandicapBonus":"N",      # Designation to add the number of handicaped stones to the calculation of komi (default:undefined)
  "analysisPVLen":15,            # KataGo search depth (default:15)
  "reportDuringSearchEvery":2.0, # Interval (in seconds) at which KataGo sends analysis data to LeelaGUI at analyze (default:2.0)
  "overrideSettings":
    {
      "wideRootNoise":0.0        # KataGo search range (default:0.04)
    }
  "maxVisitsAnalysis":1000000,   # Maximum number of KataGo searches at analyze (default:1000000)
  "maxTimeAnalysis":3600         # Maximum KataGo search time (seconds) at analyze (default:3600)
}
```
## Build from source code with cmake
wxWidgets-3.2 or later requires cmake 3.24 or later.  
```
git clone https://github.com/MAOmao000/LeelaGUI.git
cd LeelaGUI
mkdir build
cd build
cmake .. -DUSE_GPU=1 -DUSE_THREAD=0 -DUSE_WLCOPY=0 -DBOOST_ROOT=C:\boost\x64 -DOpenCL_ROOT=C:\OpenCL-SDK\install -DOPENBLAS_ROOT=C:\OpenBLAS-0.3.20-x64 -DwxWidgets_ROOT_DIR=C:\wxWidgets-3.2.0 -DwxWidgets_CONFIGURATION=mswu
cmake --build .
```
- The arguments are as follows.  
	+ -DUSE_GPU=0(or1)  
	Specify if OpenCL (GPU) is used (optional)  
	+ -DUSE_THREAD=0(or1)  
	Specify if communicating with KataGo via threaded interface (optional)  
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
