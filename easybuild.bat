::
:: Easybuild - Generate a clean ET:L build
::
:: Change MSVS version to your own
:: Install assets in fs_homepath/etmain
::

@echo off
@setLocal EnableDelayedExpansion

:: variables
SET game_homepath=%USERPROFILE%\Documents\ETLegacy
SET game_basepath=%USERPROFILE%\Documents\ETLegacy-Build
SET modname=legacy
SET build_type=Release
SET batloc=%~dp0
SET build_dir=!batloc!build
SET project_dir=!batloc!project

SET build_64=0
SET mod_only=0
SET use_autoupdate=1
SET use_extra=1
SET build_r2=0
SET build_ssl=1
SET wolf_ssl=0
SET open_ssl=0

If Defined FEATURE_SSL (
    SET build_ssl=!FEATURE_SSL!
)

If Defined BUNDLED_WOLFSSL (
    SET wolf_ssl=!BUNDLED_WOLFSSL!
)

If Defined BUNDLED_OPENSSL (
    SET open_ssl=!BUNDLED_OPENSSL!
)

SET generator=
REM SET generator=Visual Studio 16 2019
REM SET platform_toolset=-T v142

:: pickup some parameters before proceeding
set i=0
:loop
IF NOT "%1"=="" (
	IF /I "%1"=="-help" (
		ECHO.
		ECHO ET: Legacy Easy Builder Help
		ECHO ===============================
		ECHO clean - clean up the build
		ECHO build - run the build process
		ECHO package - run the package process
		ECHO install - install the game into the system
		ECHO download - download assets
		ECHO crust - run the uncrustify to the source
		ECHO project - generate the project files for your platform
		ECHO release - run the entire release process
		ECHO open - open explorer to game path
		ECHO help - print this help
		ECHO.
		ECHO Properties
		ECHO -64, -debug, -mod, -noupdate, -noextra, -nor2, -generator [generator], -toolset [version] -build_dir [dir]
		ECHO.
		GOTO:EOF
	) ELSE IF /I "%1"=="-64" (
		SET build_64=1
	) ELSE IF /I "%1"=="-no-ssl" (
		SET build_ssl=0
	) ELSE IF /I "%1"=="-mod" (
		SET mod_only=1
	) ELSE IF /I "%1"=="-noupdate" (
		SET use_autoupdate=0
	) ELSE IF /I "%1"=="-noextra" (
		SET use_extra=0
	) ELSE IF /I "%1"=="-debug" (
		SET build_type=Debug
	) ELSE IF /I "%1"=="-nor2" (
		SET build_r2=0
	) ELSE IF /I "%1"=="-generator" (
		SET generator=%~2
		SHIFT
	) ELSE IF /I "%1"=="-toolset" (
		SET platform_toolset=-T %~2
		SHIFT
	) ELSE IF /I "%1"=="-build_dir" (
		SET build_dir=%~dpnx2
		SHIFT
	) ELSE (
		SET /A i+=1
		SET commands[!i!]=%~1
	)
	SHIFT
	GOTO :loop
)
SET tasks=%i%

IF NOT "%generator%"=="" (
	SET generator=-G "%generator%"
)

REM for /L %%i in (1,1,%tasks%) do echo Task number %%i: "!commands[%%i]!"

if !errorlevel!==1 exit /b !errorlevel!

REM ECHO Checking application status
where /q cmake >nul 2>&1 && (
	REM ECHO CMake ok.
) || (
	ECHO Missing CMake cannot proceed.
	GOTO:EOF
)

REM ECHO Applications ok. Proceeding.

:: Init the submdule
CALL:INITSUBMODULE

IF "%tasks%"=="0" (
	GOTO:DEFAULTPROCESS
) ELSE (
	GOTO:PROCESSCOMMANDS
)
GOTO:EOF

:: process commands if any
:PROCESSCOMMANDS
	FOR /L %%i in (1,1,%tasks%) DO (
		if !errorlevel!==1 exit /b !errorlevel!
		CALL:FUNCTIONS "!commands[%%i]!"
	)
GOTO:EOF

:FUNCTIONS
	set curvar=%~1
	IF /I "!curvar!"=="clean" CALL:DOCLEAN
	IF /I "!curvar!"=="build" CALL:DOBUILD
	IF /I "!curvar!"=="install" CALL:DOINSTALL
	IF /I "!curvar!"=="package" CALL:DOPACKAGE
	IF /I "!curvar!"=="crust" GOTO:UNCRUSTCODE
	IF /I "!curvar!"=="project" CALL:OPENPROJECT
	:: download assets to the homepath if they do not exist
	IF /I "!curvar!"=="download" CALL:DOWNLOADPAKS "https://mirror.etlegacy.com/etmain/"
	IF /I "!curvar!"=="open" explorer !game_basepath!
	IF /I "!curvar!"=="release" CALL:DORELEASE
GOTO:EOF

:DORELEASE
	CALL:DOCLEAN
	CALL:DOBUILD
	CALL:DOPACKAGE
GOTO:EOF

:INITSUBMODULE
	IF NOT EXIST "!batloc!libs\CMakeLists.txt" (
		ECHO Getting bundled libs...
		CD !batloc!
		git submodule init
		git submodule update
	)
GOTO:EOF

:SETUPFOLDERS
	IF NOT EXIST "!game_basepath!" (
		ECHO Will create base directory: "!game_basepath!"
		mkdir "!game_basepath!"
	)
	CD !build_dir!
	CALL:CLEANPATH "!game_basepath!\%modname%\" "*.pk3 *.dll *.dat"
	CALL:CLEANPATH "!game_homepath!\%modname%\" "*.pk3 *.dll *.dat"
	CALL:COPYFROMPATH "%cd%\" "et*.exe renderer_opengl*.dll" "!game_basepath!\"
	CALL:COPYFROMPATH "%cd%\legacy\" "*.pk3 qagame*.dll *.dat" "!game_basepath!\%modname%\"
	CD !batloc!
	CALL:COPYFROMPATH "%cd%\misc\etmain\" "*" "!game_basepath!\etmain\"
	CALL:COPYFROMPATH "%cd%\misc\" "description.txt" "!game_basepath!\%modname%\"
	CALL:COPYFROMPATH "%cd%\docs\" "INSTALL.txt" "!game_basepath!"
	CALL:COPYFROMPATH "%cd%\" "COPYING.txt" "!game_basepath!"
GOTO:EOF

:CLEANPATH
	IF NOT EXIST %~1 GOTO:EOF
	set bacpath=%cd%
	cd %~1
	FOR %%F IN (%~2) DO (
		DEL %%F
	)
	cd !bacpath!
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
GOTO:EOF

:DOCLEAN
	:: clean
	ECHO Cleaning...
	IF EXIST !game_basepath! RMDIR /s /q !game_basepath!
	IF EXIST !build_dir! RMDIR /s /q !build_dir!
	IF EXIST !batloc!libs (
		cd "!batloc!libs"
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
	CALL:GENERATECMAKE build_string
	IF %build_64%==1 (
		cmake !generator! !platform_toolset! -A Win64 %build_string% "%~2"
		ECHO cmake !generator! !platform_toolset! -A Win64 %build_string% "%~2"
	) ELSE (
		cmake !generator! !platform_toolset! -A Win32 %build_string% "%~2"
		ECHO cmake !generator! !platform_toolset! -A Win32 %build_string% "%~2"
	)
GOTO:EOF

:OPENPROJECT
	CALL:GENERATEPROJECT !project_dir! "!batloc!"
	ETLEGACY.sln
GOTO:EOF

:DOBUILD
	:: build
	CALL:GENERATEPROJECT !build_dir! "!batloc!"
	ECHO Building...
	REM msbuild ETLEGACY.sln /target:CMake\ALL_BUILD /p:Configuration=%build_type%
	cmake --build . --config %build_type%
GOTO:EOF

:DOINSTALL
	:: install
	ECHO Setting up the game
	CALL:SETUPFOLDERS

	:: done
	CALL:CREATELINK "ETLegacy" "!game_basepath!\etl.exe" "!game_basepath!"
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
	set SrcFolder=!batloc!src
	FOR /R "%SrcFolder%" %%G IN (*.h *.c *.cpp *.glsl) DO call:UNCRUSTFILE %%G
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

	echo Set oWS = WScript.CreateObject("WScript.Shell") >> !SCRIPT!
	echo sLinkFile = "%USERPROFILE%\Desktop\%~1.lnk" >> !SCRIPT!
	echo Set oLink = oWS.CreateShortcut(sLinkFile) >> !SCRIPT!
	echo oLink.TargetPath = "%~2" >> !SCRIPT!
	echo oLink.WorkingDirectory = "%~3" >> !SCRIPT!
	echo oLink.Save >> !SCRIPT!

	cscript /nologo !SCRIPT!
	del !SCRIPT!
GOTO:EOF

:DOWNLOADPAKS
	IF NOT EXIST "!game_homepath!\etmain" (
		md "!game_homepath!\etmain"
	)

	IF NOT EXIST "!game_homepath!\etmain\pak0.pk3" (
		bitsadmin /transfer "pak0" %~1pak0.pk3 "!game_homepath!\etmain\pak0.pk3"
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
	IF %build_64%==1 (
		SET CROSSCOMP=NO
	) ELSE (
		SET CROSSCOMP=YES
	)

	SET local_build_string=-DBUNDLED_LIBS=YES ^
	-DCMAKE_BUILD_TYPE=!build_type! ^
	-DFEATURE_AUTOUPDATE=!use_autoupdate! ^
	-DINSTALL_EXTRA=!use_extra! ^
	-DCROSS_COMPILE32=!CROSSCOMP! ^
	-DRENDERER_DYNAMIC=!build_r2! ^
	-DFEATURE_RENDERER2=!build_r2! ^
	-DBUNDLED_WOLFSSL=!wolf_ssl! ^
	-DBUNDLED_OPENSSL=!build_r2! ^
	-DFEATURE_SSL=!open_ssl!

	IF !mod_only!==1 (
		SET local_build_string=!local_build_string! ^
		-DBUILD_CLIENT=0 ^
		-DBUILD_SERVER=0
	)

	ENDLOCAL&SET "%~1=%local_build_string%"
GOTO:EOF
