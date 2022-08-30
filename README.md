```
 ________  ________   ________          ________  _______   ________  ________  _______   ________     
|\   __  \|\   ___  \|\   ____\        |\   __  \|\  ___ \ |\   __  \|\   ___ \|\  ___ \ |\   __  \    
\ \  \|\  \ \  \\ \  \ \  \___|        \ \  \|\  \ \   __/|\ \  \|\  \ \  \_|\ \ \   __/|\ \  \|\  \   
 \ \   ____\ \  \\ \  \ \  \  ___       \ \   _  _\ \  \_|/_\ \   __  \ \  \ \\ \ \  \_|/_\ \   _  _\  
  \ \  \___|\ \  \\ \  \ \  \|\  \       \ \  \\  \\ \  \_|\ \ \  \ \  \ \  \_\\ \ \  \_|\ \ \  \\  \| 
   \ \__\    \ \__\\ \__\ \_______\       \ \__\\ _\\ \_______\ \__\ \__\ \_______\ \_______\ \__\\ _\ 
    \|__|     \|__| \|__|\|_______|        \|__|\|__|\|_______|\|__|\|__|\|_______|\|_______|\|__|\|__| 
```
## About
A modern C++ png reader from spec. Mainly done as a fun learning challenge.

## Requirements
* CMake 3.20+
* C++23 (I wanted to use c++23's byteswap for handling endianness)

## Build instructions
There are no dependencies so building is straightforward (I hope).
  - Options: TRV_MULTITHREADED ON/OFF enables my attempt at multithreading the defiltering process. I haven't found it to be effective.

## Usage
Include "Image.h" and use the load_image function to load an image into memory. The template specifies the desired output data type.

# Sources
PNG Spec: http://www.libpng.org/pub/png/spec/1.2/
Zlib Spec: https://www.ietf.org/rfc/rfc1950.txt
