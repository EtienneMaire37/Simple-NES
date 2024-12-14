@echo off
REM Configuration
set CSFML_VERSION=2.5
set CSFML_URL=https://www.sfml-dev.org/files/CSFML-2.6.1-windows-32-bit.zip
set TARGET_DIR=libs

REM Check if CSFML is already downloaded
if exist "%TARGET_DIR%\CSFML\\include" (
    echo CSFML is already set up.
    exit /b 0
)

REM Create the target directory
echo Setting up CSFML...
mkdir %TARGET_DIR%

REM Download CSFML
echo Downloading CSFML version %CSFML_VERSION%...
powershell -Command "Invoke-WebRequest -Uri %CSFML_URL% -OutFile csfml.zip"

REM Extract the downloaded archive
echo Extracting CSFML...
powershell -Command "Expand-Archive -Path csfml.zip -DestinationPath %TARGET_DIR%"

REM Move extracted files to the proper structure
move "%TARGET_DIR%\CSFML-%CSFML_VERSION%-windows-gcc\include" "%TARGET_DIR%\"
move "%TARGET_DIR%\CSFML-%CSFML_VERSION%-windows-gcc\lib" "%TARGET_DIR%\"

REM Clean up
rmdir /s /q "%TARGET_DIR%\CSFML-%CSFML_VERSION%-windows-gcc"
del csfml.zip

echo CSFML setup is complete.
