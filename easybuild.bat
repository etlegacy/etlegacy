::
:: Easybuild - Generate a clean ET:L build
::
:: Change MSVS version to your own
:: Install assets in fs_homepath/etmain

@echo off

ECHO ETLegacy easybuild for Windows
CALL "C:\Program Files (x86)\Microsoft Visual Studio 12.0\Common7\Tools\VsDevCmd.bat" x86

:: variables
SET homepath=%USERPROFILE%\Documents\ETLegacy
SET basepath=%USERPROFILE%\Documents\ETLegacy-Build
SET build=Debug

PAUSE

:: clean
ECHO Cleaning...
IF EXIST %basepath% RMDIR /s /q %basepath%
IF EXIST build RMDIR /s /q build
DEL /q %homepath%\legacy\*.pk3
DEL /q %homepath%\legacy\*.dll
DEL /q %homepath%\legacy\*.dat

:: build
ECHO Building...
MKDIR build
CD build

cmake -G "NMake Makefiles" -DBUNDLED_LIBS=YES ^
-DCMAKE_BUILD_TYPE=%build% ^
-DCMAKE_INSTALL_PREFIX=%basepath% ^
-DINSTALL_DEFAULT_BASEDIR=%basepath% ^
-DINSTALL_DEFAULT_BINDIR=./ ^
-DINSTALL_DEFAULT_MODDIR=./ ^
-DINSTALL_OMNIBOT=YES ^
..

nmake

:: install
ECHO Installing...
nmake install

:: FIXME: SDL2.dll isn't copied by CMake (why?)
COPY SDL2.dll %basepath%\SDL2.dll

:: done
ECHO The %build% build has been installed in %basepath%.
PAUSE