New LeelaGUI
=====
This is a new LeelaGUI with some additional features to the forked LeelaGUI.

The original LeelaGUI is described below.
https://sjeng.org/leela.html

The main features added are as follows.

- Japanese language support based on I18N ("internationalization").
  Specifically, we provide a Japanese catalog file and some literal modifications on the source.
  Note: i18n is a number abbreviation, its "18" being due to the 18 letters in nternationalizatio between the leading i and the ending n in internationalization.

- Importing the KataGo Engine:
  The ANALYSIS interface and the GTP interface have each been incorporated.

Changes to the original LeelaGUI
=====
The following changes were made to the basic policy of not making any changes.
- If the "dispute" button in the Score Dialog has the same processing as the "OK" button, the "dispute" button is not displayed.
- Added "japanese" and "KataGo" checkboxes in Settings Dialog.
- The font of the coordinate scale on the board was slightly enlarged for easier reading, and lowercase letters were changed to uppercase letters.
- In the case of "Install for all users" specified installation in Windows, the default installation destination was set to "C:\Program Files" (previously "C:\Program Files (x86)") for 64-bit executable files.

Differences from the original LeelaGUI when started with the KataGo engine (common)
=====
- Create a "LeelaGUI_OpenCL.ini" or "LeelaGUI.ini" file with KataGo definition information in the folder containing the LeelaGUI executable.
- The holding time can no longer be set from the screen.

Differences from original LeelaGUI when started with KataGo engine (ANALYSIS interface)
=====
- Since the time to hold the game cannot be set from the screen, set it in one of the following ways.
  - "LeelaGUI_OpenCL.ini" or "LeelaGUI.ini" file with "maxTime":xx.
  - Set by "maxTime":xx in the "analysis_example.cfg" file.

Differences from original LeelaGUI when started with KataGo engine (GTP interface)
=====
- The analysis function does not work at all.
- Since the time to hold the game cannot be set from the screen, it is set in one of the following ways.
  - Set in "time_settings x y z" in the "LeelaGUI_OpenCL.ini" or "LeelaGUI.ini" file.
  - Set by "maxTime":xx in the "default_gtp.cfg" file.
- "Pondering" cannot be specified from the settings screen, so if you wish to specify it, do so in one of the following ways.
  - In the "LeelaGUI_OpenCL.ini" or "LeelaGUI.ini" file, use -override-config "ponderingEnabled=true,maxTimePondering=xx".
  - Set by "ponderingEnabled=true" and "maxTimePondering=xx" in the "default_gtp.cfg" file.

LeelaGUI_OpenCL.ini" or "LeelaGUI.ini" file (ANALYSIS interface) when using KataGo engine
=====
```
katago.exe analysis -config analysis_example.cfg -model kata1-b40c256-s11840935168-d2898845681.bin.gz -override-config "logAllRequests=true,logAllResponses=true"  
{
  "maxVisits":500,               # The maximum number of times KataGo searches in a game (smaller value means it finishes earlier)
  "rules":"chinese",             # Rules of Baduk used to determine winners and losers
  "whiteHandicapBonus":"N",      # Designation to add the number of handicaped stones to the calculation of komi
  "analysisPVLen":15,            # KataGo search depth (shallower is lighter)
  "reportDuringSearchEvery":2.0, # Interval (in seconds) at which KataGo sends analysis data to LeelaGUI during the study
  "overrideSettings":
    {
      "maxTime":5,               # KataGo's search time when playing a game (smaller search time means quicker completion) (seconds)
      "wideRootNoise":0.04       # KataGo search range (the larger the range, the more moves to search)
    },
  "maxVisitsAnalysis":1000000,   # Maximum number of KataGo searches at the time of study
  "maxTimeAnalysis":3600         # Maximum KataGo search time at the time of study (seconds)
}
```

LeelaGUI_OpenCL.ini" or "LeelaGUI.ini" file (GTP interface) when using KataGo engine
=====
```
katago.exe gtp -config default_gtp.cfg -model kata1-b40c256-s11840935168-d2898845681.bin.gz -override-config "logAllGTPCommunication=true"
time_settings 0 5 1 # GTP commands to send to KataGo right after KataGo starts
```

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
