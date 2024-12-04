gcc "./src/main.c" -o "./bin/nes-emulator.exe" -I"..\libs\CSFML\include" -L"..\libs\CSFML\lib\gcc" -lcsfml-graphics -lcsfml-window -lcsfml-system -m32 -std=c99

PAUSE