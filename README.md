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
- New game dialog allows the time to byo yomi time in addition to the time for game.
- Arbitrary values can now also be specified for engine max level in the New game dialog.

## Differences from the original LeelaGUI when started with the KataGo engine

- In the folder containing the LeelaGUI executable, you must create a "LeelaGUI_OpenCL.ini" or "LeelaGUI.ini" file containing the KataGo definition information.
- Except for the Chinese rule, the specified value of Komi should be noted. In the case of handicap games, setting KataGo's Baduk rule to anything other than Chinese rule does not affect the game itself, but the score displayed in the score displayed in the Score Game dialog is always:
	- Difference of areas in the current board + komi + number of handicap stones (or - komi - number of handicap stones) 

## LeelaGUI_OpenCL.ini" or "LeelaGUI.ini" file when using KataGo engine
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
The second and subsequent lines are optional.
- If omitted, the default values are as follows.
	+ "rules":"japanese"
	+ "whiteHandicapBonus":(Not specified)
	+ "analysisPVLen":15
	+ "reportDuringSearchEvery":2.0
	+ "wideRootNoise":(Not specified)
	+ "maxVisitsAnalysis":1000000
	+ "maxTimeAnalysis":3600

## cmake によるソースコードからのビルド
```
git clone https://github.com/MAOmao000/LeelaGUI.git
cd LeelaGUI
mkdir build
cd build
cmake .. -DUSE_GPU=1 -DUSE_THREAD=1 -DUSE_WLCOPY=1 -DPERFORMANCE=1 -DBOOST_ROOT=C:\boost\x64 -DOpenCL_ROOT=C:\OpenCL-SDK\install -DOPENBLAS_ROOT=C:\OpenBLAS-0.3.20-x64 -DwxWidgets_ROOT_DIR=C:\wxWidgets-3.2.1 -DwxWidgets_CONFIG_EXECUTABLE=/path/to/wx-config
```
- 引数は以下です.
	+ -DUSE_GPU=1:OpenCL(GPU)を使用する場合に指定します(オプション)
	+ -DUSE_THREAD=1:スレッドインタフェースでKataGoと通信する場合に指定します(オプション)
	+ -DUSE_WLCOPY=1:クリップボードコピーをwlcopyコマンドで行う場合に指定します(オプション)
	+ -DPERFORMANCE=1:対局毎に一手あたりの平均思考時間と平均試行回数をLeelaGUIPerf.logに出力します(オプション)
	+ -DBOOST_ROOT=xxx:boostのインストール先フォルダを指定します(オプション)
	+ -DOpenCL_ROOT=xxx:OpenCLのインストール先フォルダを指定します(オプション)
	+ -DOPENBLAS_ROOT=xxx:OpenBLASのインストール先フォルダを指定します(オプション)
	+ -DwxWidgets_ROOT_DIR=wxWidgetsのインストール先フォルダを指定します(Windowsオプション)
	+ -DwxWidgets_CONFIG_EXECUTABLE=xxx:wx-config というスクリプトのパスを指定します(Linuxオプション)
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
