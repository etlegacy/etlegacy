::
:: Easybuild - Generate a clean ET:L build
::
:: Change MSVS version to your own
:: Install assets in fs_homepath/etmain

@echo off
@setLocal EnableDelayedExpansion

:: The default VS version
set vsversion=12
set vsvarsbat=!VS%vsversion%0COMNTOOLS!\vsvars32.bat
:: Setup the NMake env or find the correct .bat this also finds msbuild
CALL:SETUPNMAKE

:: Init the submdule
CALL:INITSUBMODULE

:: variables
SET game_homepath=%USERPROFILE%\Documents\ETLegacy
SET game_basepath=%USERPROFILE%\Documents\ETLegacy-Build
SET build_type=Release
SET batloc=%~dp0
SET build_dir=%batloc%build

IF "%~1"=="" (
	GOTO:DEFAULTPROCESS
) ELSE (
	GOTO:PROCESSARGS
)
GOTO:EOF

:: process command line arguments if any
:PROCESSARGS
	FOR %%A IN (%*) DO (
		IF errorlevel 1 (
			echo Failure errorlevel: %errorlevel%
			GOTO:EOF
		) ELSE (
			CALL:FUNCTIONS %%A
		)
	)
GOTO:EOF

:FUNCTIONS
	set curvar=%~1
	IF /I "%curvar%"=="clean" CALL:DOCLEAN
	IF /I "%curvar%"=="build" CALL:DOBUILD
	IF /I "%curvar%"=="build64" CALL:DOBUILD " Win64"
	IF /I "%curvar%"=="install" CALL:DOINSTALL
	IF /I "%curvar%"=="pack" CALL:DOPACKAGE
	:: download pak0 - 2 to the homepath if they do not exist
	IF /I "%curvar%"=="download" CALL:DOWNLOADPAKS "http://mirror.etlegacy.com/etmain/"
	IF /I "%curvar%"=="open" explorer %game_basepath%
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

:INITSUBMODULE
	IF NOT EXIST "%batloc%libs\CMakeLists.txt" (
		ECHO Getting bundled libs...
		CD %batloc%
		git submodule init
		git submodule update
	)
GOTO:EOF

:SETUPFOLDERS
	IF NOT EXIST "%game_basepath%" (
		ECHO Will create base directory: "%game_basepath%"
		mkdir "%game_basepath%"
	)
	CD %build_dir%
	CALL:CLEANPATH "%game_basepath%\legacy\" "*.pk3 *.dll *.dat"
	CALL:CLEANPATH "%game_homepath%\legacy\" "*.pk3 *.dll *.dat"
	CALL:COPYFROMPATH "%cd%\" "et*.exe renderer_openg*.dll SDL2.dll" "%game_basepath%\"
	CALL:COPYFROMPATH "%cd%\legacy\" "*.pk3 qagame*.dll" "%game_basepath%\legacy\"
GOTO:EOF

:CLEANPATH
	IF NOT EXIST %~1 GOTO:EOF
	set bacpath=%cd%
	cd %~1
	FOR %%F IN (%~2) DO (
		DEL %%F
	)
	cd %bacpath%
GOTO:EOF

:COPYFROMPATH
	set bacpath=%cd%
	cd %~1
	FOR %%F IN (%~2) DO (
		CALL:COPYFILE "%%F" %~3
	)
	cd %bacpath%
GOTO:EOF

:COPYFILE
	REM /D
	IF EXIST %~1 XCOPY %~1 %~2 /Y >nul
GOTO:EOF

:DEFAULTPROCESS
	TITLE Building ET:Legacy
	ECHO ETLegacy easybuild for Windows

	CALL:DOCLEAN
	CALL:DOBUILD
	if errorlevel 1 (
		ECHO There was an issue with the build so no files are copied
		PAUSE
		GOTO:EOF
	)
	CALL:DOINSTALL
	SLEEP 5
GOTO:EOF

:DOCLEAN
	:: clean
	ECHO Cleaning...
	IF EXIST %game_basepath% RMDIR /s /q %game_basepath%
	IF EXIST %build_dir% RMDIR /s /q %build_dir%
GOTO:EOF

:DOBUILD
	:: build
	ECHO Building...
	IF NOT EXIST %build_dir% MKDIR %build_dir%
	CD %build_dir%

	IF "%~1" == "" (
		SET CROSSCOMP=YES
	) ELSE (
		SET CROSSCOMP=NO
	)

	cmake -G "Visual Studio %vsversion%%~1" -T v%vsversion%0_xp ^
	-DBUNDLED_LIBS=YES ^
	-DCMAKE_BUILD_TYPE=%build_type% ^
	-DINSTALL_OMNIBOT=YES ^
	-DCROSS_COMPILE32=%CROSSCOMP% ^
	..

	msbuild ETLEGACY.sln /target:ALL_BUILD /p:Configuration=%build_type%
GOTO:EOF

:DOINSTALL
	:: install
	ECHO Setting up the game
	CALL:SETUPFOLDERS

	:: done
	CALL:CREATELINK "ETLegacy" "%game_basepath%\etl.exe" "%game_basepath%"
	ECHO The %build_type% build has been installed in %game_basepath%, and shortcut has been added to your desktop
GOTO:EOF

:DOPACKAGE
	:: package
	ECHO Packaging...
	CD %build_dir%
	cpack
GOTO:EOF

:CREATELINK
	set SCRIPT="%TEMP%\%RANDOM%-%RANDOM%-%RANDOM%-%RANDOM%.vbs"

	echo Set oWS = WScript.CreateObject("WScript.Shell") >> %SCRIPT%
	echo sLinkFile = "%USERPROFILE%\Desktop\%~1.lnk" >> %SCRIPT%
	echo Set oLink = oWS.CreateShortcut(sLinkFile) >> %SCRIPT%
	echo oLink.TargetPath = "%~2" >> %SCRIPT%
	echo oLink.WorkingDirectory = "%~3" >> %SCRIPT%
	echo oLink.Save >> %SCRIPT%

	cscript /nologo %SCRIPT%
	del %SCRIPT%
GOTO:EOF

:DOWNLOADPAKS
	IF NOT EXIST "%game_homepath%\etmain" (
		md "%game_homepath%\etmain"
	)

	IF NOT EXIST "%game_homepath%\etmain\pak0.pk3" (
		bitsadmin /transfer "pak0" %~1pak0.pk3 "%game_homepath%\etmain\pak0.pk3"
	)
	IF NOT EXIST "%game_homepath%\etmain\pak1.pk3" (
		bitsadmin /transfer "pak1" %~1pak1.pk3 "%game_homepath%\etmain\pak1.pk3"
	)
	IF NOT EXIST "%game_homepath%\etmain\pak2.pk3" (
		bitsadmin /transfer "pak2" %~1pak2.pk3 "%game_homepath%\etmain\pak2.pk3"
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