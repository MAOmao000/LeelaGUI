# New LeelaGUI

This is a new LeelaGUI with some additional features to the forked LeelaGUI.

For information on the original LeelaGUI, see the second part of this resume and the following:

https://sjeng.org/leela.html

The main features added are as follows.

- Japanese language support based on I18N ("internationalization").
  Specifically, we provide a Japanese catalog file and some literal modifications on the source.
  Note: i18n is a number abbreviation, its "18" being due to the 18 letters in nternationalizatio between the leading i and the ending n in internationalization.

- Incorporating the KataGo Engine:
  Equipped with KataGo analysis engine.

## Specification changes when using the leela engine

The following changes were made.
- If the "dispute" button in the Score Dialog has the same processing as the "OK" button, the "dispute" button is not displayed.
- Added "japanese" and "KataGo" checkboxes in Settings Dialog.
- The font of the coordinate scale on the board was slightly enlarged for easier reading, and lowercase letters were changed to uppercase letters.
- When installed on Windows with "Install for all users" specified, the default installation destination was "C:\Program Files (x86)", even for 64-bit executables, but this has been changed to "C:\Program Files".

## Differences from the original LeelaGUI when started with the KataGo engine

- In the folder containing the LeelaGUI executable, you must create a "LeelaGUI_OpenCL.ini" or "LeelaGUI.ini" file containing the KataGo definition information.
- Except for the Chinese rule, the specified value of Komi should be noted. In the case of handicap games, setting KataGo's Baduk rule to anything other than Chinese does not affect the game itself, but the score displayed in the score displayed in the Score Game dialog is always:
	- Difference of areas in the current board + komi + number of handicap stones (or - komi - number of handicap stones) 

## LeelaGUI_OpenCL.ini" or "LeelaGUI.ini" file when using KataGo engine
```
katago.exe analysis -config analysis_example.cfg -model g170e-b20c256x2-s5303129600-d1228401921.bin.gz -override-config "startupPrintMessageToStderr=true,rootSymmetryPruning=false"
{
  "rules":"chinese",             # Rules of Baduk used to determine winners and losers
  "whiteHandicapBonus":"N",      # Designation to add the number of handicaped stones to the calculation of komi
  "analysisPVLen":15,            # KataGo search depth (shallower is lighter)
  "reportDuringSearchEvery":2.0, # Interval (in seconds) at which KataGo sends analysis data to LeelaGUI during the study
  "overrideSettings":
    {
      "wideRootNoise":0.0        # KataGo search range (the larger the range, the more moves to search)
    },
  "maxVisitsAnalysis":1000000,   # Maximum number of KataGo searches at the time of study
  "maxTimeAnalysis":3600         # Maximum KataGo search time at the time of study (seconds)
}
```
The second and subsequent lines are optional.
- If omitted, the default values are as follows.
	+ "rules":"japanese"
	+ "whiteHandicapBonus":(Not specified)
	+ "analysisPVLen":15
	+ "reportDuringSearchEvery":2.0
	+ "wideRootNoise":(Not specified)
	+ "maxVisitsAnalysis":1000000
	+ "maxTimeAnalysis":3600

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
