@echo off
set CSFML_VERSION=2.6.1
set CSFML_URL="https://www.sfml-dev.org/files/CSFML-2.6.1-windows-64-bit.zip"
set TARGET_DIR=libs

if exist "%TARGET_DIR%\CSFML\\include" (
    echo CSFML is already set up.
    exit /b 0
)

echo Setting up CSFML...
mkdir %TARGET_DIR%

echo Downloading CSFML version %CSFML_VERSION%...
powershell -Command "Invoke-WebRequest -Uri %CSFML_URL% -OutFile csfml.zip"

echo Extracting CSFML...
powershell -Command "Expand-Archive -Path csfml.zip -DestinationPath %TARGET_DIR%"

move "%TARGET_DIR%\CSFML-%CSFML_VERSION%-windows-gcc\include" "%TARGET_DIR%\"
move "%TARGET_DIR%\CSFML-%CSFML_VERSION%-windows-gcc\lib" "%TARGET_DIR%\"

rmdir /s /q "%TARGET_DIR%\CSFML-%CSFML_VERSION%-windows-gcc"
del csfml.zip

echo CSFML setup is complete.
