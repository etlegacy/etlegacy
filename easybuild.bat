::
:: Easybuild - Generate a clean ET:L build
::
:: Change MSVS version to your own
:: Install assets in fs_homepath/etmain

@echo off
@setLocal EnableDelayedExpansion

ECHO ETLegacy easybuild for Windows

:: The default VS version
set vsversion=12
set vsvarsbat=!VS%vsversion%0COMNTOOLS!\vsvars32.bat
:: Setup the NMake env or find the correct .bat this also finds msbuild
CALL:SETUPNMAKE

:: variables
SET game_homepath=%USERPROFILE%\Documents\ETLegacy
SET game_basepath=%USERPROFILE%\Documents\ETLegacy-Build
SET build=Release

PAUSE

:: clean
ECHO Cleaning...
IF EXIST %game_basepath% RMDIR /s /q %game_basepath%
IF EXIST build RMDIR /s /q build
DEL /q %game_homepath%\legacy\*.pk3
DEL /q %game_homepath%\legacy\*.dll
DEL /q %game_homepath%\legacy\*.dat

:: build
ECHO Building...
MKDIR build
CD build

cmake -G "Visual Studio %vsversion%" -T v%vsversion%0_xp -DBUNDLED_LIBS=YES ^
-DCMAKE_BUILD_TYPE=%build% ^
-DCMAKE_INSTALL_PREFIX=%game_basepath% ^
-DINSTALL_DEFAULT_BASEDIR=./ ^
-DINSTALL_DEFAULT_BINDIR=./ ^
-DINSTALL_DEFAULT_MODDIR=./ ^
-DINSTALL_OMNIBOT=YES ^
-DCROSS_COMPILE32=YES ^
..

msbuild ETLEGACY.sln /target:ALL_BUILD /p:Configuration=Release

:: install
ECHO Installing...
msbuild ETLEGACY.sln /target:INSTALL /p:Configuration=Release

:: FIXME: SDL2.dll isn't copied by CMake (why?)
COPY SDL2.dll %game_basepath%\SDL2.dll

:: done
ECHO The %build% build has been installed in %game_basepath%.
PAUSE
GOTO:EOF

:SETUPNMAKE
	where nmake >nul 2>&1
	if errorlevel 1 (
		IF EXIST "%vsvarsbat%" (
			CALL "%vsvarsbat%" >nul
		) ELSE (
			CALL:FINDNMAKE
		)
	)
	set errorlevel=0
GOTO:EOF

:FINDNMAKE
	ECHO Finding nmake
	FOR /F "delims==" %%G IN ('SET') DO (
		echo(%%G|findstr /r /c:"COMNTOOLS" >nul && (
			CALL:Substring vsversion %%G 2 2
			CALL "!%%G!\vsvars32.bat" >nul
			GOTO:EOF
		)
	)
GOTO:EOF

:Substring
	::Substring(retVal,string,startIndex,length)
	:: extracts the substring from string starting at startIndex for the specified length
	SET string=%2%
	SET startIndex=%3%
	SET length=%4%

	if "%4" == "0" goto :noLength
	CALL SET _substring=%%string:~%startIndex%,%length%%%
	goto :substringResult
	:noLength
	CALL SET _substring=%%string:~%startIndex%%%
	:substringResult
	set "%~1=%_substring%"
GOTO :EOF