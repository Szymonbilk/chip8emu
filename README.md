# chip8emu
A simple Chip8 emulator implemented in C.
It currently only supports the regular Chip8, and quirks are geared towards modern implementations

## Dependencies
Requires SDL3 for input and output

## Build
Enusre you have SDL3 installed on you system
In the project root, run the following two commands:
```
cmake -S . -B build
cmake --build build
```
The binary can then be found as `build/chip8emu`

## Usage
`chip8emu path/to/rom`
Simple run the binary with the path to any ROM file
The keypad is mapped to the left side of the keyboard, i.e. 123,QWE,ASD,ZXC
