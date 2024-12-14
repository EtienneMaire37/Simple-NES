# Simple-NES

SimpleNES is a simple nes emulator. It is designed to run classic NES games and is built with simplicity and portability in mind.

---

## Features
- **CPU Emulation**: Complete implementation of the 6502 processor, passing all major test ROMs.
- **PPU Rendering**: Accurate rendering of NES graphics.
- **Sound Output**: Audio playback powered by WASAPI for low-latency and high-quality sound.
- **Cross-Platform Development**: Uses CSFML for graphics and input handling.

---

## Getting Started

### Requirements
To build and run the emulator, ensure you have the following installed:

- **CMake** (version 3.15 or higher)
- **GCC** or **MinGW** (for Windows builds)
- **CSFML** (used for rendering and input handling)
- **WASAPI** (Windows Audio Session API, already included in Windows SDK)

### Setup
1. Clone the repository:
   ```bash
   git clone https://github.com/EtienneMaire37/simple-nes.git
   cd simple-nes
   ```

2. Run the setup script to download and configure dependencies:
   ```bash
   setup.bat
   ```

3. Build the project:
   ```bash
   mkdir build
   cd build
   cmake -G "MinGW Makefiles" -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ ..
   cmake --build . --config Release
   ```

The built executable will be located in the `build/bin/` folder.

### Usage
1. Place your NES ROM files in the same directory as the executable or specify their path when launching.
2. Run the emulator:
   ```bash
   ./build/bin/simple-nes.exe <path-to-rom>
   ```
3. Controls:
   - Arrow keys: D-Pad
   - C: A Button
   - X: B Button
   - LShift: Start
   - LControl: Select

---

## Contributing
Contributions are welcome! Feel free to fork the repository, make improvements, and submit a pull request. Suggestions and bug reports can be submitted to [Issues](https://github.com/EtienneMaire37/simple-nes/issues).

---

## License
This project is licensed under the MIT License. See the `LICENSE` file for details.

---

## Acknowledgments
- Special shoutout to the NESdev community for providing excellent resources and test ROMs.
- Inspired the Mesen emulator.