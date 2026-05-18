@echo off
setlocal enabledelayedexpansion

:: Handle release process
:: Updates the VERSION.txt file and creates the tag

:: current Git branch
for /f "tokens=*" %%i in ('git branch --show-current 2^>nul') do set "branch=%%i"

:: Require master
if not "%branch%"=="master" (
    echo Not in master exiting
    exit /b 1
)

:: Require the current working directory to be clean
for /f "tokens=*" %%i in ('git status --porcelain 2^>nul') do (
    echo Git repository is not clean exiting
    exit /b 1
)

:: Require the version file
if not exist VERSION.txt (
    echo VERSION.txt file not found exiting
    exit /b 1
)

:: Initialize variables
set "major=0"
set "minor=0"
set "patch=0"
set "version_changed="
set "version_message="
set "gpg_sign="

:: Parse the version file
for /f "tokens=2" %%a in ('findstr "VERSION_MAJOR" VERSION.txt') do set "major=%%a"
for /f "tokens=2" %%a in ('findstr "VERSION_MINOR" VERSION.txt') do set "minor=%%a"
for /f "tokens=2" %%a in ('findstr "VERSION_PATCH" VERSION.txt') do set "patch=%%a"

:parse_params
if "%~1"=="" goto end_parse_params
	if "%~1"=="-v" (
		@echo on
	) else if "%~1"=="--verbose" (
		@echo on
	) else if "%~1"=="--major" (
		set /a major=major+1
		set "minor=0"
		set "patch=0"
		set "version_changed=true"
	) else if "%~1"=="--minor" (
		set /a minor=minor+1
		set "patch=0"
		set "version_changed=true"
	) else if "%~1"=="--patch" (
		set /a patch=patch+1
		set "version_changed=true"
	) else if "%~1"=="--sign" (
		set "gpg_sign=true"
	) else if "%~1"=="-m" (
		set "version_message=%~2"
		shift
	) else if "%~1"=="--message" (
		set "version_message=%~2"
		shift
	) else (
		echo Unknown option: %1
		exit /b 1
	)
	shift
goto parse_params
:end_parse_params

:: If nothing has changed then just exit
if "%version_changed%"=="" (
    echo Nothing to do
    exit /b 0
)

:: Sorry tag is already taken.
for /f "tokens=*" %%i in ('git tag -l "v%major%.%minor%.%patch%" 2^>nul') do (
    if not "%%i" == "" (
        echo Tag 'v%major%.%minor%.%patch%' was taken
        exit /b 1
    )
)

if "%patch%"=="0" (
    for /f "tokens=*" %%i in ('git tag -l "v%major%.%minor%" 2^>nul') do (
        if not "%%i" == "" (
            echo Tag 'v%major%.%minor%' was taken
            exit /b 1
        )
    )
)

if "%version_message%"=="" (
    set "version_message=Version %major%.%minor%.%patch%"
)

echo Ready to commit and tag a new version: %major%.%minor%.%patch%
echo Version message will be: %version_message%
set /p "REPLY=Ready to commit? [Y/N]: "
echo.
if /i "%REPLY%"=="Y" (
    
    :: Update the version file
	type null > VERSION.txt.tmp
    for /f "delims=" %%L in (VERSION.txt) do (
        set "line=%%L"
        
        for /f "tokens=1" %%K in ("%%L") do set "first_word=%%K"
		
        if "!first_word!"=="VERSION_MAJOR" (
            echo VERSION_MAJOR !major! >> VERSION.txt.tmp
        ) else if "!first_word!"=="VERSION_MINOR" (
            echo VERSION_MINOR !minor! >> VERSION.txt.tmp
        ) else if "!first_word!"=="VERSION_PATCH" (
            echo VERSION_PATCH !patch! >> VERSION.txt.tmp
        ) else (
            echo !line!>>VERSION.txt.tmp
        )
    )
    move /y VERSION.txt.tmp VERSION.txt >nul

    if defined gpg_sign (
		echo Create a SIGNED release
	
		:: Create a SIGNED release commit
		git commit -a -S -m "Incrementing version number to v!major!.!minor!.!patch!"

		:: Tag it like a champ!
        git tag -s "v!major!.!minor!.!patch!" -m "!version_message!"
    ) else (
		echo Create an UNSIGNED release
	
		:: Create an UNSIGNED release commit
		git commit -am "Incrementing version number to v!major!.!minor!.!patch!"
		
		:: Create an UNSIGNED tag
        git tag -a "v!major!.!minor!.!patch!" -m "%version_message%"
    )

    echo Committed and tagged a new release
	
	set /p "upstream=Enter the name of the upstream remote (default: origin): "
	if "!upstream!"=="" set "upstream=origin"
    
    set /p "REPLY_PUSH=Push commit and tag to remote !upstream!? [Y/N]: "
    echo.
    if /i "!REPLY_PUSH!"=="Y" (
        git push "!upstream!" HEAD
        git push "!upstream!" "v!major!.!minor!.!patch!"
        echo Pushed data to remote !upstream!. Congrats!
    ) else (
        echo You need to 'git push !upstream! HEAD' and 'git push !upstream! --tags' manually.
    )
) else (
    echo Chicken!
)

endlocal
