Japanese version
=====

This is the Japanese version forked from LeelaGUI.

The KataGo engine has been incorporated into LeelaGUI.

Engine used by creating a file named LeelaI18N_OpenCL.ini (for LeelaGUI with GPU) or LeelaI18N.ini (for LeelaGUI without GPU) in the folder where LeelaGUI's executable file is located and defining it as follows. can be selected.

1. Do not put the above .ini file.

   Works with the traditional Leela engine.

2. If you want to operate with the KataGo engine by sending and receiving GTP commands, define as follows.
```
katago.exe gtp -config default_gtp.cfg -model kata1-b40c256-s11840935168-d2898845681.bin.gz -override-config "logAllGTPCommunication=true"
time_settings 0 5 1 # GTP commands to send to KataGo right after KataGo starts
```
Write the KataGo execution command on the first line. There should be no line breaks in between.
From the second line onwards, if there are GTP commands to be sent to KataGo immediately after KataGo starts, write one GTP command on each line.
The GTP commands that can be defined are only the following 8 commands.
    kata-set-rules xxx
    kata-set-rule xxx
    kgs-rules xxx
    kata-set-param xxx
    time_settings xxx
    kgs-time_settings xxx
    kata-time_settings xxx
    time_left xxx

3. Define as follows when operating with KataGo engine by sending and receiving queries.
```
katago.exe analysis -config analysis_example.cfg -model kata1-b40c256-s11840935168-d2898845681.bin.gz -override-config "logAllRequests=true,logAllResponses=true"
{
  "maxVisits":500,
  "rules":"Chinese",
  "whiteHandicapBonus":"N",
  "overrideSettings":
    {"maxTime":5,
     "wideRootNoise":0.04}
}
```
Write the KataGo execution command on the first line. There should be no line breaks in between.
From the second line onwards, if there are any settings you want to add to the default query sent to KataGo each time, define them in JSON format.
Default query definition values can only be added and cannot be changed, but maxVisits and maxTime used for games can be changed.
These definitions are basically unnecessary if defined in the KataGo configuration file (analysis_example.cfg).

4. Description of comments
   You can comment it out by prefixing it with #. Also, # in the middle of the line invalidates the string after that.

5. Restrictions
   In LeelaGUI using GTP commands, the analysis function of the Leela engine works instead of KataGo when examined in the analysis menu.
   There is a dialog for adjusting the time limit, but even if you set it, only the existing Leela engine will be reflected. For KataGo engine, specify it in the configuration file.

(For reference: The <- key on the keyboard moves the hand back and the -> key moves the hand forward.)

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
