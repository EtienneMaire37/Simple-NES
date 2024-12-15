# Simple-NES

SimpleNES is a simple, work in progress nes emulator. It is designed to run classic NES games and is built with simplicity and portability in mind.

<p align="center">
<img src="https://img.shields.io/github/commit-activity/m/EtienneMaire37/simple-nes
" />
<img src="https://img.shields.io/github/contributors/EtienneMaire37/simple-nes
" />
<img src="https://img.shields.io/github/last-commit/EtienneMaire37/simple-nes
" />
</p>

---

# Features

## Basics
- **CPU Emulation**: Complete implementation of the RP2A03 (6502) processor, passing all major test ROMs.
- **PPU Rendering**: Accurate rendering of NES graphics.
- **APU Output**: WASAPI powered sound output, only pulse channel 1 is supported for now
- **Cross-Platform Development**: Uses CSFML for graphics and input handling.

## Supported mappers
- ***Mapper 00***:
   - [x] **NROM**
- ***Mapper 01***:
   - [x] **MMC1**
   - [ ] **SxROM**
- ***Mapper 02***: 
   - [x] **UxROM**

---

## Getting Started

### Requirements
To build and run the emulator, ensure you have the following installed:

- **CMake** (version 3.15 or higher)
- **GCC** or **MinGW** (for Windows builds)
- **CSFML** (used for rendering and input handling), included in the `setup.bat` file
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

| Action | Key |
|--------|-----------|
| Controller **A** | **C** |
| Controller **B** | **X** |
| Controller **D-pad** | **Arrow keys** |
| Controller **Start** | **LShift** |
| Controller **Select** | **LControl** |
| **Pause/Resume emulation** | **Space** |
| **Change palette** | **Ctrl+P** |
| **Reset** | **Ctrl+R** |

## Screenshots

![Super Mario Bros screenshot](./screenshots/smb1.png)

![Galaga screenshot](./screenshots/galaga.png)

---

## Contributing
Contributions are welcome! Feel free to fork the repository, make improvements, and submit a pull request. Suggestions and bug reports can be submitted to [Issues](https://github.com/EtienneMaire37/simple-nes/issues).

---

## License
This project is licensed under the MIT License. See the `LICENSE` file for details.

---

## Acknowledgments
- Special shoutout to the NESdev community for their great wiki and test ROMs.
- Inspired by the Mesen emulator.