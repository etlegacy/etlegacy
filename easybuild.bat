::
:: Easybuild - Generate a clean ET:L build
::
:: Change MSVS version to your own
:: Install assets in fs_homepath/etmain

@echo off
@setLocal EnableDelayedExpansion

:: The default VS version (minimal supported vs version is 12)
set vsversion=9001
set vsvarsbat=!VS%vsversion%0COMNTOOLS!\vsvars32.bat
:: Setup the NMake env or find the correct .bat this also finds msbuild

CALL:SETUPNMAKE

if %errorlevel% neq 0 exit /b %errorlevel%

:: Init the submdule
CALL:INITSUBMODULE

:: variables
SET game_homepath=%USERPROFILE%\Documents\ETLegacy
SET game_basepath=%USERPROFILE%\Documents\ETLegacy-Build
SET build_type=Release
SET batloc=%~dp0
SET build_dir=%batloc%build
SET project_dir=%batloc%project

IF "%~1"=="" (
	GOTO:DEFAULTPROCESS
) ELSE (
	GOTO:PROCESSARGS
)
GOTO:EOF

:: process command line arguments if any
:PROCESSARGS
	FOR %%A IN (%*) DO (
		if %errorlevel% neq 0 exit /b %errorlevel%

		CALL:FUNCTIONS %%A
	)
GOTO:EOF

:FUNCTIONS
	set curvar=%~1
	IF /I "%curvar%"=="clean" CALL:DOCLEAN
	IF /I "%curvar%"=="build" CALL:DOBUILD
	IF /I "%curvar%"=="build64" CALL:DOBUILD " Win64"
	IF /I "%curvar%"=="install" CALL:DOINSTALL
	IF /I "%curvar%"=="package" CALL:DOPACKAGE
	IF /I "%curvar%"=="crust" GOTO:UNCRUSTCODE
	IF /I "%curvar%"=="project" CALL:GENERATEPROJECT %project_dir% "%batloc%" "" "" "YES"
	:: download pak0 - 2 to the homepath if they do not exist
	IF /I "%curvar%"=="download" CALL:DOWNLOADPAKS "http://mirror.etlegacy.com/etmain/"
	IF /I "%curvar%"=="open" explorer %game_basepath%
	IF /I "%curvar%"=="release" CALL:DORELEASE
GOTO:EOF

:DORELEASE
	CALL:DOCLEAN
	CALL:DOBUILD
	CALL:DOPACKAGE
GOTO:EOF

:SETUPNMAKE
	where nmake >nul 2>&1
	if !errorlevel neq 0 (
		SET errorlevel=0
		IF EXIST "%vsvarsbat%" (
			ECHO HOLY SHIT YOU ARE AWESOME!
			CALL "%vsvarsbat%" >nul
		) ELSE (
			CALL:FINDVSVARS
			if !errorlevel! neq 0 (
				ECHO Cannot find build environment
				exit /b %errorlevel%
			)
		)
	)
	SET errorlevel=0
GOTO:EOF

:FINDVSVARS
	ECHO Finding VSVARS
	FOR /L %%G IN (20,-1,12) DO (
		IF EXIST "!VS%%G0COMNTOOLS!\vsvars32.bat" (
			SET vsvarsbat=!VS%%G0COMNTOOLS!\vsvars32.bat
			SET vsversion=%%G
			GOTO:EOF
		)
	)
	exit /b 1
GOTO:EOF

:: @deprecated
:FINDNMAKE
	ECHO Finding nmake
	FOR /F "delims==" %%G IN ('SET') DO (
		echo(%%G|findstr /r /c:"COMNTOOLS" >nul && (
			CALL:Substring vsversion %%G 2 2
			CALL "!%%G!\vsvars32.bat" >nul
			GOTO:EOF
		)
	)
	exit /b 1
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
	TITLE Building ET: Legacy
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
	IF EXIST %batloc%libs (
		cd "%batloc%libs"
		git clean -d -f
	)
GOTO:EOF

:: GenerateProject(targetDir, sourceDir, compileType, crossCompile, buildR2)
:GENERATEPROJECT
	ECHO Generating...
	IF EXIST "%~1" RMDIR /s /q "%~1"
	MKDIR "%~1"
	CD "%~1"

	set build_string=
	CALL:GENERATECMAKE build_string "%~4" "%~5"
	cmake -G "Visual Studio %vsversion%%~3" -T v%vsversion%0_xp %build_string% "%~2"
	ETLEGACY.sln
GOTO:EOF

:DOBUILD
	:: build
	CALL:GENERATEPROJECT %build_dir% "%batloc%" "%~1" "%~1" ""
	ECHO Building...
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

:UNCRUSTCODE
	echo Uncrustifying code...
	FOR /R "%batloc%src" %%G IN (*.h *.c *.cpp *.glsl) DO call:UNCRUSTFILE %%G
GOTO:EOF

:UNCRUSTFILE
	set pathstr=%~1
	if not x%pathstr:unzip.c=%==x%pathstr% GOTO:EOF
	if not x%pathstr:sha-1=%==x%pathstr% GOTO:EOF
	if not x%pathstr:Omnibot=%==x%pathstr% GOTO:EOF
	uncrustify --no-backup -c %batloc%uncrustify.cfg %~1
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

:: GenerateCmake(outputVar, crosscompile, buildR2)
:GENERATECMAKE
	SETLOCAL
	IF "%~2" == "" (
		SET CROSSCOMP=YES
	) ELSE (
		SET CROSSCOMP=NO
	)

	IF "%~3" == "" (
		SET build_r2=NO
	) ELSE (
		SET build_r2=YES
	)

	SET loca_build_string=-DBUNDLED_LIBS=YES ^
	-DCMAKE_BUILD_TYPE=%build_type% ^
	-DINSTALL_OMNIBOT=YES ^
	-DCROSS_COMPILE32=%CROSSCOMP% ^
	-DRENDERER_DYNAMIC=%build_r2% ^
	-DFEATURE_RENDERER2=%build_r2%

	ENDLOCAL&SET "%~1=%loca_build_string%"
GOTO:EOF
